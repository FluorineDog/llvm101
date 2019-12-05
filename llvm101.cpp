#include "llvm/ADT/STLExtras.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/IR/Argument.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include <algorithm>
#include <cassert>
#include <memory>
#include <vector>
#include <random>

using namespace llvm;

using std::vector;


vector<int>
generate_vec(int size) {
    static std::default_random_engine e(67);
    vector<int> vec(size);
    for (int i = 0; i < size; ++i) {
        vec[i] = e() % 10000 + 1;
    }
    return vec;
}

class LLVM_Environment {
 public:
    LLVM_Environment() {
        InitializeNativeTarget();
        LLVMInitializeNativeAsmPrinter();
        LLVMInitializeNativeAsmParser();
    }
    ~LLVM_Environment() {
        // free all resources
        llvm_shutdown();
    }
};

enum class Op { Plus, Minus, Multiply, Divide, Modular };

using func_t = void (*)(int size, const int* va, const int* vb, const int* vc, int* vd);

class Engine {
 public:
    Engine(): Owner(make_unique<Module>("llvm101", ctx)), mod(Owner.get()), builder(ctx){
        
    }
    func_t get_function(Op op1, Op op2);
 private:
   LLVMContext ctx; 
   std::unique_ptr<Module> Owner;
   Module *mod;
   IRBuilder<> builder;
};

// excise now!
func_t Engine::get_function(Op op1, Op op2){
    // insert code here!!

    return nullptr;
}

int
main() {
    LLVM_Environment llvm_environment;
    int size = 1000;
    vector<int> vec_a = generate_vec(size);
    vector<int> vec_b = generate_vec(size);
    vector<int> vec_c = generate_vec(size);
    Engine eng;
    func_t ();
    
}