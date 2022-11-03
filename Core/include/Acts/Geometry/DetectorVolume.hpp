// This file is part of the Acts project.
//
// Copyright (C) 2022 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Common.hpp"
#include "Acts/Geometry/Extent.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/NavigationDelegates.hpp"
#include "Acts/Geometry/NavigationState.hpp"
#include "Acts/Geometry/VolumeBounds.hpp"
#include "Acts/Material/IVolumeMaterial.hpp"
#include "Acts/Surfaces/BoundaryCheck.hpp"
#include "Acts/Utilities/Delegate.hpp"
#include "Acts/Utilities/Helpers.hpp"

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace Acts {

class Surface;

namespace Experimental {

class DetectorVolume;
class Portal;

/// The Portal genertor definition
///
/// @param gctx the geometry context
/// @param bounds the volume bounds
/// @param volume the detector volume for which this generator is called
using PortalGenerator = Delegate<std::vector<std::shared_ptr<Portal>>(
    const Transform3& gctx, const VolumeBounds& bounds,
    std::shared_ptr<DetectorVolume> volume)>;

/// A detector volume description which can be:
///
/// @note A detector volume holds non-const objects internally
/// that are allowed to be modified as long as the geometry
/// is not yet closed. Using this, material can be attached,
/// and GeometryIdentifier can be set at construction time.
///
/// @note The construction of DetectorVolumes is done via a dedicated
/// factory, this is necessary as then the shared_ptr is non-weak and it
/// can be registred in the portal generator for further geometry processing.
///
/// @note Navigation is always done by plain pointers, while
/// object ownership is done by shared/unique pointers.
class DetectorVolume : public std::enable_shared_from_this<DetectorVolume> {
 public:
  friend class DetectorVolumeFactory;

  /// Nested object store that holds the internal (non-const),
  /// reference counted objects and provides an external
  /// (const raw pointer) access
  ///
  /// @tparam internal_t is the internal storage representation,
  /// has to comply with std::shared_ptr semantics
  ///
  template <typename internal_t>
  struct ObjectStore {
    /// The internal storage vector
    std::vector<internal_t> internal = {};

    /// The external storage vector, const raw pointer
    std::vector<const typename internal_t::element_type*> external = {};

    /// Store constructor
    ///
    /// @param objects are the ones copied into the internal store
    ObjectStore(const std::vector<internal_t>& objects) : internal(objects) {
      external = unpack_shared_const_vector(internal);
    }

    ObjectStore() = default;
  };

 protected:
  /// Create a detector volume - with surfaces and/or inserted volumes
  ///
  /// @param gctx the geometry context while building - for future contextual store
  /// @param name the volume name
  /// @param transform the transform defining the volume position
  /// @param bounds the volume bounds
  /// @param surfaces are the contained surfaces of this volume
  /// @param volumes are the containes volumes of this volume
  /// @param navStateUpdator the navigation state update
  ///
  /// @note throws exception if misconfigured: no bounds
  /// @note throws exception if ghe portal general or navigation
  ///       state updator delegates are not connected
  DetectorVolume(
      [[maybe_unused]] const GeometryContext& gctx, const std::string& name,
      const Transform3& transform, std::unique_ptr<VolumeBounds> bounds,
      const std::vector<std::shared_ptr<Surface>>& surfaces,
      const std::vector<std::shared_ptr<DetectorVolume>>& volumes,
      ManagedNavigationStateUpdator&& navStateUpdator) noexcept(false);

  /// Create a detector volume - empty/gap volume constructor
  ///
  /// @param gctx the geometry context while building - for future contextual store
  /// @param name the volume name
  /// @param transform the transform defining the volume position
  /// @param bounds the volume bounds
  /// @param navStateUpdator the navigation state update
  ///
  /// @note throws exception if misconfigured: no bounds
  /// @note throws exception if ghe portal general or navigation
  ///       state updator delegates are not connected
  DetectorVolume(
      [[maybe_unused]] const GeometryContext& gctx, const std::string& name,
      const Transform3& transform, std::unique_ptr<VolumeBounds> bounds,
      ManagedNavigationStateUpdator&& navStateUpdator) noexcept(false);

  /// Factory method for producing memory managed instances of DetectorVolume.
  /// Will forward all parameters and will attempt to find a suitable
  /// constructor.
  ///
  /// @note This is called by the @class DetectorVolumeFactory
  ///
  /// @tparam Args the arguments that will be forwarded
  template <typename... Args>
  static std::shared_ptr<DetectorVolume> makeShared(Args&&... args) {
    return std::shared_ptr<DetectorVolume>(
        new DetectorVolume(std::forward<Args>(args)...));
  }

 public:
  /// Retrieve a @c std::shared_ptr for this surface (non-const version)
  ///
  /// @note Will error if this was not created through the @c makeShared factory
  ///       since it needs access to the original reference. In C++14 this is
  ///       undefined behavior (but most likely implemented as a @c bad_weak_ptr
  ///       exception), in C++17 it is defined as that exception.
  /// @note Only call this if you need shared ownership of this object.
  ///
  /// @return The shared pointer
  std::shared_ptr<DetectorVolume> getSharedPtr();

  /// Retrieve a @c std::shared_ptr for this surface (const version)
  ///
  /// @note Will error if this was not created through the @c makeShared factory
  ///       since it needs access to the original reference. In C++14 this is
  ///       undefined behavior, but most likely implemented as a @c bad_weak_ptr
  ///       exception, in C++17 it is defined as that exception.
  /// @note Only call this if you need shared ownership of this object.
  ///
  /// @return The shared pointer
  std::shared_ptr<const DetectorVolume> getSharedPtr() const;

  /// Const access to the transform
  ///
  /// @param gctx the geometry contect
  ///
  /// @note the geometry context is currently ignored, but
  ///       is a placeholder for eventually misaligned volumes
  ///
  /// @return const reference to the contextual transform
  const Transform3& transform(
      const GeometryContext& gctx = GeometryContext()) const;

  /// Const access to the center
  ///
  /// @param gctx the geometry contect
  ///
  /// @note the geometry context is currently ignored, but
  ///       is a placeholder for eventually misaligned volumes
  ///
  /// @return a contextually created center
  Vector3 center(const GeometryContext& gctx = GeometryContext()) const;

  /// Const access to the volume bounds
  ///
  /// @return const reference to the volume bounds object
  const VolumeBounds& volumeBounds() const;

  /// Inside/outside method
  ///
  /// @param gctx the geometry context
  /// @param position the position for the inside check
  /// @param excludeInserts steers whether inserted volumes overwrite this
  ///
  /// @return a bool to indicate inside/outside
  bool inside(const GeometryContext& gctx, const Vector3& position,
              bool excludeInserts = true) const;

  /// The Extent for this volume
  ///
  /// @param gctx is the geometry context
  /// @param nseg is the number of segements to approximate
  ///
  /// @return an Extent object
  Extent extent(const GeometryContext& gctx, size_t nseg = 1) const;

  /// Initialize/update the navigation status in this environment
  ///
  /// This method calls:
  ///
  /// - the local navigation delegate for candidate surfaces
  /// - the portal navigation delegate for candidate exit portals
  /// - set the current detector volume
  ///
  /// @param gctx is the current geometry context
  /// @param nState [in,out] is the detector navigation state to be updated
  ///
  void updateNavigationState(const GeometryContext& gctx,
                             NavigationState& nState) const;

  /// Non-const access to the portals
  ///
  /// @return the portal shared pointer store
  std::vector<std::shared_ptr<Portal>>& portalPtrs();

  /// Non-const access to the surfaces
  ///
  /// @return the surfaces shared pointer store
  std::vector<std::shared_ptr<Surface>>& surfacePtrs();

  /// Non-const access to the volumes
  ///
  /// @return the volumes shared pointer store
  std::vector<std::shared_ptr<DetectorVolume>>& volumePtrs();

  /// Const access to the detector portals
  ///
  /// @note an empty vector indicates a container volume
  /// that has not been properly connected
  ///
  /// @return a vector to const Portal raw pointers
  const std::vector<const Portal*>& portals() const;

  /// Const access to the surfaces
  ///
  /// @note an empty vector indicates either gap volume
  /// or container volume, a non-empty vector indicates
  /// a layer volume.
  ///
  /// @return a vector to const Surface raw pointers
  const std::vector<const Surface*>& surfaces() const;

  /// Const access to sub volumes
  ///
  /// @note and empty vector indicates this is either a
  /// gap volume or a layer volume, in any case it means
  /// the volume is on navigation level and the portals
  /// need to be connected
  ///
  /// @return a vector to const DetectorVolume raw pointers
  const std::vector<const DetectorVolume*>& volumes() const;

  /// This method allows to udate the navigation state updator
  /// module.
  ///
  /// @param navStateUpdator the new navigation state updator
  /// @param surfaces the surfaces the new navigation state updator points to
  /// @param volumes the volumes the new navigation state updator points to
  ///
  void assignNavigationStateUpdator(
      ManagedNavigationStateUpdator&& navStateUpdator,
      const std::vector<std::shared_ptr<Surface>>& surfaces = {},
      const std::vector<std::shared_ptr<DetectorVolume>>& volumes = {});

  /// Const access to the navigation state updator
  const ManagedNavigationStateUpdator& navigationStateUpdator() const;

  /// Update a portal given a portal index
  ///
  /// @param portal the portal to be updated
  /// @param pIndex the portal index
  ///
  /// @note throws exception if portal index out of bounds
  void updatePortal(std::shared_ptr<Portal> portal,
                    unsigned int pIndex) noexcept(false);

  /// Assign the volume material description
  ///
  /// This method allows to load a material description during the
  /// detector geometry building, and assigning it (potentially multiple)
  /// times to detector volumes.
  ///
  /// @param material Material description associated to this volumw
  void assignVolumeMaterial(std::shared_ptr<IVolumeMaterial> material);

  /// Non-const access to the voume material (for scaling, e.g.)
  std::shared_ptr<IVolumeMaterial> volumeMaterialPtr();

  /// Const access to the volume amterial
  const IVolumeMaterial* volumeMaterial() const;

  /// Lock the geometry, this sets the GeometryIdentifier
  /// of the sub surfaces
  ///
  /// @param geometryId is the geometry base identifier of
  /// this detector volume
  void lock(
      const GeometryIdentifier& geometryId = GeometryIdentifier().setVolume(1));

  /// @return the name of the volume
  const std::string& name() const;

  /// @return the geometry identifier
  const GeometryIdentifier& geometryId() const;

 private:
  /// Internal construction method that calls the poral genenerator
  ///
  /// @param gctx the current geometry context object, e.g. alignment
  /// @param portalGenerator the generator for portals
  ///
  /// @note throws exception if provided parameters are inconsistent
  void construct(const GeometryContext& gctx,
                 const PortalGenerator& portalGenerator) noexcept(false);

  // Check containment - only in debug mode
  ///
  /// @param gctx the current geometry context object, e.g. alignment
  /// @param nseg is the number of segements to approximate
  ///
  /// @return a boolean indicating if the objects are properly contained
  bool checkContainment(const GeometryContext& gctx, size_t nseg = 1) const;

  /// Name of the volume
  std::string m_name = "Unnamed";

  /// Transform to place the bolume
  Transform3 m_transform = Transform3::Identity();

  /// Volume boundaries
  std::unique_ptr<VolumeBounds> m_bounds = nullptr;

  /// Portal store (internal/external)
  ObjectStore<std::shared_ptr<Portal>> m_portals;

  /// Surface store (internal/external)
  ObjectStore<std::shared_ptr<Surface>> m_surfaces;

  /// Volume store (internal/external)
  ObjectStore<std::shared_ptr<DetectorVolume>> m_volumes;

  /// The navigation state updator
  ManagedNavigationStateUpdator m_navigationStateUpdator;

  /// Volume material (optional)
  std::shared_ptr<IVolumeMaterial> m_volumeMaterial = nullptr;

  /// GeometryIdentifier of this volume
  GeometryIdentifier m_geometryId{0};
};

inline const Transform3& DetectorVolume::transform(
    const GeometryContext& /*gctx*/) const {
  return m_transform;
}

inline Vector3 DetectorVolume::center(const GeometryContext& gctx) const {
  return transform(gctx).translation();
}

inline const VolumeBounds& DetectorVolume::volumeBounds() const {
  return (*m_bounds.get());
}

inline std::vector<std::shared_ptr<Portal>>& DetectorVolume::portalPtrs() {
  return m_portals.internal;
}

inline std::vector<std::shared_ptr<Surface>>& DetectorVolume::surfacePtrs() {
  return m_surfaces.internal;
}

inline std::vector<std::shared_ptr<DetectorVolume>>&
DetectorVolume::volumePtrs() {
  return m_volumes.internal;
}

inline const std::vector<const Portal*>& DetectorVolume::portals() const {
  return m_portals.external;
}

inline const std::vector<const Surface*>& DetectorVolume::surfaces() const {
  return m_surfaces.external;
}

inline const std::vector<const DetectorVolume*>& DetectorVolume::volumes()
    const {
  return m_volumes.external;
}

inline const ManagedNavigationStateUpdator&
DetectorVolume::navigationStateUpdator() const {
  return m_navigationStateUpdator;
}

inline void DetectorVolume::assignVolumeMaterial(
    std::shared_ptr<IVolumeMaterial> material) {
  m_volumeMaterial = material;
}

inline std::shared_ptr<IVolumeMaterial> DetectorVolume::volumeMaterialPtr() {
  return m_volumeMaterial;
}

inline const IVolumeMaterial* DetectorVolume::volumeMaterial() const {
  return m_volumeMaterial.get();
}

inline const GeometryIdentifier& DetectorVolume::geometryId() const {
  return m_geometryId;
}

inline const std::string& DetectorVolume::name() const {
  return m_name;
}

/// @brief  A detector volume factory which first constructs the detector volume
/// and then constructs the portals. This ensures that the std::shared_ptr
/// holding the detector volume is not weak when assigning to the portals.
class DetectorVolumeFactory {
 public:
  /// Create a detector volume - from factory
  /// @param portalGenerator the volume portal generator
  /// @param gctx the geometry context for construction and potential contextual store
  template <typename... Args>
  static std::shared_ptr<DetectorVolume> construct(
      const PortalGenerator& portalGenerator, const GeometryContext& gctx,
      Args&&... args) {
    auto dVolume =
        DetectorVolume::makeShared(gctx, std::forward<Args>(args)...);
    dVolume->construct(gctx, portalGenerator);
    return dVolume;
  }
};

}  // namespace Experimental
}  // namespace Acts
