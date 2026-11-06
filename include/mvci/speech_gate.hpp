#pragma once
#include "mvci/model.hpp"
#include <algorithm>
namespace mvci {
// The reference mod uses backend speaking state where supported and text pacing
// otherwise. Only successful submissions set a hold. New manual/menu selections
// remain immediate, and blocked combat proposals are sampled again, never queued.
class SpeechGate {
 std::uint64_t submitted_=0,until_=0;
 Priority previous_=Priority::Gauge;
 bool active_=false;
 std::wstring menu_key_;
public:
 const std::wstring& source()const{return menu_key_;}
 bool allowed(Priority priority,std::uint64_t now,std::optional<bool> speaking)const{
  if(priority>=Priority::Menu||!active_||priority>previous_)return true;
  if(priority==Priority::Side&&previous_==Priority::Side)return true;
  if(speaking)return !*speaking&&now>=submitted_&&now-submitted_>=150;
  return now>=until_;
 }
 bool retired_menu(const std::wstring& current,bool fresh){
  if(menu_key_.empty()||(fresh&&menu_key_==current))return false;
  reset();return true;
 }
 void accepted(Priority priority,std::size_t characters,std::uint64_t now,std::wstring menu_key={}){
  menu_key_=std::move(menu_key);
  active_=true;previous_=priority;submitted_=now;
  const auto duration=1400+std::min<std::uint64_t>(characters,200)*70;
  until_=now+duration;
 }
 void reset(){active_=false;menu_key_.clear();}
};
}
