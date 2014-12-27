PREFIX_DIR = $(CURDIR)/Build/Prefix

dosbox: $(PREFIX_DIR)/bin/dosbox
	cp $(PREFIX_DIR)/bin/dosbox .

$(PREFIX_DIR)/bin/dosbox: $(PREFIX_DIR)/bin/sdl-config
	tar -xzf Tarballs/dosbox-0.74.tar.gz
	cd dosbox-0.74 && ./configure --enable-debug=heavy --prefix=$(PREFIX_DIR) --with-sdl-prefix=$(PREFIX_DIR) CPPFLAGS=-DNCURSES_OPAQUE=0
	cd dosbox-0.74 && make && make install

$(PREFIX_DIR)/bin/sdl-config: Tarballs/SDL-1.2.15.tar.gz
	rm -fr Build/SDL && mkdir -p Build/SDL
	tar -xzf Tarballs/SDL-1.2.15.tar.gz -C Build/SDL
	cd Build/SDL/SDL-1.2.15 && ./configure --prefix="$(PREFIX_DIR)" --without-x
	patch -p1 < Patches/SDL/001.fix-compilation.patch
	cd Build/SDL/SDL-1.2.15 && make && make install
	patch -p1 < Patches/SDL/002.force-static-libs.patch

