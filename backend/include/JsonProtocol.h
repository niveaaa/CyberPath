#pragma once

#include "json.hpp"

using namespace std;

namespace cyberpath {

using Json = nlohmann::json;

Json handleJsonRequest(const Json& request);

}  // namespace cyberpath
