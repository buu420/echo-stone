#pragma once
#include "mvci/unreal.hpp"

namespace mvci {
std::optional<Language> read_game_language(const Memory& memory, Address image_base);
}
