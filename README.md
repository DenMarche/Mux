# mux

A string encryption library for C++.

Unlike XOR-based obfuscators, mux uses ciphers (RC4, XTEA, Salsa, ChaCha) whose round counts and rotation constants are rotated per string, making Mux not pattern-scannable by design.

## Usage

```cpp
#include "include/mux.hpp"

int main() {
    std::puts(MUX_S("Hello, world")); // As simple as that ;)
}
```

## Tradeoffs

Slower and bigger than XOR.

## License

Opache
