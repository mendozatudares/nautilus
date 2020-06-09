/*
 * This file is part of the Nautilus AeroKernel developed
 * by the Hobbes and V3VEE Projects with funding from the 
 * United States National  Science Foundation and the Department of Energy.  
 *
 * The V3VEE Project is a joint project between Northwestern University
 * and the University of New Mexico.  The Hobbes Project is a collaboration
 * led by Sandia National Laboratories that includes several national 
 * laboratories and universities. You can find out more at:
 * http://www.v3vee.org  and
 * http://xstack.sandia.gov/hobbes
 *
 * Copyright (c) 2020, Drew Kersnar <drewkersnar2021@u.northwestern.edu>
 * Copyright (c) 2020, Gaurav Chaudhary <gauravchaudhary2021@u.northwestern.edu>
 * Copyright (c) 2020, Souradip Ghosh <sgh@u.northwestern.edu>
 * Copyright (c) 2020, Brian Suchy <briansuchy2022@u.northwestern.edu>
 * Copyright (c) 2020, Peter Dinda <pdinda@northwestern.edu>
 * Copyright (c) 2020, The V3VEE Project  <http://www.v3vee.org> 
 *                     The Hobbes Project <http://xstack.sandia.gov/hobbes>
 * All rights reserved.
 *
 * Authors: Drew Kersnar, Gaurav Chaudhary, Souradip Ghosh, 
 *          Brian Suchy, Peter Dinda 
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "LICENSE.txt".
 */

#include "../include/Escapes.hpp"

EscapesHandler::EscapesHandler(Module *M)
{
    DEBUG_INFO("--- Protections Constructor ---\n");

    // Set state
    this->M = M;

    // Set up data structures
    this->MemUses = std::vector<Instruction *>();

    this->_getAllNecessaryInstructions();
}

void EscapesHandler::_getAllNecessaryInstructions()
{
    for (auto &F : *M)
    {
        if ((TargetMethods.find(F.getName()) != TargetMethods.end()) 
            || (NecessaryMethods.find(F.getName()) != NecessaryMethods.end()) 
            || (F.isIntrinsic()) 
            || (!(F.getInstructionCount())))
            { continue; }

        for (auto &B : F)
        {
            for (auto &I : B)
            {
                if (isa<StoreInst>(&I))
                {
                    StoreInst *SI = static_cast<StoreInst *>(&I);
                    if (SI->getValueOperand()->getType()->isPointerTy()) 
                        MemUses.push_back(SI);                                
                }
            }
        }
    }

    return;
}

void EscapesHandler::Inject()
{
    Function *CARATEscape = NecessaryMethods[CARAT_ESCAPE];
    LLVMContext &TheContext = CARATEscape->getContext();

    // Set up types necessary for injections
    Type *VoidPointerType = Type::getInt8PtrTy(TheContext, 0); // For pointer injection

    for (auto MU : MemUses)
    {
        Instruction *InsertionPoint = MU->getNextNode();
        if (InsertionPoint == nullptr)
        {
            errs() << "Not able to instrument: ";
            MU->print(errs());
            errs() << "\n";

            continue;
        }

        // Set up injections and call instruction arguments
        IRBuilder<> MUBuilder = Utils::GetBuilder(MU->getFunction(), InsertionPoint);
        std::vector<Value *> CallArgs;

        // Get pointer operand from store instruction --- this is the
        // only parameter (casted) to the AddToEscapeTable method
        StoreInst *SMU = static_cast<StoreInst *>(MU);
        Value *PointerOperand = SMU->getPointerOperand();

        // Pointer operand casted to void pointer
        Value *PointerOperandCast = MUBuilder.CreatePointerCast(PointerOperand, VoidPointerType);

        // Add all necessary arguments, convert to LLVM data structure
        CallArgs.push_back(PointerOperandCast);
        ArrayRef<Value *> LLVMCallArgs = ArrayRef<Value *>(CallArgs);

        // Build the call instruction to CARAT escape
        CallInst *AddToEscapeTable = MUBuilder.CreateCall(CARATEscape, LLVMCallArgs);
    }

    return;
}