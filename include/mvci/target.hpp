#pragma once
#include "mvci/unreal.hpp"
namespace mvci {
inline constexpr char supported_game_sha256[]="ccf02d07677f259165cc8830eba0fcfaeea32f70c756871735e7188c367be2e6";
inline constexpr Address process_event_rva=0xabd2b0;
std::string sha256_file(const std::wstring& path);
bool event_code_matches(const Memory&,Address base);
bool ready_for_hook(const Memory&,Address base,std::uint32_t game_window_thread);
}
