#include "collection_fixture.hpp"
#include <iostream>
#include <iterator>
#include <utility>
using namespace mvci;
namespace mvci {void story_hud(Unreal&,Address,StorySnapshot&);}
static int checks=0;
#define CHECK(x) do{++checks;if(!(x))throw std::runtime_error(std::string(__func__)+":"+std::to_string(__LINE__)+" " #x);}while(false)
// Hand-encoded from the static Story HUD research (research/story-native/summary BP_HUD_Story,
// BP_EnemyNum, BP_Timer, BP_Timer_Story; research/combat ui-report §9 and ghidra t16.c for the
// DuckUIParentActor stock visibility that detaches hidden nodes). Native layouts as in the other
// captures: actor Outer level +0x20 / OwningWorld +0xb0, component owner +0xb0, ChildActor +0x308,
// UArrayProperty.Inner and UStructProperty.Struct +0x78, UStruct.Children +0x38, UField.Next +0x28,
// GNames FName entries. Reflected field offsets are fixture choices resolved by name, as in the game.
// BP_EnemyNum.UserConstructionScript NumUArray (NumVArray is ten zeroes).
constexpr float enemy_u[10]{0.f,0.0996f,0.1992f,0.2988f,0.3984f,0.498f,0.5976f,0.6972f,0.7968f,0.8964f};
struct HudFixture:mvci_test::Fixture {
 struct Enemy {Address actor=0,scene=0,root=0,tens=0,ones=0,tens_mid=0,ones_mid=0;};
 struct Timer {Address actor=0,component=0,scene=0,finite=0,infinite=0,tens=0,ones=0,mark=0;};
 Address hud=0,parent_material=0;std::map<Address,Address> layout;
 HudFixture(){
  const auto scene=cls[L"SceneComponent"];prop(scene,L"AttachParent",L"ObjectProperty",8);
  const auto mesh=klass(L"MeshComponent",klass(L"PrimitiveComponent",scene));arr(mesh,L"OverrideMaterials",L"ObjectProperty",8);
  put(cls[L"StaticMeshComponent"]+0x30,mesh);klass(L"PaperSpriteComponent",mesh);
  const auto material=klass(L"MaterialInterface");parent_material=object(L"MT_EnemyNum",klass(L"Material",material));
  const auto instance=klass(L"MaterialInstance",material);prop(instance,L"Parent",L"ObjectProperty",8);klass(L"MaterialInstanceDynamic",instance);klass(L"MaterialInstanceConstant",instance);
  const auto value=object(L"ScalarParameterValue",kind(L"ScriptStruct"),0x100);
  prop_at(value,L"ExpressionGUID",L"StructProperty",12,16);prop_at(value,L"ParameterValue",L"FloatProperty",8,4);prop_at(value,L"ParameterName",L"NameProperty",0,8);
  put(raw<Address>(arr(instance,L"ScalarParameterValues",L"StructProperty",28)+0x78)+0x78,value);
  prop(cls[L"BP_UIBase_C"],L"RootScene",L"ObjectProperty",8);const auto battle=klass(L"BP_UIBattleBase_C",cls[L"BP_UIBase_C"]);
  const auto enemy=klass(L"BP_EnemyNum_C",battle);arr(enemy,L"MIDNum",L"ObjectProperty",8);arr(enemy,L"NumUArray",L"FloatProperty",4);arr(enemy,L"NumVArray",L"FloatProperty",4);
  for(auto n:{L"Root_Num",L"MS_EnemyNum_10",L"MS_EnemyNum_01",L"MS_EnemyNum_M",L"MS_Story_EnmIcon",L"MS_EnemyBg"})prop(enemy,n,L"ObjectProperty",8);
  const auto timer=klass(L"BP_Timer_C",battle);prop(timer,L"TimeNow",L"IntProperty",4);
  for(auto n:{L"Root_Infinite",L"SP_Timer_Infinite",L"Root_Finite",L"SP_Timer_01",L"SP_Timer_10"})prop(timer,n,L"ObjectProperty",8);
  prop(klass(L"BP_Timer_Story_C",timer),L"Is20Sec",L"BoolProperty",1);
  const auto root=klass(L"BP_HUD_Root_C",cls[L"Actor"]);prop(root,L"BP_Timer",L"ObjectProperty",8);prop(root,L"Layout_Up",L"ObjectProperty",8);
  arr(klass(L"BP_HUD_Story_C",root),L"StoryHUD",L"ObjectProperty",8);hud=root_actor(L"BP_HUD_Story_C",level);
 }
 Address root_actor(const std::wstring& c,Address in){
  const auto r=object(c,cls.at(c),0x2000,in);const auto up=comp(L"SceneComponent",L"Layout_Up",r);set(r,L"Layout_Up",up);layout[r]=up;
  if(unreal.is_a(r,L"BP_HUD_Story_C"))items<Address>(r,L"StoryHUD",{});return r;
 }
 Address elsewhere(){const auto w=object(L"World",cls[L"World"]);const auto l=object(L"PersistentLevel",cls[L"Level"],0x2000,w);put(l+0xb0,w);return l;}
 // A part component owned by the actor and attached to parent (0: detached by stock visibility).
 Address node(const std::wstring& c,const std::wstring& field,Address owner,Address parent,bool drawn=true){const auto x=comp(c,field,owner,drawn);set(x,L"AttachParent",parent);set(owner,field,x);return x;}
 // GNames entry without the wide flag: the engine stores ASCII identifiers such as Param_U this way.
 int ansi(const std::wstring& s){
  const auto i=name_ids.find(s);if(i!=name_ids.end())return i->second;const int n=name_count++;const auto a=alloc(0x10+s.size()+1);put<std::uint32_t>(a,0);
  for(std::size_t j=0;j<s.size();++j)put<std::uint8_t>(a+0x10+j,static_cast<std::uint8_t>(s[j]));put(chunk+static_cast<Address>(n)*8,a);name_ids[s]=n;return n;
 }
 Address mid(Address mesh,const std::vector<std::pair<std::wstring,float>>& scalars,bool wide=false,const std::wstring& type=L"MaterialInstanceDynamic"){
  const auto m=object(type,cls.at(type),0x2000,mesh);set(m,L"Parent",parent_material);
  const auto data=alloc(std::max<std::size_t>(1,scalars.size())*28);
  for(std::size_t i=0;i<scalars.size();++i){put<int>(data+i*28,wide?name_id(scalars[i].first):ansi(scalars[i].first));put<int>(data+i*28+4,0);put<float>(data+i*28+8,scalars[i].second);}
  const auto at=m+static_cast<Address>(off(m,L"ScalarParameterValues"));put(at,data);put<int>(at+8,static_cast<int>(scalars.size()));put<int>(at+12,static_cast<int>(scalars.size()));return m;
 }
 Address scalar(Address m,int i){return raw<Address>(m+static_cast<Address>(off(m,L"ScalarParameterValues")))+static_cast<Address>(i)*28;}
 Enemy counter(int tens,int ones,bool tens_drawn,Address owner=0){
  Enemy e;e.actor=actor(cls[L"BP_EnemyNum_C"],L"BP_EnemyNum_C");tick(e.actor);
  e.scene=node(L"SceneComponent",L"RootScene",e.actor,layout.at(owner?owner:hud));e.root=node(L"SceneComponent",L"Root_Num",e.actor,e.scene);
  e.tens=node(L"StaticMeshComponent",L"MS_EnemyNum_10",e.actor,tens_drawn?e.root:0,tens_drawn);e.ones=node(L"StaticMeshComponent",L"MS_EnemyNum_01",e.actor,e.root);
  for(auto n:{L"MS_EnemyNum_M",L"MS_Story_EnmIcon",L"MS_EnemyBg"})node(L"StaticMeshComponent",n,e.actor,e.root);
  e.tens_mid=mid(e.tens,{{L"Emissive",0.f},{L"Param_U",enemy_u[tens]},{L"Param_V",0.f}});e.ones_mid=mid(e.ones,{{L"Emissive",0.f},{L"Param_U",enemy_u[ones]},{L"Param_V",0.f}});
  items<Address>(e.tens,L"OverrideMaterials",{e.tens_mid});items<Address>(e.ones,L"OverrideMaterials",{e.ones_mid});items<Address>(e.actor,L"MIDNum",{e.tens_mid,e.ones_mid});
  items<float>(e.actor,L"NumUArray",std::vector<float>(std::begin(enemy_u),std::end(enemy_u)));items<float>(e.actor,L"NumVArray",std::vector<float>(10,0.f));
  return e;
 }
 void bind(const std::vector<Address>& parts,Address owner=0){items<Address>(owner?owner:hud,L"StoryHUD",parts);}
 // BP_Timer.PreEnter shows Root_Finite (digits) or Root_Infinite (mark) and hides the other.
 Timer timer(int time,bool infinite,Address owner=0,const std::wstring& type=L"BP_Timer_Story_C"){
  Timer t;t.actor=actor(cls.at(type),type);tick(t.actor,infinite?1:2);set<int>(t.actor,L"TimeNow",time);t.component=attach(owner?owner:hud,L"BP_Timer",t.actor);
  t.scene=node(L"SceneComponent",L"RootScene",t.actor,t.component);
  t.finite=node(L"SceneComponent",L"Root_Finite",t.actor,infinite?0:t.scene,!infinite);t.infinite=node(L"SceneComponent",L"Root_Infinite",t.actor,infinite?t.scene:0,infinite);
  t.tens=node(L"PaperSpriteComponent",L"SP_Timer_10",t.actor,infinite?0:t.finite,!infinite);t.ones=node(L"PaperSpriteComponent",L"SP_Timer_01",t.actor,infinite?0:t.finite,!infinite);
  t.mark=node(L"PaperSpriteComponent",L"SP_Timer_Infinite",t.actor,infinite?t.infinite:0,infinite);
  return t;
 }
 void phase(Address part,int state,bool pre=false){const auto t=raw<Address>(part+static_cast<Address>(off(part,L"TickStateObj")));set<std::uint8_t>(t,L"State",static_cast<std::uint8_t>(state));flag(t,L"PreState",pre);}
};
static StorySnapshot capture(HudFixture& f){StorySnapshot s;story_hud(f.unreal,f.world,s);return s;}

static void enemy_counter_reads_both_drawn_digits(){
 HudFixture f;const auto e=f.counter(1,2,true);f.bind({e.actor});const auto s=capture(f);CHECK(s.valid);CHECK(s.enemies_remaining==12);CHECK(!s.time_seconds&&!s.infinite_time);
 HudFixture g;const auto forty=g.counter(4,0,true);g.bind({forty.actor});CHECK(capture(g).enemies_remaining==40);
}
static void enemy_counter_hidden_tens_mesh_leaves_one_digit(){
 // Active hides MS_EnemyNum_10 while Remain/10 is 0; its MID keeps the last tens value.
 HudFixture f;const auto e=f.counter(1,7,false);f.bind({e.actor});CHECK(capture(f).enemies_remaining==7);
 f.flag(e.ones,L"bVisible",false);CHECK(!capture(f).enemies_remaining);f.flag(e.ones,L"bVisible",true);CHECK(capture(f).enemies_remaining==7);
 // A drawn tens mesh is part of the number even when it shows 0.
 HudFixture g;const auto zero=g.counter(0,7,true);g.bind({zero.actor});CHECK(capture(g).enemies_remaining==7);
}
static void enemy_counter_decodes_only_the_drawn_material(){
 HudFixture f;const auto e=f.counter(2,5,true);f.bind({e.actor});CHECK(capture(f).enemies_remaining==25);
 const auto u=f.scalar(e.ones_mid,1),v=f.scalar(e.ones_mid,2);
 f.put<float>(u+8,0.05f);CHECK(!capture(f).enemies_remaining);f.put<float>(u+8,enemy_u[5]);
 f.put<float>(v+8,0.5f);CHECK(!capture(f).enemies_remaining);f.put<float>(v+8,0.f);CHECK(capture(f).enemies_remaining==25);
 f.put<int>(v,f.name_id(L"Emissive"));CHECK(!capture(f).enemies_remaining);f.put<int>(v,f.name_id(L"Param_V"));CHECK(capture(f).enemies_remaining==25);
 f.put<int>(u+4,1);CHECK(!capture(f).enemies_remaining);f.put<int>(u+4,0);CHECK(capture(f).enemies_remaining==25); // FName Param_U_0
 // FNames compare without case; a duplicated name has no single drawn value.
 const auto upper=f.mid(e.ones,{{L"PARAM_U",enemy_u[5]},{L"param_v",0.f}});f.items<Address>(e.ones,L"OverrideMaterials",{upper});f.items<Address>(e.actor,L"MIDNum",{e.tens_mid,upper});CHECK(capture(f).enemies_remaining==25);
 // Wide GNames entries (flag bit 0 set) decode the same way as the ANSI ones above.
 const auto wide=f.mid(e.ones,{{L"param_U",enemy_u[5]},{L"param_V",0.f}},true);f.items<Address>(e.ones,L"OverrideMaterials",{wide});f.items<Address>(e.actor,L"MIDNum",{e.tens_mid,wide});CHECK(capture(f).enemies_remaining==25);
 const auto twice=f.mid(e.ones,{{L"Param_U",enemy_u[5]},{L"Param_V",0.f},{L"param_u",enemy_u[5]}});f.items<Address>(e.ones,L"OverrideMaterials",{twice});f.items<Address>(e.actor,L"MIDNum",{e.tens_mid,twice});CHECK(!capture(f).enemies_remaining);
 f.items<Address>(e.ones,L"OverrideMaterials",{e.ones_mid});f.items<Address>(e.actor,L"MIDNum",{e.tens_mid,e.ones_mid});CHECK(capture(f).enemies_remaining==25);
 // MIDNum[1] must still be the material MS_EnemyNum_01 draws; the construction script makes exactly two.
 const auto copy=f.mid(e.ones,{{L"Param_U",enemy_u[5]},{L"Param_V",0.f}});f.items<Address>(e.ones,L"OverrideMaterials",{copy});CHECK(!capture(f).enemies_remaining);f.items<Address>(e.ones,L"OverrideMaterials",{e.ones_mid});
 f.items<Address>(e.actor,L"MIDNum",{e.tens_mid,e.ones_mid,copy});CHECK(!capture(f).enemies_remaining);f.items<Address>(e.actor,L"MIDNum",{e.tens_mid,e.ones_mid});
 f.items<Address>(e.actor,L"MIDNum",{e.ones_mid,e.tens_mid});CHECK(!capture(f).enemies_remaining);f.items<Address>(e.actor,L"MIDNum",{e.tens_mid,e.ones_mid});
 // The construction script makes dynamic instances on StaticMeshComponents only.
 const auto fixed=f.mid(e.ones,{{L"Param_U",enemy_u[5]},{L"Param_V",0.f}},false,L"MaterialInstanceConstant");f.items<Address>(e.ones,L"OverrideMaterials",{fixed});f.items<Address>(e.actor,L"MIDNum",{e.tens_mid,fixed});CHECK(!capture(f).enemies_remaining);
 f.items<Address>(e.ones,L"OverrideMaterials",{e.ones_mid});f.items<Address>(e.actor,L"MIDNum",{e.tens_mid,e.ones_mid});CHECK(capture(f).enemies_remaining==25);
 const auto sprite=f.node(L"PaperSpriteComponent",L"MS_EnemyNum_01",e.actor,e.root);f.items<Address>(sprite,L"OverrideMaterials",{e.ones_mid});CHECK(!capture(f).enemies_remaining);f.set(e.actor,L"MS_EnemyNum_01",e.ones);CHECK(capture(f).enemies_remaining==25);
 f.items<float>(e.actor,L"NumUArray",std::vector<float>(std::begin(enemy_u),std::end(enemy_u)-1));CHECK(!capture(f).enemies_remaining);
 // An atlas entry shared by two digits leaves the drawn digit ambiguous.
 auto shared=std::vector<float>(std::begin(enemy_u),std::end(enemy_u));shared[6]=enemy_u[5];f.items<float>(e.actor,L"NumUArray",shared);CHECK(!capture(f).enemies_remaining);
 f.items<float>(e.actor,L"NumUArray",std::vector<float>(std::begin(enemy_u),std::end(enemy_u)));CHECK(capture(f).enemies_remaining==25);
}
static void enemy_counter_requires_its_declared_active_part(){
 HudFixture f;const auto e=f.counter(1,2,true);f.bind({e.actor});CHECK(capture(f).enemies_remaining==12);
 // StoryHUD also holds other spawned Story parts (CreateGauge); only the declared BP_EnemyNum counts.
 const auto gauge=f.actor(f.cls[L"BP_UIBattleBase_C"],L"BP_NomalGauge_C");f.bind({gauge,e.actor});CHECK(capture(f).enemies_remaining==12);f.bind({Address(0),e.actor});CHECK(capture(f).enemies_remaining==12);
 f.bind({});CHECK(!capture(f).enemies_remaining);const auto other=f.counter(3,4,true);f.bind({e.actor,other.actor});CHECK(!capture(f).enemies_remaining);f.bind({e.actor});CHECK(capture(f).enemies_remaining==12);
 for(int state:{0,1,3}){f.phase(e.actor,state);CHECK(!capture(f).enemies_remaining);}f.phase(e.actor,2,true);CHECK(!capture(f).enemies_remaining);f.phase(e.actor,2);CHECK(capture(f).enemies_remaining==12);
 const auto tick=f.raw<Address>(e.actor+static_cast<Address>(f.off(e.actor,L"TickStateObj")));f.set(tick,L"ParentObj",other.actor);CHECK(!capture(f).enemies_remaining);f.set(tick,L"ParentObj",e.actor);
 const auto lookalike=f.klass(L"BP_TickStateObj_Copy_C");f.prop(lookalike,L"State",L"ByteProperty",1);f.prop(lookalike,L"PreState",L"BoolProperty",1);f.prop(lookalike,L"ParentObj",L"ObjectProperty",8);
 const auto copy=f.object(L"Tick",lookalike);f.set<std::uint8_t>(copy,L"State",2);f.set(copy,L"ParentObj",e.actor);f.set(e.actor,L"TickStateObj",copy);CHECK(!capture(f).enemies_remaining);f.set(e.actor,L"TickStateObj",tick);CHECK(capture(f).enemies_remaining==12);
 f.flag(e.actor,L"bHidden",true);CHECK(!capture(f).enemies_remaining);f.flag(e.actor,L"bHidden",false);
 f.put(e.actor+0x20,f.elsewhere());CHECK(!capture(f).enemies_remaining);f.put(e.actor+0x20,f.level);CHECK(capture(f).enemies_remaining==12);
}
static void enemy_counter_follows_the_stock_attachment_chain(){
 HudFixture f;const auto e=f.counter(1,2,true);f.bind({e.actor});CHECK(capture(f).enemies_remaining==12);
 f.set(e.ones,L"AttachParent",e.scene);CHECK(!capture(f).enemies_remaining);f.set(e.ones,L"AttachParent",e.root);
 f.set(e.root,L"AttachParent",Address(0));CHECK(!capture(f).enemies_remaining);f.set(e.root,L"AttachParent",e.scene);
 f.flag(e.root,L"bVisible",false);CHECK(!capture(f).enemies_remaining);f.flag(e.root,L"bVisible",true);
 f.flag(e.scene,L"bHiddenInGame",true);CHECK(!capture(f).enemies_remaining);f.flag(e.scene,L"bHiddenInGame",false);
 f.flag(e.ones,L"bHiddenInGame",true);CHECK(!capture(f).enemies_remaining);f.flag(e.ones,L"bHiddenInGame",false);
 f.put(e.ones+0xb0,f.hud);CHECK(!capture(f).enemies_remaining);f.put(e.ones+0xb0,e.actor);
 // A visible tens mesh that is not hanging from Root_Num is not the counter's drawn layout.
 f.set(e.tens,L"AttachParent",Address(0));CHECK(!capture(f).enemies_remaining);f.set(e.tens,L"AttachParent",e.root);CHECK(capture(f).enemies_remaining==12);
}
static void story_hud_needs_the_unique_drawn_story_root(){
 HudFixture f;const auto e=f.counter(1,2,true);f.bind({e.actor});f.timer(45,false);auto s=capture(f);CHECK(s.enemies_remaining==12);CHECK(s.time_seconds==45);
 const auto second=f.root_actor(L"BP_HUD_Root_C",f.level);s=capture(f);CHECK(!s.enemies_remaining&&!s.time_seconds&&!s.valid);
 f.flag(second,L"bHidden",true);CHECK(capture(f).enemies_remaining==12);
 f.root_actor(L"BP_HUD_Story_C",f.elsewhere());s=capture(f);CHECK(s.enemies_remaining==12&&s.time_seconds==45);
 f.flag(f.hud,L"bHidden",true);s=capture(f);CHECK(!s.enemies_remaining&&!s.time_seconds);f.flag(f.hud,L"bHidden",false);CHECK(capture(f).time_seconds==45);
 // A versus/arcade HUD root is not the Story HUD, even with a Story-shaped timer child.
 HudFixture g;g.flag(g.hud,L"bHidden",true);const auto plain=g.root_actor(L"BP_HUD_Root_C",g.level);g.timer(45,false,plain);CHECK(!capture(g).time_seconds);
}
static void story_timer_reads_time_now_from_drawn_digits(){
 for(int value:{45,9,0}){HudFixture f;f.timer(value,false);const auto s=capture(f);CHECK(s.valid);CHECK(s.time_seconds==value);CHECK(!s.infinite_time);CHECK(!s.enemies_remaining);}
}
static void story_timer_waits_for_an_active_refreshed_value(){
 HudFixture f;const auto t=f.timer(45,false);CHECK(capture(f).time_seconds==45);
 // The finite timer stays in Enter for its Opacity fade-in; BP_Timer_Story parks 999 in TimeNow until Active.
 f.phase(t.actor,1);CHECK(!capture(f).time_seconds);f.phase(t.actor,2,true);CHECK(!capture(f).time_seconds);f.phase(t.actor,3);CHECK(!capture(f).time_seconds);f.phase(t.actor,2);
 for(int value:{999,100,-1}){f.set<int>(t.actor,L"TimeNow",value);const auto s=capture(f);CHECK(!s.time_seconds&&!s.infinite_time);}f.set<int>(t.actor,L"TimeNow",99);CHECK(capture(f).time_seconds==99);
}
static void story_timer_reports_infinity_only_for_the_drawn_mark(){
 // Only the finite branch ends the Enter state (Opacity timeline -> SetTickStateEnd).
 HudFixture f;const auto t=f.timer(0,true);auto s=capture(f);CHECK(s.valid&&s.infinite_time&&!s.time_seconds);
 f.phase(t.actor,2);CHECK(capture(f).infinite_time);f.phase(t.actor,1,true);CHECK(!capture(f).infinite_time);f.phase(t.actor,0);CHECK(!capture(f).infinite_time);f.phase(t.actor,1);CHECK(capture(f).infinite_time);
 // Activation restores the BeginPlay tree with both branches drawn until PreEnter picks one.
 f.flag(t.finite,L"bVisible",true);f.set(t.finite,L"AttachParent",t.scene);f.flag(t.tens,L"bVisible",true);f.set(t.tens,L"AttachParent",t.finite);f.flag(t.ones,L"bVisible",true);f.set(t.ones,L"AttachParent",t.finite);
 s=capture(f);CHECK(!s.infinite_time&&!s.time_seconds);f.flag(t.tens,L"bVisible",false);f.flag(t.ones,L"bVisible",false);CHECK(capture(f).infinite_time);
 f.set(t.mark,L"AttachParent",Address(0));CHECK(!capture(f).infinite_time);f.set(t.mark,L"AttachParent",t.infinite);
 f.flag(t.infinite,L"bVisible",false);CHECK(!capture(f).infinite_time);f.flag(t.infinite,L"bVisible",true);
 f.flag(t.mark,L"bVisible",false);CHECK(!capture(f).infinite_time);f.flag(t.mark,L"bVisible",true);CHECK(capture(f).infinite_time);
}
static void story_timer_requires_the_declared_story_child(){
 HudFixture f;const auto t=f.timer(45,false);CHECK(capture(f).time_seconds==45);
 f.flag(t.ones,L"bVisible",false);CHECK(!capture(f).time_seconds);f.flag(t.ones,L"bVisible",true);
 f.flag(t.tens,L"bVisible",false);CHECK(!capture(f).time_seconds);f.flag(t.tens,L"bVisible",true);
 f.flag(t.finite,L"bVisible",false);CHECK(!capture(f).time_seconds);f.flag(t.finite,L"bVisible",true);
 f.set(t.tens,L"AttachParent",Address(0));CHECK(!capture(f).time_seconds);f.set(t.tens,L"AttachParent",t.finite);
 f.set(t.ones,L"AttachParent",Address(0));CHECK(!capture(f).time_seconds);f.set(t.ones,L"AttachParent",t.finite);
 // BP_Timer is a ChildActorComponent and the digits are PaperSpriteComponents.
 const auto plain=f.comp(L"SceneComponent",L"BP_Timer",f.hud);f.put(plain+0x308,t.actor);f.set(f.hud,L"BP_Timer",plain);CHECK(!capture(f).time_seconds);f.set(f.hud,L"BP_Timer",t.component);
 f.node(L"StaticMeshComponent",L"SP_Timer_01",t.actor,t.finite);CHECK(!capture(f).time_seconds);f.set(t.actor,L"SP_Timer_01",t.ones);CHECK(capture(f).time_seconds==45);
 f.set(t.finite,L"AttachParent",Address(0));CHECK(!capture(f).time_seconds);f.set(t.finite,L"AttachParent",t.scene);
 f.flag(t.scene,L"bVisible",false);CHECK(!capture(f).time_seconds);f.flag(t.scene,L"bVisible",true);
 f.flag(t.mark,L"bVisible",true);CHECK(!capture(f).time_seconds);f.flag(t.mark,L"bVisible",false);
 f.put(t.component+0xb0,t.actor);CHECK(!capture(f).time_seconds);f.put(t.component+0xb0,f.hud);
 f.flag(t.actor,L"bHidden",true);CHECK(!capture(f).time_seconds);f.flag(t.actor,L"bHidden",false);
 f.put(t.actor+0x20,f.elsewhere());CHECK(!capture(f).time_seconds);f.put(t.actor+0x20,f.level);
 f.set(f.hud,L"BP_Timer",Address(0));CHECK(!capture(f).time_seconds);f.set(f.hud,L"BP_Timer",t.component);CHECK(capture(f).time_seconds==45);
 // The Story HUD's BP_Timer child class is BP_Timer_Story_C.
 HudFixture g;g.timer(45,false,0,L"BP_Timer_C");CHECK(!capture(g).time_seconds);
}
static void late_changes_reject_only_the_changed_surface(){
 HudFixture f;const auto e=f.counter(1,2,true);f.bind({e.actor});const auto t=f.timer(45,false);auto s=capture(f);CHECK(s.enemies_remaining==12&&s.time_seconds==45);
 const auto u=f.scalar(e.ones_mid,1)+8,seconds=t.actor+static_cast<Address>(f.off(t.actor,L"TimeNow"));auto raw=f.memory.reader;bool fired=false;
 f.memory.reader=[&](Address at,void* out,std::size_t n){const auto ok=raw(at,out,n);if(!fired&&at==u){fired=true;f.put<float>(u,enemy_u[3]);}return ok;};
 s=capture(f);CHECK(fired);CHECK(!s.enemies_remaining);CHECK(s.time_seconds==45);
 fired=false;f.memory.reader=[&](Address at,void* out,std::size_t n){const auto ok=raw(at,out,n);if(!fired&&at==seconds){fired=true;f.put<int>(seconds,44);}return ok;};
 s=capture(f);CHECK(fired);CHECK(!s.time_seconds);CHECK(s.enemies_remaining==13);
 f.memory.reader=raw;s=capture(f);CHECK(s.enemies_remaining==13&&s.time_seconds==44);
 // A rebuilt StoryHUD array (header changed after it was read) is not the binding that was decoded.
 const auto list=f.hud+static_cast<Address>(f.off(f.hud,L"StoryHUD"));fired=false;
 f.memory.reader=[&](Address at,void* out,std::size_t n){const auto ok=raw(at,out,n);if(!fired&&at==list){fired=true;f.put<int>(list+8,0);}return ok;};
 s=capture(f);CHECK(fired);CHECK(!s.enemies_remaining);CHECK(s.time_seconds==44);f.memory.reader=raw;f.put<int>(list+8,1);CHECK(capture(f).enemies_remaining==13);
}
static void snapshot_hud_fields_are_replaced_each_sample(){
 HudFixture f;StorySnapshot s;s.enemies_remaining=3;s.time_seconds=4;s.infinite_time=true;s.captions[0]=StoryTextSnapshot{true,7,L"XGARD"};story_hud(f.unreal,f.world,s);
 CHECK(!s.enemies_remaining&&!s.time_seconds&&!s.infinite_time);CHECK(!s.valid);CHECK(s.captions[0].valid&&s.captions[0].text==L"XGARD");
 const auto e=f.counter(0,9,false);f.bind({e.actor});story_hud(f.unreal,f.world,s);CHECK(s.valid&&s.enemies_remaining==9);CHECK(s.captions[0].text==L"XGARD");
}
int main(){int failed=0;for(auto test:{enemy_counter_reads_both_drawn_digits,enemy_counter_hidden_tens_mesh_leaves_one_digit,enemy_counter_decodes_only_the_drawn_material,enemy_counter_requires_its_declared_active_part,enemy_counter_follows_the_stock_attachment_chain,story_hud_needs_the_unique_drawn_story_root,story_timer_reads_time_now_from_drawn_digits,story_timer_waits_for_an_active_refreshed_value,story_timer_reports_infinity_only_for_the_drawn_mark,story_timer_requires_the_declared_story_child,late_changes_reject_only_the_changed_surface,snapshot_hud_fields_are_replaced_each_sample}){try{test();}catch(const std::exception& e){std::cerr<<e.what()<<'\n';++failed;}}std::cout<<checks<<" story HUD checks, "<<failed<<" failed groups\n";return failed?1:0;}
