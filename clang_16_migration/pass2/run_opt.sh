#!/bin/bash


#might be useful
#https://stackoverflow.com/questions/67206238/how-to-define-and-read-cli-arguments-for-an-llvm-pass-with-the-new-pass-manager


clang-16 -S -emit-llvm -O0 -Xclang -disable-O0-optnone ../../testing/test.c

#opt-16 -S --load-pass-plugin libclambcc/libclambcc.so --passes="my-module-pass,my-function-pass" test.ll -o test.t.ll

opt-16 -S --load-pass-plugin libclambcc/libclambcc.so --passes=my-module-pass test.ll -o test.t.ll
#opt-16 -S --load-pass-plugin libclambcc/libclambcc.so --passes=my-function-pass test.ll -o test.t.ll


