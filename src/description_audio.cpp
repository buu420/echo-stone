#include "mvci/descriptions.hpp"
#include "mvci/description_bank.hpp"
#include "mvci/description_wave.hpp"
#include <filesystem>
namespace mvci {
namespace {
const DescriptionMovie* find_movie(const std::wstring& id){for(const auto& movie:description_catalog())if(movie.id==id)return &movie;return nullptr;}
const char* stage_name(DescriptionBankStage s){switch(s){case DescriptionBankStage::Idle:return "idle";case DescriptionBankStage::Loading:return "loading";case DescriptionBankStage::Ready:return "ready";case DescriptionBankStage::Failed:return "failed";}return "unknown";}
}
struct DescriptionAudio::Impl:DescriptionOutput {
 DescriptionWave wave;
 DescriptionBankLoader loader;
 std::shared_ptr<const DescriptionBank> bank;
 const DescriptionMovie* movie=nullptr;
 DescriptionSession session;
 std::uint64_t context=0,timing_bad_at=0;
 bool timing_bad=false;
 std::function<bool()> current;
 std::uint64_t sampled=0;
 explicit Impl(std::wstring root,std::function<bool()> guard):loader(std::filesystem::path(std::move(root))/L"descriptions"),session(*this),current(std::move(guard)){}
 bool playing()const override{return wave.playing();}
 bool start(const DescriptionCue& cue,double window_time)override{
  if(!bank||bank->movie!=movie||!movie)return false;
  for(std::size_t i=0;i<movie->cues.size();++i)if(&movie->cues[i]==&cue)return i<bank->clips.size()&&wave.play(bank->clips[i],[this,&cue,window_time]{
   if(current&&!current())return false;
   const auto now=GetTickCount64();if(now<sampled||now-sampled>250)return false;
   // Include time spent in the audio driver when checking the cue deadline.
   return window_time+static_cast<double>(now-sampled)/1000.+cue.duration<=cue.end+.001;
  });return false;
 }
 void pause()override{wave.pause();}
 void resume()override{wave.resume();}
 void stop()override{wave.stop();}
 void prepare(const Snapshot& s,std::uint64_t now,bool enabled){
  const auto& native=s.story.movie;const auto target=enabled?find_movie(native.id):nullptr;
  if(target!=movie){bank.reset();movie=target;loader.request(target);}
  const auto view=loader.view();bank=view.movie==movie?view.bank:nullptr;
  if(native.context!=context){context=native.context;timing_bad=false;timing_bad_at=0;}
  const bool fresh=s.foreground&&now>=s.observed_ms&&now-s.observed_ms<=250;
  const bool bad=enabled&&fresh&&movie&&native.playing&&native.presented&&!native.paused&&
                 !description_movie_time(native,*movie);
  if(bad&&!timing_bad){timing_bad_at=now;timing_bad=true;}else if(!bad){timing_bad=false;timing_bad_at=0;}
 }
 StoryTextSnapshot notice(const Snapshot& s,bool enabled)const{
  const auto& n=s.story.movie;if(!enabled||!s.foreground||!n.context||!n.playing||n.id.empty())return {};
  std::wstring text;
  if(!movie)text=L"Audio descriptions are not yet available for this movie.";
  else if(timing_bad&&GetTickCount64()>=timing_bad_at&&GetTickCount64()-timing_bad_at>=500)text=L"Audio description timing is unavailable for this movie.";
  else if(loader.view().stage==DescriptionBankStage::Failed)text=L"Audio description recordings could not be loaded.";
  else if(!wave.healthy())text=L"Audio description playback is unavailable.";
  return text.empty()?StoryTextSnapshot{}:StoryTextSnapshot{true,n.context^0x4445534352495054ull,text};
 }
 std::string diagnostic()const{
  return std::string("bank=")+stage_name(loader.view().stage)+" active="+std::to_string(wave.playing())+" device_ok="+std::to_string(wave.healthy())+" timing_bad="+std::to_string(timing_bad);
 }
};
DescriptionAudio::DescriptionAudio(std::wstring root,std::function<bool()> current):impl_(std::make_unique<Impl>(std::move(root),std::move(current))){}
DescriptionAudio::~DescriptionAudio()=default;
void DescriptionAudio::prepare(const Snapshot& s,std::uint64_t now,bool enabled){impl_->prepare(s,now,enabled);}
StoryTextSnapshot DescriptionAudio::notice(const Snapshot& s,bool enabled)const{return impl_->notice(s,enabled);}
void DescriptionAudio::tick(const Snapshot& s,std::uint64_t now,bool enabled,bool interrupted){
 impl_->prepare(s,now,enabled);
 impl_->sampled=s.observed_ms;
 impl_->session.observe(s,impl_->movie,now,enabled,interrupted);
}
std::string DescriptionAudio::diagnostic()const{return impl_->diagnostic();}
}
