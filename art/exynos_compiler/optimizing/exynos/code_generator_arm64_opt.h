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

#ifndef ART_COMPILER_OPTIMIZING_EXYNOS_CODE_GENERATOR_ARM64_OPT_H_
#define ART_COMPILER_OPTIMIZING_EXYNOS_CODE_GENERATOR_ARM64_OPT_H_

#include "../code_generator_arm64.h"
#include "nodes_opt.h"

namespace art {
namespace arm64 {

class InstructionCodeGeneratorARM64Opt : public InstructionCodeGeneratorARM64 {
 public:
  InstructionCodeGeneratorARM64Opt(HGraph* graph, CodeGeneratorARM64* codegen)
    : InstructionCodeGeneratorARM64(graph, codegen) {}

#define DECLARE_VISIT_INSTRUCTION(name, super) \
  void Visit##name(H##name* instr) OVERRIDE;

  FOR_EACH_PACKED_INSTRUCTION_ARM64(DECLARE_VISIT_INSTRUCTION)

#undef DECLARE_VISIT_INSTRUCTION

 private:
  void HandlePackedBinaryOp(HPackedBinaryOperation* instr);

  DISALLOW_COPY_AND_ASSIGN(InstructionCodeGeneratorARM64Opt);
};

class LocationsBuilderARM64Opt : public LocationsBuilderARM64 {
 public:
  LocationsBuilderARM64Opt(HGraph* graph, CodeGeneratorARM64* codegen)
    : LocationsBuilderARM64(graph, codegen) {}

#define DECLARE_VISIT_INSTRUCTION(name, super) \
  void Visit##name(H##name* instr) OVERRIDE;

  FOR_EACH_PACKED_INSTRUCTION_ARM64(DECLARE_VISIT_INSTRUCTION)

#undef DECLARE_VISIT_INSTRUCTION

 private:
  void HandlePackedBinaryOp(HPackedBinaryOperation* instr);

  DISALLOW_COPY_AND_ASSIGN(LocationsBuilderARM64Opt);
};

class ParallelMoveResolverARM64Opt : public ParallelMoveResolverARM64 {
 public:
  ParallelMoveResolverARM64Opt(ArenaAllocator* allocator, CodeGeneratorARM64* codegen)
      : ParallelMoveResolverARM64(allocator, codegen),
        codegen_(codegen) {}

 protected:
  void EmitMove(size_t index) OVERRIDE;

 private:
  CodeGeneratorARM64* const codegen_;

  DISALLOW_COPY_AND_ASSIGN(ParallelMoveResolverARM64Opt);
};

class CodeGeneratorARM64Opt : public CodeGeneratorARM64 {
 public:
  CodeGeneratorARM64Opt(HGraph* graph,
                        const Arm64InstructionSetFeatures& isa_features,
                        const CompilerOptions& compiler_options)
    : CodeGeneratorARM64(graph, isa_features, compiler_options),
      location_builder_opt_(graph, this),
      instruction_visitor_opt_(graph, this),
      move_resolver_opt_(graph->GetArena(), this) {}

  HGraphVisitor* GetLocationBuilder() OVERRIDE { return &location_builder_opt_; }
  HGraphVisitor* GetInstructionVisitor() OVERRIDE { return &instruction_visitor_opt_; }

  void MoveLocation(Location destination, Location source,
                    HInstruction* instruction,
                    Primitive::Type type = Primitive::kPrimVoid);

  ParallelMoveResolverARM64Opt* GetMoveResolver() OVERRIDE { return &move_resolver_opt_; }

 private:
  LocationsBuilderARM64Opt location_builder_opt_;
  InstructionCodeGeneratorARM64Opt instruction_visitor_opt_;
  ParallelMoveResolverARM64Opt move_resolver_opt_;

  DISALLOW_COPY_AND_ASSIGN(CodeGeneratorARM64Opt);
};

}  // namespace arm64
}  // namespace art

#endif  // ART_COMPILER_OPTIMIZING_EXYNOS_CODE_GENERATOR_ARM64_OPT_H_
