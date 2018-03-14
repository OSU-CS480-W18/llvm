#include <iostream>

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
// #include "llvm/Analysis/Verifier.h"
#include "llvm/IR/Value.h"

llvm::LLVMContext TheContext;
llvm::IRBuilder<> TheBuilder(TheContext);
llvm::Module* TheModule;


llvm::Value* numericalConstant(float val) {
  return llvm::ConstantFP::get(TheContext, llvm::APFloat(val));
}


llvm::Value* binaryOperation(llvm::Value* lhs, llvm::Value* rhs, char op) {
  if (!lhs || !rhs) {
    return NULL;
  }
  switch (op) {
    case '+':
      return TheBuilder.CreateFAdd(lhs, rhs, "addtmp");
    case '-':
      return TheBuilder.CreateFSub(lhs, rhs, "subtmp");
    case '*':
      return TheBuilder.CreateFMul(lhs, rhs, "multmp");
    case '/':
      return TheBuilder.CreateFDiv(lhs, rhs, "divtmp");
    default:
      std::cerr << "Invalid operator: " << op << std::endl;
      return NULL;
  }
}

int main(int argc, char const *argv[]) {
  TheModule = new llvm::Module("LLVM_demo", TheContext);

  llvm::FunctionType* mainFnType = llvm::FunctionType::get(
    llvm::Type::getVoidTy(TheContext), false
  );
  llvm::Function* mainFn = llvm::Function::Create(
    mainFnType, llvm::GlobalValue::InternalLinkage,
    "main", TheModule
  );
  llvm::BasicBlock* entryBlock = llvm::BasicBlock::Create(
    TheContext, "entry", mainFn
  );
  TheBuilder.SetInsertPoint(entryBlock);

  llvm::Value* expr1 = binaryOperation(
    numericalConstant(4),
    numericalConstant(2),
    '*'
  );
  llvm::Value* expr2 = binaryOperation(
    numericalConstant(8),
    expr1,
    '+'
  );

  TheBuilder.CreateRetVoid();
  llvm::verifyFunction(*mainFn);
  TheModule->print(llvm::outs(), NULL);

  delete TheModule;
  return 0;
}
