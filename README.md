# my_scanf — Custom scanf Implementation
**Author:** Naomi Beck

## Description

This project implements `my_scanf`, a custom version of the C `scanf` function.  
The implementation manually parses format strings, handles width and length modifiers, processes variadic arguments, and reads input using a custom buffering mechanism.

The program supports all required conversion specifiers and modifiers for the assignment and includes **three custom extensions**.

---

## Supported Conversions

- `%c` — character input  
- `%s` — string input  
- `%d` — signed decimal integer  
- `%x` — hexadecimal integer  
- `%f` — floating-point value  

### Modifiers

- **Width modifiers** (e.g. `%4s`, `%3c`, `%2x`)
- **Length modifiers**: `h`, `l`, `ll`, `L`  
  - Examples: `%d`, `%ld`, `%lld`, `%f`, `%lf`, `%Lf`

---

## Custom Extensions (3)

- `%q` — reads quoted text (`"hello world"` → `hello world`)  
- `%b` — reads a binary number and converts it to an integer  
- `%r` — reads the rest of the current line (until newline)

---

## Example Test Case

The following test demonstrates all three custom extensions used together.

**Code**
```c
char q[64], r[128];
unsigned int b;

int n = my_scanf("%q %b %r", q, &b, r);
printf("n=%d\nq=[%s]\nb=%u\nr=[%s]\n", n, q, b, r);
```

Input (stdin):
"quoted text" 1011 rest of line here

Output (stdout):
n=3
q=[quoted text]
b=11
r=[rest of line here]