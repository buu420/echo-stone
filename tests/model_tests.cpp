#include "mvci/model.hpp"
#include "mvci/speech_gate.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>
using namespace mvci;
static int checks=0;
#define CHECK(x) do{++checks;if(!(x))throw std::runtime_error(std::string(__func__)+":"+std::to_string(__LINE__)+" " #x);}while(false)
static Snapshot sample(std::uint64_t now=1000){Snapshot s;s.foreground=true;s.observed_ms=now;return s;}
static Snapshot fight(std::uint64_t now=1000){auto s=sample(now);s.combat.valid=true;s.combat.playable=true;s.combat.session=7;s.combat.normalized_distance=0.5;s.combat.fighters[0].x=-2;s.combat.fighters[1].x=2;return s;}
static void accept(Model& m,const Result& r,std::uint64_t now){CHECK(r.speech);CHECK(m.acknowledge(r.speech->token,true,now));}
static bool says(const Result& r,const wchar_t* text){return r.speech&&r.speech->text.find(text)!=std::wstring::npos;}

// Removing acknowledgement or using last-sampled focus would swallow failed output.
static void menus_retry_current_selection(){
 Model m;auto s=sample();s.menu={true,false,true,12,L"Main menu",L"Story",L"",L"Start story",1,6};
 auto r=m.update(s,1000);CHECK(says(r,L"Story"));CHECK(says(r,L"1 of 6"));CHECK(r.speech->priority==Priority::Menu);
 auto old=r.speech->token;CHECK(!m.acknowledge(old,false,1000));
 s.menu.label=L"Training";s.menu.position=2;s.observed_ms=1010;
 r=m.update(s,1010);CHECK(says(r,L"Training"));CHECK(!says(r,L"Story"));CHECK(!m.acknowledge(old,true,1010));accept(m,r,1010);
 CHECK(!m.update(s,1020).speech);
 s.menu.value=L"Hard";r=m.update(s,1020);CHECK(says(r,L"Hard"));accept(m,r,1020);
 s.menu.enabled=false;r=m.update(s,1030);CHECK(says(r,L"Unavailable"));accept(m,r,1030);
 r=m.update(s,1040,Request::ReadMenu);CHECK(says(r,L"Training"));CHECK(r.speech->priority==Priority::Manual);
}
static void vanished_modal_cannot_commit(){
 Model m;auto s=sample();s.menu={true,true,true,9,L"Quit",L"No",L"",L"",2,2};auto r=m.update(s,1000);CHECK(says(r,L"No"));
 const auto token=r.speech->token;s.menu.valid=false;s.observed_ms=1010;CHECK(!m.update(s,1010).speech);CHECK(!m.acknowledge(token,true,1010));
 s.menu.valid=true;s.menu.label=L"Yes";r=m.update(s,1020);CHECK(says(r,L"Yes"));
 CHECK(!m.acknowledge(r.speech->token,true,1500));
}
static void beacon_cancels_on_every_gate(){
 Model m;auto s=fight();auto r=m.update(s,1000);CHECK(m.beacon_enabled());CHECK(r.beacon_interval_ms==656u);
 // FighterZ combat.lua near=200000, combat_model.lua far=1000000. Compare
 // equivalent fractions of each game's distance scale, never copied engine units.
 const std::vector<std::pair<double,std::uint32_t>> cadence={{0,150},{.1,150},{.195,150},{.2,150},{.4,488},{.5,656},{.6,825},{1,1500},{2,1500},{-1,150}};
 for(const auto& c:cadence){s.combat.normalized_distance=c.first;CHECK(m.update(s,1000).beacon_interval_ms==c.second);}
 s.combat.normalized_distance=std::numeric_limits<double>::quiet_NaN();CHECK(!m.update(s,1000).beacon_interval_ms);s.combat.normalized_distance=0.5;
 s.foreground=false;CHECK(!m.update(s,1000).beacon_interval_ms);s.foreground=true;
 s.menu.valid=true;CHECK(!m.update(s,1000).beacon_interval_ms);s.menu.valid=false;
 s.menu.blocked=true;CHECK(!m.update(s,1000).beacon_interval_ms);s.menu.blocked=false;
 s.combat.playable=false;CHECK(!m.update(s,1000).beacon_interval_ms);s.combat.playable=true;
 s.combat.cinematic=true;CHECK(!m.update(s,1000).beacon_interval_ms);s.combat.cinematic=false;
 CHECK(!m.update(s,1251).beacon_interval_ms);s.observed_ms=1300;CHECK(!m.update(s,1250).beacon_interval_ms);
}
static void side_tracks_player_and_requires_stability(){
 Model m;auto s=fight();CHECK(!m.update(s,1000).speech);s.observed_ms=1049;CHECK(!m.update(s,1049).speech);s.observed_ms=1050;
 auto r=m.update(s,1050);CHECK(says(r,L"Left side"));accept(m,r,1050);
 s.combat.fighters[0].x=3;s.observed_ms=1100;CHECK(!m.update(s,1100).speech);s.observed_ms=1150;r=m.update(s,1150);CHECK(says(r,L"Right side"));accept(m,r,1150);
 s.combat.fighters[0].x=2.0001;s.observed_ms=1200;CHECK(!m.update(s,1200).speech);
 r=m.update(s,1200,Request::SelectPlayer);CHECK(m.player()==1);CHECK(says(r,L"Player 2"));accept(m,r,1200);
 s.combat.fighters[0].x=3;s.observed_ms=1250;m.update(s,1250);s.observed_ms=1300;r=m.update(s,1300);CHECK(says(r,L"Left side"));
 const auto token=r.speech->token;s.foreground=false;s.observed_ms=1310;m.update(s,1310);CHECK(!m.acknowledge(token,true,1310));
}
static void cues_start_enabled_and_toggle_independently(){
 Model m;auto s=fight();CHECK(m.beacon_enabled());CHECK(m.side_announcements_enabled());
 auto r=m.update(s,1000,Request::ToggleBeacon);CHECK(!m.beacon_enabled());CHECK(m.side_announcements_enabled());CHECK(!r.beacon_interval_ms);CHECK(says(r,L"beacon off"));accept(m,r,1000);
 s.combat.fighters[0].hyper_bars=2;s.observed_ms=1010;r=m.update(s,1010,Request::ToggleSide);CHECK(!m.side_announcements_enabled());CHECK(!m.beacon_enabled());CHECK(says(r,L"Side announcements off"));accept(m,r,1010);
 s.observed_ms=1020;r=m.update(s,1020);CHECK(says(r,L"Hyper Combo 2 bars"));accept(m,r,1020); // No side-settle starvation while automatic side speech is off.
 s.combat.fighters[0].x=3;s.observed_ms=2010;r=m.update(s,2010);CHECK(!says(r,L"side"));CHECK(!r.beacon_interval_ms);
 r=m.update(s,2010,Request::ToggleBeacon);CHECK(m.beacon_enabled());CHECK(!m.side_announcements_enabled());CHECK(r.beacon_interval_ms);accept(m,r,2010);
 r=m.update(s,2020,Request::ReadCombat);CHECK(says(r,L"Right side"));accept(m,r,2020); // Explicit status still includes side.
 s.observed_ms=2030;r=m.update(s,2030,Request::ToggleSide);CHECK(m.side_announcements_enabled());CHECK(says(r,L"Side announcements on"));accept(m,r,2030);
 s.observed_ms=2040;CHECK(!says(m.update(s,2040),L"Right side"));s.observed_ms=2090;r=m.update(s,2090);CHECK(says(r,L"Right side"));
}
static void side_toggle_retries_do_not_toggle_twice_or_commit_old_side(){
 Model m;auto s=fight();m.update(s,1000);s.observed_ms=1050;auto side=m.update(s,1050);CHECK(says(side,L"Left side"));
 auto off=m.update(s,1060,Request::ToggleSide);CHECK(!m.side_announcements_enabled());CHECK(says(off,L"Side announcements off"));CHECK(!m.acknowledge(side.speech->token,true,1060));CHECK(!m.acknowledge(off.speech->token,false,1060));
 s.observed_ms=1100;auto retry=m.update(s,1100);CHECK(says(retry,L"Side announcements off"));CHECK(!m.side_announcements_enabled());accept(m,retry,1100);
 s.foreground=false;m.update(s,1110,Request::ToggleSide);CHECK(!m.side_announcements_enabled());
 s.foreground=true;s.observed_ms=100;auto on=m.update(s,2000,Request::ToggleSide);CHECK(m.side_announcements_enabled());CHECK(says(on,L"Side announcements on"));CHECK(!m.acknowledge(on.speech->token,false,2000));
 retry=m.update(s,2020);CHECK(m.side_announcements_enabled());CHECK(says(retry,L"Side announcements on"));accept(m,retry,2020);
}
static void hyper_spending_rearms_only_spent_stocks(){
 Model m;auto s=fight();s.combat.fighters[0].x.reset();s.combat.fighters[0].hyper_bars=0;CHECK(!m.update(s,1000).speech);
 s.observed_ms=2000;s.combat.fighters[0].hyper_bars=2.2;auto r=m.update(s,2000);CHECK(says(r,L"Hyper Combo 2 bars"));accept(m,r,2000);
 s.observed_ms=3000;s.combat.fighters[0].hyper_bars=1.2;CHECK(!m.update(s,3000).speech);
 s.observed_ms=4000;s.combat.fighters[0].hyper_bars=2;r=m.update(s,4000);CHECK(says(r,L"Hyper Combo 2 bars"));accept(m,r,4000);
 s.observed_ms=5000;s.combat.fighters[0].hyper_bars=4;r=m.update(s,5000);CHECK(says(r,L"Hyper Combo full"));accept(m,r,5000);
 s.observed_ms=6000;s.combat.fighters[0].hyper_bars=4.5;CHECK(!m.update(s,6000).speech);
}
static void infinity_charge_is_distinct_from_storm_time(){
 Model m;auto s=fight();s.combat.fighters[0].x.reset();auto& f=s.combat.fighters[0];f.infinity_fraction=0.2;f.infinity_ready=false;f.storm_active=false;CHECK(!m.update(s,1000).speech);
 s.observed_ms=2000;f.infinity_fraction=0.5;f.infinity_ready=true;auto r=m.update(s,2000);CHECK(says(r,L"Infinity Storm ready"));accept(m,r,2000);
 s.observed_ms=3000;f.infinity_fraction=1;r=m.update(s,3000);CHECK(says(r,L"Infinity gauge full"));accept(m,r,3000);
 s.observed_ms=4000;f.storm_active=true;r=m.update(s,4000);CHECK(says(r,L"Infinity Storm active"));accept(m,r,4000);
 s.observed_ms=5000;f.infinity_fraction=0.5;CHECK(!m.update(s,5000).speech);
 r=m.update(s,5000,Request::ReadCombat);CHECK(says(r,L"remaining"));CHECK(!says(r,L"ready"));accept(m,r,5000);
 s.observed_ms=6000;f.storm_active=false;f.infinity_ready=false;f.infinity_fraction=0;r=m.update(s,6000);CHECK(says(r,L"Infinity Storm ended"));accept(m,r,6000);
 s.observed_ms=7000;f.infinity_fraction=0.5;f.infinity_ready=true;r=m.update(s,7000);CHECK(says(r,L"Infinity Storm ready"));
}
static void both_health_bars_follow_ten_point_changes_and_retry_current_values(){
 Model model;auto s=fight();auto& us=s.combat.fighters[0];auto& them=s.combat.fighters[1];
 us.x.reset();us.health_fighter=1;us.health_percent=100;them.health_fighter=2;them.health_percent=96;
 auto result=model.update(s,1000);
 CHECK(says(result,L"Your health 100 percent"));CHECK(!says(result,L"Opponent health"));accept(model,result,1000);
 s.observed_ms=1800;result=model.update(s,1800);CHECK(says(result,L"Opponent health 96 percent"));accept(model,result,1800);
 s.observed_ms=2600;us.health_percent=91;them.health_percent=93;CHECK(!model.update(s,2600).speech);
 s.observed_ms=2610;us.health_percent=89;them.health_percent=89;result=model.update(s,2610);
 CHECK(says(result,L"Your health 89 percent"));
 const auto old=result.speech->token;CHECK(!model.acknowledge(old,false,2610));
 s.observed_ms=2630;us.health_percent=68;them.health_percent=77;result=model.update(s,2630);
 CHECK(says(result,L"Your health 68 percent"));CHECK(!model.acknowledge(old,true,2630));accept(model,result,2630);
 s.observed_ms=3430;us.health_percent=55;result=model.update(s,3430);
 CHECK(says(result,L"Opponent health 77 percent"));accept(model,result,3430);
 s.observed_ms=4230;us.health_percent=65;them.health_percent=74;CHECK(!model.update(s,4230).speech);
 s.observed_ms=4250;us.health_percent=0;result=model.update(s,4250);
 CHECK(says(result,L"Your health 0 percent"));CHECK(!says(result,L"Opponent health"));accept(model,result,4250);
 ++s.combat.session;s.observed_ms=4283;us.health_percent=100;them.health_percent=100;
 result=model.update(s,4283);CHECK(says(result,L"Your health 100 percent"));accept(model,result,4283);
 s.observed_ms=5083;result=model.update(s,5083);CHECK(says(result,L"Opponent health 100 percent"));
}
static void tags_refresh_health_while_team_gauges_stay_set(){
 Model model;auto s=fight();auto& us=s.combat.fighters[0];auto& them=s.combat.fighters[1];
 us.x.reset();us.health_fighter=1;us.health_percent=82;them.health_fighter=2;them.health_percent=70;
 accept(model,model.update(s,1000,Request::ReadCombat),1000);
 s.observed_ms=1800;us.hyper_bars=1.2;auto result=model.update(s,1800);CHECK(says(result,L"Level 1 meter ready"));accept(model,result,1800);
 s.observed_ms=2600;us.health_fighter=3;us.health_percent=85;result=model.update(s,2600);
 CHECK(says(result,L"Your health 85 percent"));CHECK(!says(result,L"Hyper Combo"));accept(model,result,2600);
 s.observed_ms=3400;them.health_fighter=4;result=model.update(s,3400);CHECK(says(result,L"Opponent health 70 percent"));accept(model,result,3400);
 s.observed_ms=4200;us.health_percent.reset();CHECK(!model.update(s,4200).speech);
 result=model.update(s,4200,Request::ReadCombat);CHECK(says(result,L"Your health unavailable"));accept(model,result,4200);
 s.observed_ms=4233;us.health_percent=85;CHECK(!model.update(s,4233).speech);
 s.menu.blocked=true;s.combat.playable=false;s.observed_ms=4266;them.health_percent=52;CHECK(!model.update(s,4266).speech);
 result=model.update(s,4266,Request::ReadCombat);CHECK(says(result,L"Your health 85 percent"));CHECK(says(result,L"Opponent health 52 percent"));
 CHECK(says(result,L"Hyper Combo 1.2 of 4 bars"));accept(model,result,4266);
 s.menu.blocked=false;s.combat.playable=true;s.observed_ms=4300;CHECK(!model.update(s,4300).speech);
 s.observed_ms=4400;accept(model,model.update(s,4400,Request::SelectPlayer),4400);
 result=model.update(s,4400,Request::ReadCombat);CHECK(says(result,L"Your health 52 percent"));CHECK(says(result,L"Opponent health 85 percent"));
}
static void health_speech_follows_the_visible_fighter_until_the_tag(){
 Model model;SpeechGate gate;auto s=fight();auto& fighter=s.combat.fighters[0];
 fighter.x.reset();fighter.health_fighter=1;fighter.health_percent=88;
 const auto first=model.update(s,1000);CHECK(says(first,L"Your health 88 percent"));CHECK(!first.speech->menu_key.empty());
 accept(model,first,1000);gate.accepted(first.speech->priority,first.speech->text.size(),1000,first.speech->menu_key);
 fighter.health_percent=56;CHECK(!gate.retired_menu(speech_source_fingerprint(s,gate.source()),true));
 fighter.health_fighter=3;CHECK(gate.retired_menu(speech_source_fingerprint(s,gate.source()),true));
 s.observed_ms=1800;const auto incoming=model.update(s,1800);CHECK(says(incoming,L"Your health 56 percent"));
 const auto key=incoming.speech->menu_key;
 CHECK(speech_source_fingerprint(s,key)==key);
 s.menu.blocked=true;CHECK(speech_source_fingerprint(s,key).empty());
 s.menu.blocked=false;fighter.health_percent.reset();CHECK(speech_source_fingerprint(s,key).empty());
}
static void readiness_names_the_usable_meter_levels_and_full_storm(){
 Model model;auto s=fight();auto& f=s.combat.fighters[0];f.x.reset();f.hyper_bars=0.99;
 CHECK(!model.update(s,1000).speech);
 f.hyper_bars=0.9999;auto almost=model.update(s,1000,Request::ReadCombat);
 CHECK(says(almost,L"Hyper Combo 0.99 of 4 bars"));CHECK(!says(almost,L"meter ready"));accept(model,almost,1000);
 f.hyper_bars=1;s.observed_ms=1800;auto result=model.update(s,1800);
 CHECK(says(result,L"Hyper Combo 1 bar"));CHECK(says(result,L"Level 1 meter ready"));accept(model,result,1800);
 f.hyper_bars=3;s.observed_ms=2600;result=model.update(s,2600);
 CHECK(says(result,L"Hyper Combo 3 bars"));CHECK(says(result,L"Level 3 meter ready"));accept(model,result,2600);
 f.hyper_bars=0.6;s.observed_ms=3400;CHECK(!model.update(s,3400).speech);
 f.hyper_bars=1;s.observed_ms=3433;result=model.update(s,3433);CHECK(says(result,L"Level 1 meter ready"));accept(model,result,3433);
 f.hyper_bars=3;f.storm_active=false;f.infinity_ready=true;f.infinity_fraction=1;s.observed_ms=4300;result=model.update(s,4300);
 CHECK(says(result,L"Level 3 meter ready"));CHECK(!says(result,L"Infinity Storm ready"));accept(model,result,4300);
 s.observed_ms=5100;result=model.update(s,5100);
 CHECK(says(result,L"Infinity gauge full"));CHECK(says(result,L"Infinity Storm ready"));accept(model,result,5100);
 result=model.update(s,5100,Request::ReadCombat);CHECK(says(result,L"Infinity Storm ready"));CHECK(says(result,L"Level 3 meter ready"));
}
static void explicit_requests_win_and_unknowns_are_reported(){
 Model m;auto s=fight();s.menu={true,false,true,12,L"Pause",L"Resume",L"",L"",1,3};s.combat.fighters[0].hyper_bars=2.5;
 auto r=m.update(s,1000,Request::ReadCombat);CHECK(r.speech);CHECK(r.speech->priority==Priority::Manual);CHECK(says(r,L"2.5"));CHECK(says(r,L"Infinity unavailable"));accept(m,r,1000);
 r=m.update(s,1001);CHECK(r.speech);CHECK(r.speech->priority==Priority::Menu);accept(m,r,1001);
 s.menu.valid=false;s.combat.valid=false;s.observed_ms=1020;r=m.update(s,1020,Request::ReadCombat);CHECK(says(r,L"unavailable"));
 const auto old=r.speech->token;r=m.update(s,1030,Request::SelectPlayer);CHECK(!m.acknowledge(old,true,1030));CHECK(says(r,L"Player 2"));
 s.foreground=false;r=m.update(s,1030,Request::SelectPlayer);CHECK(!r.speech);CHECK(m.player()==1);
}
static void expired_capture_still_answers_an_explicit_request(){
 Model m;auto s=sample(100);auto r=m.update(s,1000,Request::ReadMenu);CHECK(says(r,L"unavailable"));CHECK(r.speech->priority==Priority::Manual);accept(m,r,1000);
 r=m.update(s,1010,Request::ReadCombat);CHECK(says(r,L"unavailable"));r=m.update(s,1020,Request::SelectPlayer);CHECK(m.player()==1);CHECK(says(r,L"Player 2"));r=m.update(s,1030,Request::ToggleBeacon);CHECK(!m.beacon_enabled());CHECK(!r.beacon_interval_ms);
}
static void cinematic_pose_suppression_preserves_visible_gauges(){
 Model m;auto s=fight();s.combat.cinematic=true;s.combat.fighters[0].hyper_bars=0;CHECK(!m.update(s,1000).speech);
 s.observed_ms=2000;s.combat.fighters[0].hyper_bars=2;auto r=m.update(s,2000);CHECK(says(r,L"Hyper Combo 2 bars"));CHECK(!says(r,L"side"));accept(m,r,2000);
 r=m.update(s,2000,Request::ReadCombat);CHECK(says(r,L"Hyper Combo 2"));CHECK(!says(r,L"Left side"));
 r=m.update(s,2000,Request::ToggleBeacon);CHECK(!r.beacon_interval_ms);
}
static void failed_manual_output_retries_current_state_without_retoggling(){
 Model m;auto s=fight();auto r=m.update(s,1000,Request::SelectPlayer);CHECK(says(r,L"Player 2"));CHECK(!m.acknowledge(r.speech->token,false,1000));s.observed_ms=1040;r=m.update(s,1040);CHECK(says(r,L"Player 2"));CHECK(m.player()==1);accept(m,r,1040);
 s.menu.valid=true;s.menu.identity=15;s.menu.label=L"Save warning";s.observed_ms=1100;r=m.update(s,1100,Request::ReadMenu);CHECK(says(r,L"Save warning"));CHECK(!m.acknowledge(r.speech->token,false,1100));s.menu.label=L"Training";s.observed_ms=1140;r=m.update(s,1140);CHECK(says(r,L"Training"));CHECK(!says(r,L"Save warning"));CHECK(r.speech->priority==Priority::Manual);accept(m,r,1140);
}
static void opening_side_is_not_allowed_to_cut_off_initial_resources(){
 Model m;auto s=fight();s.combat.fighters[0].hyper_bars=1;CHECK(!m.update(s,1000).speech);s.observed_ms=1050;auto r=m.update(s,1050);CHECK(says(r,L"Left side"));accept(m,r,1050);s.observed_ms=1850;r=m.update(s,1850);CHECK(says(r,L"Hyper Combo 1 bar"));
}
static void failed_explicit_unavailable_reply_survives_a_stopped_capture(){
 Model m;auto s=sample(100);auto r=m.update(s,1000,Request::ReadCombat);CHECK(says(r,L"Combat status unavailable"));CHECK(!m.acknowledge(r.speech->token,false,1000));r=m.update(s,1020);CHECK(says(r,L"Combat status unavailable"));accept(m,r,1020);
 r=m.update(s,1040,Request::SelectPlayer);CHECK(m.player()==1);CHECK(!m.acknowledge(r.speech->token,false,1040));r=m.update(s,1060);CHECK(says(r,L"Player 2"));CHECK(m.player()==1);accept(m,r,1060);
}
static void story_captions_retry_current_text_without_suppressing_combat(){
 Model m;auto s=fight();s.story.valid=true;s.story.context=44;s.story.captions[0]={true,90,L"Xgard"};s.story.captions[1]={true,91,L"88 days since the Convergence"};s.story.subtitle={true,92,L"Spoken dialogue"};
 auto r=m.update(s,1000);CHECK(says(r,L"Xgard"));CHECK(!says(r,L"88 days"));CHECK(!says(r,L"Spoken dialogue"));CHECK(r.beacon_interval_ms);CHECK(!m.acknowledge(r.speech->token,false,1000));r=m.update(s,1010);CHECK(says(r,L"Xgard"));accept(m,r,1010);
 s.observed_ms=1030;r=m.update(s,1030);CHECK(!says(r,L"Xgard"));CHECK(says(r,L"88 days"));accept(m,r,1030);s.story.captions[0]={};s.observed_ms=1040;m.update(s,1040);s.story.captions[0]={true,90,L"Xgard"};s.observed_ms=1050;r=m.update(s,1050);CHECK(says(r,L"Xgard"));CHECK(!says(r,L"88 days"));
 s.menu={true,true,true,12,L"",L"Pause"};s.observed_ms=1060;r=m.update(s,1060);CHECK(says(r,L"Pause"));CHECK(!says(r,L"Xgard"));CHECK(!r.beacon_interval_ms);
}
static void story_repeat_reads_current_subtitle_and_native_menu_wins(){
 Model m;auto s=sample();s.story.valid=true;s.story.subtitle={true,12,L"The currently displayed line"};auto r=m.update(s,1000);CHECK(!r.speech);r=m.update(s,1000,Request::ReadMenu);CHECK(says(r,L"currently displayed"));accept(m,r,1000);
 s.story.subtitle={};s.observed_ms=1030;r=m.update(s,1030,Request::ReadMenu);CHECK(says(r,L"unavailable"));accept(m,r,1030);s.story.subtitle={true,12,L"Retained dialogue"};s.menu={true,true,true,15,L"",L"Save warning"};s.observed_ms=1060;r=m.update(s,1060,Request::ReadMenu);CHECK(says(r,L"Save warning"));CHECK(!says(r,L"Retained dialogue"));
}
static void descriptions_default_on_and_retry_without_toggling_twice(){
 Model m;auto s=sample();CHECK(m.descriptions_enabled());auto r=m.update(s,1000,Request::ToggleDescriptions);CHECK(!m.descriptions_enabled());CHECK(says(r,L"descriptions off"));CHECK(!m.acknowledge(r.speech->token,false,1000));s.observed_ms=1033;r=m.update(s,1033);CHECK(!m.descriptions_enabled());CHECK(says(r,L"descriptions off"));accept(m,r,1033);r=m.update(s,1066,Request::ToggleDescriptions);CHECK(m.descriptions_enabled());CHECK(says(r,L"descriptions on"));
}
static void automatic_story_caption_delivery_is_independent(){
 Model m;SpeechGate gate;auto s=sample();s.story.valid=true;s.story.captions[0]={true,20,L"A location caption that is still being spoken"};
 auto r=m.update(s,1000);CHECK(r.speech);gate.accepted(r.speech->priority,r.speech->text.size(),1000,r.speech->menu_key);accept(m,r,1000);
 s.observed_ms=1033;s.story.captions[1]={true,21,L"A new date caption"};s.story.captions[5]={true,22,L"A new description notice"};CHECK(!gate.retired_menu(speech_source_fingerprint(s,gate.source()),true));
 r=m.update(s,1033);CHECK(says(r,L"new date"));CHECK(!says(r,L"location"));CHECK(!says(r,L"description notice"));CHECK(!gate.allowed(r.speech->priority,1033,true));CHECK(!m.acknowledge(r.speech->token,false,1033));
 s.story.captions[5]={};s.observed_ms=1200;CHECK(!gate.retired_menu(speech_source_fingerprint(s,gate.source()),true));r=m.update(s,1200);CHECK(says(r,L"new date"));CHECK(gate.allowed(r.speech->priority,1200,false));gate.accepted(r.speech->priority,r.speech->text.size(),1200,r.speech->menu_key);accept(m,r,1200);
 s.story.captions[0]={};s.observed_ms=1233;CHECK(!gate.retired_menu(speech_source_fingerprint(s,gate.source()),true));s.story.captions[1]={};CHECK(gate.retired_menu(speech_source_fingerprint(s,gate.source()),true));
}
static void simultaneous_story_captions_are_acknowledged_one_at_a_time(){
 Model m;auto s=sample();s.story.valid=true;s.story.captions[0]={true,20,L"Location"};s.story.captions[1]={true,21,L"Date"};auto r=m.update(s,1000);CHECK(says(r,L"Location"));CHECK(!says(r,L"Date"));accept(m,r,1000);
 s.observed_ms=1033;r=m.update(s,1033);CHECK(says(r,L"Date"));CHECK(!says(r,L"Location"));CHECK(!m.acknowledge(r.speech->token,false,1033));s.story.captions[1]={};s.observed_ms=1066;r=m.update(s,1066);CHECK(!says(r,L"Date"));
}
static void accepted_loading_survives_capture_gaps_but_rearms_after_a_native_boundary(){
 Model m;auto s=sample();s.story.valid=true;s.story.captions[3]={true,43,L"Now loading..."};auto r=m.update(s,1000);CHECK(says(r,L"Now loading"));accept(m,r,1000);
 CHECK(!says(m.update(s,2000),L"Now loading"));s.observed_ms=2033;CHECK(!says(m.update(s,2033),L"Now loading"));
 s.story={};s.observed_ms=2066;m.update(s,2066);s.story.valid=true;s.story.captions[3]={true,43,L"Now loading..."};s.observed_ms=2100;CHECK(!says(m.update(s,2100),L"Now loading"));
 r=m.update(s,2100,Request::ReadMenu);CHECK(says(r,L"Now loading"));accept(m,r,2100);
 s.story={};s.menu={true,false,true,44,L"",L"Story"};s.observed_ms=2133;r=m.update(s,2133);CHECK(says(r,L"Story"));accept(m,r,2133);
 s.menu={};s.story.valid=true;s.story.captions[3]={true,43,L"Now loading..."};s.observed_ms=2166;CHECK(says(m.update(s,2166),L"Now loading"));
}
static void unacknowledged_loading_retries_but_new_owner_does_not_repeat_an_accepted_episode(){
 Model m;auto s=sample();s.story.valid=true;s.story.captions[3]={true,43,L"Now loading..."};auto r=m.update(s,1000);CHECK(says(r,L"Now loading"));CHECK(!m.acknowledge(r.speech->token,false,1000));m.update(s,2000);s.observed_ms=2033;r=m.update(s,2033);CHECK(says(r,L"Now loading"));accept(m,r,2033);
 s.observed_ms=2066;s.story.captions[3].identity=44;CHECK(!says(m.update(s,2066),L"Now loading"));
}
static void native_loading_absence_does_not_rearm_before_a_new_presentation(){
 Model m;auto s=sample();s.story.valid=true;s.story.captions[3]={true,43,L"Now loading..."};s.story.loading={true,true,43};auto r=m.update(s,1000);accept(m,r,1000);
 for(int step=0;step<3;++step){s.observed_ms=1033+66*step;s.story={};s.story.loading={true,false,43};m.update(s,s.observed_ms);s.observed_ms+=33;s.story.valid=true;s.story.loading={true,true,43};s.story.captions[3]={true,43,L"Now loading..."};CHECK(!says(m.update(s,s.observed_ms),L"Now loading"));}
 s.observed_ms=1300;s.story={};s.menu={true,false,true,44,L"Title",L"PRESS ANY BUTTON"};r=m.update(s,1300);CHECK(says(r,L"PRESS ANY BUTTON"));accept(m,r,1300);
 s.menu={};s.observed_ms=1333;s.story.valid=true;s.story.captions[3]={true,43,L"Now loading..."};CHECK(says(m.update(s,1333),L"Now loading"));
}
static void accepted_speech_commits_at_fresh_submission_even_when_backend_returns_late(){
 Model m;auto s=sample();s.story.valid=true;s.story.captions[3]={true,43,L"Now loading..."};auto r=m.update(s,1000);CHECK(says(r,L"Now loading"));
 // The backend accepted speech submitted at1050; it returned at1400.
 CHECK(m.acknowledge(r.speech->token,true,1400,1050));
 s.observed_ms=1433;CHECK(!says(m.update(s,1433),L"Now loading"));
 Model stale;r=stale.update(sample(),1000,Request::ReadMenu);CHECK(!stale.acknowledge(r.speech->token,true,1400,1300));
 Model reversed;r=reversed.update(sample(),1000,Request::ReadMenu);CHECK(!reversed.acknowledge(r.speech->token,true,1020,1040));
}
static void speech_submission_gate_rejects_stale_and_wrong_proposals(){
 Model m;auto r=m.update(sample(),1000,Request::ToggleBeacon);CHECK(r.speech);const auto token=r.speech->token;
 CHECK(m.submission_fresh(token,1000));CHECK(m.submission_fresh(token,1250));CHECK(!m.submission_fresh(token,1251));CHECK(!m.submission_fresh(token,999));CHECK(!m.submission_fresh(token+1,1000));
 CHECK(!m.acknowledge(token,false,1100,1050));CHECK(!m.beacon_enabled());r=m.update(sample(),1120);CHECK(says(r,L"beacon off"));CHECK(!m.beacon_enabled());CHECK(m.submission_fresh(r.speech->token,1120));accept(m,r,1120);
}
static void prepared_movies_and_nonplayable_huds_do_not_rearm_loading(){
 for(int surface=0;surface<2;++surface){Model m;auto s=sample();s.story.valid=true;s.story.captions[3]={true,43,L"Now loading..."};auto r=m.update(s,1000);accept(m,r,1000);
  s.story={};s.observed_ms=1033;
  if(surface==0){s.story.movie.valid=true;s.story.movie.playing=true;s.story.movie.presented=false;}else{s.combat.valid=true;s.combat.playable=false;}
  m.update(s,1033);s.story={};s.combat={};s.story.valid=true;s.story.captions[3]={true,43,L"Now loading..."};s.observed_ms=1066;CHECK(!says(m.update(s,1066),L"Now loading"));
  s.story={};s.observed_ms=1100;
  if(surface==0){s.story.movie.valid=true;s.story.movie.playing=true;s.story.movie.presented=true;}else{s.combat.valid=true;s.combat.playable=true;}
  m.update(s,1100);s.story={};s.combat={};s.story.valid=true;s.story.captions[3]={true,43,L"Now loading..."};s.observed_ms=1133;CHECK(says(m.update(s,1133),L"Now loading"));
 }
}
static void unreadable_menu_records_do_not_rearm_loading(){
 Model m;auto s=sample();s.story.valid=true;s.story.captions[3]={true,43,L"Now loading..."};auto r=m.update(s,1000);accept(m,r,1000);
 s.story={};s.menu.valid=true;s.observed_ms=1033;m.update(s,1033);s.menu={};s.story.valid=true;s.story.captions[3]={true,43,L"Now loading..."};s.observed_ms=1066;CHECK(!says(m.update(s,1066),L"Now loading"));
}
static void a_known_loading_overlay_still_blocks_rearming_if_its_text_is_unreadable(){
 Model m;auto s=sample();s.story.valid=true;s.story.loading={true,true,43};s.story.captions[3]={true,43,L"Now loading..."};auto r=m.update(s,1000);accept(m,r,1000);
 s.story.captions[3]={};s.menu={true,false,true,44,L"Title",L"PRESS ANY BUTTON"};s.observed_ms=1033;m.update(s,1033);
 s.menu={};s.story.captions[3]={true,43,L"Now loading..."};s.observed_ms=1066;CHECK(!says(m.update(s,1066),L"Now loading"));
}
int main(){
 const std::vector<void(*)()> tests={menus_retry_current_selection,vanished_modal_cannot_commit,beacon_cancels_on_every_gate,side_tracks_player_and_requires_stability,cues_start_enabled_and_toggle_independently,side_toggle_retries_do_not_toggle_twice_or_commit_old_side,hyper_spending_rearms_only_spent_stocks,infinity_charge_is_distinct_from_storm_time,both_health_bars_follow_ten_point_changes_and_retry_current_values,tags_refresh_health_while_team_gauges_stay_set,health_speech_follows_the_visible_fighter_until_the_tag,readiness_names_the_usable_meter_levels_and_full_storm,explicit_requests_win_and_unknowns_are_reported,expired_capture_still_answers_an_explicit_request,cinematic_pose_suppression_preserves_visible_gauges,failed_manual_output_retries_current_state_without_retoggling,opening_side_is_not_allowed_to_cut_off_initial_resources,failed_explicit_unavailable_reply_survives_a_stopped_capture};int failed=0;
 for(auto test:{accepted_speech_commits_at_fresh_submission_even_when_backend_returns_late,speech_submission_gate_rejects_stale_and_wrong_proposals,a_known_loading_overlay_still_blocks_rearming_if_its_text_is_unreadable,native_loading_absence_does_not_rearm_before_a_new_presentation,prepared_movies_and_nonplayable_huds_do_not_rearm_loading,unreadable_menu_records_do_not_rearm_loading,accepted_loading_survives_capture_gaps_but_rearms_after_a_native_boundary,unacknowledged_loading_retries_but_new_owner_does_not_repeat_an_accepted_episode,automatic_story_caption_delivery_is_independent,simultaneous_story_captions_are_acknowledged_one_at_a_time,story_captions_retry_current_text_without_suppressing_combat,story_repeat_reads_current_subtitle_and_native_menu_wins,descriptions_default_on_and_retry_without_toggling_twice}){try{test();}catch(const std::exception& e){std::cerr<<e.what()<<'\n';++failed;}}
 for(auto test:tests){try{test();}catch(const std::exception& e){std::cerr<<e.what()<<'\n';++failed;}}
 std::cout<<checks<<" checks, "<<failed<<" failed groups\n";return failed?1:0;
}
