#pragma once

#include <string_view>

void CheckImage(std::string_view filename, std::string_view comment = "");
void CheckInvalidImage(std::string_view filename);
void CheckMissingImage(std::string_view filename);
