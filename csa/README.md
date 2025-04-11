### `README.md`

# Beacon Payload Development Environment

This folder contains everything needed to **build a CSA-compliant application** for the Beacon OS.

## Getting Started

Before you begin, make sure all required tools are installed:

```bash
./install_packages.sh
```

This script installs:

- `gcc-multilib`
- `make`
- `binutils`
- `python3`
- `pv`
- `socat`

These are required to compile and send your payload to Beacon.

---

## Folder Contents

- `link.ld` – Linker script set to load the binary at `0x200000`
- `makefile` – Builds the payload and outputs a CSA-wrapped binary
- `script.py` – Python script that wraps your binary with the CSA header
- `syscall.h` – The full(ish) Beacon syscall table definition

---

## Usage Guidelines

### Code Organization

**Important:** Your application’s entry point must be in `main.c`.  
Due to how the CSA loader operates, **only the `main()` function in `main.c` is executed directly**.

All other functions, variables, and logic should be declared in separate `.c` and `.h` files and linked in via the `makefile`.

### Syscall Usage

The syscall table is loaded at memory address `0x200000`.  
Access it like this:

```c
#include "syscall.h"

void main(void) {
    syscall_table_t* sys = *(syscall_table_t**)0x200000;
    sys->print("Hello from Beacon!\n");
}
```

You’ll find all available functions in `syscall.h`.  
These include basic IO, cursor control, screen rendering, sound, random numbers, and string operations.

---

## Sending to Beacon via QEMU (Serial Example)

To send the final `.csa` payload to Beacon over serial using QEMU:

```bash
pv -L 1000 ./program.csa | socat -u - /dev/pts/3
```

Adjust the `pts` device path as needed depending on your host machine or virtual device mapping.

---

## Building

To build the project and produce `program.csa`:

```bash
make clean && make
```

This will:
- Compile all source files
- Link them using `link.ld`
- Output a raw `payload.bin`
- Generate `program.csa` with the CSA header applied

---

## Credits

CSA format tools, and examples created by tuvalutorture

Beacon created by Turrnut

Licensed under the GPL v3 License

---

For more information about Beacon or the CSA loader, please refer to the relevant documentation or source repositories.