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

#ifndef ART_COMPILER_OPTIMIZING_EXYNOS_NODES_OPT_H_
#define ART_COMPILER_OPTIMIZING_EXYNOS_NODES_OPT_H_

#include "../nodes.h"
#include "base/bit_vector-inl.h"

namespace art {

class HGraphOpt : public HGraph {
 public:
  HGraphOpt(ArenaAllocator* arena,
            const DexFile& dex_file,
            uint32_t method_idx,
            InstructionSet instruction_set,
            bool debuggable = false,
            bool opt_on = false);

  virtual ~HGraphOpt() {}

  bool FrontEndPassGate(uint32_t flag);
  bool BackEndPassGate(uint32_t flag);

  virtual bool TryBuildingSsa();
  virtual void SimplifyLoop(HBasicBlock* header);

  void AnalyzeSingleEBBLoop();
  bool IsInnermostLoop(HBasicBlock* header);
  void FindSingleEBBLoop(HBasicBlock* header);
  void LoopInversionForSingleEBBLoop();
  bool IsCopyableInstruction(HInstruction* instruction);
  void MakeLoopPreHeader();
  void CopyFrom(HBasicBlock* block, HInstruction* instruction,
                GrowableArray<HInstruction*>* local_list, bool reverse_condition,
                HLoopInformation* loop_info);
  void ComputeImmediateDominator();
  void SplitCriticalEdgesAfterLoopInversion();
  void SplitCriticalEdgeAfterLoopInversion(HBasicBlock* block, HBasicBlock* successor);
  bool AnalyzeNaturalLoopsAfterLoopInversion() const;

  uint32_t exynos_front_opt_enable;
  uint32_t exynos_front_opt_applied;
  uint32_t exynos_back_opt_enable;
  uint32_t exynos_back_opt_applied;

  bool exynos_filter_on;
  uint32_t exynos_filter_front_opt_enable;
  uint32_t exynos_filter_back_opt_enable;

  std::set<HBasicBlock*> blocks_with_packed_hirs_;

private:
  GrowableArray<HBasicBlock*> sebb_loop_headers_;
  DISALLOW_COPY_AND_ASSIGN(HGraphOpt);
};

}  // namespace art

#endif  // ART_COMPILER_OPTIMIZING_EXYNOS_NODES_OPT_H_
