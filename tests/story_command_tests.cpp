// Story Command List document reader (research/story-native/cmdlist/command-focus.md,
// research/story-commands/README.md). Hand-encoded UE4.15 layouts; Blueprint fields via reflection.
#include "collection_fixture.hpp"
#include <cwchar>
#include <iostream>
namespace mvci {MenuSnapshot story_commands(Unreal&,const std::function<std::optional<int>(Address)>&);extern int story_commands_rejection_line;}
using namespace mvci;
static int checks=0;
#define CHECK(x) do{++checks;if(!(x))throw std::runtime_error(std::string(__func__)+":"+std::to_string(__LINE__)+" " #x+" (reader line "+std::to_string(mvci::story_commands_rejection_line)+")");}while(false)
struct Row {std::wstring name,text;};
struct CommandFixture:mvci_test::Fixture {
 Address v2=0,margin=0,anchors=0,anchor_data=0,child_size=0,transform=0;
 Address tree_class=0,panel_class=0,slot_class=0,canvas_slot=0,vbox_slot=0,hbox_slot=0,vbox=0,hbox=0,image=0,spacer=0,mi=0;
 Address menu_class=0,pause_class=0,list_class=0,button_class=0,category_class=0,guide_class=0;
 Address pause=0,popup=0,list=0,main=0,viewport=0,box=0,box_slot=0,more_up=0,more_down=0;
 std::vector<Address> rows;std::vector<Address> icons;
 Address member(Address s,const wchar_t* n,const wchar_t* k,int o,int size,Address sub=0){auto p=prop_at(s,n,k,o,size);if(sub)put(p+0x78,sub);return p;}
 Address structure(const wchar_t* n){return object(n,kind(L"ScriptStruct"),0x100);}
 Address struct_prop(Address c,const wchar_t* n,Address s,int size){auto p=prop(c,n,L"StructProperty",size);put(p+0x78,s);while(next_offset<raw<int>(p+0x50)+size)next_offset+=0x20;return p;}
 CommandFixture(){
  v2=structure(L"Vector2D");member(v2,L"X",L"FloatProperty",0,4);member(v2,L"Y",L"FloatProperty",4,4);
  margin=structure(L"Margin");member(margin,L"Left",L"FloatProperty",0,4);member(margin,L"Top",L"FloatProperty",4,4);member(margin,L"Right",L"FloatProperty",8,4);member(margin,L"Bottom",L"FloatProperty",12,4);
  anchors=structure(L"Anchors");member(anchors,L"Minimum",L"StructProperty",0,8,v2);member(anchors,L"Maximum",L"StructProperty",8,8,v2);
  anchor_data=structure(L"AnchorData");member(anchor_data,L"Offsets",L"StructProperty",0,16,margin);member(anchor_data,L"Anchors",L"StructProperty",16,16,anchors);member(anchor_data,L"Alignment",L"StructProperty",32,8,v2);
  child_size=structure(L"SlateChildSize");member(child_size,L"Value",L"FloatProperty",0,4);member(child_size,L"SizeRule",L"ByteProperty",4,1);
  transform=structure(L"WidgetTransform");member(transform,L"Translation",L"StructProperty",0,8,v2);member(transform,L"Scale",L"StructProperty",8,8,v2);member(transform,L"Shear",L"StructProperty",16,8,v2);member(transform,L"Angle",L"FloatProperty",24,4);
  struct_prop(cls[L"Widget"],L"RenderTransform",transform,28);
  tree_class=klass(L"WidgetTree");prop(tree_class,L"RootWidget",L"ObjectProperty",8);prop(cls[L"UserWidget"],L"WidgetTree",L"ObjectProperty",8);
  panel_class=klass(L"PanelWidget",cls[L"Widget"]);arr(panel_class,L"Slots",L"ObjectProperty",8);
  put(cls[L"CanvasPanel"]+0x30,panel_class);vbox=klass(L"VerticalBox",panel_class);hbox=klass(L"HorizontalBox",panel_class);
  slot_class=klass(L"PanelSlot");prop_at(slot_class,L"Parent",L"ObjectProperty",0x28,8);prop_at(slot_class,L"Content",L"ObjectProperty",0x30,8);
  canvas_slot=klass(L"CanvasPanelSlot",slot_class);struct_prop(canvas_slot,L"LayoutData",anchor_data,40);prop(canvas_slot,L"bAutoSize",L"BoolProperty",1);
  vbox_slot=klass(L"VerticalBoxSlot",slot_class);struct_prop(vbox_slot,L"Padding",margin,16);struct_prop(vbox_slot,L"Size",child_size,8);
  hbox_slot=klass(L"HorizontalBoxSlot",slot_class);
  image=klass(L"Image",cls[L"Widget"]);{auto s=structure(L"SlateBrush");prop_at(s,L"ResourceObject",L"ObjectProperty",8,8);auto p=prop_at(image,L"Brush",L"StructProperty",0x140,32);put(p+0x78,s);}
  spacer=klass(L"Spacer",cls[L"Widget"]);mi=klass(L"MaterialInstanceConstant",klass(L"MaterialInstance",klass(L"MaterialInterface")));
  menu_class=cls[L"WD_MenuBase_C"];prop(menu_class,L"IsMenuBaseInput",L"BoolProperty",1);arr(menu_class,L"PopupItem",L"ObjectProperty",8);prop(menu_class,L"CanvasPanel_Popup",L"ObjectProperty",8);
  prop(cls[L"WD_UIParent_C"],L"ParentCanvas",L"ObjectProperty",8);prop(cls[L"WD_UIParent_C"],L"CanvasSlot",L"ObjectProperty",8);
  pause_class=klass(L"WD_PauseMenu_Story_C",menu_class);
  guide_class=klass(L"WD_BtnNavi_C",cls[L"UserWidget"]);arr(guide_class,L"TextArray",L"StrProperty",16);
  list_class=klass(L"WD_CommandList_C",menu_class);
  for(auto n:{L"focusIndex",L"ListMax",L"ListShowMax"})prop(list_class,n,L"IntProperty",4);prop(list_class,L"ScrollInterval",L"FloatProperty",4);struct_prop(list_class,L"DestinationPos",v2,8);
  arr(list_class,L"CmdTextArray",L"StrProperty",16);arr(list_class,L"CmdNameArray",L"TextProperty",24);
  for(auto n:{L"VerticalBoxPanelSlot",L"VerticalBox_328",L"TextBlock_CharName",L"TextBlock_Notice",L"Image_MoreUP",L"Image_MoreBottom",L"CanvasPanel_Main",L"WD_BtnNavi"})prop(list_class,n,L"ObjectProperty",8);
  button_class=klass(L"WD_CmdListButton_C",cls[L"UserWidget"]);for(auto n:{L"TextBlock_Name",L"HorizontalBox_251"})prop(button_class,n,L"ObjectProperty",8);prop(button_class,L"CommandText",L"StrProperty",16);prop(button_class,L"CommandName",L"TextProperty",24);prop(button_class,L"isArabic",L"BoolProperty",1);arr(button_class,L"MI_CmdIconArray",L"ObjectProperty",8);
  category_class=klass(L"WD_CmdListCategory_C",cls[L"UserWidget"]);prop(category_class,L"TextBlock_Caption",L"ObjectProperty",8);prop(category_class,L"CategoryName",L"TextProperty",24);
  for(int i=0;i<22;++i){wchar_t name[32];swprintf(name,32,L"mi_CmdIcon_ff%02x",0x71+i);auto pkg=object(std::wstring(L"/Game/UI/Menu/CommandList/Icon/")+name,kind(L"Package"));icons.push_back(object(name,mi,0x100,pkg));}
 }
 // Widget tree with a reflected WidgetTree.RootWidget, unlike Fixture::user.
 Address widget(Address c,const std::wstring& n,bool view){auto w=object(n,c);auto tree=object(L"WidgetTree",tree_class,0x2000,w);auto root=object(L"CanvasPanel_0",cls[L"CanvasPanel"],0x2000,tree);set(tree,L"RootWidget",root);set(w,L"WidgetTree",tree);
  if(view){auto shared=alloc(16);put<int>(shared+8,1);put(w+0x208,heap);put(w+0x210,shared);}return w;}
 Address root(Address w){return raw<Address>(raw<Address>(w+off(w,L"WidgetTree"))+off(raw<Address>(w+off(w,L"WidgetTree")),L"RootWidget"));}
 template<class T> void member_put(Address o,const wchar_t* p,int at,T v){put(o+static_cast<Address>(off(o,p)+at),v);}
 void layout(Address s,float l,float t,float r,float b,float ax=0,float ay=0,float bx=0,float by=0){for(auto [at,v]:std::initializer_list<std::pair<int,float>>{{0,l},{4,t},{8,r},{12,b},{16,ax},{20,ay},{24,bx},{28,by}})member_put<float>(s,L"LayoutData",at,v);}
 Address link(Address child,Address parent,Address sc){auto s=object(L"Slot",sc);put(s+0x28,parent);put(s+0x30,child);put(child+0x30,s);
  auto at=parent+static_cast<Address>(off(parent,L"Slots"));auto n=raw<int>(at+8);auto data=alloc(static_cast<std::size_t>(n+1)*8);for(int i=0;i<n;++i)put(data+i*8,raw<Address>(raw<Address>(at)+i*8));put(data+n*8,s);put(at,data);put<int>(at+8,n+1);put<int>(at+12,n+1);return s;}
 Address text_in(Address owner,Address panel,const wchar_t* field,const std::wstring& v){auto b=object(field,cls[L"TextBlock"]);link(b,panel,canvas_slot);text(b+0x140,v);set(owner,field,b);return b;}
 Address image_in(Address owner,Address panel,const wchar_t* field){auto i=object(field,image);link(i,panel,canvas_slot);set(owner,field,i);return i;}
 Address token(Address row,Address h,const std::wstring& t){
  link(object(L"Spacer",spacer),h,hbox_slot);Address item=0;int code=0;
  if(t.size()==4&&(code=static_cast<int>(std::wcstol(t.c_str(),nullptr,16)))>=0xff71&&code<=0xff86){item=object(L"Image",image,0x2000,row);link(item,h,hbox_slot);put(item+0x148,icons[static_cast<std::size_t>(code-0xff71)]);}
  else{item=object(L"TextBlock",cls[L"TextBlock"],0x2000,row);link(item,h,hbox_slot);text(item+0x140,t);}
  link(object(L"Spacer",spacer),h,hbox_slot);return item;
 }
 static std::vector<std::wstring> split(const std::wstring& s){std::vector<std::wstring> out;std::wstring cur;for(auto c:s){if(c==L'$'){if(!cur.empty())out.push_back(cur);cur.clear();}else cur+=c;}if(!cur.empty())out.push_back(cur);return out;}
 Address make_row(const Row& r,bool arabic=false){
  const bool cat=r.text.size()>=2&&r.text.front()==L'<'&&r.text.back()==L'>';auto w=widget(cat?category_class:button_class,cat?L"Category":L"Button",false);auto rt=root(w);
  auto inner=object(L"CanvasPanel_1",cls[L"CanvasPanel"],0x2000,raw<Address>(w+off(w,L"WidgetTree")));auto s=link(inner,rt,canvas_slot);
  if(cat){layout(s,-800,0,1600,130,.5f,0,.5f,0);text_in(w,inner,L"TextBlock_Caption",r.name);field_text(w,L"CategoryName",r.name);}
  else{layout(s,0,0,1600,130);text_in(w,inner,L"TextBlock_Name",r.name);field_text(w,L"CommandName",r.name);str(w+off(w,L"CommandText"),r.text);flag(w,L"isArabic",arabic);items<Address>(w,L"MI_CmdIconArray",icons);
   auto h=object(L"HorizontalBox_251",hbox,0x2000,raw<Address>(w+off(w,L"WidgetTree")));link(h,inner,canvas_slot);set(w,L"HorizontalBox_251",h);auto t=split(r.text);if(arabic)std::reverse(t.begin(),t.end());for(auto& x:t)token(w,h,x);}
  return w;
 }
 void build(const std::vector<Row>& data,int focus,float y,bool arabic=false){
  pause=widget(pause_class,L"StoryPause",true);tick(pause);flag(pause,L"IsMenuBaseInput",true);popup=object(L"CanvasPanel_Popup",cls[L"CanvasPanel"],0x2000);link(popup,root(pause),canvas_slot);set(pause,L"CanvasPanel_Popup",popup);
  list=widget(list_class,L"CommandList",false);tick(list);auto ls=link(list,popup,canvas_slot);set(list,L"ParentCanvas",popup);set(list,L"CanvasSlot",ls);items<Address>(pause,L"PopupItem",{list});
  set<int>(list,L"ListShowMax",6);set<float>(list,L"ScrollInterval",130.f);set<int>(list,L"ListMax",static_cast<int>(data.size()));set<int>(list,L"focusIndex",focus);member_put<float>(list,L"DestinationPos",4,static_cast<float>(focus)*130.f);
  auto values=alloc(data.size()*16);for(std::size_t i=0;i<data.size();++i)str(values+i*16,data[i].text);auto at=list+off(list,L"CmdTextArray");put(at,values);put<int>(at+8,static_cast<int>(data.size()));put<int>(at+12,static_cast<int>(data.size()));
  std::vector<std::wstring> labels;for(auto& r:data)labels.push_back(r.name);texts(list,L"CmdNameArray",labels);
  main=object(L"CanvasPanel_Main",cls[L"CanvasPanel"],0x2000);layout(link(main,root(list),canvas_slot),-900,0,1800,1080,.5f,0,.5f,0);set(list,L"CanvasPanel_Main",main);member_put<float>(main,L"RenderTransform",8,1.f);member_put<float>(main,L"RenderTransform",12,1.f);
  viewport=object(L"CanvasPanel_1",cls[L"CanvasPanel"],0x2000);layout(link(viewport,main,canvas_slot),-800,186,1600,790,.5f,0,.5f,0);
  box=object(L"VerticalBox_328",vbox,0x2000);box_slot=link(box,viewport,canvas_slot);layout(box_slot,0,y,0,0);flag(box_slot,L"bAutoSize",true);set(list,L"VerticalBox_328",box);set(list,L"VerticalBoxPanelSlot",box_slot);
  rows.clear();for(auto& r:data){auto w=make_row(r,arabic);link(w,box,vbox_slot);rows.push_back(w);}
  auto header=object(L"CanvasPanel_CharInfo",cls[L"CanvasPanel"],0x2000);link(header,main,canvas_slot);text_in(list,header,L"TextBlock_CharName",L"Ryu");text_in(list,header,L"TextBlock_Notice",L"Commands shown for a character facing right.");
  more_up=image_in(list,main,L"Image_MoreUP");more_down=image_in(list,main,L"Image_MoreBottom");put<std::uint8_t>(more_up+0x91,focus==0?2:0);put<std::uint8_t>(more_down+0x91,focus==6-static_cast<int>(data.size())?2:0);
  auto guide=object(L"Guide",guide_class);link(guide,root(list),canvas_slot);set(list,L"WD_BtnNavi",guide);auto g=alloc(16);str(g,L"<_B>Back");auto ga=guide+off(guide,L"TextArray");put(ga,g);put<int>(ga+8,1);put<int>(ga+12,1);
 }
 std::vector<Row> standard(){return {{L"Unique Skills",L"<>"},{L"Hadoken",L"ff71$ff80"},{L"Shoryuken",L"ff73$ff81"},{L"Special Moves",L"<>"},{L"Tatsumaki",L"ff72$ff82"},{L"Joudan",L"ff7a$or$ff84"},{L"Hyper Combos",L"<>"},{L"Shinku Hadoken",L"ff71$ff71$ff85"},{L"Partner",L"ff86"},{L"Last",L"ff7c"}};}
 MenuSnapshot read(){story_commands_rejection_line=0;return story_commands(unreal,{});}
 Address row_text(std::size_t i,const wchar_t* f){return raw<Address>(rows[i]+off(rows[i],f));}
 Address hchild(std::size_t i,int n){auto h=row_text(i,L"HorizontalBox_251");auto s=raw<Address>(raw<Address>(h+off(h,L"Slots"))+static_cast<Address>(n)*8);return raw<Address>(s+0x30);}
};
static void reads_only_whole_visible_rows_as_a_document(){
 CommandFixture f;f.build(f.standard(),-2,-260.f);auto s=f.read();CHECK(s.valid);CHECK(!s.blocked);CHECK(s.position==0&&s.count==0);
 CHECK(s.screen==L"Command list: Ryu");
 CHECK(s.label==L"Shoryuken: Z-shaped motion ending right, heavy punch. Special Moves. Tatsumaki: quarter circle down to left, light kick. Joudan: right arrow, or, double down arrow. Hyper Combos. Shinku Hadoken: quarter circle down to right, quarter circle down to right, Infinity Stone icon");
 CHECK(s.label.find(L"Hadoken: quarter circle down to right, light punch")==std::wstring::npos);CHECK(s.label.find(L"Unique Skills")==std::wstring::npos);CHECK(s.label.find(L"Partner")==std::wstring::npos);
 CHECK(s.value==L"Rows 3 to 8 of 10. More above. More below");
 CHECK(s.help==L"Commands shown for a character facing right. Back");
 f.set<int>(f.list,L"focusIndex",0);f.member_put<float>(f.list,L"DestinationPos",4,0.f);f.layout(f.box_slot,0,0,0,0);f.put<std::uint8_t>(f.more_up+0x91,2);s=f.read();CHECK(s.valid);CHECK(s.value==L"Rows 1 to 6 of 10. More below");CHECK(s.label.rfind(L"Unique Skills. Hadoken: quarter circle down to right, light punch",0)==0);
 f.set<int>(f.list,L"focusIndex",-4);f.member_put<float>(f.list,L"DestinationPos",4,-520.f);f.layout(f.box_slot,0,-520.f+.25f,0,0);f.put<std::uint8_t>(f.more_up+0x91,0);f.put<std::uint8_t>(f.more_down+0x91,2);s=f.read();CHECK(s.valid);CHECK(s.value==L"Rows 5 to 10 of 10. More above");CHECK(s.label.find(L"Last: down arrow")!=std::wstring::npos);
}
static void open_command_list_owns_normal_menu_capture(){
 CommandFixture f;f.build(f.standard(),0,0);
 f.arr(f.menu_class,L"Items",L"ObjectProperty",8);f.prop(f.menu_class,L"ItemIndex",L"IntProperty",4);
 f.prop(f.menu_class,L"IsPopupActive",L"BoolProperty",1);f.flag(f.pause,L"IsPopupActive",true);
 const auto item_class=f.klass(L"WD_OptionItem_Base_C",f.cls[L"UserWidget"]);
 f.prop(item_class,L"ItemName",L"TextProperty",24);
 const auto background=f.widget(f.menu_class,L"OtherMenu",true);
 const auto item=f.widget(item_class,L"OtherMenuItem",false);f.link(item,f.root(background),f.canvas_slot);
 f.flag(background,L"IsMenuBaseInput",true);f.items<Address>(background,L"Items",{item});f.field_text(item,L"ItemName",L"Background choice");
 CHECK(f.unreal.prepare_capture(1000));MenuCapture capture(f.unreal);
 auto s=capture.sample(1000);CHECK(s.valid&&s.screen==L"Command list: Ryu");
 const auto window_class=f.klass(L"WD_WindowBase_C",f.cls[L"WD_UIParent_C"]);
 f.prop(window_class,L"PopupActive",L"BoolProperty",1);f.prop(window_class,L"MessageText",L"TextProperty",24);
 const auto dialog=f.widget(window_class,L"NativeDialog",true);
 f.flag(dialog,L"PopupActive",true);f.field_text(dialog,L"MessageText",L"Confirm exit?");
 CHECK(f.unreal.prepare_capture(1033));s=capture.sample(1033);CHECK(s.modal&&s.label==L"Confirm exit?");
 f.flag(dialog,L"PopupActive",false);
 f.layout(f.box_slot,0,-65,0,0);s=capture.sample(1066);CHECK(s.blocked&&!s.valid);
 f.put<std::uint8_t>(f.list+0x91,2);s=capture.sample(1099);CHECK(s.valid&&s.label==L"Background choice");
}
static void command_list_can_be_attached_directly_to_the_viewport(){
 CommandFixture f;f.build(f.standard(),0,0);
 f.set(f.pause,L"CanvasPanel_Popup",Address(0));f.set(f.list,L"ParentCanvas",Address(0));
 f.set(f.list,L"CanvasSlot",Address(0));f.put(f.list+0x30,Address(0));
 const auto shared=f.alloc(16);f.put<int>(shared+8,1);f.put(f.list+0x208,f.heap);f.put(f.list+0x210,shared);
 CHECK(f.read().valid);
 f.set(f.list,L"ParentCanvas",f.popup);CHECK(f.read().blocked);
 f.set(f.list,L"ParentCanvas",Address(0));f.put<int>(shared+8,0);CHECK(!f.read().valid);
}
static void short_command_documents_read_the_rows_that_fit(){
 CommandFixture f;auto rows=f.standard();rows.resize(4);f.build(rows,0,0);
 f.put<std::uint8_t>(f.more_down+0x91,2);
 auto s=f.read();CHECK(s.valid&&s.value==L"Rows 1 to 4 of 4");
 CHECK(s.label.find(L"Special Moves")!=std::wstring::npos);
}
static void focus_index_is_only_the_target_and_motion_blocks(){
 CommandFixture f;f.build(f.standard(),-2,-200.f);auto s=f.read();CHECK(!s.valid);CHECK(s.blocked);
 // A whole-row-aligned position that is not the target (still easing) is not read either.
 f.layout(f.box_slot,0,-130.f,0,0);s=f.read();CHECK(!s.valid);CHECK(s.blocked);
 f.layout(f.box_slot,0,-260.f,0,0);f.set<int>(f.list,L"focusIndex",-3);s=f.read();CHECK(!s.valid&&s.blocked);
 f.set<int>(f.list,L"focusIndex",-2);f.member_put<float>(f.list,L"DestinationPos",0,4.f);CHECK(f.read().blocked);f.member_put<float>(f.list,L"DestinationPos",0,0.f);CHECK(f.read().valid);
 f.set<int>(f.list,L"focusIndex",1);f.member_put<float>(f.list,L"DestinationPos",4,130.f);f.layout(f.box_slot,0,130.f,0,0);CHECK(f.read().blocked);
 f.set<int>(f.list,L"focusIndex",-2);f.member_put<float>(f.list,L"DestinationPos",4,-260.f);f.layout(f.box_slot,0,-260.f,0,0);f.member_put<float>(f.main,L"RenderTransform",0,-480.f);CHECK(f.read().blocked);
 f.member_put<float>(f.main,L"RenderTransform",0,0.f);CHECK(f.read().valid);
}
static void ownership_requires_the_first_shown_popup_of_an_accepting_menu(){
 {CommandFixture f;f.build(f.standard(),0,0.f);f.items<Address>(f.pause,L"PopupItem",{});auto s=f.read();CHECK(!s.valid&&s.blocked);}
 {CommandFixture f;f.build(f.standard(),0,0.f);f.flag(f.pause,L"IsMenuBaseInput",false);CHECK(f.read().blocked);}
 {CommandFixture f;f.build(f.standard(),0,0.f);auto t=f.raw<Address>(f.pause+f.off(f.pause,L"TickStateObj"));f.set<std::uint8_t>(t,L"State",1);CHECK(f.read().blocked);}
 {CommandFixture f;f.build(f.standard(),0,0.f);auto other=f.widget(f.list_class,L"Other",false);f.tick(other);f.items<Address>(f.pause,L"PopupItem",{other,f.list});CHECK(f.read().blocked);
  auto t=f.raw<Address>(other+f.off(other,L"TickStateObj"));f.set<std::uint8_t>(t,L"State",0);CHECK(f.read().valid);f.items<Address>(f.pause,L"PopupItem",{Address(0),f.list});CHECK(f.read().blocked);}
 {CommandFixture f;f.build(f.standard(),0,0.f);auto other=f.object(L"Elsewhere",f.cls[L"CanvasPanel"],0x2000);f.set(f.list,L"ParentCanvas",other);CHECK(f.read().blocked);}
 {CommandFixture f;f.build(f.standard(),0,0.f);auto second=f.widget(f.pause_class,L"SecondPause",true);f.tick(second);f.flag(second,L"IsMenuBaseInput",true);f.items<Address>(second,L"PopupItem",{f.list});CHECK(f.read().blocked);}
 {CommandFixture f;f.build(f.standard(),0,0.f);f.put<std::uint8_t>(f.list+0x91,2);auto s=f.read();CHECK(!s.valid&&!s.blocked);}
 {CommandFixture f;f.build(f.standard(),0,0.f);auto t=f.raw<Address>(f.list+f.off(f.list,L"TickStateObj"));f.set<std::uint8_t>(t,L"State",0);auto s=f.read();CHECK(!s.valid&&!s.blocked);f.set<std::uint8_t>(t,L"State",1);CHECK(f.read().blocked);f.set<std::uint8_t>(t,L"State",2);f.flag(t,L"PreState",true);CHECK(f.read().blocked);}
 {CommandFixture f;f.build(f.standard(),0,0.f);CommandFixture g;(void)g;auto twin=f.widget(f.list_class,L"Twin",false);f.tick(twin);auto ps=f.link(twin,f.popup,f.canvas_slot);(void)ps;CHECK(f.read().blocked);}
}
static void layout_and_index_identity_mismatches_block(){
 const auto blocked=[](auto mutate){CommandFixture f;f.build(f.standard(),-2,-260.f);CHECK(f.read().valid);mutate(f);auto s=f.read();CHECK(!s.valid);CHECK(s.blocked);};
 blocked([](CommandFixture& f){f.set<int>(f.list,L"ListMax",9);});
 blocked([](CommandFixture& f){f.put<int>(f.box+f.off(f.box,L"Slots")+8,9);});
 blocked([](CommandFixture& f){f.put(f.rows[4]+0x30,f.raw<Address>(f.rows[5]+0x30));});
 blocked([](CommandFixture& f){f.set<float>(f.list,L"ScrollInterval",120.f);});
 blocked([](CommandFixture& f){f.set<int>(f.list,L"ListShowMax",7);});
 blocked([](CommandFixture& f){auto vp=f.raw<Address>(f.viewport+0x30);f.layout(vp,-800,186,1600,800,.5f,0,.5f,0);});
 blocked([](CommandFixture& f){auto rs=f.raw<Address>(f.raw<Address>(f.root(f.rows[0])+f.off(f.root(f.rows[0]),L"Slots")));f.layout(rs,-800,0,1600,120,.5f,0,.5f,0);});
 blocked([](CommandFixture& f){f.put<std::uint8_t>(f.rows[9]+0x91,1);});
 blocked([](CommandFixture& f){auto s=f.raw<Address>(f.rows[4]+0x30);f.member_put<float>(s,L"Padding",4,2.f);});
 blocked([](CommandFixture& f){auto s=f.raw<Address>(f.rows[4]+0x30);f.member_put<std::uint8_t>(s,L"Size",4,1);});
 blocked([](CommandFixture& f){auto s=f.raw<Address>(f.rows[4]+0x30);f.put(s+0x30,f.rows[5]);});
 blocked([](CommandFixture& f){auto a=f.raw<Address>(f.box+f.off(f.box,L"Slots"));auto s3=f.raw<Address>(a+24),s4=f.raw<Address>(a+32);f.put(a+24,s4);f.put(a+32,s3);});
 blocked([](CommandFixture& f){auto values=f.raw<Address>(f.list+f.off(f.list,L"CmdTextArray"));f.str(values+16*4,L"<>");});
 blocked([](CommandFixture& f){f.str(f.rows[2]+f.off(f.rows[2],L"CommandText"),L"ff73$ff80");});
 // Offscreen index identity still matters: an offscreen button under a category marker shifts every row.
 blocked([](CommandFixture& f){auto values=f.raw<Address>(f.list+f.off(f.list,L"CmdTextArray"));f.str(values+16*9,L"<Extra>");});
 blocked([](CommandFixture& f){f.flag(f.box_slot,L"bAutoSize",false);});
 blocked([](CommandFixture& f){f.layout(f.box_slot,3,-260,0,0);});
 blocked([](CommandFixture& f){f.put(f.box_slot+0x28,f.main);});
 blocked([](CommandFixture& f){f.set(f.list,L"VerticalBoxPanelSlot",f.raw<Address>(f.viewport+0x30));});
}
static void rendered_glyphs_must_match_the_token_sequence(){
 const auto blocked=[](auto mutate){CommandFixture f;f.build(f.standard(),-2,-260.f);CHECK(f.read().valid);mutate(f);auto s=f.read();CHECK(!s.valid);CHECK(s.blocked);};
 blocked([](CommandFixture& f){f.put(f.hchild(2,1)+0x148,f.icons[1]);});
 blocked([](CommandFixture& f){auto other=f.object(L"mi_CmdIcon_ff73",f.mi,0x100,f.object(L"/Game/Elsewhere/mi_CmdIcon_ff73",f.kind(L"Package")));f.put(f.hchild(2,1)+0x148,other);});
 blocked([](CommandFixture& f){f.put<std::uint8_t>(f.hchild(2,4)+0x91,2);});
 blocked([](CommandFixture& f){f.text(f.hchild(5,4)+0x140,L"and");});
 blocked([](CommandFixture& f){auto h=f.row_text(2,L"HorizontalBox_251");f.put<int>(h+f.off(h,L"Slots")+8,5);});
 blocked([](CommandFixture& f){f.text(f.row_text(2,L"TextBlock_Name")+0x140,L"Text Block");});
 blocked([](CommandFixture& f){f.text(f.row_text(3,L"TextBlock_Caption")+0x140,L"msg_sys_com_0077");});
 blocked([](CommandFixture& f){f.text(f.row_text(2,L"TextBlock_Name")+0x140,L"Other name");});
 blocked([](CommandFixture& f){f.put<std::uint8_t>(f.row_text(2,L"TextBlock_Name")+0x91,2);});
 // An offscreen row's unreadable content is never read or spoken.
 {CommandFixture f;f.build(f.standard(),-2,-260.f);f.text(f.row_text(0,L"TextBlock_Caption")+0x140,L"msg_hidden");f.text(f.row_text(9,L"TextBlock_Name")+0x140,L"Text Block");CHECK(f.read().valid);}
}
static void arabic_rows_validate_drawn_reverse_and_speak_logical_order(){
 CommandFixture f;f.build(f.standard(),-2,-260.f,true);auto s=f.read();CHECK(s.valid);CHECK(s.label.find(L"Joudan: right arrow, or, double down arrow")!=std::wstring::npos);
 CommandFixture g;g.build(g.standard(),-2,-260.f,false);g.flag(g.rows[5],L"isArabic",true);CHECK(g.read().blocked);
}
static void late_changes_and_ambiguous_documents_block(){
 for(int change=0;change<3;++change){CommandFixture f;f.build(f.standard(),-2,-260.f);auto target=f.row_text(5,L"TextBlock_Name");auto raw=f.memory.reader;bool fired=false;
  f.memory.reader=[&](Address a,void* out,std::size_t n){auto ok=raw(a,out,n);if(!fired&&a==target+0x140){fired=true;if(change==0)f.layout(f.box_slot,0,-130,0,0);else if(change==1)f.set<int>(f.list,L"focusIndex",-3);else f.text(target+0x140,L"Rebuilt");}return ok;};
  auto s=f.read();CHECK(fired);CHECK(!s.valid);CHECK(s.blocked);}
 {CommandFixture d;d.build(d.standard(),-2,-260.f);CHECK(d.read().valid);d.build(d.standard(),0,0.f);auto s=d.read();CHECK(!s.valid);CHECK(s.blocked);}
 CommandFixture f;f.build(f.standard(),-2,-260.f);auto first=f.list;CommandFixture& g=f;auto pause2=g.widget(g.pause_class,L"Pause2",true);g.tick(pause2);g.flag(pause2,L"IsMenuBaseInput",true);auto popup2=g.object(L"Popup2",g.cls[L"CanvasPanel"],0x2000);g.link(popup2,g.root(pause2),g.canvas_slot);g.set(pause2,L"CanvasPanel_Popup",popup2);
 auto list2=g.widget(g.list_class,L"CommandList2",false);g.tick(list2);auto ls=g.link(list2,popup2,g.canvas_slot);g.set(list2,L"ParentCanvas",popup2);g.set(list2,L"CanvasSlot",ls);g.items<Address>(pause2,L"PopupItem",{list2});(void)first;CHECK(f.read().blocked);
}
int main(){int failed=0;for(auto test:{reads_only_whole_visible_rows_as_a_document,open_command_list_owns_normal_menu_capture,command_list_can_be_attached_directly_to_the_viewport,short_command_documents_read_the_rows_that_fit,focus_index_is_only_the_target_and_motion_blocks,ownership_requires_the_first_shown_popup_of_an_accepting_menu,layout_and_index_identity_mismatches_block,rendered_glyphs_must_match_the_token_sequence,arabic_rows_validate_drawn_reverse_and_speak_logical_order,late_changes_and_ambiguous_documents_block}){try{test();}catch(const std::exception& e){std::cerr<<e.what()<<'\n';++failed;}}std::cout<<checks<<" story command checks, "<<failed<<" failed groups\n";return failed?1:0;}
