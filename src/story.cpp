#include "mvci/story.hpp"
#include "story_read.hpp"
#include <cmath>
namespace mvci {
StoryCaptureResult story_surfaces(Unreal&,Address,const std::function<std::optional<int>(Address)>&);
void story_hud(Unreal&,Address,StorySnapshot&);
StoryTextSnapshot fight_announcements(Unreal&,Address,const std::function<std::optional<int>(Address)>&);
namespace {
struct NativeArray {Address data;int count,capacity;};
std::optional<Address> first(const Unreal& u,Address at){
 NativeArray a{};if(!u.memory().read(at,a)||a.count<1||a.count>a.capacity||a.capacity>16||a.capacity<0)return {};
 return u.memory().get<Address>(a.data);
}
bool clean(const std::optional<ObjectRef>& o){return o&&!(o->flags&0x30);}
std::uint64_t tag(const ObjectRef& o,std::uint64_t kind){return ((static_cast<std::uint64_t>(static_cast<std::uint32_t>(o.serial))<<32)|static_cast<std::uint32_t>(o.index))^kind;}
std::uint64_t rotate(std::uint64_t value,unsigned shift){return (value<<shift)|(value>>(64-shift));}
bool in_world(const Unreal& u,Address actor,Address current){
 if(!clean(u.object(actor)))return false;const auto level=u.memory().get<Address>(actor+0x20);
 return level&&u.is_a(*level,L"Level")&&u.memory().get<Address>(*level+0xb0)==std::optional<Address>(current);
}
bool environment_world(story_detail::Frame& f,Address environment,Address current){
 if(!f.retain(environment))return false;const auto level=f.get<Address>(environment+0x20);return level&&*level&&f.retain(*level)&&f.u.is_a(*level,L"Level")&&f.get<Address>(*level+0xb0)==std::optional<Address>(current);
}
std::optional<StoryTextSnapshot> caption(Unreal& u,Address environment,Address current,const wchar_t* field,const wchar_t* type,const std::function<std::optional<int>(Address)>& visibility){
 story_detail::Frame f{u,visibility,{},{},{}};if(!environment_world(f,environment,current))return {};
 const auto root=f.field(environment,field);if(!root||!*root||!u.is_a(*root,type)||!f.active(*root)||!f.shown(*root))return {};
 const auto child=f.field(*root,L"WD_FreeText_34");if(!child||!*child||!u.is_a(*child,L"WD_FreeText_C")||!f.within(*child,*root)||!f.shown(*child))return {};
 const auto block=f.field(*child,L"TextBlock_32");if(!block||!*block||!u.is_a(*block,L"TextBlock")||!f.within(*block,*child)||!f.shown(*block))return {};
 const auto alpha=f.text_alpha(*block);if(!alpha||*alpha<=0)return {};const auto value=f.display(*block);if(!value||!story_detail::readable(*value)||!f.finish())return {};
 const auto ref=u.object(*root);if(!ref)return {};return StoryTextSnapshot{true,tag(*ref,0x43415054494f4e00ull),*value};
}
std::optional<StoryTextSnapshot> subtitle(Unreal& u,Address environment,Address current,const std::function<std::optional<int>(Address)>& visibility){
 story_detail::Frame f{u,visibility,{},{},{}};if(!environment_world(f,environment,current))return {};
 const auto root=f.field(environment,L"Subtitle_Obj");if(!root||!*root||!u.is_a(*root,L"WD_StorySubtitle_C")||!f.active(*root)||!f.shown(*root))return {};
 const auto child=f.field(*root,L"WD_Subtitling_30");if(!child||!*child||!u.is_a(*child,L"WD_Subtitling_C")||!f.within(*child,*root)||!f.shown(*child))return {};
 std::wstring text;std::set<Address> seen;
 // Text1..4 are alternative complete layouts (not four stored subtitle lines).
 // Left/right are the special two-speaker layout. Only rendered blocks count.
 for(auto field:{L"Text1",L"Text2",L"Text3",L"Text4",L"TextBlock_Left",L"TextBlock_Right"}){
  const auto b=f.field(*child,field);if(!b||!*b||!seen.insert(*b).second||!f.shown(*b))continue;
  if(!u.is_a(*b,L"TextBlock")||!f.within(*b,*child))return {};
  const auto alpha=f.text_alpha(*b);if(!alpha)return {};if(*alpha<=0)continue;
  const auto value=f.display(*b);if(!value)return {};if(value->empty())continue;if(!story_detail::readable(*value))return {};
  if(!text.empty())text+=L" ";text+=*value;if(text.size()>8192)return {};
 }
 if(text.empty()||!f.finish())return {};const auto ref=u.object(*root);if(!ref)return {};
 return StoryTextSnapshot{true,tag(*ref,0x5355425449544c45ull),text};
}
bool presented(Unreal& u,Address movie,Address texture,const std::function<std::optional<int>(Address)>& visibility){
 story_detail::Frame f{u,visibility,{},{},{}};if(!f.retain(movie)||f.field(movie,L"ManaTexture")!=std::optional<Address>(texture)||f.integer(movie,L"State")!=std::optional<int>(3))return false;
 const auto widget=f.field(movie,L"MoviePlayWidget");if(!widget||!*widget||!u.is_a(*widget,L"WD_MoviePlay_C")||!f.active(*widget)||!f.shown(*widget))return false;
 const auto image=f.field(*widget,L"Image_48"),material=f.field(*widget,L"MID_Image");
 return image&&*image&&u.is_a(*image,L"Image")&&f.within(*image,*widget)&&f.shown(*image)&&material&&*material&&u.is_a(*material,L"MaterialInstanceDynamic")&&f.image_resource(*image)==material&&f.finish();
}
std::optional<bool> covered(Unreal& u,Address environment,Address current,const std::function<std::optional<int>(Address)>& visibility){
 story_detail::Frame f{u,visibility,{},{},{}};if(!environment_world(f,environment,current))return {};
 const auto fade=f.field(environment,L"ViewFade_Obj");if(!fade)return {};if(!*fade)return f.finish()?std::optional<bool>(false):std::nullopt;
 if(!u.is_a(*fade,L"WD_DemoViewFade_C"))return {};const auto image=f.field(*fade,L"FadeImage");
 if(!image||!*image||!u.is_a(*image,L"Image")||!f.within(*image,*fade))return {};
 const auto color=u.property(*image,L"ColorAndOpacity");if(!color||color->kind!=L"StructProperty"||color->size!=16)return {};
 const auto alpha=f.get<float>(*image+static_cast<Address>(color->offset)+12);const auto visible=f.integer(*image,L"Visibility");
 if(!alpha||!std::isfinite(*alpha)||*alpha<0||*alpha>1||!visible||*visible<0||*visible>4||!f.finish())return {};
 // BP_SequencerMovieEnv adds this overlay at Z10; captions are at Z3 and
 // subtitles Z15. Suppress descriptions/location text during any visible fade.
 return *alpha>0&&*visible!=1&&*visible!=2;
}
std::wstring movie_id(const Unreal& u,Address sequence){
 if(!u.name_is(sequence,L"SQ_Story")||!u.is_a(sequence,L"LevelSequence"))return {};
 const auto outer=u.memory().get<Address>(sequence+0x20);if(!outer||!u.is_a(*outer,L"Package"))return {};
 auto path=u.name(*outer);for(auto& c:path)if(c>=L'a'&&c<=L'z')c-=L'a'-L'A';
 const std::wstring prefix=L"/GAME/DEMO/DATA/",tail=L"/SQ_STORY";
 if(path.size()!=prefix.size()+3+1+3+tail.size()||path.rfind(prefix,0)!=0||path.substr(path.size()-tail.size())!=tail)return {};
 const auto scene=path.substr(prefix.size(),3),cut=path.substr(prefix.size()+4,3);
 if(scene[0]<L'0'||scene[0]>L'9'||scene[1]!=L'_'||scene[2]<L'A'||scene[2]>L'Z'||cut[0]!=L'D'||cut[1]<L'0'||cut[1]>L'9'||cut[2]<L'0'||cut[2]>L'9')return {};
 return scene.substr(0,1)+L"-"+scene.substr(2)+L"_DEMO"+cut.substr(1);
}
}
StoryCaptureResult StoryCapture::sample(std::uint64_t,bool read_hud){
 StoryCaptureResult result;rejection_=L"story: no unique current sequence";const auto current=unreal_.current_world();if(!current)return result;
 result=story_surfaces(unreal_,*current,visibility_);
 if(!result.story.captions[4].valid&&!result.story.loading.visible){
  const auto announcement=fight_announcements(unreal_,*current,visibility_);
  if(announcement.valid){result.story.valid=true;result.story.captions[4]=announcement;}
 }
 if(read_hud)story_hud(unreal_,*current,result.story);
 std::optional<ObjectRef> system,actor,player,sequence;std::optional<double> position;bool paused=false;
 for(const auto& candidate:unreal_.instances(L"DemoSystem")){
  if(!clean(unreal_.object(candidate.address)))continue;
  // The exact native GetSequencePlaybackPosition chain, also verified against
  // reflected TimeCursorPosition during the user's pause/resume check.
  const auto a=first(unreal_,candidate.address+0xa0);if(!a||!*a||!unreal_.is_a(*a,L"LevelSequenceActor")||!in_world(unreal_,*a,*current))continue;
  const auto p=unreal_.memory().get<Address>(*a+0x3c0);if(!p||!unreal_.is_a(*p,L"LevelSequencePlayer"))continue;
  const auto seq=unreal_.object_field(*p,L"Sequence");if(!seq||!*seq||movie_id(unreal_,*seq).empty())continue;
  const auto time=unreal_.number(*p,L"TimeCursorPosition"),start=unreal_.number(*p,L"StartTime"),end=unreal_.number(*p,L"EndTime");const auto pause=unreal_.memory().get<std::uint8_t>(candidate.address+0xb0);
  if(!time||!start||!end||*end<*start||*time<*start-.5||*time>*end+.5||!pause||*pause>1)continue;
  if(system){rejection_=L"story: ambiguous current sequence";return {};}
  system=candidate;actor=unreal_.object(*a);player=unreal_.object(*p);sequence=unreal_.object(*seq);position=time;paused=*pause!=0;
 }
 if(!system||!actor||!player||!sequence)return result;
 auto& s=result.story;s.valid=true;s.context=tag(*actor,0x53544f5259000000ull)^rotate(tag(*sequence,0),17)^rotate(tag(*player,0),37);s.movie.context=s.context;s.movie.id=movie_id(unreal_,sequence->address);s.movie.sequence_seconds=position;s.movie.paused=paused;
 const auto m=first(unreal_,system->address+0x90);if(m&&*m&&unreal_.is_a(*m,L"MoviePlayBase")&&in_world(unreal_,*m,*current)){
  const auto mr=unreal_.object(*m);const auto texture=unreal_.object_field(*m,L"ManaTexture"),component=unreal_.object_field(*m,L"mpManaComponent");const auto state=unreal_.integer(*m,L"State");
  if(mr&&texture&&*texture&&unreal_.is_a(*texture,L"ManaTexture")&&unreal_.name_is(*texture,s.movie.id)&&component&&*component&&unreal_.is_a(*component,L"ManaComponent")&&unreal_.memory().get<Address>(*component+0x20)==m&&unreal_.memory().get<Address>(*component+0xf0)==texture&&state&&*state>=0&&*state<=4){
   const auto cr=unreal_.object(*component),tr=unreal_.object(*texture);s.movie.valid=true;s.movie.playing=*state==3;
   // Playback objects can be replaced while the sequence and its cursor stay
   // continuous. Their registry identities must retire the old owned recording.
   if(cr&&tr)s.movie.context^=rotate(tag(*mr,0),7)^rotate(tag(*cr,0),23)^rotate(tag(*tr,0),47);
   if(s.movie.playing){const auto frame=unreal_.memory().get<int>(*component+0x1f8),ms=unreal_.integer(*component,L"mManaPlayTimeMS");
    if(frame&&*frame>=0&&*frame<=10000000)s.movie.frame=frame;
    if(ms&&*ms>=0)s.movie.movie_seconds=static_cast<double>(*ms)/1000.;
    if(!s.movie.frame||!s.movie.movie_seconds)s.movie.valid=false;
    s.movie.presented=presented(unreal_,*m,*texture,visibility_);
    if(s.movie.valid&&s.movie.presented){
     // UpdateManualTimerSyncSound stores the last native sound time on the
     // movie actor. Mana subtracts this component's origin before using it.
     const auto sound=unreal_.memory().get<int>(*m+0x390);
     const auto origin=unreal_.memory().get<int>(*component+0x1fc);
     if(sound&&origin&&*origin>=0&&*sound>=*origin)
      s.movie.soundtrack=MovieSoundtrack{*origin,*sound-*origin};
    }
   }
   if(!cr||!tr||!unreal_.alive(*mr)||!unreal_.alive(*cr)||!unreal_.alive(*tr)||first(unreal_,system->address+0x90)!=m||unreal_.object_field(*m,L"ManaTexture")!=texture||unreal_.object_field(*m,L"mpManaComponent")!=component||unreal_.memory().get<Address>(*component+0x20)!=m||unreal_.memory().get<Address>(*component+0xf0)!=texture||unreal_.integer(*m,L"State")!=state)s.movie.valid=false;
  }
 }
 std::array<bool,3> ambiguous{};bool obscured=false;
 for(const auto& environment:unreal_.instances(L"BP_SequencerMovieEnv_C")){
  if(!in_world(unreal_,environment.address,*current))continue;
  // Story subtitle roots are added at Z15, above the Z10 fade overlay.
  if(const auto text=subtitle(unreal_,environment.address,*current,visibility_);text&&!ambiguous[2]){auto& prior=s.subtitle;if(prior.valid&&(prior.identity!=text->identity||prior.text!=text->text)){prior={};ambiguous[2]=true;}else prior=*text;}
  const auto cover=covered(unreal_,environment.address,*current,visibility_);if(!cover||*cover){obscured=true;continue;}
  for(int i=0;i<2;++i){const auto text=caption(unreal_,environment.address,*current,i==0?L"StageTitle_Obj":L"DayText_Obj",i==0?L"WD_StageTitle_C":L"WD_DayText_C",visibility_);if(!text||ambiguous[static_cast<std::size_t>(i)])continue;
   auto& prior=s.captions[static_cast<std::size_t>(i)];if(prior.valid&&(prior.identity!=text->identity||prior.text!=text->text)){prior={};ambiguous[static_cast<std::size_t>(i)]=true;}else prior=*text;
  }
 }
 if(obscured){s.movie.presented=false;s.captions[0]={};s.captions[1]={};}
 if(!s.movie.valid||!s.movie.presented)s.movie.soundtrack.reset();
 if(!unreal_.alive(*system)||!unreal_.alive(*actor)||!unreal_.alive(*player)||!unreal_.alive(*sequence)||unreal_.current_world()!=current||first(unreal_,system->address+0xa0)!=std::optional<Address>(actor->address)||unreal_.memory().get<Address>(actor->address+0x3c0)!=std::optional<Address>(player->address)||unreal_.object_field(player->address,L"Sequence")!=std::optional<Address>(sequence->address)||unreal_.memory().get<std::uint8_t>(system->address+0xb0)!=std::optional<std::uint8_t>(static_cast<std::uint8_t>(paused?1:0))||!in_world(unreal_,actor->address,*current))return {};
 rejection_.clear();return result;
}
}
