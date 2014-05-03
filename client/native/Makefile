OUT=.out
SDL2_CONFIG=sdl2-config
PNG_CONFIG=libpng-config
# GLFW_CONFIG=/home/donovan/lib/glfw3/lib/pkgconfig/glfw3.pc

CFLAGS=-I../../server/src/.out -I../.. -std=c++11 -g -Wall \
	`$(SDL2_CONFIG) --cflags` \
	`$(PNG_CONFIG) --cflags`
#`pkg-config --cflags $(GLFW_CONFIG)` 

#SDL_TTF_LFLAGS=-lSDL2_ttf

LFLAGS=-L../../hexcom -lhexcom `$(SDL2_CONFIG) --libs`  $(SDL_TTF_LFLAGS) \
	`$(PNG_CONFIG) --ldflags` \
	-lGLU -lGL -lprotobuf -ljsoncpp -lz -lpthread
#`pkg-config --libs $(GLFW_CONFIG)`

CPP_SOURCES=login.cpp main.cpp mesh.cpp world.cpp connection.cpp text_ui.cpp \
	sound.cpp skymap.cpp clientoptions.cpp overlay.cpp fieldbook.cpp

SERVER_OUT_DIR=../../server/src/.out

PROTO_SOURCES=\
   $(SERVER_OUT_DIR)/wire/hello.pb.cc \
   $(SERVER_OUT_DIR)/wire/model.pb.cc \
   $(SERVER_OUT_DIR)/wire/entity.pb.cc \
   $(SERVER_OUT_DIR)/wire/terrain.pb.cc \
   $(SERVER_OUT_DIR)/wire/major.pb.cc

OBJ_FILES=$(patsubst %.cpp,$(OUT)/%.o,$(CPP_SOURCES)) \
	  $(patsubst $(SERVER_OUT_DIR)/wire/%.cc,$(OUT)/wire/%.o,$(PROTO_SOURCES))
PROTO_H_FILES=$(patsubst $(SERVER_OUT_DIR)/wire/%.cc,$(SERVER_OUT_DIR)/wire/%.h,$(PROTO_SOURCES))

ASSET_FILES=textures/login.png textures/terrain.png

#sp: sound.cpp
#	g++ $(CFLAGS) sound.cpp -o sp $(LFLAGS) es

#mss: merge-sort-stack.cpp	
#	g++ merge-sort-stack.cpp -o mss

hc: $(OBJ_FILES) $(ASSET_FILES)
	./make-build-info > $(OUT)/build.cpp
	g++ $(OBJ_FILES) $(OUT)/build.cpp -o hc $(LFLAGS)

orbit: orbits.cpp
	g++ $(CFLAGS) orbits.cpp $(SERVER_OUT_DIR)/wire/terrain.pb.cc -o orbit $(LFLAGS)


$(OUT)/%.o: %.cpp $(PROTO_H_FILES)
	@mkdir -p $(OUT)
	g++ $(CFLAGS) -MD -c $< -o $@

$(OUT)/wire/%.o: $(SERVER_OUT_DIR)/wire/%.cc
	@mkdir -p $(OUT)/wire
	g++ $(CFLAGS) -MD -c $< -o $@

clean::
	rm -rf $(OUT) hc orbit


-include $(OUT)/*.d
-include $(OUT)/wire/*.d

#
#  Install support; adapted from Debian packaging
#

DESTDIR=
BIN=$(DESTDIR)/bin
ASSETDIR=$(DESTDIR)

TEXTURES=terrain.png toolsel.png toolbar.png turtle.png galaxy.dat 8x8font.png \
	inputbox.png login.png field-book-open.png field-book-closed.png bluebonnet2.png \
	status-hexgrid.png
SHADERS=terrain water skybox robot outline

INSTALLED_TEXTURES=$(patsubst %,$(ASSETDIR)/textures/%,$(TEXTURES))
INSTALLED_FRAG_SHADERS=$(patsubst %,$(ASSETDIR)/shaders/%.frag.glsl,$(SHADERS))
INSTALLED_VERTEX_SHADERS=$(patsubst %,$(ASSETDIR)/shaders/%.vertex.glsl,$(SHADERS))

install:: $(BIN)/hexplore \
	$(INSTALLED_TEXTURES) \
	$(INSTALLED_FRAG_SHADERS) \
	$(INSTALLED_VERTEX_SHADERS)

textures:: $(patsubst %,textures/%,$(TEXTURES))

$(BIN)/hexplore: hc
	install --strip -m 0755 -D hc $(BIN)/hexplore

$(ASSETDIR)/textures/%.png: textures/%.png
	install -m 0644 -D $< $@

$(ASSETDIR)/textures/%.dat: textures/%.dat.gz
	@mkdir -p $(ASSETDIR)/textures
	gunzip -d < $< > $@

$(ASSETDIR)/shaders/%.glsl: shaders/%.glsl
	install -m 0644 -D $< $@

textures/%.png: textures/%.xcf
	xcf2png $< -o $@

textures/field-book-open.xcf:
	curl -Lo $@ https://www.dropbox.com/s/6xmfg7vavheqhud/field-book-open.xcf

textures/field-book-closed.xcf:
	curl -Lo $@ https://www.dropbox.com/s/95q1xfg7zbveojf/field-book-closed.xcf
