/*
 * Copyright (C) 2014 The Android Open Source Project
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

#ifndef ART_COMPILER_OPTIMIZING_OPTIMIZING_COMPILER_H_
#define ART_COMPILER_OPTIMIZING_OPTIMIZING_COMPILER_H_

#ifdef EXYNOS_ART_OPT
#include "base/dumpable.h"
#include "code_generator.h"
#include "driver/compiler_driver.h"
#include "graph_visualizer.h"
#include "nodes.h"
#endif

namespace art {

class Compiler;
class CompilerDriver;

Compiler* CreateOptimizingCompiler(CompilerDriver* driver);

// Returns whether we are compiling against a "core" image, which
// is an indicative we are running tests. The compiler will use that
// information for checking invariants.
bool IsCompilingWithCoreImage();

#ifdef EXYNOS_ART_OPT
/**
 * Filter to apply to the visualizer. Methods whose name contain that filter will
 * be dumped.
 */
static const char* kStringFilter = "";

class PassInfo;

class PassInfoPrinter : public ValueObject {
 public:
  PassInfoPrinter(HGraph* graph,
                  const char* method_name,
                  const CodeGenerator& codegen,
                  std::ostream* visualizer_output,
                  CompilerDriver* compiler_driver)
      : method_name_(method_name),
        timing_logger_enabled_(compiler_driver->GetDumpPasses()),
        timing_logger_(method_name, true, true),
        visualizer_enabled_(!compiler_driver->GetDumpCfgFileName().empty()),
        visualizer_(visualizer_output, graph, codegen) {
    if (strstr(method_name, kStringFilter) == nullptr) {
      timing_logger_enabled_ = visualizer_enabled_ = false;
    }
    if (visualizer_enabled_) {
      visualizer_.PrintHeader(method_name_);
    }
  }

  ~PassInfoPrinter() {
    if (timing_logger_enabled_) {
      LOG(INFO) << "TIMINGS " << method_name_;
      LOG(INFO) << Dumpable<TimingLogger>(timing_logger_);
    }
  }

 private:
  void StartPass(const char* pass_name) {
    // Dump graph first, then start timer.
    if (visualizer_enabled_) {
      visualizer_.DumpGraph(pass_name, /* is_after_pass */ false);
    }
    if (timing_logger_enabled_) {
      timing_logger_.StartTiming(pass_name);
    }
  }

  void EndPass(const char* pass_name) {
    // Pause timer first, then dump graph.
    if (timing_logger_enabled_) {
      timing_logger_.EndTiming();
    }
    if (visualizer_enabled_) {
      visualizer_.DumpGraph(pass_name, /* is_after_pass */ true);
    }
  }

  const char* method_name_;

  bool timing_logger_enabled_;
  TimingLogger timing_logger_;

  bool visualizer_enabled_;
  HGraphVisualizer visualizer_;

  friend PassInfo;

  DISALLOW_COPY_AND_ASSIGN(PassInfoPrinter);
};

class PassInfo : public ValueObject {
 public:
  PassInfo(const char *pass_name, PassInfoPrinter* pass_info_printer)
      : pass_name_(pass_name),
        pass_info_printer_(pass_info_printer) {
    pass_info_printer_->StartPass(pass_name_);
  }

  ~PassInfo() {
    pass_info_printer_->EndPass(pass_name_);
  }

 private:
  const char* const pass_name_;
  PassInfoPrinter* const pass_info_printer_;
};
#endif

}  // namespace art

#endif  // ART_COMPILER_OPTIMIZING_OPTIMIZING_COMPILER_H_
