/*
 * Copyright (C) 2015 Samsung Electronics S.LSI
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

#ifndef ART_COMPILER_OPTIMIZING_EXYNOS_SEBB_LOOP_INFO_H_
#define ART_COMPILER_OPTIMIZING_EXYNOS_SEBB_LOOP_INFO_H_

#include "../nodes.h"

namespace art {

class HArrayLength;
class HBoundsCheck;

typedef struct IVInfo {
  HInstruction* ssa_reg;
  HInstruction* basic_ssa_reg;
  HInstruction* init_ssa_reg;
  int m;              /* multiplier */
  int c;              /* constant */
  int64_t int_inc;            /* loop incriment */
  double fp_inc;
  bool c_is_constant; /* supporting the case that c is not constant */
  bool in_cond_bb;
  HInstruction* final_ssa_reg;
  HInstruction* cycle_ssa_reg;
  bool isConstInitVal;
  bool isConstFinalVal;
  bool is64bits; /* long, double type */
  bool isFloatingPointType; /* float, double type */
  int64_t int_init_val;
  int64_t int_final_val;
  int64_t int_trip_count;
  double fp_init_val;
  double fp_final_val;
  double fp_trip_count;
  std::vector<HInstruction*> linear_insts;
  HInstruction* delta;    /* valid for DIV, nullptr for BIV */
} IVInfo;

typedef struct ArrayAccessInfo {
  HInstruction* array_reg;
  HInstruction* iv_reg;
  HArrayLength* array_length;
  HBoundsCheck* bound_check;
  std::set<HBoundsCheck*> bound_checks_with_const_c;
  int64_t inc;                  /*  loop incriment */
  HInstruction* delta;      /* index delta ex) a[i+1] delta: 1, a[i+c] delta: c */
  int max_constant;         /*  upper bound checking */
  int min_constant;         /*  lower bound checking */
  bool c_is_constant;       /*  supporting the case that c is not constant */
  HInstruction* idx_reg; /*  supporting the case that idx value is loop invariant */
  bool idx_is_loop_invariant; /*  supporting the case that idx value is loop invariant */
  bool const_loop_invariant_index;
} ArrayAccessInfo;

// This enum represents the kinds of reductions that we support.
enum ReductionKind {
  RK_NoReduction, // Not a reduction.
  RK_IntegerAdd,  // 1: Sum of integers.
  RK_IntegerMult, // 2: Product of integers.
  RK_IntegerOr,   // 3: Bitwise or logical OR of numbers.
  RK_IntegerAnd,  // 4: Bitwise or logical AND of numbers.
  RK_IntegerXor,  // 5: Bitwise or logical XOR of numbers.
  RK_IntegerMinMax, // 6: Min/max implemented in terms of select(cmp()).
  RK_FloatAdd,    // 7: Sum of floats.
  RK_FloatMult,   // 8: Product of floats.
  RK_FloatMinMax,  // 9: Min/max implemented in terms of select(cmp()).
  RK_IntegerSub,  // 10
  RK_FloatSub,    // 11
  RK_IntegerDiv,  // 12
  RK_FloatDiv     // 13
};

// This struct holds information about reduction variables.
typedef struct RVInfo {
  HInstruction* phi;
  HInstruction* init_input;
  HInstruction* cycle_input;

  // The instruction who's value is used outside the loop.
  HInstruction* LoopExitInstr;

  HInstruction* TypeCastInst;

  // The kind of the reduction.
  ReductionKind Kind;

  bool isConstInitVal;

  bool is64bits;

  int64_t init_val;

} RVInfo;

}  // namespace art.

#endif  // ART_COMPILER_OPTIMIZING_EXYNOS_SEBB_LOOP_INFO_H_
