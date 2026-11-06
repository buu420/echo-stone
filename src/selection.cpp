#include "mvci/selection.hpp"
#include "mvci/localization.hpp"
#include <algorithm>
#include <cmath>
#include <set>
namespace mvci {
namespace {
// GEngine (research/combat report.md): +0x5d8 GameViewport, +0x80 World.
// g_GameSetting and its CharaAttr table: research/selection/ghidra/VERIFIED.md (exec 0x528af0 / 0x525740).
constexpr Address engine_rva=0x36bc348,game_setting_rva=0x348cb48,names_rva=0x35cafd0;
constexpr int tick_active=2,tick_hidden_state=0,max_table=1024;
constexpr std::uint64_t rescan_ms=500;
// BP_SelectGem.Initialize builds MT_ArrGemName in PLAYER1_GemType.ArrText order.
constexpr const wchar_t* stone_material[6]{L"MT_Space",L"MT_Power",L"MT_Reality",L"MT_Soul",L"MT_Mind",L"MT_Time"};
bool supported(const std::wstring& t){return !t.empty()&&t.rfind(L"msg_",0)!=0;}
// Button glyph materials /Game/UI/BaseAsset/Platform/<platform>/MI_Btn_<EButtonType> (FC_UI_Object.GetButtonMaterial).
// Only the letter/shoulder/trigger/stick names are spoken; WD_KCbuttonParts.setupButton itself keys icon size off
// RS/LS/RT/LT/RB/LB in the material name. Other glyphs (Dir, Sti, SP_L, NA, ...) are atlas cells not decoded here.
constexpr const wchar_t* glyph_names[10]{L"A",L"B",L"X",L"Y",L"LB",L"RB",L"LT",L"RT",L"LS",L"RS"};
// WD_KeyConf/WD_KeyboardConf.changeFocus_Main and ChangeFocus_Sub: ButtonFocusID -> focused part (main 0..17, sub 20..).
constexpr const wchar_t* key_main[18]{L"WD_KCbuttonParts5_ToOperation",L"WD_KCbuttonParts_P1",L"WD_KCbuttonParts_K1",L"WD_KCbuttonParts_P2",L"WD_KCbuttonParts_K2",
 L"WD_KCbuttonParts_Stone",L"WD_KCbuttonParts_Change",L"WD_KCbuttonParts2_Burst",L"WD_KCbuttonParts2_P1P2",L"WD_KCbuttonParts2_K1K2",L"WD_KCbuttonParts2_P1K1",
 L"WD_KCbuttonParts2_P2K2",L"WDKCbuttonParts_Taunt",L"WD_KCbuttonParts2_PlayOrSChange",L"WDKCbuttonParts_Rec",L"WD_KCbuttonParts_DMC2",L"WD_KCbuttonParts3_Decide",L"WD_KCbuttonParts3_Reset"};
constexpr const wchar_t* pad_sub[6]{L"WD_KCbuttonParts4_AutoCombo",L"WD_KCbuttonParts4_EasyHC",L"WD_KCbuttonParts4_AutoSJump",L"WD_KCbuttonParts4_PauseType",L"WD_KCbuttonParts3_SubExit",L"WD_KCbuttonParts3_SubReset"};
constexpr const wchar_t* keyboard_sub[10]{L"WD_KCbuttonParts4_AutoCombo",L"WD_KCbuttonParts4_EasyHC",L"WD_KCbuttonParts4_AutoSJump",L"WD_KCbuttonParts4_PauseType",
 L"WD_KCbuttonParts_UP",L"WD_KCbuttonParts_DOWN",L"WD_KCbuttonParts_LEFT",L"WD_KCbuttonParts_RIGHT",L"WD_KCbuttonParts3_SubExit",L"WD_KCbuttonParts3_SubReset"};
// WD_KeyboardConf.ConvertFocusIDToArrayIndex: ButtonFocusID -> KeyboardConfigData index (-1: row has no key).
constexpr int keyboard_main_index[18]{-1,4,6,5,7,8,9,14,10,11,12,13,17,16,15,18,-1,-1};
constexpr int keyboard_sub_index[10]{-1,-1,-1,-1,0,1,2,3,-1,-1};
std::wstring side(int team){return team?tr(L"Player 2"):tr(L"Player 1");} // Mod-owned tracking context (F5).
void sentence(std::wstring& out,const std::wstring& part){
 if(part.empty())return;
 if(!out.empty()){const wchar_t last=out.back();out+=(last==L'.'||last==L'!'||last==L'?')?L" ":L". ";}
 out+=part;
}
}

const std::vector<ObjectRef>& SelectionCapture::cached(Cache& c,const wchar_t* cls,std::uint64_t now)const{
 // Rescan on a new registry count, a retired (dead) cached object, or every rescan_ms.
 const bool retired=std::any_of(c.objects.begin(),c.objects.end(),[this](const ObjectRef& o){return !unreal_.alive(o);});
 if(unreal_.shared_discovery()||!c.filled||c.count!=count_||retired||now<c.at||now-c.at>=rescan_ms){c.count=count_;c.objects=unreal_.instances(cls);c.at=now;c.filled=true;}
 return c.objects;
}
SelectionCapture::Root SelectionCapture::unique(const std::vector<ObjectRef>& list,const std::function<bool(Address)>& valid)const{
 Root r;
 for(const auto& o:list){
  if(!unreal_.alive(o)||!valid(o.address))continue;
  if(r.address){r.address.reset();r.ambiguous=true;return r;} // Never pick an arbitrary first candidate.
  r.address=o.address;
 }
 return r;
}
bool SelectionCapture::clean(Address a)const{auto o=unreal_.object(a);return o&&!(o->flags&0x30);}
bool SelectionCapture::in_world(Address actor,Address world)const{
 // AActor's Outer is its ULevel; ULevel.OwningWorld (+0xb0) must be the viewport's current world.
 const auto level=get<Address>(actor+0x20);if(!level||!*level||!unreal_.is_a(*level,L"Level"))return false;
 const auto owning=get<Address>(*level+0xb0);return owning&&*owning==world;
}
bool SelectionCapture::shown_actor(Address actor,Address world)const{
 return clean(actor)&&unreal_.is_a(actor,L"Actor")&&unreal_.boolean(actor,L"bHidden")==std::optional<bool>(false)&&in_world(actor,world);
}
bool SelectionCapture::active(Address owner,bool prestate)const{
 // Input owners must be Active. PreState must have cleared through their
 // implemented callback; callers pass false for owners without IF_UI_Active.
 const auto tick=unreal_.object_field(owner,L"TickStateObj");if(!tick||!*tick||!clean(*tick)||unreal_.type(*tick)!=L"BP_TickStateObj_C")return false;
 if(unreal_.object_field(*tick,L"ParentObj")!=std::optional<Address>(owner))return false;
 if(unreal_.integer(*tick,L"State")!=std::optional<int>(tick_active))return false;
 return !prestate||unreal_.boolean(*tick,L"PreState")==std::optional<bool>(false);
}
bool SelectionCapture::preview_ready(Address screen)const{
 // BP_Screen is a passive portrait, not an input owner. After Stage Select
 // returns, the live P1 portrait remains Enter (1), PreState=false while its
 // character is visibly drawn and matches the grid. Do not wait for Active (2).
 // The caller still requires active root/grid input ownership, shown actor and
 // both shown components, matching team and tile/preview character identities.
 if(!unreal_.is_a(screen,L"BP_Screen_C"))return false;
 const auto tick=unreal_.object_field(screen,L"TickStateObj");if(!tick||!*tick||!clean(*tick)||unreal_.type(*tick)!=L"BP_TickStateObj_C")return false;
 if(unreal_.object_field(*tick,L"ParentObj")!=std::optional<Address>(screen))return false;
 const auto state=unreal_.integer(*tick,L"State");
 return state&&(*state==1||*state==tick_active)&&unreal_.boolean(*tick,L"PreState")==std::optional<bool>(false);
}
bool SelectionCapture::tick_hidden(Address owner)const{
 const auto tick=unreal_.object_field(owner,L"TickStateObj");if(!tick||!*tick||!clean(*tick)||unreal_.type(*tick)!=L"BP_TickStateObj_C")return true;
 if(unreal_.object_field(*tick,L"ParentObj")!=std::optional<Address>(owner))return true;
 const auto state=unreal_.integer(*tick,L"State");return !state||*state==tick_hidden_state;
}
std::wstring SelectionCapture::tick_desc(Address owner)const{
 const auto tick=unreal_.object_field(owner,L"TickStateObj");if(!tick||!*tick||!clean(*tick))return L"no tick object";
 const auto state=unreal_.integer(*tick,L"State");const auto pre=unreal_.boolean(*tick,L"PreState");
 return L"tick "+(state?std::to_wstring(*state):std::wstring(L"?"))+L", prestate "+(pre?std::wstring(*pre?L"1":L"0"):std::wstring(L"?"));
}
bool SelectionCapture::component_shown(Address c,Address owner)const{
 if(!c||!clean(c)||!unreal_.is_a(c,L"SceneComponent"))return false;
 const auto o=get<Address>(c+0xb0);
 return o&&*o==owner&&unreal_.boolean(c,L"bVisible")==std::optional<bool>(true)&&unreal_.boolean(c,L"bHiddenInGame")==std::optional<bool>(false);
}
bool SelectionCapture::widget_shown(Address w)const{return w&&clean(w)&&unreal_.shown(w,visibility_);}
bool SelectionCapture::locally_shown(Address a,Address root)const{
 // 3D WidgetComponent widgets are never in the viewport: walk ancestors only up to the bound root widget.
 std::set<Address> seen;
 while(a&&seen.size()<64&&seen.insert(a).second){
  if(!clean(a)||!unreal_.is_a(a,L"Widget"))return false;
  const auto visibility=visibility_?visibility_(a):unreal_.integer(a,L"Visibility");
  if(!visibility||*visibility<0||*visibility>4||*visibility==1||*visibility==2)return false;
  if(auto color=unreal_.property(a,L"ColorAndOpacity");color&&color->kind==L"StructProperty"&&color->size==16){const auto alpha=get<float>(a+color->offset+12);if(!alpha||!std::isfinite(*alpha)||!(*alpha>0))return false;}
  if(a==root)return true;
  const auto slot=get<Address>(a+0x30);if(!slot)return false;
  if(*slot){if(!unreal_.object(*slot))return false;const auto parent=get<Address>(*slot+0x28);if(!parent)return false;if(*parent){a=*parent;continue;}}
  const auto outer=get<Address>(a+0x20);if(!outer||!*outer)return false;
  if(unreal_.type(*outer)==L"WidgetTree"){const auto owner=get<Address>(*outer+0x20);if(!owner)return false;a=*owner;}
  else if(unreal_.is_a(*outer,L"UserWidget"))a=*outer;
  else return false;
 }
 return false;
}
std::optional<std::wstring> SelectionCapture::fname(Address at)const{
 // FName {ComparisonIndex,Number} resolved through the same name table as Unreal::name (GNames 0x35cafd0).
 const auto index=get<int>(at),number=get<int>(at+4);if(!index||!number||*index<0||*index>=1048576||*number<0||*number>10000000)return {};
 const auto table=get<Address>(unreal_.base()+names_rva);if(!table||!*table)return {};
 const auto chunk=get<Address>(*table+static_cast<Address>(*index/16384)*8);if(!chunk||!*chunk)return {};
 const auto entry=get<Address>(*chunk+static_cast<Address>(*index%16384)*8);if(!entry||!*entry)return {};
 const auto flags=get<std::uint32_t>(*entry);if(!flags)return {};
 std::wstring value;
 for(std::size_t i=0;i<256;++i){
  std::uint16_t c=0;
  if(*flags&1){const auto w=get<std::uint16_t>(*entry+0x10+i*2);if(!w)return {};c=*w;}
  else{const auto b=get<std::uint8_t>(*entry+0x10+i);if(!b)return {};c=*b;}
  if(!c){if(*number)value+=L"_"+std::to_wstring(*number-1);return value;}
  value.push_back(static_cast<wchar_t>(c));
 }
 return {};
}
std::optional<SelectionCapture::Member> SelectionCapture::member(Address s,const wchar_t* name)const{
 // UScriptStruct members: UStruct.Children +0x38, UField.Next +0x28, UProperty offset +0x50 / size +0x34.
 if(!s||!clean(s))return {};
 auto field=get<Address>(s+0x38);std::set<Address> seen;
 while(field&&*field&&seen.size()<512&&seen.insert(*field).second){
  if(!unreal_.object(*field))return {};
  if(unreal_.name_is(*field,name)){
   const auto offset=get<int>(*field+0x50),size=get<int>(*field+0x34);if(!offset||!size||*offset<0||*size<=0||*offset>0x10000)return {};
   return Member{*offset,*size,unreal_.type(*field)};
  }
  field=get<Address>(*field+0x28);
 }
 return {};
}
std::optional<Address> SelectionCapture::struct_of(Address property,bool array)const{
 // UStructProperty.Struct and UArrayProperty.Inner are both at +0x78.
 auto p=property;if(array){const auto inner=get<Address>(p+0x78);if(!inner||!*inner||!unreal_.object(*inner)||unreal_.type(*inner)!=L"StructProperty")return {};p=*inner;}
 const auto s=get<Address>(p+0x78);if(!s||!*s||!clean(*s))return {};return s;
}
std::optional<Address> SelectionCapture::brush_resource(Address image)const{
 if(!unreal_.is_a(image,L"Image"))return {};
 const auto p=unreal_.property(image,L"Brush");if(!p||p->kind!=L"StructProperty")return {};
 const auto s=struct_of(p->address,false);if(!s)return {};
 const auto m=member(*s,L"ResourceObject");if(!m||m->kind!=L"ObjectProperty"||m->size!=8||m->offset+8>p->size)return {};
 const auto v=get<Address>(image+static_cast<Address>(p->offset+m->offset));if(!v||!*v||!clean(*v))return {};
 return v;
}
std::optional<Address> SelectionCapture::parameter(Address material,const wchar_t* field,const wchar_t* name,const wchar_t* kind)const{
 // UMaterialInstance.{Scalar,Texture}ParameterValues: FName ParameterName + ParameterValue (UE4.15 layout via reflection).
 if(!unreal_.is_a(material,L"MaterialInstance"))return {};
 const auto p=unreal_.property(material,field);if(!p||p->inner!=L"StructProperty")return {};
 const auto arr=unreal_.array(material,*p,128);const auto s=struct_of(p->address,true);if(!arr||!s)return {};
 const auto n=member(*s,L"ParameterName"),v=member(*s,L"ParameterValue");
 if(!n||!v||n->kind!=L"NameProperty"||v->kind!=kind||n->offset+8>p->stride||v->offset+v->size>p->stride)return {};
 std::optional<Address> found;
 for(int i=0;i<arr->count;++i){
  const Address e=arr->data+static_cast<Address>(i)*static_cast<Address>(p->stride);
  if(fname(e+static_cast<Address>(n->offset))!=std::optional<std::wstring>(name))continue;
  if(found)return {}; // Duplicate parameter names: no single drawn value.
  found=e+static_cast<Address>(v->offset);
 }
 return found;
}
std::wstring SelectionCapture::glyph(Address image)const{
 const auto resource=brush_resource(image);if(!resource)return tr(L"(button icon)");
 std::wstring n=unreal_.name(*resource,true);
 if(unreal_.is_a(*resource,L"MaterialInstanceDynamic")){const auto parent=unreal_.object_field(*resource,L"Parent");if(!parent||!*parent)return tr(L"(button icon)");n=unreal_.name(*parent,true);}
 const std::wstring prefix=L"MI_Btn_";
 if(n.rfind(prefix,0)!=0)return tr(L"(button icon)");
 const auto suffix=n.substr(prefix.size());
 for(const auto g:glyph_names)if(suffix==g)return suffix;
 return tr(L"(button icon)");
}
std::optional<std::wstring> SelectionCapture::line_text(Address box)const{
 // WD_KeyConfWaiting.SetHorizontalBox fills each box with spawned TextBlocks and button Images (PanelWidget.Slots[i].Content).
 if(!box||!unreal_.is_a(box,L"PanelWidget")||!widget_shown(box))return {};
 const auto n=count(box,L"Slots",L"ObjectProperty",8,64);if(!n)return {};
 std::wstring out;
 for(int i=0;i<*n;++i){
  const auto slot=element(box,L"Slots",i,64);if(!slot)return {};if(!*slot)continue;
  const auto content=unreal_.object_field(*slot,L"Content");if(!content||!*content||!widget_shown(*content))continue;
  std::wstring piece;
  if(unreal_.is_a(*content,L"TextBlock")){const auto t=unreal_.text_field(*content,L"Text");if(!t)return {};if(t->rfind(L"msg_",0)==0)return {};piece=*t;}
  else if(unreal_.is_a(*content,L"Image"))piece=glyph(*content);
  if(piece.empty())continue;
  if(!out.empty())out+=L" ";
  out+=piece;
 }
 if(out.empty())return {};return out;
}
std::optional<int> SelectionCapture::portrait(Address image)const{
 // WD_SelectTeamList.SetCharacterIcon: Image dynamic material 'TEX' = /Game/Chara/<hex PlType>/UI_{N,AR1,AR2}/HUD/<n>P/TEX_<color>
 // (FC_UI_Object.GetCharaTexPath_HUD). The drawn texture's package path names the character; nothing is executed.
 if(!widget_shown(image))return {};
 const auto resource=brush_resource(image);if(!resource||!unreal_.is_a(*resource,L"MaterialInstanceDynamic"))return {};
 const auto at=parameter(*resource,L"TextureParameterValues",L"TEX",L"ObjectProperty");if(!at)return {};
 const auto texture=get<Address>(*at);if(!texture||!*texture||!clean(*texture))return {};
 const auto package=get<Address>(*texture+0x20);if(!package||!*package||!unreal_.object(*package))return {};
 const std::wstring path=unreal_.name(*package),prefix=L"/Game/Chara/";
 if(path.rfind(prefix,0)!=0||path.size()<prefix.size()+4)return {};
 int value=0;
 for(std::size_t i=prefix.size();i<prefix.size()+3;++i){const wchar_t c=path[i];int d=-1;if(c>=L'0'&&c<=L'9')d=c-L'0';else if(c>=L'A'&&c<=L'F')d=c-L'A'+10;if(d<0)return {};value=value*16+d;}
 const auto rest=path.substr(prefix.size()+3);
 if(rest.rfind(L"/UI_N/HUD/",0)!=0&&rest.rfind(L"/UI_AR1/HUD/",0)!=0&&rest.rfind(L"/UI_AR2/HUD/",0)!=0)return {};
 return value;
}
std::optional<Address> SelectionCapture::child(Address owner,const wchar_t* component,const wchar_t* cls,Address world)const{
 // UChildActorComponent: owner +0xb0, ChildActor +0x308.
 const auto c=unreal_.object_field(owner,component);if(!c||!*c||!clean(*c)||!unreal_.is_a(*c,L"ChildActorComponent"))return {};
 const auto o=get<Address>(*c+0xb0);if(!o||*o!=owner)return {};
 const auto actor=get<Address>(*c+0x308);if(!actor||!*actor||!clean(*actor)||!unreal_.is_a(*actor,cls)||!in_world(*actor,world))return {};
 return actor;
}
std::optional<int> SelectionCapture::count(Address owner,const wchar_t* field,const wchar_t* inner,int stride,int limit)const{
 const auto p=unreal_.property(owner,field);if(!p||p->inner!=inner||p->stride!=stride)return {};
 const auto arr=unreal_.array(owner,*p,limit);if(!arr)return {};return arr->count;
}
std::optional<Address> SelectionCapture::element(Address owner,const wchar_t* field,int index,int limit)const{
 const auto p=unreal_.property(owner,field);if(!p||p->inner!=L"ObjectProperty"||p->stride!=8)return {};
 const auto arr=unreal_.array(owner,*p,limit);if(!arr||index<0||index>=arr->count)return {};
 const auto v=get<Address>(arr->data+static_cast<Address>(index)*8);if(!v||(*v&&!unreal_.object(*v)))return {};
 return v;
}
std::optional<int> SelectionCapture::int_element(Address owner,const wchar_t* field,int index,int limit)const{
 const auto p=unreal_.property(owner,field);if(!p||p->inner!=L"IntProperty"||p->stride!=4)return {};
 const auto arr=unreal_.array(owner,*p,limit);if(!arr||index<0||index>=arr->count)return {};
 return get<int>(arr->data+static_cast<Address>(index)*4);
}
std::optional<std::wstring> SelectionCapture::rendered(Address owner,const wchar_t* field)const{
 const auto b=unreal_.object_field(owner,field);if(!b||!*b||!unreal_.is_a(*b,L"TextBlock")||!widget_shown(*b))return {};
 auto t=unreal_.text_field(*b,L"Text");if(!t||!supported(*t))return {};return t;
}
std::optional<std::wstring> SelectionCapture::rendered_local(Address owner,const wchar_t* field,Address root)const{
 const auto b=unreal_.object_field(owner,field);if(!b||!*b||!unreal_.is_a(*b,L"TextBlock")||!locally_shown(*b,root))return {};
 auto t=unreal_.text_field(*b,L"Text");if(!t||!supported(*t))return {};return t;
}
std::optional<std::wstring> SelectionCapture::chara_name(int player_type)const{
 // GameSetting::GetCharaAttrData: TArray<UDuckCharaAttrDataAsset*> data +0x28, Num +0x30, bounds-checked.
 const auto gs=get<Address>(unreal_.base()+game_setting_rva);if(!gs||!*gs||!clean(*gs)||!unreal_.is_a(*gs,L"GameSetting"))return {};
 const auto data=get<Address>(*gs+0x28);const auto num=get<int>(*gs+0x30),capacity=get<int>(*gs+0x34);
 if(!data||!num||!capacity||*num<0||*num>max_table||*capacity<*num||player_type<0||player_type>=*num)return {};
 const auto asset=get<Address>(*data+static_cast<Address>(player_type)*8);
 if(!asset||!*asset||!clean(*asset)||!unreal_.is_a(*asset,L"DuckCharaAttrDataAsset"))return {};
 if(unreal_.integer(*asset,L"CharaID")!=std::optional<int>(player_type))return {};
 auto name=unreal_.text_field(*asset,L"CharaName");if(!name||!supported(*name))return {};return name;
}
std::optional<std::wstring> SelectionCapture::stone_name(int index,std::uint64_t now){
 if(index<0||index>=6)return {};
 std::optional<Address> item;bool ambiguous=false;
 for(const auto& o:cached(gem_items_,L"UIMenuItemDataAsset",now)){
  if(!unreal_.alive(o)||unreal_.name(o.address,true)!=L"PLAYER1_GemType")continue;
  if(item){ambiguous=true;break;}item=o.address;
 }
 if(item&&!ambiguous){
  const auto p=unreal_.property(*item,L"ArrText");
  if(p&&p->inner==L"TextProperty"&&p->stride==24){
   const auto arr=unreal_.array(*item,*p,16);
   if(arr&&arr->count==6){auto t=unreal_.text(arr->data+static_cast<Address>(index)*24);if(t&&supported(*t))return t;}
  }
 }
 // The focused material proves the visible stone even when the text asset is unavailable.
 switch(index){
 case 0:return tr(L"Space Stone");
 case 1:return tr(L"Power Stone");
 case 2:return tr(L"Reality Stone");
 case 3:return tr(L"Soul Stone");
 case 4:return tr(L"Mind Stone");
 case 5:return tr(L"Time Stone");
 default:return {};
 }
}

std::optional<MenuSnapshot> SelectionCapture::grid(Address root,Address parent,Address world,int team)const{
 const auto uid=int_element(parent,L"FocusUniqId",team);if(!uid)return reject(L"grid: focus index unreadable");
 const auto tile=element(parent,L"ArrSelectChild",*uid);
 if(!tile||!*tile||!unreal_.is_a(*tile,L"BP_Select_Child_C")||!shown_actor(*tile,world)||unreal_.integer(*tile,L"UniqId")!=uid)return reject(L"grid: focused tile missing or hidden");
 const auto bits=unreal_.integer(*tile,L"FocusTeamNoBit");
 if(!bits||!(*bits&(1<<team)))return reject(L"grid: focused tile lacks this side's focus bit (bits "+(bits?std::to_wstring(*bits):std::wstring(L"?"))+L")");
 // The big screen for the slot being picked shows the focused character (CharaSelect_OnMoveCursor -> Screen_Change).
 const auto decided=int_element(root,L"DecideCnt_Arr",team);if(!decided)return reject(L"grid: decide count unreadable");
 if(*decided<0||*decided>1)return reject(L"grid: decide count "+std::to_wstring(*decided)+L" out of range");
 const wchar_t* fields[2][2]{{L"BP_Screen_1P_Main",L"BP_Screen_1P_Sub"},{L"BP_Screen_2P_Main",L"BP_Screen_2P_Sub"}};
 const auto screen=child(root,fields[team][*decided],L"BP_Screen_C",world);
 if(!screen||!shown_actor(*screen,world))return reject(L"grid: screen missing or hidden");
 if(!preview_ready(*screen))return reject(L"grid: screen not ready ("+tick_desc(*screen)+L")");
 if(unreal_.integer(*screen,L"TeamNo")!=std::optional<int>(team))return reject(L"grid: screen belongs to another side");
 const auto chr=unreal_.object_field(*screen,L"ChrRoot"),mesh=unreal_.object_field(*screen,L"SM_Character");
 if(!chr||!component_shown(*chr,*screen)||!mesh||!component_shown(*mesh,*screen))return reject(L"grid: screen character hidden");
 const auto random=unreal_.boolean(*screen,L"Random");if(!random)return reject(L"grid: random flag unreadable");
 MenuSnapshot s;s.valid=true;s.screen=side(team);
 if(auto o=unreal_.object(*screen))s.identity=(static_cast<std::uint64_t>(static_cast<std::uint32_t>(o->serial))<<32)|static_cast<std::uint32_t>(o->index);
 if(const auto note=element(parent,L"AttentionObj",team,2);note&&*note&&unreal_.is_a(*note,L"WD_KeyConfAttention_C")&&!tick_hidden(*note)&&widget_shown(*note)){
  if(const auto text=rendered(*note,L"TextBlock_Label"))s.value=*text;
 }
 if(*random){s.label=L"?????";return s;} // TEX_Name100 placeholder. The hidden ID is never resolved.
 const auto shown_type=unreal_.integer(*screen,L"PlayerType");
 const auto data=unreal_.property(*tile,L"CharaSelectData"); // FCharaSelectData.PlType: int32 at struct +0.
 if(!shown_type||!data||data->kind!=L"StructProperty"||data->size<4)return reject(L"grid: screen character differs from focused tile");
 const auto tile_type=get<int>(*tile+static_cast<Address>(data->offset));if(!tile_type||*tile_type!=*shown_type)return reject(L"grid: screen character differs from focused tile");
 const auto name=chara_name(*shown_type);
 if(!name){if(s.value.empty())return reject(L"grid: character name unavailable");s.label=std::move(s.value);s.value.clear();return s;}
 s.label=*name;return s;
}
std::optional<MenuSnapshot> SelectionCapture::costume(Address root,Address menu,int team)const{
 if(element(root,L"Menu_ArrObj",team)!=std::optional<Address>(menu)||!unreal_.is_a(menu,L"WD_Menu_Con_C")||!widget_shown(menu)||!active(menu,true)||unreal_.integer(menu,L"TeamNo")!=std::optional<int>(team))return {};
 const auto focus=unreal_.integer(menu,L"FocusMenu");if(!focus||(*focus!=0&&*focus!=1))return {};
 const auto row=unreal_.object_field(menu,*focus?L"WD_ListItem01":L"WD_ListItem00");
 if(!row||!*row||!unreal_.is_a(*row,*focus?L"WD_ListItem01_C":L"WD_ListItem00_C")||!widget_shown(*row))return {};
 const auto cursor=unreal_.object_field(*row,L"CanvasPanel_Cursor");if(!cursor||!widget_shown(*cursor))return {};
 const auto label=rendered(*row,L"TextBlock_Label");if(!label)return {};
 MenuSnapshot s;s.valid=true;s.screen=side(team);s.label=*label;
 if(auto o=unreal_.object(*row))s.identity=(static_cast<std::uint64_t>(static_cast<std::uint32_t>(o->serial))<<32)|static_cast<std::uint32_t>(o->index);
 if(!*focus){
  const auto value=rendered(*row,L"TextBlock_Subitem");const auto index=unreal_.integer(*row,L"CurrentSubitem");
  const auto p=unreal_.property(*row,L"SubItemNames");if(!value||!index||!p||p->inner!=L"TextProperty"||p->stride!=24)return {};
  const auto arr=unreal_.array(*row,*p,16);if(!arr||*index<0||*index>=arr->count)return {};
  // The rendered value must be the entry the index selects (WD_ListItem00.SetSubItem).
  if(unreal_.text(arr->data+static_cast<Address>(*index)*24)!=value)return {};
  s.value=*value;s.position=*index+1;s.count=arr->count;
 }else{
  const auto index=unreal_.integer(*row,L"ColorFocus"),max=unreal_.integer(*row,L"ColorMax");
  const auto chips=count(*row,L"ColorNoTextOnArray",L"ObjectProperty",8,16);
  if(!index||!max||!chips||*index<0||*max>*chips||*index>=*max)return {};
  const auto chip=element(*row,L"ColorNoTextOnArray",*index,16);
  if(!chip||!*chip||!unreal_.is_a(*chip,L"TextBlock")||!widget_shown(*chip))return {};
  const auto value=unreal_.text_field(*chip,L"Text");if(!value||!supported(*value))return {};
  s.value=*value;s.position=*index+1;s.count=*max;
 }
 if(const auto note=unreal_.object_field(menu,L"WD_KeyConfAttention_153");note&&*note&&unreal_.is_a(*note,L"WD_KeyConfAttention_C")&&!tick_hidden(*note)&&widget_shown(*note)){
  if(const auto text=rendered(*note,L"TextBlock_Label"))sentence(s.value,*text);
 }
 return s;
}
std::optional<MenuSnapshot> SelectionCapture::stone(Address root,Address gem,Address world,int team,std::uint64_t now){
 if(child(root,team?L"BP_SelectGem_2P":L"BP_SelectGem_1P",L"BP_SelectGem_C",world)!=std::optional<Address>(gem)||element(root,L"Gem_ArrObj",team)!=std::optional<Address>(gem))return {};
 if(!shown_actor(gem,world)||!active(gem,true)||unreal_.integer(gem,L"TeamNo")!=std::optional<int>(team))return {};
 const auto focus=int_element(gem,L"FocusGemNoArray",0);if(!focus||*focus<0||*focus>=6)return {};
 const auto p=unreal_.property(gem,L"InvalidGemType");if(!p||p->inner!=L"BoolProperty"||p->stride!=1)return {};
 const auto arr=unreal_.array(gem,*p,16);if(!arr||arr->count!=6)return {};
 int valid=0,position=0;
 for(int i=0;i<6;++i){const auto invalid=get<std::uint8_t>(arr->data+static_cast<Address>(i));if(!invalid)return {};if(*invalid){if(i==*focus)return {};continue;}++valid;if(i==*focus)position=valid;}
 // Bind the label to the rendered name: the focused name mesh is drawn and its material is the expected stone.
 const auto material=element(gem,L"MT_ArrGemName",*focus,16);if(!material||!*material||unreal_.name(*material,true)!=stone_material[*focus])return {};
 const auto mesh=element(gem,L"ArrGemNameMesh",*focus,16);if(!mesh||!component_shown(*mesh,gem))return {};
 const auto label=stone_name(*focus,now);if(!label)return {};
 MenuSnapshot s;s.valid=true;s.screen=side(team);s.label=*label;s.position=position;s.count=valid;
 if(auto o=unreal_.object(gem))s.identity=((static_cast<std::uint64_t>(static_cast<std::uint32_t>(o->serial))<<32)|static_cast<std::uint32_t>(o->index))^0x5300000000000000ull;
 // Storm/Surge details: the 3D WidgetComponent GemTypeText must be drawn by the visible detail-window actor.
 const auto window=unreal_.object_field(gem,L"DetailWindow");
 if(!window||!*window||child(gem,L"BP_DetailWindow",L"BP_DetailWindow_C",world)!=window||!shown_actor(*window,world))return s;
 const auto wc=unreal_.object_field(*window,L"GemTypeText");if(!wc||!*wc||!unreal_.is_a(*wc,L"WidgetComponent")||!component_shown(*wc,*window))return s;
 const auto w=unreal_.object_field(*wc,L"Widget");
 if(!w||!*w||unreal_.object_field(*window,L"GemDetail")!=w||!unreal_.is_a(*w,L"WD_GemDetail_C"))return s;
 for(const auto& pair:{std::pair<const wchar_t*,const wchar_t*>{L"TriggerText",L"TriggerEffectText"},{L"PossessionText",L"PossessionEffectText"}}){
  const auto name=rendered_local(*w,pair.first,*w),effect=rendered_local(*w,pair.second,*w);
  if(name&&effect)sentence(s.value,tr(L"{0}: {1}",{*name,*effect}));
 }
 return s;
}
std::optional<MenuSnapshot> SelectionCapture::team_preset(Address root,Address window,int team)const{
 // Members are drawn only as portraits; each name is resolved from the drawn portrait texture's package path.
 if(element(root,L"TeamSelect_Arr",team)!=std::optional<Address>(window)||!unreal_.is_a(window,L"WD_SelectTeamWindow_C")||!widget_shown(window)||!active(window,false)||unreal_.integer(window,L"TeamNo")!=std::optional<int>(team))return {};
 const auto caption=rendered(window,L"TextBlock_Caption");const auto focus=unreal_.integer(window,L"MenuFocus");const auto n=count(window,L"ArrSelectTeam",L"ObjectProperty",8,16);
 if(!caption||!focus||!n||*focus<0||*focus>=*n)return {};
 const auto item=element(window,L"ArrSelectTeam",*focus,16);if(!item||!widget_shown(*item))return {};
 MenuSnapshot s;s.valid=true;s.screen=side(team);s.label=*caption;s.position=*focus+1;s.count=*n;
 int member=0;
 for(const auto field:{L"Image_MainChr",L"Image_SubChr"}){
  const auto color_field=member++?L"TextBlock_Color2":L"TextBlock_Color1";
  const auto image=unreal_.object_field(*item,field);if(!image||!*image)continue;
  const auto type=portrait(*image);if(!type)continue;
  const auto name=chara_name(*type);if(!name)continue;
  if(!s.value.empty())s.value+=L", ";
  if(const auto color=rendered(*item,color_field))s.value+=tr(L"{0}, color {1}",{*name,*color});
  else s.value+=*name;
 }
 if(auto o=unreal_.object(*item))s.identity=(static_cast<std::uint64_t>(static_cast<std::uint32_t>(o->serial))<<32)|static_cast<std::uint32_t>(o->index);
 return s;
}
std::optional<MenuSnapshot> SelectionCapture::handicap(Address form,int team)const{
 // WD_Handicap: Focus is clamped to 0..4; SetLevel finally writes the gauge material scalar 'index' = Focus+1
 // (second loop runs Focus+1..5). The drawn value is read back and must match, so the level is not inferred.
 if(unreal_.integer(form,L"TeamNo")!=std::optional<int>(team))return {};
 const auto canvas=unreal_.object_field(form,L"CanvasPanel_Handi");if(!canvas||!*canvas||!widget_shown(*canvas))return {};
 const auto label=rendered(form,team?L"TextBlock_Label2P":L"TextBlock_Label1P");if(!label)return {};
 const auto focus=unreal_.integer(form,L"Focus");if(!focus||*focus<0||*focus>4)return {};
 const auto image=unreal_.object_field(form,L"Image_Gauge");if(!image||!*image||!widget_shown(*image))return {};
 const auto resource=brush_resource(*image);if(!resource||!unreal_.is_a(*resource,L"MaterialInstanceDynamic"))return {};
 const auto at=parameter(*resource,L"ScalarParameterValues",L"index",L"FloatProperty");if(!at)return {};
 const auto drawn=get<float>(*at);if(!drawn||!std::isfinite(*drawn)||*drawn!=static_cast<float>(*focus+1))return {};
 MenuSnapshot s;s.valid=true;s.screen=side(team);s.label=*label;s.position=*focus+1;s.count=5;
 if(auto o=unreal_.object(form))s.identity=((static_cast<std::uint64_t>(static_cast<std::uint32_t>(o->serial))<<32)|static_cast<std::uint32_t>(o->index))^0x4800000000000000ull;
 return s;
}
std::optional<MenuSnapshot> SelectionCapture::battle(Address root,Address world,int team)const{
 const auto main=child(root,L"BP_BattleSettingMain",L"BP_BattleSettingMain_C",world);
 if(!main||unreal_.object_field(root,L"BattleSettingObj")!=main||!shown_actor(*main,world)||!active(*main,true))return {};
 const auto con=unreal_.object_field(*main,L"BattleSetting_Obj");if(!con||!*con||!unreal_.is_a(*con,L"WD_BattleSetting_Con_C")||!widget_shown(*con)||!active(*con,true))return {};
 // An active handicap form for the tracked side owns that side's input.
 if(const auto form=unreal_.object_field(*con,team?L"WD_Handicap_2P":L"WD_Handicap_1P");form&&*form&&unreal_.is_a(*form,L"WD_Handicap_C")&&widget_shown(*form)&&active(*form,true))return handicap(*form,team);
 const auto panel=unreal_.object_field(*con,L"WD_BattleSettigNew");if(!panel||!*panel||!unreal_.is_a(*panel,L"WD_BattleSettig_New_C")||!widget_shown(*panel)||!active(*panel,true))return {};
 const auto item=unreal_.object_field(*panel,L"FocusListItem");if(!item||!*item||!unreal_.is_a(*item,L"WD_BS_ListItem0_C")||!widget_shown(*item))return {};
 const auto n=count(*panel,L"ButtonArray",L"ObjectProperty",8,32);if(!n)return {};
 int shown=0,position=0;
 for(int i=0;i<*n;++i){const auto row=element(*panel,L"ButtonArray",i,32);if(!row)return {};if(*row&&widget_shown(*row)){++shown;if(*row==*item)position=shown;}}
 if(!position)return {};
 const auto label=rendered(*item,L"TextBlock_Label"),value=rendered(*item,L"TextBlock_Subitem");if(!label||!value)return {};
 MenuSnapshot s;s.valid=true;s.label=*label;s.value=*value;s.position=position;s.count=shown;
 if(const auto header=unreal_.object_field(*panel,L"WD_Header");header&&*header&&widget_shown(*header)){if(auto t=rendered(*header,L"TextBlock_Caption"))s.screen=*t;}
 if(auto o=unreal_.object(*item))s.identity=(static_cast<std::uint64_t>(static_cast<std::uint32_t>(o->serial))<<32)|static_cast<std::uint32_t>(o->index);
 return s;
}
std::optional<MenuSnapshot> SelectionCapture::key_form(Address form,bool keyboard)const{
 MenuSnapshot s;s.valid=true;
 if(const auto caption=rendered(form,L"TextBlock_Label"))s.screen=*caption;
 if(auto o=unreal_.object(form))s.identity=((static_cast<std::uint64_t>(static_cast<std::uint32_t>(o->serial))<<32)|static_cast<std::uint32_t>(o->index))^0x4b00000000000000ull;
 if(keyboard){
  // WD_KeyboardConfBind: the prompt drawn while a keyboard binding is being captured.
  const auto bind=unreal_.object_field(form,L"WD_KeyboardConfBind_138");
  if(bind&&*bind&&unreal_.is_a(*bind,L"WD_KeyboardConfBind_C")&&widget_shown(*bind)&&active(*bind,true)){const auto prompt=rendered(*bind,L"TextBlock_Label");if(!prompt)return {};s.label=*prompt;return s;}
 }
 const auto id=unreal_.integer(form,L"ButtonFocusID");const auto main=unreal_.boolean(form,L"IsActiveMainPanel");if(!id||!main)return {};
 const wchar_t* field=nullptr;int key_index=-1;
 if(*main&&*id>=0&&*id<18){field=key_main[*id];if(keyboard)key_index=keyboard_main_index[*id];}
 else if(!*main&&*id>=20&&*id<20+(keyboard?10:6)){field=keyboard?keyboard_sub[*id-20]:pad_sub[*id-20];if(keyboard)key_index=keyboard_sub_index[*id-20];}
 if(!field)return {};
 const auto part=unreal_.object_field(form,field);if(!part||!*part||!widget_shown(*part))return {};
 const auto label=rendered(*part,L"TextBlock_Label");if(!label)return {};s.label=*label;
 if(unreal_.is_a(*part,L"WD_KCbuttonParts4_C")){if(const auto v=rendered(*part,L"TextBlock_Subitem"))s.value=*v;else return {};}
 else if(unreal_.is_a(*part,L"WD_KCbuttonParts5_C")){if(const auto v=rendered(*part,L"TextBlock_SubItem"))s.value=*v;else return {};}
 else if(unreal_.is_a(*part,L"WD_KCbuttonParts_C")||unreal_.is_a(*part,L"WD_KCbuttonParts2_C")){
  const auto icon=unreal_.object_field(*part,L"Image_Button");
  if(icon&&*icon&&widget_shown(*icon)){
   if(!keyboard)s.value=glyph(*icon);
   else if(key_index>=0){
    // WD_KeyboardConf.UpdateItemIcon_Single draws KeyboardConfigData[ConvertFocusIDToArrayIndex(id)]; Escape draws a blank cell.
    const auto p=unreal_.property(form,L"KeyboardConfigData");if(!p||p->inner!=L"StructProperty")return {};
    const auto arr=unreal_.array(form,*p,64);const auto st=struct_of(p->address,true);if(!arr||!st||key_index>=arr->count)return {};
    const auto m=member(*st,L"KeyName");if(!m||m->kind!=L"NameProperty"||m->offset+8>p->stride)return {};
    const auto key=fname(arr->data+static_cast<Address>(key_index)*static_cast<Address>(p->stride)+static_cast<Address>(m->offset));if(!key)return {};
    if(*key!=L"Escape")s.value=*key;
   }
  }
 }
 else if(!unreal_.is_a(*part,L"WD_KCbuttonParts3_C"))return {};
 return s;
}
std::optional<MenuSnapshot> SelectionCapture::key_config(Address root,bool keyboard,int team)const{
 // WD_KeyConfMain: Left/Right pad forms; WD_KeyboardConfMain: keyboard form (left) + pad form (right).
 // The tracked side's form is preferred; otherwise the other drawn side, which carries its own caption.
 const wchar_t* forms[2]{keyboard?L"WD_KeyboardConf":L"WD_KeyConf_Left",L"WD_KeyConf_Right"};
 const wchar_t* waits[2]{L"WD_KeyConf_WaitingL",L"WD_KeyConf_WaitingR"};
 const wchar_t* notes[2]{keyboard?L"WD_KeyboardConf_Attention":L"WD_KeyConf_Attention_L",L"WD_KeyConf_Attention_R"};
 for(const int s:{team,1-team}){
  std::optional<MenuSnapshot> out;
  const auto form=unreal_.object_field(root,forms[s]);
  const bool kb=keyboard&&s==0;
  if(form&&*form&&unreal_.is_a(*form,kb?L"WD_KeyboardConf_C":L"WD_KeyConf_C")&&widget_shown(*form)&&active(*form,true)){out=key_form(*form,kb);if(!out)return {};}
  else if(const auto wait=unreal_.object_field(root,waits[s]);wait&&*wait&&unreal_.is_a(*wait,L"WD_KeyConfWaiting_C")&&widget_shown(*wait)&&active(*wait,true)){
   MenuSnapshot w;w.valid=true;
   if(const auto t=rendered(*wait,L"TextBlock_Label"))sentence(w.label,*t);
   for(const auto box:{L"HorizontalBox_174",L"HorizontalBox_175",L"HorizontalBox_176",L"HorizontalBox_177"}){const auto b=unreal_.object_field(*wait,box);if(b&&*b){if(const auto t=line_text(*b))sentence(w.label,*t);}}
   if(w.label.empty())return {};
   if(auto o=unreal_.object(*wait))w.identity=((static_cast<std::uint64_t>(static_cast<std::uint32_t>(o->serial))<<32)|static_cast<std::uint32_t>(o->index))^0x5700000000000000ull;
   out=w;
  }
  if(!out)continue;
  if(const auto note=unreal_.object_field(root,notes[s]);note&&*note&&unreal_.is_a(*note,L"WD_KeyConfAttention_C")&&!tick_hidden(*note)&&widget_shown(*note)){if(const auto t=rendered(*note,L"TextBlock_Label"))sentence(out->value,*t);}
  return out;
 }
 return {};
}
std::optional<MenuSnapshot> SelectionCapture::chara(Address root,Address world,int team,std::uint64_t now){
 const auto parent=child(root,L"BP_Select_Parent",L"BP_Select_Parent_C",world);
 if(!parent||unreal_.object_field(root,L"SelectParentObj")!=parent||!shown_actor(*parent,world))return reject(L"chara: select parent missing or hidden");
 if(!active(*parent,true))return reject(L"chara: select parent not active ("+tick_desc(*parent)+L")");
 // Battle settings take the main side's input while open (BP_CharaSelectMain.SendInputCheck_CharaSelectMain).
 if(unreal_.boolean(root,L"BattleSettingActive")==std::optional<bool>(true))return battle(root,world,team);
 // Input_ArrObj[team] is the object receiving that side's input; its class is the phase.
 const auto owner=element(root,L"Input_ArrObj",team,8);if(!owner||!*owner)return reject(team?L"chara: no input owner for Player 2":L"chara: no input owner for Player 1");
 if(*owner==*parent)return grid(root,*parent,world,team);
 if(unreal_.is_a(*owner,L"WD_Menu_Con_C"))return costume(root,*owner,team);
 if(unreal_.is_a(*owner,L"BP_SelectGem_C"))return stone(root,*owner,world,team,now);
 if(unreal_.is_a(*owner,L"WD_SelectTeamWindow_C"))return team_preset(root,*owner,team);
 return reject(L"chara: unhandled input owner"); // Button config and unknown owners: not covered here.
}
std::optional<MenuSnapshot> SelectionCapture::stage(Address root,Address world)const{
 const auto parent=child(root,L"BP_Stage_Parent",L"BP_Stage_Parent_C",world);
 if(!parent||unreal_.object_field(root,L"StageParentObj")!=parent||!shown_actor(*parent,world)||!active(*parent,true))return {};
 const auto team=unreal_.integer(root,L"StageSelectTeamNo");if(!team||(*team!=0&&*team!=1))return {};
 const auto uid=unreal_.integer(*parent,L"FocusUniqId"),type=unreal_.integer(*parent,L"FocusStageType");if(!uid||!type)return {};
 const auto tile=element(*parent,L"ArrChildStage",*uid,64);if(!tile||!*tile||!unreal_.is_a(*tile,L"BP_Stage_C")||!shown_actor(*tile,world))return {};
 const auto data=unreal_.property(*tile,L"StageData"); // FStageSelectData.StageType: byte at struct +0.
 if(!data||data->kind!=L"StructProperty"||data->size<1)return {};
 const auto tile_type=get<std::uint8_t>(*tile+static_cast<Address>(data->offset));if(!tile_type||*tile_type!=*type)return {};
 const auto name=unreal_.object_field(*parent,L"StageName");
 if(!name||!*name||child(*parent,L"BP_Select_Stage_Name",L"BP_Stage_Name_C",world)!=name||!shown_actor(*name,world)||!active(*name,true))return {};
 const auto wc=unreal_.object_field(*name,L"StageName");if(!wc||!*wc||!unreal_.is_a(*wc,L"WidgetComponent")||!component_shown(*wc,*name))return {};
 const auto w=unreal_.object_field(*wc,L"Widget");if(!w||!*w||unreal_.object_field(*name,L"WD_StageName")!=w||!unreal_.is_a(*w,L"WD_StageName_C"))return {};
 const auto label=rendered_local(*w,L"TextBlock_StageName",*w);if(!label)return {};
 MenuSnapshot s;s.valid=true;s.screen=side(*team);s.label=*label;
 if(auto o=unreal_.object(*tile))s.identity=(static_cast<std::uint64_t>(static_cast<std::uint32_t>(o->serial))<<32)|static_cast<std::uint32_t>(o->index);
 return s;
}
std::optional<MenuSnapshot> SelectionCapture::controller(Address root)const{
 // WD_CtrlSelectMain: CtrlSlotArray[0] = controller on the left (1P) slot, [1] = right (2P), -1 empty;
 // ConnectPadNoArray[i] >= 0 = connected. Every connected, drawn controller is listed (no input focus).
 const auto left=rendered(root,L"TextBlock_Caption1P"),right=rendered(root,L"TextBlock_Caption2P");if(!left||!right)return {};
 const auto slot0=int_element(root,L"CtrlSlotArray",0,8),slot1=int_element(root,L"CtrlSlotArray",1,8);if(!slot0||!slot1)return {};
 const auto n=count(root,L"CtrlSymbols",L"ObjectProperty",8,8);if(!n)return {};
 std::wstring label;
 for(int i=0;i<*n;++i){
  const auto symbol=element(root,L"CtrlSymbols",i,8);if(!symbol)return {};
  if(!*symbol||!unreal_.is_a(*symbol,L"WD_CtrlSymbol0_C")||!widget_shown(*symbol))continue;
  const auto pad=int_element(root,L"ConnectPadNoArray",i,8);if(!pad||*pad<0)continue;
  const auto name=rendered(*symbol,L"UserName");if(!name)continue; // Connected but unnamed: not labelled here.
  const auto placement=*slot0==i?*left:*slot1==i?*right:tr(L"center");
  sentence(label,tr(L"{0}: {1}",{*name,placement}));
 }
 if(label.empty())return {};
 MenuSnapshot s;s.valid=true;s.label=label;
 if(const auto header=unreal_.object_field(root,L"WD_Header");header&&*header&&widget_shown(*header)){if(auto t=rendered(*header,L"TextBlock_Caption"))s.screen=*t;}
 if(auto o=unreal_.object(root))s.identity=(static_cast<std::uint64_t>(static_cast<std::uint32_t>(o->serial))<<32)|static_cast<std::uint32_t>(o->index);
 return s;
}

MenuSnapshot SelectionCapture::sample(std::uint64_t now,int team){
 rejection_.clear();
 if(team!=0&&team!=1){rejection_=L"select: invalid player";return {};}
 const auto count=unreal_.object_count();if(!count){rejection_=L"select: object registry unreadable";return {};}count_=*count;
 // Button/keyboard config is a viewport popup over options, pause or character select: it owns the screen while drawn.
 const Root pad=unique(cached(pad_conf_,L"WD_KeyConfMain_C",now),[&](Address a){return widget_shown(a)&&active(a,false);});
 const Root keyboard=unique(cached(keyboard_conf_,L"WD_KeyboardConfMain_C",now),[&](Address a){return widget_shown(a)&&active(a,false);});
 if(pad.ambiguous||keyboard.ambiguous||(pad.address&&keyboard.address)){rejection_=L"select: ambiguous button configuration";return {};}
 if(pad.address||keyboard.address){const auto s=key_config(pad.address?*pad.address:*keyboard.address,keyboard.address.has_value(),team);if(!s)rejection_=L"select: active button configuration unreadable";return s?*s:MenuSnapshot{};}
 const auto engine=get<Address>(unreal_.base()+engine_rva);
 const auto viewport=engine&&*engine?get<Address>(*engine+0x5d8):std::nullopt;
 const auto world=viewport&&*viewport?get<Address>(*viewport+0x80):std::nullopt;
 if(!world||!*world||!clean(*world)||!unreal_.is_a(*world,L"World")){rejection_=L"select: no current world";return {};}
 const Root ctrl=unique(cached(ctrl_,L"WD_CtrlSelectMain_C",now),[&](Address a){return widget_shown(a)&&active(a,false);});
 const Root chara_root=unique(cached(chara_,L"BP_CharaSelectSimple_C",now),[&](Address a){return shown_actor(a,*world)&&active(a,true);});
 const Root stage_root=unique(cached(stage_,L"BP_StageSelectMain_C",now),[&](Address a){return shown_actor(a,*world)&&active(a,true);});
 if(ctrl.ambiguous||chara_root.ambiguous||stage_root.ambiguous){rejection_=L"select: ambiguous roots";return {};}
 if((ctrl.address?1:0)+(chara_root.address?1:0)+(stage_root.address?1:0)!=1){rejection_=L"select: no unique select root";return {};}
 std::optional<MenuSnapshot> s;
 if(ctrl.address)s=controller(*ctrl.address);
 else if(chara_root.address){
  int input_team=team;
  // BP_CharaSelectSingle.SetSelectTeamPadNo binds both teams to the same local
  // controller. Gem_OnDecideContinue clears the finished side's Input_ArrObj,
  // so its single remaining owner is the cursor the user now controls. Combat
  // tracking stays separate; two-input and other subclasses retain team choice.
  if(unreal_.is_a(*chara_root.address,L"BP_CharaSelectSingle_C")){
   const auto one=element(*chara_root.address,L"Input_ArrObj",0,8),two=element(*chara_root.address,L"Input_ArrObj",1,8);
   if(!one||!two){rejection_=L"chara: input owners unreadable";return {};}
   if((*one!=0)!=(*two!=0))input_team=*one?0:1;
  }
  s=chara(*chara_root.address,*world,input_team,now);
 }
 else s=stage(*stage_root.address,*world);
 if(s)rejection_.clear();
 else if(rejection_.empty())rejection_=L"select: no snapshot";
 return s?*s:MenuSnapshot{};
}
}
