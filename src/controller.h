#pragma once
#if defined(TILES)
#include <array>
#include <string>
#include <vector>
#include "input.h"
#include "sdl_wrappers.h"
inline constexpr auto PAD_LS_LEFT = 4096 + 1;
inline constexpr auto PAD_LS_RIGHT = 4096 + 2;
inline constexpr auto PAD_LS_UP = 4096 + 3;
inline constexpr auto PAD_LS_DOWN = 4096 + 4;
inline constexpr auto PAD_LS_UP_RIGHT = 4096 + 5;
inline constexpr auto PAD_LS_DOWN_RIGHT = 4096 + 6;
inline constexpr auto PAD_LS_UP_LEFT = 4096 + 7;
inline constexpr auto PAD_LS_DOWN_LEFT = 4096 + 8;

inline constexpr auto PAD_RS_LEFT = 4096 + 9;
inline constexpr auto PAD_RS_RIGHT = 4096 + 10;
inline constexpr auto PAD_RS_UP = 4096 + 11;
inline constexpr auto PAD_RS_DOWN = 4096 + 12;
inline constexpr auto PAD_RS_UP_RIGHT = 4096 + 13;
inline constexpr auto PAD_RS_DOWN_RIGHT = 4096 + 14;
inline constexpr auto PAD_RS_UP_LEFT = 4096 + 15;
inline constexpr auto PAD_RS_DOWN_LEFT = 4096 + 16;

// Named gamepad buttons (SDL controller mapping)
// Face buttons (SDL: A=0, B=1, X=2, Y=3)
inline constexpr auto PAD_A = 4096 + 32;
inline constexpr auto PAD_B = 4096 + 33;
inline constexpr auto PAD_X = 4096 + 34;
inline constexpr auto PAD_Y = 4096 + 35;

// Shoulder buttons
inline constexpr auto PAD_LB = 4096 + 36;
inline constexpr auto PAD_RB = 4096 + 37;

// Triggers
inline constexpr auto PAD_LT = 4096 + 38;
inline constexpr auto PAD_RT = 4096 + 39;

// Stick clicks
inline constexpr auto PAD_LS = 4096 + 40;
inline constexpr auto PAD_RS = 4096 + 41;

// D-pad (for gameplay actions, not menu navigation)
inline constexpr auto PAD_UP = 4096 + 42;
inline constexpr auto PAD_DOWN = 4096 + 43;
inline constexpr auto PAD_LEFT = 4096 + 44;
inline constexpr auto PAD_RIGHT = 4096 + 45;

// Start/Back
inline constexpr auto PAD_START = 4096 + 46;
inline constexpr auto PAD_BACK = 4096 + 47;

// ALT variants (when LT is held)
inline constexpr auto PAD_ALT_A = 4096 + 48;
inline constexpr auto PAD_ALT_B = 4096 + 49;
inline constexpr auto PAD_ALT_X = 4096 + 50;
inline constexpr auto PAD_ALT_Y = 4096 + 51;
inline constexpr auto PAD_ALT_RB = 4096 + 52;
inline constexpr auto PAD_ALT_LB = 4096 + 53;
inline constexpr auto PAD_ALT_RT = 4096 + 55;
inline constexpr auto PAD_ALT_LS = 4096 + 56;
inline constexpr auto PAD_ALT_RS = 4096 + 57;
inline constexpr auto PAD_ALT_UP = 4096 + 58;
inline constexpr auto PAD_ALT_DOWN = 4096 + 59;
inline constexpr auto PAD_ALT_LEFT = 4096 + 60;
inline constexpr auto PAD_ALT_RIGHT = 4096 + 61;
inline constexpr auto PAD_ALT_START = 4096 + 62;
inline constexpr auto PAD_ALT_BACK = 4096 + 63;

// Left radial menu
inline constexpr auto PAD_L_RADIAL_N = 4096 + 64;
inline constexpr auto PAD_L_RADIAL_NE = 4096 + 65;
inline constexpr auto PAD_L_RADIAL_E = 4096 + 66;
inline constexpr auto PAD_L_RADIAL_SE = 4096 + 67;
inline constexpr auto PAD_L_RADIAL_S = 4096 + 68;
inline constexpr auto PAD_L_RADIAL_SW = 4096 + 69;
inline constexpr auto PAD_L_RADIAL_W = 4096 + 70;
inline constexpr auto PAD_L_RADIAL_NW = 4096 + 71;

// Right radial menu
inline constexpr auto PAD_R_RADIAL_N = 4096 + 72;
inline constexpr auto PAD_R_RADIAL_NE = 4096 + 73;
inline constexpr auto PAD_R_RADIAL_E = 4096 + 74;
inline constexpr auto PAD_R_RADIAL_SE = 4096 + 75;
inline constexpr auto PAD_R_RADIAL_S = 4096 + 76;
inline constexpr auto PAD_R_RADIAL_SW = 4096 + 77;
inline constexpr auto PAD_R_RADIAL_W = 4096 + 78;
inline constexpr auto PAD_R_RADIAL_NW = 4096 + 79;





namespace gamepad
{
enum class direction : int { NONE = 0, N, NE, E, SE, S, SW, W, NW };
struct radial_selection { int page = 0; direction dir = direction::NONE; };
class radial_state
{
    public:
        int page = 0;
        int owner = -1;
        direction selected = direction::NONE;
        bool blocked = false;
        bool await_center = false;
        auto move( int stick, direction dir ) -> bool;
        auto cycle( int delta ) -> void;
        auto cancel() -> void;
        auto release() -> radial_selection;
        auto open() const -> bool { return owner >= 0; }
};
struct radial_entry { std::string action; std::string label; bool available = false; };
struct control_hint { int code; std::string symbol; std::string action; bool alternate; };
class context_scope
{
        const input_context *previous;
    public:
        explicit context_scope( const input_context &ctxt );
        ~context_scope();
        context_scope( const context_scope & ) = delete;
        auto operator=( const context_scope & ) -> context_scope & = delete; // *NOPAD*
};
auto current_context() -> const input_context *; // *NOPAD*
auto init() -> void;
auto quit() -> void;
auto is_active() -> bool;
auto is_alt_held() -> bool;
auto uses_menu( const input_context &ctxt ) -> bool;
auto handle_event( const SDL_Event &event ) -> bool;
auto tick() -> bool;
auto radial_input_captured() -> bool;
auto radial_cancelled() -> bool;
auto is_radial_left_open() -> bool;
auto is_radial_right_open() -> bool;
auto get_radial_left_direction() -> direction;
auto get_left_stick_direction() -> direction;
auto current_radial_page() -> int;
auto radial_entries( const input_context &ctxt, int page ) -> std::array<radial_entry, 8>;
auto radial_page_name( int page ) -> std::string;
auto control_symbol( int code ) -> std::string;
auto event_for_control( int code, bool alternate, bool menu ) -> input_event;
auto context_hints( const input_context &ctxt ) -> std::vector<control_hint>;
auto action_hint( const input_context &ctxt, const std::string &action ) -> std::string;
auto take_action( const input_context &ctxt ) -> std::string;
auto show_bindings( const input_context &ctxt ) -> bool;
auto submit_event( const input_event &event ) -> void;
auto request_refresh() -> void;
auto compact_layout() -> bool;
}
#endif
