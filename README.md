# mux

A string encryption library for C++.

Mux uses ciphers - **RC4, XTEA, Salsa, ChaCha** rather than plain XOR, so the encrypted output is harder to pattern-match or decrypt, *but* the cost is speed.

**The decrypted output can be retreieved by a runtime debugger unless you wipe it with "wipe_all()"! See before ( 1 ) and after ( 2 ).**

<img width="694" height="250" alt="image" src="https://github.com/user-attachments/assets/1300c788-a151-418f-ade7-c4796d31f0ca" />

<img width="875" height="513" alt="image" src="https://github.com/user-attachments/assets/1fe0f324-8fb6-4c49-a412-a1c3dae43467" />


## Usage

```cpp
#include "mux.hpp"

int main() {
    std::puts(MUX_S("Hello, world"));
}

