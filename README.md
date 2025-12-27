# Custom Interpreter: Glide

A simple interpreter implemented in **C**, inspired by  
**_Crafting Interpreters_** by Robert Nystrom.

This project extends the core ideas from the book by adding variable scoping and loop constructs, while retaining a bytecode-based virtual machine architecture.

---

## Features

- **Arithmetic operations**: `+`, `-`, `*`, `/`
- **Unary operations**: `-` (negation)
- **Grouping**: Parentheses for explicit precedence
- **String operations**: concatenation and comparison
- **Variables**:
  - Dynamic typing
  - Variable declaration and usage
  - Lexical scoping
- **Control Flow**:
  - `if` statements
  - `while` loops
  - `for` loops
- **Printing**
- **Debugging support**:
  - Bytecode disassembly
  - Stack trace output (via debug flags)

---

## Implementation Overview

The interpreter closely follows the design patterns described in *Crafting Interpreters*, including:

- A **scanner (lexer)** for tokenizing input source code
- A **bytecode virtual machine** for execution
- **Pratt parsing** (Vaughan Pratt’s top-down operator precedence parser)
- A **stack-based VM** with dynamically typed values
- **Variable declaration and scope management**
- Control-flow constructs implemented at the bytecode level

Additional features such as scoped variables and looping constructs (`while`, `for`) were implemented based on later chapters of the book.

---

## Building and Running

### Prerequisites

- Unix-like environment (Linux or macOS)
- `gcc` or `clang`
- `make`
- `bash`

---

### Build

This project includes a `build.sh` script that handles compilation.

To build:
```bash
./build.sh
```

You're all set! Run the following to execute any Glide code
```bash
./main <file_name.txt>
```

Note: debug flags for assembly and bytecode output can be enabled in utility.h  

Note: If you are getting "permission denied" errors when running ```bash build.sh```, allow permission by running:  
```bash
chmod +x build.sh
```
