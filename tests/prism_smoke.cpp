#include "mvci/prism.hpp"
#include <iostream>
#include <thread>
int wmain(int argc,wchar_t** argv){if(argc!=2)return 2;mvci::Prism prism;if(!prism.open(argv[1])){std::cerr<<"Prism backend unavailable\n";return 1;}bool wrong_thread=true;std::thread t([&]{wrong_thread=prism.speak(L"Wrong-thread test must stay silent.");});t.join();if(wrong_thread){std::cerr<<"speech thread ownership failed\n";return 3;}std::cout<<"backend "<<prism.backend_name()<<'\n';bool accepted=prism.speak(L"Prism speech test for MVC Infinite.");std::cout<<"speech accepted "<<accepted<<'\n';return accepted?0:4;}
