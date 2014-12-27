PREFIX_DIR = $(CURDIR)/Build/Prefix

dosbox: dosbox-0.74/src/dosbox
	cp dosbox-0.74/src/dosbox .

dosbox-0.74/src/dosbox: $(PREFIX_DIR)/bin/sdl-config dosbox-0.74
	cd dosbox-0.74 && ./configure --enable-debug=heavy --prefix=$(PREFIX_DIR) --with-sdl-prefix=$(PREFIX_DIR) CPPFLAGS=-DNCURSES_OPAQUE=0
	cd dosbox-0.74 && make

$(PREFIX_DIR)/bin/sdl-config: Tarballs/SDL-1.2.15.tar.gz
	rm -fr Build/SDL && mkdir -p Build/SDL
	tar -xzf Tarballs/SDL-1.2.15.tar.gz -C Build/SDL
	cd Build/SDL/SDL-1.2.15 && ./configure --prefix="$(PREFIX_DIR)" --without-x
	patch -p1 < Patches/SDL/001.fix-compilation.patch
	cd Build/SDL/SDL-1.2.15 && make && make install
	patch -p1 < Patches/SDL/002.force-static-libs.patch

