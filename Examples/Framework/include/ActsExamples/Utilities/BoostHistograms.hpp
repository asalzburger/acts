// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <mutex>

// Include boost histogram forward declaration for TProfile
#include <boost/histogram.hpp>

namespace acts_boost {
// A thread-safe wrapper around the standard weighted_mean accumulator state
template <typename ValueType>
class atomic_weighted_mean {
 private:
  // The mutex to protect the data_ member
  boost::histogram::accumulators::weighted_mean<ValueType> data;
  mutable std::shared_ptr<std::mutex> mtx = std::make_shared<std::mutex>();

 public:
  using value_type = ValueType;
  using pointer = ValueType*;
  using size_type = std::size_t;

  static constexpr bool has_threading_support = true;

  // --- Required for C++ Standard Allocator Concept (Dummy implementations) ---
  pointer allocate(size_type n) { return std::allocator<ValueType>{}.allocate(n); }
  void deallocate(pointer p, size_type n) { std::allocator<ValueType>{}.deallocate(p, n); }

  // Default constructor
  atomic_weighted_mean() = default;

  // Add an allocator-aware constructor required by the standard library
  // It takes any allocator type as an argument (we ignore it in this case)
  template <typename Alloc>
  atomic_weighted_mean(const Alloc&) noexcept {}

  /// The essential operation: fill the accumulator with a weight and a sample
  /// value. This is where we acquire the lock.
  /// @param weight The weight of the sample
  /// @param sample_value The sample value
  void operator()(ValueType weight, ValueType sample_value) {
    std::lock_guard<std::mutex> lock(*mtx);
    data(weight, sample_value);
  }

  /// Merge operation: add the contents of another atomic_weighted_mean to this
  /// one. This requires locking both the source and destination in a specific
  /// order to avoid deadlock.
  /// @param other The other atomic_weighted_mean to merge from
  void operator+=(const atomic_weighted_mean& other) {
    // Use std::scoped_lock for C++17 or std::lock for C++11/14 to lock multiple
    // mutexes safely
    std::scoped_lock lock(*mtx, *other.mtx);
    data += other.data;
  }

  /// @brief    Get the value of the accumulator
  /// @return   The value of the accumulator
  ValueType value() const {
    std::lock_guard<std::mutex> lock(*mtx);
    return data.value();
  }

  /// @brief    Get the variance of the accumulator
  /// @return   The variance of the accumulator
  ValueType variance() const {
    std::lock_guard<std::mutex> lock(*mtx);
    return data.variance();
  }

  /// @brief Get the sum of weights in the accumulator
  /// @return The sum of weights
  ValueType sum_of_weights() const {
    std::lock_guard<std::mutex> lock(*mtx);
    return data.sum_of_weights();
  }

  /// @brief Get the count of samples in the accumulator
  /// @return The count of samples
  ValueType count() const {
    std::lock_guard<std::mutex> lock(*mtx);
    return data.count();
  }
};

}  // namespace acts_boost
using BoostAxisVariant = boost::histogram::axis::variant<boost::histogram::axis::regular<double>>;

using BoostRegularAxis = boost::histogram::axis::regular<double, boost::histogram::use_default>;

using BoostProfile =
    boost::histogram::histogram<std::vector<BoostAxisVariant>,
                                boost::histogram::unlimited_storage<
                                    acts_boost::atomic_weighted_mean<double>>>;
