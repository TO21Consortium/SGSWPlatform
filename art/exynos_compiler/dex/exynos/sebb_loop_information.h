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

#ifndef ART_COMPILER_DEX_EXYNOS_SINGLE_EBB_LOOP_INFORMATION_H_
#define ART_COMPILER_DEX_EXYNOS_SINGLE_EBB_LOOP_INFORMATION_H_

#include "../compiler_internals.h"
#include "loop_information.h"

namespace art {

/*
 * An induction variable is represented by "m*i + c", where i is a basic
 * induction variable.
 * Currently, only simple counted loop is optimizable. (m == 1)
 */
typedef struct InductionVariableInformation {
  int ssa_reg;
  int basic_ssa_reg;
  int init_ssa_reg;
  int m;              /* multiplier */
  int c;              /* constant */
  int64_t inc;            /* loop incriment */
  bool c_is_constant; /* supporting the case that c is not constant */
  bool in_cond_bb;
  int final_ssa_reg;
  int cycle_ssa_reg;
  bool isConstInitVal;
  bool isConstFinalVal;
  bool is64bits; /* long, double type */
  bool isFloatingPointType; /* float, double type */
  int64_t init_val;
  int64_t final_val;
  int64_t trip_count;
  std::vector<MIR*> linear_mirs;
} InductionVariableInformation;

typedef struct ArrayAccessInformation {
  int offset;
  int array_reg;
  int iv_reg;
  int inc;                  /*  loop incriment */
  int max_constant;         /*  upper bound checking */
  int min_constant;         /*  lower bound checking */
  bool c_is_constant;       /*  supporting the case that c is not constant */
  int idx_reg; /*  supporting the case that idx value is loop invariant */
  bool idx_is_loop_invariant; /*  supporting the case that idx value is loop invariant */
} ArrayAccessInformation;

typedef struct LowerBoundInformation {
  int array_reg;
  int iv_reg;
  int dec;            /* loop incriment */
  int global_min_c;
  int global_min_offset;
  bool c_is_constant;
} LowerBoundInformation;

typedef struct PhiInformation {
  int init_ssa_reg;
  int cycle_ssa_reg;
  bool is_found_init_ssa_reg;
  bool is_found_cycle_ssa_reg;
} PhiInformation;

// This enum represents the kinds of reductions that we support.
enum ReductionKind {
  RK_NoReduction, // Not a reduction.
  RK_IntegerAdd,  // Sum of integers.
  RK_IntegerMult, // Product of integers.
  RK_IntegerOr,   // Bitwise or logical OR of numbers.
  RK_IntegerAnd,  // Bitwise or logical AND of numbers.
  RK_IntegerXor,  // Bitwise or logical XOR of numbers.
  RK_IntegerMinMax, // Min/max implemented in terms of select(cmp()).
  RK_FloatAdd,    // Sum of floats.
  RK_FloatMult,   // Product of floats.
  RK_FloatMinMax,  // Min/max implemented in terms of select(cmp()).
  RK_IntegerSub,
  RK_FloatSub,
  RK_IntegerDiv,
  RK_FloatDiv
};

// This struct holds information about reduction variables.
typedef struct ReductionVariableInformation {
  int def_ssa_reg;
  //starting ssa reg of the reduction.
  int init_ssa_reg;
  int cycle_ssa_reg;

  // The instruction who's value is used outside the loop.
  MIR *LoopExitInstr;

  MIR *TypeCastMIR;

  // The kind of the reduction.
  ReductionKind Kind;

  bool isConstInitVal;

  bool is64bits;

  int64_t init_val;

} ReductionVariableInfo;

class SingleEBBLoopInformation : public LoopInformation {
 public:
  explicit SingleEBBLoopInformation(CompilationUnit* cu)
    : LoopInformation(cu),
      basic_blocks_(arena_->Adapter(kArenaAllocBBList)),
      backward_branch_blocks_(arena_->Adapter(kArenaAllocBBList)),
      exiting_blocks_(arena_->Adapter(kArenaAllocBBList)),
      exit_blocks_(arena_->Adapter(kArenaAllocBBList)),
      phi_mir_(arena_->Adapter(kArenaAllocMisc)),
      iv_information_(arena_->Adapter(kArenaAllocMisc)),
      array_information_(arena_->Adapter(kArenaAllocMisc)),
      lower_bound_information_(arena_->Adapter(kArenaAllocMisc)),
      phi_information_(arena_->Adapter(kArenaAllocMisc)),
      rv_information_(arena_->Adapter(kArenaAllocMisc)) {

    /* Loop Defined Variables */
    loop_defined_variables_ =
      new (arena_) ArenaBitVector(arena_, mir_graph_->GetNumOfCodeVRs(), true);

    /* Loop Invariant Variables */
    loop_invariant_variables_ =
      new (arena_) ArenaBitVector(arena_, mir_graph_->GetNumOfCodeVRs(), true);

    /* Induction Variables */
    induction_variables_ =
      new (arena_) ArenaBitVector(arena_, mir_graph_->GetNumOfCodeVRs(), true);

    /* Reduction Variables */
    reduction_variables_ =
      new (arena_) ArenaBitVector(arena_, mir_graph_->GetNumOfCodeVRs(), true);

    trip_count_ = -1;
  }

  ~SingleEBBLoopInformation() {}

  int GetDepth() const;

  void AddExitBlock(BasicBlockId bb_id);
  void AddExitingBlock(BasicBlockId bb_id);
  void AddBasicBlock(BasicBlockId bb_id);
  void AddBackwardBranchBlock(BasicBlockId bb_id);

  void DelExitBlock(BasicBlockId bb_id);
  void DelExitingBlock(BasicBlockId bb_id);
  void DelBasicBlock(BasicBlockId bb_id);
  void DelBackwardBranchBlock(BasicBlockId bb_id);

  uint32_t GetNumExitBlock() const;
  uint32_t GetNumExitingBlock() const;
  uint32_t GetNumBackwardBranches() const;

  const ArenaVector<BasicBlockId> GetExitBlocks() const {
    return exit_blocks_;
  }

  const ArenaVector<BasicBlockId> GetExitingBlocks() const {
    return exiting_blocks_;
  }

  const ArenaVector<BasicBlockId> GetBasicBlocks() const {
    return basic_blocks_;
  }

  const ArenaVector<BasicBlockId> GetBackwardBrancheBlocks() const {
    return backward_branch_blocks_;
  }

  BasicBlockId GetExitingBlock() const;
  BasicBlockId GetBackwardBranchBlock() const;
  BasicBlockId GetExitBlock() const;
  BasicBlockId GetLoopHeadBlock() const;
  BasicBlockId GetLoopEndBlock() const;
  BasicBlockId GetDeOptLoopHeadBlock() const;
  BasicBlockId GetDeOptLoopEndBlock() const;
  BasicBlockId GetLoopPreHeadBlock() const;

  void SetLoopPreHeadBlock(BasicBlockId bb);
  void SetLoopHeadBlock(BasicBlockId bb_id);
  void SetLoopEndBlock(BasicBlockId bb_id);
  void SetDeOptLoopHeadBlock(BasicBlockId bb_id);
  void SetDeOptLoopEndBlock(BasicBlockId bb_id);
  void DeleteDeOptLoopBlocks();

  void AddPhiMIR(MIR* mir) {
    phi_mir_.push_back(mir);
  }
  const ArenaVector<MIR*> GetPhiMIRs() {
    return phi_mir_;
  }

  ArenaBitVector* GetLoopDefinedVariables() const {
    return loop_defined_variables_;
  }
  ArenaBitVector* GetLoopInvariantVariables() const {
    return loop_invariant_variables_;
  }
  ArenaBitVector* GetInductionVariables() const {
    return induction_variables_;
  }
  void SetInductionVariablesBit(int32_t def) {
    induction_variables_->SetBit(def);
  }
  ArenaBitVector* GetReductionVariables() const {
    return reduction_variables_;
  }
  void SetReductionVariablesBit(int32_t def) {
    reduction_variables_->SetBit(def);
  }
  ArenaVector<InductionVariableInformation*> GetInductionVariableInformations() const {
    return iv_information_;
  }
  void AddInductionVariableInformation(InductionVariableInformation* ivInfo) {
    iv_information_.push_back(ivInfo);
  }
  ArenaVector<ArrayAccessInformation*> GetArrayAccessInformations() const {
    return array_information_;
  }
  ArenaVector<LowerBoundInformation*> GetLowerBoundInformations() const {
    return lower_bound_information_;
  }
  ArenaVector<PhiInformation*> GetPhiInformations() const {
    return phi_information_;
  }
  void AddPhiInformation(PhiInformation* phi_info) {
    phi_information_.push_back(phi_info);
  }
  ArenaVector<ReductionVariableInformation*> GetReductionVariableInformations() const {
    return rv_information_;
  }
  void AddReductionVariableInformation(ReductionVariableInfo* rv_info){
    rv_information_.push_back(rv_info);
  }
  /* Below Value is Only Valid when Loop is Simplified */
  InductionVariableInformation* GetBIVInformation() const {
    return biv_info_;
  }
  void SetBIVInformation(InductionVariableInformation* biv_info) {
    biv_info_ = biv_info;
  }
  InductionVariableInformation* GetLoopExitIVInformation() const {
    return loop_exit_iv_;
  }
  void SetLoopExitIVInformation(InductionVariableInformation* loop_exit_iv) {
    loop_exit_iv_ = loop_exit_iv;
  }

  int32_t GetExitingConditionSSAValue() const {
    if (GetExitingBlock() == NullBasicBlockId)
      return -1;

    MIR* exiting_mir = mir_graph_->GetBasicBlock(GetExitingBlock())->last_mir_insn;
    if (exiting_mir->ssa_rep->num_uses == 2)
      return exiting_mir->ssa_rep->uses[1];
    return -1;
  }
  Instruction::Code GetExitingConditionOpCode() const {
    if (GetExitingBlock() == NullBasicBlockId)
      return static_cast<Instruction::Code>(kMirOpNop);

    MIR* exiting_mir = mir_graph_->GetBasicBlock(GetExitingBlock())->last_mir_insn;
    return exiting_mir->dalvikInsn.opcode;
  }

  void AddInstructionToExits(MIR* mir);

  bool CanThrow() const;
  bool HasInvoke() const;
  bool HasArray() const;
  bool HasSwitch() const;
  bool HasObjectDefine() const;
  bool IsLoopBody(BasicBlockId bb_id) const;
  bool IsDoWhileLoop() {
    return do_while_loop_;
  }

  bool IsLoopSimplifyForm() const {
    /*
     * Simplify Loop Form
     *  1. Only One Induction Variable
     *  2. Phi uses[0] is always belong to loop body.
     *  3. Exiting Branches uses[0] must be BIV.
     */
    return simplified_loop_;
  }

  void SetSimplifiedLoopInfo(bool is_simplify) {
    simplified_loop_ = is_simplify;
  }

  bool IsCountUpLoop() const {
    return is_count_up_loop_;
  }

  int GetMIRCount() const {
    return mir_count_;
  }

  void SetMIRCount(int val) {
    mir_count_ = val;
  }

  int GetTripCount() const {
    return trip_count_;
  }

  void SetTripCount(int val) {
    trip_count_ = val;
  }

  void SetCountUpLoopValue(bool is_count_up) {
    is_count_up_loop_ = is_count_up;
  }

  void SetDoWhileLoopInfo(bool is_do_while) {
    do_while_loop_ = is_do_while;
  }

  bool verifyLoop() const;

  bool CanApplyRangeCheckElimination();

 private:
  ArenaVector<BasicBlockId> basic_blocks_;
  ArenaVector<BasicBlockId> backward_branch_blocks_;
  ArenaVector<BasicBlockId> exiting_blocks_;
  ArenaVector<BasicBlockId> exit_blocks_;

  BasicBlockId loop_pre_head_block_;
  BasicBlockId loop_head_block_;
  BasicBlockId loop_end_block_;
  BasicBlockId deopt_loop_head_block_;
  BasicBlockId deopt_loop_end_block_;

  bool do_while_loop_;
  bool simplified_loop_;
  bool is_count_up_loop_;

  int mir_count_;
  int trip_count_;

  /* Phi MIR */
  ArenaVector<MIR*> phi_mir_;

  /* Loop Defined Variables */
  ArenaBitVector* loop_defined_variables_;

  /* Loop Invariant Variables */
  ArenaBitVector* loop_invariant_variables_;

  /* Induction Variables */
  ArenaBitVector* induction_variables_;

  /* Reduction Variables */
  ArenaBitVector* reduction_variables_;

  ArenaVector<InductionVariableInformation*> iv_information_;
  InductionVariableInformation* biv_info_;

  ArenaVector<ArrayAccessInformation*> array_information_;
  ArenaVector<LowerBoundInformation*> lower_bound_information_;

  ArenaVector<PhiInformation*> phi_information_;

  ArenaVector<ReductionVariableInformation*> rv_information_;

  /* Induction variable used in loop exit condition test */
  InductionVariableInformation* loop_exit_iv_;

  };

}  // namespace art.

#endif  // ART_COMPILER_DEX_EXYNOS_SINGLE_EBB_LOOP_INFORMATION_H_
