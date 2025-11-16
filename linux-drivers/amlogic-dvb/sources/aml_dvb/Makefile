# SPDX-License-Identifier: GPL-2.0
# Makefile for Amlogic DVB drivers
# Ported from CoreELEC 22 (kernel 5.15.170) → LibreELEC (kernel 6.x)

# === Moduły ===
obj-m := aml_dvb.o aml_dmx.o aml_ts.o aml_dsc.o

# === Źródła ===
aml_dvb-objs := aml_dvb_core.o \
                aml_dvb_reg.o \
                aml_dvb_hw.o \
                aml_dvb_frontend.o

aml_dmx-objs := aml_dmx_core.o \
                aml_dmx_hw.o \
                aml_dmx_filter.o \
                aml_dmx_section.o \
                aml_dmx_pcr.o

aml_ts-objs := aml_ts_core.o \
               aml_ts_serial.o \
               aml_ts_parallel.o \
               aml_ts_dma.o

aml_dsc-objs := aml_dsc_core.o \
                aml_dsc_cam.o

# === Flagi kompilatora ===
ccflags-y := -I$(src)/include \
             -I$(srctree)/drivers/media/dvb-core \
             -I$(srctree)/drivers/media/dvb-frontends \
             -DCONFIG_AMLOGIC_DVB=1 \
             -DCONFIG_AMLOGIC_TS_SERIAL=1 \
             -DCONFIG_AMLOGIC_TS_PARALLEL=1 \
             -DMODULE_VERSION=\"6.0-gxl\"

# Debug (odkomentuj przy potrzebie)
# ccflags-y += -DDEBUG -DVERBOSE_DEBUG

# === Kompatybilność z kernel 6.x ===
KERNEL_VERSION := $(shell uname -r | cut -d. -f1)
KERNEL_PATCHLEVEL := $(shell uname -r | cut -d. -f2)

ifeq ($(shell test $(KERNEL_VERSION) -ge 6; echo $$?),0)
    ccflags-y += -DKERNEL_6X_COMPAT=1
endif

# === Środowisko budowania ===
KERNEL_SRC ?= /lib/modules/$(shell uname -r)/build
CROSS_COMPILE ?=

# === Targety ===
.PHONY: all clean install help modules

all modules:
	$(MAKE) -C $(KERNEL_SRC) \
		M=$(PWD) \
		CROSS_COMPILE=$(CROSS_COMPILE) \
		modules

clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(PWD) clean
	rm -f *.o *.ko *.mod.c *.mod *.order *.symvers *.cmd Module.symvers modules.builtin

install:
	$(MAKE) -C $(KERNEL_SRC) M=$(PWD) modules_install

help:
	@echo "Amlogic DVB Driver Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all / modules - Build all modules"
	@echo "  clean         - Remove build artifacts"
	@echo "  install       - Install modules to system"
	@echo ""
	@echo "Modules:"
	@echo "  aml_dvb.ko  - Core DVB driver"
	@echo "  aml_dmx.ko  - Hardware demultiplexer"
	@echo "  aml_ts.ko   - Transport Stream interface"
	@echo "  aml_dsc.ko  - Descrambler (CAM/CI)"
	@echo ""
	@echo "Environment:"
	@echo "  KERNEL_SRC    - Path to kernel source (default: auto)"
	@echo "  CROSS_COMPILE - Cross-compiler prefix"
