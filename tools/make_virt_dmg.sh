#!/bin/bash
# Package the machine.virt (QEMU aarch64 virt) build into a runnable macOS
# .app bundle and a DMG image. The bundle carries its own qemu-system-aarch64
# and all Homebrew shared-library dependencies, so it runs on any macOS
# (Apple Silicon) box with nothing installed.
#
# Prerequisites (already built):
#   machine.virt/kernel/kernel8.img
#   machine.virt/system/root_aarch64.img
# Prerequisites (installed, build host only):
#   qemu-system-aarch64 (brew), hdiutil (macOS builtin)
#
# Usage: tools/make_virt_dmg.sh [output.dmg]

set -euo pipefail

ROOT_DIR=$(cd "$(dirname "$0")/.." && pwd)
KERNEL_IMG="$ROOT_DIR/machine.virt/kernel/kernel8.img"
ROOTFS_IMG="$ROOT_DIR/machine.virt/system/root_aarch64.img"
OUT_DMG=${1:-"$ROOT_DIR/EwokOS-virt-aarch64.dmg"}

APP_NAME="EwokOS-Virt"
STAGE=$(mktemp -d)
APP_DIR="$STAGE/$APP_NAME.app"
trap 'rm -rf "$STAGE"' EXIT

[ -f "$KERNEL_IMG" ] || { echo "missing $KERNEL_IMG (run: make -C machine.virt/kernel)"; exit 1; }
[ -f "$ROOTFS_IMG" ] || { echo "missing $ROOTFS_IMG (run: make -C machine.virt/system sd)"; exit 1; }
QEMU_BIN=$(command -v qemu-system-aarch64) || { echo "qemu-system-aarch64 not found"; exit 1; }

mkdir -p "$APP_DIR/Contents/MacOS" "$APP_DIR/Contents/Resources" "$APP_DIR/Contents/Frameworks"

# ---------------------------------------------------------------------------
# Bundle qemu and every non-system dylib it (transitively) needs.
# References are rewritten to @rpath/<basename> with rpath @loader_path so the
# bundle is location-independent; everything is then ad-hoc re-signed.
# ---------------------------------------------------------------------------
bundle_qemu() {
    local FW="$APP_DIR/Contents/Frameworks"
    local main="$APP_DIR/Contents/MacOS/qemu-system-aarch64"
    cp "$QEMU_BIN" "$main"

    # BFS over the dependency graph (bash-3.2 compatible: no assoc arrays)
    local queue="$main"
    local done_list=":"
    while [ -n "$queue" ]; do
        local f=${queue%%$'\n'*}
        if [ "$queue" = "$f" ]; then queue=""; else queue=${queue#*$'\n'}; fi
        case "$done_list" in *":$f:"*) continue ;; esac
        done_list="$done_list$f:"
        # deps printed by otool -L, minus the header (first line)
        local deps
        deps=$(otool -L "$f" | tail -n +2 | sed -e 's/(.*//' -e 's/^[[:space:]]*//' -e 's/[[:space:]]*$//')
        local dep
        for dep in $deps; do
            [ -n "$dep" ] || continue
            case "$dep" in
                /System/*|/usr/lib/*|@rpath/*|@executable_path/*|@loader_path/*) continue ;;
            esac
            local base depfile="$dep"
            [ -f "$depfile" ] || continue
            base=$(basename "$depfile")
            [ -f "$FW/$base" ] || cp "$depfile" "$FW/$base"
            case "$done_list" in *":$FW/$base:"*) ;; *)
                if [ -n "$queue" ]; then queue="$queue
$FW/$base"; else queue="$FW/$base"; fi ;; esac
        done
    done

    # rewrite load commands: every copied lib gets id @rpath/<base>, and every
    # file referencing a bundled lib points at @rpath/<base>
    local lib
    for lib in "$FW"/*; do
        install_name_tool -id "@rpath/$(basename "$lib")" "$lib" >/dev/null 2>&1 || true
        install_name_tool -add_rpath @loader_path "$lib" >/dev/null 2>&1 || true
    done
    install_name_tool -add_rpath "@loader_path/../Frameworks" "$main" >/dev/null 2>&1 || true

    local f2 dep2
    for f2 in "$main" "$FW"/*; do
        for dep2 in $(otool -L "$f2" | tail -n +2 | sed -e 's/(.*//' -e 's/^[[:space:]]*//' -e 's/[[:space:]]*$//'); do
            [ -n "$dep2" ] || continue
            case "$dep2" in
                /System/*|/usr/lib/*|@rpath/*|@executable_path/*|@loader_path/*) continue ;;
            esac
            [ -f "$FW/$(basename "$dep2")" ] || continue
            install_name_tool -change "$dep2" "@rpath/$(basename "$dep2")" "$f2"
        done
    done

    # modifying the binaries invalidates their signatures: re-sign ad-hoc
    codesign --force --sign - "$main" >/dev/null
    for lib in "$FW"/*; do codesign --force --sign - "$lib" >/dev/null; done
    echo "bundled qemu with $(ls "$FW" | wc -l | tr -d ' ') shared libraries"
}
bundle_qemu

cat > "$APP_DIR/Contents/Info.plist" <<'PLIST'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleName</key>            <string>EwokOS-Virt</string>
    <key>CFBundleDisplayName</key>     <string>EwokOS virt</string>
    <key>CFBundleIdentifier</key>      <string>org.ewokos.virt</string>
    <key>CFBundleVersion</key>         <string>1.0</string>
    <key>CFBundleShortVersionString</key> <string>1.0</string>
    <key>CFBundlePackageType</key>     <string>APPL</string>
    <key>CFBundleExecutable</key>      <string>EwokOS-Virt</string>
    <key>LSMinimumSystemVersion</key>  <string>12.0</string>
    <key>NSHighResolutionCapable</key> <true/>
    <key>NSAppTransportSecurity</key>  <dict><key>NSAllowsArbitraryLoads</key><true/></dict>
</dict>
</plist>
PLIST

# The bundle payload: kernel + fresh rootfs image (guest writes persist in the
# app support copy, the bundle itself may live on a read-only DMG).
cp "$KERNEL_IMG" "$APP_DIR/Contents/Resources/kernel8.img"
cp "$ROOTFS_IMG" "$APP_DIR/Contents/Resources/root_aarch64.img"

cat > "$APP_DIR/Contents/MacOS/$APP_NAME" <<LAUNCHER
#!/bin/bash
# EwokOS QEMU virt launcher (aarch64, GUI)
RES="\$(cd "\$(dirname "\$0")/../Resources" && pwd)"
# prefer the qemu bundled inside this app; fall back to PATH then brew
QEMU="\$(cd "\$(dirname "\$0")" && pwd)/qemu-system-aarch64"
[ -x "\$QEMU" ] || QEMU="\$(command -v qemu-system-aarch64 || echo '$QEMU_BIN')"
DATA_DIR="\$HOME/Library/Application Support/EwokOS"
mkdir -p "\$DATA_DIR"
ROOTFS="\$DATA_DIR/root_aarch64.img"
if [ ! -f "\$ROOTFS" ]; then
    cp "\$RES/root_aarch64.img" "\$ROOTFS"
fi

# prefer hvf on Apple Silicon, fall back to tcg elsewhere
ACCEL="-M virt,highmem=on,accel=hvf -cpu host"
if ! \$QEMU -M help >/dev/null 2>&1 || ! sysctl -n kern.hv_support 2>/dev/null | grep -q 1; then
    ACCEL="-M virt,highmem=on,accel=tcg -cpu cortex-a72"
fi

exec \$QEMU \$ACCEL -m 8192 -smp 4 \\
    -serial mon:stdio \\
    -device ramfb -display cocoa,zoom-to-fit=on,zoom-interpolation=on \\
    -kernel "\$RES/kernel8.img" \\
    -drive file="\$ROOTFS",format=raw,id=blk0,if=none -device virtio-blk-device,drive=blk0 \\
    -netdev user,id=net0,hostfwd=tcp:127.0.0.1:2222-:22 -device virtio-net-device,netdev=net0 \\
    -fsdev local,id=fsdev0,path="\$HOME",security_model=none -device virtio-9p-device,fsdev=fsdev0,mount_tag=hostshare \\
    -device virtio-tablet-device -device virtio-keyboard-device \\
    -audiodev coreaudio,id=audio0 -device virtio-sound-device,audiodev=audio0
LAUNCHER
chmod +x "$APP_DIR/Contents/MacOS/$APP_NAME"

echo "EwokOS virt (QEMU aarch64)
Double-click EwokOS-Virt.app to boot the GUI system.
No dependencies required: QEMU and all its libraries are bundled inside the app.
SSH into the guest: ssh -p 2222 root@127.0.0.1
Guest rootfs writes persist in ~/Library/Application Support/EwokOS.
To reset the guest disk, delete ~/Library/Application Support/EwokOS/root_aarch64.img." > "$STAGE/README.txt"
cp "$ROOT_DIR/LICENSE" "$STAGE/LICENSE" 2>/dev/null || true

rm -f "$OUT_DMG"
hdiutil create -volname "EwokOS" -srcfolder "$STAGE" -ov -format UDZO "$OUT_DMG"
echo "created $OUT_DMG"
