# Makefile for compiling the Atmel software package into library files

# Default target
libs:

TOP := .

include $(TOP)/scripts/Makefile.vars

BUILDDIR ?= ./build

include $(TOP)/utils/Makefile.inc
include $(TOP)/target/Makefile.inc
include $(TOP)/drivers/Makefile.inc
include $(TOP)/lib/Makefile.inc
include $(TOP)/samba_applets/common/Makefile.inc
include $(TOP)/scripts/Makefile.config

vpath %.c $(TOP)
vpath %.S $(TOP)

LIBS := $(addprefix $(BUILDDIR)/,$(lib-y))

.PHONY: libs clean

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

clean:
	@rm -rf $(BUILDDIR) settings

print:
	@echo $(OBJS:.o=.d)
