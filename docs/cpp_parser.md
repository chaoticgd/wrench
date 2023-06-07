# C++ Parser

This parser can on be turned using the following line in a referenced header file:

```
// wrench parser on
```

And can be turned off with the following line:

```
// wrench parser off
```

All code between these two delimiters must conform to the limitations of the parser. The following language features are supported:

- C-style structs and unions.
- Basic built-in types: `char`, `unsigned char`, `signed char`, `short`, `unsigned short`, `int`, `unsigned int`, `long`, `unsigned long`, `long long`, `unsigned long long`, `float`, `double`, `bool`.
- Built-in fixed-width integer types: `s8`, `u8`, `s16`, `u16`, `s32`, `u32`, `s64`, `u64`, `s128`, `u128`.
- Built-in floating point vector types: `vec2`, `vec3`, `vec4`.
- Built-in link types for each type of instance e.g. `mobylink`.
- Pointers, references and C arrays.

A (probably incomplete) list of language features that are currently unsupported is provided below:

- Anything that isn't a type.
- Most C++-specific features such as inheritance, member functions, templates, etc.
- Comma-separated lists of variables.
- Function pointers.

The pvar data stored in the instances file conforms to the PS2 ABI.
