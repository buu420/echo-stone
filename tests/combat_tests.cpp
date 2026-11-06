#include "mvci/combat.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace mvci;
static int checks=0;
#define CHECK(x) do{++checks;if(!(x))throw std::runtime_error(std::string(__func__)+":"+std::to_string(__LINE__)+" " #x);}while(false)
// Independent hand-encoded layout of the static proof (research/combat report.md, ui-report.md §9):
// HudInfo base+0x3476150 (vt 0x28eec88, ptr 0x34760f0), BattleStatus ptr 0x34730a8 (vt 0x28ec0e0),
// CineCameraMode 0x33e219c, GEngine 0x36bc348 (+0x5d8 viewport, +0x80 world, +0xc80 game instance +0xe8
// DuckGamePause), World+0x30 level, Level+0xb0 OwningWorld, +0x2e0 WorldSettings, +0x500 Pauser,
// component owner +0xb0, ChildActorComponent.ChildActor +0x308. Blueprint offsets are fixture choices:
// the capture resolves them by reflected name.
struct Fixture {
 static constexpr Address base=0x140000000,heap=0x20000000,hud=base+0x3476150;
 std::map<Address,std::vector<unsigned char>> blocks;
 Address next=heap,entries=0,chunk=0;int objects=0,name_count=0;
 std::map<std::wstring,int> ids;std::map<std::wstring,Address> classes;
 Memory memory;Unreal unreal;
 Address world=0,level=0,engine=0,instance=0,settings=0,status=0,root=0,gem_ctrl=0;
 Address spacon[2]{},gem[2]{},spacon_state[2]{},gem_state[2]{},spacon_comp[2]{},gem_comp[2]{},bar[2]{},digit[2]{},gem_bar[2]{},gem_ctrl_comp=0;
 Address vital[2]{},vital_component[2]{},vital_root[2][2]{},vital_mesh[2][2]{},vital_values[2]{};
 Fixture():unreal(memory,base){
  memory.reader=[this](Address a,void* out,std::size_t n){auto i=blocks.upper_bound(a);if(i==blocks.begin())return false;--i;if(a<i->first||a-i->first>i->second.size()||n>i->second.size()-(a-i->first))return false;std::memcpy(out,i->second.data()+a-i->first,n);return true;};
  for(Address g:{Address(0x35cafd0),Address(0x34760f0),Address(0x34730a8),Address(0x33e219c),Address(0x36bc348)})blocks[base+g].resize(8);
  blocks[base+0x35d36f0].resize(0x20);blocks[hud].resize(0x700);
  auto names=alloc(0x408);chunk=alloc(16384*8);entries=alloc(400*24);
  put(base+0x35cafd0,names);put(names,chunk);put(base+0x35d36f0+0x10,entries);put<int>(base+0x35d36f0+0x18,400);
  build();
 }
 Address alloc(std::size_t n){Address a=next;next+=(n+0x1f)&~std::size_t(0x1f);blocks[a].resize(n);return a;}
 template<class T> T raw(Address a){T v{};if(!memory.reader(a,&v,sizeof(v)))throw std::runtime_error("fixture read");return v;}
 template<class T> void put(Address a,T v){auto i=blocks.upper_bound(a);if(i==blocks.begin())throw std::runtime_error("fixture range");--i;if(a-i->first+sizeof(T)>i->second.size())throw std::runtime_error("fixture bounds");std::memcpy(i->second.data()+a-i->first,&v,sizeof(v));}
 int id(const std::wstring& s){auto i=ids.find(s);if(i!=ids.end())return i->second;int n=name_count++;auto a=alloc(0x10+2*(s.size()+1));put<std::uint32_t>(a,1);for(std::size_t j=0;j<s.size();++j)put<std::uint16_t>(a+0x10+j*2,static_cast<std::uint16_t>(s[j]));put(chunk+static_cast<Address>(n)*8,a);ids[s]=n;return n;}
 Address object(const std::wstring& name,Address cls,std::size_t size=0x600,Address outer=0){auto a=alloc(size);int index=objects++;put(a,base+0x1000);put(entries+index*24,a);put<int>(entries+index*24+0x10,100+index);put<int>(a+0xc,index);put(a+0x10,cls);put<int>(a+0x18,id(name));put(a+0x20,outer);put<int>(base+0x35d36f0+0x1c,objects);return a;}
 Address meta(const std::wstring& name){if(classes.count(name))return classes[name];if(name==L"Class"){auto a=object(name,0);put(a+0x10,a);return classes[name]=a;}return classes[name]=object(name,meta(L"Class"));}
 Address klass(const std::wstring& name,Address parent=0){auto a=object(name,meta(L"Class"));put(a+0x30,parent);put<int>(a+0x40,0x600);return classes[name]=a;}
 Address prop(Address cls,const std::wstring& name,const std::wstring& kind,int offset,int size,std::uint8_t mask=1){auto p=object(name,meta(kind));put<int>(p+0x50,offset);put<int>(p+0x34,size);put(p+0x28,raw<Address>(cls+0x38));put(cls+0x38,p);if(kind==L"BoolProperty")put<std::uint8_t>(p+0x7b,mask);return p;}
 void flag(Address a,int offset,std::uint8_t mask,bool on){auto b=raw<std::uint8_t>(a+offset);put<std::uint8_t>(a+offset,static_cast<std::uint8_t>(on?(b|mask):(b&~mask)));}
 Address mesh(const std::wstring& name,Address owner){auto m=object(name,classes[L"StaticMeshComponent"],0x600,owner);put(m+0xb0,owner);flag(m,0x100,0x10,true);return m;}
 Address part(const std::wstring& name,const std::wstring& cls,int team){
  auto p=object(name,classes[cls],0x600,level);put<int>(p+0x308,team);flag(p,0x30c,1,true);
  auto t=object(name+L"_State",classes[L"BP_TickStateObj_C"],0x600);put(p+0x300,t);put<std::uint8_t>(t+0x30,2);put(t+0x38,p);
  return p;
 }
 Address component(const std::wstring& name,Address owner,Address child){auto c=object(name,classes[L"ChildActorComponent"],0x600,owner);put(c+0xb0,owner);put(c+0x308,child);return c;}
 void build(){
  auto object_cls=klass(L"Object"),actor=klass(L"Actor",object_cls),scene=klass(L"SceneComponent",object_cls);
  prop(actor,L"bHidden",L"BoolProperty",0x58,1,0x04);prop(scene,L"bVisible",L"BoolProperty",0x100,1,0x10);prop(scene,L"bHiddenInGame",L"BoolProperty",0x100,1,0x20);
  klass(L"StaticMeshComponent",scene);klass(L"ChildActorComponent",scene);auto level_cls=klass(L"Level",object_cls),world_cls=klass(L"World",object_cls);
  auto duck=klass(L"DuckGameInstance",klass(L"GameInstance",object_cls));
  auto ui=klass(L"BP_UIBase_C",klass(L"DuckUIParentActor",actor));prop(ui,L"TickStateObj",L"ObjectProperty",0x300,8);
  auto battle=klass(L"BP_UIBattleBase_C",ui);prop(battle,L"TeamNo",L"IntProperty",0x308,4);prop(battle,L"IsActivateEnable",L"BoolProperty",0x30c,1);
  auto tick=klass(L"BP_TickStateObj_C",object_cls);prop(tick,L"State",L"ByteProperty",0x30,1);prop(tick,L"PreState",L"BoolProperty",0x31,1);prop(tick,L"ParentObj",L"ObjectProperty",0x38,8);
  auto sp=klass(L"BP_Spacon_C",battle);prop(sp,L"GaugeLevel",L"IntProperty",0x400,4);prop(sp,L"GaugeValue",L"FloatProperty",0x404,4);prop(sp,L"GaugeMaxExist",L"BoolProperty",0x408,1);prop(sp,L"GaugeLevelMax",L"IntProperty",0x40c,4);prop(sp,L"MS_Spacon_Gauge",L"ObjectProperty",0x410,8);prop(sp,L"MS_Spacon_Number",L"ObjectProperty",0x418,8);
  auto gg=klass(L"BP_GemGauge_C",battle);prop(gg,L"GemGaugeKind",L"IntProperty",0x400,4);prop(gg,L"VerGemGauge",L"FloatProperty",0x404,4);prop(gg,L"IsUsed",L"BoolProperty",0x408,1);prop(gg,L"MS_Gauge",L"ObjectProperty",0x410,8);
  auto gc=klass(L"BP_GemCtrl_C",battle);prop(gc,L"BP_GemGauge_1P",L"ObjectProperty",0x400,8);prop(gc,L"BP_GemGauge_2P",L"ObjectProperty",0x408,8);
  auto hr=klass(L"BP_HUD_Root_C",actor);prop(hr,L"CurrentState",L"ByteProperty",0x300,1);prop(hr,L"BP_spacon_1P",L"ObjectProperty",0x308,8);prop(hr,L"BP_spacon_2P",L"ObjectProperty",0x310,8);prop(hr,L"BP_GemCtrl",L"ObjectProperty",0x318,8);
  klass(L"BP_HUD_Story_C",hr);
  world=object(L"World",world_cls);level=object(L"PersistentLevel",level_cls,0x600,world);put(level+0xb0,world);put(world+0x30,level);
  settings=object(L"WorldSettings",actor,0x600,level);put(level+0x2e0,settings);
  engine=object(L"GameEngine",object_cls,0x1000);put(base+0x36bc348,engine);auto viewport=object(L"Viewport",object_cls);put(engine+0x5d8,viewport);put(viewport+0x80,world);
  instance=object(L"Instance",duck);put(engine+0xc80,instance);
  put(hud,base+0x28eec88);put(base+0x34760f0,hud);status=alloc(0x260);put(status,base+0x28ec0e0);put(base+0x34730a8,status);put<std::uint8_t>(hud+0x5bc,4);
  root=object(L"BP_HUD_Root_C_0",hr,0x600,level);put<std::uint8_t>(root+0x300,4);
  gem_ctrl=part(L"GemCtrl",L"BP_GemCtrl_C",0);gem_ctrl_comp=component(L"BP_GemCtrl",root,gem_ctrl);put(root+0x318,gem_ctrl_comp);
  for(int t=0;t<2;++t){
   const std::wstring n=std::to_wstring(t+1)+L"P";
   spacon[t]=part(L"Spacon"+n,L"BP_Spacon_C",t);spacon_comp[t]=component(L"BP_spacon_"+n,root,spacon[t]);put(root+0x308+t*8,spacon_comp[t]);
   bar[t]=mesh(L"MS_Spacon_Gauge",spacon[t]);digit[t]=mesh(L"MS_Spacon_Number",spacon[t]);put(spacon[t]+0x410,bar[t]);put(spacon[t]+0x418,digit[t]);
   put<int>(spacon[t]+0x400,2);put<float>(spacon[t]+0x404,0.25f);put<int>(spacon[t]+0x40c,4);
   gem[t]=part(L"Gem"+n,L"BP_GemGauge_C",t);gem_comp[t]=component(L"BP_GemGauge_"+n,gem_ctrl,gem[t]);put(gem_ctrl+0x400+t*8,gem_comp[t]);
   gem_bar[t]=mesh(L"MS_Gauge",gem[t]);put(gem[t]+0x410,gem_bar[t]);put<int>(gem[t]+0x400,t);put<float>(gem[t]+0x404,0.567f);
   spacon_state[t]=raw<Address>(spacon[t]+0x300);gem_state[t]=raw<Address>(gem[t]+0x300);
   // Point = slot 0 for team t (plId t), record HudInfo+plId*0xe8.
   put<std::int8_t>(hud+0x584+t*4,0);position(t,t?300.0f:-300.0f,0.0f);
  }
 }
 void position(int pl,float x,float height){const Address r=hud+static_cast<Address>(pl)*0xe8;put<int>(r+0x10,pl%2);put<std::uint8_t>(r+0x18,static_cast<std::uint8_t>(pl+1));put<std::uint8_t>(r+0x38,1);put<std::uint8_t>(r+0x39,0);put<std::uint8_t>(r+0x3c,0);put<std::uint8_t>(r+0x42,1);put<std::uint8_t>(r+0x43,0);put<float>(r+0x44,x);put<float>(r+0x48,0);put<float>(r+0x4c,height);}
 void flow(int f){put<std::uint8_t>(hud+0x5bc,static_cast<std::uint8_t>(f));put<std::uint8_t>(root+0x300,static_cast<std::uint8_t>(f));}
 void draw_health(){
  const auto hud_class=classes[L"BP_HUD_Root_C"], vitality=klass(L"BP_Vital_C",classes[L"BP_UIBattleBase_C"]);
  klass(L"BP_Vital_Story_C",vitality);
  prop(hud_class,L"BP_Vital_1P",L"ObjectProperty",0x320,8);
  prop(hud_class,L"BP_Vital_2P",L"ObjectProperty",0x328,8);
  prop(classes[L"SceneComponent"],L"AttachParent",L"ObjectProperty",0x108,8);
  prop(vitality,L" MainId",L"IntProperty",0x400,4);
  const auto ratios=prop(vitality,L"VitalGauge",L"ArrayProperty",0x408,16);
  const auto inner=object(L"VitalGaugeInner",meta(L"FloatProperty"));put<int>(inner+0x34,4);put(ratios+0x78,inner);
  for(int slot=0;slot<2;++slot){
   prop(vitality,slot?L"Root_B":L"Root_A",L"ObjectProperty",0x418+slot*8,8);
   prop(vitality,slot?L"MS_Vital_Gauge_B":L"MS_Vital_Gauge_A",L"ObjectProperty",0x428+slot*8,8);
  }
  for(int team=0;team<2;++team){
   vital[team]=part(L"Vital",team?L"BP_Vital_Story_C":L"BP_Vital_C",team);
   vital_component[team]=component(L"VitalChild",root,vital[team]);
   flag(vital_component[team],0x100,0x10,true);put(root+0x320+team*8,vital_component[team]);
   vital_values[team]=alloc(8);put(vital[team]+0x408,vital_values[team]);
   put<int>(vital[team]+0x410,2);put<int>(vital[team]+0x414,2);
   for(int slot=0;slot<2;++slot){
    const auto actor=vital[team];
    vital_root[team][slot]=mesh(L"VitalRoot",actor);put(actor+0x418+slot*8,vital_root[team][slot]);
    vital_mesh[team][slot]=mesh(L"VitalFill",actor);put(actor+0x428+slot*8,vital_mesh[team][slot]);
    put(vital_mesh[team][slot]+0x108,vital_root[team][slot]);
    const int player=team+slot*2;position(player,team?300.f:-300.f,0.f);
    const auto record=hud+static_cast<Address>(player)*0xe8;
    const int current=slot?2000:team?6000:5000;
    put<int>(record+0x80,current);put<int>(record+0x84,8000);
    put<float>(vital_values[team]+slot*4,static_cast<float>(current)/8000.f);
   }
  }
 }
};
static bool close_to(const std::optional<double>& v,double e){return v&&std::abs(*v-e)<1e-6;}
static void drawn_gauges_positions_and_distance(){
 Fixture f;CombatCapture c(f.unreal);auto s=c.sample(1000,false);
 CHECK(s.valid);CHECK(s.playable);CHECK(!s.cinematic);CHECK(s.session!=0);
 CHECK(close_to(s.fighters[0].hyper_bars,2.25));CHECK(close_to(s.fighters[1].hyper_bars,2.25));
 CHECK(close_to(s.fighters[0].infinity_fraction,0.57));CHECK(s.fighters[0].storm_active==std::optional<bool>(false));CHECK(s.fighters[0].infinity_ready==std::optional<bool>(true));
 CHECK(close_to(s.fighters[0].x,-300));CHECK(close_to(s.fighters[1].x,300));CHECK(close_to(s.normalized_distance,1));CHECK(!s.fighters[0].health_percent);
 f.position(1,0.0f,0.0f);s=c.sample(1010,false);CHECK(close_to(s.normalized_distance,0.5));
 f.put<std::uint8_t>(f.spacon[0]+0x408,1);f.put<int>(f.spacon[0]+0x400,4);f.put<float>(f.spacon[0]+0x404,1.0f);CHECK(close_to(c.sample(1020,false).fighters[0].hyper_bars,4));
 f.put<float>(f.gem[0]+0x404,0.49f);s=c.sample(1030,false);CHECK(s.fighters[0].infinity_ready==std::optional<bool>(false));
}
static void storm_display_is_the_drawn_bar(){
 Fixture f;CombatCapture c(f.unreal);f.put<std::uint8_t>(f.gem[1]+0x408,1);f.put<float>(f.gem[1]+0x404,0.3333f);auto s=c.sample(1000,false);
 CHECK(s.fighters[1].storm_active==std::optional<bool>(true));CHECK(close_to(s.fighters[1].infinity_fraction,0.33));CHECK(s.fighters[1].infinity_ready==std::optional<bool>(false));
 f.put<int>(f.gem[1]+0x400,-1);s=c.sample(1010,false);CHECK(!s.fighters[1].infinity_fraction);CHECK(!s.fighters[1].storm_active);CHECK(close_to(s.fighters[0].infinity_fraction,0.57));
}
static void hidden_or_unrefreshed_parts_are_unavailable(){
 Fixture f;CombatCapture c(f.unreal);
 f.flag(f.spacon[0],0x58,0x04,true);auto s=c.sample(1000,false);CHECK(s.valid);CHECK(!s.fighters[0].hyper_bars);CHECK(s.fighters[0].infinity_fraction);CHECK(s.fighters[1].hyper_bars);f.flag(f.spacon[0],0x58,0x04,false);
 f.put<std::uint8_t>(f.spacon_state[0]+0x31,1);CHECK(!c.sample(1010,false).fighters[0].hyper_bars);f.put<std::uint8_t>(f.spacon_state[0]+0x31,0);
 for(int state:{0,1,3}){f.put<std::uint8_t>(f.gem_state[0]+0x30,static_cast<std::uint8_t>(state));CHECK(!c.sample(1020,false).fighters[0].infinity_fraction);}f.put<std::uint8_t>(f.gem_state[0]+0x30,2);
 f.flag(f.bar[1],0x100,0x10,false);CHECK(!c.sample(1030,false).fighters[1].hyper_bars);f.flag(f.bar[1],0x100,0x10,true);
 f.flag(f.digit[1],0x100,0x20,true);CHECK(!c.sample(1040,false).fighters[1].hyper_bars);f.flag(f.digit[1],0x100,0x20,false);
 f.flag(f.gem[1],0x30c,1,false);CHECK(!c.sample(1050,false).fighters[1].infinity_fraction);f.flag(f.gem[1],0x30c,1,true);
 f.put(f.spacon_state[1]+0x38,f.spacon[0]);CHECK(!c.sample(1060,false).fighters[1].hyper_bars);f.put(f.spacon_state[1]+0x38,f.spacon[1]);
 CHECK(c.sample(1070,false).fighters[1].hyper_bars);
}
static void owner_team_and_world_bindings(){
 Fixture f;CombatCapture c(f.unreal);
 f.put(f.spacon_comp[0]+0xb0,f.gem_ctrl);CHECK(!c.sample(1000,false).fighters[0].hyper_bars);f.put(f.spacon_comp[0]+0xb0,f.root);
 f.put<int>(f.spacon[0]+0x308,1);CHECK(!c.sample(1010,false).fighters[0].hyper_bars);f.put<int>(f.spacon[0]+0x308,0);
 f.put(f.bar[0]+0xb0,f.spacon[1]);CHECK(!c.sample(1020,false).fighters[0].hyper_bars);f.put(f.bar[0]+0xb0,f.spacon[0]);
 f.put(f.gem_comp[1]+0xb0,f.root);CHECK(!c.sample(1030,false).fighters[1].infinity_fraction);f.put(f.gem_comp[1]+0xb0,f.gem_ctrl);
 auto other=f.object(L"OtherLevel",f.classes[L"Level"]);f.put(other+0xb0,f.object(L"OtherWorld",f.classes[L"World"]));
 f.put(f.gem[0]+0x20,other);CHECK(!c.sample(1040,false).fighters[0].infinity_fraction);f.put(f.gem[0]+0x20,f.level);
 f.put(f.root+0x20,other);auto s=c.sample(1050,false);CHECK(!s.valid);CHECK(!s.fighters[0].hyper_bars);CHECK(!s.fighters[0].x);
 f.put(f.root+0x20,f.level);CHECK(c.sample(1600,false).valid);
}
static void stale_root_cdo_and_lifetime(){
 Fixture f;CombatCapture c(f.unreal);auto first=c.sample(1000,false);CHECK(first.valid);
 f.put<std::uint32_t>(f.root+8,0x10);CHECK(!c.sample(1010,false).valid);f.put<std::uint32_t>(f.root+8,0);
 const int index=f.raw<int>(f.root+0xc);f.put<int>(f.entries+index*24+0x10,999);auto renewed=c.sample(1600,false);CHECK(renewed.valid);CHECK(renewed.session!=first.session);
 f.put(Fixture::base+0x34730a8,Address(0));CHECK(!c.sample(1610,false).valid);f.put(Fixture::base+0x34730a8,f.status);
 f.put(f.status,Fixture::base+0x1234);CHECK(!c.sample(1620,false).valid);f.put(f.status,Fixture::base+0x28ec0e0);
 f.put(Fixture::hud,Fixture::base+0x1234);CHECK(!c.sample(1630,false).valid);f.put(Fixture::hud,Fixture::base+0x28eec88);
 f.flag(f.root,0x58,0x04,true);CHECK(!c.sample(2200,false).valid);f.flag(f.root,0x58,0x04,false);
 auto again=c.sample(2800,false);CHECK(again.valid);CHECK(again.session!=renewed.session);
 // A root that is only transiently rejected while the battle context stays intact is the same battle.
 f.flag(f.root,0x58,0x04,true);CHECK(!c.sample(2810,false).valid);f.flag(f.root,0x58,0x04,false);CHECK(c.sample(3400,false).session==again.session);
}
static void cinema_keeps_gauges_but_drops_positions(){
 Fixture f;CombatCapture c(f.unreal);f.put<int>(Fixture::base+0x33e219c,1);auto s=c.sample(1000,false);
 CHECK(s.valid);CHECK(s.cinematic);CHECK(s.fighters[0].hyper_bars);CHECK(s.fighters[1].infinity_fraction);CHECK(!s.fighters[0].x);CHECK(!s.normalized_distance);
 f.put<int>(Fixture::base+0x33e219c,0);s=c.sample(1010,false);CHECK(s.fighters[0].x);CHECK(close_to(s.normalized_distance,1));
}
static void pause_and_menu_suppress_play_and_positions(){
 Fixture f;CombatCapture c(f.unreal);
 f.put<std::uint8_t>(f.instance+0xe8,1);auto s=c.sample(1000,false);CHECK(s.valid);CHECK(!s.playable);CHECK(!s.fighters[0].x);CHECK(s.fighters[0].hyper_bars);f.put<std::uint8_t>(f.instance+0xe8,0);
 f.put(f.settings+0x500,f.root);s=c.sample(1010,false);CHECK(!s.playable);CHECK(!s.normalized_distance);f.put(f.settings+0x500,Address(0));
 s=c.sample(1020,true);CHECK(!s.playable);CHECK(!s.fighters[1].x);
 f.put(f.engine+0xc80,f.root);CHECK(!c.sample(1030,false).playable);f.put(f.engine+0xc80,f.instance);
 CHECK(c.sample(1040,false).playable);
}
static void flow_and_root_state_gate_resources(){
 Fixture f;CombatCapture c(f.unreal);
 f.flow(3);auto s=c.sample(1000,false);CHECK(s.valid);CHECK(!s.playable);CHECK(s.fighters[0].hyper_bars);CHECK(!s.fighters[0].x);
 f.put<std::uint8_t>(f.root+0x300,4);s=c.sample(1010,false);CHECK(s.valid);CHECK(!s.fighters[0].hyper_bars);CHECK(!s.fighters[0].infinity_fraction);
 f.flow(9);s=c.sample(1020,false);CHECK(s.valid);CHECK(!s.playable);CHECK(!s.fighters[0].hyper_bars);
}
static void round_session_and_distance_calibration(){
 Fixture f;CombatCapture c(f.unreal);f.flow(3);auto ready=c.sample(1000,false);CHECK(!ready.normalized_distance);
 f.flow(4);f.position(1,-100.0f,0.0f);auto s=c.sample(1010,false);CHECK(s.session==ready.session);CHECK(close_to(s.normalized_distance,1.0/3.0));
 f.put<std::uint8_t>(Fixture::hud+0xe8+0x3c,1);s=c.sample(1020,false);CHECK(!s.normalized_distance);CHECK(!s.fighters[1].x);CHECK(s.fighters[0].x);
 f.put<std::uint8_t>(Fixture::hud+0xe8+0x3c,0);f.position(1,0.0f,400.0f);s=c.sample(1030,false);CHECK(close_to(s.normalized_distance,std::hypot(300.0,400.0)/600.0));
 f.flow(5);c.sample(1040,false);f.flow(0);auto next=c.sample(1050,false);CHECK(next.valid);CHECK(next.session!=s.session);
 f.flow(3);f.position(1,-200.0f,0.0f);c.sample(1060,false);f.flow(4);s=c.sample(1070,false);CHECK(next.session==s.session);CHECK(close_to(s.normalized_distance,1));
 Fixture g;CombatCapture d(g.unreal);g.position(1,-300.0f,0.0f);CHECK(!d.sample(1000,false).normalized_distance);
 g.put<float>(Fixture::hud+0xe8+0x44,std::nanf(""));s=d.sample(1010,false);CHECK(!s.fighters[1].x);CHECK(!s.normalized_distance);
 g.put<std::int8_t>(Fixture::hud+0x584,3);s=d.sample(1020,false);CHECK(!s.fighters[0].x);
 g.put<std::int8_t>(Fixture::hud+0x584,1);s=d.sample(1030,false);CHECK(!s.fighters[0].x);
}
static void story_root_subclass_and_duplicates(){
 Fixture f;f.put(f.root+0x10,f.classes[L"BP_HUD_Story_C"]);CombatCapture c(f.unreal);CHECK(c.sample(1000,false).valid);
 f.object(L"BP_HUD_Root_C_1",f.classes[L"BP_HUD_Root_C"],0x600,f.level);CombatCapture d(f.unreal);CHECK(!d.sample(1000,false).valid);
}
static void native_team_character_and_displayed_max_are_validated(){
 Fixture f;CombatCapture c(f.unreal);CHECK(c.sample(1000,false).fighters[1].x);f.put<int>(Fixture::hud+0xe8+0x10,0);CHECK(!c.sample(1010,false).fighters[1].x);f.put<int>(Fixture::hud+0xe8+0x10,1);f.put<std::uint8_t>(Fixture::hud+0xe8+0x18,47);CHECK(!c.sample(1020,false).fighters[1].x);f.put<std::uint8_t>(Fixture::hud+0xe8+0x18,2);f.put<int>(f.spacon[0]+0x40c,3);CHECK(!c.sample(1030,false).fighters[0].hyper_bars);
}
static void capture_wait_does_not_recalibrate_mid_round(){
 Fixture f;CombatCapture c(f.unreal);auto first=c.sample(1000,false);f.flow(12);c.sample(1010,false);f.position(0,-150,0);f.position(1,150,0);f.flow(4);auto s=c.sample(1020,false);CHECK(s.session==first.session);CHECK(close_to(s.normalized_distance,.5));
}

static void shared_runtime_discovery_preserves_gauges_and_menu_suppression(){
 Fixture f;CombatCapture cap(f.unreal);CHECK(f.unreal.prepare_capture(1000));auto s=cap.sample(1000,false);CHECK(s.valid);CHECK(s.playable);CHECK(close_to(s.fighters[0].hyper_bars,2.25));CHECK(close_to(s.normalized_distance,1));
 CHECK(f.unreal.prepare_capture(1033));auto menu=cap.sample(1033,true);CHECK(menu.valid);CHECK(!menu.playable);CHECK(!menu.normalized_distance);CHECK(f.unreal.discovery_stats().fallback_scans==0);
}

static void health_follows_the_drawn_active_bar_even_offscreen(){
 Fixture f;f.draw_health();CombatCapture capture(f.unreal);
 CHECK(f.unreal.prepare_capture(1000));
 auto s=capture.sample(1000,false);
 CHECK(close_to(s.fighters[0].health_percent,62.5));CHECK(close_to(s.fighters[1].health_percent,75));
 const auto initial=s.fighters[0].health_fighter;CHECK(initial);
 f.put<std::uint8_t>(Fixture::hud+0xe8+0x3c,1);s=capture.sample(1033,false);
 CHECK(!s.fighters[1].x&&close_to(s.fighters[1].health_percent,75));
 f.put<std::int8_t>(Fixture::hud+0x584,1);CHECK(!capture.sample(1066,false).fighters[0].health_percent);
 f.put<int>(f.vital[0]+0x400,1);s=capture.sample(1099,false);
 CHECK(close_to(s.fighters[0].health_percent,25));CHECK(s.fighters[0].health_fighter&&s.fighters[0].health_fighter!=initial);
 CHECK(close_to(s.fighters[0].hyper_bars,2.25));
 CHECK(f.unreal.discovery_stats().fallback_scans==0);
}
static void health_requires_a_current_visible_and_initialized_display(){
 Fixture f;f.draw_health();CombatCapture capture(f.unreal);
 CHECK(capture.sample(1000,false).fighters[0].health_percent);
 f.flag(f.vital_mesh[0][0],0x100,0x20,true);CHECK(!capture.sample(1033,false).fighters[0].health_percent);
 f.flag(f.vital_mesh[0][0],0x100,0x20,false);
 f.flag(f.vital_root[0][0],0x100,0x10,false);CHECK(!capture.sample(1066,false).fighters[0].health_percent);
 f.flag(f.vital_root[0][0],0x100,0x10,true);
 f.flag(f.vital_component[0],0x100,0x10,false);CHECK(!capture.sample(1099,false).fighters[0].health_percent);
 f.flag(f.vital_component[0],0x100,0x10,true);
 f.put(f.vital_mesh[0][0]+0x108,f.vital_root[0][1]);CHECK(!capture.sample(1132,false).fighters[0].health_percent);
 f.put(f.vital_mesh[0][0]+0x108,f.vital_root[0][0]);
 f.put<float>(f.vital_values[0],0.f);CHECK(!capture.sample(1165,false).fighters[0].health_percent);
 f.put<int>(Fixture::hud+0x80,0);CHECK(close_to(capture.sample(1198,false).fighters[0].health_percent,0));
 f.put<int>(Fixture::hud+0x84,0);CHECK(!capture.sample(1231,false).fighters[0].health_percent);
 f.put<int>(Fixture::hud+0x84,8000);f.put<float>(f.vital_values[0],std::nanf(""));
 CHECK(!capture.sample(1264,false).fighters[0].health_percent);
 f.put<float>(f.vital_values[0],0.f);f.flow(9);CHECK(!capture.sample(1297,false).fighters[0].health_percent);
}
static void a_tag_during_the_health_read_discards_the_outgoing_fighter(){
 Fixture f;f.draw_health();CombatCapture capture(f.unreal);
 const auto underlying=f.memory.reader;
 f.memory.reader=[&](Address address,void* output,std::size_t size){
  const bool read=underlying(address,output,size);
  if(read&&address==f.vital_values[0]&&size==sizeof(float))f.put<std::int8_t>(Fixture::hud+0x584,1);
  return read;
 };
 const auto s=capture.sample(1000,false);
 CHECK(!s.fighters[0].health_percent&&!s.fighters[0].health_fighter);
 CHECK(close_to(s.fighters[1].health_percent,75));
}

int main(){int failed=0;for(auto test:{shared_runtime_discovery_preserves_gauges_and_menu_suppression,drawn_gauges_positions_and_distance,storm_display_is_the_drawn_bar,hidden_or_unrefreshed_parts_are_unavailable,owner_team_and_world_bindings,stale_root_cdo_and_lifetime,cinema_keeps_gauges_but_drops_positions,pause_and_menu_suppress_play_and_positions,flow_and_root_state_gate_resources,round_session_and_distance_calibration,story_root_subclass_and_duplicates,native_team_character_and_displayed_max_are_validated,capture_wait_does_not_recalibrate_mid_round,health_follows_the_drawn_active_bar_even_offscreen,health_requires_a_current_visible_and_initialized_display,a_tag_during_the_health_read_discards_the_outgoing_fighter}){try{test();}catch(const std::exception& e){std::cerr<<e.what()<<'\n';++failed;}}std::cout<<checks<<" combat checks, "<<failed<<" failed groups\n";return failed?1:0;}
