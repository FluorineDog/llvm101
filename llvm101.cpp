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
#include <iostream>

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
    Engine()
        : Owner(make_unique<Module>("llvm101", ctx)), mod(Owner.get()), builder(ctx) {}
    func_t get_function(Op op1, Op op2);

 private:
    LLVMContext ctx;
    std::unique_ptr<Module> Owner;
    Module* mod;
    IRBuilder<> builder;
    std::unique_ptr<ExecutionEngine> EE;
};


// excise now!
func_t
Engine::get_function(Op op1, Op op2) {
    // insert code here!!
    auto ft = FunctionType::get(Type::getVoidTy(ctx),
                                {Type::getInt32Ty(ctx),
                                 Type::getInt32PtrTy(ctx),
                                 Type::getInt32PtrTy(ctx),
                                 Type::getInt32PtrTy(ctx),
                                 Type::getInt32PtrTy(ctx)},
                                false);
    static int counter = 0;
    auto name = "func_" + std::to_string(counter++);
    auto f = Function::Create(ft, Function::ExternalLinkage, name, mod);
    auto init_bb = BasicBlock::Create(ctx, "init_bb", f);
    auto cond_bb = BasicBlock::Create(ctx, "cond_bb", f);
    auto body_bb = BasicBlock::Create(ctx, "body_bb", f);
    auto exit_bb = BasicBlock::Create(ctx, "exit_bb", f);

    auto arg_iter = f->arg_begin();
    auto size = arg_iter++;
    auto vec_a = arg_iter++;
    auto vec_b = arg_iter++;
    auto vec_c = arg_iter++;
    auto vec_d = arg_iter++;
    assert(arg_iter == f->arg_end());

    builder.SetInsertPoint(init_bb);
    auto local_index = builder.CreateAlloca(Type::getInt32Ty(ctx), nullptr, "local_index");
    builder.CreateStore(builder.getInt32(0), local_index);
    builder.CreateBr(cond_bb);

    builder.SetInsertPoint(cond_bb);
    auto indexCond = builder.CreateLoad(local_index);
    auto is_cond = builder.CreateICmpSLE(indexCond, size, "is_cond");
    builder.CreateCondBr(is_cond, body_bb, exit_bb);

    builder.SetInsertPoint(body_bb);
    auto index_body = builder.CreateLoad(local_index, "index");
    auto a = builder.CreateLoad(builder.CreateGEP(vec_a, index_body), "va");
    auto addr_d = builder.CreateGEP(vec_d, index_body);
    builder.CreateStore(a, addr_d);
    auto index_body_plus_1 =
        builder.CreateAdd(index_body, builder.getInt32(1), "index_plus_1");
    builder.CreateStore(index_body_plus_1, local_index);
    builder.CreateBr(cond_bb);

    builder.SetInsertPoint(exit_bb);
    builder.CreateRetVoid();

    EE = std::unique_ptr<ExecutionEngine>(EngineBuilder(std::move(Owner)).create());
    outs() << *mod;
    auto fptr_ =  EE->getFunctionAddress(name);
    return reinterpret_cast<func_t>(fptr_);
}

int
main() {
    LLVM_Environment llvm_environment;
    int size = 1000;
    vector<int> vec_a = generate_vec(size);
    vector<int> vec_b = generate_vec(size);
    vector<int> vec_c = generate_vec(size);
    Engine eng;
    func_t func_multiply_plus = eng.get_function(Op::Multiply, Op::Plus);

    vector<int> vec_d(size);
    func_multiply_plus(size, vec_a.data(), vec_b.data(), vec_c.data(), vec_d.data());
    //
    for (int i = 0; i < size; ++i) {
        auto ans = vec_d[i];
        // auto ref = vec_a[i] * vec_b[i] + vec_c[i];
        auto ref = vec_a[i];
        if (ref != ans) {
            std::cout << "error at i=" << i << " ans=" << ans << " ref=" << ref;
            exit(-1);
        }
    }
    std::cout << "all is ok";
    return 0;
}
