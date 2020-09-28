#pragma once

#include <optional>
#include <string>
#include <string_view>

std::optional<std::string> GetNonTranslocatedPath(std::string_view path);
