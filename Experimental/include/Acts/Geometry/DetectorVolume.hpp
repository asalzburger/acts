// This file is part of the Acts project.
//
// Copyright (C) 2020 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <climits>
#include <vector>

#include "Acts/Geometry/BoundaryPortal.hpp"
#include "Acts/Geometry/ContainerStructure.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/GeometryObject.hpp"
#include "Acts/Geometry/IVolumeStructure.hpp"
#include "Acts/Geometry/NavigationOptions.hpp"
#include "Acts/Material/IVolumeMaterial.hpp"
#include "Acts/Utilities/Definitions.hpp"
#include "Acts/Utilities/Intersection.hpp"

namespace Acts {

class VolumeBounds;

/// @brief Volume class for the description of the TrackingGeometry
///
/// The DetectorVolume object is characterised by the boundary volume
/// portal mechanism, i.e. between DetectorVolumes the boundary portals
/// move you from one to another.
///
/// It can come with different internal descriptions, which steer the
/// behavior:
/// - as a container volume
/// - as a layer description
/// - as a bounding box hierarchical descrription
///
/// @note That the descriptors contain the bounds of the object
class DetectorVolume : public GeometryObject {
  
  friend DetectorVolume;

 public:
  /// Detector volume as shared pointer
  using DetectorVolumePtr = std::shared_ptr<DetectorVolume>;

  /// Boundary portal short hands
  using BoundaryPortal = BoundaryPortal<DetectorVolume>;
  using BoundaryPortalPtr = std::shared_ptr<BoundaryPortal>;
  using BoundaryPortalPtrVector = std::vector<BoundaryPortalPtr>;
  using BoundaryPortalIntersection =
      ObjectIntersection<BoundaryPortal, Surface>;
  using BoundaryPortalCandidates = std::vector<BoundaryPortalIntersection>;
  using BoundaryOptions = NavigationOptions<BoundaryPortal>;

  /// Surface intersection short hands
  using SurfaceIntersection = ObjectIntersection<Surface>;
  using SurfaceCandidates = std::vector<SurfaceIntersection>;
  using SurfaceOptions = NavigationOptions<Surface>;

  /// Defaulted Destructor
  virtual ~DetectorVolume(){};

  /// Factory for producing memory managed instances of DetectoVolumes.
  /// Will forward all parameters and will attempt to find a suitable
  /// constructor.
  template <typename... Args>
  static DetectorVolumePtr makeShared(Args&&... args) {
    return DetectorVolumePtr(new DetectorVolume(std::forward<Args>(args)...));
  }

  /// Return the VolumeBounds
  const VolumeBounds& volumeBounds() const;

  /// Return methods for geometry transform
  ///
  /// @param gctx The geometry context, it will allow in future to
  /// also unpack the transform context for potential large scale
  /// misalignment
  const Transform3D& transform(const GeometryContext& gctx) const;

  /// Return methods for geometry inverse transform
  ///
  /// @param gctx The geometry context, it will allow in future to
  /// also unpack the transform context for potential large scale
  /// misalignment
  const Transform3D& inverseTransform(const GeometryContext& gctx) const;

  /// Return method for the center() access
  ///
  /// @param gctx The geometry context, it will allow in future to
  /// also unpack the transform context for potential large scale
  /// misalignment
  const Vector3D center(const GeometryContext& gctx) const;

  /// Inside check for this volume
  ///
  /// @param gctx The geometry context
  /// @param position The position for the inside check
  /// @param tolerance The inside tolerance
  bool inside(const GeometryContext& gctx, const Vector3D& position,
              double tolerance = s_onSurfaceTolerance) const;

  /// Attaching another volume to this one, with stitching option
  ///
  /// It will check if any of the surfaces can work for attachment,
  /// and throw a domain error if that's not possible. If attachment
  /// is possible, the portal surface of dvolume is used and the 
  /// attached volumes are set.
  ///
  /// If stitching is chosen, the connecting surfaces are stitched
  /// together and the attached volumes are set.
  ///
  /// @param dvolume The detector volume to attach to it
  /// @param stitch A boolean directive whether to stich 
  void attach(DetectorVolumePtr& dvolume, bool stitch=true);

  /// Return method for the detector volume in the boundary portal
  /// world, i.e this is the lowest volume in hierarchy and by
  /// definition can not have a hierarchy volume anymore
  /// You can only leave it through a boundary portal again
  ///
  /// @param gctx The geometry context for the volume search
  /// @param position The global search position
  const DetectorVolume* portalVolume(const GeometryContext& gctx,
                                     const Vector3D& position) const;

  /// @brief Returns all boundary portals
  ///
  /// @tparam options_t Type of navigation options object for decomposition
  ///
  /// @param gctx The current geometry context object, e.g. alignment
  /// @param position The position for searching
  /// @param direction The direction for searching
  /// @param options The templated navigation options - here foir boundary
  /// portals
  ///
  /// @return is the templated boundary intersection
  BoundaryPortalCandidates boundaryPortalCandidates(
      const GeometryContext& gctx, const Vector3D& position,
      const Vector3D& direction, const BoundaryOptions& options) const;

  /// @brief Returns all surface candidates - boundaries excluded
  ///
  /// @tparam options_t Type of navigation options object for decomposition
  ///
  /// @param gctx The current geometry context object, e.g. alignment
  /// @param position The position for searching
  /// @param direction The direction for searching
  /// @param options The templated navigation options - here for surfaces
  ///
  /// @return is the templated boundary intersection
  SurfaceCandidates surfaceCandidates(const GeometryContext& gctx,
                                      const Vector3D& position,
                                      const Vector3D& direction,
                                      const SurfaceOptions& options) const;

  /// Return the boundary portals
  const BoundaryPortalPtrVector& boundaryPortals() const;

  /// Return the container structure
  const ContainerStructure* containerStructure() const;

  /// Return the layer structure
  const IVolumeStructure* volumeStructure() const;

  /// Return the volume material
  const IVolumeMaterial* volumeMaterial() const;

  /// Return the volume name
  const std::string& volumeName() const;

  /// The binning position method
  /// - as default the center is given, but may be overloaded
  ///
  /// @param gctx The current geometry context object, e.g. alignment
  /// @param bValue is the binning value schema
  ///
  /// @return vector 3D that can be used for the binning
  const Vector3D binningPosition(const GeometryContext& gctx,
                                 BinningValue bValue) const final;

 protected:
  /// Constructor for container volumes
  /// @param transform A detector transform
  /// @param container The container description
  /// @param name The name given to this volume
  DetectorVolume(const Transform3D& transform,
                 std::unique_ptr<ContainerStructure> container,
                 const std::string& name = "Unnamed");

  /// Constructor for a volume: describing a layer/hierarchy structure
  /// @param transform A detector transform
  /// @param structure The layer / volume structure
  /// @param volumeMaterial The (optional) volume material description
  /// @param name The name given to this volume
  DetectorVolume(const Transform3D& transform,
                 std::unique_ptr<IVolumeStructure> structure,
                 std::unique_ptr<IVolumeMaterial> volumeMaterial,
                 const std::string& name = "Unnamed");

 private:
  /// Create Boundary Portals from the VolumeBpounds
  /// this is called by the constructor
  void createBoundaryPortals();

  /// The volume transform
  Transform3D m_transform = Transform3D::Identity();

  /// The invere volume transform
  Transform3D m_inverseTransform = Transform3D::Identity();

  // The boundary surfaces
  BoundaryPortalPtrVector m_boundaryPortals = {};

  /// The volume bounds, assigned at contruction by the structure
  const VolumeBounds* m_volumeBounds = nullptr;

  /// The Container structure - nullptr when not a container
  std::unique_ptr<ContainerStructure> m_containerStructure = nullptr;

  /// The Layer structure - can be optionally layer or hierarchy
  std::unique_ptr<IVolumeStructure> m_volumeStructure = nullptr;

  /// The Volume material
  std::unique_ptr<IVolumeMaterial> m_volumeMaterial = nullptr;

  /// The name of the volume
  std::string m_volumeName = "Unnamed";
};

#include "detail/DetectorVolume.ipp"

}  // namespace Acts
