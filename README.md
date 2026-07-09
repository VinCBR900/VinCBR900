# A Stack of Tiny BASICs

This space contains Tiny BASIC interpreters for for several late 1970s CPUs, along with assembler and C simulator used to build and test them. Tiny BASICs are minimal BASIC interpreters from the dawn of home computing — see the original 1976 [Dr. Dobb's Journal Vol. 1](https://archive.org/details/dr_dobbs_journal_vol_01) for the history.

I first came across Tiny BASIC via single chip micros with the [Zilog Z8671](https://hc-ddr.hucki.net/wiki/lib/exe/fetch.php/einplatinenrechner/z8671_app_note.pdf) in the late 1980s and built an [Intel 8052AHBasic](https://www.bitsavers.org/components/intel/8051/MCS_BASIC-52/270010-003_MCS_BASIC-52_Users_Manual_Nov1986.pdf) toy system in early 1990s, and was fascinated in BASIC functionality in tiny ROM.  However I never found a 2 KB Tiny BASIC for 6502 that would fit comfortably in a 2716 EPROM (Apple 1 Integer BASIC was 4kbyte, see below).

As per Dr Dobbs, the classic implementation approach was to use an Intermediate Language (IL) between the host CPU and the BASIC parser, which eased porting but cost speed. A hand-assembled 6502 IL version was written by Tom Pittman and is documented at [ittybittycomputers.com](http://www.ittybittycomputers.com/IttyBitty/TinyBasic/index.htm). 

Writing a non-IL Tiny BASIC like [Li Chen's 2kbyte 8080 Palo Alto Tiny BASIC](https://archive.org/details/Palo_Alto_Tiny_BASIC_Version_3_Li-Chen_Wang_1977) myself seemed daunting, until in 2020 I came across [x86 BootBASIC](https://github.com/nanochess/bootBASIC) by Oscar Toledo, which sparked the idea of a short, doable direct (non-IL) 6502 version, but after trying it was obvious my 6502 skills were just not up to it.  

Time inevitably passed, then recently [Anthropic made a press release where Claude developed A C compiler itself](https://www.anthropic.com/engineering/building-c-compiler), so I thought I'd give it a try on Tiny BASIC.  With significant help from [Claude AI](https://claude.ai)  the 65C02 uBASIC Tiny BASIC emerged. This went so well (in part because Claude had a double off-peak usage promo) I decided to expand to two other projects I saw when young - the obscure Signetics 2650 (popular in Australia) and the Intel 8088 I saw in Byte magazine a long time ago.  The original sequence plan was MOS 6502, Signetics 2650 then Intel 8088, but the 2650 version was a struggle as the instruction set architecture is ... different.   

You can play with the Signetics 2650, MOS 6502 and Intel 8088 versions online at the Links below -Thanks to [8Bitworkshop](https://8bitworkshop.com/) for the IDE versions

[MOS 6502 Tiny BASIC](http://8bitworkshop.com/v3.12.1/?redir.html?platform=verilog&githubURL=https%3A%2F%2Fgithub.com%2FVinCBR900%2Fmango_one&file=mango1.v)

[Intel 8088 Tiny BASIC](http://8bitworkshop.com/v3.12.1/?redir.html?platform=x86&githubURL=https%3A%2F%2Fgithub.com%2FVinCBR900%2F8086-Tiny-BASIC&file=uBASIC8088.asm)

[Signetics 2650 Tiny BASIC](https://vincbr900.github.io/2650-Tiny-BASIC/)

---

## Comparison: Dr Dobbs Tiny BASIC, Apple 1 4K BASIC vs These 

This section compares multiple interpreters from the same tradition: 
  - The original Tiny BASIC specification published in Dr. Dobb's Journal (1975–1976),
  - These 6502/65C02 2KB uBASICs,
  - Apple 1 BASIC written by Steve Wozniak (1976),
  - This 65c02 4KB BASIC,
  - This 2KB uBASIC8088,
  - This 4KB uBASIC for Signetics 2650

### Background

**Original Tiny BASIC** was not a single implementation but a language specification published in the People's Computer Company newsletter in September 1975 by Dennis Allison, then elaborated in the first issues of Dr. Dobb's Journal of Computer Calisthenics and Orthodontia (January 1976 onwards). The journal was launched specifically on the back of reader enthusiasm for Tiny BASIC. The canonical feature set is defined by the BNF grammar published with the specification: `PRINT`, `IF…THEN`, `GOTO`, `INPUT`, `LET`, `GOSUB`, `RETURN`, `CLEAR`, `LIST`, `RUN`, `END` — and nothing else. No `FOR`, no `REM`, no `DATA`, no functions beyond what the expression grammar provides. Li-Chen Wang's Palo Alto Tiny BASIC (DDJ Vol 1 No 5, May 1976) is the most celebrated implementation, fitting in 1.77 KB for the 8080.

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

| Feature | Original Tiny BASIC (spec) | uBASIC 6502 | Apple 1 BASIC | 65C02 4K Tiny BASIC | uBASIC 8088 | uBASIC 2650 |
|---------|---------------------------|-------------------------------|-----------------------------|-----------------------------|-------------------|-------------------|
| **Size** | Spec only | <2 KByte ROM | 4096 bytes (cassette) | 4093 bytes ROM | 2 KByte ROM | 4 KByte ROM |
| **CPU target** | N/A | NMOS 6502 | NMOS 6502 | 65C02 | Intel 8088 | Signetics 2650 |
| **Tokenised** | ✗ (raw ASCII) | ✗ 2 byte match | ✓ | ✓ | ✓ | ✗ 2 byte match | 
| **Variable Type** | Signed 16-bit INT|Signed 16-bit INT|Signed 16-bit INT|Signed 16-bit INT|Signed 16-bit INT|Signed 16-bit INT|
| **Variables** | A–Z | A–Z | A–Z, An (letter+digit) | A–Z | A–Z | A–Z |
| **Integer arrays / DIM** | ✗ | ✗ | `DIM A(n)` | ✗ | ✗ | ✗ |
| **Strings** | ✗ (`PRINT "str"`) | ✗ | ✓ (char arrays, `DIM A$(n)`) | ✗ (`PRINT "str"`) |✗ (`PRINT "str"`) |✗ (`PRINT "str"`) |
| **Multi-statement `:`** | ✗ | ✓ | ✓ | ✓ | ✓ | ✗ | 
| **PRINT `;` no-newline** | ✗ | ✓ | ✓ | ✓ | ✓ | ✓ |
| **INPUT** | ✓ | ✓ | ✓ (With Prompt) | ✓ | ✓ | ✓ |
| **LET** | ✓ (required) | ✓ (optional) | ✓ (optional) | ✓ (optional) | ✓ (optional) | ✓ (optional) |
| **IF/THEN** | ✓ (line number or stmt) | ✓ | ✓ (stmt or line number) | ✓ | ✓ | ✓ |
| **ELSE** | ✗ | ✗ | ✗ | ✓ | ✗ | ✗ |
| **GOTO expression** | ✓ (computed) | ✓ (computed) | ✓ (computed) | ✓ (computed) | ✓ (computed) | ✓ (computed) |
| **GOSUB expression** | ✓ (computed) | ✗ | ✓ (computed) | ✓ (computed) | ✓ (computed) | ✗ |
| **GOSUB/RETURN** | ✓ | ✗ | ✓ | ✓ | ✓ | ✓ |
| **GOSUB nesting depth** | n/a | n/a | 8 max | 8 | 8 | 7 |
| **FOR/NEXT/STEP** | ✗ | ✗ | ✓ | ✓ | ✓ | ✓ (runs once) | 
| **FOR nesting depth** | n/a | n/a | 8 | 8 | 8 | 8 |
| **DATA/READ/RESTORE** | ✗ | ✗ | ✗ | ✓ | ✗ | ✗ |
| **REM** | ✗ | ✓ | ✗ | ✓ | ✓ | ✓ |
| **END** | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| **CLEAR / NEW** | `CLEAR` | `NEW` | `NEW` | `NEW` | `NEW` | `NEW` |
| **RUN / LIST** | ✓ | ✓ (Optional `start,end` | ✓ | ✓ | ✓ (Optional `start,end`) | ✓ (Optional `start,end` |
| **PEEK / POKE** | ✗ | ✓ | ✓ | ✓ | ✓ and `IN`/`OUT`| ✓ |
| **Machine Langauge** | ✗ | `USR(addr)` (Tail call, return in ZP $00)  | `CALL addr` | `USR(addr)` (Tail call, returns ZP $00) | `USR(addr)` (CALL, return in AX) | `USR(addr)` (Tail Call, return in R0) |
| **Math Functions** | ✗ | `ABS` | `ABS` | `ABS` `SGN` `SIN` `COS`| `ABS` | `ABS` `NEG`|  
| **RND** | ✗ | ✓ `RND` → 1..32767 | ✓ `RND(n)` → 0..n-1 | ✓ `RND` → 1..32767 | ✓ `RND(n)` → -n..n | ✓ `RND(n)` → 0..n |
| **Character Conv** | ✗ | `CHR$` | ✗ | `ASC` `CHR$` | `CHR$` | `CHR$` |
| **MOD / %** | ✗ | ✓ `%` | ✗ | ✓ both | ✓ `%` | ✓ `%` |
| **Bitwise Ops** | ✗ | ✗ | ✓ `AND` `OR` `NOT` | ✓ `AND` `OR` `NOT` `XOR` | ✓ `&` `\|` `NOT(val)`| ✓ `AND` `OR` `NOT` `XOR` |
| **Relational ops** | `<` `>` `=` `<=` `>=` `<>` | ✓ | ✓ (also `#` for `<>`) | ✓ | ✓ | ✓ |
| **`PRINT` Cursor positioning** | ✗ | ✓ `TAB(spaces)`| ✗ (dumb terminal only) | ✓ `TAB(spaces)` | ✓ `TAB(spaces)`| ✓ `TAB(spaces)`|
| **Memory Query** | ✗ | ✓ `PRINT FREE` | ✓ `HIMEM=` / `LOMEM=` | ✓ `FREE`| ✓ `FREE`| ✓ `FREE` |
| **keyword list** `HELP` | ✗ | ✗ | ✗ | ✓ | ✓ | ✗ |
| **Line number range** | 1–32767 | 0–32767 | 0–32767 | 0–32767 | 1–32767 | 1–32767 |

#### Notes on each column

**Original Tiny BASIC specification** (DDJ Vol 1, 1975–1976). The spec is intentionally minimal — Dennis Allison's goal was a BASIC small enough to fit in 4 KB of RAM on an 8080 with room left for programs. `IF` does not have `ELSE`. There is no `REM`, no `FOR`, no `DATA`, no functions. `GOSUB` and computed `GOTO` are present from the start. The single-array `@(i)` was added by some implementations (notably Palo Alto Tiny BASIC) but is not in the base specification. String literals appear only as arguments to `PRINT`.

**uBASIC 6502/65c02** (~2 KB, this project). In spirit to the original Tiny BASIC spec. Adds `REM`, `%` (modulo), `CHR$(n)`, `USR(addr)`, `PEEK`/`POKE`, bitwise operators, and `FREE`/  Omits `FOR`, `GOSUB`/`RETURN` to stay within 2 KB. Does not tokenise: programs are stored and interpreted as raw ASCII, which costs some speed but saves tokeniser code. Loops and subroutines are implemented with `GOTO` and variable-based state, as in the classic Tiny BASIC tradition.

**Apple 1 BASIC** (~4 KB, Wozniak 1976). Fills its 4 KB cassette image with considerably more than the Tiny BASIC spec. Tokenised for speed; Wozniak noted it outperformed Microsoft BASIC on benchmarks of the day. Adds `FOR`/`NEXT`, integer arrays, character-array strings with `DIM`, `ABS`, `RND`, `AND`/`OR`/`NOT`, `CALL`, `AUTO`, and `HIMEM=`/`LOMEM=`. The `IF` condition uses a value of 1 for true (not just non-zero) which differs from most BASICs. Notably absent: `DATA`/`READ`, `REM`, `ELSE`, `ON…GOTO`, `MOD`, `CHR$`, `ASC`, `SGN`. The Apple 1 had no graphics hardware and no cursor positioning — just a raw serial output, so there is no `HOME`, `TAB`, `VTAB`, `PLOT`, `GR`, etc. at all (those came with the Apple II port). Program execution stops if any key is pressed, which made it easy to accidentally interrupt a running program.

**4K BASIC 65c02** (~4 KB, this project). This was a test expanding 2kbyte Tiny BASIC for the 6502 in the Kowalski emulator: tokenised, includes `FOR`/`NEXT`, `GOSUB`/`RETURN`, `DATA`/`READ`/`RESTORE`, `ON n GOTO/GOSUB`, `ELSE`, `SGN`, `ABS`, `RND`, `ASC`, `CHR$`, `MOD`/`%`, `XOR`, `INKEY`, `CLS`, and `AT(col,row)` cursor control, still no arrays arrays or strings. In all honesty, if you are looking for a modern 6502 BASIC and arent restricting yourself to small ROMs,  then [EhBASIC](https://github.com/picocomputer/ehbasic) is hard to beat. 

**uBASIC 8088** (~2 KB, this project). Ported from 65C02 uBASIC, but due to the intrinsec signed 16 bit instruction set provides ROM space for extra functionality: `DELAY`, `FOR..TO..[STEP]`/`NEXT`, `GOSUB`/`RETURN`, `IN`/`OUT`, optional (start, end) for `LIST`, `TAB(n)` in addition to `CHR$` in `PRINT`.  Keywords are mostly tokenized to save RAM space.  

**uBASIC 2650** (~4 KB, this project). In spirit to the original Tiny BASIC spec. Ported from 6502 version with  `REM`, `CHR$(n)`.  Omits `FOR`, `GOSUB`/`RETURN` to stay within 4 KB. Does not tokenise but only parses 2 bytes of each command. Loops and subroutines are implemented with `GOTO` and variable-based state, as in the classic Tiny BASIC tradition.  This has been the most difficult to develop and has fewer features for more memory. 

#### What Apple 1 BASIC has that non of my Tiny BASIC variants provide

- **Integer arrays.** `DIM A(20)` allocates a numeric array. Wozniak's original game programs used arrays heavily for board state.
- **Character-array strings.** `DIM A$(20)` plus slice indexing `A$(2,5)` — an HP BASIC style approach that avoids the overhead of a string heap. There is no equivalent in any Tiny BASIC without significant added code.
- **`AUTO` line numbering.** Prompt with incrementing line numbers — saves typing.
- **`HIMEM=` / `LOMEM=`.** Direct control of the program/variable memory boundaries. Useful when BASIC shares memory with machine code.
- **Cassette LOAD/SAVE.** Via the Apple Cassette Interface — entirely hardware-specific. Nowdays Little point for these toy systems apart from educational.

#### What these Tiny BASIC variants have that Apple 1 BASIC doesn't

- **`MOD` / `%`** (not 2650). Apple 1 BASIC has no modulo; programmers used `A - (A/B)*B`.
- **`REM`** (all). Apple 1 BASIC has no comment statement at all.
- **`AND` / `OR` / `XOR` / `NOT`** (65C02 4K BASIC, uBASIC 8088).
- **`CHR$(n)` / `ASC(c)`** (2650/6502/8088 uBASIC CHR$ only, 65C02 4K BASIC all three). Useful for character-based I/O.
- **Print Formatting - AT(ROW,COL), TAB(n)** (65C02 4K BASIC has `AT`, uBASIC2650/uBASIC8088 has `TAB(n)` after `PRINT`. The Apple 1's dumb terminal did not allow.

6502 4k BASIC Only
- **`DATA` / `READ` / `RESTORE`** - Wozniak deliberately omitted these as unnecessary for game programming. 4K BASIC includes full support; look-up tables and static sequences are much more convenient with `DATA`.
- **`ON n GOTO` / `ON n GOSUB`** - Multi-way computed dispatch without needing a computed `GOTO` expression and careful arithmetic.
- **`ELSE`** - Apple 1 BASIC's `IF` has no else branch; a second `IF NOT (...)` line is needed.
- **`INKEY`** - Non-blocking key read. Apple 1 BASIC stops execution on any keypress, making non-blocking input impossible.
- **`CLS`** - clear screen

### Size perspective

| Platform | Interpreter | Year | Size | 
|----------|-------------|------|------|
| any | Original Tiny BASIC spec | 1975 | — (spec) |
| 8080 | Palo Alto Tiny BASIC v1 (Li-Chen Wang) | 1976 | 1.77 KB |
| 6502 | Apple 1 BASIC (Wozniak) | 1976 | 4.0 KB | 
| 6502/65c02 | uBASIC (this project) | 2026 | 2.0 KB | 
| 65c02 | 4K BASIC (this project) | 2026 | 4.0 KB | 
| 8088 | uBASIC (this project) | 2026 | 2.0 KB | 
| 2650 | uBASIC (this project) | 2026 | 4.0 KB | 


Apple 1 BASIC and 4K BASIC both occupy 4 KB, yet spend that budget in distinctly different ways. Wozniak used much of the space on arrays, strings, and `RND`; the 4K BASIC uses the same space for `DATA`/`READ`, `ELSE`, `ON…GOTO`, `SGN`, `ASC`/`CHR$`, `INKEY`, `CLS`, and cursor control — features more useful on a modern embedded target than array support. Apple 1 BASIC had the original 6502, while these Tiny BASICs uses the 65C02's extra instructions (`STZ`, `BRA`, zero-page indirect addressing) which were not available to Wozniak in 1976.

The signetics 2650 is an extinct CPU with powerful feature like register incrementing, indexing, indrect pointers and conditional returns, but limited by the 8 deep harware stack.  Additionally, most Branch instructions take 3 bytes as the relative range is only +/- 63 bytes. 

---

## Credits & Similar Projects

- **Oscar Toledo** for [x86 BootBASIC](https://github.com/nanochess/bootBASIC) — my original inspiration for a non-IL Tiny BASIC approach.
- **Will Stevens'** [1kbyte 8080 Tiny BASIC](https://github.com/WillStevens/basic1K) was also a more recent inspiration and taught me a few old skool tricks on code density. 
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
