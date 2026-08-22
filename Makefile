PREFIX := $(HOME)/.local
MAN_DIR := man
BINARY := build/mank
BINARY_SHELL := build/mksh

.PHONY: all build configure install uninstall clean

all: build

configure:
	cmake -B build

build: configure
	cmake --build build

install: build
	@echo "Installing mank..."
	install -m 755 $(BINARY) $(PREFIX)/bin/
	install -m 755  $(BINARY_SHELL) $(PREFIX)/bin/
	mkdir -p $(PREFIX)/bin/man
	cp $(MAN_DIR)/* $(PREFIX)/bin/man/
	@if ! grep -q 'MANK_MAN_PATH' ~/.bashrc; then \
		echo 'export MANK_MAN_PATH=$(PREFIX)/bin/man' >> ~/.bashrc; \
	fi
	@echo "Done. Restart your shell or run: source ~/.bashrc"

uninstall:
	@echo "Uninstalling mank..."
	rm -f $(PREFIX)/bin/mank
	rm -f $(PREFIX)/bin/mksh
	rm -rf $(PREFIX)/bin/man
	@sed -i '/MANK_MAN_PATH/d' ~/.bashrc
	@echo "Done."

clean:
	rm -rf build
