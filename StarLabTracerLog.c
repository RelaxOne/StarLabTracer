#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#define TRACE_FILE_LOAD_AND_STORE "trace.txt"
#define TRACE_FILE_CALL "call.txt"
#define TRACE_FILE_ALLOCA "alloca.txt"

/* 函数声明 */
void StarLab_load_and_store(int type, uint64_t length, uint64_t address, int line);
void StarLab_call(int type, uint64_t length, uint64_t address, int line);
void StarLab_alloca(uint64_t address, int line);
bool StarLab_Open_File();
void StarLab_close_file();
void StarLab_init();

/* 全局变量 */
bool init = false;
char *content;
FILE *fp_alloca = NULL;
FILE *fp_call = NULL;
FILE *fp_load_and_store = NULL;


/* 通过 load/store 指令得到的 trace 记录 */
void StarLab_load_and_store(int type, uint64_t length, uint64_t address, int line) {
    assert(init == true);
    sprintf(content,"%d, %ld, %lx, %d\n", type, length, address, line);
    fputs(content, fp_load_and_store);
}

/* 通过系统函数分配的堆上的内存空间 */
void StarLab_call(int type, uint64_t length, uint64_t address, int line){
    assert(init == true);
    sprintf(content,"%d, %ld, %lx, %d\n", type, length, address, line);
    fputs(content, fp_call);
}

/* 在栈上分配的局部变量*/
void StarLab_alloca(uint64_t address, int line){
    assert(init == true);
    sprintf(content,"%lx, %d\n", address, line);
    fputs(content, fp_alloca);
}

/* 打开文件并申请一个内存空间 */
bool StarLab_Open_File(){
    // 申请一个固定的内存空间
    content = (char*)malloc(256);
    /* 打开三个文件 */
    fp_alloca = fopen(TRACE_FILE_ALLOCA, "a+");
    if(fp_alloca == NULL)
        return false;
    fp_call = fopen(TRACE_FILE_CALL, "a+");
    if(fp_call == NULL)
        return false;
    fp_load_and_store = fopen(TRACE_FILE_LOAD_AND_STORE, "a+");
    if(fp_load_and_store == NULL)
        return false;
    init = true;
    return init;
}

/* 释放内存空间并关闭相关文件 */
void StarLab_close_file(){
    free(content);
    fclose(fp_alloca);
    fclose(fp_call);
    fclose(fp_load_and_store);
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