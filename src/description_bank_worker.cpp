#include "mvci/description_bank.hpp"
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
namespace mvci {
struct DescriptionBankLoader::Impl {
 std::filesystem::path root;Load load;
 mutable std::mutex mutex;std::condition_variable changed;
 std::atomic<bool> quit{false};std::atomic<std::uint64_t> generation{0};
 const DescriptionMovie* desired=nullptr;DescriptionBankView result;
 std::thread worker;
 Impl(std::filesystem::path path,Load function):root(std::move(path)),load(function?std::move(function):load_description_bank){worker=std::thread([this]{run();});}
 ~Impl(){quit=true;++generation;changed.notify_all();if(worker.joinable())worker.join();}
 void run(){
#ifdef _WIN32
  SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_BELOW_NORMAL);
#endif
  std::uint64_t handled=0;
  for(;;){
   const DescriptionMovie* movie=nullptr;std::uint64_t task=0;
   {std::unique_lock<std::mutex> lock(mutex);changed.wait(lock,[&]{return quit.load()||generation.load()!=handled;});if(quit)return;movie=desired;task=generation.load();}
   std::shared_ptr<const DescriptionBank> bank;
   try{if(movie)bank=load(root,*movie,[this,task]{return quit.load()||generation.load()!=task;});}catch(...){bank.reset();}
   {std::lock_guard<std::mutex> lock(mutex);handled=task;if(quit)return;if(generation.load()!=task)continue;
    if(!movie)result={};else if(bank&&bank->movie==movie)result={movie,DescriptionBankStage::Ready,std::move(bank)};else result={movie,DescriptionBankStage::Failed,{}};
   }
  }
 }
 void request(const DescriptionMovie* movie){
  {std::lock_guard<std::mutex> lock(mutex);if(desired==movie)return;desired=movie;++generation;result={movie,movie?DescriptionBankStage::Loading:DescriptionBankStage::Idle,{}};}
  changed.notify_one();
 }
 DescriptionBankView view()const{std::lock_guard<std::mutex> lock(mutex);return result;}
};
DescriptionBankLoader::DescriptionBankLoader(std::filesystem::path root,Load load):impl_(std::make_unique<Impl>(std::move(root),std::move(load))){}
DescriptionBankLoader::~DescriptionBankLoader()=default;
void DescriptionBankLoader::request(const DescriptionMovie* movie){impl_->request(movie);}
DescriptionBankView DescriptionBankLoader::view()const{return impl_->view();}
}
