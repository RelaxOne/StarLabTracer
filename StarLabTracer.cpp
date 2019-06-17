#define DEBUG_TYPE "StarLabTracer"

#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/DebugInfo.h"

#include <cstdlib>
#include <cstdio>
#include <vector>
#include <stdbool.h>
#include <fstream>
#include <string>

#define StarLab_Load_Global 0
#define StarLab_Load_Not_Global 1
#define StarLab_Store_Global 2
#define StarLab_Store_Not_Global 3

#define StarLab_Malloc 4
#define StarLab_Realloc 5
#define StarLab_Calloc 6
#define StarLab_New 7
#define StarLab_New_Array 8


using namespace llvm;

namespace {
    struct  StarLabTracer : public FunctionPass{
        static char ID;
        StarLabTracer() : FunctionPass(ID){}
        
        virtual bool runOnFunction(Function &F){
            Module *M = F.getParent();
            /* 若当前方法是 main 方法，则在 main 方法的前面添加一条call 指令初始化函数 */
            std::string funcName = F.getName().str();
            if(funcName == "main"){
                Function *StarLab_Log_init = cast<Function>(M->getOrInsertFunction("StarLab_init", Type::getVoidTy(M->getContext()), NULL));
                IRBuilder<> IRB(F.front().getFirstInsertionPt());
                IRB.CreateCall(StarLab_Log_init);
            }
            IntegerType *Type_Int64 = Type::getInt64Ty(M->getContext());
            
            for(Function::iterator basicblock = F.begin(); basicblock != F.end(); basicblock++){
                for(BasicBlock::iterator instruction = basicblock->begin(); instruction != basicblock->end(); instruction++){      
                    // 获取当前指令在源码中的行号
                    int lineNo = StarLab_getLineNo(instruction);
                    
                    if(isa<CallInst>(instruction)){
                        Function *StarLab_log_Call = cast<Function>(
                                M->getOrInsertFunction("StarLab_call", Type::getVoidTy(M->getContext()), Type_Int64, Type_Int64, Type_Int64, Type_Int64, NULL));
                        StarLab_handleCallInstruction(instruction, lineNo, StarLab_log_Call);
                    }else if (isa<AllocaInst>(instruction)) { 
                        Function *StarLab_log_Alloca = cast<Function>(
                            M->getOrInsertFunction("StarLab_alloca", Type::getVoidTy(M->getContext()), Type_Int64, Type_Int64, NULL));
                        StarLab_handleAllocaInstruction(instruction, lineNo, StarLab_log_Alloca);
                    }else{
                        Function *StarLab_log_Load_and_Store = cast<Function>(
                            M->getOrInsertFunction("StarLab_load_and_store", Type::getVoidTy(M->getContext()), Type_Int64, Type_Int64, Type_Int64, Type_Int64, NULL));
                        StarLab_handleLoadAndStoreInstruction(instruction, lineNo, StarLab_log_Load_and_Store);
                    } 
                }
            }
            return true;
        }

         /* 
          * 处理调用系统的指令 
          * 输出数据解析：
          *     type:  表示是何种函数申请的内存地址空间
          *     len :  所申请内存地址的长度
          *     addr:  申请地址空间首地址
          *     line:  源码的行号
          */
        void StarLab_handleCallInstruction(Instruction *instruction, int lineNo, Function *function){
            
            CallInst *CI = dyn_cast<CallInst>(instruction);
            Function *callFunction = CI->getCalledFunction();
            if(!callFunction){
                return;
            }
            std::string funcName = callFunction->getName().str();
            TerminatorInst *TInst = instruction->getParent()->getTerminator();
            IRBuilder<> IRB(TInst);
            Value *line = ConstantInt::get(IRB.getInt64Ty(), lineNo);
            if(funcName == "malloc"){
                Value *type = ConstantInt::get(IRB.getInt64Ty(), StarLab_Malloc);
                Value *len = instruction->getOperand(0);
                Value *addr = IRB.CreatePtrToInt(instruction, IRB.getInt64Ty());
                Value *args[] = {type, len, addr, line};
                IRB.CreateCall(function, args);
            }else if(funcName == "realloc"){
                Value *type = ConstantInt::get(IRB.getInt64Ty(), StarLab_Realloc);
                Value *len = instruction->getOperand(1);
                Value *addr = IRB.CreatePtrToInt(instruction, IRB.getInt64Ty());
                Value *args[] = {type, len, addr, line};
                IRB.CreateCall(function, args);
            }else if(funcName == "calloc"){
                Value *type = ConstantInt::get(IRB.getInt64Ty(), StarLab_Calloc);
                Value *len = IRB.CreateMul(instruction->getOperand(0), instruction->getOperand(1));
                Value *addr = IRB.CreatePtrToInt(instruction, IRB.getInt64Ty());
                Value *args[] = {type, len, addr, line};
                IRB.CreateCall(function, args);
            }else if(funcName == "_Znam"){
                Value *type = ConstantInt::get(IRB.getInt64Ty(), StarLab_New_Array);
                Value *len = instruction->getOperand(0);
                Value *addr = IRB.CreatePtrToInt(instruction, IRB.getInt64Ty());
                Value *args[] = {type, len, addr, line};
                IRB.CreateCall(function, args);
	        }else if(funcName == "_Znwm"){
		        Value *type = ConstantInt::get(IRB.getInt64Ty(), StarLab_New);
                Value *len = instruction->getOperand(0);
                Value *addr = IRB.CreatePtrToInt(instruction, IRB.getInt64Ty());
                Value *args[] = {type, len, addr, line};
                IRB.CreateCall(function, args);
	        }
        }

         /* 
          * 处理 llvm 中 Alloca 指令
          * 输出数据解析：
          *     type:  表示是 LLVM Alloca 指令产生的地址
          *     len :  所申请内存地址的长度
          *     addr:  申请地址空间首地址
          */
        void StarLab_handleAllocaInstruction(Instruction *instruction, int lineNo, Function *function){
            TerminatorInst *TInst = instruction->getParent()->getTerminator();
            IRBuilder<> IRB(TInst);
            Value *len = ConstantInt::get(IRB.getInt64Ty(), StarLab_getLengthOfData(instruction));
            Value *address = IRB.CreatePtrToInt(instruction, IRB.getInt64Ty());
            Value *args[] = {len, address};
            IRB.CreateCall(function, args);
        }

         /* 
          * 处理 Load / Store 指令 
          * 输出数据解析：
          *     type:  表示是 Load / Store 指令
          *     len :  Load / Store 地址的长度
          *     addr:  Load / Store 指令的地址
          *     line:  源码的行号
          */
        void StarLab_handleLoadAndStoreInstruction(Instruction *instruction, int lineNo, Function *function){
            TerminatorInst *TInst = instruction->getParent()->getTerminator();
            IRBuilder <>IRB(TInst);
            Value *type;
            Value *line = ConstantInt::get(IRB.getInt64Ty(), lineNo);
            if(instruction->getOpcode() == Instruction::Load){
                LoadInst *load = dyn_cast<LoadInst>(instruction);
                if(!load)
                    return;
                Value *pointer = load->getPointerOperand();
                if(isa<GlobalVariable>(pointer)){
                    type = ConstantInt::get(IRB.getInt64Ty(), StarLab_Load_Global);
                }else{
                    type = ConstantInt::get(IRB.getInt64Ty(), StarLab_Load_Not_Global);
                }
                
                Value *len = ConstantInt::get(IRB.getInt64Ty(), 64);
                Value *address = IRB.CreatePtrToInt(instruction->getOperand(0), IRB.getInt64Ty());
                Value *args[] = {type, len, address, line};
                IRB.CreateCall(function, args);
            }else if(instruction->getOpcode() == Instruction::Store){
                StoreInst *store = dyn_cast<StoreInst>(instruction);
                if(!store)
                    return;
                Value *pointer = store->getPointerOperand();
                if(isa<GlobalVariable>(pointer)){
                    type = ConstantInt::get(IRB.getInt64Ty(), StarLab_Store_Global);
                }else{
                    type = ConstantInt::get(IRB.getInt64Ty(), StarLab_Store_Not_Global);
                }
                Value *len = ConstantInt::get(IRB.getInt64Ty(), 64);
                Value *address = IRB.CreatePtrToInt(instruction->getOperand(1), IRB.getInt64Ty());
                Value *args[] = {type, len, address, line};
                IRB.CreateCall(function, args);
            }
        }

        /* 获取一条指令对应源码的行号 */
        int StarLab_getLineNo(Instruction *instruction){
            if (MDNode *N = instruction->getMetadata("dbg")) {
                DILocation Loc(N);
                std::string file = Loc.getFilename();
                std::string dir = Loc.getDirectory();
                return Loc.getLineNumber();
            }
            return -1;
        }

        /* 获取数据的长度 */
        int StarLab_getLengthOfData(Instruction *instruction){
            Type *type = instruction->getType();
            int size = 0;
            if (type->isPointerTy())
                return 8 * 8;
            else if (type->isFunctionTy())
                size = 0;
            else if (type->isFloatingPointTy()) {
                switch (type->getTypeID()) {
                    case llvm::Type::HalfTyID:
                        size = 16;
                        break;
                    case llvm::Type::FloatTyID:
                        size = 4 * 8;
                        break;
                    case llvm::Type::DoubleTyID:
                        size = 8 * 8;
                        break;
                    case llvm::Type::X86_FP80TyID:  
                        size = 10 * 8;
                        break;
                    case llvm::Type::FP128TyID:
                        size = 16 * 8;
                        break;
                    case llvm::Type::PPC_FP128TyID:
                        size = 16 * 8;
                        break;
                    default:
                        errs() << "[ERROR]: Unknown floating point type " << *type << "\n";
                        assert(false);
                }
            } else if (type->isIntegerTy()) {
                size = cast<IntegerType>(type)->getBitWidth();
            } else if (type->isVectorTy()) {
                size = cast<VectorType>(type)->getBitWidth();
            } else if (type->isArrayTy()) {
                ArrayType *A = dyn_cast<ArrayType>(type);
                size = (int)A->getNumElements() * A->getElementType()->getPrimitiveSizeInBits();
            } else {
                errs() << "[ERROR]: Unknown data type " << *type << "\n";
                assert(false);
            }
            return size;
        }
    };
}

char StarLabTracer::ID = 0;
static RegisterPass<StarLabTracer> X("starlab-tracer", "StarLab Tracer");
