#include "mvci/beacon.hpp"
#include "mvci/beacon_wave.hpp"
namespace mvci {
std::vector<std::int16_t> MakeBeaconPulse(){auto wave=beacon_wave();return {wave.begin(),wave.end()};}
constexpr unsigned kBeaconRate=beacon_sample_rate;
class BeaconSchedule {
 bool armed_=false;std::uint64_t last_=0;
public:
 bool Tick(bool enabled,std::uint32_t interval,std::uint64_t now){
  if(!enabled||interval<80){armed_=false;return false;}
  if(!armed_){armed_=true;last_=now;return true;}
  if(now<last_){last_=now;return false;}
  if(now-last_<interval)return false;
  last_=now;return true;
 }
};
namespace {
template<class T>T Symbol(HMODULE module,const char* name){return reinterpret_cast<T>(GetProcAddress(module,name));}
bool CompleteApi(const WaveOutputApi& a){return a.open&&a.prepare&&a.write&&a.reset&&a.unprepare&&a.close;}
struct PulseBuffer {WAVEHDR header{};std::vector<std::int16_t> samples=MakeBeaconPulse();};
}
struct BeaconAudio::Impl {
 WaveOutputApi api{};bool injected=false,prepared=false,queued=false,poisoned=false;
 HMODULE module=nullptr;HWAVEOUT device=nullptr;
 std::unique_ptr<PulseBuffer> buffer=std::make_unique<PulseBuffer>();
 BeaconSchedule schedule;std::uint64_t retry_at=0;
 explicit Impl(WaveOutputApi functions={},bool supplied=false):api(functions),injected(supplied){}
 void Abandon(){
  // A broken driver that refuses cancellation may still own this memory and
  // code. Retain them until process exit and disable further device creation.
  buffer.release();module=nullptr;device=nullptr;queued=prepared=false;poisoned=true;
 }
 bool Retire(bool close){
  if(!device)return !poisoned;
  if(queued&&api.reset(device)!=MMSYSERR_NOERROR){Abandon();return false;}
  queued=false;
  if(!close)return true;
  if(prepared&&api.unprepare(device,&buffer->header,sizeof(WAVEHDR))!=MMSYSERR_NOERROR){Abandon();return false;}
  prepared=false;
  if(api.close(device)!=MMSYSERR_NOERROR){Abandon();return false;}
  device=nullptr;return true;
 }
 ~Impl(){if(Retire(true)&&module)FreeLibrary(module);}
 bool Open(){
  if(!injected&&!module){
   module=LoadLibraryExW(L"winmm.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);
   if(module)api={Symbol<decltype(api.open)>(module,"waveOutOpen"),Symbol<decltype(api.prepare)>(module,"waveOutPrepareHeader"),Symbol<decltype(api.write)>(module,"waveOutWrite"),Symbol<decltype(api.reset)>(module,"waveOutReset"),Symbol<decltype(api.unprepare)>(module,"waveOutUnprepareHeader"),Symbol<decltype(api.close)>(module,"waveOutClose")};
  }
  if(!CompleteApi(api))return false;
  WAVEFORMATEX format{};format.wFormatTag=WAVE_FORMAT_PCM;format.nChannels=2;format.nSamplesPerSec=kBeaconRate;format.wBitsPerSample=16;format.nBlockAlign=4;format.nAvgBytesPerSec=kBeaconRate*4;
  if(api.open(&device,WAVE_MAPPER,&format,0,0,CALLBACK_NULL)!=MMSYSERR_NOERROR){device=nullptr;return false;}
  auto& header=buffer->header;header={};header.lpData=reinterpret_cast<char*>(buffer->samples.data());header.dwBufferLength=static_cast<DWORD>(buffer->samples.size()*sizeof(std::int16_t));
  if(api.prepare(device,&header,sizeof(header))!=MMSYSERR_NOERROR){Retire(true);return false;}
  prepared=true;return true;
 }
 BeaconAudioResult Tick(bool enabled,std::uint32_t interval,std::uint64_t now){
  if(!enabled||interval<80){schedule.Tick(false,0,now);return Retire(false)?BeaconAudioResult::Idle:BeaconAudioResult::Unavailable;}
  if(poisoned)return BeaconAudioResult::Unavailable;
  if(!device){
   if(now<retry_at)return BeaconAudioResult::Unavailable;
   if(!Open()){retry_at=now+2000;return BeaconAudioResult::Unavailable;}
  }
  if(queued){
   const auto flags=static_cast<const volatile WAVEHDR&>(buffer->header).dwFlags;
   if(!(flags&WHDR_DONE))return BeaconAudioResult::Idle;
   queued=false;
  }
  if(!schedule.Tick(true,interval,now))return BeaconAudioResult::Idle;
  if(api.write(device,&buffer->header,sizeof(WAVEHDR))!=MMSYSERR_NOERROR){retry_at=now+2000;schedule.Tick(false,0,now);Retire(true);return BeaconAudioResult::Unavailable;}
  queued=true;return BeaconAudioResult::Played;
 }
};
BeaconAudio::BeaconAudio():impl_(std::make_unique<Impl>()){}
BeaconAudio::BeaconAudio(WaveOutputApi api):impl_(std::make_unique<Impl>(api,true)){}
BeaconAudio::~BeaconAudio()=default;
BeaconAudioResult BeaconAudio::Tick(bool enabled,std::uint32_t interval,std::uint64_t now){return impl_->Tick(enabled,interval,now);}
}
