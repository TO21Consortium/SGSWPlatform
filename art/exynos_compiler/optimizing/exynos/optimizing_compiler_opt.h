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

#ifndef ART_COMPILER_OPTIMIZING_EXYNOS_OPTIMIZING_COMPILER_OPT_H_
#define ART_COMPILER_OPTIMIZING_EXYNOS_OPTIMIZING_COMPILER_OPT_H_

#include "../optimizing_compiler.h"

namespace art {

extern bool ExynosAllocateRegisters(HGraph* graph,
                                    CodeGenerator* codegen,
                                    PassInfoPrinter* pass_info_printer);

extern void RunExynosOptimizations(HGraph* graph,
                             CompilerDriver* driver,
                             OptimizingCompilerStats* stats,
                             const DexFile& dex_file,
                             const DexCompilationUnit& dex_compilation_unit,
                             PassInfoPrinter* pass_info_printer,
                             StackHandleScopeCollection* handles);
}  // namespace art

#endif  // ART_COMPILER_OPTIMIZING_EXYNOS_OPTIMIZING_COMPILER_OPT_H_
