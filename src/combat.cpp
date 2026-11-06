#include "mvci/combat.hpp"
#include <algorithm>
#include <cmath>
namespace mvci {
namespace {
// Static proofs: research/combat report.md (native) and ui-report.md §9 (HUD Blueprints).
constexpr Address engine_rva=0x36bc348,hud_rva=0x3476150,hud_vtable=0x28eec88,hud_ptr_rva=0x34760f0;
constexpr Address status_ptr_rva=0x34730a8,status_vtable=0x28ec0e0,cine_rva=0x33e219c;
constexpr int flow_ready=3,flow_now=4,tick_active=2;
constexpr Address roster_order=0x584,fighter_stride=0xe8;
constexpr std::uint64_t rescan_ms=500;
constexpr double coordinate_limit=100000;
bool clean(const std::optional<ObjectRef>& o){return o&&!(o->flags&0x30);}
}

bool CombatCapture::in_world(Address actor,Address world)const{
 // AActor's Outer is its ULevel; ULevel.OwningWorld (+0xb0) is the world it is loaded into.
 const auto level=get<Address>(actor+0x20);if(!level||!unreal_.is_a(*level,L"Level"))return false;
 const auto owning=get<Address>(*level+0xb0);return owning&&*owning==world;
}
bool CombatCapture::shown_actor(Address actor,Address world)const{
 return clean(unreal_.object(actor))&&unreal_.boolean(actor,L"bHidden")==std::optional<bool>(false)&&in_world(actor,world);
}
bool CombatCapture::root_ok(const ObjectRef& root,Address world)const{
 return unreal_.alive(root)&&unreal_.is_a(root.address,L"BP_HUD_Root_C")&&shown_actor(root.address,world);
}
std::optional<ObjectRef> CombatCapture::find_root(Address world,std::uint64_t now){
 const auto count=unreal_.object_count();if(!count)return {};
 const bool rescan=unreal_.shared_discovery()||!scanned_||*count!=scanned_count_||now<scanned_at_||now-scanned_at_>=rescan_ms;
 if(root_&&root_ok(*root_,world)&&!rescan)return root_;
 root_.reset();
 if(!rescan)return {};
 scanned_=true;scanned_at_=now;scanned_count_=*count;
 std::optional<ObjectRef> found;
 for(const auto& o:unreal_.instances(L"BP_HUD_Root_C")){
  if(!root_ok(o,world))continue;
  if(found)return {}; // Two drawn HUD roots in one world: no single owner of the display.
  found=o;
 }
 root_=found;return root_;
}
bool CombatCapture::visible_component(Address owner,const wchar_t* name)const{
 const auto component=unreal_.object_field(owner,name);
 if(!component||!*component||!clean(unreal_.object(*component))||!unreal_.is_a(*component,L"SceneComponent"))return false;
 return get<Address>(*component+0xb0)==std::optional<Address>(owner)&&
  unreal_.boolean(*component,L"bVisible")==std::optional<bool>(true)&&
  unreal_.boolean(*component,L"bHiddenInGame")==std::optional<bool>(false);
}
std::optional<Address> CombatCapture::child(Address owner,const wchar_t* component,const wchar_t* cls,Address world)const{
 // The HUD binds its parts through UChildActorComponent (owner +0xb0, ChildActor +0x308).
 const auto c=unreal_.object_field(owner,component);if(!c||!*c||!clean(unreal_.object(*c))||!unreal_.is_a(*c,L"ChildActorComponent"))return {};
 const auto o=get<Address>(*c+0xb0);if(!o||*o!=owner)return {};
 const auto actor=get<Address>(*c+0x308);
 if(!actor||!clean(unreal_.object(*actor))||!unreal_.is_a(*actor,cls)||!in_world(*actor,world))return {};
 return actor;
}
std::optional<Address> CombatCapture::part(Address owner,const wchar_t* component,const wchar_t* cls,int team,Address world)const{
 const auto p=child(owner,component,cls,world);if(!p||!shown_actor(*p,world))return {};
 if(unreal_.integer(*p,L"TeamNo")!=std::optional<int>(team)||unreal_.boolean(*p,L"IsActivateEnable")!=std::optional<bool>(true))return {};
 // Active and refreshed at least once: State==Active and PreState cleared by the first Tick_Active.
 const auto tick=unreal_.object_field(*p,L"TickStateObj");if(!tick||!*tick||!unreal_.is_a(*tick,L"BP_TickStateObj_C"))return {};
 if(unreal_.object_field(*tick,L"ParentObj")!=std::optional<Address>(*p))return {};
 if(unreal_.integer(*tick,L"State")!=std::optional<int>(tick_active)||unreal_.boolean(*tick,L"PreState")!=std::optional<bool>(false))return {};
 return p;
}
void CombatCapture::hyper(Address root,int team,Address world,FighterSnapshot& out)const{
 const auto p=part(root,team?L"BP_spacon_2P":L"BP_spacon_1P",L"BP_Spacon_C",team,world);
 if(!p||!visible_component(*p,L"MS_Spacon_Gauge")||!visible_component(*p,L"MS_Spacon_Number"))return;
 const auto level=unreal_.integer(*p,L"GaugeLevel");const auto value=unreal_.number(*p,L"GaugeValue");const auto max=unreal_.boolean(*p,L"GaugeMaxExist");
 const auto limit=unreal_.integer(*p,L"GaugeLevelMax");
 if(!level||!value||!max||limit!=std::optional<int>(4)||*level<0||*level>4||*value<0||*value>1||(*max&&*level!=4))return;
 // At MAX the digit already shows the full count and the bar is pinned at 1.0.
 const double bars=*max?*level:*level+*value;
 if(bars<=4)out.hyper_bars=bars;
}
void CombatCapture::infinity(Address root,int team,Address world,FighterSnapshot& out)const{
 const auto ctrl=child(root,L"BP_GemCtrl",L"BP_GemCtrl_C",world);if(!ctrl)return;
 const auto p=part(*ctrl,team?L"BP_GemGauge_2P":L"BP_GemGauge_1P",L"BP_GemGauge_C",team,world);
 if(!p||!visible_component(*p,L"MS_Gauge"))return;
 const auto kind=unreal_.integer(*p,L"GemGaugeKind");const auto value=unreal_.number(*p,L"VerGemGauge");const auto used=unreal_.boolean(*p,L"IsUsed");
 if(!kind||*kind<0||*kind>5||!value||*value<0||*value>1||!used)return;
 const double drawn=std::round(*value*100)/100;
 out.infinity_fraction=drawn;out.storm_active=*used;
 // The ready glow (ConvValueType) starts at half charge and is not drawn during Storm.
 out.infinity_ready=!*used&&*value>=0.5;
}
std::optional<CombatCapture::FighterRecord> CombatCapture::active_fighter(Address hud,int team)const{
 const auto slot=get<std::int8_t>(hud+roster_order+static_cast<Address>(team)*4);
 if(!slot||*slot<0||*slot>2)return {};
 const Address r=hud+static_cast<Address>(team+2*(*slot))*fighter_stride;
 const auto record_team=get<std::int32_t>(r+0x10);const auto chara=get<std::uint8_t>(r+0x18);
 if(record_team!=std::optional<std::int32_t>(team)||!chara||*chara>=47)return {};
 return FighterRecord{r,*slot,*chara};
}

void CombatCapture::health(Address root,Address hud,int team,Address world,const FighterRecord& fighter,FighterSnapshot& out)const{
 if(fighter.slot>1)return;
 const auto field=team?L"BP_Vital_2P":L"BP_Vital_1P";
 const auto attachment=unreal_.object_field(root,field);
 if(!attachment||!*attachment||!visible_component(root,field))return;
 const auto vital=part(root,field,L"BP_Vital_C",team,world);
 // This shipped Blueprint property has a leading space. ChangeChara updates
 // it when the HUD swaps the primary and reserve bar meshes.
 if(!vital||unreal_.integer(*vital,L" MainId")!=std::optional<int>(fighter.slot))return;
 const auto ref=unreal_.object(*vital);
 const auto holder_field=fighter.slot?L"Root_B":L"Root_A";
 const auto mesh_field=fighter.slot?L"MS_Vital_Gauge_B":L"MS_Vital_Gauge_A";
 const auto holder=unreal_.object_field(*vital,holder_field), mesh=unreal_.object_field(*vital,mesh_field);
 if(!ref||!holder||!*holder||!mesh||!*mesh||unreal_.object_field(*mesh,L"AttachParent")!=holder||
    !visible_component(*vital,holder_field)||!visible_component(*vital,mesh_field))return;

 const auto ratios=unreal_.property(*vital,L"VitalGauge");
 if(!ratios||ratios->inner!=L"FloatProperty"||ratios->stride!=4)return;
 const auto values=unreal_.array(*vital,*ratios,2);
 if(!values||values->count!=2)return;
 const Address fill_address=values->data+static_cast<Address>(fighter.slot)*4;
 const auto fill=get<float>(fill_address);
 if(!fill||!std::isfinite(*fill)||*fill<0||*fill>1)return;

 // DK_ChangeVital copies the result of GetPlLifeRateForEach (RVA 446d60)
 // to VitalGauge and the fill material together. Require a current matching
 // HUD record so reset values and partially refreshed tags stay unavailable.
 const auto current=get<int>(fighter.address+0x80), maximum=get<int>(fighter.address+0x84);
 if(!current||!maximum||*maximum<=0||*current<0||*current>*maximum||
    get<std::uint8_t>(fighter.address+0x38)!=std::optional<std::uint8_t>(std::uint8_t{1})||
    get<std::uint8_t>(fighter.address+0x43)!=std::optional<std::uint8_t>(std::uint8_t{0}))return;
 const float rate=static_cast<float>(*current)/static_cast<float>(*maximum);
 if((*fill==0)!=(*current==0)||std::abs(*fill-rate)>0.000001f)return;

 const auto selected=active_fighter(hud,team);
 const auto latest_values=unreal_.array(*vital,*ratios,2);
 if(!selected||selected->address!=fighter.address||selected->character!=fighter.character||
    !unreal_.alive(*ref)||unreal_.integer(*vital,L" MainId")!=std::optional<int>(fighter.slot)||
    unreal_.object_field(root,field)!=attachment||get<Address>(*attachment+0x308)!=vital||
    !latest_values||latest_values->data!=values->data||latest_values->count!=values->count||
    get<float>(fill_address)!=fill||unreal_.current_world()!=std::optional<Address>(world))return;
 out.health_percent=static_cast<double>(*fill)*100.0;
 out.health_fighter=fighter.slot*256+fighter.character;
}

std::optional<CombatCapture::Point> CombatCapture::point(const FighterRecord& fighter)const{
 const Address r=fighter.address;
 const auto valid=get<std::uint8_t>(r+0x38),vanish=get<std::uint8_t>(r+0x39),offscreen=get<std::uint8_t>(r+0x3c),active=get<std::uint8_t>(r+0x42),unused=get<std::uint8_t>(r+0x43);
 if(!valid||*valid!=1||!vanish||*vanish||!offscreen||*offscreen||!active||!*active||!unused||*unused)return {};
 const auto x=get<float>(r+0x44),height=get<float>(r+0x4c);
 if(!x||!height||!std::isfinite(*x)||!std::isfinite(*height)||std::abs(*x)>coordinate_limit||std::abs(*height)>coordinate_limit)return {};
 return Point{*x,*height};
}

CombatSnapshot CombatCapture::sample(std::uint64_t now,bool menu_visible){
 CombatSnapshot s;const Address base=unreal_.base(),hud=base+hud_rva;
 const auto hud_vt=get<Address>(hud),hud_ptr=get<Address>(base+hud_ptr_rva),status=get<Address>(base+status_ptr_rva);
 const auto status_vt=status&&*status?get<Address>(*status):std::nullopt;
 const auto engine=get<Address>(base+engine_rva);
 const auto viewport=engine&&*engine?get<Address>(*engine+0x5d8):std::nullopt;
 const auto world=viewport&&*viewport?get<Address>(*viewport+0x80):std::nullopt;
 const auto flow=get<std::uint8_t>(hud+0x5bc);
 if(hud_vt!=std::optional<Address>(base+hud_vtable)||hud_ptr!=std::optional<Address>(hud)||status_vt!=std::optional<Address>(base+status_vtable)||
    !world||!*world||!clean(unreal_.object(*world))||!unreal_.is_a(*world,L"World")||!flow){
  // No battle: the next confirmed battle is a new session.
  root_.reset();identity_=false;round_=false;played_=false;last_flow_=-1;reference_=0;return s;
 }
 if((last_flow_>0&&*flow==0)||(played_&&*flow<=flow_ready))round_=true;
 last_flow_=*flow;
 const auto root=find_root(*world,now);if(!root)return s;
 if(!identity_||round_||world_!=*world||battle_!=*status||session_root_.address!=root->address||session_root_.index!=root->index||session_root_.serial!=root->serial){
  ++session_;identity_=true;round_=false;played_=false;world_=*world;battle_=*status;session_root_=*root;reference_=0;
 }
 if(*flow==flow_now)played_=true;
 s.valid=true;s.session=session_;

 bool paused=true;
 if(const auto instance=get<Address>(*engine+0xc80);instance&&*instance&&unreal_.is_a(*instance,L"DuckGameInstance")){
  const auto pause=get<std::uint8_t>(*instance+0xe8);
  const auto level=get<Address>(*world+0x30);
  const auto settings=level&&*level?get<Address>(*level+0x2e0):std::nullopt;
  const auto pauser=settings&&*settings?get<Address>(*settings+0x500):std::nullopt;
  if(pause&&pauser)paused=*pause||*pauser;
 }
 const auto cine=get<std::int32_t>(base+cine_rva);
 s.cinematic=!cine||*cine!=0;
 const bool fighting=*flow==flow_ready||*flow==flow_now;
 s.playable=*flow==flow_now&&!paused&&!menu_visible;
 if(!fighting)return s;
 const std::array active{active_fighter(hud,0),active_fighter(hud,1)};
 if(unreal_.integer(root->address,L"CurrentState")==std::optional<int>(*flow)){
  for(int team=0;team<2;++team){
   auto& fighter=s.fighters[team];
   hyper(root->address,team,*world,fighter);infinity(root->address,team,*world,fighter);
   if(active[team])health(root->address,hud,team,*world,*active[team],fighter);
  }
 }
 if(s.cinematic||paused||menu_visible)return s;
 const auto p1=active[0]?point(*active[0]):std::nullopt,p2=active[1]?point(*active[1]):std::nullopt;
 std::optional<double> distance;
 if(p1&&p2){const double d=std::hypot(p1->x-p2->x,p1->height-p2->height);if(std::isfinite(d))distance=d;}
 // The opening separation of the round (first valid READY/NOW sample) is the scale; it is never re-fit mid-round.
 if(reference_<=0&&distance&&*distance>=1)reference_=*distance;
 if(!s.playable)return s;
 if(p1)s.fighters[0].x=p1->x;
 if(p2)s.fighters[1].x=p2->x;
 if(distance&&reference_>0)s.normalized_distance=std::min(1.0,*distance/reference_);
 return s;
}
}
