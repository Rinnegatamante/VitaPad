TARGET		:= VitaPad
TITLE		:= VPAD00001
SOURCES		:= source
INCLUDES	:= include

LIBS = -lvita2d -lSceLibKernel_stub -lSceSysmodule_stub -lSceCtrl_stub -lSceTouch_stub \
	-lm -lSceNet_stub -lSceNetCtl_stub -lSceAppUtil_stub -lScePgf_stub \
	-ljpeg -lfreetype -lc -lScePower_stub -lSceCommonDialog_stub -lpng16 -lz \
	-lSceGxm_stub -lSceDisplay_stub -lSceAppMgr_stub

CFILES   := $(foreach dir,$(SOURCES), $(wildcard $(dir)/*.c))
CPPFILES   := $(foreach dir,$(SOURCES), $(wildcard $(dir)/*.cpp))
BINFILES := $(foreach dir,$(DATA), $(wildcard $(dir)/*.bin))
OBJS     := $(addsuffix .o,$(BINFILES)) $(CFILES:.c=.o) $(CPPFILES:.cpp=.o) 

PREFIX  = arm-vita-eabi
CC      = $(PREFIX)-gcc
CXX      = $(PREFIX)-g++
CFLAGS  = -Wl,-q -O3 -Wall
CXXFLAGS  = $(CFLAGS) -fno-exceptions -std=gnu++11
ASFLAGS = $(CFLAGS)

all: $(TARGET).vpk

$(TARGET).vpk: $(TARGET).velf
	vita-make-fself -s -c $< ./build/eboot.bin
	vita-mksfoex -s TITLE_ID=$(TITLE) "$(TARGET)" param.sfo
	cp -f param.sfo ./build/sce_sys/param.sfo
	vita-pack-vpk -s ./build/sce_sys/param.sfo -b ./build/eboot.bin \
				  --add ./build/sce_sys/icon0.png=sce_sys/icon0.png \
				  --add ./build/sce_sys/livearea/contents/bg.png=sce_sys/livearea/contents/bg.png \
				  --add ./build/sce_sys/livearea/contents/startup.png=sce_sys/livearea/contents/startup.png \
				  --add ./build/sce_sys/livearea/contents/template.xml=sce_sys/livearea/contents/template.xml \
				  ./$(TARGET).vpk
	
%.velf: %.elf
	$(PREFIX)-strip -g $<
	vita-elf-create $< $@

$(TARGET).elf: $(OBJS)
	$(CXX) $(CXXFLAGS) $^ $(LIBS) -o $@

clean:
	@rm -rf $(TARGET).velf $(TARGET).elf $(OBJS)
