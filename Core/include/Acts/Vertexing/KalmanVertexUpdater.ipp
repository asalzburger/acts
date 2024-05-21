// This file is part of the Acts project.
//
// Copyright (C) 2019-2023 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <algorithm>

template <unsigned int nDimVertex>
void Acts::KalmanVertexUpdater::updateVertexWithTrack(Vertex& vtx,
                                                      TrackAtVertex& trk) {
  std::pair<double, double> fitQuality = vtx.fitQuality();
  detail::updateVertexWithTrack(vtx.fullPosition(), vtx.fullCovariance(),
                                fitQuality, trk, 1, nDimVertex);
  vtx.setFitQuality(fitQuality);
}
