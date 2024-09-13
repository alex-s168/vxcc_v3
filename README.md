http://vxcc.vxcc.dev/

# building
use the `./build.sh`

## deps 
`clang`: different C compiler can be used by doing `CC=someothercc ./build.sh [build|test]` (`test` builds the project and then runs all tests)

`python3`: it is recommended to create a venv in the repository root using `python -m venv venv`

## include into projects
Link with `build/lib.a` and `build/x86.a`
