# C++ Parser

Wrench includes a special-purpose C++ parser which is used for reading data type definitions for pvars.

## Enablement

This parser can be turned on using the following line in a header file:

```
#pragma wrench parser on
```

And can be turned off with the following line:

```
#pragma wrench parser off
```

## Language Feature Support

All code between these two delimiters must conform to the limitations of the parser. The following language features are supported:

- C-style structs and unions.
- Basic built-in types: `char`, `unsigned char`, `signed char`, `short`, `unsigned short`, `int`, `unsigned int`, `long`, `unsigned long`, `long long`, `unsigned long long`, `float`, `double`, `bool` and `void`.
- Pointers, references and C arrays.

A (probably incomplete) list of language features that are currently unsupported is provided below:

- Anything that isn't a type.
- Most C++-specific features such as inheritance, member functions, templates, etc.
- Comma-separated lists of variables.
- Function pointers.

The pvar data stored in the instances file conforms to the PS2 ABI.
