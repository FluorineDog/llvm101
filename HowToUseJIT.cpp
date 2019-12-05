//===-- examples/HowToUseJIT/HowToUseJIT.cpp - An example use of the JIT --===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//  This small program provides an example of how to quickly build a small
//  module with two functions and execute it with the JIT.
//
// Goal:
//  The goal of this snippet is to create in the memory
//  the LLVM module consisting of two functions as follow:
//
// int add1(int x) {
//   return x+1;
// }
//
// int foo() {
//   return add1(10);
// }
//
// then compile the module via JIT, then execute the `foo'
// function and return result to a driver, i.e. to a "host program".
//
// Some remarks and questions:
//
// - could we invoke some code using noname functions too?
//   e.g. evaluate "foo()+foo()" without fears to introduce
//   conflict of temporary function name with some real
//   existing function name?
//
//===----------------------------------------------------------------------===//

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

using namespace llvm;

using std::vector;
vector<double>
generate_data(int size, double step) {
    vector<double> vec(size);
    for (int i = 0; i < size; ++i) {
        vec[i] = i * step;
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

int
main() {
    LLVM_Environment llvm_enviroment;

    LLVMContext ctx;
    // Create some module to put our function into it.
    std::unique_ptr<Module> module_owner = make_unique<Module>("test", ctx);
    Module* mod = module_owner.get();
    // Create a basic block builder with default parameters.
    IRBuilder<> builder(ctx);

    // Create the add1 function entry and insert this entry into module mod.  The
    // function will have a return type of "int" and take an argument of "int".
    FunctionType* func_type = FunctionType::get(
        /*return_type*/ Type::getInt32Ty(ctx),
        /*arg_types*/ {Type::getInt32Ty(ctx)},
        /*is_var_args*/ false);
    Function* Add1F = Function::Create(func_type, Function::ExternalLinkage, "add1", mod);

    // Add a basic block to the function. As before, it automatically inserts
    // because of the last argument.
    BasicBlock* BB = BasicBlock::Create(ctx, "EntryBlock", Add1F);
    builder.SetInsertPoint(BB);

    // Get pointers to the constant `1'.
    Constant* One = builder.getInt32(1);

    // Get pointers to the integer argument of the add1 function...
    auto iter = Add1F->arg_begin();
    Argument* input_x = iter;
    input_x->setName("input_x");    // Give it a nice symbolic name for fun.
    iter++;
    assert(iter == Add1F->arg_end());

    // Create the add instruction, inserting it into the end of BB.
    Value* X_plus_1 = builder.CreateAdd(input_x, One);
    // Create the return instruction and add it to the basic block
    builder.CreateRet(X_plus_1);

    // Now, function add1 is ready.

    // Now we're going to create function `foo', which returns an int and takes no
    // arguments.
    Function* FooF = Function::Create(FunctionType::get(Type::getInt32Ty(ctx), {}, false),
                                      Function::ExternalLinkage,
                                      "foo",
                                      mod);
    auto start_bb = BasicBlock::Create(ctx, "start_bb", FooF);
    auto then_bb = BasicBlock::Create(ctx, "then_bb", FooF);
    auto last_bb = BasicBlock::Create(ctx, "last_bb", FooF);

    builder.SetInsertPoint(start_bb);
    auto local_x = builder.CreateAlloca(Type::getInt32Ty(ctx), nullptr, "local_x");
    Constant* Two = builder.getInt32(2);
    Constant* Three = builder.getInt32(3);
    auto if_cond = builder.CreateICmpNE(Two, Three);
    builder.CreateCondBr(if_cond, then_bb, last_bb);


    builder.SetInsertPoint(then_bb);
    Constant* Ten = builder.getInt32(10);
    builder.CreateStore(Ten, local_x);
    builder.CreateBr(last_bb);

    builder.SetInsertPoint(last_bb);
    auto x = builder.CreateLoad(local_x, "x");
    auto final_res = builder.CreateCall(Add1F, x);
    builder.CreateRet(final_res);

    // Now we create the MCJIT.
    std::unique_ptr<ExecutionEngine> EE(EngineBuilder(std::move(module_owner)).create());
    outs() << *mod;

    // Call the `foo' function with no arguments:
    uint64_t fptr_ = EE->getFunctionAddress("foo");
    int (*fptr)() = reinterpret_cast<int (*)()>(fptr_);
    // std::vector<GenericValue> noargs();
    // GenericValue gv = EE->runFunction(FooF, noargs);
    auto result = fptr();
    // Import result of execution:
    outs() << "Result: " << result << "\n";
    return 0;
}