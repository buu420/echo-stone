#include "mvci/controls.hpp"
#include <iostream>
using namespace mvci;
static int original_checks(){Controls c;c.observe({true,false,false,false},true);if(c.next()!=Request::SelectPlayer)return 1;c.observe({true,false,false,false},true);if(c.next()!=Request::None)return 2;c.observe({false,false,false,false},true);c.observe({false,true,true,true},true);for(auto r:{Request::ToggleBeacon,Request::ReadCombat,Request::ReadMenu})if(c.next()!=r)return 3;if(c.next()!=Request::None)return 4;c.observe({false,false,false,false},false);c.observe({false,false,true,false},false);c.observe({false,false,true,false},true);if(c.next()!=Request::None)return 5;c.observe({false,false,false,false},true);c.observe({false,false,true,false},true);if(c.next()!=Request::ReadCombat)return 6;c.observe({true,false,false,false},true);c.observe({true,false,false,false},false);if(c.next()!=Request::None)return 7;std::cout<<"foreground edges, ordering, held keys and focus-loss cancellation verified\n";return 0;}

static int side_toggle_edges(){
 Controls c;c.observe({false,false,false,false,true},true);if(c.next()!=Request::ToggleSide)return 10;
 c.observe({false,false,false,false,true},true);if(c.next()!=Request::None)return 11;
 c.observe({false,false,false,false,false},true);c.observe({true,false,false,false,true},true);
 if(c.next()!=Request::SelectPlayer||c.next()!=Request::ToggleSide||c.next()!=Request::None)return 12;
 c.observe({false,false,false,false,false},false);c.observe({false,false,false,false,true},false);
 c.observe({false,false,false,false,true},true);if(c.next()!=Request::None)return 13;
 c.observe({false,false,false,false,false},true);c.observe({false,false,false,false,true},true);if(c.next()!=Request::ToggleSide)return 14;
 c.observe({false,false,false,false,false},true);c.observe({false,false,false,false,true},true);
 c.observe({false,false,false,false,true},false);if(c.next()!=Request::None)return 15;
 return 0;
}
int main(){const int old=original_checks();if(old)return old;const int side=side_toggle_edges();if(side)return side;Controls descriptions;descriptions.observe({false,false,false,false,false,true},true);if(descriptions.next()!=Request::ToggleDescriptions)return 16;descriptions.observe({false,false,false,false,false,true},true);if(descriptions.next()!=Request::None)return 17;descriptions.observe({false,false,false,false,false,true},false);if(descriptions.next()!=Request::None)return 18;std::cout<<"F9/F10 toggle edges, ordering and focus cancellation verified\n";return 0;}
