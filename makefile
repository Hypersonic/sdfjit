MACHINE = $(shell uname -s)
SOURCES = src
BUILD   = build
DEVEXT  = -dev
RELEXT  = -release

.PHONY: all clean

CXXFLAGS  += -march=native -fPIC -fno-rtti -Werror -Wall -Wextra -Wfloat-equal -Wshadow -Wcast-align -Wunreachable-code -Wunused-variable -std=c++17 -Isrc/

LDFLAGS += 

ifeq ($(MACHINE), Linux) # Linux-specific setup
	LIBS = 
	CFLAGS += -DUNIX -DLINUX -lm
else
	ifeq ($(MACHINE), Darwin) # OSX specific setup
		LIBS = 
		CFLAGS += -DUNIX -DDARWIN
	else
		# TODO: Windows "support"
	endif
endif

MKDIR  = mkdir -p
RM     = rm -rf

MODE = devel
BNRY = sdfjit
SDRS = $(shell find $(SOURCES) -type d | xargs echo)
SRCS = $(filter-out %.inc.cpp,$(foreach d,$(SDRS),$(wildcard $(addprefix $(d)/*,.cpp))))
OBJS = $(patsubst %.cpp,%.o,$(addprefix $(BUILD)/$(MODE)/,$(SRCS)))
RES = $(shell find res -type f | xargs echo)
DEPS = $(OBJS:%.o=%.d)
DIRS = $(sort $(dir $(OBJS)))

ifdef DEBUG
	BNRY := $(BNRY)$(DEVEXT)
	CXXFLAGS += -O1 -g -DDEBUG_MODE -fsanitize=undefined -fsanitize=address
	MODE = debug
else ifdef RELEASE
	BNRY := $(BNRY)$(RELEXT)
	CXXFLAGS += -DRELEASE -O3 -flto
	MODE = release
else
	CXXFLAGS += -O3 -g
	MODE = devel
endif

ifdef PROFILE
	CXXFLAGS += -pg
endif

run: all
	./$(BNRY)

all: game 

game: $(BNRY)
	
clean:
	$(RM) $(BUILD) $(BNRY) $(BNRY)$(DEVEXT) $(BNRY)$(RELEXT) jits frames

$(DIRS):
	$(MKDIR) $@

$(BNRY): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $@ $(LIBS)

$(OBJS): | $(DIRS)

$(BUILD)/$(MODE)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -MMD -MP -c $< -o $@

-include $(DEPS)
