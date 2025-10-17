// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsPlugins/DD4hep/DD4hepProcessors.hpp"

void ActsPlugins::DD4hepGraphVizPrinter::operator()(
    const dd4hep::DetElement& detElement){

    stream << "  \"" << detElement.name() << "_" << detElement.id() << "\""
             << " [label=\"" << detElement.name() << "\\nID: " << detElement.id()
             << "\"];" << '\n';

}