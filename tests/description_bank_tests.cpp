#include "mvci/description_bank.hpp"
#include "picosha2.h"
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace mvci;
static int checks=0;
#define CHECK(x) do{++checks;if(!(x))throw std::runtime_error(std::string(__func__)+":"+std::to_string(__LINE__)+" " #x);}while(false)
struct Files {
 std::filesystem::path root;
 Files(){root=std::filesystem::canonical(std::filesystem::temp_directory_path())/std::filesystem::path(L"mvci-bank-"+std::to_wstring(std::chrono::steady_clock::now().time_since_epoch().count()));if(!std::filesystem::create_directory(root))throw std::runtime_error("Temporary fixture exists");}
 ~Files(){std::error_code ec;const auto resolved=std::filesystem::weakly_canonical(root,ec);if(!ec&&resolved==root&&resolved.parent_path()==std::filesystem::canonical(std::filesystem::temp_directory_path(),ec)&&root.filename().wstring().rfind(L"mvci-bank-",0)==0)std::filesystem::remove_all(root,ec);}
 std::vector<unsigned char> wave(){std::vector<unsigned char> b(48044);std::memcpy(b.data(),"RIFF",4);std::memcpy(b.data()+8,"WAVEfmt ",8);const auto put=[&](std::size_t at,auto v){std::memcpy(b.data()+at,&v,sizeof(v));};put(4,std::uint32_t(48036));put(16,std::uint32_t(16));put(20,std::uint16_t(1));put(22,std::uint16_t(1));put(24,std::uint32_t(24000));put(28,std::uint32_t(48000));put(32,std::uint16_t(2));put(34,std::uint16_t(16));std::memcpy(b.data()+36,"data",4);put(40,std::uint32_t(48000));return b;}
 void write(const std::string& name,const std::vector<unsigned char>& b){std::ofstream f(root/name,std::ios::binary);f.write(reinterpret_cast<const char*>(b.data()),static_cast<std::streamsize>(b.size()));}
 DescriptionMovie film(){const auto bytes=wave();write("test.wav",bytes);DescriptionCue c;c.id="test";c.file="test.wav";c.sha256=picosha2::hash256_hex_string(bytes);c.text=L"Test";c.start=1;c.end=3;c.duration=1;c.bytes=static_cast<std::uint32_t>(bytes.size());return {L"0-A_DEMO01",90,7.25,{c}};}
};
static void exact_recordings_are_loaded_and_corruption_or_noncanonical_data_rejected(){
 Files f;auto film=f.film();const auto bank=load_description_bank(f.root,film);CHECK(bank&&bank->movie==&film&&bank->clips.size()==1);CHECK(bank->clips[0].size()==48044);auto data=f.wave();data.back()=1;f.write("test.wav",data);CHECK(!load_description_bank(f.root,film));film.cues[0].sha256=picosha2::hash256_hex_string(data);data[22]=2;f.write("test.wav",data);film.cues[0].sha256=picosha2::hash256_hex_string(data);CHECK(!load_description_bank(f.root,film));
}
static void paths_budgets_duration_and_cancellation_fail_closed(){
 Files f;const auto original=f.film();auto film=original;film.cues[0].file="../test.wav";CHECK(!load_description_bank(f.root,film));film=original;film.cues[0].bytes=3000000;CHECK(!load_description_bank(f.root,film));film=original;film.cues[0].duration=2;CHECK(!load_description_bank(f.root,film));film=original;film.cues.resize(513,film.cues[0]);CHECK(!load_description_bank(f.root,film));CHECK(!load_description_bank(f.root,original,[]{return true;}));
}
int main(){int failed=0;for(auto test:{exact_recordings_are_loaded_and_corruption_or_noncanonical_data_rejected,paths_budgets_duration_and_cancellation_fail_closed})try{test();}catch(const std::exception& e){std::cerr<<e.what()<<'\n';++failed;}std::cout<<checks<<" bank checks, "<<failed<<" failed groups\n";return failed?1:0;}
