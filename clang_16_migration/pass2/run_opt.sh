#!/bin/bash

clang-16 -S -emit-llvm -O0 -Xclang -disable-O0-optnone ../../testing/test.c

opt-16 -S --load-pass-plugin libclambcc/libclambcc.so --passes=my-function-pass test.ll -o test.t.ll


