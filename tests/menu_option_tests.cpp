#include "collection_fixture.hpp"
#include <iostream>

using namespace mvci;
namespace {
int checks = 0;
#define CHECK(x) do { ++checks; if (!(x)) throw std::runtime_error(std::string(__func__) + ":" + std::to_string(__LINE__) + " " #x); } while (false)

struct OptionFixture : mvci_test::Fixture {
 Address menu_class = 0, base_item_class = 0, label_class = 0, menu = 0;

 OptionFixture() {
  auto base_menu = cls[L"WD_MenuBase_C"];
  prop(base_menu, L"IsMenuBaseInput", L"BoolProperty", 1);
  prop(base_menu, L"IsPopupActive", L"BoolProperty", 1);
  prop(base_menu, L"ItemIndex", L"IntProperty", 4);
  arr(base_menu, L"Items", L"ObjectProperty", 8);
  menu_class = klass(L"WD_PauseMenu_Training_C", klass(L"WD_PauseMenu_Base_C", base_menu));
  prop(menu_class, L"WD_PauseCommon", L"ObjectProperty", 8);
  menu = user(menu_class, L"TrainingPause", true);
  tick(menu);
  flag(menu, L"IsMenuBaseInput", true);

  base_item_class = klass(L"WD_OptionItemBase_C", cls[L"WD_UIParent_C"]);
  prop(base_item_class, L"ItemName", L"TextProperty", 24);
  prop(base_item_class, L"SubItemIndex", L"IntProperty", 4);
  arr(base_item_class, L"SubItemNames", L"TextProperty", 24);
  klass(L"WD_OptionItem_Simple_C", base_item_class);
  label_class = klass(L"WD_OptionItem_Base_C", cls[L"UserWidget"]);
  prop(label_class, L"ItemLabel", L"ObjectProperty", 8);
  klass(L"Image", cls[L"Widget"]);
 }

 Address row(const wchar_t* type, int label_number, const wchar_t* stored, const wchar_t* drawn) {
  auto row_class = klass(type, cls[L"WD_OptionItem_Simple_C"]);
  numbered_field(row_class, L"WD_OptionItem_Base", label_number);
  auto item = user(row_class, L"Row", false);
  slot(item, menu);
  items<Address>(menu, L"Items", {item});
  field_text(item, L"ItemName", stored);
  const auto field = L"WD_OptionItem_Base_" + std::to_wstring(label_number);
  auto title = nested(item, field, label_class, L"RowTitle");
  block(title, L"ItemLabel", drawn);
  return item;
 }

 void numbered_field(Address c, const wchar_t* field, int number) {
  put<int>(prop(c, field, L"ObjectProperty", 8) + 0x1c, number + 1);
 }

 Address image(Address item, const wchar_t* field, bool visible = true) {
  auto image = object(field, cls[L"Image"]);
  slot(image, item);
  set(item, field, image);
  show(image, visible);
  return image;
 }

 void show(Address widget, bool visible) {
  set<std::uint8_t>(widget, L"Visibility", visible ? 0 : 2);
 }
};

void recording_slot_reads_the_rendered_duration_and_selection() {
 OptionFixture f;
 const auto row = f.row(L"WD_OptionItem_RadioBtn_C", 71, L"Slot 2", L"Slot 2 00:04.25");
 const auto row_class = f.cls[L"WD_OptionItem_RadioBtn_C"];
 for (const auto field : {L"Element", L"Image_Radio"}) f.prop(row_class, field, L"ObjectProperty", 8);
 f.block(row, L"Element", L"Record");
 const auto marker = f.image(row, L"Image_Radio");
 MenuCapture capture(f.unreal);
 auto result = capture.sample(1000);
 CHECK(result.valid && result.label == L"Slot 2 00:04.25");
 CHECK(result.value == L"Record. Selected");
 f.show(marker, false);
 CHECK(capture.sample(1001).value == L"Record");
}

void spinner_reads_the_drawn_element_and_omits_it_when_hidden() {
 OptionFixture f;
 const auto row = f.row(L"WD_OptionItem_SpinText_C", 71, L"Guard", L"Guard");
 f.prop(f.cls[L"WD_OptionItem_SpinText_C"], L"Element", L"ObjectProperty", 8);
 const auto value = f.block(row, L"Element", L"After First Hit");
 f.texts(row, L"SubItemNames", {L"Stale stored choice"});
 MenuCapture capture(f.unreal);
 auto result = capture.sample(1000);
 CHECK(result.valid && result.value == L"After First Hit");
 f.show(value, false);
 result = capture.sample(1001);
 CHECK(result.valid && result.label == L"Guard" && result.value.empty());
}

void checkbox_follows_the_visible_marker() {
 OptionFixture f;
 const auto row = f.row(L"WD_OptionItem_Checkbox_C", 140, L"Attack Data", L"Attack Data");
 for (const auto field : {L"Check1", L"Check2", L"Element1", L"Element2"}) {
  f.prop(f.cls[L"WD_OptionItem_Checkbox_C"], field, L"ObjectProperty", 8);
 }
 const auto first = f.image(row, L"Check1", false);
 const auto second = f.image(row, L"Check2");
 f.block(row, L"Element1", L"Off");
 f.block(row, L"Element2", L"On");
 f.texts(row, L"SubItemNames", {L"Off", L"On"});
 MenuCapture capture(f.unreal);
 auto result = capture.sample(1000);
 CHECK(result.valid && result.value == L"On");
 f.show(first, true);
 result = capture.sample(1001);
 CHECK(result.value.empty());
 f.show(second, false);
 CHECK(capture.sample(1002).value == L"Off");
}

void slider_reads_the_drawn_fill_at_mouse_and_keyboard_positions() {
 OptionFixture f;
 const auto row = f.row(L"WD_OptionItem_Slider_C", 88, L"Volume", L"Volume");
 for (const auto field : {L"Image_Slider", L"Image_SliderLine"}) {
  f.prop(f.cls[L"WD_OptionItem_Slider_C"], field, L"ObjectProperty", 8);
 }
 const auto vector = f.object(L"Vector2D", f.kind(L"ScriptStruct"), 0x100);
 f.prop_at(vector, L"X", L"FloatProperty", 0, 4);
 f.prop_at(vector, L"Y", L"FloatProperty", 4, 4);
 const auto transform = f.object(L"WidgetTransform", f.kind(L"ScriptStruct"), 0x100);
 f.put(f.prop_at(transform, L"Scale", L"StructProperty", 8, 8) + 0x78, vector);
 f.put(f.prop_at(f.cls[L"Widget"], L"RenderTransform", L"StructProperty", 0x60, 28) + 0x78, transform);
 const auto thumb = f.image(row, L"Image_Slider");
 const auto fill = f.image(row, L"Image_SliderLine");
 f.put<float>(fill + 0x68, .42f);
 f.put<float>(fill + 0x6c, 1.f);
 MenuCapture capture(f.unreal);
 CHECK(capture.sample(1000).value == L"42 percent");
 f.put<float>(fill + 0x68, 0.f);
 CHECK(capture.sample(1001).value == L"0 percent");
 f.show(thumb, false);
 CHECK(capture.sample(1002).value.empty());
}

void pause_reads_only_the_owned_caption_and_numbered_guide() {
 OptionFixture f;
 const auto row = f.user(f.base_item_class, L"Resume", false);
 f.slot(row, f.menu);
 f.items<Address>(f.menu, L"Items", {row});
 f.field_text(row, L"ItemName", L"Resume Training");
 const auto common_class = f.klass(L"WD_PauseCommon_C", f.cls[L"WD_UIParent_C"]);
 f.prop(common_class, L"TextBlock_Caption", L"ObjectProperty", 8);
 f.numbered_field(common_class, L"WD_BtnNavi", 88);
 const auto common = f.nested(f.menu, L"WD_PauseCommon", common_class, L"PauseCommon");
 f.block(common, L"TextBlock_Caption", L"Training Menu");
 const auto guide_class = f.klass(L"WD_BtnNavi_C", f.cls[L"UserWidget"]);
 f.arr(guide_class, L"TextArray", L"StrProperty", 16);
 const auto guide = f.nested(common, L"WD_BtnNavi_88", guide_class, L"Guide");
 const auto array = f.alloc(16);
 f.str(array, L"<_Decide>Confirm choice");
 const auto field = guide + static_cast<Address>(f.off(guide, L"TextArray"));
 f.put(field, array);
 f.put<int>(field + 8, 1);
 f.put<int>(field + 12, 1);
 MenuCapture capture(f.unreal);
 auto result = capture.sample(1000);
 CHECK(result.valid && result.screen == L"Training Menu");
 CHECK(result.help == L"Confirm: Confirm choice");
 const auto unrelated = f.user(f.cls[L"UserWidget"], L"Other", true);
 f.slot(guide, unrelated);
 result = capture.sample(1001);
 CHECK(result.valid && result.screen == L"Training Menu" && result.help.empty());
 f.show(common, false);
 CHECK(capture.sample(1002).screen.empty());
}

void replaced_option_label_cannot_fall_back_to_stored_text() {
 OptionFixture f;
 const auto row = f.row(L"WD_OptionItem_PressBtn_C", 71, L"Retained old choice", L"Resume Training");
 f.prop(f.cls[L"WD_OptionItem_PressBtn_C"], L"Element", L"ObjectProperty", 8);
 f.show(f.block(row, L"Element", L"Inactive cue"), false);
 const auto title = *f.unreal.object_field(row, L"WD_OptionItem_Base_71");
 const auto label = *f.unreal.object_field(title, L"ItemLabel");
 const auto reader = f.memory.reader;
 const auto old_text = f.raw<Address>(label + 0x140);
 f.field_text(label, L"Text", L"A replacement line");
 const auto new_text = f.raw<Address>(label + 0x140);
 f.put(label + 0x140, old_text);
 f.memory.reader = [&](Address address, void* output, std::size_t size) {
  const bool read = reader(address, output, size);
  if (address == label + 0x140 && size == sizeof(Address)) f.put(label + 0x140, new_text);
  return read;
 };
 MenuCapture capture(f.unreal);
 const auto result = capture.sample(1000);
 CHECK(!result.valid && result.blocked);
 CHECK(capture.sample(1001).label == L"A replacement line");
}
}

int main() {
 int failed = 0;
 for (const auto test : {recording_slot_reads_the_rendered_duration_and_selection,
                         spinner_reads_the_drawn_element_and_omits_it_when_hidden,
                         checkbox_follows_the_visible_marker,
                         slider_reads_the_drawn_fill_at_mouse_and_keyboard_positions,
                         pause_reads_only_the_owned_caption_and_numbered_guide,
                         replaced_option_label_cannot_fall_back_to_stored_text}) {
  try { test(); } catch (const std::exception& e) { std::cerr << e.what() << '\n'; ++failed; }
 }
 std::cout << checks << " menu option checks, " << failed << " failed groups\n";
 return failed ? 1 : 0;
}
