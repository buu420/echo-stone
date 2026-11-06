#include "mvci/unreal.hpp"
#include "mvci/localization.hpp"
#include <algorithm>
#include <cmath>
#include <cwctype>
#include <set>
namespace mvci {
MenuSnapshot story_commands(Unreal&,const std::function<std::optional<int>(Address)>&);
MenuSnapshot online_menus(Unreal&,Address,const std::function<std::optional<int>(Address)>&);
std::optional<MenuSnapshot> rendered_menu_option(Unreal&,Address,const std::function<std::optional<int>(Address)>&);
void append_pause_context(Unreal&,Address,MenuSnapshot&,const std::function<std::optional<int>(Address)>&);
namespace {
struct Header {Address vtable;std::uint32_t flags;int index;Address cls;int name,number;Address outer;};
struct NativeArray {Address data;int count,capacity;};
struct Registry {Address data;int capacity,count;};
struct Item {Address object;std::uint32_t flags;int cluster,serial,padding;};
static_assert(sizeof(Header)==40&&sizeof(NativeArray)==16&&sizeof(Item)==24);
constexpr Address names_rva=0x35cafd0,objects_rva=0x35d36f0;
bool pointer(Address a){return a>=0x10000&&a<0x800000000000;}
std::uint64_t identity(const ObjectRef& o){return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(o.serial))<<32)|static_cast<std::uint32_t>(o.index);}
bool supported_label(const std::wstring& text){return !text.empty()&&text.rfind(L"msg_",0)!=0;}
std::wstring legend(std::wstring s,bool keyboard){
 if(s.rfind(L"<_",0)!=0)return s;
 const auto end=s.find(L'>');if(end==std::wstring::npos)return s;
 const auto token=s.substr(0,end+1),label=s.substr(end+1);
 if(token==L"<_Decide>")return keyboard?tr(L"Enter: {0}",{label}):tr(L"Confirm: {0}",{label});
 if(token==L"<_Cancel>")return keyboard?tr(L"Backspace: {0}",{label}):tr(L"Cancel: {0}",{label});
 return label;
}
}

bool is_umg_default_text(std::wstring_view text){
 // Engine.locres UMG.TextBlockDefaultValue for the shipped engine cultures.
 return text==L"Text Block"||text==L"Bloque de texto"||text==L"テキストブロック"||
        text==L"텍스트 블록"||text==L"Bloco de Texto";
}

bool Memory::bytes(Address a,void* out,std::size_t size)const{
 if(!out||!reader||!pointer(a)||size>16*1024*1024||a+size<a||a+size>=0x800000000000)return false;
 try{return reader(a,out,size);}catch(...){return false;}
}
std::optional<ObjectRef> Unreal::object(Address a)const{
 Header h{};if(!memory_.read(a,h)||!pointer(h.vtable)||!pointer(h.cls)||h.index<0||(h.flags&0x18000))return {};
 Registry registry{};if(!memory_.read(base_+objects_rva+0x10,registry)||registry.count<0||registry.count>registry.capacity||registry.capacity>0x400000||h.index>=registry.count)return {};
 Item item{};if(!memory_.read(registry.data+static_cast<Address>(h.index)*24,item)||item.object!=a||(item.flags&0x30000000))return {};
 return ObjectRef{a,h.cls,h.index,item.serial,h.flags};
}
bool Unreal::alive(const ObjectRef& old)const{auto now=object(old.address);return now&&now->cls==old.cls&&now->index==old.index&&now->serial==old.serial;}
std::optional<Address> Unreal::current_world()const{
 const auto engine=memory_.get<Address>(base_+0x36bc348);if(!engine||!*engine)return {};
 const auto viewport=memory_.get<Address>(*engine+0x5d8);if(!viewport||!*viewport)return {};
 const auto world=memory_.get<Address>(*viewport+0x80);if(!world)return {};
 const auto ref=object(*world);if(!ref||(ref->flags&0x30)||!is_a(*world,L"World"))return {};
 return world;
}
std::wstring Unreal::name(Address a,bool numbered)const{
 Header h{};if(!memory_.read(a,h)||h.name<0||h.name>=1048576||h.number<0||h.number>10000000)return {};
 auto found=names_.find(h.name);std::wstring value;
 if(found!=names_.end())value=found->second;
 else{
  const auto table=memory_.get<Address>(base_+names_rva);if(!table||!pointer(*table))return {};
  const auto chunk=memory_.get<Address>(*table+static_cast<Address>(h.name/16384)*8);if(!chunk)return {};
  const auto entry=memory_.get<Address>(*chunk+static_cast<Address>(h.name%16384)*8);if(!entry)return {};
  const auto flags=memory_.get<std::uint32_t>(*entry);if(!flags)return {};
  bool ended=false;
  for(std::size_t i=0;i<256;++i){
   std::uint16_t c=0;
   if(*flags&1){if(!memory_.read(*entry+0x10+i*2,c))return {};}
   else{std::uint8_t byte=0;if(!memory_.read(*entry+0x10+i,byte))return {};c=byte;}
   if(!c){ended=true;break;}value.push_back(static_cast<wchar_t>(c));
  }
  if(!ended)return {};names_[h.name]=value;
 }
 if(numbered&&h.number)value+=L"_"+std::to_wstring(h.number-1);
 return value;
}
bool Unreal::name_is(Address a,const std::wstring& expected)const{
 // UE4 FNames compare without case; their retained spelling can depend on load order.
 // These supported-build identifiers are ASCII. Keep numeric suffixes and all other
 // characters exact, and leave localized labels/display strings untouched.
 const auto actual=name(a,true);
 const auto fold=[](wchar_t c){return c>=L'A'&&c<=L'Z'?static_cast<wchar_t>(c+(L'a'-L'A')):c;};
 return !expected.empty()&&actual.size()==expected.size()&&std::equal(actual.begin(),actual.end(),expected.begin(),[&](wchar_t x,wchar_t y){return fold(x)==fold(y);});
}
std::wstring Unreal::type(Address a)const{auto o=object(a);return o?name(o->cls):L"";}
bool Unreal::is_a(Address a,const std::wstring& expected)const{
 auto o=object(a);if(!o)return false;Address cls=o->cls;std::set<Address> seen;
 while(cls&&seen.size()<64&&seen.insert(cls).second){if(!object(cls))return false;if(name(cls)==expected)return true;auto parent=memory_.get<Address>(cls+0x30);if(!parent)return false;cls=*parent;}
 return false;
}
std::optional<Property> Unreal::property(Address a,const std::wstring& wanted)const{
 auto o=object(a);if(!o)return {};const auto object_size=memory_.get<int>(o->cls+0x40);if(!object_size||*object_size<40||*object_size>0x1000000)return {};
 std::set<Address> classes;Address cls=o->cls;
 while(cls&&classes.size()<64&&classes.insert(cls).second){
  if(!object(cls))return {};auto field=memory_.get<Address>(cls+0x38);if(!field)return {};std::set<Address> seen;
  while(*field&&seen.size()<4096&&seen.insert(*field).second){
   auto ref=object(*field);if(!ref)return {};
   if(name_is(*field,wanted)){
    Property p;p.address=*field;p.kind=type(*field);const auto offset=memory_.get<int>(*field+0x50),size=memory_.get<int>(*field+0x34);
    if(p.kind.size()<8||p.kind.substr(p.kind.size()-8)!=L"Property"||!offset||!size||*offset<0||*size<=0||*offset>*object_size||*size>*object_size-*offset)return {};
    p.offset=*offset;p.size=*size;
    if(p.kind==L"BoolProperty"){
     const auto byte=memory_.get<std::uint8_t>(*field+0x79),mask=memory_.get<std::uint8_t>(*field+0x7b);if(!byte||!mask||!*mask||*byte>=p.size)return {};p.bool_byte=*byte;p.bool_mask=*mask;
    }
    if(p.kind==L"ArrayProperty"){
     auto inner=memory_.get<Address>(*field+0x78);if(!inner||!object(*inner))return {};p.inner=type(*inner);auto stride=memory_.get<int>(*inner+0x34);if(!stride||*stride<=0||*stride>4096)return {};p.stride=*stride;
    }
    return p;
   }
   field=memory_.get<Address>(*field+0x28);if(!field)return {};
  }
  auto parent=memory_.get<Address>(cls+0x30);if(!parent)return {};cls=*parent;
 }
 return {};
}
std::optional<std::wstring> Unreal::string(Address a)const{
 NativeArray v{};if(!memory_.read(a,v)||v.count<0||v.count>8192||v.capacity<v.count||v.capacity>65536)return {};
 if(!v.count)return L"";std::vector<std::uint16_t> chars(static_cast<std::size_t>(v.count));if(!memory_.bytes(v.data,chars.data(),chars.size()*2)||chars.back()!=0)return {};
 std::wstring out;out.reserve(chars.size()-1);for(std::size_t i=0;i+1<chars.size();++i){if(!chars[i])return {};out.push_back(static_cast<wchar_t>(chars[i]));}return normalize_text(out);
}
std::optional<std::wstring> Unreal::text(Address a)const{
 const auto data=memory_.get<Address>(a);if(!data||!pointer(*data))return {};
 const auto vt=memory_.get<Address>(*data);if(!vt)return {};const auto getter=memory_.get<Address>(*vt+8);
 // Supported-build IFTextData getters, verified in Ghidra. All return the
 // shared localized display at +8 when present. The two owned/formatted
 // implementations otherwise return their cached inline FString (+38/+48).
 // Never evaluate FTextHistory or execute a getter to regenerate its text.
 if(!getter||(*getter!=base_+0x1876760&&*getter!=base_+0x9677c0&&*getter!=base_+0x967790))return {};
 const auto display=memory_.get<Address>(*data+8);if(!display)return {};
 if(*display)return string(*display);
 if(*getter==base_+0x9677c0)return string(*data+0x38);
 if(*getter==base_+0x967790)return string(*data+0x48);
 return {};
}
std::optional<ArrayView> Unreal::array(Address a,const Property& p,int limit)const{
 if(p.kind!=L"ArrayProperty"||p.size!=16||p.stride<=0||limit<0)return {};NativeArray v{};
 if(!memory_.read(a+static_cast<Address>(p.offset),v)||v.count<0||v.count>limit||v.capacity<v.count||v.capacity>65536||(v.count&&!pointer(v.data)))return {};
 return ArrayView{v.data,v.count,p.stride};
}
std::optional<Address> Unreal::object_field(Address a,const std::wstring& n)const{
 auto p=property(a,n);if(!p||p->kind!=L"ObjectProperty"||p->size!=8)return {};auto v=memory_.get<Address>(a+p->offset);if(!v||(*v&&!object(*v)))return {};return v;
}
std::optional<int> Unreal::integer(Address a,const std::wstring& n)const{
 auto p=property(a,n);if(!p)return {};if(p->kind==L"IntProperty"&&p->size==4)return memory_.get<int>(a+p->offset);
 if((p->kind==L"ByteProperty"||p->kind==L"EnumProperty")&&p->size==1){auto b=memory_.get<std::uint8_t>(a+p->offset);if(b)return *b;}return {};
}
std::optional<double> Unreal::number(Address a,const std::wstring& n)const{
 auto p=property(a,n);if(!p)return {};
 std::optional<double> result;
 if(p->kind==L"FloatProperty"&&p->size==4){if(auto v=memory_.get<float>(a+p->offset))result=static_cast<double>(*v);}
 else if(p->kind==L"DoubleProperty"&&p->size==8)result=memory_.get<double>(a+p->offset);
 return result&&std::isfinite(*result)?result:std::nullopt;
}
std::optional<bool> Unreal::boolean(Address a,const std::wstring& n)const{
 auto p=property(a,n);if(!p||p->kind!=L"BoolProperty")return {};auto b=memory_.get<std::uint8_t>(a+p->offset+p->bool_byte);if(!b)return {};return (*b&p->bool_mask)!=0;
}
std::optional<std::wstring> Unreal::text_field(Address a,const std::wstring& n)const{
 auto p=property(a,n);if(!p||p->kind!=L"TextProperty"||p->size!=24)return {};return text(a+p->offset);
}
std::vector<ObjectRef> Unreal::widgets()const{
 return instances(L"UserWidget");
}
std::optional<int> Unreal::object_count()const{
 Registry r{};if(!memory_.read(base_+objects_rva+0x10,r)||r.count<0||r.count>r.capacity||r.capacity<0||r.capacity>0x400000)return {};return r.count;
}
std::vector<ObjectRef> Unreal::instances(const std::wstring& class_name)const{
 if(discovery_managed_){if(const auto mask=discovery_class(class_name))return discovered_instances(*mask);++discovery_stats_.fallback_scans;}
 std::vector<ObjectRef> out;Registry registry{};if(!memory_.read(base_+objects_rva+0x10,registry)||registry.count<0||registry.count>registry.capacity||registry.capacity>0x400000)return out;
 std::map<Address,bool> class_is_widget;
 for(int i=0;i<registry.count;++i){
  Item item{};if(!memory_.read(registry.data+static_cast<Address>(i)*24,item))return {};if(!item.object||(item.flags&0x30000000))continue;
  auto o=object(item.object);if(!o||(o->flags&0x30))continue;
  auto found=class_is_widget.find(o->cls);bool widget=false;
  if(found==class_is_widget.end()){widget=is_a(o->address,class_name);class_is_widget[o->cls]=widget;}else widget=found->second;
  if(widget)out.push_back(*o);
 }
 return out;
}
bool Unreal::shown(Address a,const std::function<std::optional<int>(Address)>& effective)const{
 std::set<Address> seen;
 while(a&&seen.size()<64&&seen.insert(a).second){
  auto ref=object(a);if(!ref||(ref->flags&0x30)||!is_a(a,L"Widget"))return false;
  const auto visibility=effective?effective(a):integer(a,L"Visibility");if(!visibility||*visibility<0||*visibility>4||*visibility==1||*visibility==2)return false;
  if(auto color=property(a,L"ColorAndOpacity");color&&color->kind==L"StructProperty"&&color->size==16){auto alpha=memory_.get<float>(a+color->offset+12);if(!alpha||!std::isfinite(*alpha)||*alpha<=0)return false;}
  auto slot=memory_.get<Address>(a+0x30);if(!slot)return false;
  if(*slot){if(!object(*slot))return false;auto parent=memory_.get<Address>(*slot+0x28);if(!parent)return false;if(*parent){a=*parent;continue;}}
  if(is_a(a,L"UserWidget")){
   auto view=memory_.get<Address>(a+0x208),shared=memory_.get<Address>(a+0x210);
   if(view&&*view&&shared&&*shared){auto count=memory_.get<int>(*shared+8);if(count&&*count>0)return true;}
  }
  auto outer=memory_.get<Address>(a+0x20);if(!outer||!*outer)return false;
  if(type(*outer)==L"WidgetTree"){auto owner=memory_.get<Address>(*outer+0x20);if(!owner)return false;a=*owner;}
  else if(is_a(*outer,L"UserWidget"))a=*outer;
  else return false;
 }
 return false;
}
std::wstring menu_hints(const Unreal& unreal,Address root,const std::function<std::optional<int>(Address)>& visibility,bool direct_btnnavi){
 Address source=root;std::wstring field=L"ButtonNaviArray";bool keyboard=false;
 if(direct_btnnavi){
  // Numbered guide fields (WD_ArtListTextNavi.WD_BtnNavi_9) hand over the guide itself.
  if(!unreal.object(root)||!unreal.is_a(root,L"WD_BtnNavi_C")||!unreal.shown(root,visibility))return {};
  field=L"TextArray";
 }
 else if(auto nav=unreal.object_field(root,L"WD_BtnNavi")){if(!*nav||!unreal.shown(*nav,visibility))return {};source=*nav;field=L"TextArray";}
 // WD_BtnNavi switches these actual rendered panels when input changes.
 // ConvBtnIconPC maps Decide/Cancel to the Enter/Backspace glyphs observed
 // at the save dialog. Never infer PC mode merely from the host platform.
 const auto pc=unreal.object_field(source,L"HorizontalBox_PC"),pad=unreal.object_field(source,L"HorizontalBox_32");
 keyboard=pc&&*pc&&unreal.shown(*pc,visibility)&&pad&&*pad&&!unreal.shown(*pad,visibility);
 auto p=unreal.property(source,field);if(!p||p->inner!=L"StrProperty"||p->stride!=16)return {};auto arr=unreal.array(source,*p,24);if(!arr)return {};
 std::wstring out;for(int i=0;i<arr->count;++i){auto text=unreal.string(arr->data+static_cast<Address>(i)*16);if(!text)return {};auto line=normalize_text(legend(*text,keyboard));if(line.empty())continue;if(!out.empty())out+=L". ";out+=line;}return out;
}
std::wstring MenuCapture::hints(Address root)const{return menu_hints(unreal_,root,visibility_);}
std::optional<MenuSnapshot> MenuCapture::dialog(const ObjectRef& o)const{
 if(!unreal_.is_a(o.address,L"WD_WindowBase_C")||!unreal_.shown(o.address,visibility_)||unreal_.boolean(o.address,L"PopupActive")!=std::optional<bool>(true))return {};
 auto body=unreal_.text_field(o.address,L"MessageText");
 if(unreal_.property(o.address,L"TextBlock_MessageMain")){
  const auto block=unreal_.object_field(o.address,L"TextBlock_MessageMain");if(!block||!*block||!unreal_.shown(*block,visibility_))return {};
  const auto rendered=unreal_.text_field(*block,L"Text");
  // SetupWindow copies MessageText into this block after the dialog is spawned.
  // Before that copy the block still contains its designer layout placeholder.
  // Require the completed native copy, without speaking the stored value alone.
  if(!body||!rendered||*body!=*rendered)return {};
  body=rendered;
 }
 if(!body||!supported_label(*body)||!unreal_.alive(o))return {};
 MenuSnapshot s;s.valid=true;s.modal=true;s.identity=identity(o);s.screen=tr(L"Dialog");s.label=*body;s.help=hints(o.address);return s;
}
std::optional<MenuSnapshot> MenuCapture::movie_pause(const ObjectRef& o)const{
 // This screen's two guide actions are direct inputs, not focused Items.
 if(!unreal_.is_a(o.address,L"WD_MoviePause_C")||!unreal_.shown(o.address,visibility_)||unreal_.boolean(o.address,L"IsMenuBaseInput")!=std::optional<bool>(true))return {};
 MenuSnapshot pending;pending.identity=identity(o);pending.blocked=true;
 const auto block=unreal_.object_field(o.address,L"TextBlock_55");if(!block||!*block||!unreal_.is_a(*block,L"TextBlock")||!unreal_.shown(*block,visibility_))return pending;
 const auto owned=[&]{const auto tree=unreal_.memory().get<Address>(*block+0x20);return tree&&unreal_.type(*tree)==L"WidgetTree"&&unreal_.memory().get<Address>(*tree+0x20)==std::optional<Address>(o.address);};
 const auto ref=unreal_.object(*block);if(!ref||!owned())return pending;const auto label=unreal_.text_field(*block,L"Text");if(!label||!supported_label(*label))return pending;
 MenuSnapshot s;s.valid=true;s.identity=identity(o);s.label=*label;s.help=hints(o.address);
 if(!unreal_.alive(o)||!unreal_.alive(*ref)||!owned()||unreal_.object_field(o.address,L"TextBlock_55")!=block||unreal_.boolean(o.address,L"IsMenuBaseInput")!=std::optional<bool>(true)||!unreal_.shown(o.address,visibility_)||!unreal_.shown(*block,visibility_))return {};
 return s;
}
std::optional<MenuSnapshot> MenuCapture::menu(const ObjectRef& o)const{
 if(!unreal_.is_a(o.address,L"WD_MenuBase_C")||!unreal_.shown(o.address,visibility_)||unreal_.boolean(o.address,L"IsMenuBaseInput")!=std::optional<bool>(true))return {};
 if(unreal_.boolean(o.address,L"IsPopupActive")==std::optional<bool>(true))return {};
 auto prop=unreal_.property(o.address,L"Items");auto index=unreal_.integer(o.address,L"ItemIndex");if(!prop||!index||prop->inner!=L"ObjectProperty"||prop->stride!=8)return {};
 auto arr=unreal_.array(o.address,*prop,256);if(!arr||*index<0||*index>=arr->count)return {};auto item=unreal_.memory().get<Address>(arr->data+static_cast<Address>(*index)*8);
 const auto item_ref=item?unreal_.object(*item):std::nullopt;
 if(!item_ref||!unreal_.shown(*item,visibility_))return {};
 MenuSnapshot s;
 if(auto rendered=rendered_menu_option(unreal_,*item,visibility_)){
  s=std::move(*rendered);
  if(s.blocked){s.identity=identity(o);return s;}
 }else{
  auto label=unreal_.text_field(*item,L"ItemName");
  if(unreal_.property(*item,L"ItemLabel")){
   const auto block=unreal_.object_field(*item,L"ItemLabel");if(!block||!*block||!unreal_.shown(*block,visibility_))return {};
   label=unreal_.text_field(*block,L"Text");
  }
  if(!label||!supported_label(*label))return {};
  s.valid=true;s.label=*label;
  const auto subprop=unreal_.property(*item,L"SubItemNames");const auto subindex=unreal_.integer(*item,L"SubItemIndex");
  if(subprop&&subindex&&subprop->inner==L"TextProperty"&&subprop->stride==24){auto sub=unreal_.array(*item,*subprop,256);if(sub&&*subindex>=0&&*subindex<sub->count){auto value=unreal_.text(sub->data+static_cast<Address>(*subindex)*24);if(value&&supported_label(*value))s.value=*value;}}
 }
 s.identity=identity(o);s.help=hints(o.address);
 auto disabled=unreal_.boolean(*item,L"IsDisable");if(!disabled)disabled=unreal_.boolean(*item,L"isDisable");s.enabled=!disabled.value_or(false)&&unreal_.boolean(*item,L"bIsEnabled").value_or(true);
 if(auto help=unreal_.text_field(*item,L"LineHelpText");help&&supported_label(*help))s.help=*help+(s.help.empty()?L"":L". "+s.help);
 int visible=0,position=0;bool known=true;
 for(int i=0;i<arr->count;++i){auto row=unreal_.memory().get<Address>(arr->data+static_cast<Address>(i)*8);if(!row||!*row){known=false;break;}if(unreal_.shown(*row,visibility_)){++visible;if(i==*index)position=visible;}}
 if(known){s.position=position;s.count=visible;}
 append_pause_context(unreal_,o.address,s,visibility_);
 const auto current=unreal_.array(o.address,*prop,256);
 if(!unreal_.alive(o)||!unreal_.alive(*item_ref)||!current||current->data!=arr->data||current->count!=arr->count||
    unreal_.boolean(o.address,L"IsMenuBaseInput")!=std::optional<bool>(true)||unreal_.boolean(o.address,L"IsPopupActive")==std::optional<bool>(true)||
    unreal_.integer(o.address,L"ItemIndex")!=index||unreal_.memory().get<Address>(arr->data+static_cast<Address>(*index)*8)!=item){
  MenuSnapshot pending;pending.blocked=true;return pending;
 }
 return s;
}
std::optional<MenuSnapshot> MenuCapture::main_menu(const ObjectRef& o)const{
 if(!unreal_.is_a(o.address,L"WD_MainMenu_C")||!unreal_.shown(o.address,visibility_)||unreal_.boolean(o.address,L"IsMainMenuInputEnable")!=std::optional<bool>(true))return {};
 if(unreal_.object_field(o.address,L"ActiveMenu")!=std::optional<Address>(o.address))return {};
 auto p=unreal_.property(o.address,L"ListButtonItemArray");if(!p||p->inner!=L"ObjectProperty"||p->stride!=8)return {};auto arr=unreal_.array(o.address,*p,32);if(!arr)return {};
 // FocusFlag queues an entry animation and is cleared by Tick. IndexFocus is
 // the persistent selection used by keyboard, pad, hover and cursor rendering.
 const auto index=unreal_.integer(o.address,L"IndexFocus");if(!index||*index<0||*index>=arr->count)return {};
 const auto item=unreal_.memory().get<Address>(arr->data+static_cast<Address>(*index)*8);
 if(!item||!*item||!unreal_.is_a(*item,L"WD_MainMenuItem_C")||unreal_.integer(*item,L"index")!=index||!unreal_.shown(*item,visibility_))return {};
 const auto block=unreal_.object_field(*item,L"TextBlock_Item");if(!block||!*block||!unreal_.shown(*block,visibility_))return {};const auto label=unreal_.text_field(*block,L"Text");if(!label||!supported_label(*label)||!unreal_.alive(o))return {};
 MenuSnapshot s;s.valid=true;s.identity=identity(o);s.label=*label;s.position=*index+1;s.count=arr->count;s.help=hints(o.address);return s;
}
std::optional<MenuSnapshot> MenuCapture::title(const ObjectRef& o)const{
 if(!unreal_.is_a(o.address,L"WD_Title_C")||!unreal_.shown(o.address,visibility_))return {};
 auto block=unreal_.object_field(o.address,L"TextBlock_213");if(!block||!*block||!unreal_.shown(*block,visibility_))return {};
 auto text=unreal_.text_field(*block,L"Text");if(!text||!supported_label(*text))return {};
 MenuSnapshot s;s.valid=true;s.identity=identity(o);s.screen=tr(L"Title");s.label=*text;s.help=hints(o.address);return s;
}
MenuSnapshot MenuCapture::sample(std::uint64_t now){
 const auto count=unreal_.object_count();if(!count){widgets_.clear();object_count_=-1;return {};}
 const bool retired=!unreal_.shared_discovery()&&std::any_of(widgets_.begin(),widgets_.end(),[this](const ObjectRef& o){return !unreal_.alive(o);});
 if(unreal_.shared_discovery()||widgets_.empty()||retired||*count!=object_count_||now<refreshed_||now-refreshed_>=500){widgets_=unreal_.menu_widgets();refreshed_=now;object_count_=*count;}
 std::optional<MenuSnapshot> modal,normal,main,start;int modals=0,menus=0,mains=0,titles=0;
 static const std::wstring window_family=L"WD_WindowBase_C",base_family=L"WD_MenuBase_C",main_family=L"WD_MainMenu_C",title_family=L"WD_Title_C";
 for(const auto& o:widgets_){
  if(!unreal_.alive(o))continue;
  const auto candidate=[&](const std::wstring& family){return unreal_.discovered_class_match(o,family).value_or(true);};
  if(candidate(window_family)&&unreal_.is_a(o.address,L"WD_WindowBase_C")&&unreal_.shown(o.address,visibility_)&&unreal_.boolean(o.address,L"PopupActive")==std::optional<bool>(true)){
   ++modals;if(auto s=dialog(o))modal=s;continue;
  }
  if(candidate(base_family)){if(auto s=movie_pause(o)){normal=s;++menus;continue;}if(auto s=menu(o)){normal=s;++menus;continue;}}
  if(candidate(main_family))if(auto s=main_menu(o)){main=s;++mains;continue;}
  if(candidate(title_family))if(auto s=title(o)){start=s;++titles;}
 }
 if(modals){if(modals==1&&modal)return *modal;MenuSnapshot pending;pending.modal=true;pending.blocked=true;return pending;}
 const auto commands=story_commands(unreal_,visibility_);
 if(commands.valid||commands.blocked)return commands;
 if(const auto world=unreal_.current_world()){
  const auto online=online_menus(unreal_,*world,visibility_);
  if(online.valid||online.blocked)return online;
 }
 if(menus)return menus==1?*normal:MenuSnapshot{};if(mains==1)return *main;if(titles==1)return *start;return {};
}
std::wstring normalize_text(const std::wstring& text){
 std::wstring out;bool space=false;for(wchar_t c:text){if(std::iswspace(c)||c<32){space=!out.empty();continue;}if(space){out.push_back(L' ');space=false;}out.push_back(c);}return out;
}
}
