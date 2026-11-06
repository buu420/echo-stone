#include "mvci/game_language.hpp"
#include <iostream>
#include <map>

int main() {
 using namespace mvci;
 constexpr Address image=0x140000000, singleton_slot=image+0x349aea8;
 constexpr Address singleton=0x50000000, culture=0x50001000, controller=0x50002000, text=0x50003000;
 struct String {Address data;std::int32_t count,capacity;};
 std::map<Address,unsigned char> fixture;
 const auto put=[&](Address address,const auto& value) {
  const auto* bytes=reinterpret_cast<const unsigned char*>(&value);
  for(std::size_t i=0;i<sizeof(value);++i)fixture[address+i]=bytes[i];
 };
 int culture_reads=0;
 bool swap_culture=false;
 Memory memory;
 memory.reader=[&](Address address,void* buffer,std::size_t size){
  if(address==singleton+0x88&&++culture_reads==2&&swap_culture)put(singleton+0x88,culture+0x1000);
  auto* bytes=static_cast<unsigned char*>(buffer);
  for(std::size_t i=0;i<size;++i){
   const auto found=fixture.find(address+i);
   if(found==fixture.end())return false;
   bytes[i]=found->second;
  }
  return true;
 };
 int failures=0;
 const auto check=[&](bool ok,const char* why){if(!ok){++failures;std::cerr<<why<<'\n';}};
 put(singleton_slot,singleton);
 put(singleton,std::uint8_t{0});
 put(singleton+0x88,culture);
 put(singleton+0x90,controller);
 put(culture+0x30,String{text,3,8});
 put(text,u"fr");
 check(!read_game_language(memory,image),"Uninitialized internationalization state was accepted.");
 put(singleton,std::uint8_t{1});
 check(read_game_language(memory,image)==Language::French,"Current French culture was not read.");
 put(culture+0x30,String{text,8,8});
 put(text,u"zh-Hant");
 check(read_game_language(memory,image)==Language::TraditionalChinese,"Live culture change was not read.");
 put(culture+0x30,String{text,8,7});
 check(!read_game_language(memory,image),"Invalid culture capacity was accepted.");
 put(culture+0x30,String{text,32,32});
 check(!read_game_language(memory,image),"Unreadable culture was accepted.");
 put(culture+0x30,String{text,8,8});
 swap_culture=true;culture_reads=0;
 check(!read_game_language(memory,image),"A changed native culture owner was accepted.");
 return failures?1:0;
}
