#!/usr/bin/env python3
"""
asmdup.py - v1.1
Copyright Vincent Crabtree 2026 * MIT License see LICENCE

Purpose:
    Find duplicated instruction sequences in assembly source files, to help
    identify candidates for subroutine extraction (size optimization on
    ROM-constrained targets, e.g. 65C02 / 2650). Unlike character/brace-based
    duplicate detectors, this works at instruction-line granularity, which is
    what actually matters for asm dedup.

Usage:
    asmdup.py [options] file1.asm [file2.asm ...]
    cmd | asmdup.py -f            (read file list from stdin, filter by ext)

Options:
    -f              Filter stdin file list by known asm extensions (.asm,.s,.inc)
    -n N            Minimum window length in instruction lines (default 4)
    --fuzzy         Normalize operands (numeric literals, label names) so
                    structurally-identical-but-differently-labeled sequences
                    still match. Default is exact-text match (post comment/ws strip).
    --min-occur N   Minimum occurrence count to report (default 2)
    --json          Emit JSON instead of text report

History:
    v1.0  2026-07-08  Initial version. Sliding-window rolling hash over
                       normalized instruction lines; merges overlapping runs;
                       reports occurrence count + approximate line savings.
                       Known limitation: savings estimate is in LINES, not
                       bytes -- real byte savings depend on final assembled
                       opcode sizes and are not computed here.
    v1.1  2026-07-08  Fixed: overlapping seed windows from the same real
                       duplicated run were reported as many short, redundant
                       groups. Now extends each match forward/backward while
                       all occurrences agree, then dedups groups that
                       converge to the same maximal region, and drops any
                       result whose location set is a strict subset of a
                       longer kept result.
"""

import sys, os, re, json, argparse
from collections import defaultdict

#------------------------------------------------------------------------------
# Args
#------------------------------------------------------------------------------

parser = argparse.ArgumentParser()
parser.add_argument('-f', action='store_true', help='Filter stdin file list by known extensions.')
parser.add_argument('-n', type=int, default=4, help='Minimum window length in instruction lines.')
parser.add_argument('--fuzzy', action='store_true', help='Normalize operands/labels before matching.')
parser.add_argument('--min-occur', type=int, default=2, help='Minimum occurrence count to report.')
parser.add_argument('--json', action='store_true', help='Emit JSON instead of text report.')
args, inputs = parser.parse_known_args()

ASM_EXTS = ('.asm', '.s', '.inc', '.a65', '.a02')

if not inputs:
    inputs = sys.stdin.read().split('\n')
    if args.f:
        inputs = [f for f in inputs if any(f.endswith(e) for e in ASM_EXTS)]

#------------------------------------------------------------------------------
# Line normalization
#------------------------------------------------------------------------------

COMMENT_RE = re.compile(r';.*$')
WS_RE      = re.compile(r'\s+')
NUM_RE     = re.compile(r'\$[0-9A-Fa-f]+|\b\d+\b')
IDENT_RE   = re.compile(r'\b[A-Za-z_][A-Za-z0-9_]*\b')

MNEMONICS = {
    # not exhaustive; used only to decide what counts as "an instruction line"
    # for both 65C02 and 2650-style listings. Anything with a recognizable
    # opcode-like first token after label-stripping is treated as code.
}

def normalize_line(line, fuzzy):
    """
    Purpose: reduce a raw source line to a comparable token for hashing.
    Inputs:  line (str) - raw source line
             fuzzy (bool) - if True, fold numeric literals and identifiers
    Outputs: (kind, text) where kind is 'code'/'blank'/'label'/'directive'
    Clobbers: none (pure function)
    """
    raw = line.rstrip('\n')
    code = COMMENT_RE.sub('', raw).rstrip()
    stripped = code.strip()

    if not stripped:
        return ('blank', '')

    label_only = re.match(r'^[A-Za-z_.$][A-Za-z0-9_.$]*:\s*$', stripped)
    if label_only:
        # pure label line (label: with nothing else on the line)
        return ('label', '')

    if stripped.startswith('.') or stripped.startswith('*') :
        return ('directive', '')

    text = stripped
    if fuzzy:
        text = NUM_RE.sub('#', text)
        # fold operand identifiers (2nd+ token) but keep the mnemonic itself
        parts = WS_RE.split(text, maxsplit=1)
        if len(parts) == 2:
            mnem, operand = parts
            operand = IDENT_RE.sub('@', operand)
            text = mnem + ' ' + operand
    text = WS_RE.sub(' ', text).strip()
    return ('code', text)

#------------------------------------------------------------------------------
# Load + normalize all files into one flat sequence, tracking (file, lineno)
#------------------------------------------------------------------------------

class Line:
    __slots__ = ('file', 'lineno', 'raw', 'norm')
    def __init__(self, file, lineno, raw, norm):
        self.file = file
        self.lineno = lineno
        self.raw = raw
        self.norm = norm

all_lines = []   # only 'code' kind lines, per file (blocks don't cross files)
file_lines = {}  # file -> list of Line (code only)

nfiles = 0
for fn in inputs:
    fn = fn.strip()
    if not fn or not os.path.isfile(fn):
        continue
    nfiles += 1
    lst = []
    try:
        with open(fn, errors='replace') as f:
            for i, raw in enumerate(f, start=1):
                kind, norm = normalize_line(raw, args.fuzzy)
                if kind == 'code':
                    lst.append(Line(fn, i, raw.rstrip('\n'), norm))
    except (OSError, UnicodeDecodeError):
        continue
    file_lines[fn] = lst

if nfiles == 0:
    print('No input files found.', file=sys.stderr)
    sys.exit(1)

#------------------------------------------------------------------------------
# Sliding window hashing (per file; windows never cross file boundaries)
#------------------------------------------------------------------------------

N = args.n
window_map = defaultdict(list)  # key(tuple of norm text) -> list of (file, start_idx)

for fn, lst in file_lines.items():
    L = len(lst)
    if L < N:
        continue
    for i in range(L - N + 1):
        key = tuple(lst[i + k].norm for k in range(N))
        window_map[key].append((fn, i))

#------------------------------------------------------------------------------
# Keep only windows with enough occurrences
#------------------------------------------------------------------------------

dup_windows = {k: v for k, v in window_map.items() if len(v) >= args.min_occur}

#------------------------------------------------------------------------------
# Extend each group's occurrences into a MAXIMAL duplicate run.
#
# Purpose: a raw N-line window match is just a seed. Overlapping windows
#          (start i, i+1, i+2...) that lie inside the same true duplicated
#          region will each produce their own seed group -- if left as-is
#          that shows up as many short, redundant, overlapping reports for
#          what is really one longer duplicated block. To fix this we grow
#          each seed group forward and backward, one line at a time, for as
#          long as ALL occurrences in the group still agree on the next/prev
#          normalized line. This yields the true maximal run per group.
#          Groups that started as different overlapping seeds of the same
#          real duplication converge to the same (file,start,length) set
#          after extension, so we dedup on that afterward.
# Inputs:  dup_windows (key -> list[(file,start_idx)]), file_lines, N
# Outputs: list of {'locs': [(file,start,length)]} maximal, deduped runs
# Clobbers: none (pure)
#------------------------------------------------------------------------------

def extend_group(locs, N):
    """
    Purpose: grow a seed set of equal-length matching windows outward
             (forward then backward) while all occurrences keep agreeing.
    Inputs:  locs - list of (file, start_idx), all denoting windows of the
                    same initial length N with identical normalized content
             N    - seed window length
    Outputs: (start_offset, length) - amount extended backward, and final
             length; caller applies same offset/length to every location
    Clobbers: none
    """
    length = N
    back = 0

    # extend forward
    while True:
        nxt = []
        ok = True
        for fn, idx in locs:
            lst = file_lines[fn]
            j = idx + length
            if j >= len(lst):
                ok = False; break
            nxt.append(lst[j].norm)
        if not ok or len(set(nxt)) != 1:
            break
        length += 1

    # extend backward
    while True:
        prv = []
        ok = True
        for fn, idx in locs:
            j = idx - back - 1
            if j < 0:
                ok = False; break
            prv.append(file_lines[fn][j].norm)
        if not ok or len(set(prv)) != 1:
            break
        back += 1

    return back, length

raw_groups = []
for key, locs in dup_windows.items():
    locs_sorted = sorted(set(locs))
    back, length = extend_group(locs_sorted, N)
    final_locs = tuple(sorted((fn, idx - back) for fn, idx in locs_sorted))
    raw_groups.append((final_locs, length))

# dedup: many seed keys converge to the same extended (locations, length)
dedup = {}
for final_locs, length in raw_groups:
    dedup[final_locs] = length  # same key => same length by construction

groups = [{'locs': [(fn, idx, length) for fn, idx in locs], 'count': len(locs)}
          for locs, length in dedup.items()]

#------------------------------------------------------------------------------
# Report
#------------------------------------------------------------------------------

def loc_line_range(fn, idx, length):
    lst = file_lines[fn]
    return (lst[idx].lineno, lst[idx + length - 1].lineno)

results = []
for g in groups:
    locs = g['locs']
    if len(locs) < args.min_occur:
        continue
    entries = []
    run_len = locs[0][2]
    for fn, idx, length in locs:
        l0, l1 = loc_line_range(fn, idx, length)
        entries.append({'file': fn, 'line_start': l0, 'line_end': l1, 'lines': length})
    occ = len(entries)
    # approximate savings in LINES: (occ-1) copies removed, each replaced by
    # ~1 call-site line (e.g. JSR), plus one RTS added at the new subroutine.
    savings_lines = (occ - 1) * run_len - occ - 1
    if savings_lines <= 0:
        continue
    results.append({'occurrences': occ, 'run_lines': run_len,
                     'approx_line_savings': savings_lines, 'locations': entries})

# a shorter run that is fully nested (same location set is a superset match
# already covered) can still slip through from different seeds landing on
# sub-runs of a longer duplication with fewer occurrences; drop any result
# whose location set is a subset of another result's location set with a
# greater or equal run length.
def loc_set(r):
    return frozenset((e['file'], e['line_start']) for e in r['locations'])

results.sort(key=lambda r: (-r['run_lines'], -r['occurrences']))
kept = []
for r in results:
    s = loc_set(r)
    if any(s <= loc_set(k) and k['run_lines'] >= r['run_lines'] and k is not r for k in kept):
        continue
    kept.append(r)
results = kept

results.sort(key=lambda r: -r['approx_line_savings'])

if args.json:
    print(json.dumps({'files': nfiles, 'window': N, 'results': results}, indent=2))
    sys.exit(0)

if not results:
    print('No duplicate sequences found (window=%d, min_occur=%d, files=%d).' % (N, args.min_occur, nfiles))
    sys.exit(0)

print('%d duplicate sequence group(s) found (window=%d, min_occur=%d)\n' % (len(results), N, args.min_occur))
for r in results:
    print('--- %d occurrences, %d lines each, ~%d lines saveable ---' %
          (r['occurrences'], r['run_lines'], r['approx_line_savings']))
    for e in r['locations']:
        print('    %s:%d-%d' % (e['file'], e['line_start'], e['line_end']))
    print()
