#include "mvci/story.hpp"
#include "story_read.hpp"
#include <cmath>
// Story battle HUD exactly as drawn (research/story-native/summary BP_HUD_Story, BP_EnemyNum, BP_Timer,
// BP_Timer_Story; research/combat ui-report §9). Reads only; never the logical remaining-enemy count.
namespace mvci {
namespace {
using story_detail::Frame;
constexpr Address names_rva=0x35cafd0; // GNames, as unreal.cpp
constexpr int digits=10,infinity=-1;
struct Member {int offset=0,size=0;std::wstring kind;};
using Properties=std::map<std::pair<Address,std::wstring>,std::optional<Property>>;
// Each reflected property is resolved once per class per capture; the Frame fences the bytes read
// at the resolved offsets, so the late recheck re-reads values instead of walking reflection again.
struct Reader {
 Frame& f;Properties& cache;
 std::optional<Property> property(Address a,const wchar_t* name){
  const auto o=f.u.object(a);if(!o)return {};auto key=std::make_pair(o->cls,std::wstring(name));
  auto found=cache.find(key);if(found==cache.end())found=cache.emplace(std::move(key),f.u.property(a,name)).first;return found->second;
 }
 std::optional<Address> field(Address a,const wchar_t* name){
  const auto p=property(a,name);if(!p||p->kind!=L"ObjectProperty"||p->size!=8)return {};
  const auto v=f.get<Address>(a+static_cast<Address>(p->offset));if(v&&*v&&!f.retain(*v))return {};return v;
 }
 std::optional<bool> boolean(Address a,const wchar_t* name){
  const auto p=property(a,name);if(!p||p->kind!=L"BoolProperty")return {};
  const auto b=f.get<std::uint8_t>(a+static_cast<Address>(p->offset+p->bool_byte));if(!b)return {};return (*b&p->bool_mask)!=0;
 }
 std::optional<int> integer(Address a,const wchar_t* name){
  const auto p=property(a,name);if(!p)return {};
  if(p->kind==L"IntProperty"&&p->size==4)return f.get<int>(a+static_cast<Address>(p->offset));
  if((p->kind==L"ByteProperty"||p->kind==L"EnumProperty")&&p->size==1){const auto b=f.get<std::uint8_t>(a+static_cast<Address>(p->offset));if(b)return *b;}
  return {};
 }
 std::optional<ArrayView> array(Address a,const std::optional<Property>& p,int limit,const wchar_t* inner,int stride){
  if(!p||p->inner!=inner||p->stride!=stride)return {};const auto v=f.u.array(a,*p,limit);
  f.checks.push_back([&u=f.u,a,prop=*p,v,limit]{const auto n=u.array(a,prop,limit);return bool(n)==bool(v)&&(!v||(n->data==v->data&&n->count==v->count&&n->stride==v->stride));});return v;
 }
 std::optional<ArrayView> array(Address a,const wchar_t* name,int limit,const wchar_t* inner=L"ObjectProperty",int stride=8){return array(a,property(a,name),limit,inner,stride);}
};
bool placed(Reader& r,Address actor,Address world){
 // AActor Outer is its ULevel; ULevel.OwningWorld (+0xb0) is the world it is loaded into.
 if(!r.f.retain(actor))return false;const auto level=r.f.get<Address>(actor+0x20);
 return level&&*level&&r.f.u.is_a(*level,L"Level")&&r.f.get<Address>(*level+0xb0)==std::optional<Address>(world)&&r.boolean(actor,L"bHidden")==std::optional<bool>(false);
}
std::optional<int> settled(Reader& r,Address part){
 // EUITickState 0 Hidden, 1 Enter, 2 Active, 3 Exit. SetState sets PreState; the state's first tick
 // clears it before PreEnter/PreActive and Enter/Active run (BP_TickStateObj).
 const auto tick=r.field(part,L"TickStateObj");const auto ref=tick&&*tick?r.f.u.object(*tick):std::nullopt;
 if(!ref||!r.f.u.name_is(ref->cls,L"BP_TickStateObj_C")||r.field(*tick,L"ParentObj")!=std::optional<Address>(part)||r.boolean(*tick,L"PreState")!=std::optional<bool>(false))return {};
 return r.integer(*tick,L"State");
}
std::optional<bool> drawn(Reader& r,Address part,const std::optional<Address>& c){
 // Rendered: owned by the part (component +0xb0), bVisible and not bHiddenInGame.
 if(!c||!*c||!r.f.u.is_a(*c,L"SceneComponent")||r.f.get<Address>(*c+0xb0)!=std::optional<Address>(part))return {};
 const auto visible=r.boolean(*c,L"bVisible"),hidden=r.boolean(*c,L"bHiddenInGame");if(!visible||!hidden)return {};
 return *visible&&!*hidden;
}
std::optional<Address> layout(Reader& r,Address part,const wchar_t* name){
 // DuckUIParentActor stock visibility (research/combat ghidra t16.c) hides a node with its children and
 // detaches them; showing re-attaches the BeginPlay tree. The declared root of a drawn node must hang
 // from BP_UIBase.RootScene, both drawn and owned by the part.
 const auto root=r.field(part,name),scene=r.field(part,L"RootScene");
 if(!root||!*root||!scene||!*scene||r.field(*root,L"AttachParent")!=scene||drawn(r,part,root)!=std::optional<bool>(true)||drawn(r,part,scene)!=std::optional<bool>(true))return {};
 return root;
}
bool named(const Unreal& u,int index,int number,const wchar_t* expected){
 // FName {ComparisonIndex, Number} through GNames like Unreal::name; identifiers compare without ASCII case.
 if(index<0||index>=1048576||number!=0)return false;
 const auto& m=u.memory();const auto table=m.get<Address>(u.base()+names_rva);if(!table||!*table)return false;
 const auto chunk=m.get<Address>(*table+static_cast<Address>(index/16384)*8);if(!chunk||!*chunk)return false;
 const auto entry=m.get<Address>(*chunk+static_cast<Address>(index%16384)*8);if(!entry||!*entry)return false;
 const auto wide=m.get<std::uint32_t>(*entry);if(!wide)return false;
 const auto fold=[](wchar_t c){return c>=L'A'&&c<=L'Z'?static_cast<wchar_t>(c+(L'a'-L'A')):c;};
 for(std::size_t i=0;;++i){
  std::optional<wchar_t> c;
  if(*wide&1){if(const auto w=m.get<std::uint16_t>(*entry+0x10+i*2))c=static_cast<wchar_t>(*w);}
  else if(const auto b=m.get<std::uint8_t>(*entry+0x10+i))c=static_cast<wchar_t>(*b);
  if(!c||fold(*c)!=fold(expected[i]))return false;if(!expected[i])return true;
 }
}
std::optional<Member> member(Frame& f,Address s,const wchar_t* name){
 // UScriptStruct members: UStruct.Children +0x38, UField.Next +0x28, UProperty offset +0x50 / size +0x34.
 auto field=f.get<Address>(s+0x38);std::set<Address> seen;
 while(field&&*field&&seen.size()<128&&seen.insert(*field).second){
  if(!f.retain(*field))return {};
  if(f.u.name_is(*field,name)){const auto offset=f.get<int>(*field+0x50),size=f.get<int>(*field+0x34);if(!offset||!size||*offset<0||*size<=0)return {};return Member{*offset,*size,f.u.type(*field)};}
  field=f.get<Address>(*field+0x28);
 }
 return {};
}
std::optional<float> scalar(Reader& r,Address material,const wchar_t* name){
 // UMaterialInstance.ScalarParameterValues {FName ParameterName, float ParameterValue, ...} is what the
 // dynamic material draws. UArrayProperty.Inner and UStructProperty.Struct are at +0x78.
 Frame& f=r.f;const auto p=r.property(material,L"ScalarParameterValues");if(!p||p->inner!=L"StructProperty")return {};
 const auto values=r.array(material,p,64,L"StructProperty",p->stride);const auto inner=f.get<Address>(p->address+0x78);
 if(!values||!inner||!*inner||!f.retain(*inner))return {};const auto s=f.get<Address>(*inner+0x78);if(!s||!*s||!f.retain(*s))return {};
 const auto key=member(f,*s,L"ParameterName"),value=member(f,*s,L"ParameterValue");
 if(!key||!value||key->kind!=L"NameProperty"||key->size!=8||value->kind!=L"FloatProperty"||value->size!=4||key->offset>p->stride-8||value->offset>p->stride-4)return {};
 std::optional<float> found;
 for(int i=0;i<values->count;++i){
  const Address e=values->data+static_cast<Address>(i)*static_cast<Address>(p->stride);
  const auto index=f.get<int>(e+static_cast<Address>(key->offset)),number=f.get<int>(e+static_cast<Address>(key->offset)+4);if(!index||!number)return {};
  if(!named(f.u,*index,*number,name))continue;
  // A duplicated name has no single drawn value.
  if(found)return {};found=f.get<float>(e+static_cast<Address>(value->offset));if(!found||!std::isfinite(*found))return {};
 }
 return found;
}
std::optional<std::vector<float>> table(Reader& r,Address part,const wchar_t* name){
 // UserConstructionScript: one atlas offset per digit 0..9.
 const auto a=r.array(part,name,16,L"FloatProperty",4);if(!a||a->count!=digits)return {};
 std::vector<float> out;for(int i=0;i<a->count;++i){const auto v=r.f.get<float>(a->data+static_cast<Address>(i)*4);if(!v||!std::isfinite(*v))return {};out.push_back(*v);}return out;
}
std::optional<int> enemy_count(Reader& r,Address hud,Address world){
 // CreateSpecialHUD_Core appends each spawned Story part to StoryHUD; CreateEnemyNum spawns BP_EnemyNum.
 Frame& f=r.f;const auto list=r.array(hud,L"StoryHUD",16);if(!list)return {};std::optional<Address> part;
 for(int i=0;i<list->count;++i){
  const auto a=f.get<Address>(list->data+static_cast<Address>(i)*8);if(!a)return {};
  if(!*a||!f.u.is_a(*a,L"BP_EnemyNum_C"))continue;if(part)return {};part=*a;
 }
 // Active() redraws both digits every tick; PreState cleared means it ran in this activation.
 if(!part||!placed(r,*part,world)||settled(r,*part)!=std::optional<int>(2))return {};
 const auto root=layout(r,*part,L"Root_Num");const auto mids=r.array(*part,L"MIDNum",4);
 const auto us=table(r,*part,L"NumUArray"),vs=table(r,*part,L"NumVArray");if(!root||!mids||mids->count!=2||!us||!vs)return {};
 // ChangeNum(Index, Number) sets MIDNum[Index] Param_U/Param_V to NumUArray/NumVArray[Number]. The
 // construction script made MIDNum[0] on MS_EnemyNum_10 and MIDNum[1] on MS_EnemyNum_01; each must
 // still be the material its mesh draws. -1: the mesh is not drawn.
 const auto digit=[&](int index,const wchar_t* name)->std::optional<int>{
  const auto mesh=r.field(*part,name);if(!mesh||!*mesh||!f.u.is_a(*mesh,L"StaticMeshComponent"))return {};
  const auto shown=drawn(r,*part,mesh);if(!shown)return {};if(!*shown)return -1;
  const auto mid=f.get<Address>(mids->data+static_cast<Address>(index)*8);const auto materials=r.array(*mesh,L"OverrideMaterials",8);
  if(r.field(*mesh,L"AttachParent")!=root||!mid||!*mid||!f.retain(*mid)||!f.u.is_a(*mid,L"MaterialInstanceDynamic")||!materials||materials->count<1||f.get<Address>(materials->data)!=mid)return {};
  const auto u=scalar(r,*mid,L"Param_U"),v=scalar(r,*mid,L"Param_V");if(!u||!v)return {};
  std::optional<int> found;
  for(int n=0;n<digits;++n)if((*us)[static_cast<std::size_t>(n)]==*u&&(*vs)[static_cast<std::size_t>(n)]==*v){if(found)return {};found=n;}
  return found;
 };
 // Active hides MS_EnemyNum_10 while Remain/10 is 0, leaving the single drawn digit.
 const auto tens=digit(0,L"MS_EnemyNum_10"),ones=digit(1,L"MS_EnemyNum_01");
 if(!tens||!ones||*ones<0)return {};return (*tens>0?*tens*10:0)+*ones;
}
std::optional<int> timer_value(Reader& r,Address hud,Address world){
 // The Story HUD's BP_Timer ChildActorComponent spawns BP_Timer_Story_C (owner +0xb0, ChildActor +0x308).
 Frame& f=r.f;const auto c=r.field(hud,L"BP_Timer");if(!c||!*c||!f.u.is_a(*c,L"ChildActorComponent")||f.get<Address>(*c+0xb0)!=std::optional<Address>(hud))return {};
 const auto part=f.get<Address>(*c+0x308);if(!part||!*part||!f.u.is_a(*part,L"BP_Timer_Story_C")||!placed(r,*part,world))return {};
 const auto state=settled(r,*part);if(!state)return {};
 const auto sprite=[&](const wchar_t* name){const auto s=r.field(*part,name);return s&&*s&&f.u.is_a(*s,L"PaperSpriteComponent")?s:std::nullopt;};
 const auto tens=sprite(L"SP_Timer_10"),ones=sprite(L"SP_Timer_01"),mark=sprite(L"SP_Timer_Infinite");if(!tens||!ones||!mark)return {};
 const auto tens_drawn=drawn(r,*part,tens),ones_drawn=drawn(r,*part,ones),mark_drawn=drawn(r,*part,mark);if(!tens_drawn||!ones_drawn||!mark_drawn)return {};
 // PreEnter shows Root_Infinite or Root_Finite. Only the finite branch plays the Opacity timeline whose
 // end calls SetTickStateEnd, so an infinite timer stays in Enter.
 if(*mark_drawn){
  const auto root=layout(r,*part,L"Root_Infinite");
  return !*tens_drawn&&!*ones_drawn&&(*state==1||*state==2)&&root&&r.field(*mark,L"AttachParent")==root?std::optional<int>(infinity):std::nullopt;
 }
 // Active refreshes TimeNow from GetTimer and at once draws TimeNow/10 and TimeNow%10 on the two
 // sprites; BP_Timer_Story.PreEnter parks 999 there until then.
 if(!*tens_drawn||!*ones_drawn||*state!=2)return {};
 const auto root=layout(r,*part,L"Root_Finite");if(!root||r.field(*tens,L"AttachParent")!=root||r.field(*ones,L"AttachParent")!=root)return {};
 const auto seconds=r.integer(*part,L"TimeNow");if(!seconds||*seconds<0||*seconds>99)return {};return seconds;
}
}
void story_hud(Unreal& u,Address world,StorySnapshot& s){
 s.enemies_remaining.reset();s.time_seconds.reset();s.infinite_time=false;
 const std::function<std::optional<int>(Address)> no_widgets;Properties cache;
 // The unique drawn battle HUD root of the current world (as CombatCapture) must be the Story HUD.
 std::optional<Address> hud;
 for(const auto& root:u.instances(L"BP_HUD_Root_C")){
  Frame f{u,no_widgets,{},{},{}};Reader r{f,cache};if(!u.alive(root)||!placed(r,root.address,world))continue;
  if(hud)return;hud=root.address;
 }
 if(!hud||!u.is_a(*hud,L"BP_HUD_Story_C"))return;
 {Frame f{u,no_widgets,{},{},{}};Reader r{f,cache};const auto n=placed(r,*hud,world)?enemy_count(r,*hud,world):std::nullopt;if(n&&f.finish())s.enemies_remaining=n;}
 {Frame f{u,no_widgets,{},{},{}};Reader r{f,cache};const auto t=placed(r,*hud,world)?timer_value(r,*hud,world):std::nullopt;if(t&&f.finish()){if(*t==infinity)s.infinite_time=true;else s.time_seconds=t;}}
 if(s.enemies_remaining||s.time_seconds||s.infinite_time)s.valid=true;
}
}
