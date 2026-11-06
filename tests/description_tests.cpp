#include "mvci/descriptions.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <map>
#include <stdexcept>
using namespace mvci;
static int checks=0;
#define CHECK(x) do{++checks;if(!(x))throw std::runtime_error(std::string(__func__)+":"+std::to_string(__LINE__)+" " #x);}while(false)
struct Output:DescriptionOutput {
 bool busy=false,paused=false,accept=true;int starts=0,stops=0,pauses=0,resumes=0;
 std::string id;
 bool playing()const override{return busy;}
 bool start(const DescriptionCue& c,double)override{if(!accept)return false;busy=true;id=c.id;++starts;return true;}
 void pause()override{paused=true;++pauses;}
 void resume()override{paused=false;++resumes;}
 void stop()override{busy=false;paused=false;++stops;}
};
static DescriptionMovie movie(){return {L"0-A_DEMO01",90,7.25,{{"first","first.wav","",L"First",1,4,2,100},{"second","second.wav","",L"Second",10,14,2,100}}};}
static Snapshot sample(double time,std::uint64_t now=1000){Snapshot s;s.foreground=true;s.observed_ms=now;s.story.valid=true;auto& m=s.story.movie;m.valid=true;m.playing=true;m.presented=true;m.context=123;m.id=L"0-A_DEMO01";m.movie_seconds=time;m.sequence_seconds=time+7.25;m.frame=static_cast<int>(time*30);return s;}
static void native_time_starts_and_pauses_the_owned_clip(){
 Output out;DescriptionSession session(out);const auto film=movie();session.observe(sample(.5),&film,1000,true);CHECK(!out.busy);session.observe(sample(1.0,1033),&film,1033,true);CHECK(out.busy&&out.id=="first");CHECK(out.starts==1);
 auto s=sample(1.2,1066);s.story.movie.paused=true;session.observe(s,&film,1066,true);CHECK(out.paused);s.observed_ms=1300;session.observe(s,&film,1300,true);CHECK(out.pauses==1);s=sample(1.3,1333);session.observe(s,&film,1333,true);CHECK(!out.paused&&out.resumes==1);session.observe(sample(4.1,1400),&film,1400,true);CHECK(!out.busy);
}
static void opening_movie_uses_presented_time_when_sequence_drifts(){
 const auto& catalog=description_catalog();
 const auto film=std::find_if(catalog.begin(),catalog.end(),[](const DescriptionMovie& entry){
  return entry.id==L"0-B_DEMO01";
 });
 CHECK(film!=catalog.end());
 Output out;
 DescriptionSession session(out);
 const auto observe=[&](double movie_seconds,double sequence_seconds,std::uint64_t now,bool paused=false){
  auto s=sample(movie_seconds,now);
  s.story.movie.id=film->id;
  s.story.movie.sequence_seconds=sequence_seconds;
  s.story.movie.paused=paused;
  session.observe(s,&*film,now,true);
 };

 // The opening sequence has a 44.7 second pre-roll. The live log reached
 // frame zero at 44.732 and stopped the first recording 625 ms after starting.
 // Simulate a separate sequence cursor drifting beyond the old 200 ms gate.
 observe(0,44.732,1000);
 CHECK(out.starts==0);
 observe(4.400,49.132,5500);
 observe(4.466,49.198,5533);
 CHECK(out.playing()&&out.id=="story-0-b-01-001");
 observe(5.100,50.075,6158);
 CHECK(out.playing()&&out.starts==1);

 observe(5.100,50.108,6191,true);
 CHECK(out.playing()&&out.paused&&out.pauses==1);
 observe(5.100,50.108,6750,true);
 CHECK(out.paused&&out.starts==1&&out.pauses==1);
 observe(5.133,50.141,6783);
 CHECK(out.playing()&&!out.paused&&out.resumes==1&&out.starts==1);

 observe(7.533,52.541,9183);
 CHECK(!out.playing());
 observe(7.566,52.574,9216);
 CHECK(out.playing()&&out.id=="xgard-001"&&out.starts==2);
}
static void second_movie_skips_a_line_after_its_soundtrack_window(){
 const auto& catalog=description_catalog();
 const auto film=std::find_if(catalog.begin(),catalog.end(),[](const DescriptionMovie& entry){
  return entry.id==L"0-B_DEMO02";
 });
 CHECK(film!=catalog.end());
 Output out;
 DescriptionSession session(out);
 const auto observe=[&](DescriptionSession& target,double movie_seconds,double sequence_seconds,double soundtrack_seconds,std::uint64_t now){
  auto s=sample(movie_seconds,now);
  s.story.movie.id=film->id;
  s.story.movie.sequence_seconds=sequence_seconds;
  s.story.movie.soundtrack=MovieSoundtrack{0,static_cast<int>(std::lround(soundtrack_seconds*1000))};
  target.observe(s,&*film,now,true);
 };

 // Candidate 14 first observed frame 206, then started a line on frame 276
 // when the sequence was already at its end. Simulate the soundtrack having
 // moved ahead as well. The original trace did not record its timestamp.
 observe(session,6.866,12.577,12.577,1000);
 observe(session,9.200,25.233335,25.233335,21031);
 CHECK(!out.playing()&&out.starts==0);
 observe(session,17.066,25.233335,25.233335,34844);
 CHECK(out.starts==0);

 Output aligned;
 DescriptionSession aligned_session(aligned);
 observe(aligned_session,6.866,12.577,6.899,1000);
 observe(aligned_session,9.200,25.233335,9.233,21031);
 CHECK(aligned.playing()&&aligned.id=="story-0-b-02-003"&&aligned.starts==1);
}
static void soundtrack_deadline_keeps_picture_start_and_retires_stalled_audio(){
 const auto film=movie();
 Output out;
 DescriptionSession session(out);
 const auto observe=[&](double movie_seconds,double soundtrack_seconds,std::uint64_t now){
  auto s=sample(movie_seconds,now);
  s.story.movie.soundtrack=MovieSoundtrack{0,static_cast<int>(std::lround(soundtrack_seconds*1000))};
  session.observe(s,&film,now,true);
 };

 observe(.5,.6,1000);
 observe(.95,1.02,1033);
 CHECK(out.starts==0);
 observe(1.0,1.05,1066);
 CHECK(out.playing()&&out.starts==1);
 observe(1.0,1.2,1233);
 CHECK(out.paused);
 observe(1.1,4.05,1266);
 CHECK(!out.playing()&&out.resumes==0);
 observe(10.03,10.08,1300);
 CHECK(out.playing()&&out.id=="second"&&out.starts==2);
}
static void soundtrack_reset_does_not_retire_a_still_visible_cue(){
 const auto film=movie();
 for(int reset=0;reset<4;++reset){
  Output out;
  DescriptionSession session(out);
  const auto observe=[&](double picture,std::optional<MovieSoundtrack> sound,std::uint64_t now,bool interrupted=false){
   auto s=sample(picture,now);
   s.story.movie.soundtrack=sound;
   session.observe(s,&film,now,true,interrupted);
  };
  observe(.5,MovieSoundtrack{0,3900},1000);
  observe(.95,MovieSoundtrack{0,4000},1033);
  observe(1.0,MovieSoundtrack{0,4000},1066,true);
  observe(1.033,MovieSoundtrack{0,4000},1099);
  observe(1.066,MovieSoundtrack{0,4000},1132);
  CHECK(out.starts==0);

  if(reset==0)observe(1.1,MovieSoundtrack{100,4000},1165);
  if(reset==1)observe(1.1,{},1165);
  if(reset==2)observe(1.1,MovieSoundtrack{0,800},1165);
  if(reset==3){
   observe(.6,MovieSoundtrack{0,4000},1165);
   observe(1.1,MovieSoundtrack{0,4000},1198);
  }
  CHECK(out.playing()&&out.id=="first"&&out.starts==1);
 }
}
static void every_safety_gate_cancels_its_own_output(){
 const auto film=movie();for(int gate=0;gate<10;++gate){Output out;DescriptionSession session(out);session.observe(sample(.5),&film,1000,true);session.observe(sample(1,1033),&film,1033,true);CHECK(out.busy);auto s=sample(1.1,1066);bool enabled=true,interrupt=false;
  if(gate==0)s.foreground=false;if(gate==1)s.observed_ms=1;if(gate==2)s.story.movie.valid=false;if(gate==3)s.story.movie.playing=false;if(gate==4)s.story.movie.id=L"Other";if(gate==5)enabled=false;if(gate==6)interrupt=true;if(gate==7)s.story.movie.movie_seconds=std::numeric_limits<double>::quiet_NaN();if(gate==8)s.story.movie.frame.reset();if(gate==9)s.story.movie.presented=false;
  session.observe(s,&film,1066,enabled,interrupt);CHECK(!out.busy);
 }
}
static void stalled_clock_pauses_and_advancing_clock_resumes(){
 Output out;DescriptionSession session(out);const auto film=movie();session.observe(sample(.5),&film,1000,true);session.observe(sample(1,1033),&film,1033,true);session.observe(sample(1,1100),&film,1100,true);CHECK(!out.paused);session.observe(sample(1,1200),&film,1200,true);CHECK(out.paused);session.observe(sample(1.1,1233),&film,1233,true);CHECK(!out.paused&&out.resumes==1);
}
static void late_banks_rejected_output_seeks_and_owners_do_not_replay_expired_cues(){
 const auto film=movie();Output out;DescriptionSession session(out);session.observe(sample(3),&film,1000,true);session.observe(sample(3.1,1033),&film,1033,true);CHECK(!out.busy);session.observe(sample(10,1100),&film,1100,true);CHECK(out.id=="second");CHECK(out.starts==1);
 auto s=sample(10.1,1133);s.story.movie.context=124;session.observe(s,&film,1133,true);CHECK(!out.busy);s=sample(10.2,1166);s.story.movie.context=124;session.observe(s,&film,1166,true);CHECK(out.starts==2);session.observe(sample(1,1200),&film,1200,true);CHECK(!out.busy);session.observe(sample(1.1,1233),&film,1233,true);CHECK(out.id=="first");
 Output retry;retry.accept=false;DescriptionSession r(retry);r.observe(sample(.5),&film,1000,true);r.observe(sample(1,1033),&film,1033,true);CHECK(!retry.busy);retry.accept=true;r.observe(sample(1.1,1066),&film,1066,true);CHECK(retry.busy);
}
static void public_recordings_survive_normal_capture_and_worker_phase_alignment(){
 // A ready driver starts 10 ms after the 20 ms worker. Include a one-frame
 // soundtrack lead when checking the reviewed windows.
 struct ClockedOutput:DescriptionOutput {
  std::uint64_t now=0,sampled=0,until=0;bool busy=false;std::map<std::string,int> starts;
  bool playing()const override{return busy&&now<until;}
  bool start(const DescriptionCue& cue,double window_time)override{
   const double delivered=window_time+(now-sampled+10)/1000.;if(delivered+cue.duration>cue.end+.001)return false;
   ++starts[cue.id];busy=true;until=now+10+static_cast<std::uint64_t>(std::ceil(cue.duration*1000));return true;
  }
  void stop()override{busy=false;}void pause()override{}void resume()override{}
 };
 for(const auto& film:description_catalog())for(const int cadence:{50,80,100})for(int phase=0;phase<cadence;++phase){
  ClockedOutput out;DescriptionSession session(out);Snapshot s;s.foreground=true;int capture=phase;
  for(int ms=0;ms<static_cast<int>(film.duration*1000)+40;ms+=20){
   while(capture<=ms){s.observed_ms=1000+capture;auto& native=s.story.movie;
    native.valid=native.playing=native.presented=true;native.context=123;native.id=film.id;
    native.movie_seconds=capture/1000.;native.sequence_seconds=*native.movie_seconds+film.sequence_offset;
    native.soundtrack=MovieSoundtrack{0,capture+33};native.frame=capture*30/1000;capture+=cadence;}
   out.now=ms;out.sampled=s.observed_ms>=1000?s.observed_ms-1000:0;session.observe(s,&film,1000+ms,true);
  }
  for(const auto& cue:film.cues){++checks;if(out.starts[cue.id]!=1)throw std::runtime_error("Catalog missed "+cue.id+" cadence="+std::to_string(cadence)+" phase="+std::to_string(phase));}
 }
}
int main(){int failed=0;for(auto test:{native_time_starts_and_pauses_the_owned_clip,opening_movie_uses_presented_time_when_sequence_drifts,second_movie_skips_a_line_after_its_soundtrack_window,soundtrack_deadline_keeps_picture_start_and_retires_stalled_audio,soundtrack_reset_does_not_retire_a_still_visible_cue,every_safety_gate_cancels_its_own_output,stalled_clock_pauses_and_advancing_clock_resumes,late_banks_rejected_output_seeks_and_owners_do_not_replay_expired_cues,public_recordings_survive_normal_capture_and_worker_phase_alignment})try{test();}catch(const std::exception& e){std::cerr<<e.what()<<'\n';++failed;}std::cout<<checks<<" description checks, "<<failed<<" failed groups\n";return failed?1:0;}
