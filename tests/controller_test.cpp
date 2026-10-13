#if defined(TILES)
#    include "catch/catch.hpp"
#    include "controller.h"
#    include "input.h"

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
#endif
