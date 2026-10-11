#!/usr/bin/env bash
set -euo pipefail
root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
device="${1:-auto}"
if [[ "$device" == auto ]]; then
    device=desktop
    if [[ -r /etc/os-release ]] && grep -q '^ID=steamos' /etc/os-release; then
        device=small
    fi
fi
case "$device" in
    desktop|small) ;;
    *) echo 'Usage: install-device.sh [desktop|small|auto]' >&2; exit 2 ;;
esac
mkdir -p "$root/profile/config" "$root/profile/save" "$root/profile/memorial"
for file in options.json fonts.json; do
    if [[ ! -e "$root/profile/config/$file" ]]; then
        cp "$root/presets/$device/config/$file" "$root/profile/config/$file"
    fi
done
chmod +x "$root/launch.sh" "$root/app/cataclysm-bn-tiles"
echo "CBN Gamepad ready: $root/launch.sh ($device preset; existing settings preserved)"
