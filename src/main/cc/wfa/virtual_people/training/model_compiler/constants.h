// Copyright 2021 The Cross-Media Measurement Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef SRC_MAIN_CC_WFA_VIRTUAL_PEOPLE_TRAINING_MODEL_COMPILER_CONSTANTS_H_
#define SRC_MAIN_CC_WFA_VIRTUAL_PEOPLE_TRAINING_MODEL_COMPILER_CONSTANTS_H_

namespace wfa_virtual_people {

// In the compiled model, the values of population_offset and total_population
// in the population pools must be multiples of kDiscretization.
constexpr uint64_t kDiscretization = 1000;

// The offset and size of the cookie monster pools.
// ID range starting from kCookieMonsterOffset, including those >=
// kCookieMonsterSize, is reserved.
constexpr uint64_t kCookieMonsterOffset = 1000000000000000000;  // 10^18
constexpr uint64_t kCookieMonsterSize = 100000000000000;        // 10^14

}  // namespace wfa_virtual_people

#endif  // SRC_MAIN_CC_WFA_VIRTUAL_PEOPLE_TRAINING_MODEL_COMPILER_CONSTANTS_H_
