CXXFLAGS_ALL = $(shell pkg-config --cflags --static sdl2 vorbisfile vorbis) $(CXXFLAGS) \
               -DBASE_PATH='"$(BASE_PATH)"'

LDFLAGS_ALL = $(LDFLAGS)
LIBS_ALL = $(shell pkg-config --libs --static sdl2 vorbisfile vorbis) $(LIBS)

SOURCES = RSDKv4/Animation.cpp     \
          RSDKv4/Audio.cpp         \
          RSDKv4/Collision.cpp     \
          RSDKv4/Debug.cpp         \
          RSDKv4/Drawing.cpp       \
          RSDKv4/Ini.cpp           \
          RSDKv4/Input.cpp         \
          RSDKv4/main.cpp          \
          RSDKv4/Math.cpp          \
          RSDKv4/Object.cpp        \
          RSDKv4/Palette.cpp       \
          RSDKv4/PauseMenu.cpp     \
          RSDKv4/Reader.cpp        \
          RSDKv4/RetroEngine.cpp   \
          RSDKv4/RetroGameLoop.cpp \
          RSDKv4/Scene.cpp         \
          RSDKv4/Scene3D.cpp       \
          RSDKv4/Script.cpp        \
          RSDKv4/Sprite.cpp        \
          RSDKv4/String.cpp        \
          RSDKv4/Text.cpp          \
          RSDKv4/Userdata.cpp      \
	  
ifneq ($(FORCE_CASE_INSENSITIVE),)
	CXXFLAGS_ALL += -DFORCE_CASE_INSENSITIVE
	SOURCES += RSDKv4/fcaseopen.c
endif

# Drawing Drivers
SOURCES += \
        RSDKv4/drivers/drawing/drawing_SDL1.cpp     \
        RSDKv4/drivers/drawing/drawing_SDL2.cpp     \
        RSDKv4/drivers/drawing/drawing_GL.cpp       \
        RSDKv4/drivers/drawing/drawing_PS2.cpp      \
        RSDKv4/drivers/drawing/drawing_NULL.cpp     \

# Input Drivers
SOURCES += \
        RSDKv4/drivers/input/input_SDL1.cpp     \
        RSDKv4/drivers/input/input_SDL2.cpp     \
        RSDKv4/drivers/input/input_PS2.cpp      \
        RSDKv4/drivers/input/input_NULL.cpp     \

CXXFLAGS_ALL += -O0 -g

objects/%.o: %
	mkdir -p $(@D)
	$(CXX) $(CXXFLAGS_ALL) -std=c++17 $^ -o $@ -c

bin/RSDKv4: $(SOURCES:%=objects/%.o)
	mkdir -p $(@D)
	$(CXX) $(CXXFLAGS_ALL) $(LDFLAGS_ALL) $^ -o $@ $(LIBS_ALL)

install: bin/RSDKv4
	install -Dp -m755 bin/RSDKv4 $(prefix)/bin/RSDKv4

clean:
	rm -r -f bin/RSDKv4 && rm -r -f objects
