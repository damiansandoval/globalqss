#!/bin/bash
# Smoke test for GlobalQSS under Plasma: start Qt/KDE apps with GlobalQSS as the
# configured KDE widget style (like "widgetStyle=GlobalQSS" in kdeglobals) and check
# that they stay alive and that the Human theme gets applied.
#
# Uses a throwaway XDG_CONFIG_HOME, so the real ~/.config is never touched.
#
#   tests/plasma-smoke.sh [build-dir] [app ...]
#   default build dir: build (expects build/plugins/styles/libglobalqssplugin.so)
#   default apps: kwrite okular qdbusviewer6

REPO="$(cd "$(dirname "$(readlink -f "$0")")/.." && pwd)"
BUILD="$(readlink -f "${1:-$REPO/build}")"
shift
APPS=("${@:-kwrite okular qdbusviewer6}")
read -r -a APPS <<< "${APPS[*]}"
THEME=Human
WAIT=5

[ -f "$BUILD/plugins/styles/libglobalqssplugin.so" ] || { echo "no plugin in $BUILD/plugins/styles"; exit 2; }

CONF="$(mktemp -d)"
LOGS="$(mktemp -d)"
trap 'rm -rf "$CONF"' EXIT
mkdir -p "$CONF/gtk-3.0"
printf '[KDE]\nwidgetStyle=GlobalQSS\n' > "$CONF/kdeglobals"
printf '[Settings]\ngtk-theme-name=%s\n' "$THEME" > "$CONF/gtk-3.0/settings.ini"

fails=0
for app in "${APPS[@]}"; do
    log="$LOGS/$app.log"
    env XDG_CONFIG_HOME="$CONF" QT_PLUGIN_PATH="$BUILD/plugins" GQSS_DEBUG=1 QT_FORCE_STDERR_LOGGING=1 \
        timeout -s TERM "$WAIT" "$app" > "$log" 2>&1
    rc=$?
    if [ $rc -eq 124 ]; then
        alive=ok
    else
        alive="FAIL (exit $rc)"
        fails=$((fails + 1))
    fi
    if grep -q "GQSS: theme \"$THEME\"" "$log" && grep -q 'GQSS: setStyleSheet' "$log"; then
        theme=ok
    else
        theme="FAIL ($(grep -m1 -o 'GQSS: theme .*' "$log" || echo 'no theme'))"
        fails=$((fails + 1))
    fi
    printf '%-14s alive: %-16s theme %s: %s   (log %s)\n' "$app" "$alive" "$THEME" "$theme" "$log"
done
echo "$fails failure(s)"
[ $fails -eq 0 ]
