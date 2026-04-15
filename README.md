# Various Tiny BASICs

This repository contains Tiny BASIC interpreters for for several late 1970s CPUs, along with assembler and C simulator used to build and test them. Tiny BASICs are minimal BASIC interpreters from the dawn of home computing — see the original 1976 [Dr. Dobb's Journal Vol. 1](https://archive.org/details/dr_dobbs_journal_vol_01) for the history.

As per Dr Dobbs, the classic approach was to use an Intermediate Language (IL) between the host CPU and the BASIC parser, which eased porting but cost speed. A hand-assembled 6502 IL version was written by Tom Pittman and is documented at [ittybittycomputers.com](http://www.ittybittycomputers.com/IttyBitty/TinyBasic/index.htm). 

I first came across Tiny BASIC single chip micros with the [Zilog Z8671](https://hc-ddr.hucki.net/wiki/lib/exe/fetch.php/einplatinenrechner/z8671_app_note.pdf) in the late 1980s and built an [Intel 8052AHBasic](https://www.bitsavers.org/components/intel/8051/MCS_BASIC-52/270010-003_MCS_BASIC-52_Users_Manual_Nov1986.pdf) toy system in early 1990s, and was fascinated in BASIC functionality in tiny ROM.  However I never found a 2 KB Tiny BASIC that would fit comfortably in a 2716 EPROM (Apple 1 Integer BASIC was 4kbyte, see below).

Writing a non-IL Tiny BASIC like [Li Chen's 2kbyte 8080 Palo Alto Tiny BASIC](https://archive.org/details/Palo_Alto_Tiny_BASIC_Version_3_Li-Chen_Wang_1977) myself seemed daunting, until I came across [x86 BootBASIC](https://github.com/nanochess/bootBASIC) by Oscar Toledo, which sparked the idea of a short, doable direct (non-IL) 6502 version, but after trying it was obvious my 65c02 were just not up to it.  

Time inevitably passed, then recently [Anthropic made a press release where Claude developed A C compiler itself](https://www.anthropic.com/engineering/building-c-compiler), so I thought I'd give it a try on Tiny BASIC.  With significant help from [Claude AI](https://claude.ai), firstly the 6502 Tiny BASIC emeged, then others. See [Using Claude to modify the interpreters](#using-claude-to-modify-the-interpreters) below.

You can play with the 6502 and 8088 version on 8bit workshop

[6502 Tiny BASIC](http://8bitworkshop.com/v3.12.1/?redir.html?platform=verilog&githubURL=https%3A%2F%2Fgithub.com%2FVinCBR900%2Fmango_one&file=mango1.v)

[8088 Tiny BASIC](http://8bitworkshop.com/v3.12.1/?redir.html?platform=x86&githubURL=https%3A%2F%2Fgithub.com%2FVinCBR900%2F8086-Tiny-BASIC&file=uBASIC8088.asm)

---

## Using Claude to Modify the Interpreters

As I'm an Assembly Novice, these interpreters were written collaboratively with [Claude AI](https://claude.ai) — which understood the 65C02 and 8088 instruction set, the space constraints, and the design tradeoffs. The assembler and simulators were also written this way, and together the three tools form a tight workflow that makes the source highly accessible to modification even if you are not an assembly expert. If you are then YMMV.

This may be old news to many people but is included here for those to whom this is new. 

### The workflow

The key insight is that you do not need to understand every Assembly opcode to extend these interpreters. The workflow for 6502 is below, others similar:

1. **Create a 'Project' in Claude**, Upload ASM65c02.c, SIM65c02.c to the files section, add rules like below, and in the chat window upload the ASM version you want to modify and tell it to review.
```asm
To avoid wasting tokens, provide brief, terse updates during debugging and resume normal verbosity when concluding. 
Always uprev tool versions if you need to modify them by updating the header file and update the trace file.
To avoid using wrong version, Copy old source version to an archive folder and only use them for regression testing.
```
3. **Describe what you want** to Claude in plain English — a new statement, a new operator, a bug fix, or a size optimisation.
4. **Claude proposes the assembly change** with full explanation of what it is doing and whether the tools need updating.
5. **Tell Claude to implement and Test with the simulator** - If it is interrupted **Dont't Click Retry** but type "continue from Trace file" and it should.
6. **When complete, Paste the modified source** into the Kowalski simulator or Interactive Sim to cross check:
   ```bash
   ./sim65c02_interactive uBASIC.asm "
   ```

### Things to watch out for

- **Fall-through chains.** Several functions share a single RTS by falling through into the next function. These are clearly marked in the source. Inserting code between them without understanding the fall-through will break things — tell Claude to watch out for them.

---

## Comparison: Original Tiny BASIC, uBASIC, Apple 1 BASIC, and 4K BASIC

This section compares four interpreters from the same tradition: the original Tiny BASIC specification published in Dr. Dobb's Journal (1975–1976), these 6502/65C02 2KB uBASICs, Apple 1 BASIC written by Steve Wozniak (1976), andthis  65c02 4KB BASIC.

### Background

**Original Tiny BASIC** was not a single implementation but a language specification published in the People's Computer Company newsletter in September 1975 by Dennis Allison, then elaborated in the first issues of Dr. Dobb's Journal of Computer Calisthenics and Orthodontia (January 1976 onwards). The journal was launched specifically on the back of reader enthusiasm for Tiny BASIC. The canonical feature set is defined by the BNF grammar published with the specification: `PRINT`, `IF…THEN`, `GOTO`, `INPUT`, `LET`, `GOSUB`, `RETURN`, `CLEAR`, `LIST`, `RUN`, `END` — and nothing else. No `FOR`, no `REM`, no `DATA`, no functions beyond what the expression grammar provides. Li-Chen Wang's Palo Alto Tiny BASIC (DDJ Vol 1 No 5, May 1976) is the most celebrated implementation, fitting in 1.77 KB for the 8080.

**Apple 1 BASIC** was written by Steve Wozniak — a hardware engineer who had not written a BASIC interpreter before — as a cassette-tape program for the Apple 1 computer in 1976. It occupied exactly 4 KB of RAM (loaded at `$E000`–`$EFFF`). Wozniak was a member of the Homebrew Computer Club alongside Wang and other Tiny BASIC authors. The Apple 1 BASIC manual is the primary source for its feature set; unlike the Apple II version, the Apple 1 had no lo-res graphics hardware at all, no paddles, and no screen-positioning capability — just a dumb serial terminal interface. The interpreter evolved through at least four versions (A–D) during 1976, with version A lacking even `INPUT`. The table below reflects the final/stable version D that shipped with the cassette. It is tokenised, works in signed 16-bit integers, supports arrays (`DIM`), strings as character arrays, `FOR`/`NEXT`, `GOSUB`/`RETURN`, and `GOTO` to expressions. Notably absent: `DATA`/`READ`, `ELSE`, `ON…GOTO`, `REM`, `MOD`, `RND`, and any screen control.

### Original Tiny BASIC specification grammar

From the BNF published in Dr. Dobb's Journal Vol 1 (1975–1976):

```
line       ::= number statement CR | statement CR
statement  ::= PRINT expr-list
             | IF expression relop expression THEN statement
             | GOTO expression
             | INPUT var-list
             | LET var = expression
             | GOSUB expression
             | RETURN
             | CLEAR
             | LIST
             | RUN
             | END
expr-list  ::= (string | expression) (, (string | expression))*
var-list   ::= var (, var)*
expression ::= (+|-|ε) term ((+|-) term)*
term       ::= factor ((*|/) factor)*
factor     ::= var | number | (expression)
var        ::= A | B | C ... | Z
number     ::= digit digit*
relop      ::= < (>|=|ε) | > (<|=|ε) | =
```

Key points: variables are single letters A–Z only (no arrays, no strings). Numbers are unsigned in the grammar but implementations typically used signed 16-bit. `IF` has no `ELSE`. `GOTO`/`GOSUB` take expressions (computed jumps were part of the spec from the start). String literals can appear in `PRINT` lists. There is no `REM`, no `FOR`, no `DATA`, no `RND`, no `PEEK`/`POKE`, no `ABS`.

### Feature comparison table

| Feature | Original Tiny BASIC (spec) | uBASIC / uBASIC6502 (~2 KB) | Apple 1 BASIC (~4 KB, 6502) | 4K BASIC (~4 KB, 6502) |
|---------|---------------------------|-------------------------------|-----------------------------|-----------------------------|
| **Size** | Spec only | uBASIC: ~2 KB, uBASIC6502: 2006 bytes | 4096 bytes (cassette) | 4093 bytes (ROM) |
| **CPU target** | N/A | uBASIC: 65C02, uBASIC6502: NMOS 6502 | 6502 | 65C02 |
| **Tokenised** | ✗ (most impls raw ASCII) | ✗ (raw ASCII) | ✓ | ✓ |
| **Integer only** | ✓ signed 16-bit | ✓ signed 16-bit | ✓ signed 16-bit | ✓ signed 16-bit |
| **Variables** | A–Z | A–Z | A–Z, An (letter+digit) | A–Z |
| **Integer arrays / DIM** | ✗ | ✗ | `DIM A(n)` | ✗ |
| **Strings** | ✗ (literals in PRINT only) | ✗ | ✓ (char arrays, `DIM A$(n)`) | ✗ (literals in PRINT only) |
| **Multi-statement `:`** | ✗ | ✓ | ✓ | ✓ |
| **PRINT `;` no-newline** | ✗ | ✓ | ✓ | ✓ |
| **PRINT string literal** | ✓ | ✓ | ✓ | ✓ |
| **INPUT** | ✓ | ✓ | ✓ (With Prompt) | ✓ |
| **LET** | ✓ (required) | ✓ (optional) | ✓ (optional) | ✓ (optional) |
| **IF/THEN** | ✓ (line number or stmt) | ✓ | ✓ (stmt or line number) | ✓ |
| **ELSE** | ✗ | ✗ | ✗ | ✓ |
| **GOTO expression** | ✓ (computed) | ✗ (literal only) | ✓ (computed) | ✓ (computed) |
| **GOSUB expression** | ✓ (computed) | ✗ | ✓ (computed) | ✓ (computed) |
| **RETURN** | ✓ | ✗ | ✓ | ✓ |
| **ON n GOTO/GOSUB** | ✗ | ✗ | ✗ | ✓ |
| **FOR/NEXT/STEP** | ✗ | ✗ | ✓ (up to 8 nested) | ✓ |
| **DATA/READ/RESTORE** | ✗ | ✗ | ✗ | ✓ |
| **REM** | ✗ | ✓ | ✗ | ✓ |
| **END** | ✓ | ✓ | ✓ | ✓ |
| **CLEAR / NEW** | `CLEAR` | `NEW` | `NEW` | `NEW` |
| **RUN / LIST** | ✓ | ✓ | ✓ | ✓ |
| **PEEK / POKE** | ✗ | ✓ | ✓ | ✓ |
| **Machine Langauge** | ✗ | `USR(addr)` (JSR, returns A)  | `CALL addr` (JSR, no retval) | `USR(addr)` (JSR, returns A) |
| **Arithmetic Ops** | ✗ | ✗ | `ABS` | `ABS` `SGN` |
| **RND** | ✗ | ✗ | ✓ `RND(n)` → 0..n-1 | ✓ `RND` → 1..32767 |
| **Character Conv** | ✗ | `CHR$` | ✗ | `ASC` `CHR$` |
| **LEN(str)** | ✗ | ✗ | ✓ (on DIM'd strings) | ✗ |
| **MOD / %** | ✗ | ✓ `%` | ✗ | ✓ both |
| **Logical Ops** | ✗ | ✗ | ✓ bitwise `AND` `OR` `NOT`) | ✓ bitwise `AND` `OR` `NOT` `XOR` |
| **Relational ops** | `<` `>` `=` `<=` `>=` `<>` | ✓ | ✓ (also `#` for `<>`) | ✓ |
| **INKEY (non-blocking)** | ✗ | ✗ | ✗ | ✓ |
| **CLS / HOME (clear screen)** | ✗ | ✗ | ✗ | ✓ `CLS` |
| **Cursor positioning** | ✗ | ✗ | ✗ (dumb terminal only) | ✓ `AT(col,row)` in PRINT |
| **FREE (memory query)** | ✗ | uBASIC: ✓, uBASIC6502: ✗ | ✓ `HIMEM=` / `LOMEM=` | ✓ |
| **HELP / keyword list** | ✗ | uBASIC: ✓, uBASIC6502: ✗ | ✗ | ✓ |
| **AUTO line numbering** | ✗ | ✗ | ✓ | ✗ |
| **Cassette LOAD/SAVE** | ✗ | ✗ | ✓ (via ACI hardware) | ✗ |
| **GOSUB nesting depth** | impl-dependent | n/a | 8 max | 8 |
| **FOR nesting depth** | n/a | n/a | 8 max | 8 |
| **Line number range** | 1–32767 | 0–32767 | 0–32767 | 0–32767 |

#### Notes on each column

**Original Tiny BASIC specification** (DDJ Vol 1, 1975–1976). The spec is intentionally minimal — Dennis Allison's goal was a BASIC small enough to fit in 4 KB of RAM on an 8080 with room left for programs. `IF` does not have `ELSE`. There is no `REM`, no `FOR`, no `DATA`, no functions. `GOSUB` and computed `GOTO` are present from the start. The single-array `@(i)` was added by some implementations (notably Palo Alto Tiny BASIC) but is not in the base specification. String literals appear only as arguments to `PRINT`.

**uBASIC** (~2 KB, this project). Closest in spirit to the original Tiny BASIC spec. Adds `REM`, `%` (modulo), `CHR$(n)`, `USR(addr)`, `PEEK`/`POKE`, bitwise operators, and `FREE` — all things a bare-metal 6502 programmer needs immediately. Omits `FOR`, `GOSUB`, and computed `GOTO` to stay within 2 KB. Does not tokenise: programs are stored and interpreted as raw ASCII, which costs some speed but saves tokeniser code. Loops and subroutines are implemented with `GOTO` and variable-based state, as in the classic Tiny BASIC tradition.

**Apple 1 BASIC** (~4 KB, Wozniak 1976). Fills its 4 KB cassette image with considerably more than the spec. Tokenised for speed; Wozniak noted it outperformed Microsoft BASIC on benchmarks of the day. Adds `FOR`/`NEXT`, integer arrays, character-array strings with `DIM`, `ABS`, `RND`, `AND`/`OR`/`NOT`, `CALL`, `AUTO`, and `HIMEM=`/`LOMEM=`. The `IF` condition uses a value of 1 for true (not just non-zero) which differs from most BASICs. Notably absent: `DATA`/`READ`, `REM`, `ELSE`, `ON…GOTO`, `MOD`, `CHR$`, `ASC`, `SGN`. The Apple 1 had no graphics hardware and no cursor positioning — just a raw serial output, so there is no `HOME`, `TAB`, `VTAB`, `PLOT`, `GR`, etc. at all (those came with the Apple II port). Program execution stops if any key is pressed, which made it easy to accidentally interrupt a running program.

**4K BASIC** (~4 KB, this project). Takes the same 4 KB budget as Apple 1 BASIC and spends it differently: tokenised, includes `FOR`/`NEXT`, `GOSUB`/`RETURN`, `DATA`/`READ`/`RESTORE`, `ON n GOTO/GOSUB`, `ELSE`, `SGN`, `ABS`, `RND`, `ASC`, `CHR$`, `MOD`/`%`, `XOR`, `INKEY`, `CLS`, and `AT(col,row)` cursor control — while omitting arrays and strings. Uses the 65C02's additional instructions (`STZ`, `BRA`, zero-page indirect) to squeeze more features per byte than was possible on the original 6502.

#### What Apple 1 BASIC has that neither Tiny BASIC variant provides

- **Integer arrays.** `DIM A(20)` allocates a numeric array. Wozniak's original game programs used arrays heavily for board state.
- **Character-array strings.** `DIM A$(20)` plus slice indexing `A$(2,5)` — an HP BASIC style approach that avoids the overhead of a string heap. There is no equivalent in any Tiny BASIC without significant added code.
- **`AUTO` line numbering.** Prompt with incrementing line numbers — saves typing.
- **`HIMEM=` / `LOMEM=`.** Direct control of the program/variable memory boundaries. Useful when BASIC shares memory with machine code.
- **Cassette LOAD/SAVE.** Via the Apple Cassette Interface — entirely hardware-specific but functional.

#### What these Tiny BASIC variants have that Apple 1 BASIC doesn't

- **`DATA` / `READ` / `RESTORE`** (4K BASIC). Wozniak deliberately omitted these as unnecessary for game programming. 4K BASIC includes full support; look-up tables and static sequences are much more convenient with `DATA`.
- **`ON n GOTO` / `ON n GOSUB`** (4K BASIC). Multi-way computed dispatch without needing a computed `GOTO` expression and careful arithmetic.
- **`ELSE`** (4K BASIC). Apple 1 BASIC's `IF` has no else branch; a second `IF NOT (...)` line is needed.
- **`MOD` / `%`** (both). Apple 1 BASIC has no modulo; programmers used `A - (A/B)*B`.
- **`REM`** (uBASIC, 4K BASIC). Apple 1 BASIC has no comment statement at all.
- **`AND` / `OR` / `XOR` / `NOT`** (4K BASIC).
- **`CHR$(n)` / `ASC(c)`** (uBASIC, 4K BASIC). Useful for character-based I/O.
- **`INKEY`** (4K BASIC). Non-blocking key read. Apple 1 BASIC stops execution on any keypress, making non-blocking input impossible.
- **Cursor control** (4K BASIC). `AT(col,row)` in PRINT. The Apple 1's dumb terminal interface made this hardware-impossible.
- **`CLS`** (4K BASIC).

### Size perspective

| Interpreter | Year | Size | Platform |
|-------------|------|------|----------|
| Original Tiny BASIC spec | 1975 | — (spec) | any |
| Palo Alto Tiny BASIC v1 (Li-Chen Wang) | 1976 | 1.77 KB | 8080 |
| Apple 1 BASIC (Wozniak) | 1976 | 4.0 KB | 6502 |
| uBASIC (this project) | 2026 | 2.0 KB | 6502/65C02 |
| 4K BASIC (this project) | 2026 | 4.0 KB | 65C02 |

Apple 1 BASIC and 4K BASIC both occupy 4 KB, yet spend that budget in distinctly different ways. Wozniak used much of the space on arrays, strings, and `RND`; the 4K BASIC uses the same space for `DATA`/`READ`, `ELSE`, `ON…GOTO`, `SGN`, `ASC`/`CHR$`, `INKEY`, `CLS`, and cursor control — features more useful on a modern embedded target than array support. Apple 1 BASIC had the original 6502, while these Tiny BASICs uses the 65C02's extra instructions (`STZ`, `BRA`, zero-page indirect addressing) which were not available to Wozniak in 1976.

uBASIC at 2 KB sits squarely in the original Tiny BASIC tradition: non-tokenised, no `FOR`, no `GOSUB`, immediate and simple. It extends the spec with the minimum needed for a real 6502 system: `PEEK`/`POKE`, `USR()`, `CHR$`, `%`, bitwise operators.

---

## Credits & Similar Projects

- **Oscar Toledo** for [x86 BootBASIC](https://github.com/nanochess/bootBASIC) — my original inspiration for a non-IL Tiny BASIC approach.
- **Will Stevens'** [1kbyte 8080 Tiny BASIC](https://github.com/WillStevens/basic1K) was also a more recent inspiration and taught me a few old skool tricks on code density. 
- **Hans Otten** for a thorough [6502 Tiny BASIC site](http://retro.hansotten.nl/6502-sbc/kim-1-manuals-and-software/kim-1-software/tiny-basic).
- **[Claude AI](https://claude.ai)** for making it possible for a non-expert to ship something that had been on the back burner since 1989.

---

## Licence

Copyright (c) 2026 Vincent Crabtree

**MIT License**

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
