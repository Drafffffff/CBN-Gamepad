#if defined(TILES)
#include "controller.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <set>
#include "catacharset.h"
#include "options.h"
#include "sdltiles.h"
#include "string_formatter.h"
#include "translations.h"
#include "ui.h"
namespace gamepad
{
namespace
{
auto active_context = static_cast<const input_context *>( nullptr );
auto controller = static_cast<SDL_Gamepad *>( nullptr );
auto radial = radial_state{};
auto alternate = false;
auto left_dir = direction::NONE;
auto right_dir = direction::NONE;
auto require_center = std::array<bool, 2> {};
auto trigger_down = std::array<bool, 2> {};
auto repeat_event = input_event{};
auto repeat_at = Uint64{0};
auto repeat_button = -1;
auto pending_action = std::string{};
auto pending_context = static_cast<const input_context *>( nullptr );
auto keyboard_help = false;
constexpr auto action_event = 0x7fff;
auto get_alt_button( int base_code ) -> int
{
    switch( base_code ) {
        case PAD_A:
            return PAD_ALT_A;
        case PAD_B:
            return PAD_ALT_B;
        case PAD_X:
            return PAD_ALT_X;
        case PAD_Y:
            return PAD_ALT_Y;
        case PAD_LB:
            return PAD_ALT_LB;
        case PAD_RB:
            return PAD_ALT_RB;
        case PAD_RT:
            return PAD_ALT_RT;
        case PAD_LS:
            return PAD_ALT_LS;
        case PAD_RS:
            return PAD_ALT_RS;
        case PAD_UP:
            return PAD_ALT_UP;
        case PAD_DOWN:
            return PAD_ALT_DOWN;
        case PAD_LEFT:
            return PAD_ALT_LEFT;
        case PAD_RIGHT:
            return PAD_ALT_RIGHT;
        case PAD_START:
            return PAD_ALT_START;
        case PAD_BACK:
            return PAD_ALT_BACK;
        default:
            return base_code;
    }
}

} // namespace

auto radial_state::move( int stick, direction dir ) -> bool
{
    if( blocked || ( open() && owner != stick ) ) {
        return false;
    }
    if( !open() ) {
        if( dir == direction::NONE ) {
            return false;
        }
        owner = stick;
    }
    if( await_center && dir != direction::NONE ) {
        return false;
    }
    if( dir == direction::NONE ) {
        await_center = false;
    }
    const auto changed = selected != dir;
    selected = dir;
    return changed;
}

auto radial_state::cycle( int delta ) -> void
{
    page = ( page + delta + 4 ) % 4;
    selected = direction::NONE;
    await_center = true;
}

auto radial_state::cancel() -> void
{
    owner = -1;
    selected = direction::NONE;
    await_center = false;
    blocked = true;
}

auto radial_state::release() -> radial_selection
{
    const auto result = radial_selection{ .page = page, .dir = open() && !blocked && !await_center ? selected : direction::NONE };
    owner = -1;
    selected = direction::NONE;
    await_center = false;
    blocked = false;
    return result;
}

auto radial_page_name( int page ) -> std::string
{
    const auto names = std::array<const char *, 4> { translate_marker( "Explore" ),
        translate_marker( "Items" ), translate_marker( "Survival" ), translate_marker( "Information" )
                                                   };
    return _( names[page % 4] );
}

auto radial_entries( const input_context &ctxt, int page ) -> std::array<radial_entry, 8>
{
    // Action metadata is shared across contexts; availability comes exclusively
    // from the consumer's registered actions. No screen-specific prompt tables.
    const auto actions = std::array<std::array<const char *, 8>, 4> {{
            {{ "look", "examine", "pickup", "chat", "open", "close", "control_vehicle", "action_menu" }},
            {{ "inventory", "advinv", "apply", "reload_item", "unload", "wear", "wield", "drop" }},
            {{ "craft", "construct", "disassemble", "read", "wait", "sleep", "medical", "eat" }},
            {{ "map", "player_data", "bodystatus", "morale", "missions", "factions", "listitems", "HELP_KEYBINDINGS" }}
        }
    };
    struct categorized_action { const char *action; int page; };
    const auto extra = std::vector<categorized_action> {
        { .action = "CHOOSE_DESTINATION", .page = 0 }, { .action = "GO_TO_DESTINATION", .page = 0 }, { .action = "CENTER_ON_DESTINATION", .page = 0 },
        { .action = "TRAVEL_TO", .page = 0 }, { .action = "CENTER", .page = 0 }, { .action = "SEARCH", .page = 0 }, { .action = "CREATE_POINT_OF_INTEREST", .page = 0 },
        { .action = "EXAMINE", .page = 0 }, { .action = "LOOK", .page = 0 }, { .action = "peek", .page = 0 },
        { .action = "SORT", .page = 1 }, { .action = "FILTER", .page = 1 }, { .action = "RESET_FILTER", .page = 1 }, { .action = "INVENTORY_FILTER", .page = 1 },
        { .action = "MOVE_SINGLE_ITEM", .page = 1 }, { .action = "MOVE_ITEM_STACK", .page = 1 }, { .action = "MOVE_ALL_ITEMS", .page = 1 },
        { .action = "TOGGLE_FAVORITE", .page = 1 },
        { .action = "MEDICAL_MENU", .page = 2 }, { .action = "TOGGLE_OVERMAP_WEATHER", .page = 2 }, { .action = "TOGGLE_HORDES", .page = 2 },
        { .action = "MARK_DANGER", .page = 2 },
        { .action = "zoom_in", .page = 3 }, { .action = "zoom_out", .page = 3 }, { .action = "CREATE_NOTE", .page = 3 }, { .action = "DELETE_NOTE", .page = 3 }, { .action = "LIST_NOTES", .page = 3 }, { .action = "MISSIONS", .page = 3 },
        { .action = "TOGGLE_MAP_NOTES", .page = 3 }, { .action = "TOGGLE_OVERLAYS", .page = 3 }, { .action = "TOGGLE_CITY_LABELS", .page = 3 },
        { .action = "TOGGLE_EXPLORED", .page = 3 }, { .action = "messages", .page = 3 }, { .action = "TOGGLE_LAND_USE_CODES", .page = 3 },
        { .action = "TOGGLE_MAP_REVEALS", .page = 3 }, { .action = "TOGGLE_FOREST_TRAILS", .page = 3 }, { .action = "TOGGLE_FAST_TRAVEL", .page = 0 },
        { .action = "zones", .page = 0 }, { .action = "auto_travel_mode", .page = 0 },
        { .action = "compare", .page = 1 }, { .action = "organize", .page = 1 }, { .action = "sort_armor", .page = 1 }, { .action = "insert", .page = 1 },
        { .action = "take_off", .page = 1 }, { .action = "unload_container", .page = 1 }, { .action = "item_action_menu", .page = 1 },
        { .action = "butcher", .page = 2 }, { .action = "workout", .page = 2 }, { .action = "bionics", .page = 2 }, { .action = "mutations", .page = 2 },
        { .action = "recraft", .page = 2 }, { .action = "long_craft", .page = 2 },
        { .action = "diary", .page = 3 }, { .action = "sky", .page = 3 }, { .action = "help", .page = 3 }, { .action = "open_options", .page = 3 }
    };
    auto result = std::array<radial_entry, 8> {};
    const auto registered = ctxt.get_registered_actions_copy();
    std::set<std::string> included;
    // The wheel complements actual direct controls, including LT chords. Resolve
    // through the same context routing so customized bindings also free slots.
    const auto direct_controls = std::array<int, 15> { PAD_A, PAD_B, PAD_X, PAD_Y,
        PAD_LB, PAD_RB, PAD_RT, PAD_LS, PAD_RS, PAD_START, PAD_BACK,
        PAD_UP, PAD_DOWN, PAD_LEFT, PAD_RIGHT
                                                     };
    for( bool alternate : { false, true } ) {
        for( const auto code : direct_controls ) {
            const auto event = event_for_control( code, alternate, uses_menu( ctxt ) );
            const auto &action = ctxt.input_to_action( event );
            if( action != "ERROR" ) {
                included.insert( action );
            }
        }
    }
    for( auto i = size_t{0}; i < result.size(); ++i ) {
        auto action = actions[page % 4][i];
        // With look available directly on R3, use its exploration slot for peek.
        // Consumers without peek (such as the overmap) retain look in this slot.
        if( page % 4 == 0 && i == 0 &&
            std::ranges::find( registered, "peek" ) != registered.end() ) {
            action = "peek";
        }
        if( !included.count( action ) &&
            std::ranges::find( registered, action ) != registered.end() ) {
            result[i] = { .action = action, .label = ctxt.get_action_name( action ), .available = true };
            included.insert( action );
        }
    }
    const auto append = [&]( const std::string & action ) {
        if( included.count( action ) ||
            std::ranges::find( registered, action ) == registered.end() ) {
            return;
        }
        const auto free = std::ranges::find_if( result, []( const radial_entry & entry ) {
            return !entry.available;
        } );
        if( free != result.end() ) {
            *free = { .action = action, .label = ctxt.get_action_name( action ), .available = true };
            included.insert( action );
        }
    };
    for( const auto &entry : extra ) {
        if( entry.page == page % 4 ) {
            append( entry.action );
        }
    }
    // Preserve other existing context wheel bindings (inventory, targeting, etc.).
    for( auto code = PAD_L_RADIAL_N; code <= PAD_R_RADIAL_NW; ++code ) {
        const auto &action = ctxt.input_to_action( input_event( code, input_event_t::gamepad ) );
        if( action == "ERROR" ) {
            continue;
        }
        auto known = false;
        for( const auto &group : actions ) {
            known |= std::ranges::find( group, action ) != group.end();
        }
        for( const auto &entry : extra ) {
            known |= action == entry.action;
        }
        if( !known && page == 0 ) {
            append( action );
        }
    }
    return result;
}

auto control_symbol( int code ) -> std::string
{
    if( code >= PAD_ALT_A && code <= PAD_ALT_BACK ) {
        const auto base = std::array<int, 16> { PAD_A, PAD_B, PAD_X, PAD_Y, PAD_RB,
                                                PAD_LB, PAD_LT, PAD_RT, PAD_LS, PAD_RS, PAD_UP, PAD_DOWN, PAD_LEFT,
                                                PAD_RIGHT, PAD_START, PAD_BACK
                                              };
        return "LT " + control_symbol( base[code - PAD_ALT_A] );
    }
    const auto directions = std::array<const char *, 8> { "↑", "↗", "→", "↘", "↓", "↙", "←", "↖" };
    if( code >= PAD_L_RADIAL_N && code <= PAD_R_RADIAL_NW ) {
        const auto index = code - PAD_L_RADIAL_N;
        return std::string( index < 8 ? "LT L" : "LT R" ) + directions[index % 8];
    }
    switch( code ) {
        case PAD_A:
            return "Ⓐ";
        case PAD_B:
            return "Ⓑ";
        case PAD_X:
            return "Ⓧ";
        case PAD_Y:
            return "Ⓨ";
        case PAD_LB:
            return "LB";
        case PAD_RB:
            return "RB";
        case PAD_LT:
            return "LT";
        case PAD_RT:
            return "RT";
        case PAD_LS:
            return "Ⓛ";
        case PAD_RS:
            return "Ⓡ";
        case PAD_START:
            return "☰";
        case PAD_BACK:
            return "▱";
        case PAD_UP:
            return "↑";
        case PAD_DOWN:
            return "↓";
        case PAD_LEFT:
            return "←";
        case PAD_RIGHT:
            return "→";
        case PAD_LS_UP:
            return "L↑ RT";
        case PAD_LS_UP_RIGHT:
            return "L↗ RT";
        case PAD_LS_RIGHT:
            return "L→ RT";
        case PAD_LS_DOWN_RIGHT:
            return "L↘ RT";
        case PAD_LS_DOWN:
            return "L↓ RT";
        case PAD_LS_DOWN_LEFT:
            return "L↙ RT";
        case PAD_LS_LEFT:
            return "L← RT";
        case PAD_LS_UP_LEFT:
            return "L↖ RT";
        case PAD_RS_UP:
            return "R↑";
        case PAD_RS_UP_RIGHT:
            return "R↗";
        case PAD_RS_RIGHT:
            return "R→";
        case PAD_RS_DOWN_RIGHT:
            return "R↘";
        case PAD_RS_DOWN:
            return "R↓";
        case PAD_RS_DOWN_LEFT:
            return "R↙";
        case PAD_RS_LEFT:
            return "R←";
        case PAD_RS_UP_LEFT:
            return "R↖";
        default:
            return "[" + std::to_string( code ) + "]";
    }
}

auto event_for_control( int code, bool alternate, bool menu ) -> input_event
{
    // This chord remains a native event even inside keyboard-driven menus.
    if( alternate && code == PAD_START ) {
        return input_event( PAD_ALT_START, input_event_t::gamepad );
    }
    if( menu ) {
        switch( code ) {
            case PAD_A:
                return input_event( '\n', input_event_t::keyboard );
            case PAD_B:
            case PAD_START:
            case PAD_BACK:
                return input_event( KEY_ESCAPE, input_event_t::keyboard );
            case PAD_LB:
                return input_event( KEY_BTAB, input_event_t::keyboard );
            case PAD_RB:
                return input_event( '\t', input_event_t::keyboard );
            default:
                break;
        }
        if( !alternate ) {
            switch( code ) {
                case PAD_UP:
                case PAD_LS_UP:
                case PAD_LS_UP_LEFT:
                case PAD_LS_UP_RIGHT:
                    return input_event( KEY_UP, input_event_t::keyboard );
                case PAD_DOWN:
                case PAD_LS_DOWN:
                case PAD_LS_DOWN_LEFT:
                case PAD_LS_DOWN_RIGHT:
                    return input_event( KEY_DOWN, input_event_t::keyboard );
                case PAD_LEFT:
                case PAD_LS_LEFT:
                    return input_event( KEY_LEFT, input_event_t::keyboard );
                case PAD_RIGHT:
                case PAD_LS_RIGHT:
                    return input_event( KEY_RIGHT, input_event_t::keyboard );
                case PAD_RS_UP:
                    return input_event( '8', input_event_t::keyboard );
                case PAD_RS_UP_RIGHT:
                    return input_event( '9', input_event_t::keyboard );
                case PAD_RS_RIGHT:
                    return input_event( '6', input_event_t::keyboard );
                case PAD_RS_DOWN_RIGHT:
                    return input_event( '3', input_event_t::keyboard );
                case PAD_RS_DOWN:
                    return input_event( '2', input_event_t::keyboard );
                case PAD_RS_DOWN_LEFT:
                    return input_event( '1', input_event_t::keyboard );
                case PAD_RS_LEFT:
                    return input_event( '4', input_event_t::keyboard );
                case PAD_RS_UP_LEFT:
                    return input_event( '7', input_event_t::keyboard );
                default:
                    break;
            }
        }
    }
    return input_event( alternate ? get_alt_button( code ) : code, input_event_t::gamepad );
}

context_scope::context_scope( const input_context &ctxt ) : previous( active_context )
{
    active_context = &ctxt;
    if( previous != &ctxt && is_active() ) { request_refresh(); }
}
context_scope::~context_scope() { active_context = previous; }
auto current_context() -> const input_context * { return active_context; } // *NOPAD*
auto is_active() -> bool { return controller != nullptr; }
auto is_alt_held() -> bool { return alternate; }
auto current_radial_page() -> int { return radial.page; }
auto radial_input_captured() -> bool { return radial.open() || radial.blocked; }
auto radial_cancelled() -> bool { return radial.blocked; }
auto is_radial_left_open() -> bool { return radial.owner == 0; }
auto is_radial_right_open() -> bool { return radial.owner == 1; }
auto get_radial_left_direction() -> direction { return radial.selected; }
auto get_left_stick_direction() -> direction { return left_dir; }
auto uses_menu( const input_context &ctxt ) -> bool
{
    // Direction prompts reuse DEFAULTMODE, but do not register the gameplay menu.
    const auto actions = ctxt.get_registered_actions_copy();
    return ctxt.controller_category() != "DEFAULTMODE" ||
           std::ranges::find( actions, "main_menu" ) == actions.end();
}
auto init() -> void
{
    if( !get_option<bool>( "ENABLE_JOYSTICK" ) ) { return; }
    SDL_InitSubSystem( SDL_INIT_GAMEPAD );
    auto count = 0;
    auto *ids = SDL_GetGamepads( &count );
    for( auto i = 0; i < count && !controller; ++i ) { controller = SDL_OpenGamepad( ids[i] ); }
    SDL_free( ids );
    SDL_SetGamepadEventsEnabled( true );
}
auto quit() -> void
{
    if( controller ) { SDL_CloseGamepad( controller ); }
    controller = nullptr;
    radial.release();
    alternate = false;
    left_dir = right_dir = direction::NONE;
    trigger_down.fill( false );
    require_center.fill( false );
    repeat_at = 0;
    pending_action.clear();
    pending_context = nullptr;
}
namespace
{
auto send( const input_event &event ) -> void { submit_event( event ); }
auto mapped( int code ) -> input_event
{
    return event_for_control( code, alternate, active_context && uses_menu( *active_context ) );
}
auto stick_code( direction dir, bool right ) -> int
{
    constexpr auto left_codes = std::array{ PAD_LS_UP, PAD_LS_UP_RIGHT, PAD_LS_RIGHT,
                                            PAD_LS_DOWN_RIGHT, PAD_LS_DOWN, PAD_LS_DOWN_LEFT, PAD_LS_LEFT, PAD_LS_UP_LEFT };
    constexpr auto right_codes = std::array{ PAD_RS_UP, PAD_RS_UP_RIGHT, PAD_RS_RIGHT,
        PAD_RS_DOWN_RIGHT, PAD_RS_DOWN, PAD_RS_DOWN_LEFT, PAD_RS_LEFT, PAD_RS_UP_LEFT };
    const auto slot = static_cast<int>( dir ) - 1;
    return slot < 0 ? -1 : ( right ? right_codes[slot] : left_codes[slot] );
}
auto axis_direction( int stick ) -> direction
{
    const auto x = SDL_GetGamepadAxis( controller,
                                       stick == 0 ? SDL_GAMEPAD_AXIS_LEFTX : SDL_GAMEPAD_AXIS_RIGHTX );
    const auto y = SDL_GetGamepadAxis( controller,
                                       stick == 0 ? SDL_GAMEPAD_AXIS_LEFTY : SDL_GAMEPAD_AXIS_RIGHTY );
    if( std::hypot( x, y ) < 16000 ) { return direction::NONE; }
    const auto angle = std::atan2( static_cast<double>( x ), static_cast<double>( -y ) );
    const auto slot = ( static_cast<int>( std::floor( angle * 4.0 / std::acos(
            -1.0 ) + 0.5 ) ) + 8 ) % 8;
    return static_cast<direction>( slot + 1 );
}
auto start_repeat( int button, const input_event &event ) -> void
{
    repeat_button = button;
    repeat_event = event;
    repeat_at = SDL_GetTicks() + 400;
    send( event );
}
}
auto tick() -> bool
{
    if( !is_active() || alternate || !repeat_at || SDL_GetTicks() < repeat_at ) { return false; }
    repeat_at = SDL_GetTicks() + 100;
    send( repeat_button == PAD_RT &&
          left_dir != direction::NONE ? mapped( stick_code( left_dir, false ) ) : repeat_event );
    return true;
}
auto handle_event( const SDL_Event &event ) -> bool
{
    if( std::getenv( "CBN_GAMEPAD_TRACE" ) && event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN ) {
        std::fprintf( stderr,
                      "CBN controller event: type=%u id=%u selected=%u button=%u focus=%d age_ms=%.1f\n",
                      event.type, event.gbutton.which, controller ? SDL_GetGamepadID( controller ) : 0,
                      event.gbutton.button, SDL_GetKeyboardFocus() != nullptr,
                      static_cast<double>( SDL_GetTicksNS() - event.common.timestamp ) / 1000000.0 );
    }
    if( event.type == SDL_EVENT_GAMEPAD_ADDED ) {
        if( !controller && get_option<bool>( "ENABLE_JOYSTICK" ) ) { controller = SDL_OpenGamepad( event.gdevice.which ); }
        return true;
    }
    if( event.type == SDL_EVENT_GAMEPAD_REMOVED ) {
        if( controller && event.gdevice.which == SDL_GetGamepadID( controller ) ) { quit(); init(); }
        return true;
    }
    if( !controller ) { return false; }
    if( event.type == SDL_EVENT_GAMEPAD_AXIS_MOTION ) {
        if( event.gaxis.which != SDL_GetGamepadID( controller ) ) { return true; }
        const auto axis = event.gaxis.axis;
        if( axis == SDL_GAMEPAD_AXIS_LEFT_TRIGGER || axis == SDL_GAMEPAD_AXIS_RIGHT_TRIGGER ) {
            const auto index = axis == SDL_GAMEPAD_AXIS_LEFT_TRIGGER ? 0 : 1;
            const auto down = trigger_down[index] ? event.gaxis.value > 14000 : event.gaxis.value > 18000;
            if( down == trigger_down[index] ) { return true; }
            trigger_down[index] = down;
            if( index == 0 ) {
                alternate = down;
                repeat_at = 0;
                if( down ) { radial.release(); }
                else {
                    const auto captured = radial_input_captured();
                    const auto selection = radial.release();
                    if( captured ) {
                        require_center = { left_dir != direction::NONE, right_dir != direction::NONE };
                        left_dir = right_dir = direction::NONE;
                    }
                    if( active_context && selection.dir != direction::NONE ) {
                        const auto entry = radial_entries( *active_context,
                                                           selection.page )[static_cast<int>( selection.dir ) - 1];
                        if( entry.available ) {
                            pending_context = active_context;
                            pending_action = entry.action;
                            send( input_event( action_event, input_event_t::gamepad ) );
                        }
                    }
                }
            } else if( !radial_input_captured() ) {
                if( down ) {
                    const auto code = !alternate &&
                                      left_dir != direction::NONE ? stick_code( left_dir, false ) : PAD_RT;
                    start_repeat( PAD_RT, mapped( code ) );
                } else if( repeat_button == PAD_RT ) { repeat_at = 0; }
            }
            request_refresh();
            return true;
        }
        const auto stick = axis == SDL_GAMEPAD_AXIS_LEFTX || axis == SDL_GAMEPAD_AXIS_LEFTY ? 0 : 1;
        const auto dir = axis_direction( stick );
        if( require_center[stick] ) {
            if( dir == direction::NONE ) { require_center[stick] = false; }
            return true;
        }
        auto &old = stick == 0 ? left_dir : right_dir;
        const auto changed = old != dir;
        old = dir;
        if( alternate ) { radial.move( stick, dir ); repeat_at = 0; }
        else if( changed && ( stick == 1 || ( active_context && uses_menu( *active_context ) ) ) ) {
            const auto repeat_control = stick == 0 ? PAD_LS : PAD_RS;
            if( dir == direction::NONE ) {
                if( repeat_button == repeat_control ) { repeat_at = 0; }
            } else {
                start_repeat( repeat_control, mapped( stick_code( dir, stick == 1 ) ) );
            }
        }
        if( changed ) { request_refresh(); }
        return true;
    }
    if( event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN || event.type == SDL_EVENT_GAMEPAD_BUTTON_UP ) {
        if( event.gbutton.which != SDL_GetGamepadID( controller ) ) { return true; }
        const auto button = event.gbutton.button;
        if( event.type == SDL_EVENT_GAMEPAD_BUTTON_UP ) {
            if( repeat_button == button ) { repeat_at = 0; }
            return true;
        }
        if( radial_input_captured() ) {
            if( button == SDL_GAMEPAD_BUTTON_EAST ) { radial.cancel(); }
            if( radial.open() && ( button == SDL_GAMEPAD_BUTTON_LEFT_SHOULDER ||
                                   button == SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER ) ) {
                radial.cycle( button == SDL_GAMEPAD_BUTTON_LEFT_SHOULDER ? -1 : 1 );
            }
            request_refresh();
            return true;
        }
        constexpr auto codes = std::array{ PAD_A, PAD_B, PAD_X, PAD_Y, PAD_BACK, 0, PAD_START,
                                           PAD_LS, PAD_RS, PAD_LB, PAD_RB, PAD_UP, PAD_DOWN, PAD_LEFT, PAD_RIGHT };
        if( button < codes.size() && codes[button] ) {
            const auto event_input = mapped( codes[button] );
            if( button >= SDL_GAMEPAD_BUTTON_DPAD_UP && button <= SDL_GAMEPAD_BUTTON_DPAD_RIGHT ) { start_repeat( button, event_input ); }
            else { send( event_input ); }
        }
        return true;
    }
    return false;
}
auto context_hints( const input_context &ctxt ) -> std::vector<control_hint>
{
    auto result = std::vector<control_hint> {};
    constexpr auto controls = std::array{ PAD_A, PAD_B, PAD_X, PAD_Y, PAD_LB, PAD_RB,
                                          PAD_RT, PAD_LS, PAD_RS, PAD_START, PAD_BACK, PAD_UP, PAD_DOWN, PAD_LEFT, PAD_RIGHT,
                                          PAD_RS_UP, PAD_RS_UP_RIGHT, PAD_RS_RIGHT, PAD_RS_DOWN_RIGHT, PAD_RS_DOWN, PAD_RS_DOWN_LEFT, PAD_RS_LEFT, PAD_RS_UP_LEFT };
    for( const auto alt : { false, true } ) {
        for( const auto code : controls ) {
            if( alt && code >= PAD_RS_LEFT && code <= PAD_RS_DOWN_LEFT ) { continue; }
            const auto &action = ctxt.input_to_action( event_for_control( code, alt, uses_menu( ctxt ) ) );
            if( action != "ERROR" ) {
                result.push_back( { .code = code, .symbol = ( alt ? "LT " : "" ) + control_symbol( code ), .action = action, .alternate = alt } );
            }
        }
    }
    for( auto page = 0; page < 4; ++page ) {
        const auto entries = radial_entries( ctxt, page );
        constexpr auto arrows = std::array{ "↑", "↗", "→", "↘", "↓", "↙", "←", "↖" };
        for( auto slot = 0; slot < 8; ++slot ) {
            if( entries[slot].available ) {
                result.push_back( { .code = PAD_L_RADIAL_N + slot, .symbol = "LT L/R" + std::string( arrows[slot] ) + " · " + radial_page_name( page ), .action = entries[slot].action, .alternate = true } );
            }
        }
    }
    for( auto code = PAD_LS_LEFT; code <= PAD_LS_DOWN_LEFT; ++code ) {
        const auto &action = ctxt.input_to_action( input_event( code, input_event_t::gamepad ) );
        if( action != "ERROR" ) { result.push_back( { .code = code, .symbol = control_symbol( code ), .action = action, .alternate = false } ); }
    }
    return result;
}
auto action_hint( const input_context &ctxt, const std::string &action ) -> std::string
{
    auto result = std::string{};
    for( const auto &hint : context_hints( ctxt ) ) {
        if( hint.action == action ) { result += ( result.empty() ? "" : " / " ) + hint.symbol; }
    }
    return result;
}
auto take_action( const input_context &ctxt ) -> std::string
{
    const auto result = pending_context == &ctxt ? pending_action : std::string{};
    pending_action.clear();
    pending_context = nullptr;
    return result;
}
auto show_bindings( const input_context &ctxt ) -> bool
{
    if( !is_active() || keyboard_help || ctxt.controller_category() == "GAMEPAD_HELP" ) { return false; }
    auto menu = uilist{};
    menu.input_category = "GAMEPAD_HELP";
    menu.title = _( "Controller shortcuts" );
    menu.text = _( "LT + ☰: shortcuts · Ⓐ: select · Ⓑ: back" );
    menu.addentry( 0, true, 'k', _( "Keyboard bindings" ) );
    for( const auto &action : ctxt.get_registered_actions_copy() ) {
        if( action == "ANY_INPUT" || action == "COORDINATE" ) { continue; }
        const auto hint = action_hint( ctxt, action );
        menu.addentry( -1, false, 0, ( hint.empty() ? _( "No direct controller binding" ) : hint ) + "  " +
                       ctxt.get_action_name( action ) );
    }
    menu.hilight_disabled = true;
    menu.query();
    if( menu.ret == 0 ) {
        keyboard_help = true;
        try { const_cast<input_context &>( ctxt ).display_menu(); }
        catch( ... ) { keyboard_help = false; throw; }
        keyboard_help = false;
    }
    return true;
}
} // namespace gamepad
#endif
