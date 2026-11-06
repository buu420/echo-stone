#pragma once
#include "mvci/unreal.hpp"

namespace mvci {
struct MissionBattleSnapshot {
 std::uint64_t hud_identity=0, command_view=0;
 MenuSnapshot menu;
 StoryTextSnapshot objective, commands;
};
// Missions uses GF_Mission.MissionMain and per-page cursor fields, rather
// than the generic WD_MenuBase Items contract. See research/missions/report.md.
class MissionCapture {
 struct Cache {std::vector<ObjectRef> objects;std::uint64_t at=0;int count=-1;bool filled=false;};
 Unreal& unreal_;
 std::function<std::optional<int>(Address)> visibility_;
 Cache flows_;
 std::wstring rejection_;
public:
 explicit MissionCapture(Unreal& u,std::function<std::optional<int>(Address)> visibility={}):unreal_(u),visibility_(std::move(visibility)){}
 MenuSnapshot sample(std::uint64_t now);
 MissionBattleSnapshot battle(bool read_hud=true,bool read_commands=true)const;
 const std::wstring& rejection()const{return rejection_;}
};
}
