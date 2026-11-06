#include "mvci/description_bank.hpp"
#include <atomic>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>
using namespace mvci;
static int checks=0;
#define CHECK(x) do{++checks;if(!(x))throw std::runtime_error(std::string(__func__)+":"+std::to_string(__LINE__)+" " #x);}while(false)
template<class F>bool until(F predicate){for(int n=0;n<1000;++n){if(predicate())return true;std::this_thread::sleep_for(std::chrono::milliseconds(1));}return false;}
struct Release {std::atomic<bool>& value;~Release(){value=true;}};
static void io_runs_off_the_caller_and_replaced_requests_cannot_publish_old_banks(){
 std::atomic<bool> started=false,release=false,other_thread=false,cancelled=false;const auto caller=std::this_thread::get_id();DescriptionMovie first,second;first.id=L"first";second.id=L"second";
 DescriptionBankLoader loader(L"unused",[&](const auto&,const DescriptionMovie& movie,const auto& cancel){
  if(&movie==&first){other_thread=std::this_thread::get_id()!=caller;started=true;while(!release&&!cancel())std::this_thread::sleep_for(std::chrono::milliseconds(1));cancelled=cancel();}
  auto bank=std::make_shared<DescriptionBank>();bank->movie=&movie;return bank;
 });Release guard{release};
 loader.request(&first);CHECK(until([&]{return started.load();}));CHECK(other_thread);CHECK(loader.view().stage==DescriptionBankStage::Loading);
 loader.request(&second);CHECK(until([&]{return loader.view().stage==DescriptionBankStage::Ready;}));const auto state=loader.view();CHECK(cancelled);CHECK(state.movie==&second&&state.bank&&state.bank->movie==&second);
 loader.request(nullptr);CHECK(loader.view().stage==DescriptionBankStage::Idle&&!loader.view().bank);
}
static void loader_exceptions_and_missing_banks_are_copied_failures(){
 DescriptionMovie film;DescriptionBankLoader loader(L"unused",[](const auto&,const auto&,const auto&)->std::shared_ptr<const DescriptionBank>{throw std::runtime_error("file failure");});loader.request(&film);CHECK(until([&]{return loader.view().stage==DescriptionBankStage::Failed;}));CHECK(!loader.view().bank);
}
int main(){int failed=0;for(auto test:{io_runs_off_the_caller_and_replaced_requests_cannot_publish_old_banks,loader_exceptions_and_missing_banks_are_copied_failures})try{test();}catch(const std::exception& e){std::cerr<<e.what()<<'\n';++failed;}std::cout<<checks<<" worker checks, "<<failed<<" failed groups\n";return failed?1:0;}
