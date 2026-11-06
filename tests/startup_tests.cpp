#include "mvci/target.hpp"
#include <iostream>
int main(){
 mvci::Memory memory;std::array<unsigned char,16> bytes={0x40,0x55,0x56,0x57,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57,0x48,0x81,0xec,0xf0};
 memory.reader=[&](mvci::Address at,void* out,std::size_t n){if(at!=0x140000000+mvci::process_event_rva||n!=bytes.size())return false;std::memcpy(out,bytes.data(),n);return true;};
 if(mvci::ready_for_hook(memory,0x140000000,0)){std::cerr<<"Decoded bytes before the real game window must not permit a hook\n";return 1;}
 if(!mvci::ready_for_hook(memory,0x140000000,123)){std::cerr<<"Ready game window and exact decoded code rejected\n";return 2;}
 bytes[3]^=1;if(mvci::ready_for_hook(memory,0x140000000,123)){std::cerr<<"Different decoded code accepted\n";return 3;}
 std::cout<<"startup requires the real game window and exact decoded code\n";return 0;
}
