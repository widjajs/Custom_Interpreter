# Custom Interpreter

A simple interpreter implemented in C, inspired by **Crafting Interpreters** by Robert Nystrom.
## Features
- **Arithmetic operations**: `+`, `-`, `*`, `/`
- **Unary operations**: `-` (negation)
- **Grouping**: Parentheses for explicit precedence
- **String operations**: concatenation and comparison
- **Variable Declaration & Use**: dynamically cast variables with scope capability.
- **Debugging**: Includes flags for dissasembly and stack trace
- **Printing**

## Implementation
The evaluator follows the design patterns from *Crafting Interpreters*, including:
- A scanner for tokenizing input
- A "virtual machine" to execute byte instructions
- Vaughan Pratt’s “top-down operator precedence parsing

## Instructions to Run
- Download this repository
- Run make and verify that everything built properly by inspecting the build directory
- Run ./main *<test_file_name>* 
- Debug flags are set in the *utility.h* file
