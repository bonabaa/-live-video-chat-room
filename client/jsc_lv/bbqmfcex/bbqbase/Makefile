NAME	= bbqbase
AR      = $(CROSS)ar
ARFLAGS = crv
RANLIB = $(CROSS)ranlib

VERSION	= 1.5.2011

SRC	= md5.cxx \
	log.cxx \
	thread.cxx \
	bytepack.cxx \
	byteblock.cxx \
	bbqidmsg.cxx \
	msgconnection.cxx \
	msgterminal.cxx \
	bbqclient.cxx \
	bbqsvr.cxx \
	bbqproxy.cxx \
	bbqswitch.cxx \
	bbqsvruser.cxx \
	bbqtree.cxx \


ifneq ($(release), )
  OFLAG	= -O2
  prefix   = release-$(CROSS)
  suffix   = r
  pwtarget = opt
else
  OFLAG	= -g -DPTRACING=1
  prefix   = $(CROSS)
  suffix   = d
  pwtarget = debug
endif

CC	= $(CROSS)gcc
CXX     = $(CROSS)g++

PWLIBROOT	= ../pwlib

CFLAGS	= -pipe $(OFLAG) -Wall -g \
		-DPHAS_TEMPLATES \
		-D_SF_PWLIB \
		-I. -I$(PWLIBROOT)/include \
		-DVERSION=\"$(VERSION)\" \
		-DUNIX  

ifneq ($(CROSS), )
CFLAGS  += -DCROSS
endif

LFLAGS	= -L. -L$(PWLIBROOT)/$(CROSS)lib
	  #-L../../../third-party/lib/cppunit

LIBS	= -lpthread 

ifneq (,$(findstring arm,$(CROSS)))
ARCH = arm
else
ARCH = x86
endif

LIBS += -lpt_linux_$(ARCH)_$(suffix)_s

MKDEP	= makedepend
MKDIR	= mkdir -p
CP	= cp
LN	= ln
RM	= rm -f

ROOT	= .
OUTLIB	= $(ROOT)/$(prefix)lib
OBJDIR  = $(ROOT)/$(prefix)obj
OBJ	= $(patsubst %.cxx,$(OBJDIR)/%.o,$(SRC))


OUT	= $(OUTLIB)/lib$(NAME).a

define RULE_MAKE_TARGET_DIR
[ -d $(dir $@) ] || mkdir -p $(dir $@)
endef

all: $(OBJDIR)/.depend $(OUT)

$(OUT): $(OBJ)
	@$(RULE_MAKE_TARGET_DIR)
	$(RM)  $(OUT) 
	$(AR) $(ARFLAGS) $(OUT) $(OBJ)

.SUFFIXES: .o .c .cxx
$(OBJDIR)/%.o: %.cxx
	@$(RULE_MAKE_TARGET_DIR)
	$(CXX) $(CFLAGS) -o $@ -c $<

.PHONY: dep depned
dep depend $(OBJDIR)/.depend: $(SRC)
	@$(RULE_MAKE_TARGET_DIR)
	$(MKDEP) -p$(OBJDIR)/ -f- -- $(CFLAGS) -- $(SRC) >$(OBJDIR)/.depend 2>/dev/null

clean:
	-rm -f $(OUT) $(OBJ) core* $(OBJDIR)/.depend

cleancore:
	-rm -f core*

cleanall : clean

-include $(OBJDIR)/.depend
# DO NOT DELETE
