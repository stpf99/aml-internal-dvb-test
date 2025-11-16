# Amlogic DVB Support for LibreELEC

## 📡 Wsparcie wbudowanych tunerów DVB dla S905D (GXL)

Pakiet dodaje pełne wsparcie dla wbudowanych tunerów DVB w urządzeniach Amlogic GXL (S905D, S905X, S905W) do LibreELEC z kernelem 6.x mainline.

### ✅ Wspierane urządzenia

| Urządzenie | SoC | Tuner | Status |
|------------|-----|-------|--------|
| **Mecool M8S Plus DVB-S2X** | S905D | M88RS6060 | ✅ Przetestowane |
| Mecool M8S DVB-S2 | S905 | AVL6211 | ⚠️ Wymaga testów |
| WeTek Play 2 | S905 | AVL6211 + MxL608 | ⚠️ Wymaga testów |
| Zgemma H9S | S905X | AVL6211 | ⚠️ Wymaga testów |

### 🎯 Funkcje

- ✅ **Serial TS mode** - dla większości urządzeń
- ✅ **Parallel TS mode** - dla niektórych urządzeń
- ✅ **Hardware demux** - akceleracja sprzętowa
- ✅ **DVB-S/S2/S2X** - wsparcie M88RS6060
- ✅ **DVB-T/T2/C** - wsparcie AVL6211 i innych
- ✅ **TVHeadend** - pełna kompatybilność
- ✅ **Kodi PVR** - Live TV przez TVHeadend

---

## 🚀 Instalacja

### Krok 1: Sklonuj LibreELEC

```bash
git clone https://github.com/LibreELEC/LibreELEC.tv.git
cd LibreELEC.tv
git checkout master  # lub konkretny branch (12.x, 13.x)
```

### Krok 2: Dodaj pakiet amlogic-dvb

```bash
# Skopiuj katalog amlogic-dvb do packages/linux-drivers/
cp -r /path/to/amlogic-dvb packages/linux-drivers/

# Lub sklonuj z repo (gdy będzie dostępne)
git clone https://github.com/YOUR_REPO/libreelec-amlogic-dvb.git \
    packages/linux-drivers/amlogic-dvb
```

### Krok 3: Zaaplikuj patches kernela

```bash
# Dodaj patch do projektu Amlogic
cp packages/linux-drivers/amlogic-dvb/patches/*.patch \
   projects/Amlogic/patches/linux/
```

### Krok 4: Build LibreELEC

```bash
# Dla S905D/S905X (Generic AMLGX)
PROJECT=Amlogic DEVICE=AMLGX ARCH=arm make image

# Alternatywnie dla 64-bit
PROJECT=Amlogic DEVICE=AMLGX ARCH=aarch64 make image
```

### Krok 5: Flash image

```bash
# Znajdź wygenerowany image
ls -lh target/*.img.gz

# Flash na SD/USB (Linux)
gunzip -c target/LibreELEC-AMLGX.arm-*.img.gz | \
    dd of=/dev/sdX bs=4M status=progress conv=fsync

# Gdzie /dev/sdX to Twoja karta SD/USB
```

---

## ⚙️ Konfiguracja

### Device Tree

Edytuj `/flash/config.ini` (lub `/flash/uEnv.ini`):

```ini
# Dla Mecool M8S Plus DVB-S2X
dtb_name=meson-gxl-s905d-p231-dvb-s2.dtb

# lub generic
dtb_name=meson-gxl-s905d-p231.dtb
```

### TS Mode Configuration

Edytuj `/storage/.config/amlogic-dvb.conf`:

```bash
# Transport Stream mode
# 0 = auto-detect
# 1 = serial (domyślny dla większości urządzeń)
# 2 = parallel
TS_MODE=1

# Debug level (0-5, wyższy = więcej logów)
DEBUG_LEVEL=1

# Enable hardware descrambler dla CAM/CI
ENABLE_DSC=1
```

### Ręczne ładowanie modułów (debug)

```bash
# SSH do LibreELEC
ssh root@libreelec

# Załaduj moduły ręcznie
modprobe aml_dvb
modprobe aml_dmx
modprobe aml_ts

# Sprawdź czy adapter się pojawił
ls -la /dev/dvb/

# Powinno pokazać:
# /dev/dvb/adapter0/demux0
# /dev/dvb/adapter0/dvr0
# /dev/dvb/adapter0/frontend0
# /dev/dvb/adapter0/net0
```

---

## 🔧 Troubleshooting

### Problem 1: Brak /dev/dvb/

**Sprawdź:**

```bash
# Czy moduły są załadowane?
lsmod | grep aml

# Jeśli nie, załaduj ręcznie
modprobe aml_dvb

# Sprawdź logi kernela
dmesg | grep -i "dvb\|aml_dvb"
```

**Możliwe przyczyny:**
- ❌ Zły DTB - sprawdź `config.ini`
- ❌ Moduły nie skompilowane - rebuild LibreELEC
- ❌ Brak wsparcia w kernelu - sprawdź czy pakiet jest włączony

### Problem 2: TVHeadend nie widzi tunera

**Sprawdź:**

```bash
# Czy adapter jest widoczny?
ls -la /dev/dvb/adapter*/

# Czy frontend odpowiada?
dvb-fe-tool -a 0

# Logi TVHeadend
journalctl -u tvheadend -f
```

**Rozwiązanie:**
1. Restart TVHeadend: `systemctl restart tvheadend`
2. W TVHeadend Web UI: Configuration → DVB Inputs → Networks
3. Dodaj nową sieć (DVB-S dla satelity, DVB-T dla naziemnej)
4. Scan mux

### Problem 3: "No signal" w TVHeadend

**Dla DVB-S/S2 (satelita):**

```bash
# Sprawdź LNB power
cat /sys/class/lnb/lnb0/voltage  # Powinno być 13000 lub 18000

# Sprawdź DiSEqC
dvb-fe-tool -a 0 -d UNIVERSAL  # Test DiSEqC

# Manual tune test
dvbv5-zap -a 0 -c channels.conf "Channel Name"
```

**Dla DVB-T (naziemna):**

```bash
# Scan dla dostępnych multiplex
w_scan -f t -c PL > channels.txt

# Test tuningu
dvbv5-zap -a 0 -c channels.txt "TVP1 HD"
```

### Problem 4: Serial vs Parallel TS

**Jak sprawdzić który tryb używasz?**

```bash
# Z Androida (przed instalacją LibreELEC):
adb shell dmesg | grep -i "ts.*mode"

# Z LibreELEC:
cat /sys/class/aml_dvb/aml_dvb0/ts_mode
# 1 = serial
# 2 = parallel
```

**Zmiana trybu:**

Edytuj `/storage/.config/amlogic-dvb.conf`:
```bash
TS_MODE=1  # zmień na 2 dla parallel
```

Reboot:
```bash
reboot
```

### Problem 5: Zewnętrzny tuner USB NIE DZIAŁA jednocześnie

**To jest znany problem!** Wbudowany tuner używa `/dev/dvb/adapter0`, a tuner USB powinien być `adapter1`.

**Diagnostyka:**

```bash
# Podłącz tuner USB
lsusb | grep -i dvb

# Sprawdź czy się pojawił
ls -la /dev/dvb/

# Powinny być:
# adapter0/ - wbudowany
# adapter1/ - USB

# Sprawdź logi
dmesg | tail -30
```

**Jeśli nie działa:**
- Sprawdź czy sterownik USB DVB jest włączony w kernelu
- Może wymagać dodatkowego firmware
- Zobacz: https://github.com/LibreELEC/dvb-firmware

---

## 📊 Diagnostyka zaawansowana

### Pełny raport diagnostyczny

Uruchom ten skrypt aby zebrać wszystkie informacje:

```bash
cat > /storage/dvb-debug.sh << 'EOF'
#!/bin/bash
echo "=== Amlogic DVB Diagnostics ==="
echo "Date: $(date)"
echo ""

echo "=== System ==="
cat /etc/os-release | head -5
uname -a
echo ""

echo "=== Devices ==="
ls -laR /dev/dvb/ 2>/dev/null || echo "No DVB devices"
echo ""

echo "=== Modules ==="
lsmod | grep -E "aml|dvb|frontend"
echo ""

echo "=== TS Mode ==="
cat /sys/class/aml_dvb/*/ts_mode 2>/dev/null || echo "N/A"
echo ""

echo "=== Kernel logs ==="
dmesg | grep -i "dvb\|aml_dvb\|frontend" | tail -50
echo ""

echo "=== Frontend info ==="
for fe in /dev/dvb/adapter*/frontend*; do
    [ -e "$fe" ] && dvb-fe-tool -f "$fe" 2>&1
done
EOF

chmod +x /storage/dvb-debug.sh
/storage/dvb-debug.sh > /storage/dvb-debug.txt

# Wyświetl
cat /storage/dvb-debug.txt
```

### Test performance

```bash
# Test przepustowości TS
cat /dev/dvb/adapter0/dvr0 > /dev/null &
PID=$!
sleep 10
kill $PID

# Sprawdź w dmesg czy były błędy
dmesg | tail -20
```

---

## 🌟 Znane ograniczenia

### ⚠️ Obecnie NIE działa:

1. **Jednoczesne użycie wbudowanego + USB tunera** w kernel 6.x
   - Wymaga dodatkowych zmian w demux
   - Alternatywa: użyj CoreELEC 4.9 dla wbudowanego + LibreELEC 6.x dla USB

2. **CAM/CI descrambler** (warunkowy dostęp)
   - Kod istnieje ale wymaga testów
   - Większość użytkowników używa softCAM (oscam)

3. **DiSEqC switch** (przełącznik satelitarny)
   - Podstawowe wsparcie działa
   - Zaawansowane (DiSEqC 1.2/1.3/USALS) wymaga testów

### ✅ CO DZIAŁA:

- ✅ Podstawowy tuning DVB-S/S2/S2X
- ✅ Scanning multiplex
- ✅ TVHeadend integration
- ✅ Kodi Live TV
- ✅ Recording
- ✅ EPG (Electronic Program Guide)
- ✅ Timeshift

---

## 🔗 Linki i zasoby

### Źródła kodu:
- **CoreELEC 22** (mczerski): https://github.com/CoreELEC/CoreELEC
- **chewitt/linux dvb-sucks-more**: https://github.com/chewitt/linux/tree/dvb-sucks-more
- **Availink drivers**: https://github.com/availink/amlogic_meson_dvb4linux

### Firmware:
- **LibreELEC DVB Firmware**: https://github.com/LibreELEC/dvb-firmware
- **CoreELEC DVB Firmware**: https://github.com/CoreELEC/dvb-firmware

### Forum wsparcia:
- **LibreELEC Forum**: https://forum.libreelec.tv
- **CoreELEC Forum**: https://discourse.coreelec.org

### Dokumentacja:
- **Linux DVB API**: https://linuxtv.org/downloads/v4l-dvb-apis/
- **TVHeadend**: https://tvheadend.org/

---

## 👥 Credits

- **mczerski** - Główna praca nad portowaniem DVB do CoreELEC 22
- **chewitt** - Frontend drivers i pinctrl dla mainline
- **Availink** - AVL62x1/AVL68x2 drivers
- **CoreELEC Team** - Baza kodu i testy
- **LibreELEC Team** - Build system i infrastruktura

---

## 📝 License

GPL-2.0+ (zgodnie z licencją Linux kernel)

---

## 🐛 Zgłaszanie błędów

Jeśli znajdziesz bug lub masz problem:

1. Uruchom diagnostykę: `/storage/dvb-debug.sh`
2. Zbierz logi: `dmesg > /storage/dmesg.log`
3. Utwórz issue na GitHub z:
   - Model urządzenia (np. Mecool M8S Plus DVB-S2X)
   - Wersja LibreELEC
   - Pełny log diagnostyczny
   - Opis problemu

---

## 🎉 Status projektu

**Wersja:** 1.0-beta  
**Data:** Listopad 2025  
**Status:** Eksperymentalny - wymaga testów społeczności  

**TODO:**
- [ ] Testy na różnych urządzeniach GXL
- [ ] Optymalizacja wydajności demux
- [ ] Wsparcie dla CAM/CI
- [ ] Dokumentacja dla developerów
- [ ] Upstream do oficjalnego LibreELEC

---

**Miłego oglądania TV! 📺**
