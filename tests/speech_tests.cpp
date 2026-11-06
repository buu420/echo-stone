#include "mvci/speech_gate.hpp"
#include <iostream>
#include <stdexcept>
using namespace mvci;
int main(){try{
 SpeechGate gate;if(!gate.allowed(Priority::Side,1000,{}))throw std::runtime_error("Initial speech blocked");
 gate.accepted(Priority::Manual,70,1000);
 if(gate.allowed(Priority::Side,1080,{})||gate.allowed(Priority::Gauge,1080,{}))throw std::runtime_error("Automatic line cuts off accepted F5/startup confirmation");
 if(!gate.allowed(Priority::Manual,1080,{})||!gate.allowed(Priority::Menu,1080,{}))throw std::runtime_error("Fresh user navigation was delayed");
 if(gate.allowed(Priority::Side,10000,true))throw std::runtime_error("Backend still speaking");
 if(gate.allowed(Priority::Side,1050,false))throw std::runtime_error("Backend startup race");
 if(!gate.allowed(Priority::Side,1200,false))throw std::runtime_error("Reported completion ignored");
 if(!gate.allowed(Priority::Side,10000,{}))throw std::runtime_error("Unsupported backend never released hold");
 gate.accepted(Priority::Gauge,25,11000);if(!gate.allowed(Priority::Side,11050,{}))throw std::runtime_error("Higher priority side suppressed");
 if(gate.allowed(Priority::Gauge,11050,{}))throw std::runtime_error("Gauge line interrupts gauge line");
 gate.reset();if(!gate.allowed(Priority::Gauge,11060,{}))throw std::runtime_error("Focus loss did not reset hold");
 gate.accepted(Priority::Menu,250,12000,L"modal1|warning");if(!gate.retired_menu(L"",true))throw std::runtime_error("Accepted vanished modal was not cancelled");
 if(gate.retired_menu(L"",true))throw std::runtime_error("Retired modal cancelled twice");
 gate.accepted(Priority::Manual,250,13000,L"menu2|old selection");if(!gate.retired_menu(L"menu2|new selection",true))throw std::runtime_error("Obsolete F8 reading was not cancelled");
 gate.accepted(Priority::Menu,250,14000,L"modal1|warning");if(!gate.retired_menu(L"modal1|warning",false))throw std::runtime_error("Stale accepted menu kept speaking");
 gate.accepted(Priority::Menu,250,15000,L"modal1|warning");if(gate.retired_menu(L"modal1|warning",true))throw std::runtime_error("Current warning was cancelled");
 std::cout<<"16 speech priority/lifetime checks passed\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
