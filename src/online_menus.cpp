#include "mvci/unreal.hpp"
#include "mvci/localization.hpp"
#include "widget_geometry.hpp"
#include <limits>
#include <string_view>

namespace mvci {
std::optional<MenuSnapshot> rendered_menu_option(Unreal&, Address, const std::function<std::optional<int>(Address)>&);
namespace {
using story_detail::Frame;

constexpr Address engine_rva = 0x36bc348;
constexpr int lobby_search_flow = 1;
constexpr int lobby_main_flow = 0;
constexpr int replay_select_data = 1;
constexpr int lobby_normal = 0;
constexpr int lobby_show_members = 1;
constexpr int server_replay_slots = 5;

MenuSnapshot pending() {
    MenuSnapshot out;
    out.blocked = true;
    return out;
}

MenuSnapshot selection(Frame& frame, Address owner, std::wstring screen, std::wstring label) {
    MenuSnapshot out;
    const auto ref = frame.u.object(owner);
    if (!ref || !frame.retain(owner)) return pending();
    out.valid = true;
    out.identity = (static_cast<std::uint64_t>(static_cast<std::uint32_t>(ref->serial)) << 32) |
                   static_cast<std::uint32_t>(ref->index);
    out.screen = std::move(screen);
    out.label = std::move(label);
    return out;
}

bool in_world(Frame& frame, Address actor, Address world) {
    if (!frame.retain(actor)) return false;
    const auto level = frame.get<Address>(actor + 0x20);
    return level && *level && frame.retain(*level) && frame.u.is_a(*level, L"Level") &&
           frame.get<Address>(*level + 0xb0) == std::optional<Address>(world);
}

bool current_world(Frame& frame, Address world) {
    if (!frame.retain(world) || !frame.u.is_a(world, L"World")) return false;
    const auto engine = frame.get<Address>(frame.u.base() + engine_rva);
    if (!engine || !*engine || !frame.retain(*engine)) return false;
    const auto viewport = frame.get<Address>(*engine + 0x5d8);
    return viewport && *viewport && frame.retain(*viewport) &&
           frame.get<Address>(*viewport + 0x80) == std::optional<Address>(world);
}

bool exact(Frame& frame, Address address, const wchar_t* class_name) {
    const auto ref = frame.u.object(address);
    return ref && frame.u.name_is(ref->cls, class_name);
}

std::optional<int> tick_state(Frame& frame, Address owner) {
    const auto tick = frame.field(owner, L"TickStateObj");
    if (!tick || !*tick || !exact(frame, *tick, L"BP_TickStateObj_C") ||
        frame.field(*tick, L"ParentObj") != std::optional<Address>(owner) ||
        frame.boolean(*tick, L"PreState") != std::optional<bool>(false)) return {};
    return frame.integer(*tick, L"State");
}

bool active(Frame& frame, Address owner) {
    return tick_state(frame, owner) == std::optional<int>(2);
}

std::optional<Address> shown_child(Frame& frame, Address owner, const wchar_t* field, const wchar_t* class_name) {
    const auto child = frame.field(owner, field);
    if (!child || !*child || !frame.u.is_a(*child, class_name) ||
        !frame.within(*child, owner) || !frame.shown(*child)) return {};
    return child;
}

bool viewport_root(Frame& frame, Address widget) {
    // Each GameFlow passes a null ParentCanvas to Activate.
    const auto viewport = frame.get<Address>(widget + 0x208);
    const auto shared = frame.get<Address>(widget + 0x210);
    if (!viewport || !*viewport || !shared || !*shared ||
        frame.get<Address>(widget + 0x30) != std::optional<Address>(0)) return false;
    const auto strong = frame.get<int>(*shared + 8);
    return strong && *strong > 0;
}

bool popup_clear(Frame& frame, Address root, const wchar_t* field) {
    const auto popup = frame.field(root, field);
    return popup && (!*popup || !frame.shown(*popup));
}

std::optional<Address> slot_of(Frame& frame, Address widget) {
    const auto slot = frame.get<Address>(widget + 0x30);
    if (!slot || !*slot || !frame.retain(*slot) ||
        frame.field(*slot, L"Content") != std::optional<Address>(widget)) return {};
    return slot;
}

std::optional<Address> row_at(Frame& frame, const ArrayView& array, int index,
                              Address owner, const wchar_t* type, Address panel = 0) {
    if (index < 0 || index >= array.count) return {};
    const auto row = frame.get<Address>(array.data + static_cast<Address>(index) * 8);
    if (!row || !*row || !frame.retain(*row) || !exact(frame, *row, type) ||
        !frame.within(*row, owner) || !frame.shown(*row)) return {};
    if (panel && !widget_geometry::slot_of(frame, *row, panel)) return {};
    return row;
}

bool at_position(Frame& frame, Address widget, float target_x, float target_y) {
    const auto slot = slot_of(frame, widget);
    if (!slot || !frame.u.is_a(*slot, L"CanvasPanelSlot")) return false;
    const auto x = widget_geometry::number(frame, *slot, L"LayoutData", {L"Offsets", L"Left"});
    const auto y = widget_geometry::number(frame, *slot, L"LayoutData", {L"Offsets", L"Top"});
    constexpr float tolerance = 0.5f;
    return widget_geometry::near(x, target_x, tolerance) && widget_geometry::near(y, target_y, tolerance);
}

bool at_destination(Frame& frame, Address root, Address widget, const wchar_t* destination) {
    // The Blueprints interpolate the CanvasPanelSlot position toward a reflected Vector2D.
    const auto target_x = widget_geometry::number(frame, root, destination, {L"X"});
    const auto target_y = widget_geometry::number(frame, root, destination, {L"Y"});
    return target_x && target_y && at_position(frame, widget, *target_x, *target_y);
}

std::wstring drawn_text(Frame& frame, Address owner, const wchar_t* field) {
    const auto block = shown_child(frame, owner, field, L"TextBlock");
    if (!block || !slot_of(frame, *block)) return {};
    if (frame.text_alpha(*block) == std::optional<float>(0.f)) return {};
    const auto displayed = frame.display(*block);
    auto text = displayed ? normalize_text(*displayed) : std::wstring{};
    return story_detail::readable(text) ? text : std::wstring{};
}

void append(std::wstring& out, const std::wstring& part) {
    if (part.empty() || out == part) return;
    if (!out.empty()) out += out.back() == L'.' ? L" " : L". ";
    out += part;
}

void captioned(std::wstring& out, std::wstring_view message, const std::wstring& value) {
    if (!value.empty()) append(out, tr(message, {value}));
}

std::wstring visible_guide(Frame& frame, Address root, const wchar_t* line_field = nullptr) {
    if (line_field) {
        if (const auto guide = shown_child(frame, root, line_field, L"WD_LineHelp_C")) {
            const auto pc = shown_child(frame, *guide, L"HorizontalBox_PC", L"HorizontalBox");
            const auto pad = shown_child(frame, *guide, L"HorizontalBox_Cage", L"HorizontalBox");
            if (pc.has_value() != pad.has_value()) {
                if (const auto text = frame.line(pc ? *pc : *pad, *guide)) return *text;
            }
        }
    }
    const auto guide = shown_child(frame, root, L"WD_BtnNavi", L"WD_BtnNavi_C");
    return guide ? frame.hints(*guide, true) : std::wstring{};
}

MenuSnapshot profile_card(Frame&, Address);
MenuSnapshot lobby_members(Frame&, Address);

MenuSnapshot lobby_results(Frame& frame, Address root) {
    const auto state = frame.integer(root, L"LobbySearchResultState");
    if (state == std::optional<int>(lobby_show_members)) {
        const auto popup = shown_child(frame, root, L"WD_PopupMembers", L"WD_PopupMembers_C");
        return popup ? lobby_members(frame, *popup) : pending();
    }
    if (state != std::optional<int>(lobby_normal) || !popup_clear(frame, root, L"WD_PopupMembers")) return pending();
    const auto rows = frame.array(root, L"ListItemArray", 20);
    const auto cursor = shown_child(frame, root, L"Image_Cursor", L"Image");
    auto screen = drawn_text(frame, root, L"TextBlock_Caption");
    if (screen.empty() || !rows) return pending();
    // The no-match branch clears ListItemArray and hides Image_Cursor.
    if (rows->count == 0) {
        const auto hidden_cursor = frame.field(root, L"Image_Cursor");
        if (cursor || !hidden_cursor || !*hidden_cursor || !frame.within(*hidden_cursor, root) ||
            !frame.u.is_a(*hidden_cursor, L"Image") || frame.shown(*hidden_cursor)) return pending();
        auto out = selection(frame, root, std::move(screen), tr(L"No lobbies found"));
        out.help = visible_guide(frame, root);
        return out;
    }
    const auto list = shown_child(frame, root, L"VerticalBox_List", L"VerticalBox");
    const auto focus = frame.integer(root, L"FocusIndex");
    const auto visual_index = frame.integer(root, L"IndexCursor");
    const auto scroll = frame.integer(root, L"ViewRangeTop");
    const auto display_count = frame.integer(root, L"ListShowMax");
    const auto last = frame.integer(root, L"ListLastIndex");
    // Both mouse and pad set the highlighted row from FocusIndex. ViewRangeTop is a negative scroll offset.
    if (!cursor || !list || !focus || !visual_index || !scroll || !display_count ||
        last != std::optional<int>(rows->count - 1) || *focus < 0 || *focus >= rows->count ||
        *display_count <= 0 || *display_count > 20 || *scroll > 0 || *scroll < -rows->count ||
        *visual_index != *focus + *scroll || *visual_index < 0 || *visual_index >= *display_count ||
        !at_destination(frame, root, *cursor, L"DestinationCursorPos") ||
        !at_destination(frame, root, *list, L"DestinationScrollPos")) return pending();
    const auto row = row_at(frame, *rows, *focus, root, L"WD_LobbyListItem_C", *list);
    if (!row) return pending();
    auto host = drawn_text(frame, *row, L"TextBlock_PlayerID");
    if (host.empty()) return pending();
    auto out = selection(frame, root, std::move(screen), std::move(host));
    out.position = *focus + 1;
    out.count = rows->count;
    for (const auto* name : {L"TextBlock_LobbyTitle", L"TextBlock_LobbySize", L"TextBlock_MatchType",
                             L"TextBlock_BattleType", L"TextBlock_Region", L"TextBlock_Level", L"TextBlock_Time"}) {
        append(out.value, drawn_text(frame, *row, name));
    }
    out.help = visible_guide(frame, root);
    return out;
}

bool equal_rank(const std::wstring& text, int expected) {
    // The widget formats ranks without grouping. Match the drawn number in either
    // Western or localized Arabic/fullwidth digits before reading a recycled row.
    if (text.empty() || text.size() > 10) return false;
    std::int64_t rank = 0;
    for (const auto ch : text) {
        int digit = -1;
        for (const wchar_t zero : {L'0', L'\u0660', L'\u06f0', L'\uff10'}) {
            if (ch >= zero && ch <= zero + 9) digit = ch - zero;
        }
        if (digit < 0) return false;
        rank = 10 * rank + digit;
    }
    return rank == expected;
}

MenuSnapshot rankings(Frame& frame, Address root) {
    const auto profile_open = frame.boolean(root, L"isShowProfileCard");
    if (profile_open == std::optional<bool>(true)) {
        const auto profile = frame.field(root, L"ProfileCard");
        return profile && *profile ? profile_card(frame, *profile) : pending();
    }
    if (profile_open != std::optional<bool>(false) ||
        frame.boolean(root, L"enterFinished") != std::optional<bool>(true) ||
        frame.integer(root, L"ScrollingNum") != std::optional<int>(0) ||
        !popup_clear(frame, root, L"Dialog_GetRankingData") ||
        !shown_child(frame, root, L"CanvasPanel_Cursor", L"CanvasPanel")) return pending();
    const auto list = shown_child(frame, root, L"CanvasPanel_List", L"CanvasPanel");
    const auto rows = frame.array(root, L"ListItemArray", 64);
    const auto focus = frame.integer(root, L"IndexFocus");
    const auto top = frame.integer(root, L"ViewTopItem");
    if (!list || !rows || !focus || !top || *top < 0 || *focus < 0 || *focus >= rows->count ||
        *top >= (std::numeric_limits<int>::max)() - *focus) return pending();
    const auto row = row_at(frame, *rows, *focus, root, L"WD_RankList_C", *list);
    if (!row) return pending();
    auto screen = drawn_text(frame, root, L"TextBlock_BoardTitle");
    if (screen.empty() || is_umg_default_text(screen)) screen = tr(L"Rankings");
    const auto body = shown_child(frame, *row, L"CanvasPanel_Body", L"CanvasPanel");
    const auto no_data = shown_child(frame, *row, L"CanvasPanel_NoDate", L"CanvasPanel");
    if (body.has_value() == no_data.has_value()) return pending();
    if (no_data) {
        auto out = selection(frame, *row, std::move(screen), tr(L"No ranking data"));
        out.help = visible_guide(frame, root, L"WD_LineHelp");
        return out;
    }
    const auto scale = widget_geometry::number(frame, *body, L"RenderTransform", {L"Scale", L"Y"});
    if (scale && *scale <= 0.f) return pending();
    const auto rank = drawn_text(frame, *row, L"TextBlock_RankNumber");
    auto player = drawn_text(frame, *row, L"TextBlock_PlayerID");
    if (!equal_rank(rank, *top + *focus + 1) || player.empty()) return pending();
    auto out = selection(frame, *row, std::move(screen), std::move(player));
    const auto suffix = drawn_text(frame, *row, L"TextBlock_Suffix");
    out.value = tr(L"Rank {0}", {rank + suffix});
    const auto points = drawn_text(frame, *row, L"TextBlock_Points");
    const auto column = drawn_text(frame, root, L"TextBlock_Column2");
    if (!points.empty()) append(out.value, column.empty() ? points : column + L" " + points);
    out.help = visible_guide(frame, root, L"WD_LineHelp");
    return out;
}

bool replay_empty_date(const std::wstring& text) {
    // setEmpty fills the platform-formatted date using only dashes and separators.
    return text.size() >= 8 && text.find(L"----") != std::wstring::npos &&
           text.find_first_not_of(L"-/ .") == std::wstring::npos;
}

MenuSnapshot replay_row(Frame& frame, Address row, std::wstring screen, std::wstring section) {
    const auto date = drawn_text(frame, row, L"TextBlock_Date");
    const auto first = drawn_text(frame, row, L"TextBlock_UID1");
    const auto second = drawn_text(frame, row, L"TextBlock_UID2");
    // Both widget designer placeholders are invariant across the shipped cultures.
    if (date.empty() || date == L"99/99/2099" || date == L"DATE") return pending();
    if (replay_empty_date(date) && first.empty() && second.empty()) {
        auto out = selection(frame, row, std::move(screen), tr(L"Empty replay slot"));
        out.value = std::move(section);
        return out;
    }
    if (first.empty() && second.empty()) return pending();
    const auto left_result = drawn_text(frame, row, L"TextBlock_Result1P");
    const auto right_result = drawn_text(frame, row, L"TextBlock_Result2P");
    auto out = selection(frame, row, std::move(screen),
                         first.empty() ? second : second.empty() ? first : tr(L"{0} versus {1}", {first, second}));
    append(out.value, section);
    append(out.value, date);
    append(out.value, drawn_text(frame, row, L"TextBlock_MatchType"));
    if (!first.empty() && !left_result.empty()) append(out.value, first + L" " + left_result);
    if (!second.empty() && !right_result.empty()) append(out.value, second + L" " + right_result);
    if (shown_child(frame, row, L"Image_Favorite", L"Image")) append(out.value, tr(L"Marked favorite"));
    return out;
}

MenuSnapshot replays(Frame& frame, Address root) {
    if (frame.boolean(root, L"IsSpawnDialog") != std::optional<bool>(false) ||
        !popup_clear(frame, root, L"SaveReplayDialog") || !popup_clear(frame, root, L"GetReplayDialog") ||
        !popup_clear(frame, root, L"DeleteDialog")) return pending();
    const auto cursor = shown_child(frame, root, L"CanvasPanel_Cursor", L"CanvasPanel");
    const auto focus = frame.integer(root, L"FocusIndex");
    if (!cursor || !focus || *focus < 0 || !at_destination(frame, root, *cursor, L"DestinationCursorPos")) return pending();
    std::optional<Address> row;
    std::wstring section;
    if (*focus < server_replay_slots) {
        const auto rows = frame.array(root, L"ServerListWg", server_replay_slots);
        if (!rows || rows->count != server_replay_slots) return pending();
        row = row_at(frame, *rows, *focus, root, L"WD_ReplayListSmall_C");
        constexpr const wchar_t* slots[]{L"WD_ReplayListSmall_Auto", L"WD_ReplayListSmall_Preset1",
                                         L"WD_ReplayListSmall_Preset2", L"WD_ReplayListSmall_Preset3",
                                         L"WD_ReplayListSmall_Preset4"};
        if (row != frame.field(root, slots[*focus])) return pending();
        section = drawn_text(frame, root, *focus ? L"TextBlock_Label_Favorite" : L"TextBlock_Label_Auto");
    } else {
        const auto list = shown_child(frame, root, L"VerticalBox_List", L"VerticalBox");
        const auto rows = frame.array(root, L"RocalListWg", 50);
        const auto visual_index = frame.integer(root, L"CursorIndex");
        const auto display_count = frame.integer(root, L"ListShowMax");
        const auto scroll = frame.integer(root, L"ScrollValue");
        const auto index = *focus - server_replay_slots;
        if (!list || !rows || !visual_index || !display_count || !scroll ||
            *scroll > 0 || *scroll < -rows->count || *display_count < 1 || *display_count > 50 ||
            index >= rows->count || *visual_index != index + *scroll || *visual_index < 0 ||
            *visual_index >= *display_count || !at_destination(frame, root, *list, L"DestinationScrollPos")) return pending();
        row = row_at(frame, *rows, index, root, L"WD_ReplayListSmall_C", *list);
        section = drawn_text(frame, root, L"TextBlock_Label");
    }
    if (!row) return pending();
    auto out = replay_row(frame, *row, tr(L"Replays"), std::move(section));
    out.help = visible_guide(frame, root, L"WD_LineHelp_366");
    return out;
}

MenuSnapshot character_edit(Frame& frame, Address page) {
    const auto panel = shown_child(frame, page, L"CanvasPanel_CharcterEdit", L"CanvasPanel");
    const auto focus = frame.integer(page, L"FocusIndex");
    if (!panel || !focus || *focus < 0 || *focus > 7 ||
        frame.boolean(page, L"isCharEdit") != std::optional<bool>(true)) return pending();
    if (*focus == 0) {
        if (!shown_child(frame, page, L"Image_CursorTop", L"Image")) return pending();
        auto target = drawn_text(frame, page, L"TextBlock_TargetType");
        if (target.empty()) return pending();
        return selection(frame, page, tr(L"Character Edit"), std::move(target));
    }
    // ChangeFocusCore selects SettingItemArray[FocusIndex - 1]. UpdateCursor
    // moves the drawn list highlight toward that same index.
    const auto cursor = shown_child(frame, page, L"CanvasPanel_Cursor", L"CanvasPanel");
    const auto rows = frame.array(page, L"SettingItemArray", 7);
    const auto count = frame.integer(page, L"SettingListMax");
    const auto step_prop = frame.u.property(page, L"CursorListStep");
    if (!cursor || !rows || rows->count != 7 || !count || *count < *focus || *count > rows->count ||
        !step_prop || step_prop->kind != L"FloatProperty" || step_prop->size != 4) return pending();
    const auto index = *focus - 1;
    const auto x = widget_geometry::number(frame, page, L"CursorDefaultPos_List", {L"X"});
    const auto y = widget_geometry::number(frame, page, L"CursorDefaultPos_List", {L"Y"});
    const auto step = frame.get<float>(page + static_cast<Address>(step_prop->offset));
    if (!x || !y || !step || !std::isfinite(*step) || *step <= 0.f ||
        !at_position(frame, *cursor, *x, *y + static_cast<float>(index) * *step)) return pending();
    const auto row = row_at(frame, *rows, index, page, L"WD_CEspinbox_C");
    constexpr const wchar_t* fields[]{L"WD_CEspinbox_Item0", L"WD_CEspinbox_Item1", L"WD_CEspinbox_Item2", L"WD_CEspinbox_Item3",
                                      L"WD_CEspinbox_Item4", L"WD_CEspinbox_Item5", L"WD_CEspinbox_Item6"};
    if (!row || frame.field(page, fields[index]) != row || !frame.within(*row, *panel)) return pending();
    auto label = drawn_text(frame, *row, L"TextBlock_ListItem");
    auto value = drawn_text(frame, *row, L"TextBlock_Value");
    if (label.empty() || value.empty()) return pending();
    auto out = selection(frame, *row, tr(L"Character Edit"), std::move(label));
    out.value = std::move(value);
    return out;
}

MenuSnapshot acquired_titles(Frame& frame, Address page) {
    constexpr int columns = 5;
    // SetupItemWidget examines authored IDs 0 through 300, at five cells per row.
    const auto rows = frame.array(page, L"ListItemArray", 61);
    const auto list = shown_child(frame, page, L"VerticalBox_0", L"VerticalBox");
    const auto cursor = shown_child(frame, page, L"Image_ListCursor", L"Image");
    const auto focus = frame.integer(page, L"indexFocus");
    const auto cursor_x = frame.integer(page, L"indexCursorX");
    const auto cursor_y = frame.integer(page, L"indexCursorY");
    const auto scroll = frame.integer(page, L"ViewRangeTop");
    const auto height = frame.integer(page, L"ListShowMax");
    if (!rows || !list || !cursor || !focus || !cursor_x || !cursor_y || !scroll || !height ||
        frame.boolean(page, L"isActive") != std::optional<bool>(true) ||
        frame.integer(page, L"ListShowMax_W") != std::optional<int>(columns) ||
        *focus < 0 || *focus >= rows->count * columns || *scroll > 0 || *scroll < -rows->count ||
        *height <= 0 || *cursor_y < 0 || *cursor_y >= *height ||
        *cursor_x != *focus % columns || *cursor_y != *focus / columns + *scroll ||
        !at_destination(frame, page, *cursor, L"DestinationCursorPos") ||
        !at_destination(frame, page, *list, L"DestinationScrollPos")) return pending();
    const auto row = row_at(frame, *rows, *focus / columns, page, L"WD_AQPanel_C", *list);
    if (!row) return pending();
    constexpr const wchar_t* fields[]{L"TextBlock_AqTitle_0", L"TextBlock_AqTitle_1", L"TextBlock_AqTitle_2",
                                      L"TextBlock_AqTitle_3", L"TextBlock_AqTitle_4"};
    constexpr const wchar_t* plates[]{L"Image_0", L"Image_1", L"Image_2", L"Image_3", L"Image_4"};
    const auto labels = frame.array(*row, L"TextBlockArray", columns);
    const auto field = fields[*cursor_x];
    const auto label_block = frame.field(*row, field);
    if (!labels || labels->count != columns || !label_block || !*label_block ||
        frame.get<Address>(labels->data + static_cast<Address>(*cursor_x) * 8) != label_block ||
        !frame.u.is_a(*label_block, L"TextBlock") || !frame.within(*label_block, *row)) return pending();
    auto title = drawn_text(frame, *row, field);
    if ((*cursor_x == 1 || *cursor_x == 2) && is_umg_default_text(title)) return pending();
    bool locked = false;
    if (title.empty()) {
        // setDisable hides the title but draws a dark plate. setAllItem hides both for a blank slot.
        if (frame.integer(*label_block, L"Visibility") != std::optional<int>(2) ||
            !shown_child(frame, *row, plates[*cursor_x], L"Image")) return pending();
        title = tr(L"Locked title");
        locked = true;
    }
    auto out = selection(frame, *label_block, tr(L"Acquired Titles"), std::move(title));
    out.enabled = !locked;
    const auto details = frame.boolean(page, L"isPopupActive");
    if (!details) return pending();
    if (*details) {
        const auto popup = shown_child(frame, page, L"CanvasPanel_Popup", L"CanvasPanel");
        const auto condition = frame.field(page, L"TextBlock_Condition");
        if (!popup || !condition || !*condition || !frame.within(*condition, *popup)) return pending();
        out.value = drawn_text(frame, page, L"TextBlock_Condition");
        if (out.value.empty() || is_umg_default_text(out.value)) return pending();
    }
    out.help = visible_guide(frame, page);
    return out;
}

MenuSnapshot profile_card(Frame& frame, Address spawn) {
    if (!exact(frame, spawn, L"WD_ProfCardSpawn_C") || !frame.shown(spawn) || !active(frame, spawn) ||
        !popup_clear(frame, spawn, L"ErrDialog") || !popup_clear(frame, spawn, L"LoginWaitDialog") ||
        !popup_clear(frame, spawn, L"SaveDialog")) return pending();
    const auto card = frame.field(spawn, L"CardMain");
    if (!card || !*card || !exact(frame, *card, L"WD_ProfCardMain_C") || !viewport_root(frame, *card) ||
        !frame.shown(*card) || !active(frame, *card) ||
        frame.boolean(*card, L"isExit") != std::optional<bool>(false)) return pending();
    const auto pages = frame.array(*card, L"PageWidgetArray", 5);
    const auto current = frame.integer(*card, L"IndexCurrentPage");
    const auto edit = frame.boolean(*card, L"isCharEdit");
    const auto titles = frame.boolean(*card, L"isAqTitleSelect");
    if (!pages || !current || *current < 0 || *current >= 5 || !edit || !titles ||
        (*edit && *current != 0) || (*titles != (*current == 4))) return pending();
    const auto panel = shown_child(frame, *card, *current ? L"CanvasPanel_inPage" : L"CanvasPanel_Top", L"CanvasPanel");
    if (!panel) return pending();
    constexpr const wchar_t* types[]{L"WD_PFcardTop_C", L"WD_PFcardBattleRecord_C", L"WD_PFcardBattleRecord_C",
                                     L"WD_PFcardReplay_C", L"WD_PFcardAqSelect_C"};
    const auto page = row_at(frame, *pages, *current, *card, types[*current], *panel);
    if (!page) return pending();
    if (*titles) return acquired_titles(frame, *page);
    if (*edit) {
        auto out = character_edit(frame, *page);
        out.help = visible_guide(frame, *card, L"WD_LineHelp");
        return out;
    }
    const auto header = shown_child(frame, *card, L"WD_PFcardBase", L"WD_PFcardBase_C");
    auto screen = header ? drawn_text(frame, *header, L"TextBlock_Caption") : std::wstring{};
    if (screen.empty() || is_umg_default_text(screen)) screen = tr(L"Profile Card");
    MenuSnapshot out;
    if (*current == 0) {
        if (!popup_clear(frame, *page, L"CanvasPanel_CharcterEdit")) return pending();
        auto player = drawn_text(frame, *page, L"TextBlock_UserID");
        if (player.empty()) return pending();
        out = selection(frame, *page, std::move(screen), std::move(player));
        append(out.value, drawn_text(frame, *page, L"TextBlock_AcquiredTitle"));
        captioned(out.value, L"Matches {0}", drawn_text(frame, *page, L"TextBlock_TotalP"));
        captioned(out.value, L"Wins {0}", drawn_text(frame, *page, L"TextBlock_WinP"));
        captioned(out.value, L"Losses {0}", drawn_text(frame, *page, L"TextBlock_LossP"));
        captioned(out.value, L"Draws {0}", drawn_text(frame, *page, L"TextBlock_DrawP"));
    } else if (*current == 3) {
        const auto rows = frame.array(*page, L"ListItemArray", 5);
        const auto focus = frame.integer(*page, L"indexFocus");
        if (!rows || !focus || !shown_child(frame, *page, L"CanvasPanel_Cursor", L"CanvasPanel")) return pending();
        const auto row = row_at(frame, *rows, *focus, *page, L"WD_ReplayListProf_C");
        if (!row) return pending();
        const auto body = shown_child(frame, *row, L"CanvasPanel_List", L"CanvasPanel");
        const auto empty = shown_child(frame, *row, L"CanvasPanel_NoData", L"CanvasPanel");
        if (body.has_value() == empty.has_value()) return pending();
        out = empty ? selection(frame, *row, std::move(screen), tr(L"Empty replay slot"))
                    : replay_row(frame, *row, std::move(screen), {});
    } else {
        if (frame.boolean(*page, L"onReady") != std::optional<bool>(true)) return pending();
        const auto matches = drawn_text(frame, *page, L"TextBlock_Total");
        if (matches.empty()) return pending();
        out = selection(frame, *page, std::move(screen), *current == 1 ? tr(L"Offline battle record") : tr(L"Online battle record"));
        captioned(out.value, L"Matches {0}", matches);
        captioned(out.value, L"Wins {0}", drawn_text(frame, *page, L"TextBlock_Wins"));
        captioned(out.value, L"Losses {0}", drawn_text(frame, *page, L"TextBlock_Losses"));
        captioned(out.value, L"Draws {0}", drawn_text(frame, *page, L"TextBlock_Draws"));
        captioned(out.value, L"Win rate {0}", drawn_text(frame, *page, L"TextBlock_WinRatio"));
        captioned(out.value, L"Play time {0}", drawn_text(frame, *page, L"TextBlock_PlayTime"));
    }
    out.help = visible_guide(frame, *card, L"WD_LineHelp");
    return out;
}

MenuSnapshot lobby_members(Frame& frame, Address popup) {
    if (!active(frame, popup)) return pending();
    const auto profile_open = frame.boolean(popup, L"IsProfCardActive");
    if (profile_open == std::optional<bool>(true)) {
        const auto profile = frame.field(popup, L"ProfCardReference");
        return profile && *profile ? profile_card(frame, *profile) : pending();
    }
    const auto focus = frame.integer(popup, L"FocusIndex");
    if (profile_open != std::optional<bool>(false) || !focus || *focus < 0 || *focus > 9 ||
        !shown_child(frame, popup, L"CanvasPanel_Cursor", L"CanvasPanel")) return pending();
    auto screen = drawn_text(frame, popup, L"TextBlock_Caption");
    if (screen.empty()) screen = tr(L"Lobby Members");
    MenuSnapshot out;
    if (*focus < 8) {
        const auto rows = frame.array(popup, L"WDListButtons", 8);
        const auto row = rows ? row_at(frame, *rows, *focus, popup, L"WD_MemberListBtn_C") : std::nullopt;
        if (!row) return pending();
        auto name = drawn_text(frame, *row, L"TextBlock_M0");
        if (name.empty()) return pending();
        out = selection(frame, *row, std::move(screen), std::move(name));
    } else {
        const auto spin = shown_child(frame, popup, *focus == 8 ? L"WD_SpinTextVoiceChat" : L"WD_SpinTextMemberDisp", L"WD_LobbySpinText_C");
        if (!spin || !shown_child(frame, popup, L"Image_GlowSubItem", L"Image")) return pending();
        auto label = drawn_text(frame, *spin, L"TextBlock_Label");
        auto value = drawn_text(frame, *spin, L"TextBlock_Subitem");
        if (label.empty() || value.empty()) return pending();
        out = selection(frame, *spin, std::move(screen), std::move(label));
        out.value = std::move(value);
    }
    out.help = visible_guide(frame, popup);
    return out;
}

MenuSnapshot room_choice(Frame& frame, Address popup) {
    const auto focus = frame.integer(popup, L"FocusIndex");
    if (!focus || *focus < 0 || *focus > 1 ||
        frame.boolean(popup, L"IsInput") != std::optional<bool>(true) ||
        frame.boolean(popup, L"IsDecide") != std::optional<bool>(false) ||
        !shown_child(frame, popup, L"CanvasPanel_Cursor", L"CanvasPanel")) return pending();
    auto label = drawn_text(frame, popup, *focus ? L"TextBlock_ButtonText_Training" : L"TextBlock_ButtonText_Battle");
    const auto disabled = drawn_text(frame, popup, *focus ? L"TextBlock_Bottom_Disable" : L"TextBlock_Top_Disable");
    if (!label.empty() && !disabled.empty() && label != disabled) return pending();
    if (label.empty()) label = disabled;
    if (label.empty()) return pending();
    auto out = selection(frame, popup, drawn_text(frame, popup, L"TextBlock_Caption"), std::move(label));
    out.enabled = disabled.empty();
    out.position = *focus + 1;
    out.count = 2;
    out.help = visible_guide(frame, popup);
    return out;
}

MenuSnapshot lobby_rule(Frame& frame, Address popup) {
    auto screen = drawn_text(frame, popup, L"TextBlock_Caption");
    if (screen.empty()) screen = tr(L"Lobby Rules");
    const auto host = frame.boolean(popup, L"isHost");
    if (!host) return pending();
    const auto value = [&](Address row) {
        const auto rendered = rendered_menu_option(frame.u, row, frame.visibility);
        if (!rendered || !rendered->valid) return std::optional<MenuSnapshot>{};
        frame.bindings.push_back([&frame, row, label = rendered->label, value = rendered->value] {
            const auto later = rendered_menu_option(frame.u, row, frame.visibility);
            return later && later->valid && later->label == label && later->value == value;
        });
        return rendered;
    };
    if (!*host) {
        auto out = selection(frame, popup, {}, std::move(screen));
        for (const auto* field : {L"item_battleRule", L"item_battleTime", L"item_battleStyle"}) {
            const auto row = shown_child(frame, popup, field, L"WD_OptionItem_SpinText_C");
            if (!row) continue;
            if (const auto opt = value(*row); opt && !opt->value.empty()) {
                append(out.value, opt->label + L" " + opt->value);
            }
        }
        if (out.value.empty()) return pending();
        out.help = visible_guide(frame, popup, L"WD_LineHelp");
        return out;
    }
    const auto focus = frame.integer(popup, L"ItemIndex");
    const auto displayed = frame.integer(popup, L"FocusIndex");
    const auto rows = frame.array(popup, L"Items", 16);
    const auto panel = shown_child(frame, popup, L"CanvasPanel_Menu", L"CanvasPanel");
    if (!focus || displayed != focus || !rows || *focus < 0 || *focus >= rows->count || !panel ||
        !shown_child(frame, popup, L"WD_OptionListCursor", L"WD_OptionListCursor_C")) return pending();
    const auto row = frame.get<Address>(rows->data + static_cast<Address>(*focus) * 8);
    if (!row || !*row || !frame.retain(*row) || !widget_geometry::slot_of(frame, *row, *panel)) return pending();
    const auto opt = value(*row);
    if (!opt) return pending();
    auto out = selection(frame, *row, std::move(screen), opt->label);
    out.value = opt->value;
    out.help = visible_guide(frame, popup, L"WD_LineHelp");
    return out;
}

MenuSnapshot lobby_room(Frame& frame, Address root) {
    const auto popup = frame.field(root, L"ActivePopup");
    if (!popup || frame.boolean(root, L"IsLobbyDialogActive") != std::optional<bool>(false)) return pending();
    if (*popup) {
        if (!frame.within(*popup, root) || !frame.shown(*popup) || !active(frame, *popup)) return pending();
        if (frame.field(root, L"WD_PopupMembers") == popup && exact(frame, *popup, L"WD_PopupMembers_C")) return lobby_members(frame, *popup);
        if (frame.field(root, L"WD_PopupRoomType") == popup && exact(frame, *popup, L"WD_PopupRoomType_C")) return room_choice(frame, *popup);
        if (frame.field(root, L"WD_PopupLobbyRule") == popup && exact(frame, *popup, L"WD_PopupLobbyRule_C")) return lobby_rule(frame, *popup);
        return pending();
    }
    if (!popup_clear(frame, root, L"WD_PopupMembers") || !popup_clear(frame, root, L"WD_PopupRoomType") ||
        !popup_clear(frame, root, L"WD_PopupLobbyRule") ||
        frame.integer(root, L"InLobbyState") != std::optional<int>(0)) return pending();
    const auto cursor = shown_child(frame, root, L"Image_Cursor", L"Image");
    const auto rows = frame.array(root, L"Rooms", 4);
    const auto focus = frame.integer(root, L"CursorMoveX");
    if (!cursor || !rows || !focus || *focus < 0 || *focus >= 4 ||
        !shown_child(frame, root, L"WD_RoomFocus", L"WD_RoomFocus_C") ||
        !at_destination(frame, root, *cursor, L"DestinationCursorPos")) return pending();
    const auto room = row_at(frame, *rows, *focus, root, L"WD_Room_C");
    constexpr const wchar_t* room_fields[]{L"WD_Room01", L"WD_Room02", L"WD_Room03", L"WD_Room04"};
    if (!room || frame.field(root, room_fields[*focus]) != room) return pending();
    auto label = drawn_text(frame, *room, L"TextBlock_TypeNo");
    if (label.empty()) return pending();
    auto screen = drawn_text(frame, root, L"TextBlock_Title");
    if (screen.empty()) screen = tr(L"Lobby");
    auto out = selection(frame, *room, std::move(screen), std::move(label));
    append(out.value, drawn_text(frame, *room, L"TextBlock_inBattle"));
    append(out.value, drawn_text(frame, *room, L"TextBlock_inTraining"));
    append(out.value, drawn_text(frame, *room, L"TextBlock_1PID"));
    append(out.value, drawn_text(frame, *room, L"TextBlock_2PID"));
    captioned(out.value, L"Host {0}", drawn_text(frame, root, L"TextBlock_HostID"));
    captioned(out.value, L"Lobby ID {0}", drawn_text(frame, root, L"TextBlock_UniqueID"));
    out.help = visible_guide(frame, root, L"WD_LineHelp");
    return out;
}

bool attached_popup(Frame& frame, Address owner, Address child) {
    const auto panel = frame.field(owner, L"CanvasPanel_Popup");
    if (!panel || frame.field(child, L"ParentCanvas") != panel || !frame.shown(child)) return false;
    // PressBtn forwards PopupCanvas to Activate. When unset, DuckAddToViewport
    // uses the viewport path, while PopupItem still establishes ownership.
    if (!*panel) return viewport_root(frame, child);
    return frame.u.is_a(*panel, L"CanvasPanel") && frame.within(*panel, owner) &&
           widget_geometry::slot_of(frame, child, *panel).has_value();
}

std::optional<Address> open_popup(Frame& frame, Address owner, const wchar_t* type) {
    // WD_MenuBase.PopupUpdate sends input to the first non-hidden PopupItem.
    if (!active(frame, owner)) return {};
    const auto children = frame.array(owner, L"PopupItem", 32);
    if (!children) return {};
    for (int i = 0; i < children->count; ++i) {
        const auto child = frame.get<Address>(children->data + static_cast<Address>(i) * 8);
        if (!child || !*child || !frame.retain(*child)) return {};
        const auto state = tick_state(frame, *child);
        if (state == std::optional<int>(0)) continue;
        if (state != std::optional<int>(2) || !exact(frame, *child, type) ||
            !attached_popup(frame, owner, *child)) return {};
        return child;
    }
    return {};
}

std::optional<MenuSnapshot> title_surface(Frame& frame, Address flow) {
    if (frame.boolean(flow, L"IsTravel") != std::optional<bool>(false)) return {};
    const auto main = frame.field(flow, L"WD_MainMenu");
    if (!main || !*main || frame.field(flow, L"InputObject") != main ||
        !exact(frame, *main, L"WD_MainMenu_C") || !viewport_root(frame, *main) || !frame.shown(*main)) return {};
    const auto active_menu = frame.field(*main, L"ActiveMenu");
    if (!active_menu || !*active_menu || !frame.shown(*active_menu)) return {};
    if (exact(frame, *active_menu, L"WD_ProfCardSpawn_C") &&
        frame.field(*main, L"WD_ProfCardSpawn_159") == active_menu) return profile_card(frame, *active_menu);
    if (!exact(frame, *active_menu, L"WD_MMVersus_C")) return {};

    const auto online = open_popup(frame, *active_menu, L"WD_OnlineMenu_C");
    if (!online) return {};
    const auto lobby = open_popup(frame, *online, L"WD_LobbyMenu_C");
    if (!lobby) return {};
    const auto search = open_popup(frame, *lobby, L"WD_LobbySearch_C");
    if (!search || frame.boolean(*search, L"IDInputActive") != std::optional<bool>(true)) return {};
    const auto form = frame.field(*search, L"Popup_IDInput");
    if (!form || !*form || !exact(frame, *form, L"WD_LobbyIDInput_C") ||
        !attached_popup(frame, *search, *form) || !active(frame, *form) ||
        !shown_child(frame, *form, L"EditableText_132", L"EditableText")) return pending();
    // EditableText.Text and FormText can lag Slate's editable layout while typing.
    auto caption = drawn_text(frame, *form, L"TextBlock_Caption");
    if (caption.empty()) return pending();
    auto out = selection(frame, *form, {}, std::move(caption));
    out.help = visible_guide(frame, *form);
    return out;
}

struct Route {
    const wchar_t* flow;
    const wchar_t* field;
    const wchar_t* type;
    const wchar_t* phase;
    int selected_phase;
    MenuSnapshot (*sample)(Frame&, Address);
};

const Route routes[]{
    {L"GF_Lobby_C", L"LobbySearchResult", L"WD_LobbySearchResult_C", L"LobbyFlowState", lobby_search_flow, lobby_results},
    {L"GF_Lobby_C", L"LobbyMain", L"WD_LobbyMain_C", L"LobbyFlowState", lobby_main_flow, lobby_room},
    {L"GF_Ranking_C", L"Rank", L"WD_RankingMain_C", nullptr, 0, rankings},
    {L"GF_Replay_C", L"ReplayMain", L"WD_ReplayMain_C", L"State", replay_select_data, replays}
};
}

MenuSnapshot online_menus(Unreal& unreal, Address world, const std::function<std::optional<int>(Address)>& visibility) {
    if (!world) return {};
    Frame frame{unreal, visibility, {}, {}, {}};
    std::optional<MenuSnapshot> result;
    for (const auto* family : {L"GF_Lobby_C", L"GF_Ranking_C", L"GF_Replay_C", L"GF_Title_C"}) {
        const bool title = std::wstring_view(family) == L"GF_Title_C";
        for (const auto& flow : unreal.instances(family)) {
            if (!unreal.alive(flow) || !in_world(frame, flow.address, world)) continue;
            if (title) {
                if (auto surface = title_surface(frame, flow.address)) {
                    if (result) return pending();
                    result = std::move(surface);
                }
                continue;
            }
            for (const auto& route : routes) {
                if (std::wstring_view(family) != route.flow) continue;
                const auto widget = frame.field(flow.address, route.field);
                if (!widget || !*widget || !exact(frame, *widget, route.type) || !frame.shown(*widget)) continue;
                if (result) return pending();
                const bool selected = !route.phase ||
                                      frame.integer(flow.address, route.phase) == std::optional<int>(route.selected_phase);
                result = selected && viewport_root(frame, *widget) && active(frame, *widget)
                         ? route.sample(frame, *widget) : pending();
            }
        }
    }
    if (!result) return {};
    return current_world(frame, world) && frame.finish() ? *result : pending();
}
}
