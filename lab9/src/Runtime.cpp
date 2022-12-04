#include <iostream>

#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"

#include "SymbolicInterpreter.h"
using namespace llvm;

extern SymbolicInterpreter SI;

z3::expr eval(z3::expr &E) {
  if (E.kind() == Z3_NUMERAL_AST) {
    return E;
  } else {
    // std::cout << "--> Check mem <-- for " << E << "\n";
    MemoryTy Mem = SI.getMemory();
    Address Register(E);

    for (auto m: Mem) {
      // std::cout << "Mem is like " << m.first << " -> " << m.second << "\n";
    }
    if (Mem.find(Register) != Mem.end()) {
      return Mem.at(Register);
    } else {
      std::cout << "Warning: Cannot find register " << Register << " in memory "
                << std::endl;
      return E;
    }   
  }
  
}

/**
 * @brief Symbolically evaluate Alloca
 *
 * @param R RegisterID
 * @param Ptr Address
 */
extern "C" void __DSE_Alloca__(int R, int *Ptr) {
  MemoryTy &Mem = SI.getMemory();
  Address Register(R);
  z3::expr SE = SI.getContext().int_val((uintptr_t)Ptr);
  Mem.insert(std::make_pair(Register, SE));
}

/**
 * @brief Symbolically evaluate Store
 *
 * @param X Address
 */
extern "C" void __DSE_Store__(int *X) {
  MemoryTy &Mem = SI.getMemory();
  Address Addr(X);
  z3::expr SE = eval(SI.getStack().top());
  SI.getStack().pop();
  Mem.erase(Addr);
  Mem.insert(std::make_pair(Addr, SE));
}

/**
 * @brief Symbolically evaluate Load
 *
 * TODO: Implement this.
 *
 * @param Y Address of destination
 * @param X Address of Load source
 */
extern "C" void __DSE_Load__(int Y, int *X) {
  MemoryTy &Mem = SI.getMemory();
  Address Register(Y);
  z3::expr SE = SI.getContext().int_val((uintptr_t)X);
  Mem.insert(std::make_pair(Register, SE));
}

/**
 * @brief Symbolically evaluate Comparisions
 *
 * TODO: Implement this.
 *
 * @param R Address of Comparision result
 * @param Op Operator Kind
 */
extern "C" void __DSE_ICmp__(int R, int Op) {
  // std::cout << "__DSE_ICmp__ Args " << R << " and " << Op << "\n";
  MemoryTy &Mem = SI.getMemory();
  Address Addr(R);
  z3::expr SE2 = SI.getStack().top();
  SI.getStack().pop();
  auto SE1 = SI.getStack().top(); // get Rx

  SI.getStack().pop();
  Mem.erase(Addr);
  // std::cout << "DSE_ICMP inserts addr - " << Addr << " for SE1 - " << SE1 << " SE2 - " << SE2 << "\n";
  switch (Op) {
    case CmpInst::Predicate::ICMP_EQ:
      Mem.insert(std::make_pair(Addr, SE1 == SE2));
      break;
    case CmpInst::Predicate::ICMP_NE:
      Mem.insert(std::make_pair(Addr, SE1 != SE2));
      break;
    case CmpInst::Predicate::ICMP_SGE: 
    case CmpInst::Predicate::ICMP_UGE:
      Mem.insert(std::make_pair(Addr, SE1 >= SE2));
      break;
    case CmpInst::Predicate::ICMP_SLE: 
    case CmpInst::Predicate::ICMP_ULE:
      Mem.insert(std::make_pair(Addr, SE1 <= SE2));
      break;
    case CmpInst::Predicate::ICMP_SLT:
    case CmpInst::Predicate::ICMP_ULT:
      Mem.insert(std::make_pair(Addr, SE1 < SE2));
      break;
    case CmpInst::Predicate::ICMP_SGT: 
    case CmpInst::Predicate::ICMP_UGT:
      Mem.insert(std::make_pair(Addr, SE1 > SE2));
      break;
    default:
      return;
  }
}

/**
 * @brief Symbolically evaluate Binary Operation.
 *
 * TODO: Implement this.
 *
 * @param R Address of Binary Operation result.
 * @param Op Operator Kind
 */
extern "C" void __DSE_BinOp__(int R, int Op) {
  MemoryTy &Mem = SI.getMemory();
  Address Addr(R);
  z3::expr SE2 = eval(SI.getStack().top());
  // auto SE2 = SI.getStack().top();
  SI.getStack().pop();
  z3::expr SE1 = eval(SI.getStack().top());
  // auto SE1 = SI.getStack().top();
  SI.getStack().pop();
  Mem.erase(Addr);

  switch (Op){
      case Instruction::BinaryOps::Add:
        Mem.insert(std::make_pair(Addr, SE1 + SE2));
        break;
      case Instruction::BinaryOps::Sub:
        Mem.insert(std::make_pair(Addr, SE1 - SE2));
        break;
      case Instruction::BinaryOps::Mul:
        Mem.insert(std::make_pair(Addr, SE1 * SE2));
        break;
      case Instruction::BinaryOps::SDiv:
        Mem.insert(std::make_pair(Addr, SE1 / SE2));
        break;
      case Instruction::BinaryOps::UDiv:
        Mem.insert(std::make_pair(Addr, udiv(SE1, SE2)));
        break;
      case Instruction::BinaryOps::SRem:
        Mem.insert(std::make_pair(Addr, srem(SE1, SE2)));
        break;
      case Instruction::BinaryOps::URem:
        Mem.insert(std::make_pair(Addr, urem(SE1, SE2)));
        break;
      default:
        return;
    }
}
