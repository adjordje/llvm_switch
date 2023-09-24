#!/bin/bash

# compile the pass
clang++ -fPIC -g -O3 -Wall -Wextra -shared -o kk_pass.so pass.cpp `llvm-config --cppflags --ldflags --libs`
echo "[+] kompajliran LLVM pass"

# emit llvm to llvm ir
clang++ -emit-llvm test.cpp -O0 -S -Xclang -disable-O0-optnone
echo "[+] compiled test.cpp to ir (test.ll)"

# use the pass
opt -S -enable-new-pm=0  -load ./kk_pass.so --switchtoifelse test.ll -o output.ll
echo "[+] outputted pass to output.ll"
echo ""

# diff the files
echo "DIFF PASS:OUT v CUSTOM:OUT =>"
diff output.ll amiout.ll
echo "DIFF DEFAULT:OUT v PASS:DEFAULT =>"
diff test.ll output.ll

