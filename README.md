# Custom Interpreter: Glide

A simple interpreter implemented in **C**, inspired by  
**_Crafting Interpreters_** by Robert Nystrom.

---

## Features

- **Arithmetic operations**: `+`, `-`, `*`, `/`
- **Comparison Operators**: `==`, `!=`, `<`, `<=`, `>`, `>=`
- **Unary operations**: `-` (negation)
- **Grouping**: Parentheses for explicit precedence
- **String operations**: concatenation and comparison
- **Data Types**:
  - Numbers (integers/floats) 
  - Strings
  - Booleans
  - None/Null
- **Variables**:
  - Dynamic typing
  - Variable declaration and usage
  - Global & local variables
  - Lexical scoping
- **Control Flow**:
  - `if` statements
  - `while` loops
  - `for` loops
- **Printing**
- **Debugging support**:
  - Bytecode disassembly
  - Stack trace output (via debug flags)
- **Functions**:
  - First-class functions
  - Closures & lexical scoping
  - Function parameters & arguments
  - Recursion support
  - Native functions
- **Classes & OOP**:
  - Instance methods
  - Instance properties/fields
  - Constructors/initializers(e.g. init())
  - Method binding
  - Inheritance with `super`
- **Runtime Features**:
  - Garbage collection (with stress testing if enabled)
  - String interning
  - Stack-based VM execution
  - Constant pool/value array
- **Development Tools**:
  - File execution mode
  - Error reporting with line numbers
  - Debug & Disassembly mode
  - Runtime error messages

---

## Implementation Overview

The interpreter is inspired by some design patterns described in *Crafting Interpreters*, including:

- A **scanner (lexer)** for tokenizing input source code
- A **bytecode virtual machine** for execution
- **Pratt parsing** (Vaughan Pratt’s top-down operator precedence parser)
- A **stack-based VM** with dynamically typed values
- **Mark & Sweep** garbage colletion

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

Note: If you are getting "permission denied" errors when running `./build.sh`, allow permission by running:  
```bash
chmod +x build.sh
```
