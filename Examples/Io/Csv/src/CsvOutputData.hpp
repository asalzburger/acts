// This file is part of the Acts project.
//
// Copyright (C) 2019 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

/// @file
/// @brief Plain structs that each define one row in a TrackML csv file

#pragma once

#include <cstdint>

#include <dfe/dfe_namedtuple.hpp>

namespace ActsExamples {

struct ParticleData {
  /// Event-unique particle identifier a.k.a barcode.
  uint64_t particle_id;
  /// Particle type number a.k.a. PDG particle number.
  int32_t particle_type;
  /// Production process type.
  uint32_t process = 0u;
  /// Production position components in mm.
  float vx, vy, vz;
  // Production time in ns.
  float vt = 0.0f;
  /// Momentum components in GeV.
  float px, py, pz;
  /// Mass in GeV. Not available in the TrackML datasets
  float m = 0.0f;
  /// Charge in e.
  float q;

  DFE_NAMEDTUPLE(ParticleData, particle_id, particle_type, process, vx, vy, vz,
                 vt, px, py, pz, m, q);
};

// Write out simhits before digitization (no hi_id associated)
struct SimHitData {
  /// Hit surface identifier.
  uint64_t geometry_id = 0u;
  /// Event-unique particle identifier of the generating particle.
  uint64_t particle_id;
  /// True global hit position components in mm.
  float tx, ty, tz;
  // True global hit time in ns.
  float tt = 0.0f;
  /// True particle momentum in GeV before interaction.
  float tpx, tpy, tpz;
  /// True particle energy in GeV before interaction.
  ///
  float te = 0.0f;
  /// True four-momentum change in GeV due to interaction.
  ///
  float deltapx = 0.0f;
  float deltapy = 0.0f;
  float deltapz = 0.0f;
  float deltae = 0.0f;
  // Hit index along the trajectory.
  int32_t index = -1;

  DFE_NAMEDTUPLE(SimHitData, particle_id, geometry_id, tx, ty, tz, tt, tpx, tpy,
                 tpz, te, deltapx, deltapy, deltapz, deltae, index);
};

struct TruthHitData {
  /// Event-unique measurement identifier. As defined for the simulated hit
  /// below and used to link back to it; same value can appear multiple times
  /// here due to shared measurements in dense environments.
  uint64_t measurement_id;
  /// Hit surface identifier.
  uint64_t geometry_id = 0u;
  /// Event-unique particle identifier of the generating particle.
  uint64_t particle_id;
  /// True global hit position components in mm.
  float tx, ty, tz;
  // True global hit time in ns.
  float tt = 0.0f;
  /// True particle momentum in GeV before interaction.
  float tpx, tpy, tpz;
  /// True particle energy in GeV before interaction.
  float te = 0.0f;
  /// True four-momentum change in GeV due to interaction.
  float deltapx = 0.0f;
  float deltapy = 0.0f;
  float deltapz = 0.0f;
  float deltae = 0.0f;
  // Hit index along the trajectory.
  int32_t index = -1;

  DFE_NAMEDTUPLE(TruthHitData, measurement_id, particle_id, geometry_id, tx, ty,
                 tz, tt, tpx, tpy, tpz, te, deltapx, deltapy, deltapz, deltae,
                 index);
};

struct MeasurementData {
  /// Event-unique measurement identifier. Each value can appear at most once.
  uint64_t measurement_id;
  /// Hit surface identifier.
  uint64_t geometry_id = 0u;
  /// Partially decoded hit surface identifier components.
  uint32_t volume_id, layer_id, module_id;
  /// Local hit information - bit identification what's measured
  uint8_t local_key;
  float local0, local1, phi, theta, time;
  float cov0, cov1, covPhi, covTheta, covTime;

  DFE_NAMEDTUPLE(MeasurementData, measurement_id, geometry_id, volume_id,
                 layer_id, module_id, local0, local1, phi, theta, time, cov0,
                 cov1, covPhi, covTheta, covTime);
};

struct CellData {
  /// For reconstruction:
  /// Event-unique measurement identifier. As defined for the  measurement above
  /// and used to link back to it; same value can appear multiple times for
  /// clusters with more than one active cell/channel.
  ///
  /// For truth clusters:
  /// Particle-unique identifier
  uint64_t association_id;
  /// Digital cell address/channel identifier.
  int32_t channel0, channel1;
  /// Digital cell timestamp.
  int32_t timestamp = 0;
  /// (Digital) measured cell value, e.g. amplitude or time-over-threshold.
  float value;

  DFE_NAMEDTUPLE(CellData, association_id, channel0, channel1, timestamp,
                 value);
};

struct SurfaceData {
  /// Surface identifier.
  uint64_t geometry_id;
  /// Partially decoded surface identifier components.
  uint32_t volume_id, boundary_id, layer_id, module_id;
  /// Center position components in mm.
  float cx, cy, cz;
  /// Rotation matrix components.
  float rot_xu, rot_xv, rot_xw;
  float rot_yu, rot_yv, rot_yw;
  float rot_zu, rot_zv, rot_zw;
  /// The type of the surface bpounds object, determines the parameters filled
  int bounds_type;
  float bound_param0 = -1.f;
  float bound_param1 = -1.f;
  float bound_param2 = -1.f;
  float bound_param3 = -1.f;
  float bound_param4 = -1.f;
  float bound_param5 = -1.f;
  float bound_param6 = -1.f;

  DFE_NAMEDTUPLE(SurfaceData, geometry_id, volume_id, boundary_id, layer_id,
                 module_id, cx, cy, cz, rot_xu, rot_xv, rot_xw, rot_yu, rot_yv,
                 rot_yw, rot_zu, rot_zv, rot_zw, bounds_type, bound_param0,
                 bound_param1, bound_param2, bound_param3, bound_param4,
                 bound_param5, bound_param6);
};

}  // namespace ActsExamples
