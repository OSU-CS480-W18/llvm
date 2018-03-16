#include <iostream>
#include <map>

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
// #include "llvm/Analysis/Verifier.h"
#include "llvm/IR/Value.h"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Target/TargetMachine.h"


llvm::LLVMContext TheContext;
llvm::IRBuilder<> TheBuilder(TheContext);
llvm::Module* TheModule;
std::map<std::string, llvm::Value*> TheSymbolTable;


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


llvm::AllocaInst* generateEntryBlockAlloca(const std::string& name) {
  llvm::Function* currFn = TheBuilder.GetInsertBlock()->getParent();
  llvm::IRBuilder<> tmpBuilder(&currFn->getEntryBlock(),
    currFn->getEntryBlock().begin());
  return tmpBuilder.CreateAlloca(
    llvm::Type::getFloatTy(TheContext), 0, name.c_str()
  );
}



llvm::Value* assignmentStatement(std::string lhs, llvm::Value* rhs) {
  if (!rhs) {
    return NULL;
  }
  if (!TheSymbolTable.count(lhs)) {
    TheSymbolTable[lhs] = generateEntryBlockAlloca(lhs);
  }
  return TheBuilder.CreateStore(rhs, TheSymbolTable[lhs]);
}


llvm::Value* variableValue(std::string name) {
  llvm::Value* val = TheSymbolTable[name];
  if (!val) {
    std::cerr << "Unknown variable: " << name << std::endl;
    return NULL;
  }
  return TheBuilder.CreateLoad(val, name.c_str());
}


int main(int argc, char const *argv[]) {
  TheModule = new llvm::Module("LLVM_demo", TheContext);

  llvm::FunctionType* mainFnType = llvm::FunctionType::get(
    llvm::Type::getVoidTy(TheContext), false
  );
  llvm::Function* mainFn = llvm::Function::Create(
    mainFnType, llvm::GlobalValue::ExternalLinkage,
    "foo", TheModule
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
  llvm::Value* assign1 = assignmentStatement("a", expr2);

  llvm::Value* expr3 = binaryOperation(variableValue("a"),
    numericalConstant(4), '/');
  llvm::Value* assign2 = assignmentStatement("b", expr3);

  TheBuilder.CreateRetVoid();


  std::string targetTriple = llvm::sys::getDefaultTargetTriple();

  llvm::InitializeAllTargetInfos();
  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmParsers();
  llvm::InitializeAllAsmPrinters();

  std::string error;
  const llvm::Target* target = llvm::TargetRegistry::lookupTarget(targetTriple, error);
  if (!target) {
    std::cerr << error << std::endl;
    return 1;
  }

  llvm::TargetOptions options;
  llvm::TargetMachine* targetMachine = target->createTargetMachine(
    targetTriple, "generic", "", options,
    llvm::Optional<llvm::Reloc::Model>()
  );

  TheModule->setDataLayout(targetMachine->createDataLayout());
  TheModule->setTargetTriple(targetTriple);

  std::error_code ec;
  llvm::raw_fd_ostream file("foo.o", ec, llvm::sys::fs::F_None);
  if (ec) {
    std::cerr << ec.message() << std::endl;
    return 1;
  }

  llvm::legacy::PassManager pm;
  llvm::TargetMachine::CodeGenFileType type =
    llvm::TargetMachine::CGFT_ObjectFile;
  if (targetMachine->addPassesToEmitFile(pm, file, type)) {
    std::cerr << "" << std::endl;
    return 1;
  }

  pm.run(*TheModule);
  file.close();

  llvm::verifyFunction(*mainFn);
  TheModule->print(llvm::outs(), NULL);

  delete TheModule;
  return 0;
}
