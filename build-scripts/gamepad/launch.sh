#!/usr/bin/env bash
set -euo pipefail
script_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
root="${CBN_GAMEPAD_ROOT:-$script_root}"
profile="${CBN_GAMEPAD_PROFILE:-$root/profile}"
if [[ ! -e "$profile/config/options.json" && "$profile" == "$root/profile" ]]; then
    "$root/install-device.sh" auto
fi
mkdir -p "$profile/config" "$profile/save" "$profile/memorial"
export LD_LIBRARY_PATH="$root/runtime/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
cd "$root/app"
exec >>"$profile/launch.log" 2>&1
exec ./cataclysm-bn-tiles --userdir "$profile/" --configdir "$profile/config/" --savedir "$profile/save/" --memorialdir "$profile/memorial/" "$@"
