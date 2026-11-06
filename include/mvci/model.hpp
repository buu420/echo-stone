#pragma once
#include "mvci/localization.hpp"
#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>

namespace mvci {
struct MenuSnapshot {
 bool valid=false, modal=false, enabled=true;
 std::uint64_t identity=0;
 std::wstring screen, label, value, help;
 int position=0, count=0;
 bool blocked=false; // Displayed blocking UI exists, but its text/owner is not yet safely readable.
};
struct FighterSnapshot {
 std::optional<double> x, health_percent, hyper_bars, infinity_fraction;
 std::optional<bool> infinity_ready, storm_active;
 std::optional<int> health_fighter; // Roster slot and character of the drawn active bar.
};
struct CombatSnapshot {
 bool valid=false, playable=false, cinematic=false;
 std::uint64_t session=0;
 std::array<FighterSnapshot,2> fighters;
 std::optional<double> normalized_distance;
};
struct StoryTextSnapshot {bool valid=false;std::uint64_t identity=0;std::wstring text;};
struct LoadingSnapshot {bool known=false,visible=false;std::uint64_t identity=0;};
struct MovieSoundtrack {int origin_ms=0,elapsed_ms=0;};
struct MovieSnapshot {
 bool valid=false,playing=false,paused=false,presented=false;
 std::uint64_t context=0;
 std::wstring id;
 std::optional<double> sequence_seconds,movie_seconds;
 std::optional<int> frame;
 std::optional<MovieSoundtrack> soundtrack;
};
struct StorySnapshot {
 bool valid=false;
 std::uint64_t context=0;
 // Location, date, advice, loading, ending/result, and description notices have
 // independent speech identities. Subtitles and mission commands are repeated on explicit F8.
 std::array<StoryTextSnapshot,6> captions;
 StoryTextSnapshot subtitle;
 LoadingSnapshot loading;
 MovieSnapshot movie;
 std::optional<int> enemies_remaining,time_seconds;
 bool infinite_time=false;
};
struct Snapshot {
 bool foreground=false;
 std::uint64_t mission_hud=0, mission_command_view=0;
 std::uint64_t observed_ms=0;
 std::uint64_t hud_capture_ticket=0;
 MenuSnapshot menu;
 CombatSnapshot combat;
 StorySnapshot story;
 Language language=Language::English;
};
enum class Request { None, SelectPlayer, ToggleBeacon, ReadCombat, ReadMenu, ToggleSide, ToggleDescriptions };
enum class Priority { Gauge=50, Storm=60, Side=70, Story=80, Menu=90, Manual=100 };
struct Speech {std::uint64_t token=0; Priority priority=Priority::Gauge; std::wstring text,menu_key;Request request=Request::None;};
struct Result {std::optional<Speech> speech; std::optional<std::uint32_t> beacon_interval_ms;};
class Model {
 struct ResourceBase {std::optional<int> stocks; std::optional<bool> storm,ready,full;};
 struct HealthBase {int fighter,band;};
 struct Pending {Speech speech;std::wstring key;std::function<void()> commit;std::uint64_t sampled=0;};
 std::optional<Pending> pending_;
 Language language_=Language::English;
 Request pending_request_=Request::None;
 std::wstring accepted_menu_;
 std::array<std::wstring,6> accepted_story_;
 std::array<ResourceBase,2> resources_;
 std::array<std::optional<HealthBase>,2> health_;
 std::uint64_t session_=0,next_token_=1,last_combat_speech_=0;
 std::optional<bool> side_,candidate_;
 std::uint64_t candidate_since_=0;
 int player_=0,next_health_team_=0;
 bool beacon_=true;
 bool side_announcements_=true;
 bool descriptions_=true;
 static std::optional<HealthBase> health_base(const FighterSnapshot&);
 std::wstring control_confirmation(Request)const;
 Speech propose(std::wstring,std::wstring,Priority,std::uint64_t,std::function<void()>,std::wstring source_menu={});
public:
 Result update(const Snapshot&,std::uint64_t now,Request request=Request::None);
 bool submission_fresh(std::uint64_t token,std::uint64_t submitted_ms)const;
 bool acknowledge(std::uint64_t token,bool success,std::uint64_t now,std::optional<std::uint64_t> submitted_ms={});
 int player()const{return player_;}
 bool beacon_enabled()const{return beacon_;}
 bool side_announcements_enabled()const{return side_announcements_;}
 bool descriptions_enabled()const{return descriptions_;}
};
std::wstring menu_fingerprint(const MenuSnapshot&);
std::wstring speech_source_fingerprint(const Snapshot&,const std::wstring& expected);
}
