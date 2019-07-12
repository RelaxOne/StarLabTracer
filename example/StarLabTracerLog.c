#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#define FILE_TRACE "trace.txt"
#define LENGTH 256

/* 函数声明 */
void StarLab_trace(int type, uint64_t length, uint64_t address, int line);
bool StarLab_Open_File();
void StarLab_close_file();
void StarLab_init();

/* 全局变量 */
bool init = false;
char *content;
FILE *fp_trace = NULL;

/* 通过 load/store 指令得到的 trace 记录 */
void StarLab_trace(int type, uint64_t length, uint64_t address, int line) {
    assert(init == true);
    memset(content, 0, LENGTH);
    sprintf(content,"%d %ld %lx %d\n", type, length, address, line);
    fputs(content, fp_trace);
}

/* 打开文件并申请一个内存空间 */
bool StarLab_Open_File(){
    // 申请一个固定的内存空间
    content = (char*)malloc(LENGTH);
    fp_trace = fopen(FILE_TRACE, "a+");
    if(fp_trace == NULL)
        return false;
    init = true;
    return init;
}

/* 释放内存空间并关闭相关文件 */
void StarLab_close_file(){
    free(content);
    fclose(fp_trace);
    init = false;
}

void StarLab_init(){
    if(!init){
        if(!StarLab_Open_File()){
            printf("init file error...\n");
        }
    }
    atexit(&StarLab_close_file);
}