"Library" uno.cc represents the application as a whole ("uno" is the name of the project)
It is separated from main.cc to enable testing of uno.cc
main.cc generally does not lend itself to being conveniently tested via ctest

uno.cc is the top-level file tying together all modules that make up project uno
All other parts are "sub-modules" of uno ("module" here does not refer to C++20 modules)
Instead "module" here refers to a what "simple_module" function in SimpleModule.cmake creates
Essentially it is just a src/**/*.cc file

By convention submodules of module X are located under X/ folder, so uno::util is in src/uno/util.cc
Because we use "src/" as our include folder we can write "uno/util.hh"
This is taken to mean "interface of module uno::util"

Files like uno/util-test.cc do white box test for uno/util.cc
Experimentally in this project uno/util-test.cc includes verbatim the whole of uno/util.cc
This enables us to easily test parts of uno/util.cc which are not "exposed" in uno/util.hh

It is interesting how this pattern can be improved upon once C++20 modules become more mainstream
Alternatively we could add a secondary header "uno/util-test.hh" documenting private interface
between uno/util-test.cc and uno/util.cc
