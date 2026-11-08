#pragma once

#include "json.hpp"

namespace cyberpath {

using Json = nlohmann::json;

Json handleJsonRequest(const Json& request);

}  // namespace cyberpath
