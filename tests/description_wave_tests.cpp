#include "mvci/description_wave.hpp"
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
using namespace mvci;
static int checks=0;
#define CHECK(x) do{++checks;if(!(x))throw std::runtime_error(std::string(__func__)+":"+std::to_string(__LINE__)+" " #x);}while(false)
namespace {
struct Driver {int opened=0,written=0,resets=0,unprepared=0,closed=0,paused=0,resumed=0;bool reset_fail=false,unprepare_fail=false,write_fail=false;WAVEHDR* header=nullptr;std::function<void()> on_prepare;} driver;
MMRESULT WINAPI open(LPHWAVEOUT out,UINT,const WAVEFORMATEX* f,DWORD_PTR,DWORD_PTR,DWORD){if(f->nChannels!=1||f->nSamplesPerSec!=24000||f->wBitsPerSample!=16)return MMSYSERR_ERROR;*out=reinterpret_cast<HWAVEOUT>(0x1234);++driver.opened;return 0;}
MMRESULT WINAPI prepare(HWAVEOUT,WAVEHDR* h,UINT){h->dwFlags|=WHDR_PREPARED;if(driver.on_prepare)driver.on_prepare();return 0;}
MMRESULT WINAPI write(HWAVEOUT,WAVEHDR* h,UINT){driver.header=h;if(driver.write_fail)return MMSYSERR_ERROR;++driver.written;h->dwFlags&=~WHDR_DONE;return 0;}
MMRESULT WINAPI reset(HWAVEOUT){++driver.resets;if(driver.reset_fail)return MMSYSERR_ERROR;if(driver.header)driver.header->dwFlags|=WHDR_DONE;return 0;}
MMRESULT WINAPI unprepare(HWAVEOUT,WAVEHDR*,UINT){++driver.unprepared;return driver.unprepare_fail?WAVERR_STILLPLAYING:0;}
MMRESULT WINAPI close(HWAVEOUT){++driver.closed;return 0;}
MMRESULT WINAPI pause(HWAVEOUT){++driver.paused;return 0;}
MMRESULT WINAPI resume(HWAVEOUT){++driver.resumed;return 0;}
DescriptionWaveApi api(){return {{open,prepare,write,reset,unprepare,close},pause,resume};}
std::vector<unsigned char> wave(){std::vector<unsigned char> b(48044);const char tags[]="RIFF\0\0\0\0WAVEfmt ";std::memcpy(b.data(),tags,16);const auto put=[&](std::size_t p,auto v){std::memcpy(b.data()+p,&v,sizeof(v));};put(4,std::uint32_t(48036));put(16,std::uint32_t(16));put(20,std::uint16_t(1));put(22,std::uint16_t(1));put(24,std::uint32_t(24000));put(28,std::uint32_t(48000));put(32,std::uint16_t(2));put(34,std::uint16_t(16));std::memcpy(b.data()+36,"data",4);put(40,std::uint32_t(48000));return b;}
}
static void owned_buffer_supports_pause_resume_and_bounded_stop(){
 driver={};{DescriptionWave out(api());const auto data=wave();CHECK(out.play(data));CHECK(out.playing());CHECK(driver.header&&driver.header->lpData);CHECK(!out.play(data));out.pause();out.resume();CHECK(driver.paused==1&&driver.resumed==1);out.stop();CHECK(!out.playing());CHECK(driver.resets==1&&driver.unprepared==1);CHECK(out.play(data));driver.header->dwFlags|=WHDR_DONE;CHECK(!out.playing());}
 CHECK(driver.closed==1);
}
static void malformed_wavs_never_open_a_device(){
 driver={};DescriptionWave out(api());auto data=wave();data[22]=2;CHECK(!out.play(data));data=wave();data.pop_back();CHECK(!out.play(data));CHECK(driver.opened==0);
}
static void failed_cancellation_retains_driver_memory_and_disables_new_output(){
 driver={};DescriptionWave out(api());CHECK(out.play(wave()));auto h=driver.header;driver.reset_fail=true;out.stop();CHECK(!out.playing());CHECK(h->lpData!=nullptr);CHECK(!out.play(wave()));CHECK(driver.unprepared==0&&driver.closed==0);
}
static void failed_unprepare_and_write_are_safe(){
 driver={};{DescriptionWave out(api());CHECK(out.play(wave()));auto h=driver.header;driver.unprepare_fail=true;out.stop();CHECK(h->lpData!=nullptr);CHECK(!out.play(wave()));}
 driver={};driver.write_fail=true;{DescriptionWave out(api());CHECK(!out.play(wave()));CHECK(!out.playing());}CHECK(driver.unprepared==1&&driver.closed==1);
}
static void driver_preparation_cannot_queue_a_retired_native_observation(){
 driver={};bool current=true;driver.on_prepare=[&]{current=false;};DescriptionWave out(api());CHECK(!out.play(wave(),[&]{return current;}));CHECK(driver.written==0);CHECK(driver.unprepared==1);CHECK(out.healthy());driver.on_prepare={};current=true;CHECK(out.play(wave(),[&]{return current;}));
}
int main(){int failed=0;for(auto test:{owned_buffer_supports_pause_resume_and_bounded_stop,malformed_wavs_never_open_a_device,failed_cancellation_retains_driver_memory_and_disables_new_output,failed_unprepare_and_write_are_safe,driver_preparation_cannot_queue_a_retired_native_observation})try{test();}catch(const std::exception& e){std::cerr<<e.what()<<'\n';++failed;}std::cout<<checks<<" wave checks, "<<failed<<" failed groups\n";return failed?1:0;}
