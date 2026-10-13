#if defined(TILES)
#    include "action.h"
#    include "avatar.h"
#    include "calendar.h"
#    include "catch/catch.hpp"
#    include "controller.h"
#    include "filesystem.h"
#    include "game.h"
#    include "input.h"
#    include "item.h"
#    include "json.h"
#    include "map/map.h"
#    include "map_helpers.h"
#    include "path_info.h"
#    include "player_helpers.h"

#    include <fstream>
#    include <set>
TEST_CASE("controller wheel cancellation and category changes are modal", "[controller]") {
    auto state = gamepad::radial_state{};
    CHECK(state.move(0, gamepad::direction::N));
    CHECK_FALSE(state.move(1, gamepad::direction::S));
    CHECK(state.owner == 0);
    state.cycle(1);
    CHECK(state.page == 1);
    CHECK_FALSE(state.move(0, gamepad::direction::E));
    CHECK(state.release().dir == gamepad::direction::NONE);
    state.move(1, gamepad::direction::S);
    state.cancel();
    CHECK_FALSE(state.move(0, gamepad::direction::N));
    CHECK(state.release().dir == gamepad::direction::NONE);
    CHECK_FALSE(state.blocked);
    state.move(0, gamepad::direction::N);
    state.cycle(-1);
    state.move(0, gamepad::direction::NONE);
    state.move(0, gamepad::direction::E);
    CHECK(state.release().dir == gamepad::direction::E);
}
TEST_CASE("controller native symbols and menu navigation", "[controller]") {
    CHECK(inp_mngr.get_keycode("UP") == KEY_UP);
    CHECK(inp_mngr.get_keycode("A") == 'A');
    CHECK(inp_mngr.get_keycode("PAD_A") == PAD_A);
    CHECK(gamepad::control_symbol(PAD_RS) == "Ⓡ");
    CHECK(gamepad::control_symbol(PAD_ALT_A) == "LT Ⓐ");
    CHECK(gamepad::control_symbol(PAD_LEFT) == "←");
    CHECK(gamepad::event_for_control(PAD_A, false, true)
          == input_event('\n', input_event_t::keyboard));
    CHECK(gamepad::event_for_control(PAD_B, false, true)
          == input_event(KEY_ESCAPE, input_event_t::keyboard));
    CHECK(gamepad::event_for_control(PAD_START, true, true)
          == input_event(PAD_ALT_START, input_event_t::gamepad));
}
TEST_CASE(
    "controller hints follow actual BN bindings and wheel actions are unique", "[controller]") {
    auto ctxt = input_context("DEFAULTMODE");
    for (const auto* action :
         {"main_menu", "look", "peek", "inventory", "advinv", "listitems", "HELP_KEYBINDINGS",
          "examine", "craft", "map"}) {
        ctxt.register_action(action);
    }
    CHECK(ctxt.input_to_action(gamepad::event_for_control(PAD_RS, false, false)) == "look");
    CHECK(ctxt.input_to_action(gamepad::event_for_control(PAD_RS, true, false)) == "listitems");
    CHECK_FALSE(gamepad::action_hint(ctxt, "look").empty());
    auto actions = std::set<std::string>{};
    for (auto page = 0; page < 4; ++page) {
        for (const auto& entry : gamepad::radial_entries(ctxt, page)) {
            if (entry.available) {
                CHECK(actions.insert(entry.action).second);
                CHECK(entry.action != "look");
                CHECK(entry.action != "listitems");
            }
        }
    }
    CHECK(actions.contains("peek"));
    CHECK(actions.contains("advinv"));
    auto main_menu = input_context("MAIN_MENU");
    main_menu.register_cardinal();
    main_menu.register_action("HELP_KEYBINDINGS");
    CHECK(main_menu.input_to_action(gamepad::event_for_control(PAD_UP, false, true)) == "UP");
    CHECK(main_menu.input_to_action(gamepad::event_for_control(PAD_LS_UP, false, true)) == "UP");
    CHECK(
        main_menu.input_to_action(gamepad::event_for_control(PAD_LS_DOWN, false, true)) == "DOWN");
    CHECK(
        main_menu.input_to_action(gamepad::event_for_control(PAD_LS_LEFT, false, true)) == "LEFT");
    CHECK(main_menu.input_to_action(gamepad::event_for_control(PAD_LS_RIGHT, false, true))
          == "RIGHT");
    CHECK(gamepad::event_for_control(PAD_LS_UP, false, false)
          == input_event(PAD_LS_UP, input_event_t::gamepad));
    CHECK(main_menu.input_to_action(gamepad::event_for_control(PAD_START, true, true))
          == "HELP_KEYBINDINGS");
    auto direction = input_context("DEFAULTMODE");
    direction.register_directions();
    direction.register_action("QUIT");
    CHECK(gamepad::uses_menu(direction));
    CHECK(direction.input_to_action(
              gamepad::event_for_control(PAD_B, false, gamepad::uses_menu(direction)))
          == "QUIT");
    CHECK(direction.input_to_action(
              gamepad::event_for_control(PAD_RS_UP, false, gamepad::uses_menu(direction)))
          == "UP");
    auto items = input_context("LIST_ITEMS");
    items.register_action("CONFIRM");
    items.register_action("QUIT");
    CHECK(items.input_to_action(gamepad::event_for_control(PAD_A, false, true)) == "CONFIRM");
    CHECK(items.input_to_action(gamepad::event_for_control(PAD_B, false, true)) == "QUIT");
    for (const auto& hint : gamepad::context_hints(items)) {
        CHECK((hint.action == "CONFIRM" || hint.action == "QUIT"));
    }
}


TEST_CASE("controller world direction preview does not move or spend a turn", "[controller]") {
    clear_avatar();
    clear_map();
    const auto position = get_avatar().bub_pos();
    const auto turn = calendar::turn;
    REQUIRE(SDL_InitSubSystem(SDL_INIT_GAMEPAD));
    auto desc = SDL_VirtualJoystickDesc{};
    SDL_INIT_INTERFACE(&desc);
    desc.type = SDL_JOYSTICK_TYPE_GAMEPAD;
    desc.naxes = SDL_GAMEPAD_AXIS_COUNT;
    desc.nbuttons = SDL_GAMEPAD_BUTTON_COUNT;
    desc.name = "Controller parity test";
    const auto id = SDL_AttachVirtualJoystick(&desc);
    REQUIRE(id != 0);
    auto* joystick = SDL_OpenJoystick(id);
    REQUIRE(joystick);
    gamepad::quit();
    gamepad::init();
    REQUIRE(gamepad::is_active());
    auto ctxt = input_context("DEFAULTMODE");
    ctxt.register_action("main_menu");
    const auto context = gamepad::context_scope(ctxt);
    const auto directions = std::array<point, 8>{
        point_north, point_north_east, point_east, point_south_east,
        point_south, point_south_west, point_west, point_north_west};
    for (auto i = size_t{0}; i < directions.size(); ++i) {
        const auto axes = directions[i] * 26000;
        REQUIRE(SDL_SetJoystickVirtualAxis(joystick, SDL_GAMEPAD_AXIS_LEFTX, axes.x));
        REQUIRE(SDL_SetJoystickVirtualAxis(joystick, SDL_GAMEPAD_AXIS_LEFTY, axes.y));
        SDL_UpdateJoysticks();
        SDL_FlushEvent(SDL_EVENT_GAMEPAD_AXIS_MOTION);
        auto event = SDL_Event{};
        event.type = SDL_EVENT_GAMEPAD_AXIS_MOTION;
        event.gaxis.which = id;
        event.gaxis.axis = SDL_GAMEPAD_AXIS_LEFTY;
        CHECK(gamepad::handle_event(event));
        CHECK(gamepad::get_left_stick_direction() == static_cast<gamepad::direction>(i + 1));
        CHECK(gamepad::world_preview_visible());
        CHECK(gamepad::take_world_refresh());
        CHECK(get_avatar().bub_pos() == position);
        CHECK(calendar::turn == turn);
    }
    {
        auto menu = input_context("TARGET");
        const auto nested = gamepad::context_scope(menu);
        CHECK_FALSE(gamepad::world_preview_visible());
    }
    CHECK(gamepad::world_preview_visible());
    gamepad::quit();
    SDL_CloseJoystick(joystick);
    SDL_DetachVirtualJoystick(id);
    CHECK_FALSE(gamepad::world_preview_visible());
}

TEST_CASE(
    "controller selected tile is consumed once and respects native target validation",
    "[controller]") {
    clear_avatar();
    clear_map();
    const auto position = get_avatar().bub_pos();
    const auto turn = calendar::turn;
    for (auto i = 1; i <= 8; ++i) {
        const auto dir = static_cast<gamepad::direction>(i);
        const auto target = gamepad::direction_offset(dir);
        const auto scope = gamepad::target_scope(dir);
        CHECK(gamepad::has_target());
        CHECK(choose_direction("test") == target);
        CHECK_FALSE(gamepad::has_target());
        CHECK_FALSE(gamepad::take_target());
        CHECK(get_avatar().bub_pos() == position);
        CHECK(calendar::turn == turn);
    }
    {
        const auto outer = gamepad::target_scope(gamepad::direction::E);
        {
            const auto inner = gamepad::target_scope(gamepad::direction::N);
            CHECK(gamepad::take_target() == tripoint_rel_ms::north());
        }
        CHECK(gamepad::take_target() == tripoint_rel_ms::east());
    }
    CHECK_FALSE(gamepad::take_target());
    const auto east = position + tripoint_rel_ms::east();
    const auto north = position + tripoint_rel_ms::north();
    const auto allowed = [&](const tripoint_bub_ms& p) { return p == east; };
    {
        const auto scope = gamepad::target_scope(gamepad::direction::E);
        CHECK(choose_adjacent_highlight("test", "invalid", allowed, false) == east);
    }
    {
        const auto scope = gamepad::target_scope(gamepad::direction::N);
        // A different valid tile must never override the player's selected tile.
        CHECK_FALSE(choose_adjacent_highlight("test", "invalid", allowed, false));
        CHECK_FALSE(gamepad::take_target());
    }
    CHECK(north != east);
    CHECK(get_avatar().bub_pos() == position);
    CHECK(calendar::turn == turn);
}

TEST_CASE("controller interaction discovers actions for the exact adjacent tile", "[controller]") {
    clear_avatar();
    clear_map();
    auto& here = get_map();
    const auto position = get_avatar().bub_pos();
    const auto target = position + tripoint_rel_ms::east();
    here.furn_set(target, furn_id("f_null"));
    here.ter_set(target, ter_id("t_floor"));
    here.add_item_or_charges(target, item::spawn("rock"));
    const auto actions = gamepad::interact_actions(target);
    CHECK_FALSE(std::ranges::contains(actions, ACTION_OPEN));
    CHECK(std::ranges::contains(actions, ACTION_PICKUP));
    CHECK(std::ranges::contains(actions, ACTION_EXAMINE));
    CHECK_FALSE(std::ranges::contains(actions, ACTION_CLOSE));
    here.ter_set(target, ter_id("t_door_c"));
    CHECK(std::ranges::contains(gamepad::interact_actions(target), ACTION_OPEN));
    CHECK(gamepad::interact_actions(position + tripoint_rel_ms(8, 0, 0)).empty());
    CHECK(gamepad::interact_actions(position + tripoint_rel_ms::above()).empty());
}
TEST_CASE(
    "controller nested consumers route confirm, cancel and eight directions", "[controller]") {
    for (const auto* category :
         {"TARGET", "LOOK", "ADVANCED_INVENTORY", "PICKUP", "CRAFTING", "CONSTRUCTION", "OVERMAP",
          "DIALOGUE_CHOOSE_RESPONSE", "STRING_INPUT"}) {
        auto ctxt = input_context(category);
        ctxt.register_directions();
        ctxt.register_action("CONFIRM");
        ctxt.register_action("QUIT");
        CHECK(gamepad::uses_menu(ctxt));
        if (ctxt.controller_category() != "TARGET" && ctxt.controller_category() != "LOOK"
            && ctxt.controller_category() != "OVERMAP") {
            continue;
        }
        struct route {
            int code;
            const char* expected;
        };
        for (const auto& [code, expected] : std::array<route, 8>{
                 {{PAD_RS_UP, "UP"},
                  {PAD_RS_UP_RIGHT, "RIGHTUP"},
                  {PAD_RS_RIGHT, "RIGHT"},
                  {PAD_RS_DOWN_RIGHT, "RIGHTDOWN"},
                  {PAD_RS_DOWN, "DOWN"},
                  {PAD_RS_DOWN_LEFT, "LEFTDOWN"},
                  {PAD_RS_LEFT, "LEFT"},
                  {PAD_RS_UP_LEFT, "LEFTUP"}}}) {
            CHECK(ctxt.input_to_action(gamepad::event_for_control(code, false, true)) == expected);
        }
    }
    auto text = input_context("STRING_INPUT");
    for (const auto* action :
         {"TEXT.CONFIRM", "TEXT.QUIT", "TEXT.LEFT", "TEXT.RIGHT", "TEXT.UP", "TEXT.DOWN"}) {
        text.register_action(action);
    }
    CHECK(text.input_to_action(gamepad::event_for_control(PAD_A, false, true)) == "TEXT.CONFIRM");
    CHECK(text.input_to_action(gamepad::event_for_control(PAD_B, false, true)) == "TEXT.QUIT");
    CHECK(text.input_to_action(gamepad::event_for_control(PAD_LEFT, false, true)) == "TEXT.LEFT");
    CHECK(text.input_to_action(gamepad::event_for_control(PAD_RIGHT, false, true)) == "TEXT.RIGHT");
    auto gameplay = input_context("DEFAULTMODE");
    gameplay.register_action("grab");
    CHECK_FALSE(gamepad::uses_menu(gameplay));
    auto direction = input_context("DEFAULTMODE");
    direction.register_directions();
    direction.register_action("QUIT");
    CHECK(gamepad::uses_menu(direction));
    CHECK(direction.input_to_action(gamepad::event_for_control(PAD_B, true, true)) == "QUIT");
    auto dialogue = input_context("DIALOGUE_CHOOSE_RESPONSE");
    dialogue.register_action("CONFIRM");
    dialogue.register_action("QUIT");
    dialogue.register_action("LOOK_AT");
    dialogue.register_action("SIZE_UP_STATS");
    dialogue.register_action("YELL");
    dialogue.register_action("CHECK_OPINION");
    CHECK(dialogue.input_to_action(gamepad::event_for_control(PAD_A, false, true)) == "CONFIRM");
    CHECK(dialogue.input_to_action(gamepad::event_for_control(PAD_B, false, true)) == "QUIT");
    auto actions = std::set<std::string>{};
    for (auto page = 0; page < 4; ++page) {
        for (const auto& entry : gamepad::radial_entries(dialogue, page)) {
            if (entry.available) { actions.insert(entry.action); }
        }
    }
    CHECK(actions.contains("LOOK_AT"));
    CHECK(actions.contains("SIZE_UP_STATS"));
    CHECK(actions.contains("YELL"));
    CHECK(actions.contains("CHECK_OPINION"));
}

TEST_CASE("controller repeat schedules are independent and match CDDA cadence", "[controller]") {
    auto repeats = gamepad::repeat_queue{};
    const auto rt = input_event(PAD_RT, input_event_t::gamepad);
    const auto rs = input_event(PAD_RS_UP, input_event_t::gamepad);
    repeats.start({.control = PAD_RT, .event = rt, .now = 1000});
    repeats.start({.control = PAD_RS, .event = rs, .now = 1050});
    CHECK_FALSE(repeats.take_due(1399));
    const auto initial = repeats.take_due(1400);
    REQUIRE(initial);
    CHECK(initial->control == PAD_RT);
    CHECK_FALSE(repeats.take_due(1449));
    const auto first = repeats.take_due(1450);
    const auto second = repeats.take_due(1450);
    REQUIRE(first);
    REQUIRE(second);
    CHECK(first->control != second->control);
    repeats.stop(PAD_RS);
    const auto still_moving = repeats.take_due(1500);
    REQUIRE(still_moving);
    CHECK(still_moving->control == PAD_RT);
    repeats.clear();
    CHECK_FALSE(repeats.take_due(5000));
}
TEST_CASE(
    "controller action palette reaches registered actions across binding contexts",
    "[controller]") {
    auto checked = size_t{0};
    auto definitions = std::map<std::string, std::set<std::string>>{};
    for (const auto& path : get_files_from_path(".json", PATH_INFO::keybindingsdir(), true)) {
        auto file = std::ifstream(path);
        REQUIRE(file.good());
        auto json = JsonIn(file);
        for (const JsonObject entry : json.get_array()) {
            entry.allow_omitted_members();
            definitions[entry.get_string("category", "default")].insert(entry.get_string("id"));
        }
    }
    for (const auto& [category, actions] : definitions) {
        auto ctxt = input_context(category);
        for (const auto& action : actions) { ctxt.register_action(action); }
        for (const auto& action : actions) {
            if (action == "ANY_INPUT" || action == "COORDINATE" || action == "MOUSE_MOVE"
                || action == "HELP_KEYBINDINGS") {
                continue;
            }
            CAPTURE(category, action);
            REQUIRE(gamepad::queue_action(ctxt, action));
            CHECK(gamepad::take_action(ctxt) == action);
            CHECK(gamepad::take_action(ctxt).empty());
            checked += 1;
        }
        CHECK_FALSE(gamepad::queue_action(ctxt, "not_a_registered_action"));
    }
    CHECK(checked > 500);
    auto first = input_context("LOOK");
    first.register_action("QUIT");
    auto second = input_context("TARGET");
    second.register_action("QUIT");
    REQUIRE(gamepad::queue_action(first, "QUIT"));
    CHECK(gamepad::take_action(second).empty());
    CHECK(gamepad::take_action(first).empty());
}
TEST_CASE(
    "controller trigger repeat follows centering and focus never resumes held input",
    "[controller]") {
    REQUIRE(SDL_InitSubSystem(SDL_INIT_GAMEPAD));
    auto desc = SDL_VirtualJoystickDesc{};
    SDL_INIT_INTERFACE(&desc);
    desc.type = SDL_JOYSTICK_TYPE_GAMEPAD;
    desc.naxes = SDL_GAMEPAD_AXIS_COUNT;
    desc.nbuttons = SDL_GAMEPAD_BUTTON_COUNT;
    desc.name = "Controller focus test";
    const auto id = SDL_AttachVirtualJoystick(&desc);
    REQUIRE(id != 0);
    auto* joystick = SDL_OpenJoystick(id);
    REQUIRE(joystick);
    gamepad::quit();
    gamepad::init();
    REQUIRE(gamepad::is_active());
    auto ctxt = input_context("DEFAULTMODE");
    ctxt.register_directions();
    ctxt.register_action("pause");
    ctxt.register_action("main_menu");
    const auto scope = gamepad::context_scope(ctxt);
    const auto axis = [&](SDL_GamepadAxis code, Sint16 value) {
        const auto raw =
            code == SDL_GAMEPAD_AXIS_LEFT_TRIGGER || code == SDL_GAMEPAD_AXIS_RIGHT_TRIGGER
                ? static_cast<Sint16>(static_cast<int>(value) * 2 - 32768)
                : value;
        REQUIRE(SDL_SetJoystickVirtualAxis(joystick, code, raw));
        SDL_UpdateJoysticks();
        SDL_FlushEvent(SDL_EVENT_GAMEPAD_AXIS_MOTION);
        auto event = SDL_Event{};
        event.type = SDL_EVENT_GAMEPAD_AXIS_MOTION;
        event.gaxis.which = id;
        event.gaxis.axis = code;
        event.gaxis.value = value;
        REQUIRE(gamepad::handle_event(event));
    };
    axis(SDL_GAMEPAD_AXIS_LEFTY, -26000);
    axis(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, 26000);
    SDL_Delay(410);
    CHECK(ctxt.handle_input(0) == "UP");
    axis(SDL_GAMEPAD_AXIS_LEFTY, 0);
    SDL_Delay(60);
    CHECK(ctxt.handle_input(0) == "pause");
    // Leave release queued until after the repeat deadline. It takes priority
    // over a due repeat, even when the previous world turn took over 50ms.
    REQUIRE(SDL_SetJoystickVirtualAxis(joystick, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, -32768));
    SDL_UpdateJoysticks();
    SDL_Delay(60);
    CHECK(ctxt.handle_input(1) == "TIMEOUT");
    axis(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, 26000);
    axis(SDL_GAMEPAD_AXIS_LEFTY, -26000);
    axis(SDL_GAMEPAD_AXIS_LEFT_TRIGGER, 26000);
    axis(SDL_GAMEPAD_AXIS_RIGHTX, 26000);
    CHECK(gamepad::radial_input_captured());
    auto event = SDL_Event{};
    event.type = SDL_EVENT_WINDOW_FOCUS_LOST;
    CHECK_FALSE(gamepad::handle_event(event));
    CHECK_FALSE(gamepad::tick());
    CHECK_FALSE(gamepad::is_alt_held());
    CHECK_FALSE(gamepad::radial_input_captured());
    CHECK_FALSE(gamepad::world_preview_visible());
    event.type = SDL_EVENT_WINDOW_FOCUS_GAINED;
    CHECK_FALSE(gamepad::handle_event(event));
    axis(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, 26000);
    axis(SDL_GAMEPAD_AXIS_LEFT_TRIGGER, 26000);
    axis(SDL_GAMEPAD_AXIS_LEFTY, -26000);
    CHECK_FALSE(gamepad::tick());
    CHECK_FALSE(gamepad::is_alt_held());
    CHECK(gamepad::get_left_stick_direction() == gamepad::direction::NONE);
    axis(SDL_GAMEPAD_AXIS_LEFT_TRIGGER, 0);
    axis(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, 0);
    axis(SDL_GAMEPAD_AXIS_LEFTY, 0);
    axis(SDL_GAMEPAD_AXIS_RIGHTX, 0);
    axis(SDL_GAMEPAD_AXIS_LEFTY, -26000);
    axis(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, 26000);
    SDL_Delay(410);
    CHECK(ctxt.handle_input(0) == "UP");
    event.type = SDL_EVENT_WINDOW_MINIMIZED;
    CHECK_FALSE(gamepad::handle_event(event));
    CHECK_FALSE(gamepad::tick());
    gamepad::quit();
    SDL_CloseJoystick(joystick);
    SDL_DetachVirtualJoystick(id);
}

#endif
