#pragma once
#include "mvci/unreal.hpp"
namespace mvci {
// Read-only capture of the battle HUD exactly as drawn (research/combat report.md, ui-report.md §9).
// Gauges come only from rendered Blueprint fields of an Active, visible part; positions come from the
// native HudInfo fighter records. No game function is called and nothing is written.
class CombatCapture {
 struct Point {double x=0,height=0;};
 struct FighterRecord {Address address=0;int slot=0,character=0;};
 Unreal& unreal_;
 std::optional<ObjectRef> root_;
 ObjectRef session_root_{};
 Address world_=0,battle_=0;
 std::uint64_t scanned_at_=0,session_=0;
 int last_flow_=-1;
 int scanned_count_=-1;
 double reference_=0;
 bool scanned_=false,identity_=false,round_=false,played_=false;
 template<class T> std::optional<T> get(Address a)const{return unreal_.memory().get<T>(a);}
 bool in_world(Address actor,Address world)const;
 bool shown_actor(Address actor,Address world)const;
 bool root_ok(const ObjectRef& root,Address world)const;
 std::optional<ObjectRef> find_root(Address world,std::uint64_t now);
 bool visible_component(Address owner,const wchar_t* name)const;
 std::optional<Address> child(Address owner,const wchar_t* component,const wchar_t* cls,Address world)const;
 std::optional<Address> part(Address owner,const wchar_t* component,const wchar_t* cls,int team,Address world)const;
 void hyper(Address root,int team,Address world,FighterSnapshot& out)const;
 void infinity(Address root,int team,Address world,FighterSnapshot& out)const;
 void health(Address root,Address hud,int team,Address world,const FighterRecord&,FighterSnapshot& out)const;
 std::optional<FighterRecord> active_fighter(Address hud,int team)const;
 std::optional<Point> point(const FighterRecord&)const;
public:
 explicit CombatCapture(Unreal& unreal):unreal_(unreal){}
 CombatSnapshot sample(std::uint64_t now,bool menu_visible);
};
}
