# Misaligning and aligning the detector

## Introduction

ACTS offers a lightweight way to attach any sort of contextual geometry data to the {class}`Acts::Surface` objects and thus to the {class}`Acts::TrackingGeometry`. This is done using the {class}`Acts::GeometryContext` that can hold any soft of information and is transparenlty handed through the call chain, to guarantee that geometrical operation appear within a certain context, i.e. with a certain detector alignment. The {class}`Acts::GeometryContext` object is unpacked by an appropriate extension of the {class}`Acts::DetectorElement`, and the conditional alignment of the object resolved in such a way.


## Showcases in the Example framework: Misaligning

### `AlignedDetectorElement`, `AlignmentContext`, and `AlignmentStore`

The `Examples` offer a showcase how such an implementation can be done, however, it shall be remarked here that this is only one possible implementation.
`Examples/Detector/Common` host a little template class that allows to extend any given {class}`DetectorElement` (that in turn extends the {class}`Acts::DetectorElementBase` API) to become an `AlignedDetectorElement`. The corresponding aligned detector elements for the different geometry backends can be found in the associated sub directories `Detectors/DD4hepDetector`, `Detectors/TGeoDetector`, `Detectors/GenericDetector` amd `Detectors/GeoModelDetector`, respectively.


The listing belows illustrates the overlaod of the `transform(const GeometryContext& gctx)` which upacks the `Acts::GeometryContext`

```c++
namespace ActsExamples {

/// A detector element that is aligned with the ACTS framework
/// This class is a wrapper around the DetectorElement class
/// for different detectors and can be used with the same alignment
/// showcase infrastructure.
template <typename DetectorElement>
class Aligned : public DetectorElement {
 public:
  /// Using the constructor from the base class
  using DetectorElement::DetectorElement;
  /// An alignment aware transform call
  /// @param gctx the geometry context which is - if possible - unpacked to an AlignementContext
  /// @return The alignment-corrected transform if available, otherwise the nominal transform.
  const Acts::Transform3& transform(
      const Acts::GeometryContext& gctx) const final {
    if (gctx.hasValue()) {
      const auto* alignmentContext = gctx.maybeGet<AlignmentContext>();
      if (alignmentContext != nullptr && alignmentContext->store != nullptr) {
        const auto* transform =
            alignmentContext->store->contextualTransform(this->surface());
        // The store may only have a subset of surfaces registered
        if (transform != nullptr) {
          return *transform;
        }
      }
    }
    // If no alignment context is available, return the nominal transform
    return DetectorElement::nominalTransform();
  }
};

} // end of namespace
```

In addition, a commonly shared `ActsExamples::AlignmentContext` with the before used alignment store (that provided the contextual transform for the surface) can be found there.

### Decorating the detector with (different) misalignment(s)

The Examples framework, and particularly the `Sequencer` provides a possibility to the attach a certain detector alignment for a given event. This is done using a dedicated `ContextDecorator`, which is called by the `Sequencer` to decorate the `AlgorithmContext` with the corresponding contextual data (alignment, conditions, etc.). For showcasing the misalignment procedure, a common `AlignmentDecorator` can be used that either generates some random misalignments (using another helper class, called `AlignmentGenerator`) or reads the alignment parameters from disk in a dedicated file format.

In case of the `ActsExamples::AlignmentDecorator`, both either a IOV (interval of validity) identified alignment store container, or certain alignment generators can be provided.

```c++
  /// Configuration struct
  struct Config {
    /// I/O mode - this if for showcase examples where the alignment data is
    /// read
    std::vector<std::tuple<IOV, std::shared_ptr<IAlignmentStore>>>
        iovStores;  //!< Stores for alignment data, each with a validity
                    //!< interval

    /// Generation mode - this is for showcase examples where the alignment data
    /// is generated`
    std::shared_ptr<IAlignmentStore> nominalStore =
        nullptr;  //!< The nominal alignment store

    /// Run garbage collection on the alignment data
    bool garbageCollection =
        false;  //!< Whether to run garbage collection on the alignment data

    /// Number of events to wait before garbage collection
    std::size_t gcInterval =
        100u;  //!< The number of events to wait before garbage collection

    std::vector<std::tuple<IOV, std::function<void(Acts::Transform3*)>>>
        iovGenerators;  //!< Generators for alignment data, each with a validity
                        //!< interval
  };
  ```

:::{todo}
Write a recipe how to align the detector
:::
