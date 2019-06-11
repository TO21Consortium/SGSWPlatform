/*
 * Copyright (C) 2014 Samsung Electronics S.LSI
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ART_COMPILER_DEX_EXYNOS_MIR_GRAPH_OPT_H_
#define ART_COMPILER_DEX_EXYNOS_MIR_GRAPH_OPT_H_

#include "../compiler_internals.h"
#include "sebb_loop_information.h"

namespace art {

#define MIR_DO_WHILE_BACKWARD_BRANCH    (1 << kMIRDoWhileBackwardBranch)

class MIRGraphOpt : public MIRGraph {
 public:
  MIRGraphOpt(CompilationUnit* cu, ArenaAllocator* arena)
    : MIRGraph(cu, arena),
      sebb_loop_informations_(arena->Adapter(kArenaAllocMisc)) {
  }

  ~MIRGraphOpt() {};

  virtual void DumpCFG(const char* dir_prefix, bool all_blocks, const char* suffix = nullptr);
  bool FrontEndPassGate(uint32_t flag);
  bool BackEndPassGate(uint32_t flag);
  void InitArtDebug(bool enable, std::string method_name);
  void FindSingleEBBLoop(class BasicBlock* bb);
  void PreSimplifySingleEBBLoop();
  void FindIVForSingleEBBLoop();
  void FindTripCountForSingleEBBLoop();
  void PostSimplifySingleEBBLoop();
  void RangeCheckEliminationForSingleEBBLoop();

  void FindReductionVariablesForSingleEBBLoop();
  void LoopPreCalculation();

  const ArenaVector<SingleEBBLoopInformation*>& GetSingleEBBLoopInformation() {
    return sebb_loop_informations_;
  }

  bool IsMIRInBasicBlock(MIR* mir, BasicBlock* bb);
  int GetOatDFAttr(Instruction::Code opcode) {
    return oat_data_flow_attributes_[opcode];
  }
  void LoopVectorization();
  void InsertBlock(BasicBlock* bb);

  int PositionofRef(MIR* mir) {
     int refIdx;

     switch(mir->dalvikInsn.opcode)
     {
       case Instruction::APUT_WIDE:
           refIdx = 2;
           break;
       case Instruction::AGET:
       case Instruction::AGET_WIDE:
       case Instruction::AGET_OBJECT:
       case Instruction::AGET_BOOLEAN:
       case Instruction::AGET_BYTE:
       case Instruction::AGET_CHAR:
       case Instruction::AGET_SHORT:
           refIdx = 0;
           break;
       case Instruction::APUT:
       case Instruction::APUT_OBJECT:
       case Instruction::APUT_BOOLEAN:
       case Instruction::APUT_BYTE:
       case Instruction::APUT_CHAR:
       case Instruction::APUT_SHORT:
           refIdx = 1;
           break;
       default:
           refIdx = -1;
           break;
     }
     return refIdx;
  }


 private:
  /* Single EBB Loop */
  ArenaVector<SingleEBBLoopInformation*> sebb_loop_informations_;

  /* For Simplifying */
  void CodeLayoutForSingleEBBLoop();
  void LoopInversionForSingleEBB();
  void MakePreHeaderForSingleEBBLoop();
  void MakeExitsForSingleEBBLoop();
  void MakeDeOptLoopBlocksForSingeEBBLoop();

  /* For IV */
  void FindPhiMIRsForSingleEBBLoop();
  void FindDefinedVariablesForSingleEBBLoop();
  void FindLoopInvariantVariablesForSingleEBBLoop();
  void FindBasicInductionVariablesForSingleEBBLoop();
  void FindDerivedInductionVariablesForSingleEBBLoop();
  void FindPhiInformation(SingleEBBLoopInformation*, MIR*);

  /* For RangeCheck Elimination for SingleEBBLoop */
  void CollectArrayAccessInfo(SingleEBBLoopInformation* loop_information);
  void UpdateRangeCheckInfoForIV(SingleEBBLoopInformation* loop_analysis,
                                 int array_reg, int idx_reg, int offset);
  void UpdateRangeCheckInfoForLoopInvariant(SingleEBBLoopInformation* loop_analysis,
                                            int array_reg, int idx_reg, int offset);
  bool DoCodeMotion(SingleEBBLoopInformation* loop_information);



  bool IsBackedge(BasicBlock* branch_bb, BasicBlockId target_bb_id) {
    return ((target_bb_id != NullBasicBlockId) &&
            (GetBasicBlock(target_bb_id)->start_offset <= branch_bb->start_offset));
  }
};

}  // namespace art

#endif  // ART_COMPILER_DEX_EXYNOS_MIR_GRAPH_OPT_H_
