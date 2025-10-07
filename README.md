# CrimsonCAS
Computer Algebra System for the TI-84 CE.

### What is a Computer Algebra System? / Background
A Computer Algebra System (or CAS, for short) is a program that can manipulate algebraic expressions. They're used mainly in mathematics, but can be used in other fields such as physics & programming languages. The reason why I made CrimsonCAS is that the TI-84 CE doesn't support CAS, and I thought it'd be a fun project to try to do. CrimsonCAS was made in C, and compiled using the [TI-84 CE C Toolchain](https://ce-programming.github.io/toolchain/).
### How do CAS systems work?
First, it gets user input (In CrimsonCAS, it's a string input by the calculator), and then it makes a list of tokens, which is used to make a syntax tree
2*5+5 might be tokenized into `[2][*][5][+][5]`, which then gets parsed into a tree, which might look like:
```
        +
      /  \
    *      5
  /  \
2      5
```
In CrimsonCAS, each node can have one of several types:
```C
typedef enum {
    // LEFT_PAREN & RIGHT_PAREN are only used in the tokenizer; it's not used in any of the actual functions.
    IDENTIFIER=0, // Variables; x, y, z, etc;
    NUMBER=1, // numbers, 1, 2, 3, etc;
    OPERATOR=2, // self explanatory; +, -, /, log, abs, etc;
    LEFT_PAREN=3, // (
    RIGHT_PAREN=4, // )
    NONE=7 // Unknown node type (not assigned unless some error happens.)
} Node_Type;
```
Then, with the tree, CrimsonCAS can do several operations with it.
For doing raw arithmetic, it starts from the bottom, and goes up (this obeying PEMDAS).
For actual algebra, along with user input, it gets one of several equation types:
```C
typedef enum  {
    ARITHMETIC=0, //
    LINEAR=1, //
    QUADRATIC=2, //
    ABS=3, //
    SYSTEMS=4, //
} Equation_Type;
```
Which is used by CrimsonCAS to decide what gets done.
### What can CrimsonCAS actually do?
Well, as you read above, it can do Arithmetic, Linear equations, quadratics, absolute value equations*, and systems.
Examples of problems it can do:
```
2.242*5(243)
2x+51=x+43-7x
7x^2+2x-123=0
abs(7+x)=2
{
5x+2y-9z=2
-2x-7y+18z=18
-4x+10y+2z
} (I wouldn't trust it's systems answers; I didn't even know gaussian elimination when I started making this)
```
### Should you install CrimsonCAS on your calculator?
I wouldn't recommend it; it's extremely buggy. There's probably better CAS programs for the TI-84 CE.


