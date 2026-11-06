#include "mvci/descriptions.hpp"
#include <algorithm>
#include <cmath>
namespace mvci {
namespace {
bool finite(double value){return std::isfinite(value);}
bool valid_movie(const DescriptionMovie& m){
 if(m.id.empty()||!finite(m.duration)||m.duration<=0||!finite(m.sequence_offset)||m.sequence_offset<0||m.cues.empty()||m.cues.size()>512)return false;
 double end=0;std::uint64_t bytes=0;
 for(const auto& c:m.cues){if(c.id.empty()||c.text.empty()||!finite(c.start)||!finite(c.end)||!finite(c.duration)||c.start<end||c.end>m.duration||c.duration<=0||c.start+c.duration>c.end+.001||c.bytes>2097152)return false;end=c.end;bytes+=c.bytes;}
 return bytes<=33554432;
}
}
std::optional<double> description_movie_time(const MovieSnapshot& native,const DescriptionMovie& movie){
 if(!native.valid||!native.playing||!native.presented||!native.context||native.id!=movie.id||
    !native.frame||*native.frame<0||!native.movie_seconds)return {};

 // Mana writes this timestamp when it hands the decoded frame to the movie
 // texture. The active sequence establishes ownership and pause state, while
 // its cursor advances separately from the frame being displayed.
 const double time=*native.movie_seconds;
 if(!finite(time)||time<0||time>movie.duration+.1)return {};
 return time;
}
void DescriptionSession::silence(){output_.stop();active_=nullptr;paused_=false;}
void DescriptionSession::observe(const Snapshot& s,const DescriptionMovie* movie,std::uint64_t now,bool enabled,bool interrupted){
 const auto& native=s.story.movie;
 if(movie!=movie_||native.context!=owner_){silence();movie_=movie;owner_=native.context;next_=0;have_time_=false;advance_at_=observed_at_=now;}
 const bool fresh=s.foreground&&now>=s.observed_ms&&now-s.observed_ms<=250;
 if(!enabled||!fresh||!movie||!valid_movie(*movie)||((s.menu.valid||s.menu.blocked)&&!native.paused)){
  silence();have_time_=false;return;
 }
 const auto presented_time=description_movie_time(native,*movie);
 if(!presented_time){silence();have_time_=false;return;}
 const double time=*presented_time;
 const bool rewound=have_time_&&time<time_-.1;
 const auto& sound=native.soundtrack;
 // The game can retain a cached sound cursor across a reset. Only use it
 // after it advances in this movie with the same native sync origin.
 if(!have_time_||rewound||!soundtrack_||!sound||sound->origin_ms!=soundtrack_->origin_ms||
    sound->elapsed_ms<soundtrack_->elapsed_ms) soundtrack_current_=false;
 else if(sound->elapsed_ms>soundtrack_->elapsed_ms) soundtrack_current_=true;
 soundtrack_=sound;
 const double window_time=soundtrack_current_&&sound?std::max(time,sound->elapsed_ms/1000.):time;
 if(!have_time_){time_=time;observed_at_=advance_at_=now;have_time_=true;return;}
 const auto elapsed=now>=observed_at_?static_cast<double>(now-observed_at_)/1000.:0.;
 const bool advancing=time>time_+.0001;
 if(rewound){silence();next_=0;}
 else if(time-time_>std::max(.75,elapsed*1.5+.15))silence();
 if(advancing)advance_at_=now;
 time_=time;observed_at_=now;
 if(interrupted){silence();return;}
 if(active_&&(window_time>=active_->end||!output_.playing()))silence();
 const bool suspended=native.paused||now<advance_at_||now-advance_at_>=120;
 if(suspended){if(output_.playing()&&!paused_){output_.pause();paused_=true;}return;}
 if(paused_){output_.resume();paused_=false;}
 if(output_.playing()||!advancing)return;
 while(next_<movie->cues.size()){
  const auto& cue=movie->cues[next_];if(time<cue.start)return;
  // A bank load, seek or frame hitch cannot spill a stale line into dialogue.
  if(time+cue.duration>cue.end+.001){++next_;continue;}
  // A sound-only veto is reversible if the native audio clock is reset.
  if(window_time+cue.duration>cue.end+.001)return;
  if(output_.start(cue,window_time)){active_=&cue;++next_;}return;
 }
}
}
