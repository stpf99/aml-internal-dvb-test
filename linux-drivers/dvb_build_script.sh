#!/bin/bash
# LibreELEC Amlogic DVB - Automated Build Script
# Version: 1.1 (FIXED for GXL + DVB)
# Date: November 2025

set -e

# === Kolory ===
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# === Konfiguracja ===
LIBREELEC_REPO="https://github.com/LibreELEC/LibreELEC.tv.git"
LIBREELEC_BRANCH="master"
DVB_PACKAGE_DIR="$(pwd)/amlogic-dvb"
BUILD_DIR="$HOME/libreelec-build"
PROJECT="Amlogic"
DEVICE="AMLGX"           # POPRAWKA: M8S = S905D
ARCH="aarch64"         # POPRAWKA: 64-bit
KERNEL_PATCHES_DIR="$BUILD_DIR/projects/$PROJECT/patches/linux"
DTS_DIR="$BUILD_DIR/linux/arch/arm64/boot/dts/amlogic"

# === Funkcje ===
log_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
log_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
log_warn() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; exit 1; }

check_dependencies() {
    log_info "Sprawdzanie zależności..."
    local deps=("git" "make" "gcc" "wget" "bc" "gawk" "patch")
    for dep in "${deps[@]}"; do
        command -v "$dep" &> /dev/null || log_error "Brak: $dep (sudo apt install $dep)"
    done
    log_success "Wszystkie zależności OK"
}

clone_libreelec() {
    log_info "Klonowanie LibreELEC..."
    if [ -d "$BUILD_DIR" ]; then
        cd "$BUILD_DIR" && git pull || true
    else
        git clone --depth=1 -b "$LIBREELEC_BRANCH" "$LIBREELEC_REPO" "$BUILD_DIR"
    fi
    cd "$BUILD_DIR"
    log_success "LibreELEC gotowy"
}

install_dvb_package() {
    log_info "Instalowanie pakietu DVB..."
    local target_dir="$BUILD_DIR/packages/linux-drivers/amlogic-dvb"

    # Utwórz katalog, jeśli nie istnieje
    mkdir -p "$target_dir"

    # Usuń stare (jeśli istnieje)
    rm -rf "$target_dir"/*

    # Skopiuj
    cp -r "$DVB_PACKAGE_DIR"/* "$target_dir/"

    # Patche do kernela
    local kernel_patch_dir="$BUILD_DIR/projects/$PROJECT/patches/linux"
    mkdir -p "$kernel_patch_dir"
    cp "$target_dir/patches"/*.patch "$kernel_patch_dir/" 2>/dev/null || true

    # Device Tree
    local dts_dir="$BUILD_DIR/linux/arch/arm64/boot/dts/amlogic"
    mkdir -p "$dts_dir"
    cp "$target_dir/meson-gxl-dvb.dtsi" "$dts_dir/" 2>/dev/null || true

    # Dołącz do .dtsi
    local main_dtsi="$dts_dir/meson-gxl-s905d.dtsi"
    if [ -f "$main_dtsi" ] && ! grep -q "meson-gxl-dvb.dtsi" "$main_dtsi"; then
        echo '#include "meson-gxl-dvb.dtsi"' >> "$main_dtsi"
        log_success "Dołączono Device Tree"
    fi

    log_success "Pakiet DVB zainstalowany"
}

configure_build() {
    log_info "Konfiguracja builda..."
    export PROJECT="$PROJECT"
    export DEVICE="$DEVICE"
    export ARCH="$ARCH"
    log_info "PROJECT=$PROJECT DEVICE=$DEVICE ARCH=$ARCH"
}

build_image() {
    log_info "Budowanie obrazu (30-60 min)..."
    cd "$BUILD_DIR"

    [[ "$CLEAN_BUILD" == "1" ]] && make clean

    # Włącz DVB w configu
    sed -i 's/# CONFIG_DVB_AMLOGIC is not set/CONFIG_DVB_AMLOGIC=y/' .config 2>/dev/null || true

    PROJECT="$PROJECT" DEVICE="$DEVICE" ARCH="$ARCH" make image || log_error "Błąd budowania!"
    log_success "Obraz gotowy!"
}

show_results() {
    local img=$(ls target/*.img.gz 2>/dev/null | head -1)
    [[ -z "$img" ]] && log_error "Brak obrazu!" && return

    log_success "OBRAZ: $img"
    echo ""
    echo "Flashowanie (Linux):"
    echo "  gunzip -c \"$img\" | sudo dd of=/dev/sdX bs=4M status=progress conv=fsync"
    echo ""
    echo "Flashowanie (macOS):"
    echo "  gunzip -c \"$img\" | sudo dd of=/dev/rdiskX bs=4m"
    echo ""
    log_warn "Zamień sdX/rdiskX na swoje urządzenie!"
}

# === Główna funkcja ===
main() {
    echo -e "${GREEN}LibreELEC Amlogic DVB Build Script v1.1${NC}\n"

    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help) print_usage; exit 0 ;;
            -c|--clean) CLEAN_BUILD=1; shift ;;
            -b|--branch) LIBREELEC_BRANCH="$2"; shift 2 ;;
            -a|--arch) ARCH="$2"; shift 2 ;;
            -d|--device) DEVICE="$2"; shift 2 ;;
            --deps-only) check_dependencies; exit 0 ;;
            *) log_error "Nieznana opcja: $1"; print_usage; exit 1 ;;
        esac
    done

    [[ ! -d "./amlogic-dvb" && ! -d "../amlogic-dvb" ]] && \
        log_error "Brak katalogu amlogic-dvb!"

    [[ -d "../amlogic-dvb" ]] && DVB_PACKAGE_DIR="$(cd .. && pwd)/amlogic-dvb"

    check_dependencies
    clone_libreelec
    install_dvb_package
    configure_build
    build_image
    show_results

    log_success "Gotowe! Flashuj i ciesz się DVB!"
}

print_usage() {
    cat << EOF
Użycie: $0 [OPCJE]

Opcje:
  -h, --help           Pomoc
  -c, --clean          Wyczyść przed budowaniem
  -b, --branch BRANCH  Gałąź LibreELEC (np. 12.0)
  -a, --arch ARCH      aarch64 (domyślnie)
  -d, --device DEV     M8S (S905D), generic
  --deps-only          Tylko sprawdź zależności

Przykład:
  $0 -b 12.0 -c
EOF
}

main "$@"
