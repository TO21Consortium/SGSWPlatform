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

#ifndef ART_COMPILER_DEX_EXYNOS_LOOP_INFORMATION_H_
#define ART_COMPILER_DEX_EXYNOS_LOOP_INFORMATION_H_

#include "../compiler_internals.h"
#include "utils/growable_array.h"

namespace art {

#define MAX_LOOP_BLOCKS  50

class LoopInformation {
 public:
  LoopInformation(CompilationUnit* cu)
    : cu_(cu), arena_(&cu->arena), mir_graph_(cu->mir_graph.get()) {
  }

  virtual ~LoopInformation() {}

  CompilationUnit* GetCompilationUnit() const {
    return cu_;
  }

  virtual BasicBlockId GetEntryBlock() const {
    return entry_;
  }

  virtual void SetEntryBlock(BasicBlockId bb) {
    entry_ = bb;
  }

  virtual int GetDepth() const {
    return depth_;
  }
  virtual void SetDepth(int depth) {
    depth_ = depth;
  }

  virtual bool verifyLoop() const { return false; }

  // Allocate a new ArenaObject of 'size' bytes in the Arena.
  void* operator new(size_t size, ArenaAllocator* allocator) {
    return allocator->Alloc(size, kArenaAllocMisc);
  }
 protected:
  CompilationUnit* const cu_;
  ArenaAllocator* const arena_;
  MIRGraph* const mir_graph_;

  unsigned int depth_;
  BasicBlockId entry_;
};

}  // namespace art

#endif  // ART_COMPILER_DEX_EXYNOS_LOOP_INFORMATION_H_
