#include "mvci/unreal.hpp"
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <limits>
using namespace mvci;
static int checks=0;
#define CHECK(x) do{++checks;if(!(x))throw std::runtime_error(std::string(__func__)+":"+std::to_string(__LINE__)+" " #x);}while(false)
// Independent hand-encoded layouts from retained Ghidra/PE and live-UI evidence.
struct Fixture {
 static constexpr Address base=0x140000000,heap=0x20000000;
 std::map<Address,std::vector<unsigned char>> blocks;
 Address next=heap,entries=0,names=0,chunk=0;int objects=0,name_count=0;
 std::map<std::wstring,int> name_ids;std::map<std::wstring,Address> kinds;
 Memory memory;Unreal unreal;
 Fixture():unreal(memory,base){
  memory.reader=[this](Address a,void* out,std::size_t n){auto i=blocks.upper_bound(a);if(i==blocks.begin())return false;--i;if(a<i->first||a-i->first>i->second.size()||n>i->second.size()-(a-i->first))return false;std::memcpy(out,i->second.data()+a-i->first,n);return true;};
  blocks[base+0x35cafd0].resize(8);blocks[base+0x35d36f0].resize(0x20);names=alloc(0x408);chunk=alloc(16384*8);entries=alloc(200*24);
  put(base+0x35cafd0,names);put(names,chunk);put(base+0x35d36f0+0x10,entries);put<int>(base+0x35d36f0+0x18,200);put<int>(base+0x35d36f0+0x1c,0);
 }
 Address alloc(std::size_t n){Address a=next;next+=(n+0x1f)&~std::size_t(0x1f);blocks[a].resize(n);return a;}
 template<class T> T raw(Address a){T v{};if(!memory.reader(a,&v,sizeof(v)))throw std::runtime_error("fixture read");return v;}
 template<class T> void put(Address a,T v){auto i=blocks.upper_bound(a);if(i==blocks.begin())throw std::runtime_error("fixture range");--i;if(a-i->first+sizeof(T)>i->second.size())throw std::runtime_error("fixture bounds");std::memcpy(i->second.data()+a-i->first,&v,sizeof(v));}
 int name_id(const std::wstring& s){auto i=name_ids.find(s);if(i!=name_ids.end())return i->second;int n=name_count++;auto a=alloc(0x10+2*(s.size()+1));put<std::uint32_t>(a,1);for(std::size_t j=0;j<s.size();++j)put<std::uint16_t>(a+0x10+j*2,static_cast<std::uint16_t>(s[j]));put(chunk+static_cast<Address>(n)*8,a);name_ids[s]=n;return n;}
 Address object(const std::wstring& name,Address cls=0,std::size_t size=0x500){auto a=alloc(size);int index=objects++;put(a,base+0x1000);put(entries+index*24,a);put<int>(entries+index*24+0x10,100+index);put<int>(a+0xc,index);put(a+0x10,cls);put<int>(a+0x18,name_id(name));put<int>(base+0x35d36f0+0x1c,objects);return a;}
 Address kind(const std::wstring& name){if(kinds.count(name))return kinds[name];if(name==L"Class"){auto a=object(name);put(a+0x10,a);return kinds[name]=a;}return kinds[name]=object(name,kind(L"Class"));}
 Address klass(const std::wstring& name,Address parent=0){auto a=object(name,kind(L"Class"));put(a+0x30,parent);put<int>(a+0x40,0x500);return a;}
 Address prop(Address cls,const std::wstring& name,const std::wstring& kind_name,int offset,int size){auto p=object(name,kind(kind_name));put<int>(p+0x50,offset);put<int>(p+0x34,size);auto old=raw<Address>(cls+0x38);put(p+0x28,old);put(cls+0x38,p);if(kind_name==L"BoolProperty"){put<std::uint8_t>(p+0x78,1);put<std::uint8_t>(p+0x7b,0xff);}return p;}
 void str(Address at,const std::wstring& value){auto data=alloc((value.size()+1)*2);for(std::size_t j=0;j<value.size();++j)put<std::uint16_t>(data+j*2,static_cast<std::uint16_t>(value[j]));put(at,data);put<int>(at+8,static_cast<int>(value.size()+1));put<int>(at+12,static_cast<int>(value.size()+1));}
 void text(Address at,const std::wstring& value){auto td=alloc(0x80),vt=alloc(24),view=alloc(16);put(vt+8,base+0x1876760);put(td,vt);put(td+8,view);str(view,value);put(at,td);}
 Address window(){auto widget=klass(L"Widget"),user=klass(L"UserWidget",widget),window_cls=klass(L"WD_WindowBase_C",user);prop(widget,L"Visibility",L"ByteProperty",0x91,1);prop(widget,L"Slot",L"ObjectProperty",0x30,8);prop(window_cls,L"MessageText",L"TextProperty",0x400,24);prop(window_cls,L"PopupActive",L"BoolProperty",0x420,1);prop(window_cls,L"ButtonNaviArray",L"ArrayProperty",0x440,16);
  // Set inner StrProperty metadata directly; this is not a serialized game asset.
  auto p=raw<Address>(window_cls+0x38);auto inner=object(L"Inner",kind(L"StrProperty"));put<int>(inner+0x34,16);put(p+0x78,inner);
  auto w=object(L"WD_WindowBase_C_0",window_cls);auto shared=alloc(16);put<int>(shared+8,1);put(w+0x208,heap);put(w+0x210,shared);put<std::uint8_t>(w+0x420,1);text(w+0x400,L"Save data could not be found.\r\nCreate new save data now?\r\nNote: If your data is corrupted, it will be overwritten.");
  auto nav=alloc(32);str(nav,L"<_Decide>Yes");str(nav+16,L"<_Cancel>No");put(w+0x440,nav);put<int>(w+0x448,2);put<int>(w+0x44c,2);return w;}
};
static void string_bounds_and_localized_display(){
 Fixture f;auto a=f.alloc(24);f.str(a,L"Visible value");CHECK(f.unreal.string(a)==std::optional<std::wstring>(L"Visible value"));f.put<int>(a+8,80000);CHECK(!f.unreal.string(a));
 f.text(a,L"Save warning");CHECK(f.unreal.text(a)==std::optional<std::wstring>(L"Save warning"));Address td=0;f.memory.read(a,td);f.put(td+8,Address(1));CHECK(!f.unreal.text(a));CHECK(!f.unreal.text(1));
}
static void owned_and_formatted_text_use_the_cached_localized_display(){
 // Live Missions progress and rate use different IFTextData getters. Ghidra
 // proves each returns the shared display at +8, or an inline FString fallback.
 Fixture f;
 for(auto layout:{std::pair<Address,Address>{0x9677c0,0x38},{0x967790,0x48}}){
  auto at=f.alloc(24),data=f.alloc(0x80),vt=f.alloc(24);f.put(at,data);f.put(data,vt);f.put(vt+8,Fixture::base+layout.first);
  f.str(data+layout.second,L"Completed: 0/5");CHECK(f.unreal.text(at)==std::optional<std::wstring>(L"Completed: 0/5"));
  auto shared=f.alloc(16);f.str(shared,L"Localized shared display");f.put(data+8,shared);CHECK(f.unreal.text(at)==std::optional<std::wstring>(L"Localized shared display"));
  f.put(data+8,Address(0));f.put<int>(data+layout.second+8,90000);CHECK(!f.unreal.text(at));
  f.put(vt+8,Fixture::base+0x9677d0);CHECK(!f.unreal.text(at)); // Unknown getter remains unavailable.
 }
}
static void object_lifetime_and_class_cycle(){
 Fixture f;auto cls=f.klass(L"Widget");auto o=f.object(L"Sample",cls);auto ref=f.unreal.object(o);CHECK(ref);CHECK(f.unreal.alive(*ref));CHECK(f.unreal.is_a(o,L"Widget"));CHECK(!f.unreal.is_a(o,L"UserWidget"));f.put<int>(f.entries+ref->index*24+0x10,999);CHECK(!f.unreal.alive(*ref));f.put(cls+0x30,cls);CHECK(!f.unreal.is_a(o,L"Missing"));f.put<std::uint32_t>(o+8,0x8000);CHECK(!f.unreal.object(o));
}
static void startup_dialog_reads_warning_and_buttons(){
 Fixture f;auto w=f.window();MenuCapture capture(f.unreal);auto s=capture.sample(1000);CHECK(s.valid);CHECK(s.modal);CHECK(s.label.find(L"overwritten")!=std::wstring::npos);CHECK(s.help.find(L"Yes")!=std::wstring::npos);CHECK(s.help.find(L"No")!=std::wstring::npos);CHECK(s.help.find(L"Confirm")!=std::wstring::npos);
 f.put<std::uint8_t>(w+0x91,2);CHECK(!capture.sample(1010).valid);f.put<std::uint8_t>(w+0x91,0);Address shared=0;f.memory.read(w+0x210,shared);f.put<int>(shared+8,0);CHECK(!capture.sample(1020).valid);
}
static void malformed_arrays_and_hidden_ancestors(){
 Fixture f;auto w=f.window();auto p=f.unreal.property(w,L"ButtonNaviArray");CHECK(p);CHECK(f.unreal.array(w,*p)->count==2);f.put<int>(w+0x448,3);CHECK(!f.unreal.array(w,*p));f.put<int>(w+0x448,-1);CHECK(!f.unreal.array(w,*p));f.put<int>(w+0x448,2);
 auto slot=f.alloc(0x40);f.put(w+0x30,slot);f.put(slot+0x28,w);CHECK(!f.unreal.shown(w));
}
static void installed_registry_reserve_does_not_hide_live_objects(){
 Fixture f;auto w=f.window();f.put<int>(Fixture::base+0x35d36f0+0x18,0x200000);CHECK(f.unreal.object(w));
}
static void enum_visibility_is_read_at_its_native_byte_width(){
 Fixture f;auto w=f.window();auto p=f.unreal.property(w,L"Visibility");CHECK(p);f.put(p->address+0x10,f.kind(L"EnumProperty"));CHECK(f.unreal.shown(w));f.put<std::uint8_t>(w+0x91,1);CHECK(!f.unreal.shown(w));
}
static void title_reads_the_live_platform_prompt(){
 Fixture f;auto widget=f.klass(L"Widget"),user=f.klass(L"UserWidget",widget),title=f.klass(L"WD_Title_C",user),text=f.klass(L"TextBlock",widget);f.prop(widget,L"Visibility",L"EnumProperty",0x91,1);auto prompt=f.prop(title,L"TextBlock",L"ObjectProperty",0x400,8);f.put<int>(prompt+0x1c,214);f.prop(text,L"Text",L"TextProperty",0x140,24);auto root=f.object(L"Title",title),label=f.object(L"Prompt",text),slot=f.object(L"Slot",f.kind(L"CanvasPanelSlot"));f.put(root+0x400,label);f.put(slot+0x28,root);f.put(label+0x30,slot);auto shared=f.alloc(16);f.put<int>(shared+8,1);f.put(root+0x208,Fixture::heap);f.put(root+0x210,shared);f.text(label+0x140,L"Press Enter");MenuCapture capture(f.unreal);auto s=capture.sample(1000);CHECK(s.valid);CHECK(!s.modal);CHECK(s.label==L"Press Enter");f.put<std::uint8_t>(label+0x91,2);CHECK(!capture.sample(1010).valid);
}
static void numbered_property_names_are_exact_and_distinct(){
 Fixture f;auto cls=f.klass(L"NumberedFields"),first=f.prop(cls,L"TextBlock",L"ObjectProperty",0x400,8),second=f.prop(cls,L"TextBlock",L"ObjectProperty",0x408,8),root=f.object(L"Instance",cls);f.put<int>(first+0x1c,214);f.put<int>(second+0x1c,215);
 auto a=f.unreal.property(root,L"TextBlock_213"),b=f.unreal.property(root,L"TextBlock_214");CHECK(a&&a->offset==0x400);CHECK(b&&b->offset==0x408);CHECK(!f.unreal.property(root,L"TextBlock"));CHECK(!f.unreal.property(root,L"TextBlock_215"));
 // FName comparison is case-insensitive; the numeric identity remains exact.
 auto mixed=f.unreal.property(root,L"tEXTbLOCK_213");CHECK(mixed&&mixed->offset==0x400);CHECK(!f.unreal.property(root,L"textblock_212"));
 CHECK(f.unreal.name(first,true)==L"TextBlock_213"); // Displayed strings are not rewritten.
 f.prop(cls,L"teamNo",L"IntProperty",0x410,4);f.put<int>(root+0x410,1);
 CHECK(f.unreal.integer(root,L"TeamNo")==std::optional<int>(1));CHECK(!f.unreal.property(root,L"Team"));
}
static void drawn_numeric_values_are_typed_finite_and_instance_bound(){
 Fixture f;auto cls=f.klass(L"BP_GemGauge_C");f.prop(cls,L"VerGemGauge",L"FloatProperty",0x400,4);auto actor=f.object(L"Gauge",cls);f.put<float>(actor+0x400,.625f);CHECK(f.unreal.number(actor,L"VerGemGauge")==std::optional<double>(.625));f.put<float>(actor+0x400,std::numeric_limits<float>::infinity());CHECK(!f.unreal.number(actor,L"VerGemGauge"));CHECK(!f.unreal.number(actor,L"Missing"));CHECK(f.unreal.instances(L"BP_GemGauge_C").size()==1);f.put<std::uint32_t>(actor+8,0x10);CHECK(f.unreal.instances(L"BP_GemGauge_C").empty());
}
static void newly_created_modal_is_seen_on_the_first_valid_capture(){
 Fixture f;auto old=f.window();f.put<std::uint8_t>(old+0x420,0);MenuCapture c(f.unreal);CHECK(!c.sample(1000).valid);auto fresh=f.window();f.text(fresh+0x400,L"New prompt");auto s=c.sample(1001);CHECK(s.valid);CHECK(s.label==L"New prompt");
}
static void prompt_names_the_keys_only_when_the_pc_guide_is_drawn(){
 Fixture f;auto w=f.window(),cls=f.raw<Address>(w+0x10),user=f.raw<Address>(cls+0x30),widget=f.raw<Address>(user+0x30);auto nav_cls=f.klass(L"WD_BtnNavi_C",user);f.prop(cls,L"WD_BtnNavi",L"ObjectProperty",0x480,8);f.prop(nav_cls,L"HorizontalBox_PC",L"ObjectProperty",0x400,8);f.prop(nav_cls,L"HorizontalBox_32",L"ObjectProperty",0x408,8);auto p=f.prop(nav_cls,L"TextArray",L"ArrayProperty",0x440,16),inner=f.object(L"Inner",f.kind(L"StrProperty"));f.put<int>(inner+0x34,16);f.put(p+0x78,inner);
 auto nav=f.object(L"Nav",nav_cls),pc=f.object(L"PC",widget),pad=f.object(L"Pad",widget);f.put(w+0x480,nav);f.put(nav+0x400,pc);f.put(nav+0x408,pad);f.put(nav+0x440,f.raw<Address>(w+0x440));f.put<int>(nav+0x448,2);f.put<int>(nav+0x44c,2);
 for(auto pair:{std::pair<Address,Address>{nav,w},{pc,nav},{pad,nav}}){auto slot=f.object(L"Slot",f.kind(L"HorizontalBoxSlot"));f.put(slot+0x28,pair.second);f.put(pair.first+0x30,slot);}f.put<std::uint8_t>(pad+0x91,2);
 MenuCapture c(f.unreal);auto s=c.sample(1000);CHECK(s.valid);CHECK(s.help.find(L"Enter: Yes")!=std::wstring::npos);CHECK(s.help.find(L"Backspace: No")!=std::wstring::npos);
 f.put<std::uint8_t>(pc+0x91,2);f.put<std::uint8_t>(pad+0x91,0);s=c.sample(1010);CHECK(s.help.find(L"Enter")==std::wstring::npos);CHECK(s.help.find(L"Confirm: Yes")!=std::wstring::npos);
}
static void hidden_dialog_body_never_exposes_the_stored_message(){
 Fixture f;auto w=f.window(),cls=f.raw<Address>(w+0x10),user=f.raw<Address>(cls+0x30),widget=f.raw<Address>(user+0x30),text_cls=f.klass(L"TextBlock",widget);f.prop(cls,L"TextBlock_MessageMain",L"ObjectProperty",0x480,8);f.prop(text_cls,L"Text",L"TextProperty",0x140,24);auto text=f.object(L"Body",text_cls),slot=f.object(L"Slot",f.kind(L"CanvasPanelSlot"));f.put(w+0x480,text);f.put(text+0x30,slot);f.put(slot+0x28,w);f.text(w+0x400,L"Rendered warning");f.text(text+0x140,L"Rendered warning");MenuCapture c(f.unreal);CHECK(c.sample(1000).label==L"Rendered warning");f.put<std::uint8_t>(text+0x91,2);auto pending=c.sample(1010);CHECK(!pending.valid);CHECK(pending.blocked);
}
static void dialog_waits_for_the_rendered_message_to_be_initialized(){
 Fixture f;auto w=f.window(),cls=f.raw<Address>(w+0x10),user=f.raw<Address>(cls+0x30),widget=f.raw<Address>(user+0x30),text_cls=f.klass(L"TextBlock",widget);f.prop(cls,L"TextBlock_MessageMain",L"ObjectProperty",0x480,8);f.prop(text_cls,L"Text",L"TextProperty",0x140,24);auto block=f.object(L"Body",text_cls),slot=f.object(L"Slot",f.kind(L"CanvasPanelSlot"));f.put(w+0x480,block);f.put(block+0x30,slot);f.put(slot+0x28,w);
 f.text(w+0x400,L"Retrieving Fighter Card...");f.text(block+0x140,L"WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW");MenuCapture c(f.unreal);auto pending=c.sample(1000);CHECK(!pending.valid);CHECK(pending.modal&&pending.blocked);
 f.text(block+0x140,L"Retrieving Fighter Card...");auto ready=c.sample(1001);CHECK(ready.valid);CHECK(ready.label==L"Retrieving Fighter Card...");
 f.text(w+0x400,L"Uploading changes...");pending=c.sample(1002);CHECK(!pending.valid);CHECK(pending.blocked);f.text(block+0x140,L"Uploading changes...");CHECK(c.sample(1003).label==L"Uploading changes...");
}
static void generic_menu_follows_displayed_focus_value_visibility_and_enabled_state(){
 Fixture f;auto unused=f.window();f.put<std::uint8_t>(unused+0x420,0);auto window_cls=f.raw<Address>(unused+0x10),user=f.raw<Address>(window_cls+0x30),widget=f.raw<Address>(user+0x30),cls=f.klass(L"WD_MenuBase_C",user),row_cls=f.klass(L"WD_OptionItemBase_C",user),text_cls=f.klass(L"TextBlock",widget);f.prop(text_cls,L"Text",L"TextProperty",0x140,24);f.prop(cls,L"ItemIndex",L"IntProperty",0x400,4);f.prop(cls,L"IsMenuBaseInput",L"BoolProperty",0x420,1);auto items=f.prop(cls,L"Items",L"ArrayProperty",0x440,16),inner=f.object(L"Inner",f.kind(L"ObjectProperty"));f.put<int>(inner+0x34,8);f.put(items+0x78,inner);f.prop(row_cls,L"ItemName",L"TextProperty",0x400,24);f.prop(row_cls,L"ItemLabel",L"ObjectProperty",0x428,8);f.prop(row_cls,L"IsDisable",L"BoolProperty",0x430,1);
 auto root=f.object(L"Options",cls),shared=f.alloc(16);f.put<int>(shared+8,1);f.put(root+0x208,Fixture::heap);f.put(root+0x210,shared);f.put<std::uint8_t>(root+0x420,1);auto list=f.alloc(16);f.put(root+0x440,list);f.put<int>(root+0x448,2);f.put<int>(root+0x44c,2);Address rows[2]{},labels[2]{};
 for(int i=0;i<2;++i){rows[i]=f.object(L"Row",row_cls);labels[i]=f.object(L"Label",text_cls);f.put(list+i*8,rows[i]);f.put(rows[i]+0x428,labels[i]);f.text(rows[i]+0x400,L"Unrendered stored label");f.text(labels[i]+0x140,i?L"Difficulty":L"Sound");for(auto pair:{std::pair<Address,Address>{rows[i],root},{labels[i],rows[i]}}){auto slot=f.object(L"Slot",f.kind(L"CanvasPanelSlot"));f.put(pair.first+0x30,slot);f.put(slot+0x28,pair.second);}}
 MenuCapture c(f.unreal);auto s=c.sample(1000);CHECK(s.valid);CHECK(s.label==L"Sound");CHECK(s.position==1&&s.count==2);f.put<int>(root+0x400,1);s=c.sample(1010);CHECK(s.label==L"Difficulty");CHECK(s.position==2);f.put<std::uint8_t>(rows[1]+0x430,1);CHECK(!c.sample(1020).enabled);f.put<std::uint8_t>(labels[1]+0x91,2);CHECK(!c.sample(1030).valid);f.put<std::uint8_t>(labels[1]+0x91,0);f.put<std::uint8_t>(rows[0]+0x91,2);s=c.sample(1040);CHECK(s.position==1&&s.count==1);f.put<int>(root+0x400,9);CHECK(!c.sample(1050).valid);f.put<int>(root+0x400,1);f.put<std::uint8_t>(root+0x420,0);CHECK(!c.sample(1060).valid);
}
static void main_menu_uses_the_selected_index_after_animation_flags_clear(){
 Fixture f;auto unused=f.window();f.put<std::uint8_t>(unused+0x420,0);auto wc=f.raw<Address>(unused+0x10),user=f.raw<Address>(wc+0x30),widget=f.raw<Address>(user+0x30),cls=f.klass(L"WD_MainMenu_C",user),row_cls=f.klass(L"WD_MainMenuItem_C",user),text_cls=f.klass(L"TextBlock",widget);
 f.prop(cls,L"IndexFocus",L"IntProperty",0x400,4);f.prop(cls,L"ActiveMenu",L"ObjectProperty",0x408,8);f.prop(cls,L"IsMainMenuInputEnable",L"BoolProperty",0x420,1);auto p=f.prop(cls,L"ListButtonItemArray",L"ArrayProperty",0x440,16),inner=f.object(L"Inner",f.kind(L"ObjectProperty"));f.put<int>(inner+0x34,8);f.put(p+0x78,inner);f.prop(row_cls,L"TextBlock_Item",L"ObjectProperty",0x400,8);f.prop(row_cls,L"FocusFlag",L"BoolProperty",0x408,1);f.prop(row_cls,L"index",L"IntProperty",0x40c,4);f.prop(text_cls,L"Text",L"TextProperty",0x140,24);
 auto root=f.object(L"Main",cls),shared=f.alloc(16),list=f.alloc(24);f.put<int>(shared+8,1);f.put(root+0x208,Fixture::heap);f.put(root+0x210,shared);f.put(root+0x408,root);f.put<int>(root+0x400,1);f.put<std::uint8_t>(root+0x420,1);f.put(root+0x440,list);f.put<int>(root+0x448,3);f.put<int>(root+0x44c,3);Address rows[3]{},labels[3]{};const wchar_t* names[]={L"STORY",L"BATTLE",L"TRAINING"};
 for(int i=0;i<3;++i){rows[i]=f.object(L"Item",row_cls);labels[i]=f.object(L"Label",text_cls);f.put(list+i*8,rows[i]);f.put(rows[i]+0x400,labels[i]);f.put<int>(rows[i]+0x40c,i);f.text(labels[i]+0x140,names[i]);for(auto pair:{std::pair<Address,Address>{rows[i],root},{labels[i],rows[i]}}){auto slot=f.object(L"Slot",f.kind(L"CanvasPanelSlot"));f.put(pair.first+0x30,slot);f.put(slot+0x28,pair.second);}}
 // The real Blueprint clears every FocusFlag after its entry animation.
 MenuCapture c(f.unreal);auto s=c.sample(1000);CHECK(s.valid);CHECK(s.label==L"BATTLE");CHECK(s.position==2&&s.count==3);
 f.put<int>(root+0x400,2);f.put<std::uint8_t>(rows[1]+0x408,1);s=c.sample(1001);CHECK(s.valid);CHECK(s.label==L"TRAINING");CHECK(s.position==3);
 f.put<std::uint8_t>(rows[1]+0x408,0);CHECK(c.sample(1002).label==L"TRAINING");f.put<int>(root+0x400,3);CHECK(!c.sample(1003).valid);f.put<int>(root+0x400,2);
 f.put<int>(rows[2]+0x40c,1);CHECK(!c.sample(1004).valid);f.put<int>(rows[2]+0x40c,2);f.put<std::uint8_t>(labels[2]+0x91,2);CHECK(!c.sample(1005).valid);f.put<std::uint8_t>(labels[2]+0x91,0);
 auto child=f.object(L"Submenu",f.klass(L"WD_Other_C",user));f.put(root+0x408,child);CHECK(!c.sample(1006).valid);f.put(root+0x408,root);CHECK(c.sample(1007).label==L"TRAINING");f.put<std::uint8_t>(root+0x420,0);CHECK(!c.sample(1008).valid);
}
int main(){int failed=0;for(auto test:{string_bounds_and_localized_display,owned_and_formatted_text_use_the_cached_localized_display,object_lifetime_and_class_cycle,startup_dialog_reads_warning_and_buttons,malformed_arrays_and_hidden_ancestors,installed_registry_reserve_does_not_hide_live_objects,enum_visibility_is_read_at_its_native_byte_width,title_reads_the_live_platform_prompt,numbered_property_names_are_exact_and_distinct,drawn_numeric_values_are_typed_finite_and_instance_bound,newly_created_modal_is_seen_on_the_first_valid_capture,prompt_names_the_keys_only_when_the_pc_guide_is_drawn,hidden_dialog_body_never_exposes_the_stored_message,dialog_waits_for_the_rendered_message_to_be_initialized,generic_menu_follows_displayed_focus_value_visibility_and_enabled_state,main_menu_uses_the_selected_index_after_animation_flags_clear}){try{test();}catch(const std::exception& e){std::cerr<<e.what()<<'\n';++failed;}}std::cout<<checks<<" native checks, "<<failed<<" failed groups\n";return failed?1:0;}
