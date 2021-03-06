/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef UNIFORM_CONSTANTS_H
#define UNIFORM_CONSTANTS_H

#include <map>
#include <set>
#include <utility>

#include "Enum.h"

namespace Uniform {

using index = uint8_t;
constexpr index BAD_OFFSET = UINT8_MAX;
const auto UNIFORM_MAX_COUNT = UINT8_MAX;

// Uniform buffer offsets to pass types
using offsets = std::set<uint32_t>;
using offsetsMapValue = std::map<offsets, std::set<PASS>>;

// Descriptor/pipeline to offsets/pass types
using offsetsMapKey = std::pair<DESCRIPTOR, PIPELINE>;
using offsetsKeyValue = std::pair<const offsetsMapKey, offsetsMapValue>;
using offsetsMap = std::map<offsetsMapKey, offsetsMapValue>;

// Constants for ease of use
extern const offsets DEFAULT_SINGLE_SET;
extern const offsets DEFAULT_ALL_SET;
extern const std::set<PASS> PASS_ALL_SET;

}  // namespace Uniform

#endif  //! UNIFORM_CONSTANTS_H