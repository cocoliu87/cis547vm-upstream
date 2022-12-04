#include "DSEInstrument.h"
#include <iostream>

using namespace llvm;

namespace dse {

static const char *DSE_INIT_FUNCTION_NAME = "__DSE_Init__";
static const char *DSE_ALLOCA_FUNCTION_NAME = "__DSE_Alloca__";
static const char *DSE_STORE_FUNCTION_NAME = "__DSE_Store__";
static const char *DSE_LOAD_FUNCTION_NAME = "__DSE_Load__";
static const char *DSE_CONST_FUNCTION_NAME = "__DSE_Const__";
static const char *DSE_REGISTER_FUNCTION_NAME = "__DSE_Register__";
static const char *DSE_ICMP_FUNCTION_NAME = "__DSE_ICmp__";
static const char *DSE_BRANCH_FUNCTION_NAME = "__DSE_Branch__";
static const char *DSE_BINOP_FUNCTION_NAME = "__DSE_BinOp__";


const char *WHITESPACES = " \t\n\r";
const size_t VARIABLE_PADDED_LEN = 8;

std::string variable(const Value *Val) {
  std::string Code;
  raw_string_ostream SS(Code);
  Val->print(SS);
  Code.erase(0, Code.find_first_not_of(WHITESPACES));
  auto RetVal = Code.substr(0, Code.find_first_of(WHITESPACES));
  if (RetVal == "ret" || RetVal == "br" || RetVal == "store") {
    return Code;
  }
  if (RetVal == "i1" || RetVal == "i8" || RetVal == "i32" || RetVal == "i64") {
    RetVal = Code;
  }
  for (auto i = RetVal.size(); i < VARIABLE_PADDED_LEN; i++) {
    RetVal += " ";
  }
  return RetVal;
}

/**
 * @brief Instrument to initialize the Z3 solver.
 *
 * TODO: Implement this.
 *
 * @param Mod Current Module
 * @param F Function to Instrument
 * @param I Instrumentation location
 */
void instrumentDSEInit(Module *Mod, Function &F, Instruction &I) {
  std::vector<Value *> Args = {};
  auto Fun = Mod->getFunction(DSE_INIT_FUNCTION_NAME);
  CallInst::Create(Fun, Args, "", &I);
}

/**
 * @brief Instrument to Alloca Instructions.
 *
 * @param Mod Current Module
 * @param AI Instrumentation location
 */
void instrumentAlloca(Module *Mod, AllocaInst *AI) {
  auto &Context = Mod->getContext();
  auto *Int32Type = Type::getInt32Ty(Context);

  Value *VarID = ConstantInt::get(Int32Type, getRegisterID(AI));
  std::vector<Value *> Args = {VarID, AI};

  auto Fun = Mod->getFunction(DSE_ALLOCA_FUNCTION_NAME);
  CallInst::Create(Fun, Args, "", AI->getNextNonDebugInstruction());
}

/**
 * @brief Instrument to Store Instructions.
 *
 * @param Mod Current Module
 * @param SI Instrumentation location
 */
void instrumentStore(Module *Mod, StoreInst *SI) {
  // instrumentValue(Mod, SI->getPointerOperand(), SI);
  // auto &Context = Mod->getContext();
  // auto *Int32Type = Type::getInt32Ty(Context);

  // Value *VarID = ConstantInt::get(Int32Type, getRegisterID(SI));
  // instrumentValue(Mod, SI->getPointerOperand(), SI);
  // std::vector<Value *> Args = {VarID};

  // auto Fun = Mod->getFunction(DSE_STORE_FUNCTION_NAME);
  // CallInst::Create(Fun, Args, "", SI);
  std::vector<Value *> Args = {SI->getPointerOperand()};

  auto Fun = Mod->getFunction(DSE_STORE_FUNCTION_NAME);
  CallInst::Create(Fun, Args, "", SI);
}

/**
 * @brief Instrument to Load Instructions.
 *
 * TODO: Implement this.
 *
 * @param Mod Current Module
 * @param LI Instrumentation location
 */
void instrumentLoad(Module *Mod, LoadInst *LI) {
  // std::vector<Value *> Args = {LI, LI->getPointerOperand()};
  auto &Context = Mod->getContext();
  auto *Int32Type = Type::getInt32Ty(Context);
  std::vector<Value *> Args = {ConstantInt::get(Int32Type, getRegisterID(LI)), LI->getPointerOperand()};

  auto Fn = Mod->getFunction(DSE_LOAD_FUNCTION_NAME);
  CallInst::Create(Fn, Args, "", LI);
}

/**
 * @brief Instrument Constant values.
 *
 * TODO: Implement this.
 *
 * @param Mod Current Module
 * @param ConstInt Constant
 * @param I Instrumentation location.
 */
void instrumentConstantValue(Module *Mod, ConstantInt *ConstInt, Instruction *I) {
  // auto &Context = Mod->getContext();
  // auto *Int32Type = Type::getInt32Ty(Context);
  std::vector<Value *> Args = {ConstInt}; 
  auto Fn = Mod->getFunction(DSE_CONST_FUNCTION_NAME);
  CallInst::Create(Fn, Args, "", I);
}

/**
 * @brief Instrument Registers.
 *
 * TODO: Implement this.
 *
 * @param Mod Current Module
 * @param Var Variable
 * @param I Instrumentation location
 */
void instrumentRegister(Module *Mod, Value *Var, Instruction *I) {
  auto &Context = Mod->getContext();
  auto *Int32Type = Type::getInt32Ty(Context);
  auto RID = ConstantInt::get(Int32Type, getRegisterID(Var));
  std::cout << "Get Register ID " << RID << " for DSE_REGISTER\n";

  std::vector<Value *> Args = {RID};
  auto Fn = Mod->getFunction(DSE_REGISTER_FUNCTION_NAME);
  CallInst::Create(Fn, Args, "", I);
}

/**
 * @brief Instrument a Value
 *
 * TODO: Implement this.
 *
 * Hint: Values are a stored in registers;
 *       some values may also be a constant.
 *       Use the function you just defined above.
 *
 * @param Mod Current Module
 * @param Val Value
 * @param I Instrumentation location
 */
void instrumentValue(Module *Mod, Value *Val, Instruction *I) {
   auto Const = dyn_cast<ConstantInt>(Val);
   if (Const) {
    instrumentConstantValue(Mod, Const, I);
   } else {
    std::cout << "Register for " << variable(Val) << "\n";
    instrumentRegister(Mod, Val, I);
   }
}

/**
 * @brief Instrument Comparision Instructions.
 *
 * TODO: Implement this.
 *
 * @param Mod Current Module
 * @param CI Instrumentation location
 */
void instrumentICmp(Module *Mod, ICmpInst *CI) {
  auto Op1 = CI->getOperand(0);
  auto Op2 = CI->getOperand(1);
  std::cout << "***ICMP Op1 - " << variable(Op1) << " Op2 - " << variable(Op2) << "\n";
  instrumentValue(Mod, Op1, CI);
  instrumentValue(Mod, Op2, CI);

  CmpInst::Predicate P = CI->getPredicate();
  auto &Context = Mod->getContext();
  auto *Int32Type = Type::getInt32Ty(Context);
  auto PInt = ConstantInt::get(Int32Type, P);
  auto RID = ConstantInt::get(Int32Type, getRegisterID(CI));
  // instrumentRegister(Mod, RID, CI);
  std::vector<Value *> Args = {RID, PInt};
  auto Fn = Mod->getFunction(DSE_ICMP_FUNCTION_NAME);
  CallInst::Create(Fn, Args, "", CI);
}

/**
 * @brief Instrument Branch Instructions.
 *
 * TODO: Implement this.
 *
 * @param Mod Current Module
 * @param BI Instrumentation location
 */
void instrumentBranch(Module *Mod, BranchInst *BI) {
  auto &Context = Mod->getContext();
  auto *Int32Type = Type::getInt32Ty(Context);
  // auto IsBranch = BI->isConditional();
  auto BID = ConstantInt::get(Int32Type, getBranchID(BI));
  // auto Path = ConstantInt::get(Int32Type, 1);
  auto Fn = Mod->getFunction(DSE_BRANCH_FUNCTION_NAME);

  Value *RID = ConstantInt::get(Int32Type, getRegisterID(BI->getOperand(0)));  
  // Value *BID = ConstantInt::get(Int32Type, BI);
  std::vector<Value *> Args = {BID, RID, BI->getCondition()};
  CallInst::Create(Fn, Args, "", BI);
}

/**
 * @brief Instrument BinaryOperator.
 *
 * TODO: Implement this.
 *
 * @param Mod Current Module
 * @param BO Instrumentation location
 */
void instrumentBinOp(Module *Mod, BinaryOperator *BO) {
  auto Op1 = BO->getOperand(0);
  auto Op2 = BO->getOperand(1);
  instrumentValue(Mod, Op1, BO);
  instrumentValue(Mod, Op2, BO);

  auto &Context = Mod->getContext();
  auto *Int32Type = Type::getInt32Ty(Context);

  auto OpType = BO->getOpcode();
  auto OpInt = ConstantInt::get(Int32Type, OpType);
  std::vector<Value *> Args = {ConstantInt::get(Int32Type, getRegisterID(BO)), OpInt};
  
  auto Fn = Mod->getFunction(DSE_BINOP_FUNCTION_NAME);
  CallInst::Create(Fn, Args, "", BO);
}

/**
 * @brief Instrument Instructions.
 *
 * TODO: Implement this.
 *
 * Hint: Make sure to instrument all the Values
 *       used by an instruction so that they are available
 *       to the DSE Engine.
 *
 * @param Mod Current Module
 * @param I Instruction to Instrument
 */
void instrument(Module *Mod, Instruction *I) {
  if (AllocaInst *AI = dyn_cast<AllocaInst>(I)) {
    // TODO: Implement.
    instrumentAlloca(Mod, AI);
  } else if (StoreInst *SI = dyn_cast<StoreInst>(I)) {
    // TODO: Implement.
    instrumentValue(Mod, SI->getOperand(0), SI);
    instrumentStore(Mod, SI);
  } else if (LoadInst *LI = dyn_cast<LoadInst>(I)) {
    // TODO: Implement.
    // instrumentRegister(Mod, LI->getOperand(0), LI);
    instrumentLoad(Mod, LI);
  } else if (ICmpInst *CI = dyn_cast<ICmpInst>(I)) {
    // TODO: Implement.
    instrumentICmp(Mod, CI);

  } else if (BranchInst *BI = dyn_cast<BranchInst>(I)) {
    if (BI->isUnconditional())
      return;
    // TODO: Implement.
    instrumentRegister(Mod, BI->getCondition(), BI);
    instrumentBranch(Mod, BI);
  } else if (BinaryOperator *BO = dyn_cast<BinaryOperator>(I)) {
    // TODO: Implement.
    instrumentBinOp(Mod, BO);
  }
}

bool DSEInstrument::runOnFunction(Function &F) {
  LLVMContext &Context = F.getContext();
  Module *Mod = F.getParent();

  Type *VoidType = Type::getVoidTy(Context);
  Type *Int1Type = Type::getInt1Ty(Context);
  auto *Int32Type = Type::getInt32Ty(Context);
  Type *Int32PtrType = Type::getInt32PtrTy(Context);

  // Insert all the DSE function declarations into Module.
  Mod->getOrInsertFunction(DSE_INIT_FUNCTION_NAME, VoidType);
  Mod->getOrInsertFunction(DSE_ALLOCA_FUNCTION_NAME, VoidType, Int32Type, Int32PtrType);
  Mod->getOrInsertFunction(DSE_STORE_FUNCTION_NAME, VoidType, Int32PtrType);
  Mod->getOrInsertFunction(DSE_LOAD_FUNCTION_NAME, VoidType, Int32Type, Int32PtrType);
  Mod->getOrInsertFunction(DSE_CONST_FUNCTION_NAME, VoidType, Int32Type);
  Mod->getOrInsertFunction(DSE_REGISTER_FUNCTION_NAME, VoidType, Int32Type);
  Mod->getOrInsertFunction(DSE_ICMP_FUNCTION_NAME, VoidType, Int32Type, Int32Type);
  Mod->getOrInsertFunction(DSE_BRANCH_FUNCTION_NAME, VoidType, Int32Type, Int32Type, Int1Type);
  Mod->getOrInsertFunction(DSE_BINOP_FUNCTION_NAME, VoidType, Int32Type, Int32Type);

  if (F.getName().equals("main")) {
    // TODO: Initilize the DSE Engine
    instrumentDSEInit(Mod, F, *inst_begin(F));
  }

  // Instrument each instruction
  for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
    instrument(Mod, &*I);
  }
  return true;
}

char DSEInstrument::ID = 1;
static RegisterPass<DSEInstrument>
    X("DSEInstrument", "Instrumentations for Dynamic Symbolic Execution", false,
      false);

} // namespace dse
