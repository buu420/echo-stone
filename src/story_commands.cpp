#include "mvci/localization.hpp"
#include "mvci/unreal.hpp"
#include "widget_geometry.hpp"
#include <cmath>
#include <cwchar>
// Pause Command List as a scrolled document (research/story-native/cmdlist/command-focus.md,
// research/story-commands/README.md). WD_CommandList_C has no selected row: focusIndex is only the scroll
// TARGET in [ListShowMax-ListMax,0]. Only whole rows inside the drawn 790px viewport at the current slot
// position are read; motion, rebuilds, unknown layout or unowned/ambiguous documents block.
namespace mvci {
// Source line of the last rejection (diagnostics/tests only; never spoken).
int story_commands_rejection_line=0;
namespace {
#define REJECT (story_commands_rejection_line=__LINE__,blocked)
using story_detail::Frame;
using namespace widget_geometry;
constexpr float row_height=130.f,viewport_height=790.f,tolerance=.5f;
constexpr int show_rows=6,max_rows=256;
std::uint64_t identity(const ObjectRef& r){return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(r.serial))<<32)|static_cast<std::uint32_t>(r.index);}
void join(std::wstring& out,const std::wstring& value,const wchar_t* sep=L". "){
 if(value.empty())return;if(!out.empty()){if(out.back()==L'.'||out.back()==L'!'||out.back()==L'?')out+=L" ";else out+=sep;}out+=value;
}
std::optional<float> scalar(Frame& f,Address object,const wchar_t* name){
 const auto p=f.u.property(object,name);if(!p||p->kind!=L"FloatProperty"||p->size!=4)return {};const auto v=f.get<float>(object+static_cast<Address>(p->offset));if(!v||!std::isfinite(*v))return {};return v;
}
std::optional<int> tick(Frame& f,Address w,bool require_settled){
 // EUITickState 0 Hidden, 1 Enter, 2 Active, 3 Exit; PreState marks a state change not yet ticked.
 const auto t=f.field(w,L"TickStateObj");const auto ref=t&&*t?f.u.object(*t):std::nullopt;
 if(!ref||!f.u.name_is(ref->cls,L"BP_TickStateObj_C")||f.field(*t,L"ParentObj")!=std::optional<Address>(w))return {};
 if(require_settled&&f.boolean(*t,L"PreState")!=std::optional<bool>(false))return {};return f.integer(*t,L"State");
}
bool untransformed(Frame& f,Address w){
 // Character changes replay _IN_Left/_IN_Right on CanvasPanel_Main.RenderTransform; only the rest pose is read.
 return near(number(f,w,L"RenderTransform",{L"Translation",L"X"}),0)&&near(number(f,w,L"RenderTransform",{L"Translation",L"Y"}),0)&&near(number(f,w,L"RenderTransform",{L"Scale",L"X"}),1)&&near(number(f,w,L"RenderTransform",{L"Scale",L"Y"}),1)
  &&near(number(f,w,L"RenderTransform",{L"Shear",L"X"}),0)&&near(number(f,w,L"RenderTransform",{L"Shear",L"Y"}),0)&&near(number(f,w,L"RenderTransform",{L"Angle"}),0);
}
std::optional<std::wstring> fstring(Frame& f,Address at){auto v=f.u.string(at);f.checks.push_back([&f,at,v]{return f.u.string(at)==v;});return v;}
std::optional<std::wstring> ftext(Frame& f,Address at){auto v=f.u.text(at);f.checks.push_back([&f,at,v]{return f.u.text(at)==v;});return v;}
std::optional<std::vector<std::wstring>> strings(Frame& f,Address owner,const wchar_t* name,const wchar_t* inner,int stride,int count){
 const auto a=f.array(owner,name,max_rows,inner,stride);if(!a||a->count!=count)return {};std::vector<std::wstring> out;
 for(int i=0;i<count;++i){const auto at=a->data+static_cast<Address>(i)*static_cast<Address>(stride);auto v=stride==16?fstring(f,at):ftext(f,at);if(!v)return {};out.push_back(*v);}return out;
}
bool category(const std::wstring& s){return s.size()>=2&&s.front()==L'<'&&s.back()==L'>';}
std::vector<std::wstring> tokens(const std::wstring& s){std::vector<std::wstring> out;std::wstring cur;for(const auto c:s){if(c==L'$'){if(!cur.empty())out.push_back(cur);cur.clear();}else cur+=c;}if(!cur.empty())out.push_back(cur);return out;}
std::optional<int> icon_code(const std::wstring& t){
 // setTextBlocks compares case-insensitively with "ff71".."ff86"; anything else is drawn as text.
 if(t.size()!=4)return {};int v=0;for(const auto c:t){int d=c>=L'0'&&c<=L'9'?c-L'0':c>=L'a'&&c<=L'f'?c-L'a'+10:c>=L'A'&&c<=L'F'?c-L'A'+10:-1;if(d<0)return {};v=v*16+d;}
 if(v<0xff71||v>0xff86)return {};return v-0xff71;
}
// Labels for tex_CmdIcon cells, reviewed visually from the decoded atlas (cmdlist/tex_CmdIcon_grid.png).
// Literal drawn shapes only; directions are as drawn (right-facing art). No mechanic is inferred.
const wchar_t* icon_label(int index){
 static const wchar_t* const labels[22]{
  L"quarter circle down to right",L"quarter circle down to left",L"Z-shaped motion ending right",L"Z-shaped motion ending left",
  L"half circle under ending right",L"half circle under ending left",L"full circle",L"up arrow",
  L"up-right arrow",L"right arrow",L"down-right arrow",L"down arrow",L"down-left arrow",L"left arrow",L"up-left arrow",L"light punch",
  L"heavy punch",L"light kick",L"heavy kick",L"double down arrow",L"Infinity Stone icon",L"tag icon"};
 return index>=0&&index<22?labels[index]:nullptr;
}
bool icon_material(Frame& f,Address resource,int index){
 if(!f.u.is_a(resource,L"MaterialInstance"))return false;
 wchar_t name[32];swprintf(name,32,L"mi_CmdIcon_ff%02x",0x71+index);
 const auto package=f.get<Address>(resource+0x20);
 return f.u.name_is(resource,name)&&package&&*package&&f.retain(*package)&&f.u.is_a(*package,L"Package")&&f.u.name_is(*package,std::wstring(L"/Game/UI/Menu/CommandList/Icon/")+name);
}
bool icon_resource(Frame& f,Address image,Address row,int index){
 const auto resource=f.image_resource(image);if(!resource)return false;
 const auto icons=f.array(row,L"MI_CmdIconArray",32);
 return icons&&icons->count==22&&f.get<Address>(icons->data+static_cast<Address>(index)*8)==resource&&icon_material(f,*resource,index);
}
// Drawn HorizontalBox_251 must be exactly spacer,item,spacer per token (visual order reversed for Arabic).
std::optional<std::wstring> command_line(Frame& f,Address row,const std::wstring& command){
 const auto box=f.field(row,L"HorizontalBox_251");if(!box||!*box||!f.u.is_a(*box,L"HorizontalBox")||!f.within(*box,row))return {};
 const auto arabic=f.boolean(row,L"isArabic");if(!arabic)return {};auto logical=tokens(command);auto drawn=logical;if(*arabic)std::reverse(drawn.begin(),drawn.end());
 const auto slots=f.array(*box,L"Slots",3*64);if(!slots||slots->count!=3*static_cast<int>(drawn.size()))return {};
 if(!drawn.empty()&&!f.shown(*box))return {};
 for(int i=0;i<slots->count;++i){
  const auto s=f.get<Address>(slots->data+static_cast<Address>(i)*8);if(!s||!*s||!f.retain(*s)||!f.u.is_a(*s,L"HorizontalBoxSlot"))return {};
  const auto c=f.field(*s,L"Content");if(!c||!*c||!slot_of(f,*c,*box))return {};
  const auto& t=drawn[static_cast<std::size_t>(i/3)];
  if(i%3!=1){if(!f.u.is_a(*c,L"Spacer"))return {};continue;}
  if(!f.shown(*c))return {};
  if(const auto code=icon_code(t)){if(!f.u.is_a(*c,L"Image")||!icon_resource(f,*c,row,*code))return {};}
  else{if(!f.u.is_a(*c,L"TextBlock"))return {};const auto v=f.display(*c);if(!v||*v!=t)return {};}
 }
 std::wstring out;for(const auto& t:logical){const auto code=icon_code(t);join(out,code?tr(icon_label(*code)):t,L", ");}return out;
}
std::optional<std::wstring> visible_text(Frame& f,Address owner,const wchar_t* name){const auto v=f.text(owner,name);if(!v||!story_detail::readable(*v))return {};return v;}
std::optional<std::wstring> row_text(Frame& f,Address row,bool is_category,const std::wstring& name,const std::wstring& command){
 if(!f.shown(row))return {};
 if(is_category){
  const auto v=visible_text(f,row,L"TextBlock_Caption");const auto p=f.u.property(row,L"CategoryName");
  if(!v||*v!=name||!p||p->kind!=L"TextProperty"||ftext(f,row+static_cast<Address>(p->offset))!=std::optional<std::wstring>(name))return {};return v;
 }
 const auto n=visible_text(f,row,L"TextBlock_Name");const auto np=f.u.property(row,L"CommandName"),cp=f.u.property(row,L"CommandText");
 if(!n||*n!=name||!np||np->kind!=L"TextProperty"||!cp||cp->kind!=L"StrProperty"||ftext(f,row+static_cast<Address>(np->offset))!=std::optional<std::wstring>(name)||fstring(f,row+static_cast<Address>(cp->offset))!=std::optional<std::wstring>(command))return {};
 const auto line=command_line(f,row,command);if(!line)return {};return line->empty()?*n:tr(L"{0}: {1}",{*n,*line});
}
// Row root: WidgetTree.RootWidget is a CanvasPanel whose single, non-collapsed, fixed-size child is docked
// vertically at Top 0, Bottom 130, so SConstraintCanvas desired height (and the VerticalBox step) is 130.
bool row_geometry(Frame& f,Address row){
 if(f.integer(row,L"Visibility")==std::optional<int>(1))return false;
 const auto tree=f.field(row,L"WidgetTree");if(!tree||!*tree)return false;const auto root=f.field(*tree,L"RootWidget");
 if(!root||!*root||!f.u.is_a(*root,L"CanvasPanel")||f.integer(*root,L"Visibility")==std::optional<int>(1))return false;
 const auto slots=f.array(*root,L"Slots",4);if(!slots||slots->count!=1)return false;const auto s=f.get<Address>(slots->data);if(!s||!*s||!f.retain(*s))return false;
 const auto c=f.field(*s,L"Content");if(!c||!*c||!slot_of(f,*c,*root)||f.integer(*c,L"Visibility")==std::optional<int>(1))return false;
 const auto b=anchor(f,*s);const auto autosize=f.boolean(*s,L"bAutoSize");
 return b&&autosize==std::optional<bool>(false)&&b->min_y==b->max_y&&(b->min_y==0||b->min_y==1)&&near(b->top,0)&&near(b->bottom,row_height);
}
struct Owner {Address menu=0;int count=0;bool ok=false;};
bool popup_attached(Frame& f,Address menu,Address list){
 const auto canvas=f.field(menu,L"CanvasPanel_Popup");
 const auto parent=f.field(list,L"ParentCanvas"), stored_slot=f.field(list,L"CanvasSlot");
 const auto slot=f.get<Address>(list+0x30);
 if(!canvas||parent!=canvas||!slot||stored_slot!=slot)return false;
 if(*canvas){
  return f.u.is_a(*canvas,L"CanvasPanel")&&f.within(*canvas,menu)&&slot_of(f,list,*canvas)==slot;
 }
 // PressBtn passes PopupCanvas to Activate. A null canvas attaches the
 // popup to the viewport through WD_UIParent.DuckAddToViewport.
 if(*slot)return false;
 const auto viewport=f.get<Address>(list+0x208), shared=f.get<Address>(list+0x210);
 if(!viewport||!*viewport||!shared||!*shared)return false;
 const auto references=f.get<int>(*shared+8);
 return references&&*references>0;
}
Owner owner(Frame& f,Address list){
 Owner out;
 for(const auto& m:f.u.instances(L"WD_MenuBase_C")){
  if(m.address==list)continue;const auto items=f.array(m.address,L"PopupItem",32);if(!items)continue;
  int at=-1;bool duplicate=false;
  for(int i=0;i<items->count;++i)if(f.get<Address>(items->data+static_cast<Address>(i)*8)==std::optional<Address>(list)){if(at<0)at=i;else duplicate=true;}
  if(at==-1)continue;++out.count;if(duplicate){out.ok=false;continue;}out.menu=m.address;
  // WD_MenuBase.PopupUpdate forwards input to the first PopupItem whose TickStateObj state is not Hidden.
  bool first=true;for(int i=0;i<at;++i){const auto e=f.get<Address>(items->data+static_cast<Address>(i)*8);if(!e||!*e||!f.retain(*e)||tick(f,*e,false)!=std::optional<int>(0))first=false;}
  out.ok=first&&f.retain(m.address)&&tick(f,m.address,true)==std::optional<int>(2)&&f.boolean(m.address,L"IsMenuBaseInput")==std::optional<bool>(true)&&f.shown(m.address)
   &&popup_attached(f,m.address,list);
 }
 if(out.count!=1)out.ok=false;return out;
}
std::optional<MenuSnapshot> document(Frame& f,Address list){
 if(!f.retain(list))return {};const auto state=tick(f,list,false);if(!state||*state==0||!f.shown(list))return {};
 MenuSnapshot blocked;blocked.blocked=true;
 if(*state!=2||tick(f,list,true)!=std::optional<int>(2)||!owner(f,list).ok)return REJECT;
 const auto main=f.field(list,L"CanvasPanel_Main");if(!main||!*main||!f.within(*main,list)||!f.shown(*main)||!untransformed(f,*main))return REJECT;
 const auto count=f.integer(list,L"ListMax"),show=f.integer(list,L"ListShowMax"),focus=f.integer(list,L"focusIndex");const auto step=scalar(f,list,L"ScrollInterval");
 if(!count||!show||!focus||*show!=show_rows||!near(step,row_height)||*count<=0||*count>max_rows)return REJECT;
 const int endpoint=*show-*count;
 if(*focus<std::min(0,endpoint)||*focus>std::max(0,endpoint))return REJECT;
 // Live geometry: VerticalBox_328 in its CanvasPanelSlot inside the declared 1600x790 viewport panel.
 const auto box=f.field(list,L"VerticalBox_328"),box_slot=f.field(list,L"VerticalBoxPanelSlot");if(!box||!*box||!box_slot||!*box_slot||!f.u.is_a(*box,L"VerticalBox")||!f.within(*box,list))return REJECT;
 const auto viewport=f.get<Address>(*box_slot+0x28);if(!viewport||!*viewport||!f.retain(*viewport)||!f.u.is_a(*viewport,L"CanvasPanel")||slot_of(f,*box,*viewport)!=box_slot)return REJECT;
 const auto view_slot=slot_of(f,*viewport,*main);const auto view=view_slot?anchor(f,*view_slot):std::nullopt;const auto at=anchor(f,*box_slot);
 if(!view||!at||view->min_x!=view->max_x||view->min_y!=view->max_y||!near(view->bottom,viewport_height)||f.boolean(*view_slot,L"bAutoSize")!=std::optional<bool>(false)||!f.shown(*viewport))return REJECT;
 if(f.boolean(*box_slot,L"bAutoSize")!=std::optional<bool>(true))return REJECT;
 if(!near(at->left,0)||at->min_x!=0||at->min_y!=0||at->max_x!=0||at->max_y!=0||at->align_x!=0||at->align_y!=0)return REJECT;
 const auto dx=number(f,list,L"DestinationPos",{L"X"}),dy=number(f,list,L"DestinationPos",{L"Y"});const float target=static_cast<float>(*focus)*row_height;
 // Tick eases 25%/frame toward DestinationPos; only a settled position on the current target is read.
 if(!near(dx,0)||!near(dy,target)||std::fabs(at->top-target)>tolerance)return REJECT;
 // Index identity: one VerticalBoxSlot per CmdTextArray entry, in order, class by "<>" category marker.
 const auto texts=strings(f,list,L"CmdTextArray",L"StrProperty",16,*count),names=strings(f,list,L"CmdNameArray",L"TextProperty",24,*count);if(!texts||!names)return REJECT;
 const auto slots=f.array(*box,L"Slots",max_rows);if(!slots||slots->count!=*count)return REJECT;
 std::vector<Address> rows;
 for(int i=0;i<*count;++i){
  const auto s=f.get<Address>(slots->data+static_cast<Address>(i)*8);if(!s||!*s||!f.retain(*s)||!f.u.is_a(*s,L"VerticalBoxSlot"))return REJECT;
  const auto row=f.field(*s,L"Content");const bool is_category=category((*texts)[static_cast<std::size_t>(i)]);
  if(!row||!*row||!slot_of(f,*row,*box)||!f.u.is_a(*row,is_category?L"WD_CmdListCategory_C":L"WD_CmdListButton_C")||!row_geometry(f,*row))return REJECT;
  for(const auto side:{L"Left",L"Top",L"Right",L"Bottom"})if(!near(number(f,*s,L"Padding",{side}),0))return REJECT;
  const auto rule=path(f,*s,L"Size",{L"SizeRule"},L"ByteProperty",1);if(!rule||f.get<std::uint8_t>(*rule)!=std::optional<std::uint8_t>(std::uint8_t{0}))return REJECT;
  rows.push_back(*row);
 }
 // Whole rows inside [0,790] at the actual position; the 10px remainder strip is never a row.
 const int first=std::max(0,static_cast<int>(std::ceil((-at->top-tolerance)/row_height)));
 const int last=std::min(*count-1,static_cast<int>(std::floor((viewport_height+tolerance-at->top)/row_height))-1);
 if(last-first+1!=std::min(*count,show_rows))return REJECT;
 std::wstring label;
 for(int i=first;i<=last;++i){
  const auto t=row_text(f,rows[static_cast<std::size_t>(i)],category((*texts)[static_cast<std::size_t>(i)]),(*names)[static_cast<std::size_t>(i)],(*texts)[static_cast<std::size_t>(i)]);if(!t)return REJECT;join(label,*t);
 }
 const auto character=visible_text(f,list,L"TextBlock_CharName");if(!character)return REJECT;
 std::wstring value=tr(L"Rows {0} to {1} of {2}",{std::to_wstring(first+1),std::to_wstring(last+1),std::to_wstring(*count)});
 for(const auto& [field,cue]:{std::pair<const wchar_t*,const wchar_t*>{L"Image_MoreUP",L"More above"},{L"Image_MoreBottom",L"More below"}}){
  const auto image=f.field(list,field);if(!image)return REJECT;if(*image&&f.within(*image,list)&&f.shown(*image))join(value,tr(cue));
 }
 std::wstring help;if(const auto notice=f.text(list,L"TextBlock_Notice");notice&&story_detail::readable(*notice))join(help,*notice);
 const auto shown_image=[&](const wchar_t* field)->Address{
  const auto image=f.field(list,field);
  return image&&*image&&f.u.is_a(*image,L"Image")&&f.within(*image,list)&&f.shown(*image)?*image:0;
 };
 if(shown_image(L"Image_ArrowL")&&shown_image(L"Image_ArrowR")){
  const auto previous=shown_image(L"Image_BtnNavi_LB"), next=shown_image(L"Image_BtnNavi_RB");
  if(previous&&next&&f.glyph(previous)==L"LB"&&f.glyph(next)==L"RB")join(help,tr(L"Switch character with {0} or {1}",{L"LB",L"RB"}));
  else join(help,tr(L"Switch character"));
 }
 join(help,f.hints(list,false));
 if(!f.finish())return REJECT;
 MenuSnapshot out;out.valid=true;out.identity=identity(*f.u.object(list));out.screen=tr(L"Command list: {0}",{*character});out.label=label;out.value=value;out.help=help;return out;
}
}
std::optional<std::wstring> story_detail::command_glyph(Frame& f,Address image){
 const auto resource=f.image_resource(image);
 if(!resource)return {};
 const auto name=f.u.name(*resource,true);
 if(name.size()<4)return {};
 const auto index=icon_code(name.substr(name.size()-4));
 if(!index||!icon_material(f,*resource,*index))return {};
 return tr(icon_label(*index));
}
MenuSnapshot story_commands(Unreal& u,const std::function<std::optional<int>(Address)>& visibility){
 MenuSnapshot out;
 for(const auto& list:u.instances(L"WD_CommandList_C")){
  Frame f{u,visibility,{},{},{}};const auto d=document(f,list.address);if(!d)continue;
  // A second drawn document (or an unreadable one) never yields to background text.
  if(out.valid||out.blocked){MenuSnapshot b;b.blocked=true;return b;}out=*d;
 }
 return out;
}
}
