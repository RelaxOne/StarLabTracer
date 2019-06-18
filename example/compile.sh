#!/bin/sh
SOURCE=/home/nvm/llvm-3.4/llvm_build
export LLVM_HOME=/home/nvm/llvm-3.4/llvm_build

clang -g -O3 -S -emit-llvm -o StarLabTracerLog.ll StarLabTracerLog.c

for file in $(ls *.cpp)
do
	clang++ -g -O0 -S -emit-llvm -o ${file%.cpp}.ll ${file}
	opt -load ${SOURCE}/lib/LLVMStarLabTracer.so -starlab-tracer -S ${file%.cpp}.ll -o  ${file%.cpp}-opt.ll
done

echo "llvm-link all llvm file to full.llvm"

llvm-link -o full.ll *-opt.ll StarLabTracerLog.ll

llc -filetype=asm -o full.s full.ll

echo "generate executable file"

g++ -fno-inline -o full_trace full.s
