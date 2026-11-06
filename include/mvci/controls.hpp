#pragma once
#include "mvci/model.hpp"
#include <array>
#include <deque>
namespace mvci {
class Controls {
 std::array<bool,6> held_{};std::deque<Request> queue_;
public:
 void observe(const std::array<bool,6>& keys,bool foreground){
  constexpr std::array<Request,6> requests={Request::SelectPlayer,Request::ToggleBeacon,Request::ReadCombat,Request::ReadMenu,Request::ToggleSide,Request::ToggleDescriptions};
  if(!foreground)queue_.clear();
  else for(std::size_t i=0;i<keys.size();++i)if(keys[i]&&!held_[i]&&queue_.size()<8)queue_.push_back(requests[i]);
  held_=keys;
 }
 Request next(){if(queue_.empty())return Request::None;auto r=queue_.front();queue_.pop_front();return r;}
};
}
