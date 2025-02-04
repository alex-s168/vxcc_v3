# VXCC
optimizing, small, simple, compiler backend.

VXCC uses SSA IR with block arguments instead of the commonly used phi-nodes.

VXCC is licenced under the [permissive Apache 2.0 license with LLVM Exceptions](LICENCE)

## compilation stages
- frontend generates non-SSA IR
- backend converts it to SSA
- backend applies optimizations
- backend converts it to "LL IR" (non-SSA + labels) and performs some more optimizations
- simple code generation

## goals
- easily modifyable and readable code
- not much special knowladge required
- easy to implement code generation for any architecture
- make it easy for people to learn about compiler backends
- good-ish optimizations
- beat QBE in terms of generated code speed

## **non**-goals
- reach complexity of GCC or even LLVM
- beat GCC or LLVM

## frontends
- [C3](https://c3-lang.org/) compiler [fork](https://github.com/alex-s168/c3c)

## current status
| goal                                    |                                                          | progress       |
| --------------------------------------- | -------------------------------------------------------- | -------------- |
| amd64 codegen                           | ![#33cc33](https://placehold.co/15x15/33cc33/33cc33.png) | done (for now) |
| codegen of basic code involving loops   | ![#33cc33](https://placehold.co/15x15/33cc33/33cc33.png) | done           |
| basic optimizations                     | ![#33cc33](https://placehold.co/15x15/33cc33/33cc33.png) | done           |
| rv32im codegen                          | ![#ff9900](https://placehold.co/15x15/ff9900/ff9900.png) | in progress    |
| proper register allocator & ISEL rework | ![#ff9900](https://placehold.co/15x15/ff9900/cc0000.png) | planned        |
| floating point numbers                  | ![#cc0000](https://placehold.co/15x15/cc0000/cc0000.png) | planned        |

## building
use the `./build.sh build`

## compile dependencies 
one of `clang`, `gcc`, or `tcc`: automatically detected. different C compiler can be used by doing `CC=someothercc ./build.sh [build|test]` (`test` can be executed after `build` to run all tests)

`python3`: it is recommended to create a venv in the repository root using `python -m venv venv`

## including into projects
Link with all files returned by `./build.sh libfiles` (at time of writing, only `build/lib.a`)

## using VXCC in a compiler
this is meant to be extremely easy to do (for the frontend developer)

There is some documentation in `frontend_dev_doc.md`.
You can also uselook at the [C3C fork](https://github.com/alex-s168/c3c) in the `src/compiler/vxcc_*` files.

If you have any questions, you can ask me on Discord (`alex_s168`) or send me an [E-Mail](mailto:alexandernutz68@gmail.com)

## contributing
all contributions are welcome! Don't hesitate to ask me if you have any questions.

please do not currently change anything related to:
- codegen system / isel & regalloc (because I want to re-do it in a specific way)

## BOUNTIES
(if you want to do any of the following, please contact me first for exact details)

| id  | task                                                                                                     | money            | dependencies        |
| --- | -------------------------------------------------------------------------------------------------------- | ---------------- | ------------------- |
|  1. | floats on amd64 using scalar sse ops                                                                     |   100€   (~103$) |                     |
|  2. | refactor number data types to allow non-power-of-2 bit widths, and also add different float types        |    50€   (~ 51$) |                     |
|  3. | lower ints pass. asks backend for supported types. lower arbitary-width integers                         |   100€   (~103$) | depends on 2        |
|  4. | identify passes by names and use S-expr KV maps for config. describe pass pipeline using S-exprs instead |   100€   (~103$) |                     |
|  5. | plugin system (ask me for details)                                                                       | **200€** (~207$) | depends on 4        |
|  6. | lower IEEE 754 floats pass. asks backend for supported types. emulate F16,F32,F64 via what backend has   | **200€** (~207$) | depends on 2, 10    |
|  7. | space-efficient binary (de-)serialization of SNode                                                       |   100€   (~103$) |                     |
|  8. | lower fixed-point floats pass. configurable fixed-point floating point emulation                         |   100€   (~103$) | depends on 2, 9, 10 |
|  9. | lower mul/div pass. lowers {MUL, {U,S}DIV} that are not supported by backend                             |   100€   (~103$) |                     |
| 10. | add ops for more float math operations. add pass that lowers unsupported ones to libm calls              | **200€** (~207$) |                     |
| 11. | loop (and if statement) invariant code movement pass                                                     |    50€   (~ 51$) |                     |

## current optimizations
- variable inlining
- duplicate code removal
- cmoves
- tailcall
- simplify loops and ifs
- simple pattern replacement of bit extract and similar
- compile time constant eval

## code-gen example
input:
```c
fn void strcpy(char* dest, char* src) @export 
{
    for (usz i = 0; src[i] != '\0'; i ++)
        dest[i] = src[i];
}
```
output:
```asm
strcpy:
  xor r11d, r11d
  .l0:
  mov r9b, byte [rsi + 1 * r11]
  test r9b, r9b
  je .l1
  mov byte [rdi + 1 * r11], r9b
  lea r8, [r11 + 1]
  mov r11, r8
  jmp .l0
  .l1:
  ret
```
This will improve in the close future.

## adding a target 
You need to complete these tasks (in any order):
- add your target to every place with the comment `// add target`.
  (you can use `git grep -Fn "// add target"` to find them)
- write a code generator (see ETC.A or X86 backend as reference) and add the file to the `build.c`
- add support for it to frontends?
