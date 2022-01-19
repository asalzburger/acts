// This file is part of the Acts project.
//
// Copyright (C) 2022 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <boost/test/data/test_case.hpp>
#include <boost/test/tools/output_test_stream.hpp>
#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Experimental/DetectorEnvironment.hpp"
#include "Acts/Experimental/Portal.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Material/HomogeneousSurfaceMaterial.hpp"
#include "Acts/Surfaces/CylinderSurface.hpp"
#include "Acts/Utilities/Delegate.hpp"

#include <iostream>

namespace Acts {

namespace Test {

/// Test struct to place check the correct assignment of PortalLink delegates
struct TestPortalLink {
  size_t surfaceCandidates = 0;
  size_t portalCandidates = 0;

  /// @note everything is unused here
  DetectorEnvironment link(const GeometryContext&, const Portal&,
                           const Vector3&, const Vector3&, ActsScalar,
                           ActsScalar, const BoundaryCheck&,
                           const std::array<ActsScalar, 2>&, bool) const {
    DetectorEnvironment dEnv;
    if (surfaceCandidates > 0) {
      dEnv.surfaces = std::vector<SurfaceIntersection>(surfaceCandidates,
                                                       SurfaceIntersection());
    }
    if (portalCandidates > 0) {
      dEnv.portals = std::vector<PortalIntersection>(portalCandidates,
                                                     PortalIntersection());
    }
    return dEnv;
  }
};

BOOST_AUTO_TEST_SUITE(Experimental)

/// Unit tests for Polyderon construction & operator +=
BOOST_AUTO_TEST_CASE(Portal_) {
  // First we create a surface
  auto surface =
      Surface::makeShared<CylinderSurface>(Transform3::Identity(), 10., 200.);
  // Then a portal
  Portal portal(surface);
  // & check if the surface is properly set
  BOOST_CHECK(&portal.surfaceRepresentation() == surface.get());

  // Assign material to the portal
  auto material = std::make_shared<HomogeneousSurfaceMaterial>();
  portal.assignSurfaceMaterial(material);
  // & check it is not zero
  BOOST_CHECK(portal.surfaceRepresentation().surfaceMaterial() != nullptr);
  // & check it is properly set
  BOOST_CHECK(portal.surfaceRepresentation().surfaceMaterial() ==
              material.get());

  // Assign the geometetry ID
  portal.assignGeometryId(GeometryIdentifier().setLayer(2));
  // & check that is is properly set
  BOOST_CHECK(portal.surfaceRepresentation().geometryId().layer() == 2);

  // Test the portal intersection, let's start a bit off (0,0,0)
  Vector3 start(0.1, 0.0, 0.);
  Vector3 direction = Vector3(1., 1., 1.).normalized();
  GeometryContext geoContext;
  // Intersect portal first
  auto portalIntersection = portal.intersect(geoContext, start, direction);
  // Then intersect the surface
  auto surfaceIntersection =
      surface->intersect(geoContext, start, direction, true);

  BOOST_CHECK(portalIntersection.intersection.position.isApprox(
      surfaceIntersection.intersection.position));
  BOOST_CHECK(portalIntersection.intersection);

  // Now unreachable within bounds
  Vector3 forwardDirection = Vector3(1., 1., 500.).normalized();
  // Intersect portal to create an outside intersection
  auto portalOutsideIntersection =
      portal.intersect(geoContext, start, forwardDirection);
  // & check that the intersection is indeed not valid
  BOOST_CHECK(not portalOutsideIntersection);

  // Unset portal gives unset detector environment
  auto detectorEnvironment =
      portal.next(geoContext, start, direction, 100, 1., true);
  // & test that it is indeed unset
  BOOST_CHECK(detectorEnvironment.currentSurface == nullptr);
  BOOST_CHECK(detectorEnvironment.currentVolume == nullptr);
  BOOST_CHECK(detectorEnvironment.surfaces.empty());
  BOOST_CHECK(detectorEnvironment.portals.empty());

  // Check that portal delegates are not connected
  BOOST_CHECK(not portal.portalLink(backward).connected());
  BOOST_CHECK(not portal.portalLink(forward).connected());

  // Create portal links, connect to delegates & check
  TestPortalLink oppositeLinkImpl{1, 3};
  TestPortalLink alongLinkImpl{10, 4};

  PortalLink oppositeLink;
  oppositeLink.connect<&TestPortalLink::link>(&oppositeLinkImpl);
  PortalLink alongLink;
  alongLink.connect<&TestPortalLink::link>(&alongLinkImpl);
  // & update the portal links
  portal.updatePortalLink(std::move(oppositeLink), backward);
  portal.updatePortalLink(std::move(alongLink), forward);

  // Check that portal delegates are indeed connected now
  BOOST_CHECK(portal.portalLink(backward).connected());
  BOOST_CHECK(portal.portalLink(forward).connected());

  // We are at the portal & let's check if the links work
  Vector3 positionAtPortal(10., 0., 0.);
  Vector3 directionAtPortal(1., 0., 0.);
  detectorEnvironment = portal.next(geoContext, positionAtPortal,
                                    -directionAtPortal, 100, 1., true);
  BOOST_CHECK(detectorEnvironment.surfaces.size() == 1);
  BOOST_CHECK(detectorEnvironment.portals.size() == 3);

  detectorEnvironment = portal.next(geoContext, positionAtPortal,
                                    directionAtPortal, 100, 1., true);
  BOOST_CHECK(detectorEnvironment.surfaces.size() == 10);
  BOOST_CHECK(detectorEnvironment.portals.size() == 4);

  // Hide behind scope to check ownership survival
  {
    // Create another round of portal link implementations as shared_ptr
    auto oppositeLinkImplPtr =
        std::make_shared<TestPortalLink>(TestPortalLink{2, 3});
    auto alongLinkImplPtr =
        std::make_shared<TestPortalLink>(TestPortalLink{11, 4});

    PortalLink oppositeLinkPtr;
    oppositeLinkPtr.connect<&TestPortalLink::link>(oppositeLinkImplPtr.get());
    PortalLink alongLinkPtr;
    alongLinkPtr.connect<&TestPortalLink::link>(alongLinkImplPtr.get());

    // & update the portal links
    portal.updatePortalLink(std::move(oppositeLinkPtr), backward,
                            oppositeLinkImplPtr);
    portal.updatePortalLink(std::move(alongLinkPtr), forward, alongLinkImplPtr);
  }

  detectorEnvironment = portal.next(geoContext, positionAtPortal,
                                    -directionAtPortal, 100, 1., true);
  BOOST_CHECK(detectorEnvironment.surfaces.size() == 2);
  BOOST_CHECK(detectorEnvironment.portals.size() == 3);

  detectorEnvironment = portal.next(geoContext, positionAtPortal,
                                    directionAtPortal, 100, 1., true);
  BOOST_CHECK(detectorEnvironment.surfaces.size() == 11);
  BOOST_CHECK(detectorEnvironment.portals.size() == 4);

}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Test
}  // namespace Acts