#include "mvci/target.hpp"
#include "picosha2.h"
#include <filesystem>
#include <fstream>
namespace mvci {
bool ready_for_hook(const Memory& memory,Address base,std::uint32_t game_window_thread){return game_window_thread!=0&&event_code_matches(memory,base);}
std::string sha256_file(const std::wstring& path){
 std::ifstream file(std::filesystem::path(path),std::ios::binary);if(!file)return {};picosha2::hash256_one_by_one hash;std::array<unsigned char,65536> bytes{};
 while(file){file.read(reinterpret_cast<char*>(bytes.data()),bytes.size());auto n=file.gcount();if(n>0)hash.process(bytes.begin(),bytes.begin()+n);}
 if(file.bad())return {};hash.finish();return picosha2::get_hash_hex_string(hash);
}
bool event_code_matches(const Memory& memory,Address base){
 constexpr std::array<unsigned char,16> expected={0x40,0x55,0x56,0x57,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57,0x48,0x81,0xec,0xf0};std::array<unsigned char,16> actual{};
 return memory.bytes(base+process_event_rva,actual.data(),actual.size())&&actual==expected;
}
}
