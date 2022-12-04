#include "SymbolicInterpreter.h"

#include <ctime>
#include <fstream>
using namespace std;

std::ostream &operator<<(std::ostream &OS, const Address &A) {
  if (A.Type == A.Memory) {
    // cout << "Operator print MEM addr " << A.Addr << "\n";
    OS << A.Addr;
  } else {
    // cout << "Operator print Register addr " << A.Addr << "\n";
    OS << "R" << A.Addr;
  }
  return OS;
}

int SymbolicInterpreter::NewInput(int *Ptr, int ID) {
  int Ret = 0;
  if (Inputs.find(ID) != Inputs.end()) {
    Ret = Inputs[ID];
  } else {
    Ret = std::rand();
    Inputs[ID] = Ret;
  }
  Address X(Ptr);
  std::string InputName = "X" + std::to_string(NumOfInputs);
  z3::expr SE = Ctx.int_const(InputName.c_str());
  Mem.insert(std::make_pair(X, SE));
  NumOfInputs++;
  return Ret;
}

SymbolicInterpreter SI;

void print(std::ostream &OS) {
  OS << "=== Inputs ===" << std::endl;
  int Idx = 0;
  for (auto &E : SI.getInputs()) {
    OS << "X" << E.first << " : " << E.second << std::endl;
  }
  OS << std::endl;
  OS << "=== Symbolic Memory ===" << std::endl;
  for (auto &E : SI.getMemory()) {
    OS << E.first << " : " << E.second << std::endl;
  }
  OS << std::endl;

  OS << "=== Path Condition ===" << std::endl;
  for (auto &E : SI.getPathCondition()) {
    std::string BID = "B" + std::to_string(E.first);
    OS << BID << " : " << E.second << std::endl;
  }
}

extern "C" void __DSE_Exit__() {
  z3::solver Solver(SI.getContext());
  std::ofstream Branch(BranchFile);
  for (auto &E : SI.getPathCondition()) {
    std::string BID = "B" + std::to_string(E.first);
    Solver.add(E.second);
    Branch << BID << "\n";
  }
  std::ofstream Smt2(FormulaFile);
  Smt2 << Solver.to_smt2();
  std::ofstream Log(LogFile);
  print(Log);
}

extern "C" void __DSE_Init__() {
  std::srand(std::time(nullptr));
  std::string Line;
  std::ifstream Input(InputFile);
  if (Input.is_open()) {
    while (getline(Input, Line)) {
      int ID = std::stoi(Line.substr(1, Line.find(",")));
      int Val = std::stoi(Line.substr(Line.find(",") + 1));
      SI.getInputs()[ID] = Val;
    }
  }
  std::atexit(__DSE_Exit__);
}

extern "C" void __DSE_Input__(int *X, int ID) { *X = (int)SI.NewInput(X, ID); }

extern "C" void __DSE_Branch__(int BID, int RID, int B) {
  MemoryTy &Mem = SI.getMemory();
  Address Addr(RID);
  z3::expr SE = Mem.at(Addr);
  z3::expr Cond =
      B ? SI.getContext().bool_val(true) : SI.getContext().bool_val(false);
  // cout << "In DSE_Branch, having BID - " << BID << " SE - " << SE << " Cond - " << Cond << "\n";
  SI.getPathCondition().push_back(std::make_pair(BID, SE == Cond));
}

extern "C" void __DSE_Const__(int X) {
  z3::expr SE = SI.getContext().int_val(X);
  // cout << "DSE_Const push " << SE << "\n";
  SI.getStack().push(SE);
}

extern "C" void __DSE_Register__(int X) {
  std::string Name = "R" + std::to_string(X);
  z3::expr SE = SI.getContext().int_const(Name.c_str());
  // cout << "DSE_Register push " << SE << "\n";
  SI.getStack().push(SE);
}
