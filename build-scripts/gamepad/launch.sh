#!/usr/bin/env bash
set -euo pipefail
root="${CBN_GAMEPAD_ROOT:-$HOME/Games/cbn-gamepad}"
profile="${CBN_GAMEPAD_PROFILE:-$root/profile}"
mkdir -p "$profile/config" "$profile/save" "$profile/memorial"
export LD_LIBRARY_PATH="$root/runtime/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
cd "$root/app"
exec >>"$profile/launch.log" 2>&1
exec ./cataclysm-bn-tiles --userdir "$profile/" --configdir "$profile/config/" --savedir "$profile/save/" --memorialdir "$profile/memorial/" "$@"
