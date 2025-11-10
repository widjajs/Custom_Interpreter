# Expression Evaluator

A simple expression evaluator implemented in C, inspired by **Crafting Interpreters** by Robert Nystrom. This project covers parsing and evaluating arithmetic expressions and basic operators. 
This will eventually be expanded to a fully fledged language.

## Features
- **Arithmetic operations**: `+`, `-`, `*`, `/`
- **Unary operations**: `-` (negation)
- **Grouping**: Parentheses for explicit precedence

## Implementation
The evaluator follows the design patterns from *Crafting Interpreters*, including:
- A scanner for tokenizing input
- A "virtual machine" to execute byte instructions
- Vaughan Pratt’s “top-down operator precedence parsing
