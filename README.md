# ELF Introduction — `hexeditplus` & Binary Patching

A small hex editor written in C (`hexeditplus`), used to inspect 32-bit ELF executables and patch them by hand: fixing a broken entry point, neutering a function with a `ret`, and replacing a buggy compiled function with a corrected one.

Done as Lab 4 of the *Extended System Programming Lab* (ESPL) course.

## Repository contents

| File | Description |
|------|-------------|
| `task1.c` | `hexeditplus` — menu-driven hex editor (load / display / modify / save) |
| `task4.c` | Correct `count_digits(char*)` implementation used as the patch for `ntsc` |
| `makefile` | Builds both programs as 32-bit ELF (`-m32`) |
| `abc` | Sample 32-bit ELF used for exploring headers (Tasks 0–1) |
| `deep_thought` | Executable with a corrupted entry point (Task 2) |
| `offensive` | Executable whose `main` is patched to do nothing (Task 3) |
| `ntsc` | Digit counter with a buggy `count_digits` (Task 4) |
| `ELF_format_file.png`, `ELF_Structure.png` | ELF layout reference diagrams |

## Building

Requires Linux (or WSL) with 32-bit support:

```bash
sudo apt install gcc-multilib   # Debian/Ubuntu
make                            # builds ./task1 (hexeditplus) and ./task4
make clean
```

## `hexeditplus` usage

```bash
./task1
```

```
Choose action:
D - Toggle Debug Mode
F - Set File Name
U - Set Unit Size
L - Load Into Memory
T - Toggle Display Mode
M - Memory Display
S - Save Into File
Y - Memory Modif(y)
Q - Quit
```

Menu letters are case-insensitive. All operations work in units of `unit_size` bytes (1, 2 or 4).

| Option | Input | What it does |
|--------|-------|--------------|
| **D** | — | Toggle debug mode (prints `unit_size`, `file_name`, `mem_count` before each menu, plus per-command details on stderr) |
| **F** | file name | Set the file all file operations use |
| **U** | `1`/`2`/`4` | Set the unit size |
| **L** | `<location hex> <length dec>` | Read `length` units from file offset `location` into the memory buffer |
| **T** | — | Switch display between hexadecimal (default) and decimal |
| **M** | `<address hex> <length dec>` | Print `length` units starting at `address` (`0` = start of the buffer) |
| **S** | `<source hex> <target hex> <length dec>` | Write `length` units from `source` (`0` = start of the buffer) to file offset `target`, without truncating the file |
| **Y** | `<location hex> <val hex>` | Overwrite one unit at buffer offset `location` with `val` |
| **Q** | — | Quit |

Example — read 5 two-byte units from `abc` at offset `0x12F`:

```
U → 2
F → abc
L → 12F 5        Loaded 5 units into memory
T                Decimal display flag now on, decimal representation
M → 0 5          256  0  12032  26988  12130
T                Decimal display flag now off, hexadecimal representation
M → 0 5          0100  0000  2F00  696C  2F62
```

Values are read as little-endian integers, which is why `00 01` in the file shows up as `0x0100`.

## Walkthrough

> Each patch below modifies the binary in place — work on a copy (e.g. `cp ntsc ntsc_patched`) if you want to keep the original.

### Task 0 — `abc`

- The entry point is stored in the ELF header at offset `0x18` (`e_entry`): **`0x080483b0`** (= `_start`).
- `abc` has **29** section headers (`e_shnum`, verify with `readelf -h abc`).

### Task 2 — fixing `deep_thought`

Running it loops forever: the header's entry point is `0x08048464`, which is not `_start`. `readelf -s` shows `_start` at **`0x08048350`** — the function that must run before `main`.

The entry point is a 4-byte field at file offset `0x18`, so:

```
F → deep_thought
U → 4
L → 18 1          (M → 0 1 shows 08048464)
Y → 0 8048350
S → 0 18 1
```

```
$ ./deep_thought
This program now working correctly, you win
```

### Task 3 — neutering `main` in `offensive`

From `readelf -s` / `readelf -S`:

- `main`: address `0x0804841d`, size 23, in section 13 (`.text`)
- `.text`: address `0x08048320`, file offset `0x320`
- file offset of `main` = `0x0804841d − 0x08048320 + 0x320` = **`0x41d`**

Dump the function (`U → 1`, `L → 41d 23`, `M → 0 23`):

```
55 89 E5 83 E4 F0 83 EC 10 C7 04 24 D0 84 04 08 E8 BE FE FF FF C9 C3
```

Replacing the first byte with `ret` (`C3`) makes `main` return immediately (alternatively, fill everything before the final `C3` with `90` NOPs):

```
Y → 0 c3
S → 0 41d 1
```

The program now prints nothing.

### Task 4 — patching `count_digits` in `ntsc`

`./ntsc 0123456789` reports 8 digits instead of 10 — the original function gets the digit range wrong. `task4.c` contains a correct version. Because it is copied byte-for-byte into another binary, it must be self-contained: no calls to other functions, no globals, no absolute addresses, and no larger than the original (93 bytes). Hence the `-fno-pie -fno-stack-protector` flags.

Locations (from `readelf`, after `make`):

| | address | `.text` addr / offset | file offset | size |
|---|---|---|---|---|
| `count_digits` in `task4` | `0x08049166` | `0x08049050` / `0x1050` | `0x1166` | 58 |
| `count_digits` in `ntsc` | `0x0804847d` | `0x08048380` / `0x380` | `0x47d` | 93 |

> The `task4` addresses depend on your compiler version — recompute them with `readelf -s task4` and `readelf -S task4`.

```
U → 1
F → task4
L → 1166 58
F → ntsc
S → 0 47d 58
```

```
$ ./ntsc 0123456789
String contains 10 digits, right???
$ ./ntsc aabbaba123baacca
String contains 3 digits, right???
```

## Useful commands

```bash
readelf -h <file>    # ELF header (entry point, section count)
readelf -S <file>    # section headers (addresses and file offsets)
readelf -s <file>    # symbol table (function addresses and sizes)
objdump -d <file>    # disassembly
hexedit <file>       # inspect / verify bytes
```
