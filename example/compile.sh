#!/bin/sh

# 指定编译好的 LLVM 目录
export LLVM_HOME=/home/nvm/llvm-3.4/llvm_build

# 将 trace 记录收集文件编译成 ll 中间文件
clang -g -O3 -S -emit-llvm -o StarLabTracerLog.ll StarLabTracerLog.c

# 循环将所有的 cpp/c/cc 文件（去除 StarLabTracerLog.c 文件）
for file in $(ls *.cpp *.c | egrep -v StarLabTracerLog.c)
do
    # 获取查询到的文件的文件名（去除后缀）
    filename=$(echo $file | cut -d . -f1)

    # 将所有 cpp 文件编译成 ll 中间文件
    clang++ -g -O0 -S -emit-llvm -o ${filename}.ll ${file}

    # 对中间文件进行插桩操作，即插入收集 trace 的目标函数
    opt -load ${LLVM_HOME}/lib/LLVMStarLabTracer.so -starlab-tracer -S ${filename}.ll -o  ${filename}-opt.ll
done

# 链接所有的 -opt.ll 文件和 输出函数 StarLabTracerLog.c 编译完的 ll 文件
llvm-link -o full.ll *-opt.ll StarLabTracerLog.ll

llc -filetype=asm -o full.s full.ll

# 联合输出文件生成可执行二进制文件
g++ -fno-inline -o full_trace full.s

# 运行可执行文件
./full_trace