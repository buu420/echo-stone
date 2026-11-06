#include "collection_fixture.hpp"
#include <iostream>

namespace mvci {
MenuSnapshot online_menus(Unreal&, Address, const std::function<std::optional<int>(Address)>&);
}

using namespace mvci;
namespace {
int checks = 0;
#define CHECK(x) do { ++checks; if (!(x)) throw std::runtime_error(std::string(__func__) + ":" + std::to_string(__LINE__) + " " #x); } while (false)

struct OnlineFixture : mvci_test::Fixture {
    Address vector{}, margin{}, anchor{}, image{}, vertical{};
    Address lobby_flow{}, lobby{}, ranking_flow{}, ranking{}, replay_flow{}, replay{};

    OnlineFixture() {
        image = klass(L"Image", cls[L"Widget"]);
        vertical = klass(L"VerticalBox", cls[L"Widget"]);
        vector = structure(L"Vector2D", {{L"X", 0}, {L"Y", 4}});
        margin = structure(L"Margin", {{L"Left", 0}, {L"Top", 4}});
        anchor = object(L"AnchorData", kind(L"ScriptStruct"), 0x100);
        put(prop_at(anchor, L"Offsets", L"StructProperty", 0, 16) + 0x78, margin);
        const auto slot_class = kind(L"CanvasPanelSlot");
        put<int>(slot_class + 0x40, 0x100);
        put(prop_at(slot_class, L"LayoutData", L"StructProperty", 0x40, 40) + 0x78, anchor);
        prop_at(slot_class, L"Content", L"ObjectProperty", 0x30, 8);
        klass(L"GF_FlowBase_C", cls[L"Actor"]);
    }

    Address structure(const wchar_t* name, std::initializer_list<std::pair<const wchar_t*, int>> members) {
        auto s = object(name, kind(L"ScriptStruct"), 0x100);
        for (const auto& [field, offset] : members) prop_at(s, field, L"FloatProperty", offset, 4);
        return s;
    }

    Address menu(const wchar_t* type, Address parent = 0) {
        return klass(type, parent ? parent : cls[L"WD_MenuBase_C"]);
    }

    void fields(Address c, std::initializer_list<const wchar_t*> members) {
        for (const auto* name : members) prop(c, name, L"ObjectProperty", 8);
    }

    void ints(Address c, std::initializer_list<const wchar_t*> members) {
        for (const auto* name : members) prop(c, name, L"IntProperty", 4);
    }

    void bools(Address c, std::initializer_list<const wchar_t*> members) {
        for (const auto* name : members) prop(c, name, L"BoolProperty", 1);
    }

    void position_fields(Address c, std::initializer_list<const wchar_t*> members) {
        for (const auto* name : members) put(prop(c, name, L"StructProperty", 8) + 0x78, vector);
    }

    struct Position { float x; float y; };
    void place(Address owner, Address widget, const wchar_t* destination, float x, float y) {
        const auto s = raw<Address>(widget + 0x30);
        set(s, L"Content", widget);
        set<Position>(s, L"LayoutData", {x, y});
        set<Position>(owner, destination, {x, y});
    }

    Address child(Address owner, const wchar_t* name, Address c) {
        const auto w = object(name, c);
        slot(w, owner);
        set(raw<Address>(w + 0x30), L"Content", w);
        set(owner, name, w);
        return w;
    }

    Address block(Address owner, const wchar_t* name, const wchar_t* value) {
        const auto w = mvci_test::Fixture::block(owner, name, value);
        set(raw<Address>(w + 0x30), L"Content", w);
        return w;
    }

    Address list_row(Address owner, Address list, Address row_class, const wchar_t* name) {
        const auto row = user(row_class, name, false);
        slot(row, owner);
        const auto s = raw<Address>(row + 0x30);
        put(s + 0x28, list);
        set(s, L"Content", row);
        return row;
    }

    Address flow(const wchar_t* type, const wchar_t* field, const wchar_t* state = nullptr) {
        const auto c = klass(type, cls[L"GF_FlowBase_C"]);
        fields(c, {field});
        if (state) prop(c, state, L"ByteProperty", 1);
        const auto w = actor(c, type);
        if (state) set<std::uint8_t>(w, state, 1);
        return w;
    }

    void hide(Address w) { put<std::uint8_t>(w + 0x91, 2); }
    void show(Address w) { put<std::uint8_t>(w + 0x91, 0); }

    Address setup_lobby() {
        lobby_flow = flow(L"GF_Lobby_C", L"LobbySearchResult", L"LobbyFlowState");
        const auto c = menu(L"WD_LobbySearchResult_C");
        fields(c, {L"TextBlock_Caption", L"Image_Cursor", L"VerticalBox_List", L"WD_PopupMembers"});
        ints(c, {L"FocusIndex", L"IndexCursor", L"ViewRangeTop", L"ListShowMax", L"ListLastIndex", L"LobbySearchResultState"});
        position_fields(c, {L"DestinationCursorPos", L"DestinationScrollPos"});
        arr(c, L"ListItemArray", L"ObjectProperty", 8);
        lobby = user(c, L"Lobby results", true);
        tick(lobby);
        set(lobby_flow, L"LobbySearchResult", lobby);
        block(lobby, L"TextBlock_Caption", L"Search Results");
        const auto list = child(lobby, L"VerticalBox_List", vertical);
        const auto cursor = child(lobby, L"Image_Cursor", image);
        place(lobby, cursor, L"DestinationCursorPos", 150.f, 200.f);
        place(lobby, list, L"DestinationScrollPos", 100.f, 200.f);
        set<int>(lobby, L"ListShowMax", 6);
        const auto row_class = menu(L"WD_LobbyListItem_C", cls[L"UserWidget"]);
        fields(row_class, {L"TextBlock_PlayerID", L"TextBlock_LobbyTitle", L"TextBlock_LobbySize", L"TextBlock_Region"});
        const auto first = list_row(lobby, list, row_class, L"First result");
        const auto second = list_row(lobby, list, row_class, L"Second result");
        block(first, L"TextBlock_PlayerID", L"First Host");
        block(second, L"TextBlock_PlayerID", L"Second Host");
        block(second, L"TextBlock_LobbyTitle", L"Friendly");
        block(second, L"TextBlock_LobbySize", L"2/4");
        hide(block(second, L"TextBlock_Region", L"Hidden region"));
        items<Address>(lobby, L"ListItemArray", {first, second});
        set<int>(lobby, L"ListLastIndex", 1);
        set<int>(lobby, L"FocusIndex", 1);
        set<int>(lobby, L"IndexCursor", 1);
        return second;
    }

    Address setup_ranking() {
        ranking_flow = flow(L"GF_Ranking_C", L"Rank");
        const auto c = menu(L"WD_RankingMain_C");
        fields(c, {L"CanvasPanel_List", L"CanvasPanel_Cursor", L"TextBlock_BoardTitle", L"TextBlock_Column2", L"Dialog_GetRankingData", L"ProfileCard"});
        ints(c, {L"IndexFocus", L"ViewTopItem", L"ScrollingNum", L"ListItemMax"});
        bools(c, {L"enterFinished", L"isShowProfileCard"});
        arr(c, L"ListItemArray", L"ObjectProperty", 8);
        ranking = user(c, L"Ranking", true);
        tick(ranking);
        set(ranking_flow, L"Rank", ranking);
        flag(ranking, L"enterFinished", true);
        set<int>(ranking, L"ViewTopItem", 13);
        set<int>(ranking, L"ListItemMax", 1);
        block(ranking, L"TextBlock_BoardTitle", L"Ranking");
        block(ranking, L"TextBlock_Column2", L"Wins");
        const auto list = child(ranking, L"CanvasPanel_List", cls[L"CanvasPanel"]);
        child(ranking, L"CanvasPanel_Cursor", cls[L"CanvasPanel"]);
        const auto row_class = menu(L"WD_RankList_C", cls[L"UserWidget"]);
        fields(row_class, {L"CanvasPanel_Body", L"CanvasPanel_NoDate", L"TextBlock_RankNumber", L"TextBlock_PlayerID", L"TextBlock_Points", L"TextBlock_Suffix"});
        const auto row = list_row(ranking, list, row_class, L"Ranked row");
        const auto body = child(row, L"CanvasPanel_Body", cls[L"CanvasPanel"]);
        hide(child(row, L"CanvasPanel_NoDate", cls[L"CanvasPanel"]));
        for (const auto& [name, value] : {std::pair{L"TextBlock_RankNumber", L"14"}, {L"TextBlock_PlayerID", L"Fighter 14"}, {L"TextBlock_Points", L"35"}}) {
            const auto b = block(row, name, value);
            put(raw<Address>(b + 0x30) + 0x28, body);
        }
        items<Address>(ranking, L"ListItemArray", {row});
        return row;
    }

    Address setup_replay() {
        replay_flow = flow(L"GF_Replay_C", L"ReplayMain", L"State");
        const auto c = menu(L"WD_ReplayMain_C");
        fields(c, {L"CanvasPanel_Cursor", L"VerticalBox_List", L"WD_ReplayListSmall_Preset2", L"TextBlock_Label", L"TextBlock_Label_Auto",
                   L"TextBlock_Label_Favorite", L"SaveReplayDialog", L"GetReplayDialog", L"DeleteDialog"});
        ints(c, {L"FocusIndex", L"CursorIndex", L"ScrollValue", L"ListShowMax"});
        bools(c, {L"IsSpawnDialog"});
        position_fields(c, {L"DestinationCursorPos", L"DestinationScrollPos"});
        arr(c, L"ServerListWg", L"ObjectProperty", 8);
        arr(c, L"RocalListWg", L"ObjectProperty", 8);
        replay = user(c, L"Replay", true);
        tick(replay);
        set(replay_flow, L"ReplayMain", replay);
        const auto list = child(replay, L"VerticalBox_List", vertical);
        const auto cursor = child(replay, L"CanvasPanel_Cursor", cls[L"CanvasPanel"]);
        place(replay, cursor, L"DestinationCursorPos", 160.f, 300.f);
        place(replay, list, L"DestinationScrollPos", 140.f, 300.f);
        set<int>(replay, L"ListShowMax", 3);
        const auto row_class = menu(L"WD_ReplayListSmall_C", cls[L"UserWidget"]);
        fields(row_class, {L"TextBlock_Date", L"TextBlock_UID1", L"TextBlock_UID2", L"TextBlock_MatchType", L"TextBlock_Result1P", L"TextBlock_Result2P", L"Image_Favorite"});
        const auto local = list_row(replay, list, row_class, L"Local replay");
        block(local, L"TextBlock_Date", L"11/05/2026");
        block(local, L"TextBlock_UID1", L"One");
        block(local, L"TextBlock_UID2", L"Two");
        block(local, L"TextBlock_MatchType", L"CASUAL MATCH");
        block(local, L"TextBlock_Result1P", L"WINNER");
        hide(block(local, L"TextBlock_Result2P", L"Hidden result"));
        hide(child(local, L"Image_Favorite", image));
        items<Address>(replay, L"RocalListWg", {local});
        const auto server = nested(replay, L"WD_ReplayListSmall_Preset2", row_class, L"Favorite replay");
        block(server, L"TextBlock_Date", L"--/--/----");
        block(replay, L"TextBlock_Label_Favorite", L"Favorite");
        items<Address>(replay, L"ServerListWg", {0, 0, server, 0, 0});
        set<int>(replay, L"FocusIndex", 5);
        return local;
    }

    void setup_lobby_room() {
        lobby_flow = flow(L"GF_Lobby_C", L"LobbyMain", L"LobbyFlowState");
        set<std::uint8_t>(lobby_flow, L"LobbyFlowState", 0);
        const auto c = menu(L"WD_LobbyMain_C");
        fields(c, {L"Image_Cursor", L"WD_RoomFocus", L"ActivePopup", L"WD_Room02", L"WD_PopupMembers", L"WD_PopupRoomType",
                   L"WD_PopupLobbyRule",
                   L"TextBlock_Title", L"TextBlock_HostID", L"TextBlock_UniqueID"});
        ints(c, {L"InLobbyState", L"CursorMoveX"});
        bools(c, {L"IsLobbyDialogActive"});
        position_fields(c, {L"DestinationCursorPos"});
        arr(c, L"Rooms", L"ObjectProperty", 8);
        lobby = user(c, L"Lobby main", true);
        tick(lobby);
        set(lobby_flow, L"LobbyMain", lobby);
        const auto cursor = child(lobby, L"Image_Cursor", image);
        place(lobby, cursor, L"DestinationCursorPos", 500.f, 200.f);
        child(lobby, L"WD_RoomFocus", menu(L"WD_RoomFocus_C", cls[L"UserWidget"]));
        block(lobby, L"TextBlock_Title", L"Friendly room");
        block(lobby, L"TextBlock_HostID", L"Lobby host");
        const auto room_class = menu(L"WD_Room_C", cls[L"UserWidget"]);
        fields(room_class, {L"TextBlock_TypeNo", L"TextBlock_1PID", L"TextBlock_2PID", L"TextBlock_inBattle"});
        const auto room = nested(lobby, L"WD_Room02", room_class, L"Room");
        block(room, L"TextBlock_TypeNo", L"Battle 02");
        block(room, L"TextBlock_1PID", L"First combatant");
        hide(block(room, L"TextBlock_2PID", L"Retained second combatant"));
        items<Address>(lobby, L"Rooms", {0, room, 0, 0});
        set<int>(lobby, L"CursorMoveX", 1);
    }

    Address setup_members(Address parent) {
        const auto c = menu(L"WD_PopupMembers_C");
        fields(c, {L"CanvasPanel_Cursor", L"TextBlock_Caption", L"ProfCardReference", L"WD_SpinTextVoiceChat",
                   L"WD_SpinTextMemberDisp"});
        ints(c, {L"FocusIndex"});
        bools(c, {L"IsProfCardActive"});
        arr(c, L"WDListButtons", L"ObjectProperty", 8);
        const auto popup = nested(parent, L"WD_PopupMembers", c, L"Members");
        tick(popup);
        const auto button_class = menu(L"WD_MemberListBtn_C", cls[L"UserWidget"]);
        fields(button_class, {L"TextBlock_M0"});
        const auto row = user(button_class, L"Member", false);
        slot(row, popup);
        block(row, L"TextBlock_M0", L"Chosen member");
        child(popup, L"CanvasPanel_Cursor", cls[L"CanvasPanel"]);
        block(popup, L"TextBlock_Caption", L"Lobby Members");
        items<Address>(popup, L"WDListButtons", {row});
        return popup;
    }

    Address setup_profile() {
        const auto spawn_class = menu(L"WD_ProfCardSpawn_C");
        fields(spawn_class, {L"CardMain", L"ErrDialog", L"LoginWaitDialog", L"SaveDialog"});
        const auto spawn = user(spawn_class, L"Card spawn", true);
        tick(spawn);
        set(ranking, L"ProfileCard", spawn);
        flag(ranking, L"isShowProfileCard", true);
        const auto c = menu(L"WD_ProfCardMain_C");
        fields(c, {L"CanvasPanel_Top", L"CanvasPanel_inPage", L"WD_PFcardBase"});
        bools(c, {L"isCharEdit", L"isAqTitleSelect", L"isExit"});
        ints(c, {L"IndexCurrentPage"});
        arr(c, L"PageWidgetArray", L"ObjectProperty", 8);
        const auto main = user(c, L"Card main", true);
        tick(main);
        set(spawn, L"CardMain", main);
        const auto base_class = menu(L"WD_PFcardBase_C", cls[L"UserWidget"]);
        fields(base_class, {L"TextBlock_Caption"});
        const auto base_widget = nested(main, L"WD_PFcardBase", base_class, L"Profile headings");
        block(base_widget, L"TextBlock_Caption", L"Profile Card");
        const auto top_panel = child(main, L"CanvasPanel_Top", cls[L"CanvasPanel"]);
        hide(child(main, L"CanvasPanel_inPage", cls[L"CanvasPanel"]));
        const auto page_class = menu(L"WD_PFcardTop_C", cls[L"UserWidget"]);
        fields(page_class, {L"TextBlock_UserID", L"TextBlock_AcquiredTitle", L"TextBlock_WinP", L"TextBlock_LossP",
                            L"CanvasPanel_CharcterEdit", L"CanvasPanel_Cursor", L"Image_CursorTop",
                            L"TextBlock_TargetType", L"WD_CEspinbox_Item1"});
        bools(page_class, {L"isCharEdit"});
        ints(page_class, {L"FocusIndex", L"SettingListMax"});
        position_fields(page_class, {L"CursorDefaultPos_List"});
        prop(page_class, L"CursorListStep", L"FloatProperty", 4);
        arr(page_class, L"SettingItemArray", L"ObjectProperty", 8);
        const auto page = list_row(main, top_panel, page_class, L"Profile overview");
        block(page, L"TextBlock_UserID", L"Profile player");
        block(page, L"TextBlock_AcquiredTitle", L"Visible title");
        hide(block(page, L"TextBlock_WinP", L"Hidden wins"));
        block(page, L"TextBlock_LossP", L"5");
        items<Address>(main, L"PageWidgetArray", {page, 0, 0, 0, 0});
        return main;
    }

    Address setup_character_edit(Address main) {
        const auto array = unreal.array(main, *unreal.property(main, L"PageWidgetArray"), 5);
        const auto page = raw<Address>(array->data);
        flag(main, L"isCharEdit", true);
        flag(page, L"isCharEdit", true);
        const auto editor = child(page, L"CanvasPanel_CharcterEdit", cls[L"CanvasPanel"]);
        const auto cursor = child(page, L"CanvasPanel_Cursor", cls[L"CanvasPanel"]);
        place(page, cursor, L"CursorDefaultPos_List", 150.f, 100.f);
        set<Position>(raw<Address>(cursor + 0x30), L"LayoutData", {150.f, 140.f});
        set<float>(page, L"CursorListStep", 40.f);
        set<int>(page, L"SettingListMax", 7);
        set<int>(page, L"FocusIndex", 2);
        child(page, L"Image_CursorTop", image);
        block(page, L"TextBlock_TargetType", L"Character 1");
        const auto item_class = menu(L"WD_CEspinbox_C", cls[L"UserWidget"]);
        fields(item_class, {L"TextBlock_ListItem", L"TextBlock_Value"});
        const auto item = list_row(page, editor, item_class, L"Costume option");
        set(page, L"WD_CEspinbox_Item1", item);
        block(item, L"TextBlock_ListItem", L"Costume");
        block(item, L"TextBlock_Value", L"Default");
        items<Address>(page, L"SettingItemArray", {0, item, 0, 0, 0, 0, 0});
        return page;
    }

    Address setup_acquired_titles(Address main) {
        const auto page_class = menu(L"WD_PFcardAqSelect_C", cls[L"UserWidget"]);
        fields(page_class, {L"Image_ListCursor", L"VerticalBox_0", L"CanvasPanel_Popup", L"TextBlock_Condition"});
        bools(page_class, {L"isActive", L"isPopupActive"});
        ints(page_class, {L"indexFocus", L"indexCursorX", L"indexCursorY", L"ViewRangeTop", L"ListShowMax", L"ListShowMax_W"});
        arr(page_class, L"ListItemArray", L"ObjectProperty", 8);
        position_fields(page_class, {L"DestinationCursorPos", L"DestinationScrollPos"});
        const auto panel = field(main, L"CanvasPanel_inPage");
        show(panel);
        hide(field(main, L"CanvasPanel_Top"));
        const auto page = list_row(main, panel, page_class, L"Acquired titles page");
        items<Address>(main, L"PageWidgetArray", {0, 0, 0, 0, page});
        set<int>(main, L"IndexCurrentPage", 4);
        flag(main, L"isAqTitleSelect", true);
        flag(page, L"isActive", true);
        set<int>(page, L"indexFocus", 1);
        set<int>(page, L"indexCursorX", 1);
        set<int>(page, L"ListShowMax", 7);
        set<int>(page, L"ListShowMax_W", 5);
        const auto cursor = child(page, L"Image_ListCursor", image);
        place(page, cursor, L"DestinationCursorPos", 330.f, 200.f);
        const auto list = child(page, L"VerticalBox_0", vertical);
        place(page, list, L"DestinationScrollPos", 100.f, 200.f);
        const auto aq_class = menu(L"WD_AQPanel_C", cls[L"UserWidget"]);
        fields(aq_class, {L"TextBlock_AqTitle_0", L"TextBlock_AqTitle_1", L"Image_1"});
        arr(aq_class, L"TextBlockArray", L"ObjectProperty", 8);
        const auto row = list_row(page, list, aq_class, L"Titles row");
        const auto first = block(row, L"TextBlock_AqTitle_0", L"Other title");
        const auto second = block(row, L"TextBlock_AqTitle_1", L"Chosen title");
        child(row, L"Image_1", image);
        items<Address>(page, L"ListItemArray", {row});
        items<Address>(row, L"TextBlockArray", {first, second, 0, 0, 0});
        const auto popup = child(page, L"CanvasPanel_Popup", cls[L"CanvasPanel"]);
        const auto condition = block(page, L"TextBlock_Condition", L"Win five matches");
        put(raw<Address>(condition + 0x30) + 0x28, popup);
        hide(popup);
        return page;
    }

    Address setup_lobby_id() {
        const auto title = flow(L"GF_Title_C", L"WD_MainMenu");
        fields(cls[L"GF_Title_C"], {L"InputObject"});
        bools(cls[L"GF_Title_C"], {L"IsTravel"});
        const auto main_class = menu(L"WD_MainMenu_C");
        fields(main_class, {L"ActiveMenu"});
        const auto main = user(main_class, L"Main menu", true);
        set(title, L"WD_MainMenu", main);
        set(title, L"InputObject", main);

        const auto base_menu = cls[L"WD_MenuBase_C"];
        fields(base_menu, {L"CanvasPanel_Popup", L"ParentCanvas"});
        arr(base_menu, L"PopupItem", L"ObjectProperty", 8);
        const auto versus = user(menu(L"WD_MMVersus_C"), L"Battle menu", true);
        tick(versus);
        set(main, L"ActiveMenu", versus);
        const auto online = popup(versus, menu(L"WD_OnlineMenu_C"), L"Online menu");
        const auto lobby_menu = popup(online, menu(L"WD_LobbyMenu_C"), L"Lobby menu");
        const auto search_class = menu(L"WD_LobbySearch_C");
        fields(search_class, {L"Popup_IDInput"});
        bools(search_class, {L"IDInputActive"});
        const auto search = popup(lobby_menu, search_class, L"Lobby search");
        flag(search, L"IDInputActive", true);
        const auto id_class = menu(L"WD_LobbyIDInput_C");
        fields(id_class, {L"TextBlock_Caption", L"EditableText_132"});
        const auto id = popup(search, id_class, L"Enter Lobby ID");
        set(search, L"Popup_IDInput", id);
        block(id, L"TextBlock_Caption", L"Enter Lobby ID");
        const auto edit_class = klass(L"EditableText", cls[L"Widget"]);
        prop(edit_class, L"Text", L"TextProperty", 24);
        const auto edit = child(id, L"EditableText_132", edit_class);
        field_text(edit, L"Text", L"Stale edit buffer");
        return id;
    }

    Address popup(Address parent, Address c, const wchar_t* name) {
        const auto panel = child(parent, L"CanvasPanel_Popup", cls[L"CanvasPanel"]);
        const auto w = list_row(parent, panel, c, name);
        set(w, L"ParentCanvas", panel);
        tick(w);
        items<Address>(parent, L"PopupItem", {w});
        return w;
    }

    MenuSnapshot sample(const std::function<std::optional<int>(Address)>& visible = {}) {
        return online_menus(unreal, world, visible);
    }

    Address field(Address w, const wchar_t* name) { return raw<Address>(w + static_cast<Address>(off(w, name))); }
};

void lobby_uses_the_focus_row_and_blocks_during_scroll_or_replacement() {
    OnlineFixture f;
    const auto row = f.setup_lobby();
    auto s = f.sample();
    CHECK(s.valid && s.screen == L"Search Results" && s.label == L"Second Host");
    CHECK(s.position == 2 && s.count == 2);
    CHECK(s.value.find(L"Friendly") != std::wstring::npos && s.value.find(L"2/4") != std::wstring::npos);
    CHECK(s.value.find(L"Hidden region") == std::wstring::npos);
    const auto slot = f.raw<Address>(f.field(f.lobby, L"VerticalBox_List") + 0x30);
    f.set<OnlineFixture::Position>(slot, L"LayoutData", {100.f, 255.f});
    s = f.sample();
    CHECK(s.blocked && !s.valid);
    f.set<OnlineFixture::Position>(slot, L"LayoutData", {100.f, 200.f});
    f.items<Address>(f.lobby, L"ListItemArray", {0, row});
    const auto foreign = f.user(f.cls[L"WD_LobbySearchResult_C"], L"Retained results", true);
    f.slot(row, foreign);
    s = f.sample();
    CHECK(s.blocked && !s.valid);
}

void lobby_empty_view_and_owner_changes() {
    OnlineFixture f;
    f.setup_lobby();
    f.items<Address>(f.lobby, L"ListItemArray", {});
    f.hide(f.field(f.lobby, L"Image_Cursor"));
    auto s = f.sample();
    CHECK(s.valid && s.label == L"No lobbies found");
    const auto second = f.actor(f.cls[L"GF_Lobby_C"], L"Second flow");
    f.set<std::uint8_t>(second, L"LobbyFlowState", 1);
    f.set(second, L"LobbySearchResult", f.lobby);
    s = f.sample();
    CHECK(s.blocked && !s.valid);
    const auto other_world = f.object(L"Old world", f.cls[L"World"]);
    const auto other_level = f.object(L"Old level", f.cls[L"Level"]);
    f.put(other_level + 0xb0, other_world);
    f.put(second + 0x20, other_level);
    CHECK(f.sample().valid);
}

void ranking_uses_the_current_visible_row_and_rejects_stale_rank() {
    OnlineFixture f;
    const auto row = f.setup_ranking();
    auto s = f.sample();
    CHECK(s.valid && s.label == L"Fighter 14");
    CHECK(s.value.find(L"14") != std::wstring::npos && s.value.find(L"Wins 35") != std::wstring::npos);
    f.set<int>(f.ranking, L"ViewTopItem", 14);
    s = f.sample();
    CHECK(s.blocked && !s.valid);
    f.set<int>(f.ranking, L"ViewTopItem", 13);
    f.flag(f.ranking, L"isShowProfileCard", true);
    s = f.sample();
    CHECK(s.blocked && !s.valid);
    f.flag(f.ranking, L"isShowProfileCard", false);
    f.hide(f.field(row, L"CanvasPanel_Body"));
    f.show(f.field(row, L"CanvasPanel_NoDate"));
    s = f.sample();
    CHECK(s.valid && s.label == L"No ranking data" && s.value.empty());
}

void replay_distinguishes_local_and_favorite_rows_and_rejects_dialogs() {
    OnlineFixture f;
    f.setup_replay();
    auto s = f.sample();
    CHECK(s.valid && s.label == L"One versus Two");
    CHECK(s.value.find(L"11/05/2026") != std::wstring::npos && s.value.find(L"WINNER") != std::wstring::npos);
    CHECK(s.value.find(L"Hidden result") == std::wstring::npos);
    f.set<int>(f.replay, L"FocusIndex", 2);
    s = f.sample();
    CHECK(s.valid && s.label == L"Empty replay slot" && s.value == L"Favorite");
    f.flag(f.replay, L"IsSpawnDialog", true);
    s = f.sample();
    CHECK(s.blocked && !s.valid);
}

void copied_selection_must_survive_the_final_visibility_read() {
    OnlineFixture f;
    const auto row = f.setup_lobby();
    const auto b = f.field(row, L"TextBlock_PlayerID");
    int reads = 0;
    const auto s = f.sample([&](Address a) -> std::optional<int> {
        if (a == b && ++reads == 2) f.set<int>(f.lobby, L"FocusIndex", 0);
        return f.raw<std::uint8_t>(a + 0x91);
    });
    CHECK(reads >= 2 && s.blocked && !s.valid);
}

void lobby_room_and_member_popup_have_distinct_visible_owners() {
    OnlineFixture f;
    f.setup_lobby_room();
    auto s = f.sample();
    CHECK(s.valid && s.label == L"Battle 02");
    CHECK(s.value.find(L"First combatant") != std::wstring::npos);
    CHECK(s.value.find(L"Retained second combatant") == std::wstring::npos);
    const auto popup = f.setup_members(f.lobby);
    f.set(f.lobby, L"ActivePopup", popup);
    s = f.sample();
    CHECK(s.valid && s.label == L"Chosen member" && s.screen == L"Lobby Members");
    f.set(f.lobby, L"ActivePopup", Address(0));
    s = f.sample();
    CHECK(s.blocked && !s.valid);
}

void ranking_profile_replaces_the_list_only_when_its_page_is_drawn() {
    OnlineFixture f;
    f.setup_ranking();
    const auto main = f.setup_profile();
    auto s = f.sample();
    CHECK(s.valid && s.label == L"Profile player");
    CHECK(s.value.find(L"Visible title") != std::wstring::npos);
    CHECK(s.value.find(L"Losses 5") != std::wstring::npos && s.value.find(L"Hidden wins") == std::wstring::npos);
    f.set<int>(main, L"IndexCurrentPage", 3);
    s = f.sample();
    CHECK(s.blocked && !s.valid);
}

void title_flow_routes_lobby_id_to_the_displayed_form() {
    OnlineFixture f;
    const auto id = f.setup_lobby_id();
    auto s = f.sample();
    CHECK(s.valid && s.label == L"Enter Lobby ID" && s.value.empty());
    f.hide(f.field(id, L"EditableText_132"));
    s = f.sample();
    CHECK(s.blocked && !s.valid);
    f.show(f.field(id, L"EditableText_132"));
    f.set(id, L"ParentCanvas", Address(0));
    s = f.sample();
    CHECK(s.blocked && !s.valid);
}

void title_popup_with_null_canvas_requires_direct_viewport_membership() {
    OnlineFixture f;
    const auto id = f.setup_lobby_id();
    const auto slot = f.raw<Address>(id + 0x30);
    const auto panel = f.raw<Address>(slot + 0x28);
    const auto tree = f.raw<Address>(panel + 0x20);
    const auto search = f.raw<Address>(tree + 0x20);
    f.set(search, L"CanvasPanel_Popup", Address(0));
    f.set(id, L"ParentCanvas", Address(0));
    f.put(id + 0x30, Address(0));
    CHECK(f.sample().blocked);
    const auto shared = f.alloc(16);
    f.put<int>(shared + 8, 1);
    f.put(id + 0x208, f.heap);
    f.put(id + 0x210, shared);
    const auto s = f.sample();
    CHECK(s.valid && s.label == L"Enter Lobby ID");
}

void favorite_replay_marker_is_reported_only_when_drawn() {
    OnlineFixture f;
    const auto row = f.setup_replay();
    CHECK(f.sample().value.find(L"Marked favorite") == std::wstring::npos);
    f.show(f.field(row, L"Image_Favorite"));
    CHECK(f.sample().value.find(L"Marked favorite") != std::wstring::npos);
}

void profile_character_edit_reads_the_focused_rendered_option() {
    OnlineFixture f;
    f.setup_ranking();
    const auto card = f.setup_profile();
    const auto page = f.setup_character_edit(card);
    auto s = f.sample();
    CHECK(s.valid && s.screen == L"Character Edit" && s.label == L"Costume" && s.value == L"Default");
    const auto cursor = f.field(page, L"CanvasPanel_Cursor");
    const auto slot = f.raw<Address>(cursor + 0x30);
    f.set<OnlineFixture::Position>(slot, L"LayoutData", {150.f, 100.f});
    CHECK(f.sample().blocked);
    f.set<int>(page, L"FocusIndex", 0);
    s = f.sample();
    CHECK(s.valid && s.label == L"Character 1");
}

void acquired_titles_read_the_drawn_cell_and_only_the_open_condition() {
    OnlineFixture f;
    f.setup_ranking();
    const auto page = f.setup_acquired_titles(f.setup_profile());
    auto s = f.sample();
    CHECK(s.valid && s.screen == L"Acquired Titles" && s.label == L"Chosen title" && s.value.empty());
    f.flag(page, L"isPopupActive", true);
    f.show(f.field(page, L"CanvasPanel_Popup"));
    s = f.sample();
    CHECK(s.valid && s.value == L"Win five matches");
    const auto rows = f.unreal.array(page, *f.unreal.property(page, L"ListItemArray"));
    const auto row = f.raw<Address>(rows->data);
    f.hide(f.field(row, L"TextBlock_AqTitle_1"));
    s = f.sample();
    CHECK(s.valid && !s.enabled && s.label == L"Locked title" && s.value == L"Win five matches");
    f.set<int>(page, L"indexCursorX", 0);
    s = f.sample();
    CHECK(s.blocked && !s.valid);
}
}

int main() {
    try {
        lobby_uses_the_focus_row_and_blocks_during_scroll_or_replacement();
        lobby_empty_view_and_owner_changes();
        ranking_uses_the_current_visible_row_and_rejects_stale_rank();
        replay_distinguishes_local_and_favorite_rows_and_rejects_dialogs();
        copied_selection_must_survive_the_final_visibility_read();
        lobby_room_and_member_popup_have_distinct_visible_owners();
        ranking_profile_replaces_the_list_only_when_its_page_is_drawn();
        title_flow_routes_lobby_id_to_the_displayed_form();
        title_popup_with_null_canvas_requires_direct_viewport_membership();
        favorite_replay_marker_is_reported_only_when_drawn();
        profile_character_edit_reads_the_focused_rendered_option();
        acquired_titles_read_the_drawn_cell_and_only_the_open_condition();
        std::cout << checks << " online menu checks passed\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
