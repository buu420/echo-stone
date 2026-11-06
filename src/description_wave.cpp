#include "mvci/description_wave.hpp"
#include <cstring>
namespace mvci {
namespace {
template<class T>T symbol(HMODULE m,const char* name){return reinterpret_cast<T>(GetProcAddress(m,name));}
template<class T>T value(const std::vector<unsigned char>& b,std::size_t at){T out{};std::memcpy(&out,b.data()+at,sizeof(T));return out;}
bool valid(const std::vector<unsigned char>& b){
 return b.size()>=46&&b.size()<=2097152&&b.size()%2==0&&std::memcmp(b.data(),"RIFF",4)==0&&value<std::uint32_t>(b,4)==b.size()-8&&std::memcmp(b.data()+8,"WAVEfmt ",8)==0&&value<std::uint32_t>(b,16)==16&&value<std::uint16_t>(b,20)==1&&value<std::uint16_t>(b,22)==1&&value<std::uint32_t>(b,24)==24000&&value<std::uint32_t>(b,28)==48000&&value<std::uint16_t>(b,32)==2&&value<std::uint16_t>(b,34)==16&&std::memcmp(b.data()+36,"data",4)==0&&value<std::uint32_t>(b,40)==b.size()-44;
}
struct Buffer {WAVEHDR header{};std::vector<unsigned char> wave;};
}
struct DescriptionWave::Impl {
 DescriptionWaveApi api;bool injected=false,poisoned=false,prepared=false,queued=false,error=false;
 HMODULE module=nullptr;HWAVEOUT device=nullptr;std::unique_ptr<Buffer> buffer;
 std::uint64_t retry_at=0;
 explicit Impl(DescriptionWaveApi functions={},bool supplied=false):api(functions),injected(supplied){}
 bool playing()const{return !poisoned&&queued&&buffer&&(static_cast<const volatile WAVEHDR&>(buffer->header).dwFlags&WHDR_DONE)==0;}
 void abandon(){
  // A failed driver cancellation/unprepare may leave its pointers live. Keep
  // that memory/module/device until process exit and prohibit any further output.
  buffer.release();module=nullptr;device=nullptr;queued=prepared=false;poisoned=error=true;
 }
 bool retire(bool close){
  if(poisoned)return false;
  if(device&&buffer){
   if(playing()&&api.wave.reset(device)!=MMSYSERR_NOERROR){abandon();return false;}
   queued=false;
   if(prepared&&api.wave.unprepare(device,&buffer->header,sizeof(WAVEHDR))!=MMSYSERR_NOERROR){abandon();return false;}
   prepared=false;buffer.reset();
  }
  if(close&&device){if(api.wave.close(device)!=MMSYSERR_NOERROR){abandon();return false;}device=nullptr;}
  return true;
 }
 ~Impl(){if(retire(true)&&module)FreeLibrary(module);}
 bool open(){
  if(!injected&&!module){
   module=LoadLibraryExW(L"winmm.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);
   if(module)api={{symbol<decltype(api.wave.open)>(module,"waveOutOpen"),symbol<decltype(api.wave.prepare)>(module,"waveOutPrepareHeader"),symbol<decltype(api.wave.write)>(module,"waveOutWrite"),symbol<decltype(api.wave.reset)>(module,"waveOutReset"),symbol<decltype(api.wave.unprepare)>(module,"waveOutUnprepareHeader"),symbol<decltype(api.wave.close)>(module,"waveOutClose")},symbol<decltype(api.pause)>(module,"waveOutPause"),symbol<decltype(api.resume)>(module,"waveOutRestart")};
  }
  if(!api.wave.open||!api.wave.prepare||!api.wave.write||!api.wave.reset||!api.wave.unprepare||!api.wave.close||!api.pause||!api.resume)return false;
  WAVEFORMATEX f{};f.wFormatTag=WAVE_FORMAT_PCM;f.nChannels=1;f.nSamplesPerSec=24000;f.nAvgBytesPerSec=48000;f.nBlockAlign=2;f.wBitsPerSample=16;
  if(api.wave.open(&device,WAVE_MAPPER,&f,0,0,CALLBACK_NULL)!=MMSYSERR_NOERROR){device=nullptr;return false;}return device!=nullptr;
 }
 bool play(const std::vector<unsigned char>& data,const std::function<bool()>& current){
  if(poisoned||playing()||!valid(data)||(current&&!current()))return false;if(!retire(false))return false;
  const auto now=GetTickCount64();if(!device){if(now<retry_at)return false;if(!open()){error=true;retry_at=now+2000;return false;}}
  if(current&&!current())return false;
  buffer=std::make_unique<Buffer>();buffer->wave=data;auto& h=buffer->header;h.lpData=reinterpret_cast<char*>(buffer->wave.data()+44);h.dwBufferLength=static_cast<DWORD>(data.size()-44);
  if(api.wave.prepare(device,&h,sizeof(h))!=MMSYSERR_NOERROR){error=true;retry_at=now+2000;retire(true);return false;}
  prepared=true;
  if(current&&!current()){retire(false);return false;}
  if(api.wave.write(device,&h,sizeof(h))!=MMSYSERR_NOERROR){error=true;retry_at=now+2000;retire(true);return false;}
  queued=true;error=false;if(current&&!current()){retire(false);return false;}return true;
 }
 void pause(){if(playing()&&api.pause(device)!=MMSYSERR_NOERROR){error=true;retire(false);}}
 void resume(){if(playing()&&api.resume(device)!=MMSYSERR_NOERROR){error=true;retire(false);}}
};
DescriptionWave::DescriptionWave():impl_(std::make_unique<Impl>()){}
DescriptionWave::DescriptionWave(DescriptionWaveApi api):impl_(std::make_unique<Impl>(api,true)){}
DescriptionWave::~DescriptionWave()=default;
bool DescriptionWave::playing()const{return impl_->playing();}
bool DescriptionWave::healthy()const{return !impl_->poisoned&&!impl_->error;}
bool DescriptionWave::play(const std::vector<unsigned char>& data,const std::function<bool()>& current){return impl_->play(data,current);}
void DescriptionWave::pause(){impl_->pause();}
void DescriptionWave::resume(){impl_->resume();}
void DescriptionWave::stop(){impl_->retire(false);}
}
