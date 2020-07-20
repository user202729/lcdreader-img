PRODUCT := main
SRCDIR := src
DEPDIR := dep
BINDIR := bin
OBJDIR := obj

USE_X11 ?= 1

INCLUDEDIRS :=

CXX ?= g++
CCEXT ?= cpp
CCFLAGS1 := -fdiagnostics-color=always -O2 -pipe -g -std=c++17 -Wall -Wextra -Werror -pedantic `pkg-config --cflags opencv4`

CCLINK1 := -pipe `pkg-config --libs opencv4`

ifeq ($(USE_X11), 1)
	CCFLAGS1 += -DUSE_X11
	CCLINK1 += -lX11
endif


CCFLAGS ?= $(CCFLAGS1)
CCLINK ?= $(CCLINK1)

FILES := $(shell find $(SRCDIR) -name *.$(CCEXT))

# RELATIVEPATH only affects GCC output

$(BINDIR)/$(PRODUCT): $(patsubst %,$(OBJDIR)/%.o,$(subst /,__,$(patsubst $(SRCDIR)/%,%,$(FILES)))) $(LIBSTOLINK) Makefile
	@$(CXX) -o $(BINDIR)/$(PRODUCT) $(patsubst %,$(OBJDIR)/%.o,$(subst /,__,$(patsubst $(SRCDIR)/%,%,$(FILES)))) $(CCLINK)

-include $(patsubst %,$(DEPDIR)/%.dep,$(subst /,__,$(patsubst $(SRCDIR)/%,%,$(FILES))))

$(OBJDIR)/%.$(CCEXT).o: Makefile
	@mkdir -p bin/ dep/ obj/
	@$(eval CPATH := $(patsubst %,$(SRCDIR)/%,$(subst __,/,$*.$(CCEXT))))
	@$(eval DPATH := $(patsubst %,$(DEPDIR)/%,$*.$(CCEXT)).dep)
	@$(eval RELBACK := `echo $(RELATIVEPATH) | sed -e 's/[^/][^/]*/\.\./g'`)
	@cd $(RELATIVEPATH). && $(CXX) $(CCFLAGS) $(subst -I,-I$(RELBACK),$(INCLUDEDIRS)) -c ./$(RELBACK)$(CPATH) -o ./$(RELBACK)$(OBJDIR)/$*.$(CCEXT).o
	@cd $(RELATIVEPATH). && $(CXX) $(CCFLAGS) $(subst -I,-I$(RELBACK),$(INCLUDEDIRS)) -MM ./$(RELBACK)$(CPATH) > ./$(RELBACK)$(DPATH)
	@mv -f $(DPATH) $(DPATH).tmp
	@sed -e 's|.*:|$(OBJDIR)/$*.$(CCEXT).o:|' < $(DPATH).tmp > $(DPATH)
	@sed -e 's/.*://' -e 's/ .$$//' -e 's/^ *//' -e 's/ *$$//' -e 's/$$/:/' < $(DPATH).tmp >> $(DPATH)
	@rm -f $(DPATH).tmp

clean:
	@rm -f $(BINDIR)/$(PRODUCT) $(OBJDIR)/*.o $(DEPDIR)/*.dep
