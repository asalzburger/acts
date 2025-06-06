// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Plugins/ExaTrkX/Stages.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <memory>
#include <vector>

namespace nvinfer1 {
class IRuntime;
class ICudaEngine;
class ILogger;
class IExecutionContext;
}  // namespace nvinfer1

namespace Acts {

class TensorRTEdgeClassifier final : public EdgeClassificationBase {
 public:
  struct Config {
    std::string modelPath;
    std::vector<int> selectedFeatures;
    float cut = 0.5;

    std::size_t numExecutionContexts = 1;
  };

  TensorRTEdgeClassifier(const Config &cfg,
                         std::unique_ptr<const Logger> logger);
  ~TensorRTEdgeClassifier();

  PipelineTensors operator()(PipelineTensors tensors,
                             const ExecutionContext &execContext = {}) override;

  Config config() const { return m_cfg; }

 private:
  std::unique_ptr<const Acts::Logger> m_logger;
  const auto &logger() const { return *m_logger; }

  Config m_cfg;

  std::unique_ptr<nvinfer1::IRuntime> m_runtime;
  std::unique_ptr<nvinfer1::ICudaEngine> m_engine;
  std::unique_ptr<nvinfer1::ILogger> m_trtLogger;

  mutable std::mutex m_contextMutex;
  mutable std::vector<std::unique_ptr<nvinfer1::IExecutionContext>> m_contexts;
};

}  // namespace Acts
