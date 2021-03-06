
# QBN - Quick Backend New

A compiler backend inspired by [QBE](https://c9x.me/compile/). Especially the IR is similar
to QBE's one.

_This project is still in a very early state. Code structure will change._

For information on the current state, have a look at the DONE/TODO list.

## Motivation
I created this project because I wasn't happy with existing compiler backends:
- GCC is not easily embeddable
- GCCJIT is embeddable but not suitable for AOT compilation: no sizeof operator, no linking, ...
- LLVM is complex, has a type system that can get in one's way, does not create ssa form automatically, not easily
extensible, among other problems
- QBE has no C-API, the source code is very brief and hard to read and therefore not easily
extensible
- learning and making your own compiler is fun

## Goal
- library, embeddable
- C-API instead of text interface (in contrast to QBE)
- scalable (multi-threadable, maybe even built-in)
- readable code, extensible
- object files as output, not assembly
- feature complete (e.g. volatile/atomic)
- support for stack switching
- x64, arm64
- Linux, Windows, Mac

Optional:
- built-in linking?
- own runtime library? (to be independent from gcc or other compilers)
- endianess? (embedded things use big endian?)
- risc-v, arm, x86

## Building

Requirements: cmake, gcc

```
git clone https://github.com/ssmid/qbn
cd qbn
cmake .
make
./test_qbn
```

## Notes

DONE:
- basic x64/sysv abi compatibility
- compiles hello world
- better api
- better instruction cache
- save callee registers
- save caller registers
- stack alignment
- temporaries
- basic register allocation
- process function parameters

TODO:
- emit jumps
- lower arithmetic instructions (c = add a, b -> t = copy a; t = add b; c = copy t)
- support more instructions
- stack allocation (temporaries, manual)
- why no convention for xmm register save?
- stack arguments
- ssa
- structs
- optimizations!
- varargs
- volatile
- atomic
- windows
- macho (x64)
- arm64
