#include "llvm/Pass.h"
#include "llvm/Demangle/Demangle.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"
#include <fstream>
#include <iostream>
#include <llvm-14/llvm/IR/BasicBlock.h>
#include <llvm-14/llvm/IR/Constants.h>
#include <llvm-14/llvm/IR/IRBuilder.h>
#include <llvm-14/llvm/IR/LLVMContext.h>
#include <llvm-14/llvm/Support/Casting.h>
#include <llvm-14/llvm/Support/raw_ostream.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>
#include <ostream>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

////////////////////////////
//_________BOJE___________//
#define COL_NME "\x1B[31m" //
#define COL_CNT "\x1B[32m" //
#define COL_BBV "\x1B[33m" //
#define COL_IOF "\x1B[34m" //
#define COL_SWI "\x1B[36m" //
#define COL_DBG "\x1B[41m" //
#define COL_CLR "\x1B[0m"  //
////////////////////////////

///////////////
//__Logging__//
//__DEBUG___3//
//__INFO____2//
//__ERR_____1//
///////////////
#define LOG_LEVEL (4)

#define LOG(message, level)                                                    \
  if (level <= LOG_LEVEL) {                                                    \
    outs() << message << "\n";                                                 \
  } else if (level == 5) {                                                     \
    outs() << COL_DBG << message << COL_CLR << "\n";                           \
  }

using namespace llvm;

namespace {
class SwitchToBranchPass : public ModulePass,
                           public InstVisitor<SwitchToBranchPass> {
public:
  std::vector<SwitchInst *> switches;
  static char ID;

  SwitchToBranchPass() : ModulePass(ID) {}

  bool runOnModule(Module &M) override {
    visit(M);
    return true;
  }

  void visitSwitchInst(SwitchInst &SI) { switches.push_back(&SI); }

  void removeSwitches() {
    for (int i = 0; i < switches.size(); i++) {
      switches[i]->removeFromParent();
    }
  }

  void transformSwitches(Module *module) {
    for (auto &function : module->functions()) {
      for (auto &basicBlock : function) {
        for (auto &instruction : basicBlock) {
          if (auto *switchInst =
                  llvm::dyn_cast<llvm::SwitchInst>(&instruction)) {
            llvm::Value *condition = switchInst->getCondition();
            llvm::BasicBlock *defaultBlock = switchInst->getDefaultDest();
            std::vector<llvm::BasicBlock *> caseBlocks;
            std::vector<llvm::ConstantInt *> caseValues;

            // Collect the case blocks and values
            for (auto &caseIt : switchInst->cases()) {
              caseBlocks.push_back(caseIt.getCaseSuccessor());
              caseValues.push_back(caseIt.getCaseValue());
            }

            // Create the new basic blocks
            llvm::BasicBlock *mergeBlock = llvm::BasicBlock::Create(
                module->getContext(), "merge", &function);
            std::vector<llvm::BasicBlock *> branchBlocks(
                caseBlocks.size(),
                llvm::BasicBlock::Create(module->getContext(), "branch",
                                         &function));

            // Create the branch instructions
            llvm::IRBuilder<> builder(switchInst);
            for (size_t i = 0; i < caseBlocks.size(); i++) {
              llvm::Value *cmp = builder.CreateICmpEQ(condition, caseValues[i]);
              builder.CreateCondBr(cmp, caseBlocks[i], branchBlocks[i]);
            }

            // Create the default branch
            builder.SetInsertPoint(switchInst);
            builder.CreateBr(defaultBlock);

            // Remove the original switch instruction
           /*  switchInst->eraseFromParent(); */

            // Update the branch blocks to point to the merge block
            for (auto *branchBlock : branchBlocks) {
              builder.SetInsertPoint(branchBlock);
              builder.CreateBr(mergeBlock);
            }

            // Set the insert point to the merge block
            builder.SetInsertPoint(mergeBlock);
          }
        }
      }
    }
  }
  bool doFinalization(Module &M) override {
    // Perform finalization operations after all functions have been processed
    LOG(COL_NME << "========================" << COL_CLR, 2);
    LOG(switches.size() << " switch statements left to branch out", 5);
    transformSwitches(&M);
    removeSwitches();
    writeModuleToFile(&M, "switchless.ll");
    return true;
  }

  void writeModuleToFile(llvm::Module *module, const std::string &filename) {
    // Open the file for writing.
    std::error_code EC;
    llvm::raw_fd_ostream outputFile(filename, EC);

    if (EC) {
      llvm::errs() << "Error opening file: " << EC.message() << "\n";
      return;
    }

    // Write the LLVM IR to the file.
    module->print(outputFile, nullptr);

    // Flush and close the file.
    outputFile.flush();
    outputFile.close();

    if (outputFile.has_error()) {
      llvm::errs() << "Error writing to file: " << EC.message() << "\n";
    }
  }
};
} // end anonymous namespace

char SwitchToBranchPass::ID = 0;
static RegisterPass<SwitchToBranchPass> X("switchtoifelse",
                                          "Switch to Branch Pass");
