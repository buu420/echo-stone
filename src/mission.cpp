#include "mvci/mission.hpp"
#include "mvci/localization.hpp"
#include "widget_geometry.hpp"
#include <algorithm>
#include <set>
namespace mvci {
namespace {
constexpr Address engine_rva=0x36bc348;
bool clean(const Unreal& u,Address a){const auto o=u.object(a);return o&&!(o->flags&0x30);}
bool derives(const Unreal& u,Address a,const wchar_t* wanted){
 const auto o=u.object(a);if(!o||(o->flags&0x30))return false;
 Address c=o->cls;std::set<Address> seen;
 while(c&&seen.size()<64&&seen.insert(c).second){if(!u.object(c))return false;if(u.name_is(c,wanted))return true;const auto next=u.memory().get<Address>(c+0x30);if(!next)return false;c=*next;}return false;
}
bool readable(const std::wstring& s){
 if(s.empty()||s.rfind(L"msg_",0)==0||is_umg_default_text(s))return false;
 const wchar_t* const templates[]{L"CLEARED X",L"CLEARED 0",L"操作",L"MISSION X",L"Mission Data Mission Data Mission Data"};
 return std::none_of(std::begin(templates),std::end(templates),[&](const wchar_t* t){return s==t;});
}
void sentence(std::wstring& out,const std::wstring& part){if(part.empty())return;if(!out.empty()){const auto last=out.back();out+=(last==L'.'||last==L'!'||last==L'?')?L" ":L". ";}out+=part;}
std::uint64_t identity(const ObjectRef& ref){return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(ref.serial))<<32)|static_cast<std::uint32_t>(ref.index);}
// Remember each authoritative read and its object serial. Visibility can invoke
// native bindings; replay visibility first, then fence all data and identities.
// This also protects the external read-only diagnostic from concurrent changes.
struct Frame : story_detail::Frame {
 std::optional<std::wstring> line(Address box,Address popup){
  if(!u.is_a(box,L"HorizontalBox")||!within(box,popup))return {};
  const auto slots=array(box,L"Slots",64);if(!slots)return {};std::wstring out;
  for(int i=0;i<slots->count;++i){
   const auto slot=get<Address>(slots->data+static_cast<Address>(i)*8);if(!slot||!*slot||!retain(*slot))return {};
   const auto content=field(*slot,L"Content");if(!content||!*content||get<Address>(*slot+0x28)!=std::optional<Address>(box)||get<Address>(*content+0x30)!=slot||!within(*content,box))return {};
   if(!shown(*content))continue;std::wstring piece;
   if(u.is_a(*content,L"TextBlock")){const auto t=display(*content);if(!t||(!t->empty()&&!readable(*t)))return {};piece=*t;}
   else if(u.is_a(*content,L"Image"))piece=glyph(*content);
   else if(u.is_a(*content,L"Spacer"))continue;
   else return {};
   if(!piece.empty()){if(!out.empty())out+=L" ";out+=piece;}if(out.size()>8192)return {};
  }return out;
 }
 std::optional<std::wstring> tutorial(Address popup){
  const auto panel=field(popup,L"CanvasPanel_PCText");if(!panel||!*panel||!within(*panel,popup))return {};
  const bool pc=shown(*panel);std::wstring out;
  // Both sets can retain children, so gate every actual box and reject a
  // conflicting visible platform set rather than expose its retained text.
  for(int set=0;set<2;++set)for(int i=0;i<7;++i){
   const std::wstring name=L"HorizontalBox_"+std::to_wstring((set?65:0)+i);const auto box=field(popup,name.c_str());if(!box)return {};if(!*box)continue;
   if(!shown(*box))continue;if((set==0)!=pc)return {};
   const auto text=line(*box,popup);if(!text)return {};if(!text->empty()){if(!out.empty())out+=L" ";out+=*text;}if(out.size()>16384)return {};
  }if(out.empty())return {};return out;
 }
};
struct Page {const wchar_t* array;const wchar_t* focus;const wchar_t* column;const wchar_t* row_class;};
const Page pages[]{
 {L"ListCharArray_C",L"IndexFocus_C",L"IndexFocusX_C",L"WD_MissionListChr_C"},
 {L"ListCharArray_M",L"IndexFocus_M",L"IndexFocusX_M",L"WD_MissionListChr_C"},
 {L"ListNameArray",L"IndexFocus_All",L"IndexFocusX_All",L"WD_MissionListName_C"},
 {L"ListCharArray_T",L"IndexFocus_T",L"IndexFocusX_T",L"WD_MissionListChr_C"}};

std::optional<Address> owned_widget(Frame& frame,Address owner,const wchar_t* name,const wchar_t* type,Address parent){
 const auto widget=frame.field(owner,name);
 if(!widget||!*widget||!frame.u.is_a(*widget,type)||!frame.within(*widget,parent))return {};
 return widget;
}
MenuSnapshot battle_tutorial(Frame& frame,Address window){
 MenuSnapshot pending;pending.modal=true;pending.blocked=true;
 if(!frame.active(window))return pending;
 const auto panel=owned_widget(frame,window,L"WD_MissionPopupTTL_213",L"WD_MissionPopupTTL_C",window);
 if(!panel||!frame.shown(*panel))return pending;
 const auto number=frame.text(*panel,L"TextBlock_Number");
 const auto instructions=frame.tutorial(*panel);
 const auto ref=frame.u.object(window);
 if(!instructions||!ref)return pending;
 MenuSnapshot result;result.valid=true;result.modal=true;result.identity=identity(*ref);
 result.screen=tr(L"Mission tutorial");
 if(number&&readable(*number)&&*number!=L"MISSION 0"){
  result.label=*number;result.help=*instructions;
 }else result.label=*instructions;
 const auto guide=owned_widget(frame,window,L"WD_BtnNavi_98",L"WD_BtnNavi_C",window);
 if(guide&&frame.shown(*guide))sentence(result.help,frame.hints(*guide,true));
 return result;
}
std::optional<std::wstring> visible_hud_text(Frame& frame,Address hud,Address panel,const wchar_t* field){
 const auto block=owned_widget(frame,hud,field,L"TextBlock",panel);
 if(!block||!frame.shown(*block))return {};
 const auto alpha=frame.text_alpha(*block);if(!alpha||*alpha<=0)return {};
 const auto text=frame.display(*block);
 if(!text||!readable(*text)||*text==L"MISSION 0")return {};
 return text;
}
std::wstring battle_objective(Frame& frame,Address hud){
 std::wstring text;
 const auto header=owned_widget(frame,hud,L"CanvasPanel_MissionSpec",L"CanvasPanel",hud);
 if(header&&frame.shown(*header)){
  const auto description=visible_hud_text(frame,hud,*header,L"TextBlock_Spec");
  if(description){
   if(const auto number=visible_hud_text(frame,hud,*header,L"TextBlock_Number"))sentence(text,*number);
   sentence(text,*description);
  }
 }
 const auto condition=owned_widget(frame,hud,L"CanvasPanel_PosCondition",L"CanvasPanel",hud);
 if(condition&&frame.shown(*condition))if(const auto detail=visible_hud_text(frame,hud,*condition,L"TextBlock_PosCondition"))sentence(text,*detail);
 return text;
}

constexpr float mission_row_height=52.f, mission_list_height=574.f;
bool top_anchored(const widget_geometry::Box& box){
 return box.min_y==0 && box.max_y==0 && box.align_y==0;
}
std::optional<widget_geometry::Box> child_layout(Frame& frame,Address child,Address parent,bool auto_size){
 const auto slot=widget_geometry::slot_of(frame,child,parent);
 if(!slot||frame.boolean(*slot,L"bAutoSize")!=std::optional<bool>(auto_size))return {};
 return widget_geometry::anchor(frame,*slot);
}
bool fixed_mission_row(Frame& frame,Address slot){
 if(!frame.u.is_a(slot,L"VerticalBoxSlot"))return false;
 const auto rule=widget_geometry::path(frame,slot,L"Size",{L"SizeRule"},L"ByteProperty",1);
 if(!rule||frame.get<std::uint8_t>(*rule)!=std::optional<std::uint8_t>(std::uint8_t{0}))return false;
 for(const auto side:{L"Left",L"Top",L"Right",L"Bottom"}){
  if(!widget_geometry::near(widget_geometry::number(frame,slot,L"Padding",{side}),0.f))return false;
 }
 return true;
}
std::optional<std::wstring> rendered_mission_command(Frame& frame,Address row,Address panel){
 const auto text=visible_hud_text(frame,row,panel,L"TextBlock_CmdText");
 const auto content=owned_widget(frame,row,L"CanvasPanel_Content",L"CanvasPanel",panel);
 if(!content)return {};
 if(!frame.shown(*content))return text;
 if(text)return {}; // The two layouts are mutually exclusive in switchDispType.

 const auto slots=frame.array(*content,L"Slots",64);
 if(!slots)return {};
 std::wstring command;
 // setCommand inserts each displayed token left to right in Canvas slot order.
 for(int i=0;i<slots->count;++i){
  const auto slot=frame.get<Address>(slots->data+static_cast<Address>(i)*8);
  if(!slot||!*slot||!frame.retain(*slot))return {};
  const auto token=frame.field(*slot,L"Content");
  if(!token||!*token||widget_geometry::slot_of(frame,*token,*content)!=slot)return {};
  if(!frame.shown(*token))continue;
  std::optional<std::wstring> part;
  if(frame.u.is_a(*token,L"TextBlock")){
   const auto alpha=frame.text_alpha(*token);
   if(!alpha)return {};
   if(*alpha==0)continue;
   part=frame.display(*token);
  }else if(frame.u.is_a(*token,L"Image")){
   part=story_detail::command_glyph(frame,*token).value_or(tr(L"(move icon)"));
  }
  if(!part||!readable(*part))return {};
  if(!command.empty())command+=L" ";
  command+=*part;
  if(command.size()>4096)return {};
 }
 return command;
}
bool current_command(Frame& frame,Address row,Address panel){
 const auto image=owned_widget(frame,row,L"Image_Judge",L"Image",panel);
 if(!image||!frame.shown(*image))return false;
 const auto color=frame.u.property(*image,L"ColorAndOpacity");
 if(!color||color->kind!=L"StructProperty"||color->size!=16)return false;
 // switchFocus paints only the current row with this yellow marker.
 const auto value=frame.get<std::array<float,4>>(*image+static_cast<Address>(color->offset));
 return value&&*value==std::array<float,4>{1.f,0.875823974609375f,0.f,1.f};
}
struct CommandPane {
 Address box=0, view_slot=0, box_slot=0;
 ArrayView rows;
 std::uint64_t identity=0;
};
std::optional<CommandPane> command_pane(Frame& frame,Address hud){
 const auto main=owned_widget(frame,hud,L"CanvasPanel_ListMain",L"CanvasPanel",hud);
 if(!main||!frame.shown(*main))return {};
 const auto view=owned_widget(frame,hud,L"CanvasPanel_List",L"CanvasPanel",*main);
 if(!view||!frame.shown(*view))return {};
 const auto box=owned_widget(frame,hud,L"VerticalBox_List",L"VerticalBox",*view);
 if(!box||!frame.shown(*box))return {};
 const auto view_slot=widget_geometry::slot_of(frame,*view,*main), box_slot=widget_geometry::slot_of(frame,*box,*view);
 if(!view_slot||!box_slot)return {};
 const auto top=widget_geometry::number(frame,*box_slot,L"LayoutData",{L"Offsets",L"Top"});
 const auto rows=frame.array(*box,L"Slots",128);
 const auto ref=frame.u.object(*box);
 const auto index=frame.integer(hud,L"checkIndex"), progress=frame.integer(hud,L"ProgressOld");
 if(!top||!rows||rows->count==0||*top>0||*top<=-mission_row_height*static_cast<float>(rows->count)||!ref||!index||!progress)return {};
 int row_visibility=-1;
 if(*index>=0&&*index<rows->count){
  const auto slot=frame.get<Address>(rows->data+static_cast<Address>(*index)*8);
  if(slot&&*slot&&frame.retain(*slot)){
   const auto row=frame.field(*slot,L"Content");
   if(row&&*row&&frame.u.is_a(*row,L"WD_MissionCmd_C")){
    const auto panel=frame.field(*row,L"CanvasPanel_1");
    if(panel&&*panel)row_visibility=frame.integer(*panel,L"Visibility").value_or(-1);
   }
  }
 }
 // A requested reading belongs to this pane and scroll position. These few
 // reads also let ordinary captures retire speech after the demand is released.
 // The row Success and switchFocus producers update these counters even when
 // the list stays put. They can invalidate a reading, but supply no speech.
 const std::uint64_t keys[]{identity(*ref),*view_slot,*box_slot,rows->data,
  static_cast<std::uint64_t>(rows->count),static_cast<std::uint64_t>(std::lround(-*top*16.f)),
  static_cast<std::uint32_t>(*index),static_cast<std::uint32_t>(*progress),static_cast<std::uint32_t>(row_visibility)};
 std::uint64_t key=14695981039346656037ull;
 for(const auto value:keys)key=(key^value)*1099511628211ull;
 return CommandPane{*box,*view_slot,*box_slot,*rows,key};
}
std::wstring battle_commands(Frame& frame,const CommandPane& pane){
 const auto bounds=widget_geometry::anchor(frame,pane.view_slot), position=widget_geometry::anchor(frame,pane.box_slot);
 if(!bounds||!position||!top_anchored(*bounds)||!top_anchored(*position)||
    bounds->bottom!=mission_list_height||position->left!=0||
    frame.boolean(pane.view_slot,L"bAutoSize")!=std::optional<bool>(false)||
    frame.boolean(pane.box_slot,L"bAutoSize")!=std::optional<bool>(true))return {};

 std::wstring commands;
 bool marked=false;
 for(int i=0;i<pane.rows.count;++i){
  const float top=position->top+mission_row_height*static_cast<float>(i);
  if(top+mission_row_height>bounds->bottom)break;
  const auto slot=frame.get<Address>(pane.rows.data+static_cast<Address>(i)*8);
  if(!slot||!*slot||!frame.retain(*slot)||!fixed_mission_row(frame,*slot))return {};
  const auto row=frame.field(*slot,L"Content");
  if(!row||!*row||!frame.u.is_a(*row,L"WD_MissionCmd_C")||widget_geometry::slot_of(frame,*row,pane.box)!=slot)return {};
  const auto visibility=frame.integer(*row,L"Visibility");
  if(!visibility||*visibility==1)return {}; // Collapsed rows invalidate the fixed layout.
  const auto panel=owned_widget(frame,*row,L"CanvasPanel_1",L"CanvasPanel",*row);
  if(!panel)return {};
  const auto parent_slot=frame.get<Address>(*panel+0x30);
  if(!parent_slot||!*parent_slot||!frame.retain(*parent_slot))return {};
  const auto parent=frame.get<Address>(*parent_slot+0x28);
  if(!parent||!*parent||!frame.within(*parent,*row))return {};
  const auto layout=child_layout(frame,*panel,*parent,false);
  if(!layout||!top_anchored(*layout)||layout->top!=0||layout->bottom!=mission_row_height||
     frame.integer(*panel,L"Visibility")==std::optional<int>(1))return {};
  if(top<0||!frame.shown(*panel))continue;
  const auto command=rendered_mission_command(frame,*row,*panel);
  if(!command||command->empty())continue;
  const bool selected=current_command(frame,*row,*panel);
  if(selected&&marked)return {};
  marked|=selected;
  sentence(commands,selected?tr(L"Current command. {0}",{*command}):*command);
  if(commands.size()>16384)return {};
 }
 if(commands.empty())return {};
 return tr(L"Mission commands. {0}",{commands});
}
}
MissionBattleSnapshot MissionCapture::battle(bool read_hud,bool read_commands)const{
 const auto flows=unreal_.instances(L"GF_FightMission_C");
 if(flows.empty())return {};
 const auto world=unreal_.current_world();if(!world)return {};
 Frame frame{{unreal_,visibility_,{},{},{}}};
 Address flow=0;
 for(const auto& candidate:flows){
  if(!unreal_.alive(candidate)||!frame.retain(candidate.address))continue;
  const auto level=frame.get<Address>(candidate.address+0x20);
  if(!level||!*level||!frame.retain(*level)||!unreal_.is_a(*level,L"Level")||frame.get<Address>(*level+0xb0)!=world)continue;
  if(flow)return {};
  flow=candidate.address;
 }
 if(!flow)return {};
 MissionBattleSnapshot result;
 const auto window=frame.field(flow,L"WD_TutorialWindow");
 if(window&&*window&&unreal_.is_a(*window,L"WD_TutorialWindow_C")&&frame.shown(*window)){
  result.menu=battle_tutorial(frame,*window);
  if(!frame.finish()||unreal_.current_world()!=world){result.menu={};result.menu.modal=true;result.menu.blocked=true;}
  return result;
 }
 if(!read_hud)return {};
 const auto hud=frame.field(flow,L"MissionHud");
 if(!hud||!*hud||!unreal_.is_a(*hud,L"WD_MissionHUD_C")||!frame.shown(*hud)||!frame.active(*hud))return {};
 const auto ref=unreal_.object(*hud);if(!ref)return {};
 const auto objective=battle_objective(frame,*hud);
 const auto pane=command_pane(frame,*hud);
 const auto commands=read_commands&&pane?battle_commands(frame,*pane):std::wstring{};
 if(!frame.finish()||unreal_.current_world()!=world)return {};
 result.hud_identity=identity(*ref);
 if(pane)result.command_view=pane->identity;
 if(!objective.empty())result.objective={true,result.hud_identity,objective};
 if(!commands.empty())result.commands={true,result.hud_identity,commands};
 return result;
}
MenuSnapshot MissionCapture::sample(std::uint64_t now){
 rejection_.clear();const auto count=unreal_.object_count();if(!count)return {};
 Frame frame{{unreal_,visibility_,{},{},{}}};
 const auto engine=frame.get<Address>(unreal_.base()+engine_rva);
 const auto viewport=engine&&*engine?frame.get<Address>(*engine+0x5d8):std::nullopt;
 const auto world=viewport&&*viewport?frame.get<Address>(*viewport+0x80):std::nullopt;
 if(!world||!*world||!frame.retain(*world)||!unreal_.is_a(*world,L"World"))return {};
 const bool retired=std::any_of(flows_.objects.begin(),flows_.objects.end(),[this](const ObjectRef& o){return !unreal_.alive(o);});
 if(unreal_.shared_discovery()||!flows_.filled||flows_.count!=*count||retired||now<flows_.at||now-flows_.at>=500){flows_.objects=unreal_.instances(L"GF_Mission_C");flows_.at=now;flows_.count=*count;flows_.filled=true;}
 std::optional<ObjectRef> flow;bool ambiguous=false;
 for(const auto& o:flows_.objects){
  if(!unreal_.alive(o)||!clean(unreal_,o.address))continue;
  const auto level=frame.get<Address>(o.address+0x20);if(!level||!*level||!unreal_.is_a(*level,L"Level"))continue;
  const auto owning=frame.get<Address>(*level+0xb0);if(!owning||*owning!=*world)continue;
  if(flow)ambiguous=true;flow=o;
 }
 if(!flow)return {};
 MenuSnapshot pending;pending.blocked=true;
 const auto fail=[this,&pending](const wchar_t* reason){rejection_=reason;return pending;};
 if(ambiguous)return fail(L"mission: ambiguous current flow");
 if(!frame.retain(flow->address)||frame.boolean(flow->address,L"IsTravel")!=std::optional<bool>(false))return fail(L"mission: flow is travelling");
 const auto root=frame.field(flow->address,L"MissionMain");
 if(!root||!*root||!derives(unreal_,*root,L"WD_MissionMain_C")||!frame.shown(*root)||!frame.active(*root))return fail(L"mission: main menu is not active");
 const auto page=frame.integer(*root,L"ViewState");if(!page||*page<0||*page>3)return fail(L"mission: unsupported page");
 const Page& p=pages[*page];const auto focus=frame.integer(*root,p.focus),column=frame.integer(*root,p.column);
 const auto rows=frame.array(*root,p.array,256);
 if(!focus||!column||*column<0||!rows||*focus<0||*focus>=rows->count)return fail(L"mission: invalid focus or rows");
 const auto max_c=frame.integer(*root,L"ListMax_C"),max_m=frame.integer(*root,L"ListMax_M"),max_t=frame.integer(*root,L"ListMax_T");
 if(!max_c||!max_m||!max_t||*max_c< -1||*max_m< -1||*max_t<0||*max_c>255||*max_m>255||*max_t>255)return fail(L"mission: invalid list bounds");
 const int expected_count=*page==0?*max_c+1:*page==1?*max_m+1:*page==2?*max_c+*max_m+2:*max_t+1;
 if(rows->count!=expected_count)return fail(L"mission: rows not initialized");
 int missions=10,number_index=*column;
 if(*page==3){
  const auto stages=frame.array(*root,L"ArrTutorialStageNum",256,L"IntProperty",4);if(!stages||stages->count!=rows->count)return fail(L"mission: tutorial stages not initialized");
  const auto n=frame.get<int>(stages->data+static_cast<Address>(*focus)*4);if(!n||*n<=0||*n>10)return fail(L"mission: invalid tutorial stage count");missions=*n;
  // getFocusMission @1420..1884 adds preceding category lengths before
  // passing the tutorial number to the popup (Blocking slot 2 is MISSION 9).
  for(int i=0;i<*focus;++i){const auto before=frame.get<int>(stages->data+static_cast<Address>(i)*4);if(!before||*before<=0||*before>10)return fail(L"mission: invalid preceding tutorial stage count");number_index+=*before;}
 }
 if(*column>=missions)return fail(L"mission: invalid mission slot");
 const auto row=frame.get<Address>(rows->data+static_cast<Address>(*focus)*8);
 if(!row||!*row||!frame.retain(*row)||!derives(unreal_,*row,p.row_class)||frame.field(*row,L"MissionMain")!=root||!frame.within(*row,*root)||!frame.shown(*row))return fail(L"mission: focused row is not drawn by this menu");
 const auto caption=frame.text(*root,L"TextBlock_ViewCaption"),label=frame.text(*row,*page==3?L"TextBlock_Command":L"TextBlock_CharName");
 if(!caption||!label||!readable(*caption)||!readable(*label))return fail(L"mission: focused text is not ready");
 const auto clear=frame.text(*row,L"TextBlock_Clear");if(!clear||!readable(*clear))return fail(L"mission: progress text is not ready");
 const wchar_t* const cursors[]{L"Image_Cursor_C",L"Image_Cursor_M",L"Image_Cursor_All",L"Image_Cursor_T"};
 const auto cursor=frame.field(*root,cursors[*page]);const auto chips=frame.array(*row,L"ProgressPartsArray",10);
 if(!cursor||!*cursor||!unreal_.is_a(*cursor,L"Image")||!frame.within(*cursor,*root)||!frame.shown(*cursor)||!chips||*column>=chips->count)return fail(L"mission: cursor or mission chip is not drawn");
 const auto chip=frame.get<Address>(chips->data+static_cast<Address>(*column)*8);
 if(!chip||!*chip||!frame.retain(*chip)||!unreal_.is_a(*chip,L"Image")||!frame.within(*chip,*row)||!frame.shown(*chip))return fail(L"mission: selected mission chip is not drawn");
 const auto decide=frame.boolean(*root,L"isDecide"),suppressed=frame.boolean(*root,L"suppressPopup");if(!decide||!suppressed)return fail(L"mission: popup state is unreadable");
 MenuSnapshot s;s.valid=true;s.modal=*decide;s.screen=tr(L"Missions. {0}",{*caption});s.label=*label;s.position=*focus+1;s.count=rows->count;
 pending.modal=*decide;
 const auto notice=frame.field(*root,L"WD_KeyConfAttention_272");
 bool notice_shown=false;
 if(notice&&*notice&&frame.shown(*notice)){
  if(!derives(unreal_,*notice,L"WD_KeyConfAttention_C")||!frame.within(*notice,*root))return fail(L"mission: invalid download notice");
  const auto text=frame.text(*notice,L"TextBlock_Label");if(!text||!readable(*text))return fail(L"mission: download notice not ready");sentence(s.help,*text);s.enabled=false;notice_shown=true;
 }
 const auto popup=frame.field(*root,*page==3?L"WD_MissionPopupTTL":L"WD_MissionPopup");
 bool popup_shown=false;
 // OpenPopup(true) explicitly bypasses suppressPopup; confirmation can be
 // visible while the ordinary preview remains configured as hidden.
 if((*decide||!*suppressed)&&!notice_shown&&popup&&*popup&&frame.shown(*popup)){
  if(!derives(unreal_,*popup,*page==3?L"WD_MissionPopupTTL_C":L"WD_MissionPopup_C")||!frame.within(*popup,*root))return fail(L"mission: wrong details popup");
  const auto number=frame.text(*popup,L"TextBlock_Number");if(!number||!readable(*number))return fail(L"mission: mission number not ready");
  const auto digits=std::to_wstring(number_index+1),suffix=L" "+digits,prefix=digits+L" ";
  if(number->rfind(prefix,0)!=0&&(number->size()<suffix.size()||number->compare(number->size()-suffix.size(),suffix.size(),suffix)!=0))return fail(L"mission: popup number does not match cursor");
  s.value=*number;
  const auto spec=*page==3?frame.tutorial(*popup):frame.text(*popup,L"TextBlock_Spec");
  if(!spec||!readable(*spec))return fail(L"mission: mission details not ready");sentence(s.help,*spec);
  if(const auto completed=frame.text(*popup,L"TextBlock_Cleared");completed){if(!readable(*completed))return fail(L"mission: completion text not ready");sentence(s.value,*completed);}
  popup_shown=true;
 }
 if(!popup_shown)s.value=tr(L"Mission {0} of {1}",{std::to_wstring(*column+1),std::to_wstring(missions)}); // Visible cursor/chip, not a save-data lookup.
 sentence(s.value,*clear);
 if(*decide){
  if(!popup_shown)return fail(L"mission: confirmation details not ready");
  const auto panel=frame.field(*popup,L"CanvasPanel_ButtonNavi");if(!panel||!*panel||!frame.within(*panel,*popup)||!frame.shown(*panel))return fail(L"mission: confirmation guide not ready");
  const auto hints=frame.hints(*popup,false);if(hints.empty())return fail(L"mission: confirmation actions not ready");sentence(s.help,hints);
 }else{
  const auto guide=frame.field(*root,L"WD_BtnNavi_176");if(guide&&*guide&&frame.within(*guide,*root)&&frame.shown(*guide))sentence(s.help,frame.hints(*guide,true));
 }
 if(!frame.finish())return fail(L"mission: state changed during capture");
 const auto ref=unreal_.object(*root);if(!ref)return fail(L"mission: menu retired");
 s.identity=((static_cast<std::uint64_t>(static_cast<std::uint32_t>(ref->serial))<<32)|static_cast<std::uint32_t>(ref->index))^0x4d00000000000000ull^
  (static_cast<std::uint64_t>(*page)<<56)^(static_cast<std::uint64_t>(*focus)<<16)^(static_cast<std::uint64_t>(*column)<<1)^static_cast<std::uint64_t>(*decide);
 return s;
}
}
