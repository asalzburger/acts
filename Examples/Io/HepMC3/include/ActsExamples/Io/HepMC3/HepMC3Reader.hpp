// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/IReader.hpp"

#include <filesystem>
#include <mutex>
#include <string>

namespace HepMC3 {
class GenEvent;
class Reader;
}  // namespace HepMC3

namespace ActsExamples {

/// HepMC3 event reader.
class HepMC3Reader final : public IReader {
 public:
  struct Config {
    /// The input file path for reading HepMC3 events.
    ///
    /// This path is handled differently based on the perEvent flag:
    /// - If perEvent is false: The path points to a single file containing all
    /// events
    /// - If perEvent is true: The path is used as a template for finding
    /// per-event files
    ///   in the format "event{number}-{filename}" in the parent directory
    ///
    /// When in per-event mode, the reader uses determineEventFilesRange() to
    /// scan the directory for matching files and determine the available event
    /// range.
    std::filesystem::path inputPath;

    /// If true, one file per event is read
    bool perEvent = false;

    /// The output collection
    std::string outputEvent;

    /// If true, print the event listing
    bool printListing = false;

    /// HepMC3 does not expose the number of events in the file, so we need to
    /// provide it here if known, otherwise the reader will have to read the
    /// whole file
    std::optional<std::size_t> numEvents = std::nullopt;

    /// If true, the reader will check if the read `HepMC3::GenEvent` has the
    /// same event number as the internal one.
    /// This will only be correct if the events were written in sequential order
    /// and numbered correctly.
    bool checkEventNumber = true;

    /// In multi-threading mode, the reader will need to buffer events to read
    /// them predictably and in order. This defines the maximum queue size being
    /// used. If this number is exceeded the reader will error out.
    std::size_t maxEventBufferSize = 128;
  };

  /// Construct the particle reader.
  ///
  /// @param [in] cfg The configuration object
  /// @param [in] lvl The logging level
  HepMC3Reader(const Config& cfg, Acts::Logging::Level lvl);

  ~HepMC3Reader() override;

  std::string name() const override;

  /// Return the available events range.
  std::pair<std::size_t, std::size_t> availableEvents() const override;

  /// Read out data from the input stream.
  ProcessCode read(const ActsExamples::AlgorithmContext& ctx) override;

  ProcessCode finalize() override;

  /// Get readonly access to the config parameters
  const Config& config() const { return m_cfg; }

  ProcessCode skip(std::size_t events) override;

 private:
  std::size_t determineNumEvents(HepMC3::Reader& reader) const;

  std::shared_ptr<HepMC3::Reader> makeReader() const;

  static std::shared_ptr<HepMC3::GenEvent> makeEvent();

  ProcessCode readPerEvent(const ActsExamples::AlgorithmContext& ctx,
                           std::shared_ptr<HepMC3::GenEvent>& event);

  ProcessCode readSingleFile(const ActsExamples::AlgorithmContext& ctx,
                             std::shared_ptr<HepMC3::GenEvent>& event);

  ProcessCode readCached(const ActsExamples::AlgorithmContext& ctx,
                         std::shared_ptr<HepMC3::GenEvent>& event);

  ProcessCode readBuffer(const ActsExamples::AlgorithmContext& ctx,
                         std::shared_ptr<HepMC3::GenEvent>& event);

  /// The configuration of this writer
  Config m_cfg;
  /// Number of events
  std::pair<std::size_t, std::size_t> m_eventsRange;
  /// The logger
  std::unique_ptr<const Acts::Logger> m_logger;

  const Acts::Logger& logger() const { return *m_logger; }

  WriteDataHandle<std::shared_ptr<HepMC3::GenEvent>> m_outputEvent{
      this, "OutputEvent"};

  std::vector<std::pair<std::size_t, std::shared_ptr<HepMC3::GenEvent>>>
      m_events;
  std::size_t m_nextEvent = 0;

  std::size_t m_maxEventBufferSize = 0;

  bool m_bufferError = false;

  std::mutex m_mutex;

  std::shared_ptr<HepMC3::Reader> m_reader;
};

}  // namespace ActsExamples
