# Makefile for compiling the Atmel software package into library files

# Default target
libs:

TOP := .

include $(TOP)/scripts/Makefile.vars

# Additional peripheral that not included to Makefile.vars
CONFIG_HAVE_ISC = y
CONFIG_IMAGE_SENSOR = y
CONFIG_ISC = y
CONFIG_LCDC = y
CONFIG_LED = y
CONFIG_SPI = y
CONFIG_QSPI = y

BUILDDIR ?= ./build
LIBPREFIX ?= lib$(TARGET)-$(VARIANT)-
#LIBPREFIX ?= lib$(TARGET)-

include $(TOP)/samba_applets/common/Makefile.inc
include $(TOP)/scripts/Makefile.config
include $(TOP)/utils/Makefile.inc
include $(TOP)/target/Makefile.inc
include $(TOP)/drivers/Makefile.inc
include $(TOP)/lib/Makefile.inc

vpath %.c $(TOP)
vpath %.S $(TOP)

LIBS := $(addprefix $(BUILDDIR)/,$(lib-y))
LIBSINSTALL := $(addprefix $(LIBPREFIX),$(notdir $(LIBS)))

.PHONY: libs clean install copy_libs

$(BUILDDIR):
	@mkdir -p $(BUILDDIR)

$(BUILDDIR)/%.d: %.c
	@mkdir -p $(dir $@)
	$(ECHO) DEP $<
	$(Q)$(CC) $(CFLAGS) $(CFLAGS_CPU) $(CFLAGS_INC) $(CFLAGS_DEFS) -MM $< -MT $(basename $@).o -o $(basename $@).d

$(BUILDDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(ECHO) CC $<
	$(Q)$(CC) $(CFLAGS) $(CFLAGS_CPU) $(CFLAGS_INC) $(CFLAGS_DEFS) -c $< -o $@

$(BUILDDIR)/%.d: %.S
	@mkdir -p $(dir $@)
	$(ECHO) DEP $<
	$(Q)$(CC) $(CFLAGS_ASM) $(CFLAGS_CPU) $(CFLAGS_INC) $(CFLAGS_DEFS) -MM $< -MT $(basename $@).o -o $(basename $@).d

$(BUILDDIR)/%.o: %.S
	@mkdir -p $(dir $@)
	$(ECHO) CC $<
	$(Q)$(CC) $(CFLAGS_ASM) $(CFLAGS_CPU) $(CFLAGS_INC) $(CFLAGS_DEFS) -c $< -o $@

libs: $(LIBS) 

install: libs
	@for i in $(LIBS) ; do cp -v $$i $(TOP) ; done ;
	@for i in $(notdir $(LIBS)) ; do mv -v $$i $(LIBPREFIX)$$i ; done ;
	
clean:
	@rm -rf $(BUILDDIR) settings

print:
	$(ECHO) $(LIBS)

test:
	@for i in $(notdir $(LIBS)) ; do echo $$i ; done ;
	