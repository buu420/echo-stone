#pragma once
#include "mvci/model.hpp"
#include <memory>
#include <optional>
#include <string>
#include <vector>
namespace mvci {
struct DescriptionCue {
 std::string id,file,sha256;
 std::wstring text;
 double start=0,end=0,duration=0;
 std::uint32_t bytes=0;
};
struct DescriptionMovie {
 std::wstring id;
 double duration=0,sequence_offset=0;
 std::vector<DescriptionCue> cues;
};
const std::vector<DescriptionMovie>& description_catalog();
std::optional<double> description_movie_time(const MovieSnapshot& native,const DescriptionMovie& movie);
class DescriptionOutput {
public:
 virtual ~DescriptionOutput()=default;
 virtual bool playing()const=0;
 virtual bool start(const DescriptionCue&,double window_time)=0;
 virtual void pause()=0;
 virtual void resume()=0;
 virtual void stop()=0;
};
// A copied native movie clock is the only source of presentation time.
class DescriptionSession {
 DescriptionOutput& output_;
 const DescriptionMovie* movie_=nullptr;
 std::uint64_t owner_=0,advance_at_=0,observed_at_=0;
 double time_=0;
 std::size_t next_=0;
 const DescriptionCue* active_=nullptr;
 std::optional<MovieSoundtrack> soundtrack_;
 bool have_time_=false,paused_=false,soundtrack_current_=false;
 void silence();
public:
 explicit DescriptionSession(DescriptionOutput& output):output_(output){}
 void observe(const Snapshot&,const DescriptionMovie*,std::uint64_t now,bool enabled,bool interrupted=false);
};
// Owned waveOut playback plus one asynchronously loaded/hash-checked movie bank.
// It never reads a native game object or invokes game input.
class DescriptionAudio {
 struct Impl;
 std::unique_ptr<Impl> impl_;
public:
 explicit DescriptionAudio(std::wstring data_directory,std::function<bool()> current={});
 ~DescriptionAudio();
 DescriptionAudio(const DescriptionAudio&)=delete;
 DescriptionAudio& operator=(const DescriptionAudio&)=delete;
 void prepare(const Snapshot&,std::uint64_t now,bool enabled);
 StoryTextSnapshot notice(const Snapshot&,bool enabled)const;
 void tick(const Snapshot&,std::uint64_t now,bool enabled,bool interrupted);
 std::string diagnostic()const;
};
}
