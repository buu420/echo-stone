#include "mvci/collection.hpp"
#include "mvci/localization.hpp"
#include <algorithm>
#include <set>
namespace mvci {
namespace {
// GEngine (research/combat report.md): +0x5d8 GameViewport, +0x80 World. Level+0xb0 OwningWorld.
constexpr Address engine_rva=0x36bc348;
constexpr int tick_active=2,list_normal=1,max_buttons=256,max_columns=64,last_list_mode=5;
constexpr std::uint64_t rescan_ms=500;
// Designer defaults still drawn before Blueprint copies the real text (retained assets, live PID 33524):
// WD_DBSpecHeader caption, WD_Collect_Parts/CharParts/ArtListTextNavi labels, WD_DB_StItData name,
// WD_DB_ChaData.TextBlock_Text (never set by Blueprint, so it is not read at all).
const wchar_t* const placeholders[]{L"TEXTÀg",L"Character ThemeÀg漢",L"TEXTÀg（アイテム名）",L"text"};
bool readable(const std::wstring& t){
 if(t.empty()||t.rfind(L"msg_",0)==0)return false;
 return std::none_of(std::begin(placeholders),std::end(placeholders),[&](const wchar_t* p){return t==p;});
}
void sentence(std::wstring& out,const std::wstring& part){
 if(part.empty())return;
 if(!out.empty()){const wchar_t last=out.back();out+=(last==L'.'||last==L'!'||last==L'?')?L" ":L". ";}
 out+=part;
}
struct Child {const wchar_t* field;const wchar_t* cls;};
// GF_Collection.State (CollectModeState) -> summoned top (Summon* / SummonWidget) and its ChildOpen forward:
// MovisTop @1435 WDChild, DBTop @1949 / BgmTop @1579 / VoiceTop @1607 ActiveChild. The child must be one of
// the top's own declared child widgets of the expected class.
struct Top {const wchar_t* cls;const wchar_t* caption;const wchar_t* forward;std::vector<Child> children;};
const Top tops[last_list_mode+1]{
 {L"WD_CollectionTop_C",L"WD_Coll_Caption",nullptr,{}},
 {L"WD_MovisTop_C",L"WD_Coll_Caption_56",L"WDChild",{{L"WD_StoryList",L"WD_StoryList_C"}}},
 {L"WD_DBTop_C",L"WD_Coll_Caption_83",L"ActiveChild",{{L"WD_ChaList_DB",L"WD_DB_ChaList_C"},{L"WD_ChaData",L"WD_DB_ChaData_C"},{L"WD_StoryList_DB",L"WD_DB_StoryList_C"},{L"WD_StoryItemData",L"WD_DB_StItData_C"}}},
 {L"WD_ArtTop_New_C",L"WD_Coll_Caption_135",nullptr,{}},
 {L"WD_BgmTop_C",L"WD_Coll_Caption_125",L"ActiveChild",{{L"WD_ChaList",L"WD_ChaList_C"},{L"WD_OtherList",L"WD_OtherList_C"}}},
 {L"WD_VoiceTop_C",L"WD_Coll_Caption_169",L"ActiveChild",{{L"WD_VoiceList",L"WD_Cha_VoiceList_C"}}}};
std::uint64_t tagged(const ObjectRef& o,std::uint64_t tag){return ((static_cast<std::uint64_t>(static_cast<std::uint32_t>(o.serial))<<32)|static_cast<std::uint32_t>(o.index))^tag;}
}

const std::vector<ObjectRef>& CollectionCapture::cached(Cache& c,const wchar_t* cls,std::uint64_t now)const{
 // Rescan on a new registry count, a retired (dead) cached object, or every rescan_ms.
 const bool retired=std::any_of(c.objects.begin(),c.objects.end(),[this](const ObjectRef& o){return !unreal_.alive(o);});
 if(unreal_.shared_discovery()||!c.filled||c.count!=count_||retired||now<c.at||now-c.at>=rescan_ms){c.count=count_;c.objects=unreal_.instances(cls);c.at=now;c.filled=true;}
 return c.objects;
}
bool CollectionCapture::clean(Address a)const{auto o=unreal_.object(a);return o&&!(o->flags&0x30);}
bool CollectionCapture::exact(Address a,const wchar_t* cls)const{auto o=unreal_.object(a);return o&&!(o->flags&0x30)&&unreal_.name_is(o->cls,cls);}
bool CollectionCapture::derives(Address a,const wchar_t* cls)const{
 auto o=unreal_.object(a);if(!o||(o->flags&0x30))return false;Address c=o->cls;std::set<Address> seen;
 while(c&&seen.size()<64&&seen.insert(c).second){if(!unreal_.object(c))return false;if(unreal_.name_is(c,cls))return true;const auto parent=get<Address>(c+0x30);if(!parent)return false;c=*parent;}
 return false;
}
bool CollectionCapture::in_world(Address actor,Address world)const{
 // AActor's Outer is its ULevel; ULevel.OwningWorld (+0xb0) must be the viewport's current world.
 const auto level=get<Address>(actor+0x20);if(!level||!*level||!unreal_.is_a(*level,L"Level"))return false;
 const auto owning=get<Address>(*level+0xb0);return owning&&*owning==world;
}
bool CollectionCapture::active(Address owner)const{
 // BP_TickStateObj.State: 0 Hidden, 1 Enter, 2 Active, 3 Exit; it must be bound back to this owner.
 const auto tick=unreal_.object_field(owner,L"TickStateObj");if(!tick||!*tick||!exact(*tick,L"BP_TickStateObj_C"))return false;
 return unreal_.object_field(*tick,L"ParentObj")==std::optional<Address>(owner)&&unreal_.integer(*tick,L"State")==std::optional<int>(tick_active);
}
bool CollectionCapture::shown(Address w)const{return w&&clean(w)&&unreal_.shown(w,visibility_);}
std::optional<Address> CollectionCapture::child(Address owner,const wchar_t* field)const{
 const auto v=unreal_.object_field(owner,field);if(!v||!*v||!clean(*v))return {};return v;
}
std::optional<std::wstring> CollectionCapture::rendered(Address owner,const wchar_t* field)const{
 const auto block=child(owner,field);if(!block||!unreal_.is_a(*block,L"TextBlock")||!shown(*block))return {};
 const auto text=unreal_.text_field(*block,L"Text");if(!text||!readable(*text))return {};return text;
}
std::optional<bool> CollectionCapture::disabled(Address block)const{
 // WD_Collect_Parts.setDisable swaps TextBlock_Label.Font.FontMaterial to M_FontDisable (else M_FontEnable).
 // The member is resolved through the reflected SlateFontInfo struct (live: FontMaterial +8).
 const auto font=unreal_.property(block,L"Font");if(!font||font->kind!=L"StructProperty")return {};
 const auto st=get<Address>(font->address+0x78);if(!st||!*st||!clean(*st))return {};
 auto member=get<Address>(*st+0x38);std::set<Address> seen;
 while(member&&*member&&seen.size()<64&&seen.insert(*member).second){
  if(!clean(*member))return {};
  if(unreal_.name_is(*member,L"FontMaterial")){
   if(unreal_.type(*member)!=L"ObjectProperty")return {};
   const auto offset=get<int>(*member+0x50);if(!offset||*offset<0||*offset+8>font->size)return {};
   const auto material=get<Address>(block+static_cast<Address>(font->offset+*offset));if(!material)return {};
   if(!*material)return false;
   return clean(*material)&&unreal_.name_is(*material,L"M_FontDisable");
  }
  member=get<Address>(*member+0x28);
 }
 return {};
}
bool CollectionCapture::badge(Address part,const wchar_t* field)const{
 // SetNewIconVisible: the NEW panel is Visible (0) when shown, Collapsed/Hidden otherwise.
 const auto p=child(part,field);return p&&shown(*p);
}
std::wstring CollectionCapture::help(Address owner,Address top)const{
 for(const Address a:{owner,top})for(const auto* n:{L"WD_Collection_Navi",L"WD_Collection_Navi_0"}){
  const auto navi=child(a,n);if(!navi||!exact(*navi,L"WD_Collection_Navi_C")||!shown(*navi))continue;
  auto line=menu_hints(unreal_,*navi,visibility_);if(!line.empty())return line;
 }
 return {};
}
std::optional<MenuSnapshot> CollectionCapture::list(Address owner,Address top,const std::wstring& caption)const{
 const auto owner_ref=unreal_.object(owner);if(!owner_ref)return {};
 // SetupButtonList creates exactly ListNum parts; the focused entry is Buttons[FocusIndex] (MoveFocus wraps).
 const auto focus=unreal_.integer(owner,L"FocusIndex"),total=unreal_.integer(owner,L"ListNum");
 const auto p=unreal_.property(owner,L"Buttons");if(!focus||!total||!p||p->inner!=L"ObjectProperty"||p->stride!=8)return {};
 const auto arr=unreal_.array(owner,*p,max_buttons);if(!arr||arr->count<1||arr->count!=*total||*focus<0||*focus>=arr->count)return {};
 const auto part=get<Address>(arr->data+static_cast<Address>(*focus)*8);if(!part||!*part)return {};
 const auto part_ref=unreal_.object(*part);if(!part_ref||!derives(*part,L"WD_Collect_PartsBase_C")||!shown(*part))return {};
 const wchar_t* label_field=exact(*part,L"WD_DB_Parts_C")?L"TextBlock_CharacterTheme":L"TextBlock_Label";
 const auto block=child(*part,label_field);const auto block_ref=block?unreal_.object(*block):std::nullopt;if(!block_ref)return {};
 const auto label=rendered(*part,label_field);if(!label)return {};
 MenuSnapshot s;s.valid=true;s.label=*label;s.position=*focus+1;s.count=arr->count;
 if(disabled(*block)==std::optional<bool>(true))s.enabled=false;
 if(badge(*part,L"CanvasPanel_NewIcon"))s.value=tr(L"New");
 s.screen=caption;
 if(const auto title=rendered(owner,L"TextBlock_ModeTitle"))s.screen=*title;
 if(exact(owner,L"WD_DB_StItData_C")){
  // The focused item's drawn detail panel; locked items take another branch, so only shown text is read.
  if(const auto header=child(owner,L"WD_DBSpecHeader");header&&shown(*header)){if(const auto t=rendered(*header,L"TextBlock_Caption"))s.screen=*t;}
  if(const auto name=rendered(owner,L"TextBlock_NameText");name&&*name!=s.label)sentence(s.value,*name);
  if(const auto desc=rendered(owner,L"DescriptionText"))sentence(s.value,*desc);
 }
 s.help=help(owner,top);
 // Visibility/help access may run native attributes. Accept only the same owner,
 // registry serials, focused array slot and rendered label captured above.
 const auto after=unreal_.array(owner,*p,max_buttons);
 if(!after||after->data!=arr->data||after->count!=arr->count||unreal_.integer(owner,L"ListNum")!=total||unreal_.integer(owner,L"FocusIndex")!=focus||unreal_.integer(owner,L"State")!=std::optional<int>(list_normal))return {};
 if(get<Address>(after->data+static_cast<Address>(*focus)*8)!=part||child(*part,label_field)!=block||unreal_.text_field(*block,L"Text")!=label)return {};
 if(!unreal_.alive(*owner_ref)||!unreal_.alive(*part_ref)||!unreal_.alive(*block_ref))return {};
 s.identity=tagged(*owner_ref,0x4300000000000000ull);
 return s;
}
std::optional<MenuSnapshot> CollectionCapture::artwork(Address top,const std::wstring& caption)const{
 // WD_ArtTop_New: only unlocked artwork is added (CheckEnableArtwark), one WD_Art_Smn per entry.
 // Cell = FocusIndex*ListViewMax_H + FocusIndex_H (GetButtonArrayIndex); SetArtTextCurrentFocus draws its caption.
 const auto o=unreal_.object(top);const auto detail=unreal_.boolean(top,L"IsDetailView");if(!o||!detail)return {};
 const auto row=unreal_.integer(top,L"FocusIndex"),column=unreal_.integer(top,L"FocusIndex_H"),columns=unreal_.integer(top,L"ListViewMax_H"),rows=unreal_.integer(top,L"ListNum");
 std::optional<Property> p;std::optional<ArrayView> arr;std::optional<Address> cell,view;std::optional<ObjectRef> cell_ref,view_ref;int index=0;
 // Retain identity and focus before any effective-visibility or guide access.
 if(*detail){
  view=child(top,L"WD_ArtDetail");view_ref=view?unreal_.object(*view):std::nullopt;
  if(!view_ref||!exact(*view,L"WD_ArtDetail_C"))return {};
 }else{
  p=unreal_.property(top,L"Buttons");if(!row||!column||!columns||!rows||!p||p->inner!=L"ObjectProperty"||p->stride!=8)return {};
  if(*columns<1||*columns>max_columns||*column<0||*column>=*columns||*row<0||*row>=*rows)return {};
  arr=unreal_.array(top,*p,max_buttons);if(!arr||arr->count<1||*rows!=(arr->count+*columns-1)/ *columns)return {};
  index=*row* *columns+*column;if(index>=arr->count)return {};
  cell=get<Address>(arr->data+static_cast<Address>(index)*8);cell_ref=cell&&*cell?unreal_.object(*cell):std::nullopt;
  if(!cell_ref||!exact(*cell,L"WD_Art_Smn_C"))return {};
 }
 const auto bar=child(top,L"WD_ArtListText");const auto bar_ref=bar?unreal_.object(*bar):std::nullopt;
 if(!bar_ref||!exact(*bar,L"WD_ArtListTextNavi_C")||!shown(*bar))return {};
 std::optional<Address> block,guide;std::optional<ObjectRef> block_ref,guide_ref;std::optional<std::wstring> label;
 MenuSnapshot s;s.valid=true;s.screen=caption;
 if(*detail){
  // ChangeTextBlock(true) hides CanvasPanel_Text and shows CanvasPanel_Navi.
  // The art caption is no longer drawn: narrate only the visible control guide.
  guide=child(*bar,L"WD_BtnNavi_9");guide_ref=guide?unreal_.object(*guide):std::nullopt;if(!guide_ref)return {};
  s.label=menu_hints(unreal_,*guide,visibility_,true);if(s.label.empty())return {};
  if(!shown(*view)||!unreal_.alive(*view_ref)||child(top,L"WD_ArtDetail")!=view)return {};
  if(!unreal_.alive(*guide_ref)||child(*bar,L"WD_BtnNavi_9")!=guide)return {};
  s.identity=tagged(*o,0x4400000000000000ull);
 }else{
  block=child(*bar,L"TextBlock_ArtIdText");block_ref=block?unreal_.object(*block):std::nullopt;if(!block_ref)return {};
  label=rendered(*bar,L"TextBlock_ArtIdText");if(!label)return {};s.label=*label;
  if(const auto navi=child(*bar,L"WD_BtnNavi_9"))s.help=menu_hints(unreal_,*navi,visibility_,true);
  if(s.help.empty())s.help=help(top,top);
  if(!shown(*cell))return {};s.position=index+1;s.count=arr->count;
  if(badge(*cell,L"CanvasPanel_NEW"))s.value=tr(L"New");
  const auto after=unreal_.array(top,*p,max_buttons);
  if(!after||after->data!=arr->data||after->count!=arr->count||unreal_.integer(top,L"FocusIndex")!=row||unreal_.integer(top,L"FocusIndex_H")!=column||unreal_.integer(top,L"ListViewMax_H")!=columns||unreal_.integer(top,L"ListNum")!=rows)return {};
  if(get<Address>(after->data+static_cast<Address>(index)*8)!=cell||!unreal_.alive(*cell_ref))return {};
  if(child(*bar,L"TextBlock_ArtIdText")!=block||unreal_.text_field(*block,L"Text")!=label||!unreal_.alive(*block_ref))return {};
  s.identity=tagged(*o,0x4100000000000000ull);
 }
 if(unreal_.boolean(top,L"IsDetailView")!=detail||child(top,L"WD_ArtListText")!=bar)return {};
 if(!unreal_.alive(*o)||!unreal_.alive(*bar_ref))return {};return s;
}
std::optional<MenuSnapshot> CollectionCapture::viewer(Address owner,Address top,const std::wstring& caption)const{
 // WD_DB_ChaData 3D viewer: the header caption (SetupTitleLine: CollectionChara.Name) and the guide only.
 // TextBlock_Text is never written by Blueprint (designer 'text') and TextBlock_NameText is designer-hidden.
 const auto owner_ref=unreal_.object(owner);if(!owner_ref)return {};
 const auto model=unreal_.boolean(owner,L"IsModelViewer");
 if(model==std::optional<bool>(true)){
  // Entering model viewer hides the header/normal guide and shows this separate
  // guide (WD_DB_ChaData ubergraph @1957..2082). Read its drawn action text only.
  const auto guide=child(owner,L"WD_BtnNavi_View");if(!guide)return {};
  const auto guide_ref=unreal_.object(*guide);if(!guide_ref||!exact(*guide,L"WD_BtnNavi_View_C")||!shown(*guide))return {};
  // NaviTextArray is an unused designer field. The actual template binds these
  // eight TextBlocks and the Blueprint toggles their visibility individually.
  const wchar_t* const fields[]{L"TextBlock_Rotation",L"TextBlock_Change",L"TextBlock_Cross",L"TextBlock_Zoom",L"TextBlock_Help",L"TextBlock_Default",L"TextBlock_Color",L"TextBlock_Back"};
  struct BoundBlock {const wchar_t* field;ObjectRef ref;};std::vector<BoundBlock> blocks;std::wstring actions;
  for(const auto field:fields){
   const auto block=unreal_.object_field(*guide,field);if(!block)return {};if(!*block)continue;
   const auto ref=unreal_.object(*block);if(!ref||!unreal_.is_a(*block,L"TextBlock"))return {};
   blocks.push_back({field,*ref});if(!shown(*block))continue;
   const auto text=unreal_.text_field(*block,L"Text");if(!text||!readable(*text))return {};sentence(actions,*text);
  }
  if(actions.empty()||unreal_.boolean(owner,L"IsModelViewer")!=model||child(owner,L"WD_BtnNavi_View")!=guide)return {};
  if(!unreal_.alive(*owner_ref)||!unreal_.alive(*guide_ref)||std::any_of(blocks.begin(),blocks.end(),[&](const BoundBlock& b){return !unreal_.alive(b.ref)||unreal_.object_field(*guide,b.field)!=std::optional<Address>(b.ref.address);}))return {};
  MenuSnapshot s;s.valid=true;s.screen=caption;s.label=actions;s.identity=tagged(*owner_ref,0x4d00000000000000ull);return s;
 }
 const auto header=child(owner,L"WD_DBSpecHeader");if(!header)return {};
 const auto header_ref=unreal_.object(*header);if(!header_ref||!exact(*header,L"WD_DBSpecHeader_C")||!shown(*header))return {};
 const auto name=rendered(*header,L"TextBlock_Caption");if(!name)return {};
 MenuSnapshot s;s.valid=true;s.screen=caption;s.label=*name;s.help=help(owner,top);
 if(!unreal_.alive(*owner_ref)||!unreal_.alive(*header_ref)||child(owner,L"WD_DBSpecHeader")!=header||unreal_.boolean(owner,L"IsModelViewer")!=model)return {};
 s.identity=tagged(*owner_ref,0x4800000000000000ull);return s;
}
MenuSnapshot CollectionCapture::sample(std::uint64_t now){
 const auto count=unreal_.object_count();if(!count)return {};count_=*count;
 const auto engine=get<Address>(unreal_.base()+engine_rva);
 const auto viewport=engine&&*engine?get<Address>(*engine+0x5d8):std::nullopt;
 const auto world=viewport&&*viewport?get<Address>(*viewport+0x80):std::nullopt;
 if(!world||!*world||!clean(*world)||!unreal_.is_a(*world,L"World"))return {};
 // One live GF_Collection_C in the current world; retained flows from older worlds are ignored.
 std::optional<ObjectRef> flow;
 for(const auto& o:cached(flows_,L"GF_Collection_C",now)){
  if(!unreal_.alive(o)||!exact(o.address,L"GF_Collection_C")||!in_world(o.address,*world))continue;
  if(flow)return {};flow=o;
 }
 if(!flow)return {};
 // State_Normal: input goes to CurrentWidget only while !IsTransition; states 6..11 travel or confirm.
 const auto state=unreal_.integer(flow->address,L"State");if(!state||*state<0||*state>last_list_mode)return {};
 if(unreal_.boolean(flow->address,L"IsTransition")!=std::optional<bool>(false))return {};
 const Top& mode=tops[*state];
 const auto top=child(flow->address,L"CurrentWidget");
 const auto top_ref=top?unreal_.object(*top):std::nullopt;
 if(!top_ref||!exact(*top,mode.cls)||!derives(*top,L"WD_Collect_ListBase_C")||!shown(*top)||!active(*top))return {};
 // WD_Coll_Caption.SetAnimationIn -> setCaptionText: the drawn mode title.
 std::wstring caption;
 if(const auto c=child(*top,mode.caption);c&&exact(*c,L"WD_Coll_Caption_C")&&shown(*c)){if(const auto t=rendered(*c,L"TextBlock_Caption"))caption=*t;}
 Address owner=*top;
 auto owner_ref=top_ref;const Child* matched_child=nullptr;
 const auto open=unreal_.property(*top,L"ChildOpen")?unreal_.boolean(*top,L"ChildOpen"):std::optional<bool>(false);
 if(!open)return {};
 if(*open){
  // The top forwards input to its child; if that child cannot be proven, stay silent rather than read the background.
  if(!mode.forward)return {};
  const auto target=child(*top,mode.forward);if(!target)return {};
  owner_ref=unreal_.object(*target);if(!owner_ref)return {};
  const Child* match=nullptr;
  for(const auto& c:mode.children){const auto v=child(*top,c.field);if(v&&*v==*target&&exact(*target,c.cls)){match=&c;break;}}
  if(!match||!shown(*target)||!active(*target))return {};
  matched_child=match;
  owner=*target;
 }
 std::optional<MenuSnapshot> s;
 if(exact(owner,L"WD_DB_ChaData_C"))s=viewer(owner,*top,caption);
 else{
  if(!derives(owner,L"WD_Collect_ListBase_C")||unreal_.integer(owner,L"State")!=std::optional<int>(list_normal))return {};
  s=owner==*top&&*state==3?artwork(owner,caption):list(owner,*top,caption);
 }
 if(!s)return {};
 // Revalidate the complete input-owner chain after all text/help/visibility reads.
 if(get<Address>(unreal_.base()+engine_rva)!=engine||get<Address>(*engine+0x5d8)!=viewport||get<Address>(*viewport+0x80)!=world)return {};
 if(unreal_.integer(flow->address,L"State")!=state||unreal_.boolean(flow->address,L"IsTransition")!=std::optional<bool>(false)||child(flow->address,L"CurrentWidget")!=top)return {};
 const auto still_open=unreal_.property(*top,L"ChildOpen")?unreal_.boolean(*top,L"ChildOpen"):std::optional<bool>(false);
 if(still_open!=open||!active(*top)||!active(owner))return {};
 if(*open&&(!matched_child||child(*top,mode.forward)!=std::optional<Address>(owner)||child(*top,matched_child->field)!=std::optional<Address>(owner)))return {};
 if(!unreal_.alive(*flow)||!unreal_.alive(*top_ref)||!unreal_.alive(*owner_ref))return {};
 return *s;
}
}
