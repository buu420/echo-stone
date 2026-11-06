#include "mvci/model.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <locale>
#include <sstream>
namespace mvci {
namespace {
constexpr std::uint64_t freshness_ms=250;
constexpr std::size_t loading_caption=3;
bool range(const std::optional<double>& v,double low,double high){return v&&std::isfinite(*v)&&*v>=low&&*v<=high;}
std::wstring number(double value){
 std::wostringstream s;s.imbue(std::locale::classic());s<<std::fixed<<std::setprecision(2)<<value;
 auto out=s.str();while(!out.empty()&&out.back()==L'0')out.pop_back();if(!out.empty()&&out.back()==L'.')out.pop_back();
 const auto language=formatting_language();
 if(language==Language::Arabic)std::replace(out.begin(),out.end(),L'.',L'\u066b');
 else if(language==Language::German||language==Language::Spanish||
         language==Language::French||
         language==Language::Italian||language==Language::Polish||
         language==Language::BrazilianPortuguese||language==Language::Russian)
  std::replace(out.begin(),out.end(),L'.',L',');
 return out;
}
std::wstring percent(double v){return std::to_wstring(static_cast<int>(std::floor(v*100+0.000001)));}
std::wstring health_text(const FighterSnapshot& fighter,bool tracked){
 if(!range(fighter.health_percent,0,100))
  return tracked?tr(L"Your health unavailable."):tr(L"Opponent health unavailable.");
 const double health=*fighter.health_percent;
 const auto value=health==0?0L:health==100?100L:std::clamp(std::lround(health),1L,99L);
 const auto amount=std::to_wstring(value);
 return tracked?tr(L"Your health {0} percent.",{amount}):tr(L"Opponent health {0} percent.",{amount});
}
std::wstring health_source(const Snapshot& s,int team){
 if(!s.combat.valid||!s.combat.playable||!s.combat.session||s.menu.valid||s.menu.blocked)return {};
 const auto& fighter=s.combat.fighters[team];
 if(!fighter.health_fighter||!range(fighter.health_percent,0,100))return {};
 return L"combat:health:"+std::to_wstring(team)+L"|"+std::to_wstring(s.combat.session)+L"|"+
  std::to_wstring(*fighter.health_fighter);
}
std::wstring meter_ready(int level){return tr(L"Level {0} meter ready.",{std::to_wstring(level)});}
std::wstring menu_text(const MenuSnapshot& m){
 std::wstring text;if(!m.screen.empty())text=m.screen+L". ";text+=m.label;
 if(!m.value.empty())text+=L", "+m.value;
 if(!m.enabled)text+=L". "+tr(L"Unavailable");
 if(m.position>0&&m.count>=m.position)text+=L". "+tr(L"{0} of {1}",{std::to_wstring(m.position),std::to_wstring(m.count)});
 if(!m.help.empty())text+=L". "+m.help;
 return text+L".";
}
std::wstring menu_key(const MenuSnapshot& m){return std::to_wstring(m.identity)+L"|"+menu_text(m);}
std::wstring story_key(const StoryTextSnapshot& s){return s.valid&&!s.text.empty()?std::to_wstring(s.identity)+L"|"+s.text:L"";}
std::wstring story_auto_source(std::size_t index,const StoryTextSnapshot& text){const auto key=story_key(text);return key.empty()?L"":L"story:auto|"+std::to_wstring(index)+L"|"+key;}
std::wstring mission_repeat_source(const Snapshot& s){
 if(!s.mission_hud||s.menu.valid||s.menu.blocked)return {};
 return L"mission:repeat|"+std::to_wstring(s.mission_hud)+L"|"+
  std::to_wstring(s.mission_command_view)+L"|"+story_key(s.story.captions[2]);
}
std::wstring story_repeat_fingerprint(const Snapshot& s){
 if(!s.story.valid||s.menu.valid||s.menu.blocked)return {};std::wstring out=L"story:repeat|";bool any=false;
 for(const auto& text:s.story.captions)if(!story_key(text).empty()){out+=story_key(text)+L"|";any=true;}
 if(!story_key(s.story.subtitle).empty()){out+=story_key(s.story.subtitle);any=true;}return any?out:L"";
}
std::wstring story_text(const StorySnapshot& s){
 std::wstring out;const auto append=[&](const StoryTextSnapshot& value){if(!value.valid||value.text.empty())return;if(!out.empty())out+=L". ";out+=value.text;};
 for(const auto& caption:s.captions)append(caption);append(s.subtitle);return out;
}
std::optional<bool> left(const CombatSnapshot& c,int player){
 if(c.cinematic)return {};
 const auto& me=c.fighters[player].x;const auto& other=c.fighters[1-player].x;
 if(!me||!other||!std::isfinite(*me)||!std::isfinite(*other))return {};
 const double d=*me-*other;if(!std::isfinite(d)||std::abs(d)<=0.001)return {};
 return d<0;
}
std::wstring status_text(const CombatSnapshot& c,int player){
 if(!c.valid||!c.session)return tr(L"Combat status unavailable.");
 const auto& f=c.fighters[player];std::wstring text=tr(L"Player {0}.",{std::to_wstring(player+1)})+L" ";
 if(const auto side=left(c,player))text+=(*side?tr(L"Left side."):tr(L"Right side."))+L" ";
 else text+=tr(L"Side unavailable.")+L" ";
 text+=health_text(f,true)+L" "+health_text(c.fighters[1-player],false)+L" ";
 if(range(f.hyper_bars,0,4)){
  // Keep the spoken stock count on the same side of the readiness boundary
  // as the displayed digit, even when the partial bar is almost full.
  const double spoken_bars=std::min(*f.hyper_bars,std::floor(*f.hyper_bars)+0.99);
  text+=tr(L"Hyper Combo {0} of 4 bars.",{number(spoken_bars)})+L" ";
  if(*f.hyper_bars>=1)text+=meter_ready(*f.hyper_bars>=3?3:1)+L" ";
 }else text+=tr(L"Hyper Combo unavailable.")+L" ";
 if(f.storm_active&&*f.storm_active){
  text+=tr(L"Infinity Storm active.")+L" ";if(range(f.infinity_fraction,0,1))text+=tr(L"Storm gauge {0} percent remaining.",{percent(*f.infinity_fraction)});
 }else if(f.storm_active&&range(f.infinity_fraction,0,1)){
  text+=tr(L"Infinity {0} percent.",{percent(*f.infinity_fraction)});
  if(f.infinity_ready&&*f.infinity_ready)text+=L" "+tr(L"Infinity Storm ready.");
 }else text+=tr(L"Infinity unavailable.");
 return text;
}
}

std::optional<Model::HealthBase> Model::health_base(const FighterSnapshot& fighter){
 if(!fighter.health_fighter||!range(fighter.health_percent,0,100))return {};
 const double value=*fighter.health_percent;
 // The native fill is a float. Keep an exact ten-point boundary stable
 // when the normalized value represents e.g. 0.8 as 0.80000001.
 const int band=value==0?0:std::max(1,static_cast<int>(std::ceil((value-0.00001)/10)));
 return HealthBase{*fighter.health_fighter,band};
}

std::wstring menu_fingerprint(const MenuSnapshot& m){return m.valid&&!m.label.empty()?menu_key(m):L"";}
std::wstring speech_source_fingerprint(const Snapshot& s,const std::wstring& expected){
 LanguageScope language(s.language);
 if(const auto menu=menu_fingerprint(s.menu);!menu.empty())return menu;
 if(expected.rfind(L"combat:health:0|",0)==0)return health_source(s,0);
 if(expected.rfind(L"combat:health:1|",0)==0)return health_source(s,1);
 if(expected.rfind(L"mission:repeat|",0)==0)return mission_repeat_source(s);
 if(expected.rfind(L"story:auto|",0)==0){
  if(!s.story.valid||s.menu.valid||s.menu.blocked)return {};
  for(std::size_t i=0;i<s.story.captions.size();++i)if(story_auto_source(i,s.story.captions[i])==expected)return expected;
  return {};
 }
 return story_repeat_fingerprint(s);
}
Speech Model::propose(std::wstring key,std::wstring text,Priority priority,std::uint64_t sampled,std::function<void()> commit,std::wstring source_menu){
 if(pending_&&pending_->key==key&&pending_->speech.text==text&&pending_->speech.priority==priority&&pending_->speech.menu_key==source_menu){
  pending_->sampled=sampled;pending_->commit=std::move(commit);return pending_->speech;
 }
 Speech speech{next_token_++,priority,std::move(text),std::move(source_menu)};
 if(priority==Priority::Manual)speech.request=pending_request_;
 pending_=Pending{speech,std::move(key),std::move(commit),sampled};return speech;
}
bool Model::submission_fresh(std::uint64_t token,std::uint64_t submitted_ms)const{
 return pending_&&pending_->speech.token==token&&submitted_ms>=pending_->sampled&&submitted_ms-pending_->sampled<=freshness_ms;
}
bool Model::acknowledge(std::uint64_t token,bool success,std::uint64_t now,std::optional<std::uint64_t> submitted_ms){
 if(!pending_||pending_->speech.token!=token)return false;
 if(!success&&pending_->speech.priority==Priority::Manual)return false;
 const auto submitted=submitted_ms.value_or(now);
 const bool fresh_submission=now>=submitted&&submission_fresh(token,submitted);
 auto pending=std::move(*pending_);pending_.reset();
 pending_request_=Request::None;
 if(!success||!fresh_submission)return false;
 if(pending.commit)pending.commit();
 if(pending.speech.priority==Priority::Side||pending.speech.priority==Priority::Storm||pending.speech.priority==Priority::Gauge)last_combat_speech_=now;
 return true;
}

std::wstring Model::control_confirmation(Request request)const{
 switch(request){
  case Request::SelectPlayer:
   return tr(L"Tracking Player {0}.",{std::to_wstring(player_+1)});
  case Request::ToggleBeacon:
   return beacon_?tr(L"Distance beacon on."):tr(L"Distance beacon off.");
  case Request::ToggleSide:
   return side_announcements_?tr(L"Side announcements on."):tr(L"Side announcements off.");
  case Request::ToggleDescriptions:
   return descriptions_?tr(L"Story audio descriptions on."):tr(L"Story audio descriptions off.");
  default:
   return {};
 }
}

Result Model::update(const Snapshot& s,std::uint64_t now,Request request){
 LanguageScope language(s.language);
 if(language_!=s.language){
  language_=s.language;
  const auto retry=pending_&&pending_->speech.priority==Priority::Manual?pending_request_:Request::None;
  auto key=retry!=Request::None?std::move(pending_->key):std::wstring{};
  pending_.reset();
  pending_request_=Request::None;
  // A demand-driven HUD read may already have its snapshot. Keep the
  // request and retire its speech token so it can be formatted again.
  if(request==Request::None&&retry!=Request::None){
   if(retry==Request::ReadCombat||retry==Request::ReadMenu)request=retry;
   else{
    pending_request_=retry;
    propose(std::move(key),control_confirmation(retry),Priority::Manual,s.observed_ms,[]{});
   }
  }
  accepted_menu_.clear();
  for(std::size_t i=0;i<accepted_story_.size();++i)if(i!=loading_caption)accepted_story_[i].clear();
 }
 Result result;
 const bool fresh=s.foreground&&now>=s.observed_ms&&now-s.observed_ms<=freshness_ms;
 if(!fresh){
  accepted_menu_.clear();for(std::size_t i=0;i<accepted_story_.size();++i)if(i!=loading_caption)accepted_story_[i].clear();candidate_.reset();
  if(s.foreground&&pending_&&pending_->speech.priority==Priority::Manual){
   Snapshot unavailable;unavailable.language=s.language;unavailable.foreground=true;unavailable.observed_ms=now;
   return update(unavailable,now,request);
  }
  pending_.reset();pending_request_=Request::None;
  // An explicit key still gets an unavailable answer when capture stops.
  // Only mod controls and unavailable readings use this empty observation.
  if(s.foreground&&request!=Request::None){Snapshot unavailable;unavailable.language=s.language;unavailable.foreground=true;unavailable.observed_ms=now;return update(unavailable,now,request);}
  return result;
 }
 const bool readable=s.menu.valid&&!s.menu.label.empty();
 if(!readable)accepted_menu_.clear();
 const bool replacement_presented=readable||(s.combat.valid&&s.combat.playable)||
  (s.story.movie.valid&&s.story.movie.playing&&s.story.movie.presented);
 for(std::size_t i=0;i<accepted_story_.size();++i){
  // Internal travel steps hide/re-add loading, and can prepare the next HUD or
  // movie. Rearm only once a replacement presentation is actually available.
  if(i==loading_caption&&(!replacement_presented||(s.story.loading.known&&s.story.loading.visible)))continue;
  if(!s.story.valid||story_key(s.story.captions[i]).empty())accepted_story_[i].clear();
 }
 const auto& c=s.combat;
 if(c.valid&&c.session&&c.session!=session_){
  session_=c.session;resources_={};health_={};next_health_team_=player_;side_.reset();candidate_.reset();if(!pending_||pending_->speech.priority!=Priority::Manual)pending_.reset();last_combat_speech_=0;
 }
 bool retry_control=false;
 if(request==Request::None&&pending_&&pending_->speech.priority==Priority::Manual){
  if(pending_request_==Request::ReadCombat||pending_request_==Request::ReadMenu)request=pending_request_;
  else{pending_->sampled=s.observed_ms;result.speech=pending_->speech;retry_control=true;}
 }
 if(request!=Request::None)pending_request_=request;
 if(request==Request::SelectPlayer){
  player_=1-player_;resources_[player_]={};health_={};next_health_team_=player_;side_.reset();candidate_.reset();pending_.reset();
  result.speech=propose(L"player",control_confirmation(request),Priority::Manual,s.observed_ms,[]{});return result;
 }
 if(request==Request::ToggleBeacon){
  beacon_=!beacon_;result.speech=propose(L"beacon",control_confirmation(request),Priority::Manual,s.observed_ms,[]{});
 }
 if(request==Request::ToggleSide){
  side_announcements_=!side_announcements_;side_.reset();candidate_.reset();pending_.reset();
  result.speech=propose(L"side-control",control_confirmation(request),Priority::Manual,s.observed_ms,[]{});
 }
 if(request==Request::ToggleDescriptions){
  descriptions_=!descriptions_;pending_.reset();
  result.speech=propose(L"descriptions-control",control_confirmation(request),Priority::Manual,s.observed_ms,[]{});
 }
 const bool playing=c.valid&&c.session&&c.playable&&!s.menu.valid&&!s.menu.blocked;
 if(playing&&!c.cinematic&&beacon_&&c.normalized_distance&&std::isfinite(*c.normalized_distance)){
  // FighterZ's runtime uses near=200000, far=1000000: the fastest pulse
  // covers the nearest 20% of its scale. Reuse that ratio and timing curve,
  // with the MVCI-specific normalized distance supplied by native capture.
  const double amount=std::clamp((std::clamp(*c.normalized_distance,0.0,1.0)-0.2)/0.8,0.0,1.0);
  result.beacon_interval_ms=static_cast<std::uint32_t>(std::lround(150+1350*amount));
 }
 if(request==Request::ToggleBeacon||request==Request::ToggleSide||request==Request::ToggleDescriptions||retry_control)return result;
 if(request==Request::ReadMenu){
  const auto story=story_repeat_fingerprint(s);const auto key=readable?menu_key(s.menu):!story.empty()?story:L"unavailable-menu";
  const auto text=readable?menu_text(s.menu):!story.empty()?story_text(s.story):tr(L"Current menu and Story text unavailable.");
  // Mission commands are captured on demand. Keep the submitted reading alive
  // for the same HUD after releasing the capture request.
  const auto source=readable?key:story.empty()?L"":s.mission_hud?mission_repeat_source(s):key;
  result.speech=propose(key,text,Priority::Manual,s.observed_ms,[this,key,readable]{if(readable)accepted_menu_=key;},source);return result;
 }
 if(request==Request::ReadCombat){
  const auto copied=c;const int player=player_;
  auto status=status_text(c,player_);
  if(s.story.valid){
   if(s.story.enemies_remaining)status+=L" "+tr(L"Enemies remaining {0}.",{std::to_wstring(*s.story.enemies_remaining)});
   if(s.story.time_seconds)status+=L" "+tr(L"Time {0} seconds.",{std::to_wstring(*s.story.time_seconds)});
   else if(s.story.infinite_time)status+=L" "+tr(L"Time infinite.");
  }
  result.speech=propose(L"combat-status",status,Priority::Manual,s.observed_ms,[this,copied,player]{
   if(!copied.valid||copied.session!=session_)return;
   const auto& f=copied.fighters[player];auto& base=resources_[player];
   if(range(f.hyper_bars,0,4))base.stocks=static_cast<int>(std::floor(*f.hyper_bars));
   if(f.storm_active)base.storm=f.storm_active;
   if(f.storm_active&&!*f.storm_active){base.ready=f.infinity_ready;if(range(f.infinity_fraction,0,1))base.full=*f.infinity_fraction>=1;}
   for(int team=0;team<2;++team)if(const auto visible=health_base(copied.fighters[team]))health_[team]=visible;
   if(const auto side=left(copied,player))side_=side;
  });return result;
 }
 if(pending_&&pending_->speech.priority==Priority::Manual&&pending_->sampled==s.observed_ms&&now-pending_->sampled<=freshness_ms){result.speech=pending_->speech;return result;}
 if(readable){
  candidate_.reset();const auto key=menu_key(s.menu);
  if(key!=accepted_menu_)result.speech=propose(key,menu_text(s.menu),Priority::Menu,s.observed_ms,[this,key]{accepted_menu_=key;},key);else pending_.reset();
  return result;
 }
 if(s.story.valid&&!s.menu.blocked){
  // Keep each informational source independent while another caption enters or
  // leaves. Waiting proposals are rebuilt from the next current observation.
  for(std::size_t i=0;i<accepted_story_.size();++i){const auto key=story_key(s.story.captions[i]);if(key.empty()||key==accepted_story_[i]||(i==loading_caption&&!accepted_story_[i].empty()))continue;
   const auto source=story_auto_source(i,s.story.captions[i]);result.speech=propose(source,s.story.captions[i].text,Priority::Story,s.observed_ms,[this,i,key]{accepted_story_[i]=key;},source);return result;
  }
 }
 if(!playing){pending_.reset();candidate_.reset();return result;}
 auto& base=resources_[player_];const auto& f=c.fighters[player_];
 const std::optional<int> stocks=range(f.hyper_bars,0,4)?std::optional<int>(static_cast<int>(std::floor(*f.hyper_bars))):std::nullopt;
 if(stocks&&((base.stocks&&*stocks<*base.stocks)||(!base.stocks&&*stocks==0)))base.stocks=stocks;
 if(f.storm_active&&!*f.storm_active&&!base.storm)base.storm=false;
 if(f.storm_active&&!*f.storm_active){
  if(f.infinity_ready&&!*f.infinity_ready)base.ready=false;
  if(range(f.infinity_fraction,0,1)&&*f.infinity_fraction<1)base.full=false;
 }
 const auto relation=left(c,player_);
 if(!side_announcements_){candidate_.reset();side_=relation;}
 else if(!relation)candidate_.reset();
 else if(candidate_!=relation){candidate_=relation;candidate_since_=now;}
 else if((!side_||side_!=relation)&&now>=candidate_since_&&now-candidate_since_>=50){
  const auto side=*relation;result.speech=propose(side?L"side-left":L"side-right",side?tr(L"Left side."):tr(L"Right side."),Priority::Side,s.observed_ms,[this,side]{side_=side;});return result;
 }
 // At round entry, let the initial side settle before starting a resource line
 // which that 50 ms side announcement would otherwise interrupt and consume.
 if(side_announcements_&&relation&&!side_)return result;
 const int player=player_;
 if(f.storm_active&&base.storm!=f.storm_active){
  const bool active=*f.storm_active;
  result.speech=propose(active?L"storm-on":L"storm-off",active?tr(L"Infinity Storm active."):tr(L"Infinity Storm ended."),Priority::Storm,s.observed_ms,[this,player,active]{auto& b=resources_[player];b.storm=active;b.ready=false;b.full=false;});return result;
 }
 if(last_combat_speech_&&now>=last_combat_speech_&&now-last_combat_speech_<800){pending_.reset();return result;}
 if(stocks&&(!base.stocks||*stocks>*base.stocks)){
  const int value=*stocks, previous=base.stocks.value_or(0);
  std::wstring text=value==4?tr(L"Hyper Combo full. 4 bars."):
   value==1?tr(L"Hyper Combo 1 bar."):tr(L"Hyper Combo {0} bars.",{std::to_wstring(value)});
  if(value>=3&&previous<3)text+=L" "+meter_ready(3);
  else if(previous==0)text+=L" "+meter_ready(1);
  result.speech=propose(L"hyper-"+std::to_wstring(value),text,Priority::Gauge,s.observed_ms,[this,player,value]{resources_[player].stocks=value;});
  return result;
 }
 if(f.storm_active&&!*f.storm_active&&range(f.infinity_fraction,0,1)){
  const bool full=*f.infinity_fraction>=1&&base.full!=std::optional<bool>(true);
  const bool ready=f.infinity_ready==std::optional<bool>(true)&&base.ready!=std::optional<bool>(true);
  if(full||ready){
   std::wstring text=ready?tr(L"Infinity Storm ready."):L"";
   if(full){if(ready)text+=L" ";text+=tr(L"Infinity gauge full.");}
   result.speech=propose(L"infinity",text,Priority::Gauge,s.observed_ms,[this,player,full,ready]{
    auto& accepted=resources_[player];if(full)accepted.full=true;if(ready)accepted.ready=true;
   });
   return result;
  }
 }
 // Acknowledging one fighter leaves the other pending. Alternate preference
 // when both are due so repeated damage on one side cannot crowd out the other.
 for(const int team:{next_health_team_,1-next_health_team_}){
  const auto& fighter=c.fighters[team];
  const auto visible=health_base(fighter);
  const auto& previous=health_[team];
  if(!visible||(previous&&previous->fighter==visible->fighter&&previous->band==visible->band))continue;
  const auto source=health_source(s,team);
  result.speech=propose(L"health",health_text(fighter,team==player),Priority::Gauge,s.observed_ms,[this,team,visible]{
   health_[team]=visible;next_health_team_=1-team;
  },source);
  return result;
 }
 pending_.reset();return result;
}
}
