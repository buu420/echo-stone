#include "collection_fixture.hpp"
#include <iostream>

namespace mvci {
StoryTextSnapshot fight_announcements(Unreal&, Address, const std::function<std::optional<int>(Address)>&);
}

using namespace mvci;
namespace {
int checks = 0;
#define CHECK(x) do { ++checks; if (!(x)) throw std::runtime_error(std::string(__func__) + ":" + std::to_string(__LINE__) + " " #x); } while (false)

struct FightFixture : mvci_test::Fixture {
 Address flow_system = 0, flow = 0, result = 0, announce = 0;

 FightFixture() {
  blocks[base + 0x348cb40].resize(8);
  flow_system = object(L"GameFlow", klass(L"GameFlowSystem"));
  put(base + 0x348cb40, flow_system);
  const auto vector = object(L"Vector2D", kind(L"ScriptStruct"), 0x100);
  prop_at(vector, L"X", L"FloatProperty", 0, 4);
  prop_at(vector, L"Y", L"FloatProperty", 4, 4);
  const auto transform = object(L"WidgetTransform", kind(L"ScriptStruct"), 0x100);
  put(prop_at(transform, L"Translation", L"StructProperty", 0, 8) + 0x78, vector);
  put(prop_at(transform, L"Scale", L"StructProperty", 8, 8) + 0x78, vector);
  put(prop_at(cls[L"Widget"], L"RenderTransform", L"StructProperty", 0x60, 28) + 0x78, transform);

  const auto image_class = klass(L"Image", cls[L"Widget"]);
  const auto brush = object(L"SlateBrush", kind(L"ScriptStruct"), 0x100);
  prop_at(brush, L"ResourceObject", L"ObjectProperty", 8, 8);
  put(prop_at(image_class, L"Brush", L"StructProperty", 0x140, 32) + 0x78, brush);
  prop_at(image_class, L"ColorAndOpacity", L"StructProperty", 0x180, 16);
  klass(L"MaterialInstanceConstant");
 }

 void fields(Address c, std::initializer_list<const wchar_t*> field_names) {
  for (const auto name : field_names) prop(c, name, L"ObjectProperty", 8);
 }

 void start(const wchar_t* type) {
  flow = actor(klass(type, cls[L"Actor"]), L"CurrentFlow");
  put(flow_system + 0xe8, flow);
 }

 void transform(Address widget, float x = 0.f, float y = 0.f) {
  put<float>(widget + 0x60, x);
  put<float>(widget + 0x64, y);
  put<float>(widget + 0x68, 1.f);
  put<float>(widget + 0x6c, 1.f);
 }

 void show(Address widget, bool visible) { set<std::uint8_t>(widget, L"Visibility", visible ? 0 : 2); }

 Address asset(const wchar_t* path, const wchar_t* name, Address type) {
  const auto package = object(std::wstring(path) + name, kind(L"Package"));
  return object(name, type, 0x2000, package);
 }

 Address image(Address owner, const wchar_t* field, const wchar_t* path, const wchar_t* name, bool visible = true) {
  const auto image = object(field, cls[L"Image"]);
  slot(image, owner);
  set(owner, field, image);
  put(image + 0x148, asset(path, name, cls[L"MaterialInstanceConstant"]));
  put<float>(image + 0x18c, 1.f);
  transform(image);
  show(image, visible);
  return image;
 }

 void start_result(bool clear = false) {
  start(clear ? L"GF_FightArcade_C" : L"GF_FightLocal_C");
  fields(raw<Address>(flow + 0x10), {L"Result_Obj"});
  const auto result_class = klass(L"BP_ResultCtrl_C", cls[L"BP_UIBase_C"]);
  fields(result_class, {L"WD_Announce"});
  arr(result_class, L"WDTotal_Arrobj", L"ObjectProperty", 8);
  result = actor(clear ? klass(L"BP_ResultCtrlArcadeClear_C", result_class) : result_class, L"Result");
  tick(result);
  set(flow, L"Result_Obj", result);
 }

 void start_announce(bool clear = false) {
  start_result(clear);
  const auto announce_class = klass(L"WD_Result_Announce_New_C", cls[L"WD_UIParent_C"]);
  fields(announce_class, {L"Winner_1P", L"Winner_2P", L"CONGRATULATIONS", L"TextMainName1P", L"TextSubName1P",
                          L"TextMainName2P", L"TextSubName2P"});
  announce = user(announce_class, L"Announcement", true);
  set(result, L"WD_Announce", announce);
  constexpr auto path = L"/Game/UI/Battle/Result/Parts/";
  image(announce, L"Winner_1P", path, L"MT_Wintext_1P", false);
  image(announce, L"Winner_2P", path, L"MT_Wintext_2P", !clear);
  image(announce, L"CONGRATULATIONS", path, L"MT_CongText", clear);
  transform(block(announce, L"TextMainName2P", L"Morrigan"));
  transform(block(announce, L"TextSubName2P", L"Spider-Man"));
 }

 StoryTextSnapshot sample() { return fight_announcements(unreal, world, {}); }
};

void arcade_stage_comes_from_the_current_shown_versus_text() {
 FightFixture f;
 f.start(L"GF_VersusArcade_C");
 f.fields(f.cls[L"GF_VersusArcade_C"], {L"VersusObj"});
 const auto versus_class = f.klass(L"BP_VersusArcade_C", f.cls[L"BP_UIBase_C"]);
 f.fields(versus_class, {L"ArcadeText_Obj"});
 f.prop(versus_class, L"InputState", L"ByteProperty", 1);
 const auto versus = f.actor(versus_class, L"Versus");
 f.set(f.flow, L"VersusObj", versus);
 f.set<std::uint8_t>(versus, L"InputState", 1);
 const auto caption_class = f.klass(L"WD_ArcadeText_C", f.cls[L"WD_UIParent_C"]);
 f.fields(caption_class, {L"TextBlock_ArcadeStage"});
 const auto caption = f.user(caption_class, L"ArcadeStage", true);
 f.tick(caption, 1);
 f.set(versus, L"ArcadeText_Obj", caption);
 const auto text = f.block(caption, L"TextBlock_ArcadeStage", L"Stage 4");
 CHECK(f.sample().text == L"Stage 4");
 f.show(text, false);
 CHECK(!f.sample().valid);
 f.show(text, true);
 f.set<std::uint8_t>(versus, L"InputState", 3);
 CHECK(!f.sample().valid);
 f.set<std::uint8_t>(versus, L"InputState", 1);
 f.put(f.flow_system + 0xe8, f.actor(f.klass(L"GF_FightOnlineVersus_C", f.cls[L"Actor"]), L"ReplacementFlow"));
 CHECK(!f.sample().valid);
}

void winner_names_follow_the_settled_winner_graphic() {
 FightFixture f;
 f.start_announce();
 auto shown = f.sample();
 CHECK(shown.valid && shown.text == L"Winner. Morrigan. Spider-Man");
 const auto winner = *f.unreal.object_field(f.announce, L"Winner_2P");
 f.transform(winner, 1000.f);
 CHECK(!f.sample().valid);
 f.transform(winner);
 const auto other = *f.unreal.object_field(f.announce, L"Winner_1P");
 f.show(other, true);
 CHECK(!f.sample().valid);
 f.show(other, false);
 const auto sub = *f.unreal.object_field(f.announce, L"TextSubName2P");
 f.show(sub, false);
 CHECK(!f.sample().valid);
}

void arcade_clear_reads_the_team_alongside_the_native_congratulations() {
 FightFixture f;
 f.start_announce(true);
 CHECK(f.sample().text == L"Arcade team. Morrigan. Spider-Man");
 const auto art = *f.unreal.object_field(f.announce, L"CONGRATULATIONS");
 f.put(art + 0x148, f.asset(L"/Game/AnotherScreen/", L"MT_CongText", f.cls[L"MaterialInstanceConstant"]));
 CHECK(!f.sample().valid);
}

void summary_reads_the_visible_player_and_rendered_counts() {
 FightFixture f;
 f.start_result();
 const auto type = f.klass(L"WD_Total_C", f.cls[L"WD_UIParent_C"]);
 f.prop(type, L"TeamNo", L"IntProperty", 4);
 f.prop(type, L"ResultType", L"ByteProperty", 1);
 f.fields(type, {L"CanvasPanel_Total", L"TextBlock_Player1", L"TextBlock_Player2", L"Image_Result_Text",
                 L"Text_Win", L"Text_WinNum", L"Text_Lose", L"Text_LoseNum", L"Text_Draw", L"Text_DrawNum"});
 const auto panel = f.user(type, L"Totals", true);
 const auto layout = f.panel(panel, L"CanvasPanel_Total");
 f.transform(layout, 200.f, 150.f);
 const auto bind = [&](Address widget) {
  f.put(f.raw<Address>(widget + 0x30) + 0x28, layout);
  return widget;
 };
 bind(f.block(panel, L"TextBlock_Player1", L"Player 1"));
 f.show(bind(f.block(panel, L"TextBlock_Player2", L"Retained other player")), false);
 bind(f.image(panel, L"Image_Result_Text", L"/Game/UI/Battle/Result/Parts/Material/", L"MT_Inst_Draw"));
 f.set<std::uint8_t>(panel, L"ResultType", 2);
 bind(f.block(panel, L"Text_Win", L"Wins"));
 const auto wins = bind(f.block(panel, L"Text_WinNum", L"7"));
 bind(f.block(panel, L"Text_Lose", L"Losses"));
 bind(f.block(panel, L"Text_LoseNum", L"3"));
 bind(f.block(panel, L"Text_Draw", L"Draws"));
 bind(f.block(panel, L"Text_DrawNum", L"1"));
 const auto hidden = f.user(type, L"DetachedOtherTotal", false);
 f.set<int>(hidden, L"TeamNo", 1);
 f.items<Address>(f.result, L"WDTotal_Arrobj", {panel, hidden});
 CHECK(f.sample().text == L"Results. Player 1. Draw. Wins 7. Losses 3. Draws 1");
 f.show(wins, false);
 CHECK(!f.sample().valid);
 f.show(wins, true);
 f.transform(layout, 968.f, 150.f);
 CHECK(!f.sample().valid);
}

void mission_clear_is_left_to_the_native_announcement() {
 FightFixture f;
 f.start(L"GF_FightMission_C");
 f.fields(f.cls[L"GF_FightMission_C"], {L"HUD_Root"});
 const auto hud_class = f.klass(L"BP_HUD_Root_C", f.cls[L"BP_UIBase_C"]);
 f.fields(hud_class, {L"AnnounceCtrl"});
 const auto hud = f.actor(hud_class, L"Hud");
 f.set(f.flow, L"HUD_Root", hud);
 const auto ctrl_class = f.klass(L"BP_AnnounceCtrl_C", f.cls[L"BP_UIBase_C"]);
 f.fields(ctrl_class, {L"BP_Clear"});
 f.arr(ctrl_class, L"ArrAnnounce", L"ObjectProperty", 8);
 const auto ctrl = f.actor(ctrl_class, L"AnnounceController");
 f.set(hud, L"AnnounceCtrl", ctrl);
 const auto clear_class = f.klass(L"BP_Clear_C", f.cls[L"BP_UIBase_C"]);
 f.fields(clear_class, {L"RootScene", L"Root", L"MS_ClearB", L"CDMIB"});
 const auto clear = f.actor(clear_class, L"Clear");
 f.tick(clear);
 f.attach(ctrl, L"BP_Clear", clear);
 auto announcements = std::vector<Address>(12);
 announcements[10] = clear;
 f.items(ctrl, L"ArrAnnounce", announcements);
 f.fields(f.cls[L"StaticMeshComponent"], {L"StaticMesh"});
 f.fields(f.cls[L"SceneComponent"], {L"AttachParent"});
 f.arr(f.cls[L"StaticMeshComponent"], L"OverrideMaterials", L"ObjectProperty", 8);
 const auto scene = f.comp(L"SceneComponent", L"RootScene", clear);
 f.set(clear, L"RootScene", scene);
 const auto root = f.comp(L"SceneComponent", L"Root", clear);
 f.set(clear, L"Root", root);
 f.set(root, L"AttachParent", scene);
 const auto mesh = f.comp(L"StaticMeshComponent", L"Mesh", clear);
 f.set(clear, L"MS_ClearB", mesh);
 f.set(mesh, L"AttachParent", root);
 constexpr auto path = L"/Game/UI/Battle/Announce/Clear/Parts/";
 f.set(mesh, L"StaticMesh", f.asset(path, L"MS_Clear", f.klass(L"StaticMesh")));
 const auto material = f.klass(L"MaterialInstanceDynamic");
 f.fields(material, {L"Parent"});
 const auto instance = f.object(L"ClearMaterial", material);
 f.set(instance, L"Parent", f.asset(path, L"MT_Clear", f.klass(L"Material")));
 f.set(clear, L"CDMIB", instance);
 f.items<Address>(mesh, L"OverrideMaterials", {instance});
 CHECK(!f.sample().valid);
}
}

int main() {
 int failed = 0;
 for (const auto test : {arcade_stage_comes_from_the_current_shown_versus_text,
                         winner_names_follow_the_settled_winner_graphic,
                         arcade_clear_reads_the_team_alongside_the_native_congratulations,
                         summary_reads_the_visible_player_and_rendered_counts, mission_clear_is_left_to_the_native_announcement}) {
  try { test(); } catch (const std::exception& e) { std::cerr << e.what() << '\n'; ++failed; }
 }
 std::cout << checks << " fight announcement checks, " << failed << " failed groups\n";
 return failed ? 1 : 0;
}
