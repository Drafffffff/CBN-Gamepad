# Cataclysm: Bright Nights - Agent Guidelines

## HARD CONSTRAINTS (NEVER VIOLATE)

Before writing **ANY** code, verify:

| ❌ VIOLATION                           | ✅ REQUIRED                                                                      |
| -------------------------------------- | -------------------------------------------------------------------------------- |
| manual iterator loops (`it++`, `++it`) | `std::ranges::*`, `collection \| std::views::*`, or range-based `for` if clearer |
| `int foo()`                            | `auto foo() -> int`                                                              |
| `Type x = value`                       | `auto x = value`                                                                 |
| `void fn(a, b, c, d, e)`               | `void fn(options_struct)`                                                        |
| `[](){\n return 1; \n }`               | `[](){ return 1; }`                                                              |

**Prefer `std::ranges`/`std::views`/`std::ranges::to`/cata_algo.h for collection work. Avoid manual iterator increment loops unless required by mutation semantics.**

- prefer function-local `using namespace std::views;` and use `transform`/`filter` unqualified.
- prefer function-local `namespace ranges = std::ranges;` and use `ranges::*` without `std::`
- prefer method/function references over lambdas whenever possible, e.g. `transform( &vpart_position::part_index )` instead of `transform( []( const auto &vp ) { return vp.part_index(); } )`.

## Coding Convention

```c++
const auto foo = 3; //< **MUST** use `auto` for type. `const` **MUST** come before `auto`.

auto bar() -> int; //< **MUST** use trailing return types.
using my_callback_t = std::function<auto( int ) -> bool>; //< **MUST** use trailing return types in type aliases.
auto baz() -> int&; // *NOPAD*  //< **MUST** append `// *NOPAD*` for references/pointer returns to prevent astyle bugs.
auto qux() -> int { return 42; } //< **MUST** use single-line functions whenever possible.

auto qux = my_struct{ .a = 1, .b = 2 }; //< **MUST** use designated initializers.
auto two_value() -> my_data; //< **MUST NOT** use `std::pair`/`std::tuple` for multiple return values. Create a struct instead.
auto may_have_value() -> std::optional<int>; //< **MUST** use `std::optional` for functions that may not return a value.
auto may_fail() -> std::expected<int, std::string>; //< **MUST** use `std::expected` for functions that may fail.

/// **MUST** use triple slash for doc comments like rust's.
/// **MUST** use snake_case for functions and variables.
struct comparable {
  int x;
  int y;
  auto operator<=>( const comparable & ) const = default; // *NOPAD* //< **MUST** use `<=>` for comparisons and append `// *NOPAD*` at the end to prevent astyle bugs.
}

auto values = xs
  | std::views::filter( []( const auto & v ) { return v.is_valid(); } ) //< **MUST** use single line expression if it's single line expression
  | std::views::transform( []( const auto & v ) { return v.get_value(); } ) //< **SHOULD** use `auto` for lambda params
  | std::ranges::to<std::vector>(); //< **MUST** use `std::ranges` over for loops for collections.

namespace { // **MUST** use anonymous namespace for internal linkage over `static`.

// **MUST** use options struct for functions with >3 parameters
struct button_options {
  point pos;
  std::string text;
  nc_color fg = c_white;
  nc_color bg = c_black;
  bool enabled = true;
};
auto print_button( const catacurses::window &w, const button_options &opts ) -> void;

} // namespace
```

- **SHOULD NOT** modify existing headers with >10 usages. Create new header with pure functions.
- **MUST** use modern C++23 features.
- **MUST** preserve unused parameter names as comments instead of deleting them, e.g. `bool /*is_avatar*/` not `bool`; applies to functions and lambdas.
- **MUST** keep Lua function parameters typed with EmmyLua/LuaLS annotations, including existing and local helper functions: `---@param` and table `---@class`/`---@field` shapes where parameters are tables. Do not require or add `---@return` solely for annotation enforcement when the return type is inferable. Before touching Lua, inspect the file's annotation style and preserve complete function typing.
- **MUST** fix missing Lua binding type declarations at the binding/doc-generation source; do not hard-code generated binding classes in `data/raw/generate_types.lua` as a shortcut.
- **MUST** test C++ Lua binding behavior with real bound objects when adding or changing bindings; Lua-only mocks may supplement but must not be the sole validation for binding correctness.
- **MUST** use options struct for functions with more than 3 parameters. Use designated initializers at call sites.
- **MUST NOT** manually write an options/struct type at a call site when the function parameter type makes it inferable; use `{ .field = value }` instead of `options_type{ .field = value }`.
- **SHOULD** search for existing solution because it's a large, legacy codebase.
- **MUST** verify helper-specific matching semantics before relying on string prefixes. For overmap terrain `OtMatchType.PREFIX` / `is_ot_match`, pass the base token without a trailing separator, e.g. `"robofachq"`, because the matcher itself requires the following character to be `_`.

## Workflow

### WHEN given a link to an issue

- **Context**: Fetch issue details via GitHub MCP.
- **Branch**: Use `coderabbitai/git-worktree-runner` to create branch: `git gtr new <type>/<issue-id>/<issue-slug>`
  - type MUST be one of: `feat`, `fix`, `refactor`, `chore`, `build`, `ci`
- **Code**: Refer to [code changes](#when-working-on-code-changes).
- **PR**: Use [Template](./.github/pull_request_template.md). **DO NOT ADD fluff**. create via `git push && gh pr create --web --fill`.
- After opening or updating a Cataclysm-BN PR, track `gh pr checks` until CI finishes or a concrete blocker is identified; inspect failing job logs, fix branch-owned failures, commit, and push before finalizing. For transient or infrastructure failures, rerun when permitted or report the exact failing job and evidence.
- Before running broad formatter targets, prefer file-scoped formatting for touched files when available; if only a broad target exists, inspect and revert unrelated formatter-only changes before continuing.

### WHEN working on code changes

- **Style**: Follow [Code Style](./docs/en/dev/explanation/code_style.md). Use `_( "text" )` for L10n.
- **Format**: Format code before building/testing.

```sh
# Format C++ code
cmake --build build --target format
# Format JSON files
cmake --build build --target style-json-parallel
# Format scripts
deno fmt
deno task dprint fmt
```

- **Verify**: Build and fix any issues. Do not skip the game binary target when validating code changes; build `cataclysm-bn-tiles` together with tests.

```sh
# Build project and tests
cmake --preset linux-full
cmake --build --preset linux-full --target cataclysm-bn-tiles cata_test-tiles
```

- **Test**: Create/update relevant `tests/` (Catch2).

```sh
# Run Tests
./out/build/linux-full/tests/cata_test-tiles "[optional-filter]"

# Validate JSON
./build-scripts/lint-json.sh

# Check Mods (validates mod JSON files)
./out/build/linux-full/cataclysm-bn-tiles --check-mods

# Generate Lua Documentation (if conflicts with lua_annotations.lua or docs/en/mod/lua/reference/lua.md)
deno task docs:gen
```

- **Commit**: Commit **ATOMICALLY**. **MUST** Follow [Conventional Commits](./docs/en/contribute/changelog_guidelines.md). **MUST NOT** add body/footer unless critical.

## WHEN working on i18n / PO context

- **MUST NOT** reduce requested string/context coverage for review risk or churn. If the user names a word and its meanings, handle every named meaning.
- If adding JSON context requires loader support, add loader support instead of leaving a source uncontexted.
- **MUST** run `msgfmt -f -c -o /tmp/ko.mo lang/po/ko.po` after touching Korean PO files and fix reported errors before PR.
- **MUST** run `./tools/check_po_printf_format.py` after touching PO files and fix reported errors before PR.
- Do not call PO/printf errors pre-existing to skip them when the task touches that locale or validation path.
- If a mistake is found during the task, update AGENTS/skill immediately and fix the current branch before summarizing.

## WHEN translating docs

When translating, MUST search for correct glossary, e.g

```sh
rg -C2 -i '<<TARGET>>' lang/po/<<LANG>>.po | rg -v '^(#:|--)' | head -n 20
rg -C2 -i 'speedway' lang/po/ko.po | rg -v '^(#:|--)' | head -n 20
```

## References

- **Docs**: [Building](./docs/en/dev/guides/building/cmake.md), [Formatting](./docs/en/dev/guides/formatting.md), [Dev Index](./docs/en/dev/).
- **Review**: [LLM Guide](./.github/llm_review_guide.md).

- When fixing a bug, preserve requested behavior and visible content unless the user explicitly asks to remove it; fix the underlying issue instead of suppressing the affected feature.
- When reviewing PRs that stop tracking generated or externally pulled files, verify ignore rules by running the generator/pull command or checking `git status --ignored`; do not assume removed tracked files are ignored.
- When generated or externally pulled files are removed from tracking, verify all CI and release consumers still receive required files or directories.

## Controller port validation

- A controller port must include native world rendering and action-target semantics, not only SDL routing and bindings. Compare the installed reference's eight-direction preview, trigger-gated movement, selected-tile actions, nested cancellation, and action reachability. Screenshot counts alone do not establish any of these behaviors; inspect contents and assert world position/turn invariants.

- Validate the real Steam input device path as well as virtual fixtures. A fixture that filters all keyboard and mouse events cannot establish that mixed Steam input works. Cover left-stick menu navigation separately from trigger-gated gameplay movement.

- Desktop input contexts do not expose Android-only getters; use the const controller category accessor and registered-action copy.
- SDL3 gamepad timestamps use nanoseconds, while SDL_GetTicks uses milliseconds; scheduler deadlines must use SDL_GetTicks consistently.
- Configure new build directories with absolute compiler paths. Changing compiler paths in a populated CMake cache can reset feature flags; recheck TILES, SOUND and USE_HOME_DIR.
- BN keeps SDL last_input private; submit controller events through the SDL bridge rather than declaring an extern input buffer.
- When merging runtime assets, copy soundpack contents directly; a sound directory symlink nested inside itself loads duplicate soundpack definitions.
- Supplemental translation headers must include Content-Transfer-Encoding: 8bit; BN rejects gettext catalogs that omit it. Reconfigure CMake after adding test source files because its test source glob is not CONFIGURE_DEPENDS.

- `register_action()` does not deduplicate: guard automatic controller-help registration. Reserve LT+Start for help; do not reuse its binding for zoom. SDL3 letter-input fixtures must emit TEXT_INPUT for translated main-menu hotkeys.
- BN shares its key-name lookup between keyboard and gamepad. Prefix all new gamepad binding names with `PAD_`; unprefixed `A`/`UP` silently overwrite keyboard codes. Test keyboard-name preservation alongside controller bindings.
- Reserving footer rows can leave fewer than the historical 24 main-menu rows. Clamp the menu window to the current terminal size; `newwin` outside it returns an invalid window and later width arithmetic can divide by zero.
- Direction prompts reuse `DEFAULTMODE` without registering gameplay actions. Classify menu routing from the registered actions too, or B remains a gameplay grab event instead of cancelling the prompt.

- Controller smoke fixtures must validate the entered screen and keep synthesized press/release pairs consistent; timestamp-dependent menu conversion must include the release event, otherwise a held stick can silently choose a different new-game path.

- Trigger movement repeat must resolve the current stick direction on every repeat, including centered state; never retain the direction captured on trigger press. Clear held input and pending wheel actions on focus loss, and require held triggers/sticks to release/center before resuming.

- Poll queued physical release/focus events before emitting scheduled repeats. Prioritizing repeat deadlines can starve the event queue when a game turn takes longer than the repeat interval.

- SDL virtual joystick trigger axes use signed joystick range: release is -32768, mapped by SDL to gamepad trigger 0. Keep injected event values consistent with SDL_GetGamepadAxis when testing focus/release behavior.

- Use a positive handle_input timeout when asserting absence of input. handle_input(0) polls without synthesizing TIMEOUT and can loop indefinitely when no registered action arrives.

- BN CLI directory arguments are concatenated literally: pass trailing slashes for --userdir, --configdir and --savedir. Check actual screenshot dimensions and loaded option files; otherwise the fixture silently uses fallback defaults and cannot validate the requested display preset. Preserve conditional vehicle/mana panels when reordering sidebar presets.

- Reserve controller footer space after hotplug as well as initial startup and resize. Test with no controller at launch, connect after loading, and inspect the last visible log line. CDDA's right-stick cursor marks the shifted view center, while its left-stick cursor selects an adjacent interaction tile; preserve those distinct actions when improving visual priority.

- Hotplug layout refresh requires an existing SDL window. Headless controller tests initialize SDL gamepads without font/window state, so never divide by font/scaling metrics in that path.
