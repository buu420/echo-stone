#include "mvci/collection.hpp"
#include "collection_fixture.hpp"
#include <iostream>
#include <limits>
using namespace mvci;
using mvci_test::Fixture;
static int checks=0;
#define CHECK(x) do{++checks;if(!(x))throw std::runtime_error(std::string(__func__)+":"+std::to_string(__LINE__)+" " #x);}while(false)
// Collection layouts (research/collection/report.md, live PID 33524 run2_top.txt). Blueprint field offsets
// are fixture choices resolved through reflection; TextBlock.Font (SlateFontInfo, FontMaterial +8) is at the
// live +0x1a0. Designer placeholders are the retained asset defaults.
const std::wstring header_placeholder=L"TEXTÀg",label_placeholder=L"Character ThemeÀg漢";
struct Coll {
 Fixture& f;Address gf=0,top=0,enable=0,disable=0,flow_cls=0;
 static constexpr int font_offset=0x1a0;
 explicit Coll(Fixture& fx):f(fx){
  const auto obj=L"ObjectProperty";
  auto font=f.prop_at(f.cls[L"TextBlock"],L"Font",L"StructProperty",font_offset,104);auto sf=f.klass(L"SlateFontInfo");
  f.prop_at(sf,L"FontObject",obj,0,8);f.prop_at(sf,L"FontMaterial",obj,8,8);f.put(font+0x78,sf);
  enable=f.object(L"M_FontEnable",f.kind(L"Material"),0x100);disable=f.object(L"M_FontDisable",f.kind(L"Material"),0x100);
  flow_cls=f.klass(L"GF_Collection_C",f.klass(L"GF_FlowBase_C",f.klass(L"FlowBase",f.cls[L"Actor"])));
  f.prop(flow_cls,L"State",L"ByteProperty",1);f.prop(flow_cls,L"CurrentWidget",obj,8);f.prop(flow_cls,L"IsTransition",L"BoolProperty",1);
  auto menu=f.cls[L"WD_MenuBase_C"],ui=f.cls[L"WD_UIParent_C"],user=f.cls[L"UserWidget"];
  auto lb=f.klass(L"WD_Collect_ListBase_C",menu);f.prop(lb,L"State",L"ByteProperty",1);f.prop(lb,L"FocusIndex",L"IntProperty",4);f.prop(lb,L"ListNum",L"IntProperty",4);f.arr(lb,L"Buttons",obj,8);
  auto pb=f.klass(L"WD_Collect_PartsBase_C",ui);
  for(auto n:{L"WD_Collect_Parts_C",L"WD_Collect_CharParts_C"}){auto c=f.klass(n,pb);f.prop(c,L"TextBlock_Label",obj,8);f.prop(c,L"CanvasPanel_NewIcon",obj,8);}
  f.prop(f.klass(L"WD_DB_Parts_C",pb),L"TextBlock_CharacterTheme",obj,8);f.prop(f.klass(L"WD_Art_Smn_C",pb),L"CanvasPanel_NEW",obj,8);
  f.prop(f.klass(L"WD_Coll_Caption_C",ui),L"TextBlock_Caption",obj,8);
  auto bn=f.klass(L"WD_BtnNavi_C",user);f.arr(bn,L"TextArray",L"StrProperty",16);f.prop(bn,L"HorizontalBox_PC",obj,8);f.prop(bn,L"HorizontalBox_32",obj,8);
  f.prop(f.klass(L"WD_Collection_Navi_C",ui),L"WD_BtnNavi",obj,8);
  f.prop(f.klass(L"WD_DBSpecHeader_C",menu),L"TextBlock_Caption",obj,8);
  for(auto n:{L"WD_StoryList_C",L"WD_DB_ChaList_C",L"WD_DB_StoryList_C",L"WD_ChaList_C",L"WD_OtherList_C",L"WD_Cha_VoiceList_C"})f.prop(f.klass(n,lb),L"TextBlock_ModeTitle",obj,8);
  auto st=f.klass(L"WD_DB_StItData_C",lb);for(auto n:{L"TextBlock_NameText",L"DescriptionText",L"WD_DBSpecHeader"})f.prop(st,n,obj,8);
  auto cd=f.klass(L"WD_DB_ChaData_C",menu);for(auto n:{L"WD_DBSpecHeader",L"WD_Collection_Navi_0",L"TextBlock_Text",L"TextBlock_NameText",L"WD_BtnNavi_View"})f.prop(cd,n,obj,8);f.prop(cd,L"IsModelViewer",L"BoolProperty",1);
  auto vg=f.klass(L"WD_BtnNavi_View_C",ui);f.arr(vg,L"NaviTextArray",obj,8);for(auto n:{L"TextBlock_Rotation",L"TextBlock_Change",L"TextBlock_Cross",L"TextBlock_Zoom",L"TextBlock_Help",L"TextBlock_Default",L"TextBlock_Color",L"TextBlock_Back"})f.prop(vg,n,obj,8);
  auto alt=f.klass(L"WD_ArtListTextNavi_C",user);for(auto n:{L"TextBlock_ArtIdText",L"WD_BtnNavi_9",L"CanvasPanel_Text",L"CanvasPanel_Navi"})f.prop(alt,n,obj,8);
  f.klass(L"WD_ArtDetail_C",menu);
  auto top0=f.klass(L"WD_CollectionTop_C",lb);for(auto n:{L"WD_Coll_Caption",L"WD_Collection_Navi",L"WD_ChaList_DB"})f.prop(top0,n,obj,8);
  auto mv=f.klass(L"WD_MovisTop_C",lb);f.prop(mv,L"ChildOpen",L"BoolProperty",1);for(auto n:{L"WDChild",L"WD_StoryList",L"WD_Coll_Caption_56",L"WD_Collection_Navi"})f.prop(mv,n,obj,8);
  auto db=f.klass(L"WD_DBTop_C",lb);f.prop(db,L"ChildOpen",L"BoolProperty",1);for(auto n:{L"ActiveChild",L"WD_ChaList_DB",L"WD_ChaData",L"WD_StoryList_DB",L"WD_StoryItemData",L"WD_Coll_Caption_83",L"WD_Collection_Navi"})f.prop(db,n,obj,8);
  auto art=f.klass(L"WD_ArtTop_New_C",lb);f.prop(art,L"FocusIndex_H",L"IntProperty",4);f.prop(art,L"ListViewMax_H",L"IntProperty",4);f.prop(art,L"IsDetailView",L"BoolProperty",1);
  for(auto n:{L"ActiveChild",L"WD_ArtListText",L"WD_ArtDetail",L"WD_Coll_Caption_135",L"WD_Collection_Navi"})f.prop(art,n,obj,8);
  auto bgm=f.klass(L"WD_BgmTop_C",lb);f.prop(bgm,L"ChildOpen",L"BoolProperty",1);for(auto n:{L"ActiveChild",L"PreActiveChild",L"WD_ChaList",L"WD_OtherList",L"WD_Coll_Caption_125",L"WD_Collection_Navi"})f.prop(bgm,n,obj,8);
  auto vo=f.klass(L"WD_VoiceTop_C",lb);f.prop(vo,L"ChildOpen",L"BoolProperty",1);for(auto n:{L"ActiveChild",L"WD_VoiceList",L"WD_Coll_Caption_169",L"WD_Collection_Navi"})f.prop(vo,n,obj,8);
  gf=f.actor(flow_cls,L"GF_Collection_C_0");
 }
 void strs(Address o,const std::wstring& field,const std::vector<std::wstring>& lines){auto data=f.alloc(std::max<std::size_t>(1,lines.size())*16);for(std::size_t i=0;i<lines.size();++i)f.str(data+i*16,lines[i]);auto at=o+static_cast<Address>(f.off(o,field));f.put(at,data);f.put<int>(at+8,static_cast<int>(lines.size()));f.put<int>(at+12,static_cast<int>(lines.size()));}
 Address btnnavi(Address owner,const std::wstring& field,const std::vector<std::wstring>& lines){auto b=f.nested(owner,field,f.cls[L"WD_BtnNavi_C"],L"WD_BtnNavi");strs(b,L"TextArray",lines);return b;}
 Address navi(Address owner,const std::wstring& field,const std::vector<std::wstring>& lines){auto n=f.nested(owner,field,f.cls[L"WD_Collection_Navi_C"],field);btnnavi(n,L"WD_BtnNavi",lines);return n;}
 Address text(Address owner,const std::wstring& field,const std::wstring& value){auto b=f.block(owner,field,value);f.put(b+font_offset+8,enable);return b;}
 Address mode(int state,const std::wstring& cls,const std::wstring& caption_field,const std::wstring& caption){
  top=f.user(f.cls[cls],cls+L"_0",true);f.tick(top);f.set<std::uint8_t>(top,L"State",1);
  auto c=f.nested(top,caption_field,f.cls[L"WD_Coll_Caption_C"],L"WD_Coll_Caption");text(c,L"TextBlock_Caption",caption);
  navi(top,L"WD_Collection_Navi",{L"<_Decide>OK",L"<_Cancel>Back"});
  f.set<std::uint8_t>(gf,L"State",static_cast<std::uint8_t>(state));f.set(gf,L"CurrentWidget",top);f.flag(gf,L"IsTransition",false);return top;
 }
 std::vector<Address> buttons(Address list,const std::vector<std::wstring>& labels,int focus,const std::wstring& part=L"WD_Collect_Parts_C",const std::wstring& label_field=L"TextBlock_Label",const std::wstring& new_field=L"CanvasPanel_NewIcon"){
  std::vector<Address> out;
  for(const auto& l:labels){auto p=f.user(f.cls[part],L"Part",false);f.slot(p,list);if(!l.empty())text(p,label_field,l);if(!new_field.empty())f.put<std::uint8_t>(f.panel(p,new_field)+0x91,2);out.push_back(p);}
  f.items<Address>(list,L"Buttons",out);f.set(list,L"ListNum",static_cast<int>(labels.size()));f.set(list,L"FocusIndex",focus);return out;
 }
 Address sub(Address owner,const std::wstring& field,const std::wstring& cls){auto c=f.nested(owner,field,f.cls[cls],field);f.tick(c);if(f.unreal.property(c,L"State"))f.set<std::uint8_t>(c,L"State",1);return c;}
 void open(Address owner,const std::wstring& child_field,Address child){f.flag(owner,L"ChildOpen",true);f.set(owner,child_field,child);}
 Address field(Address o,const std::wstring& n){return f.raw<Address>(o+static_cast<Address>(f.off(o,n)));}
 void hide(Address w){f.put<std::uint8_t>(w+0x91,2);}
 void show_new(Address part,const std::wstring& n=L"CanvasPanel_NewIcon"){f.put<std::uint8_t>(field(part,n)+0x91,0);}
 Address label_block(Address part,const std::wstring& n=L"TextBlock_Label"){return field(part,n);}
 Address overview(int focus=0){mode(0,L"WD_CollectionTop_C",L"WD_Coll_Caption",L"COLLECTION");auto b=buttons(top,{L"Movies",L"The Stark-Light Library",L"Artwork",L"Sound",L"Voices"},focus);for(int i=1;i<5;++i)show_new(b[i]);return top;}
};
static MenuSnapshot read(Fixture& f,std::uint64_t now=1000){CollectionCapture c(f.unreal);return c.sample(now);}

static void overview_reads_the_focused_entry_on_the_first_sample(){
 // Live PID 33524: FocusIndex 0 'Movies' (NEW hidden), entries 1..4 with NEW shown, navi OK/Back.
 Fixture f;Coll c(f);c.overview();auto s=read(f);CHECK(s.valid);CHECK(!s.modal);CHECK(!s.blocked);CHECK(s.screen==L"COLLECTION");CHECK(s.label==L"Movies");
 CHECK(s.position==1);CHECK(s.count==5);CHECK(s.enabled);CHECK(s.value.empty());CHECK(s.help==L"Confirm: OK. Cancel: Back");CHECK(s.identity!=0);
 f.set(c.top,L"FocusIndex",1);auto n=read(f);CHECK(n.valid);CHECK(n.label==L"The Stark-Light Library");CHECK(n.value==L"New");CHECK(n.position==2);CHECK(n.identity==s.identity);
}
static void focus_transitions_are_seen_by_a_persistent_capture_without_waiting(){
 Fixture f;Coll c(f);CollectionCapture cap(f.unreal);CHECK(!cap.sample(1000).valid); // No flow yet.
 c.overview();CHECK(cap.sample(1001).label==L"Movies"); // New registry count: rescanned immediately.
 for(int i=0;i<5;++i){f.set(c.top,L"FocusIndex",i);auto s=cap.sample(1002+i);CHECK(s.valid);CHECK(s.position==i+1);}
 CHECK(cap.sample(1010).label==L"Voices");
}
static void owner_tick_list_state_and_visibility_gate_the_read(){
 Fixture f;Coll c(f);c.overview();CHECK(read(f).valid);
 for(int st:{0,2,3}){f.set<std::uint8_t>(c.top,L"State",static_cast<std::uint8_t>(st));CHECK(!read(f).valid);}f.set<std::uint8_t>(c.top,L"State",1);
 auto tick=c.field(c.top,L"TickStateObj");for(int st:{0,1,3}){f.set<std::uint8_t>(tick,L"State",static_cast<std::uint8_t>(st));CHECK(!read(f).valid);}f.set<std::uint8_t>(tick,L"State",2);
 f.set(tick,L"ParentObj",c.gf);CHECK(!read(f).valid);f.set(tick,L"ParentObj",c.top);CHECK(read(f).valid);
 auto other=f.klass(L"OtherTick");f.prop(other,L"State",L"ByteProperty",1);f.prop(other,L"ParentObj",L"ObjectProperty",8);auto t=f.object(L"Tick",other);f.set(t,L"ParentObj",c.top);f.set<std::uint8_t>(t,L"State",2);
 f.set(c.top,L"TickStateObj",t);CHECK(!read(f).valid);f.set(c.top,L"TickStateObj",tick);
 c.hide(c.top);CHECK(!read(f).valid);f.put<std::uint8_t>(c.top+0x91,0);
 f.put(c.top+0x210,Address(0));CHECK(!read(f).valid); // Removed from the viewport.
}
static void effective_visibility_callback_is_honoured(){
 Fixture f;Coll c(f);c.overview();
 CollectionCapture shown(f.unreal,[&](Address a)->std::optional<int>{return a==c.top?std::optional<int>(1):f.unreal.integer(a,L"Visibility");});CHECK(!shown.sample(1000).valid);
 CollectionCapture plain(f.unreal,[&](Address a){return f.unreal.integer(a,L"Visibility");});CHECK(plain.sample(1000).valid);
}
static void flow_states_transitions_and_owner_linkage(){
 Fixture f;Coll c(f);c.overview();
 for(int st=6;st<=11;++st){f.set<std::uint8_t>(c.gf,L"State",static_cast<std::uint8_t>(st));CHECK(!read(f).valid);} // End, travel, movie/credit and their confirms.
 f.set<std::uint8_t>(c.gf,L"State",1);CHECK(!read(f).valid); // Movie summoned while the overview still owns CurrentWidget.
 f.set<std::uint8_t>(c.gf,L"State",0);CHECK(read(f).valid);
 f.flag(c.gf,L"IsTransition",true);CHECK(!read(f).valid);f.flag(c.gf,L"IsTransition",false);
 f.set(c.gf,L"CurrentWidget",Address(0));CHECK(!read(f).valid);
 auto stray=f.user(f.cls[L"WD_Coll_Caption_C"],L"Stray",true);f.tick(stray);f.set(c.gf,L"CurrentWidget",stray);CHECK(!read(f).valid);
 f.set(c.gf,L"CurrentWidget",c.top);CHECK(read(f).valid);
}
static void flow_must_be_unique_and_in_the_current_viewport_world(){
 Fixture f;Coll c(f);c.overview();CHECK(read(f).valid);
 // A retained flow from an earlier Collection world is ignored, not counted as ambiguous.
 auto old_world=f.object(L"World",f.cls[L"World"]);auto old_level=f.object(L"PersistentLevel",f.cls[L"Level"],0x2000,old_world);f.put(old_level+0xb0,old_world);
 auto stale=f.object(L"GF_Collection_C_1",c.flow_cls,0x2000,old_level);f.set<std::uint8_t>(stale,L"State",0);f.set(stale,L"CurrentWidget",c.top);CHECK(read(f).valid);
 f.put(c.gf+0x20,old_level);CHECK(!read(f).valid); // The only current-world candidate moved to the old world.
 f.put(c.gf+0x20,f.level);auto twin=f.actor(c.flow_cls,L"GF_Collection_C_2");f.set<std::uint8_t>(twin,L"State",0);f.set(twin,L"CurrentWidget",c.top);CHECK(!read(f).valid); // Ambiguous.
 f.put(twin+0x20,old_level);CHECK(read(f).valid);
 auto engine=f.raw<Address>(f.base+0x36bc348);auto viewport=f.raw<Address>(engine+0x5d8);f.put(viewport+0x80,Address(0));CHECK(!read(f).valid);f.put(viewport+0x80,f.world);
 f.put(f.base+0x36bc348,Address(0));CHECK(!read(f).valid);
}
static void retired_or_replaced_flows_are_seen_without_waiting_for_a_rescan(){
 Fixture f;Coll c(f);c.overview();CollectionCapture cap(f.unreal);CHECK(cap.sample(1000).valid);
 // Same registry count, the cached flow retired into a recycled slot holding another object.
 auto entry=f.entries+static_cast<Address>(f.raw<int>(c.gf+0xc))*24;auto dummy=f.object(L"Dummy",f.kind(L"Object"));
 CHECK(cap.sample(1001).valid);f.put(entry,dummy);CHECK(!cap.sample(1002).valid);
 auto fresh=f.actor(c.flow_cls,L"GF_Collection_C_3");f.set<std::uint8_t>(fresh,L"State",0);f.set(fresh,L"CurrentWidget",c.top);
 CHECK(cap.sample(1003).valid);
 // PendingKill/Unreachable registry flags retire the flow too.
 auto fe=f.entries+static_cast<Address>(f.raw<int>(fresh+0xc))*24;f.put<std::uint32_t>(fe+8,0x20000000);CHECK(!cap.sample(1004).valid);
}
static void bad_arrays_and_focus_ranges_are_rejected(){
 Fixture f;Coll c(f);c.overview(2);CHECK(read(f).label==L"Artwork");
 f.set(c.top,L"FocusIndex",-1);CHECK(!read(f).valid);f.set(c.top,L"FocusIndex",5);CHECK(!read(f).valid);f.set(c.top,L"FocusIndex",2);
 f.set(c.top,L"ListNum",6);CHECK(!read(f).valid);f.set(c.top,L"ListNum",5);CHECK(read(f).valid);
 auto at=c.top+static_cast<Address>(f.off(c.top,L"Buttons"));auto data=f.raw<Address>(at);auto keep=f.raw<Address>(data+16);
 f.put(data+16,Address(0));CHECK(!read(f).valid);f.put(data+16,f.alloc(0x100));CHECK(!read(f).valid);f.put(data+16,keep);
 f.put<int>(at+8,300);f.put<int>(at+12,300);f.set(c.top,L"ListNum",300);CHECK(!read(f).valid); // Beyond the array limit.
 f.put<int>(at+8,5);f.put<int>(at+12,4);f.set(c.top,L"ListNum",5);CHECK(!read(f).valid); // Capacity below count.
 f.put<int>(at+12,5);CHECK(read(f).valid);
 auto other=f.user(f.cls[L"WD_Coll_Caption_C"],L"NotAPart",false);f.slot(other,c.top);f.put(data+16,other);CHECK(!read(f).valid);f.put(data+16,keep);
 c.hide(keep);CHECK(!read(f).valid);f.put<std::uint8_t>(keep+0x91,0);CHECK(read(f).valid);
}
static void labels_new_badges_and_unavailable_font(){
 Fixture f;Coll c(f);auto top=c.overview(0);auto b=f.raw<Address>(top+static_cast<Address>(f.off(top,L"Buttons")));auto first=f.raw<Address>(b);auto label=c.label_block(first);
 c.hide(label);CHECK(!read(f).valid);f.put<std::uint8_t>(label+0x91,0);
 f.text(label+0x140,L"msg_sys_db1_0001");CHECK(!read(f).valid);
 f.text(label+0x140,label_placeholder);CHECK(!read(f).valid); // WD_Collect_Parts designer default.
 f.text(label+0x140,L"");CHECK(!read(f).valid);
 f.text(label+0x140,L"Movies");CHECK(read(f).value.empty());
 c.show_new(first);CHECK(read(f).value==L"New");f.put<std::uint8_t>(c.field(first,L"CanvasPanel_NewIcon")+0x91,1);CHECK(read(f).value.empty()); // Collapsed.
 f.put<std::uint8_t>(c.field(first,L"CanvasPanel_NewIcon")+0x91,0);f.set(first,L"CanvasPanel_NewIcon",Address(0));CHECK(read(f).value.empty());
 // setDisable(true): font material M_FontDisable. Anything else is not proof of unavailability.
 f.put(label+Coll::font_offset+8,c.disable);auto s=read(f);CHECK(s.valid);CHECK(!s.enabled);CHECK(s.label==L"Movies");
 f.put(label+Coll::font_offset+8,f.object(L"M_Other",f.kind(L"Material"),0x100));CHECK(read(f).enabled);
 f.put(label+Coll::font_offset+8,Address(0));CHECK(read(f).enabled);
 f.put(label+Coll::font_offset+8,c.disable);auto fm=f.unreal.property(label,L"Font");CHECK(fm);auto st=f.raw<Address>(fm->address+0x78);auto mat=f.raw<Address>(st+0x38);
 f.put<int>(mat+0x18,f.name_id(L"fontMaterial"));CHECK(!read(f).enabled); // FName case variant still matches.
}
static void database_child_owns_input_while_open(){
 Fixture f;Coll c(f);auto db=c.mode(2,L"WD_DBTop_C",L"WD_Coll_Caption_83",L"DATABASE");c.buttons(db,{L"Characters",L"Scenes"},0);
 auto list=c.sub(db,L"WD_ChaList_DB",L"WD_DB_ChaList_C");c.text(list,L"TextBlock_ModeTitle",L"CHARACTERS");c.buttons(list,{L"Ryu",L"Iron Man",L"Chun-Li"},1,L"WD_Collect_CharParts_C");
 CHECK(read(f).label==L"Characters");
 c.open(db,L"ActiveChild",list);f.set<std::uint8_t>(db,L"State",3);auto s=read(f);CHECK(s.valid);CHECK(s.label==L"Iron Man");CHECK(s.screen==L"CHARACTERS");CHECK(s.position==2);CHECK(s.count==3);CHECK(s.help==L"Confirm: OK. Cancel: Back");
 auto mt=c.field(list,L"TextBlock_ModeTitle");c.hide(mt);CHECK(read(f).screen==L"DATABASE");f.put<std::uint8_t>(mt+0x91,0);
 // Never narrate the background top while its child should own input.
 f.set<std::uint8_t>(list,L"State",3);CHECK(!read(f).valid);f.set<std::uint8_t>(list,L"State",1);
 c.hide(list);CHECK(!read(f).valid);f.put<std::uint8_t>(list+0x91,0);
 f.set<std::uint8_t>(c.field(list,L"TickStateObj"),L"State",3);CHECK(!read(f).valid);f.set<std::uint8_t>(c.field(list,L"TickStateObj"),L"State",2);
 f.set(db,L"ActiveChild",Address(0));CHECK(!read(f).valid);
 f.set(db,L"ActiveChild",f.alloc(0x100));CHECK(!read(f).valid); // Dangling.
 f.set(db,L"ActiveChild",db);CHECK(!read(f).valid); // Cycle.
 auto foreign=f.user(f.cls[L"WD_DB_ChaList_C"],L"WD_DB_ChaList_C_9",true);f.tick(foreign);f.set<std::uint8_t>(foreign,L"State",1);c.buttons(foreign,{L"Other"},0,L"WD_Collect_CharParts_C");
 f.set(db,L"ActiveChild",foreign);CHECK(!read(f).valid); // Not one of the top's declared children.
 f.set(db,L"ActiveChild",list);CHECK(read(f).valid);
 auto tick=c.field(db,L"TickStateObj");f.set<std::uint8_t>(tick,L"State",3);CHECK(!read(f).valid);f.set<std::uint8_t>(tick,L"State",2); // The forwarding top itself is exiting.
 f.flag(db,L"ChildOpen",false);f.set<std::uint8_t>(db,L"State",1);CHECK(read(f).label==L"Characters");
}
static void numbered_and_case_variant_identifiers_match(){
 Fixture f;Coll c(f);auto db=c.mode(2,L"WD_DBTop_C",L"WD_Coll_Caption_83",L"DATABASE");c.buttons(db,{L"Characters"},0);
 auto list=c.sub(db,L"WD_StoryList_DB",L"WD_DB_StoryList_C");c.buttons(list,{L"Scene 1"},0);c.open(db,L"ActiveChild",list);CHECK(read(f).label==L"Scene 1");
 auto p=f.unreal.property(db,L"ChildOpen");CHECK(p);f.put<int>(p->address+0x18,f.name_id(L"childOpen"));CHECK(read(f).label==L"Scene 1");
 f.put<int>(f.cls[L"WD_DBTop_C"]+0x18,f.name_id(L"WD_DBTop_c"));CHECK(read(f).label==L"Scene 1");
 f.put<int>(f.cls[L"WD_DB_StoryList_C"]+0x18,f.name_id(L"WD_DB_StoryList_X"));CHECK(!read(f).valid); // A different class is not accepted.
}
static void movie_story_child_uses_wdchild_and_greyed_story_entry(){
 Fixture f;Coll c(f);auto mv=c.mode(1,L"WD_MovisTop_C",L"WD_Coll_Caption_56",L"MOVIES");auto b=c.buttons(mv,{L"Story",L"Opening"},0);
 f.put(c.label_block(b[0])+Coll::font_offset+8,c.disable);auto s=read(f);CHECK(s.valid);CHECK(s.label==L"Story");CHECK(!s.enabled);CHECK(s.screen==L"MOVIES");
 auto story=c.sub(mv,L"WD_StoryList",L"WD_StoryList_C");c.buttons(story,{L"Chapter 1",L"Chapter 2"},1);
 f.flag(mv,L"ChildOpen",true);CHECK(!read(f).valid); // WDChild not yet assigned.
 f.set(mv,L"WDChild",story);s=read(f);CHECK(s.label==L"Chapter 2");CHECK(s.position==2);
}
static void bgm_and_voice_child_lists(){
 Fixture f;Coll c(f);auto bgm=c.mode(4,L"WD_BgmTop_C",L"WD_Coll_Caption_125",L"SOUND");c.buttons(bgm,{L"Character Themes",L"Other"},1);CHECK(read(f).label==L"Other");
 auto other=c.sub(bgm,L"WD_OtherList",L"WD_OtherList_C");c.buttons(other,{L"Track 1",L"Track 2",L"Track 3"},2);c.open(bgm,L"ActiveChild",other);CHECK(read(f).label==L"Track 3");CHECK(read(f).count==3);
 Fixture g;Coll v(g);auto vo=v.mode(5,L"WD_VoiceTop_C",L"WD_Coll_Caption_169",L"VOICES");v.buttons(vo,{L"Ryu"},0,L"WD_Collect_CharParts_C");CHECK(read(g).label==L"Ryu");
 auto vl=v.sub(vo,L"WD_VoiceList",L"WD_Cha_VoiceList_C");v.buttons(vl,{L"Character Voice 1",L"Character Voice 2"},0);v.open(vo,L"ActiveChild",vl);CHECK(read(g).label==L"Character Voice 1");
 g.set<std::uint8_t>(v.gf,L"State",4);CHECK(!read(g).valid); // The flow state must name this top.
}
static void story_item_details_only_when_drawn(){
 Fixture f;Coll c(f);auto db=c.mode(2,L"WD_DBTop_C",L"WD_Coll_Caption_83",L"DATABASE");c.buttons(db,{L"Characters",L"Scenes"},1);
 auto items=c.sub(db,L"WD_StoryItemData",L"WD_DB_StItData_C");auto b=c.buttons(items,{L"Helmet",L"Locked Relic"},0);c.open(db,L"ActiveChild",items);
 auto hdr=f.nested(items,L"WD_DBSpecHeader",f.cls[L"WD_DBSpecHeader_C"],L"WD_DBSpecHeader");c.text(hdr,L"TextBlock_Caption",L"Scene 3");
 auto name=c.text(items,L"TextBlock_NameText",L"Helmet");auto desc=c.text(items,L"DescriptionText",L"A dented helmet.");
 auto s=read(f);CHECK(s.valid);CHECK(s.screen==L"Scene 3");CHECK(s.label==L"Helmet");CHECK(s.value==L"A dented helmet.");CHECK(s.enabled);
 c.hide(desc);CHECK(read(f).value.empty());f.put<std::uint8_t>(desc+0x91,0);
 f.text(name+0x140,L"Helmet Mk II");CHECK(read(f).value==L"Helmet Mk II. A dented helmet.");
 f.text(name+0x140,L"TEXTÀg（アイテム名）");CHECK(read(f).value==L"A dented helmet."); // Designer default name.
 // Locked item: greyed label; the detail panel is read only if drawn.
 f.set(items,L"FocusIndex",1);f.put(c.label_block(b[1])+Coll::font_offset+8,c.disable);c.hide(desc);c.hide(name);
 s=read(f);CHECK(s.valid);CHECK(s.label==L"Locked Relic");CHECK(!s.enabled);CHECK(s.value.empty());
 c.hide(c.field(hdr,L"TextBlock_Caption"));CHECK(read(f).screen==L"DATABASE");
}
static void character_viewer_reads_header_and_never_the_placeholder_profile(){
 Fixture f;Coll c(f);auto db=c.mode(2,L"WD_DBTop_C",L"WD_Coll_Caption_83",L"DATABASE");c.buttons(db,{L"Characters"},0);
 auto view=c.sub(db,L"WD_ChaData",L"WD_DB_ChaData_C");c.open(db,L"ActiveChild",view);
 auto hdr=f.nested(view,L"WD_DBSpecHeader",f.cls[L"WD_DBSpecHeader_C"],L"WD_DBSpecHeader");auto cap=c.text(hdr,L"TextBlock_Caption",L"Ryu");
 c.text(view,L"TextBlock_Text",L"text");c.text(view,L"TextBlock_NameText",L"Hidden real name");c.navi(view,L"WD_Collection_Navi_0",{L"<_Cancel>Back",L"Model viewer"});
 auto s=read(f);CHECK(s.valid);CHECK(s.label==L"Ryu");CHECK(s.screen==L"DATABASE");CHECK(s.position==0);CHECK(s.count==0);CHECK(s.help==L"Cancel: Back. Model viewer");
 for(const auto* t:{&s.screen,&s.label,&s.value,&s.help}){CHECK(t->find(L"text")==std::wstring::npos);CHECK(t->find(L"Hidden real name")==std::wstring::npos);}
 f.text(c.field(view,L"TextBlock_Text")+0x140,L"A long profile");s=read(f);CHECK(s.value.find(L"profile")==std::wstring::npos);CHECK(s.help.find(L"profile")==std::wstring::npos);
 f.text(cap+0x140,header_placeholder);CHECK(!read(f).valid);f.text(cap+0x140,L"Ryu");
 c.hide(hdr);CHECK(!read(f).valid);f.put<std::uint8_t>(hdr+0x91,0);
 f.set<std::uint8_t>(c.field(view,L"TickStateObj"),L"State",1);CHECK(!read(f).valid);
}
static void model_viewer_reads_its_visible_control_guide_without_hidden_header(){
 Fixture f;Coll c(f);auto db=c.mode(2,L"WD_DBTop_C",L"WD_Coll_Caption_83",L"DATABASE");c.buttons(db,{L"Characters"},0);
 auto view=c.sub(db,L"WD_ChaData",L"WD_DB_ChaData_C");c.open(db,L"ActiveChild",view);
 auto hdr=f.nested(view,L"WD_DBSpecHeader",f.cls[L"WD_DBSpecHeader_C"],L"WD_DBSpecHeader");c.text(hdr,L"TextBlock_Caption",L"Hidden Ryu");c.hide(hdr);
 auto normal=c.navi(view,L"WD_Collection_Navi_0",{L"Hidden normal guide"});c.hide(normal);
 c.text(view,L"TextBlock_Text",L"Hidden profile");f.flag(view,L"IsModelViewer",true);
 auto guide=f.nested(view,L"WD_BtnNavi_View",f.cls[L"WD_BtnNavi_View_C"],L"WD_BtnNavi_View");
 c.text(guide,L"TextBlock_Rotation",L"Rotate");auto back=c.text(guide,L"TextBlock_Back",L"Back");f.items<Address>(guide,L"NaviTextArray",{}); // Unused by the retained template/bytecode.
 auto s=read(f);CHECK(s.valid);CHECK(s.label==L"Rotate. Back");CHECK(s.position==0);CHECK(s.count==0);CHECK(s.value.empty());CHECK(s.help.empty());
 CHECK(s.label.find(L"Hidden")==std::wstring::npos);c.hide(back);CHECK(read(f).label==L"Rotate");
 c.hide(guide);CHECK(!read(f).valid);f.put<std::uint8_t>(guide+0x91,0);f.flag(view,L"IsModelViewer",false);CHECK(!read(f).valid);
}
static void late_visibility_reads_cannot_return_retired_or_retargeted_selection(){
 for(int change=0;change<9;++change){
  Fixture f;Coll c(f);c.overview();const auto array=c.field(c.top,L"Buttons"),part=f.raw<Address>(array),navi=c.field(c.top,L"WD_Collection_Navi");
  bool changed=false;CollectionCapture capture(f.unreal,[&](Address a)->std::optional<int>{
   if(a==navi&&!changed){changed=true;
    const auto retire=[&](Address o){const auto ref=f.unreal.object(o);CHECK(ref);f.put<int>(f.entries+static_cast<Address>(ref->index)*24+0x10,ref->serial+1);};
    switch(change){
     case 0:retire(part);break;
     case 1:f.set(c.gf,L"CurrentWidget",Address(0));break;
     case 2:f.set(c.top,L"FocusIndex",1);break;
     case 3:f.flag(c.gf,L"IsTransition",true);break;
     case 4:f.set<std::uint8_t>(c.gf,L"State",1);break;
     case 5:retire(c.top);break;
     case 6:f.put(array,f.raw<Address>(array+8));break;
     case 7:{const auto engine=f.raw<Address>(f.base+0x36bc348);f.put(f.raw<Address>(engine+0x5d8)+0x80,Address(0));break;}
     case 8:f.set<std::uint8_t>(c.top,L"State",3);break;
    }
   }
   return f.unreal.integer(a,L"Visibility");
  });CHECK(!capture.sample(1000).valid);CHECK(changed);
 }
 Fixture f;Coll c(f);auto db=c.mode(2,L"WD_DBTop_C",L"WD_Coll_Caption_83",L"DATABASE");c.buttons(db,{L"Characters"},0);
 auto list=c.sub(db,L"WD_ChaList_DB",L"WD_DB_ChaList_C");c.buttons(list,{L"Ryu"},0,L"WD_Collect_CharParts_C");c.open(db,L"ActiveChild",list);
 bool changed=false;const auto navi=c.field(db,L"WD_Collection_Navi");CollectionCapture capture(f.unreal,[&](Address a)->std::optional<int>{
  if(a==navi&&!changed){changed=true;f.flag(db,L"ChildOpen",false);f.set(db,L"ActiveChild",Address(0));}return f.unreal.integer(a,L"Visibility");
 });CHECK(!capture.sample(1000).valid);CHECK(changed);
}
static Address art_top(Coll& c,int focus,int h,int count=8){
 auto& f=c.f;auto art=c.mode(3,L"WD_ArtTop_New_C",L"WD_Coll_Caption_135",L"ARTWORK");std::vector<std::wstring> none(static_cast<std::size_t>(count),L"");
 c.buttons(art,none,focus,L"WD_Art_Smn_C",L"",L"CanvasPanel_NEW");f.set(art,L"ListNum",(count+4)/5);f.set(art,L"ListViewMax_H",5);f.set(art,L"FocusIndex_H",h);
 auto lt=f.nested(art,L"WD_ArtListText",f.cls[L"WD_ArtListTextNavi_C"],L"WD_ArtListText");auto label=c.text(lt,L"TextBlock_ArtIdText",L"Ryu　Costume 1"),guide=c.btnnavi(lt,L"WD_BtnNavi_9",{L"<_Decide>View",L"<_Cancel>Back"});
 auto text_panel=f.panel(lt,L"CanvasPanel_Text"),navi_panel=f.panel(lt,L"CanvasPanel_Navi");
 f.put(f.raw<Address>(label+0x30)+0x28,text_panel);f.put(f.raw<Address>(guide+0x30)+0x28,navi_panel);c.hide(navi_panel); // ChangeTextBlock(false).
 auto d=f.nested(art,L"WD_ArtDetail",f.cls[L"WD_ArtDetail_C"],L"WD_ArtDetail");c.hide(d);return art;
}
static void artwork_grid_index_math_count_and_badge(){
 Fixture f;Coll c(f);auto art=art_top(c,1,2);auto s=read(f);CHECK(s.valid);CHECK(s.screen==L"ARTWORK");CHECK(s.label==L"Ryu Costume 1");CHECK(s.position==8);CHECK(s.count==8);CHECK(s.value.empty());
 CHECK(s.help==L"Confirm: OK. Cancel: Back");
 auto data=f.raw<Address>(art+static_cast<Address>(f.off(art,L"Buttons")));c.show_new(f.raw<Address>(data+7*8),L"CanvasPanel_NEW");CHECK(read(f).value==L"New");
 f.set(art,L"FocusIndex_H",3);CHECK(!read(f).valid); // Cell 8 is past the 8 drawn entries.
 f.set(art,L"FocusIndex_H",5);CHECK(!read(f).valid);f.set(art,L"FocusIndex_H",-1);CHECK(!read(f).valid);
 f.set(art,L"FocusIndex_H",0);f.set(art,L"FocusIndex",0);s=read(f);CHECK(s.position==1);
 f.set(art,L"ListViewMax_H",0);CHECK(!read(f).valid);f.set(art,L"ListViewMax_H",5);
 f.set(art,L"ListNum",3);CHECK(!read(f).valid);f.set(art,L"ListNum",2);CHECK(read(f).valid);
 auto lt=c.field(art,L"WD_ArtListText");c.hide(c.field(lt,L"WD_BtnNavi_9"));CHECK(read(f).help==L"Confirm: OK. Cancel: Back");
 c.hide(c.field(lt,L"TextBlock_ArtIdText"));CHECK(!read(f).valid);f.put<std::uint8_t>(c.field(lt,L"TextBlock_ArtIdText")+0x91,0);
 f.text(c.field(lt,L"TextBlock_ArtIdText")+0x140,label_placeholder);CHECK(!read(f).valid);
}
static void artwork_details_have_no_list_position(){
 Fixture f;Coll c(f);auto art=art_top(c,1,2);auto grid=read(f);CHECK(grid.valid);
 f.flag(art,L"IsDetailView",true);CHECK(!read(f).valid); // Detail widget not drawn yet.
 auto bar=c.field(art,L"WD_ArtListText");c.hide(c.field(bar,L"CanvasPanel_Text"));f.put<std::uint8_t>(c.field(bar,L"CanvasPanel_Navi")+0x91,0); // Actual ChangeTextBlock(true).
 CHECK(!f.unreal.shown(c.field(bar,L"TextBlock_ArtIdText")));CHECK(f.unreal.shown(c.field(bar,L"WD_BtnNavi_9")));
 f.put<std::uint8_t>(c.field(art,L"WD_ArtDetail")+0x91,4);auto s=read(f);CHECK(s.valid);CHECK(s.label==L"Confirm: View. Cancel: Back");CHECK(s.help.empty());CHECK(s.label.find(L"Ryu")==std::wstring::npos);CHECK(s.position==0);CHECK(s.count==0);CHECK(s.identity!=grid.identity);
 f.set(art,L"FocusIndex_H",4);CHECK(read(f).valid); // Detail does not depend on grid cell validity.
}
static void late_artwork_help_reads_cannot_mix_focus_or_retired_cells(){
 for(int change=0;change<4;++change){
  Fixture f;Coll c(f);auto art=art_top(c,1,2),bar=c.field(art,L"WD_ArtListText"),guide=c.field(bar,L"WD_BtnNavi_9");
  const auto cell=f.raw<Address>(c.field(art,L"Buttons")+7*8);bool changed=false;
  CollectionCapture capture(f.unreal,[&](Address a)->std::optional<int>{
   if(a==guide&&!changed){changed=true;
    if(change==0)f.set(art,L"FocusIndex_H",1);
    if(change==1||change==3){const auto ref=f.unreal.object(change==1?cell:bar);CHECK(ref);f.put<int>(f.entries+static_cast<Address>(ref->index)*24+0x10,ref->serial+1);}
    if(change==2){f.flag(art,L"IsDetailView",true);f.put<std::uint8_t>(c.field(art,L"WD_ArtDetail")+0x91,0);}
   }return f.unreal.integer(a,L"Visibility");
  });CHECK(!capture.sample(1000).valid);CHECK(changed);
 }
}
static void menu_hints_reads_a_direct_numbered_button_guide(){
 Fixture f;Coll c(f);auto top=c.overview();auto lt=f.nested(top,L"WD_ChaList_DB",f.cls[L"WD_ArtListTextNavi_C"],L"Holder");auto b=c.btnnavi(lt,L"WD_BtnNavi_9",{L"<_Decide>View",L"<_Cancel>Back"});
 CHECK(menu_hints(f.unreal,b,{},true)==L"Confirm: View. Cancel: Back");
 CHECK(menu_hints(f.unreal,lt).empty()); // No WD_BtnNavi field: numbered guides need the direct form.
 CHECK(menu_hints(f.unreal,c.field(top,L"WD_Collection_Navi"))==L"Confirm: OK. Cancel: Back"); // Existing owner form unchanged.
 CHECK(menu_hints(f.unreal,lt,{},true).empty()); // Not a WD_BtnNavi_C.
 c.hide(b);CHECK(menu_hints(f.unreal,b,{},true).empty());f.put<std::uint8_t>(b+0x91,0);
 CHECK(menu_hints(f.unreal,f.alloc(0x100),{},true).empty());
 auto pc=f.panel(b,L"HorizontalBox_PC");auto pad=f.panel(b,L"HorizontalBox_32");c.hide(pad);(void)pc;CHECK(menu_hints(f.unreal,b,{},true)==L"Enter: View. Backspace: Back");
}
static void modal_dialogs_keep_priority_over_collection_and_return_immediately(){
 Fixture f;Coll c(f);c.overview();
 const auto cls=f.klass(L"WD_WindowBase_C",f.cls[L"UserWidget"]);
 f.prop(cls,L"PopupActive",L"BoolProperty",1);f.prop(cls,L"MessageText",L"TextProperty",24);f.prop(cls,L"TextBlock_MessageMain",L"ObjectProperty",8);
 const auto window=f.user(cls,L"CollectionConfirmation",true);f.flag(window,L"PopupActive",true);
 f.field_text(window,L"MessageText",L"Play this movie?");const auto body=f.block(window,L"TextBlock_MessageMain",L"Play this movie?");
 MenuCapture menus(f.unreal);CollectionCapture collection(f.unreal);
 const auto capture=[&](std::uint64_t now){auto s=menus.sample(now);if(!s.valid&&!s.blocked)s=collection.sample(now);return s;};
 auto s=capture(1000);CHECK(s.valid);CHECK(s.modal);CHECK(s.label==L"Play this movie?");
 c.hide(body);s=capture(1010);CHECK(!s.valid);CHECK(s.blocked); // An unready modal still blocks its background.
 f.flag(window,L"PopupActive",false);s=capture(1020);CHECK(s.valid);CHECK(!s.modal);CHECK(s.label==L"Movies");
}
static void shared_runtime_discovery_preserves_collection_focus_and_ambiguity(){
 Fixture f;Coll c(f);c.overview();CollectionCapture cap(f.unreal);CHECK(f.unreal.prepare_capture(1000));auto first=cap.sample(1000);CHECK(first.valid);CHECK(first.label==L"Movies");
 f.set(c.top,L"FocusIndex",4);CHECK(f.unreal.prepare_capture(1033));auto next=cap.sample(1033);CHECK(next.valid);CHECK(next.label==L"Voices");CHECK(next.value==L"New");
 auto twin=f.actor(c.flow_cls,L"GF_Collection_C_1");f.set<std::uint8_t>(twin,L"State",0);f.set(twin,L"CurrentWidget",c.top);f.flag(twin,L"IsTransition",false);f.unreal.observe_event(twin);CHECK(f.unreal.prepare_capture(1066));CHECK(!cap.sample(1066).valid);
 CHECK(f.unreal.discovery_stats().full_passes==1);CHECK(f.unreal.discovery_stats().fallback_scans==0);
}
int main(){int failed=0;for(auto test:{shared_runtime_discovery_preserves_collection_focus_and_ambiguity,overview_reads_the_focused_entry_on_the_first_sample,focus_transitions_are_seen_by_a_persistent_capture_without_waiting,
 owner_tick_list_state_and_visibility_gate_the_read,effective_visibility_callback_is_honoured,flow_states_transitions_and_owner_linkage,flow_must_be_unique_and_in_the_current_viewport_world,
 retired_or_replaced_flows_are_seen_without_waiting_for_a_rescan,bad_arrays_and_focus_ranges_are_rejected,labels_new_badges_and_unavailable_font,database_child_owns_input_while_open,
 numbered_and_case_variant_identifiers_match,movie_story_child_uses_wdchild_and_greyed_story_entry,bgm_and_voice_child_lists,story_item_details_only_when_drawn,
 character_viewer_reads_header_and_never_the_placeholder_profile,model_viewer_reads_its_visible_control_guide_without_hidden_header,late_visibility_reads_cannot_return_retired_or_retargeted_selection,artwork_grid_index_math_count_and_badge,artwork_details_have_no_list_position,late_artwork_help_reads_cannot_mix_focus_or_retired_cells,menu_hints_reads_a_direct_numbered_button_guide,modal_dialogs_keep_priority_over_collection_and_return_immediately}){
  try{test();}catch(const std::exception& e){++failed;std::cerr<<"FAIL "<<e.what()<<"\n";}}
 std::cout<<checks<<" checks, "<<failed<<" failed\n";return failed?1:0;}
