# Makefile for compiling the Atmel software package into library files

# Default target
libs:

TOP := .

include $(TOP)/scripts/Makefile.vars

# Additional peripheral that not included to Makefile.vars
CONFIG_HAVE_ISC = y

BUILDDIR ?= ./build
LIBPREFIX ?= lib$(TARGET)-

include $(TOP)/utils/Makefile.inc
include $(TOP)/target/Makefile.inc
include $(TOP)/drivers/Makefile.inc
include $(TOP)/lib/Makefile.inc
include $(TOP)/samba_applets/common/Makefile.inc
include $(TOP)/scripts/Makefile.config

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

copy_libs:
	cp -t $(TOP) $(LIBS)

$(TOP)/$(addprefix $(LIBPREFIX),%.a): %.a
	mv $< $@

install: copy_libs $(LIBSINSTALL)

clean:
	@rm -rf $(BUILDDIR) $(LIBSINSTALL) settings

print:
	$(ECHO) $(LIBSINSTALL)
