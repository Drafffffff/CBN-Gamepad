#!/usr/bin/env bash
set -euo pipefail
root="$HOME/Games/cbn-gamepad"
upstream="$root/upstream/cataclysmbn-2026-10-11"
mkdir -p "$root/app" "$root/runtime/lib" "$root/smoke/output" "$root/smoke/profile/config"
cp "$root/source/out/gamepad/src/cataclysm-bn-tiles" "$root/app/"
cp -a "$root/source/data" "$root/app/"
cp -a "$upstream/lang" "$root/app/"
if [[ -L "$root/app/gfx" ]]; then unlink "$root/app/gfx"; fi
mkdir -p "$root/app/gfx"
cp -a "$upstream/gfx/." "$root/app/gfx/"
mkdir -p "$root/app/data/sound"
cp -a "$upstream/data/sound/." "$root/app/data/sound/"
cp -L "$root/source/out/gamepad/src/lua/libliblua.so" "$root/runtime/lib/"
cp -L "$root/source/out/gamepad/_deps/sdl3_mixer-build/libSDL3_mixer.so.0" "$root/runtime/lib/"
podman run --rm -v "$root:/work:Z" -w /work/source localhost/cbn-gamepad-builder:43 bash -lc '
clang -shared -fPIC build-scripts/gamepad/controller-smoke.c -o /work/smoke/controller-smoke.so -lSDL3 -lSDL3_image -ldl
msgunfmt /work/app/lang/mo/zh_CN/LC_MESSAGES/cataclysm-bn.mo -o /tmp/base.po
msgcat --use-first build-scripts/gamepad/controller-zh_CN.po /tmp/base.po -o /tmp/merged.po
msgfmt --check-format /tmp/merged.po -o /work/app/lang/mo/zh_CN/LC_MESSAGES/cataclysm-bn.mo
out/gamepad/tools/format/json_formatter data/raw/keybindings/keybindings.json || out/gamepad/tools/format/json_formatter data/raw/keybindings/keybindings.json
cp data/raw/keybindings/keybindings.json /work/app/data/raw/keybindings/keybindings.json
'
