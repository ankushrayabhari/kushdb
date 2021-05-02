#pragma once

#include "nn.hpp"

namespace kush::util {

// util::nn is a lot simplier in headers/BUILD files than dropbox::oxygen::nn

template <typename T>
using nn = dropbox::oxygen::nn<T>;

template <typename T>
using nn_unique_ptr = dropbox::oxygen::nn_unique_ptr<T>;

template <typename T>
using nn_shared_ptr = dropbox::oxygen::nn_shared_ptr<T>;

}  // namespace kush::util