#include "mvci/game_language.hpp"
#include <array>

namespace mvci {
namespace {
// Steam 18789511. The native GetCurrentCultureName thunk at 0x526b60
// calls 0x3f3660. Its FInternationalization singleton and FCulture name
// field are read here without invoking the allocator or any game function.
constexpr Address internationalization_rva=0x349aea8;
constexpr Address active_culture_offset=0x88;
constexpr Address culture_name_offset=0x30;

struct CultureRef {Address value,controller;};
struct CultureName {Address data;std::int32_t count,capacity;};
}

std::optional<Language> read_game_language(const Memory& memory,Address image_base) {
 const auto slot=image_base+internationalization_rva;
 const auto state=memory.get<Address>(slot);
 if(!state||!*state||memory.get<std::uint8_t>(*state)!=1)return {};

 const auto active=memory.get<CultureRef>(*state+active_culture_offset);
 if(!active||!active->value)return {};
 const auto name=memory.get<CultureName>(active->value+culture_name_offset);
 std::array<char16_t,32> units{};
 if(!name||!name->data||name->count<3||name->capacity<name->count||
    static_cast<std::size_t>(name->count)>units.size())return {};
 const auto count=static_cast<std::size_t>(name->count);
 if(!memory.bytes(name->data,units.data(),count*sizeof(char16_t))||units[count-1]!=0)return {};

 // Require the same initialized culture and FString header after copying.
 const auto current=memory.get<CultureRef>(*state+active_culture_offset);
 const auto header=memory.get<CultureName>(active->value+culture_name_offset);
 if(memory.get<Address>(slot)!=state||memory.get<std::uint8_t>(*state)!=1||
    !current||current->value!=active->value||current->controller!=active->controller||
    !header||header->data!=name->data||header->count!=name->count||header->capacity!=name->capacity)return {};

 std::array<wchar_t,32> tag{};
 for(std::size_t i=0;i<count-1;++i){
  if(units[i]<0x20||units[i]>0x7e)return {};
  tag[i]=static_cast<wchar_t>(units[i]);
 }
 return language_from_tag(std::wstring_view(tag.data(),count-1));
}
}
