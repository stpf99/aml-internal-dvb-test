################################################################################
#      This file is part of LibreELEC - https://libreelec.tv
#      Copyright (C) 2025 TimSeed & mczerski
#      Amlogic DVB Support Package - GXL (S905D/X)
################################################################################

PKG_NAME="amlogic-dvb"
PKG_VERSION="6.0-gxl"
PKG_REV="1"
PKG_ARCH="arm aarch64"
PKG_LICENSE="GPL"
PKG_SITE="https://github.com/timseed"
PKG_URL=""
PKG_DEPENDS_TARGET="toolchain linux"
PKG_SECTION="driver"
PKG_SHORTDESC="Amlogic DVB drivers for GXL (S905D/X)"
PKG_LONGDESC="DVB drivers for Amlogic GXL SoCs with internal tuners. Supports M88RS6060 and others. Ported from CoreELEC 22."
PKG_TOOLCHAIN="manual"
PKG_IS_KERNEL_PKG="yes"

# === Tylko dla GXL ===
case "$DEVICE" in
  AMLGX|Generic|M8S|M8SPLUS)
    ;;
  *)
    exit 0
    ;;
esac

# === Wymagany kernel 6.x ===
if [ "$(kernel_version)" \< "6.0" ]; then
  echo "ERROR: amlogic-dvb requires kernel 6.0+"
  exit 1
fi

# === Funkcja: ścieżka kernela ===
kernel_path() {
  echo "$(get_build_dir linux)"
}

# === Budowanie ===
make_target() {
  local src_dir="$PKG_BUILD/sources/aml_dvb"
  local kernel_dir="$(kernel_path)"

  cd "$src_dir"

  # Buduj moduły
  make -C "$kernel_dir" M="$src_dir" \
    CONFIG_AMLOGIC_DVB=m \
    CONFIG_AMLOGIC_TS=m \
    CONFIG_AMLOGIC_DMX=m \
    CONFIG_AMLOGIC_DSC=m \
    KERNEL_SRC="$kernel_dir"
}

# === Instalacja ===
makeinstall_target() {
  local mod_dir="$INSTALL/$(get_full_module_dir)/extra/amlogic-dvb"
  local src_dir="$PKG_BUILD/sources/aml_dvb"

  mkdir -p "$mod_dir"

  # Kopiuj .ko
  find "$src_dir" -name "*.ko" -exec cp {} "$mod_dir/" \;

  # modules-load.d
  mkdir -p "$INSTALL/usr/lib/modules-load.d"
  cat > "$INSTALL/usr/lib/modules-load.d/amlogic-dvb.conf" <<EOF
aml_dvb
aml_dmx
aml_ts
aml_dsc
EOF

  # udev rules
  mkdir -p "$INSTALL/usr/lib/udev/rules.d"
  cat > "$INSTALL/usr/lib/udev/rules.d/99-amlogic-dvb.rules" <<EOF
SUBSYSTEM=="dvb", GROUP="video", MODE="0660"
KERNEL=="dvb*", SYMLINK+="dvb/adapter%n/%k"
EOF

  # Konfiguracja
  mkdir -p "$INSTALL/usr/config"
  cat > "$INSTALL/usr/config/amlogic-dvb.conf" <<EOF
TS_MODE=1
DEBUG_LEVEL=1
ENABLE_DSC=1
EOF
}

# === Systemd service ===
post_makeinstall_target() {
  mkdir -p "$INSTALL/usr/lib/systemd/system"
  cat > "$INSTALL/usr/lib/systemd/system/amlogic-dvb.service" <<EOF
[Unit]
Description=Amlogic DVB Initialization
After=systemd-modules-load.service
Before=kodi.service tvheadend.service

[Service]
Type=oneshot
ExecStart=/usr/bin/amlogic-dvb-init.sh
RemainAfterExit=yes

[Install]
WantedBy=multi-user.target
EOF

  mkdir -p "$INSTALL/usr/bin"
  cat > "$INSTALL/usr/bin/amlogic-dvb-init.sh" <<'EOF'
#!/bin/bash
sleep 2
if [ -d /dev/dvb/adapter0 ]; then
  echo "DVB OK: /dev/dvb/adapter0"
  ls -la /dev/dvb/adapter0/
else
  echo "DVB NOT DETECTED!"
  dmesg | grep -i "aml_dvb\|dvb" | tail -10
fi
EOF
  chmod +x "$INSTALL/usr/bin/amlogic-dvb-init.sh"

  enable_service amlogic-dvb.service
}
