#include "mvci/beacon.hpp"
#include <iostream>
using namespace mvci;
int checks=0,failures=0;
void Check(bool value,const char* why){++checks;if(!value){++failures;std::cerr<<"FAIL "<<why<<'\n';}}
struct Driver {int opens=0,prepares=0,writes=0,resets=0,unprepares=0,closes=0;bool open_fail=false,write_fail=false,reset_fail=false;WAVEHDR* buffer=nullptr;} driver;
MMRESULT WINAPI Open(LPHWAVEOUT out,UINT id,LPCWAVEFORMATEX f,DWORD_PTR callback,DWORD_PTR instance,DWORD flags){
 ++driver.opens;Check(id==WAVE_MAPPER&&f->wFormatTag==WAVE_FORMAT_PCM&&f->nChannels==2&&f->nSamplesPerSec==44100&&f->wBitsPerSample==16&&f->nBlockAlign==4,"audio opens a shared-compatible stereo PCM format");
 Check(!callback&&!instance&&flags==CALLBACK_NULL,"audio does not install a game-thread callback");
 if(driver.open_fail)return MMSYSERR_NODRIVER;*out=reinterpret_cast<HWAVEOUT>(1);return MMSYSERR_NOERROR;
}
MMRESULT WINAPI Prepare(HWAVEOUT,WAVEHDR* h,UINT size){++driver.prepares;Check(size==sizeof(WAVEHDR)&&h->dwBufferLength==14112&&h->lpData,"the real 80-ms pulse buffer is prepared");h->dwFlags|=WHDR_PREPARED;driver.buffer=h;return MMSYSERR_NOERROR;}
MMRESULT WINAPI Write(HWAVEOUT,WAVEHDR* h,UINT){++driver.writes;if(driver.write_fail)return MMSYSERR_ERROR;Check((h->dwFlags&WHDR_PREPARED)!=0,"only a prepared pulse is submitted");h->dwFlags=(h->dwFlags&~WHDR_DONE)|WHDR_INQUEUE;return MMSYSERR_NOERROR;}
MMRESULT WINAPI Reset(HWAVEOUT){++driver.resets;if(driver.reset_fail)return MMSYSERR_ERROR;if(driver.buffer)driver.buffer->dwFlags=(driver.buffer->dwFlags&~WHDR_INQUEUE)|WHDR_DONE;return MMSYSERR_NOERROR;}
MMRESULT WINAPI Unprepare(HWAVEOUT,WAVEHDR* h,UINT){++driver.unprepares;h->dwFlags&=~WHDR_PREPARED;return MMSYSERR_NOERROR;}
MMRESULT WINAPI Close(HWAVEOUT){++driver.closes;return MMSYSERR_NOERROR;}
WaveOutputApi Api(){return {Open,Prepare,Write,Reset,Unprepare,Close};}
void Complete(){if(driver.buffer)driver.buffer->dwFlags=(driver.buffer->dwFlags&~WHDR_INQUEUE)|WHDR_DONE;}
int main(){
 {driver={};BeaconAudio audio(Api());Check(audio.Tick(false,0,1000)==BeaconAudioResult::Idle&&driver.opens==0,"inactive combat never opens an audio device");
  Check(audio.Tick(true,825,1000)==BeaconAudioResult::Played&&driver.writes==1,"enabling writes one real pulse");
  Complete();audio.Tick(true,825,1824);Check(driver.writes==1,"audio waits for the distance interval");
  audio.Tick(true,825,1825);Check(driver.writes==2&&driver.opens==1&&driver.prepares==1,"completed pulse reuses the owned stream without reopening");
  audio.Tick(true,150,2000);Check(driver.writes==2,"a busy device never gets overlapping beeps");
  audio.Tick(false,0,2010);Check(driver.resets==1,"opening a menu cancels an in-flight beep");
  audio.Tick(false,0,2020);Check(driver.resets==1,"muted polling does not repeatedly reset the device");
  audio.Tick(true,150,2030);Check(driver.writes==3,"resuming combat starts with a fresh pulse");}
 Check(driver.resets==2&&driver.unprepares==1&&driver.closes==1,"shutdown releases owned audio after stopping its queue");
 {driver={};driver.open_fail=true;BeaconAudio audio(Api());Check(audio.Tick(true,825,1000)==BeaconAudioResult::Unavailable,"missing audio device returns an explicit unavailable result");
  audio.Tick(true,825,1100);Check(driver.opens==1&&driver.writes==0,"failed output is paced instead of retried every worker tick");
  driver.open_fail=false;audio.Tick(true,825,3000);Check(driver.opens==2&&driver.writes==1,"a recovered device can resume the beacon");}
 {driver={};driver.write_fail=true;BeaconAudio audio(Api());Check(audio.Tick(true,150,1000)==BeaconAudioResult::Unavailable&&driver.unprepares==1&&driver.closes==1,"write failure closes the owned stream and keeps speech independent");}
 {driver={};auto api=Api();api.write=nullptr;BeaconAudio audio(api);Check(audio.Tick(true,150,1000)==BeaconAudioResult::Unavailable&&driver.opens==0,"missing wave API entry never calls a null pointer");}
 {driver={};BeaconAudio audio(Api());audio.Tick(true,150,1000);driver.reset_fail=true;
  Check(audio.Tick(false,0,1010)==BeaconAudioResult::Unavailable,"a failed cancellation is reported");
  audio.Tick(true,150,4000);Check(driver.opens==1&&driver.writes==1&&driver.unprepares==0&&driver.closes==0,"unretirable driver-owned buffer is retained and no new stream is opened");}
 Check(driver.buffer&&driver.buffer->lpData,"driver-held memory remains valid after failed-reset shutdown");
 std::cout<<checks<<" beacon audio checks, "<<failures<<" failures\n";return failures?1:0;
}
