PREFIX ?= "${HOME}"/.local
CC ?= cc
LDFLAGS = -lX11

debug: dwmblocks.c
	rm debug && clang dwmblocks.c $(LDFLAGS) -Wextra -g -o debug

output: dwmblocks.c
	${CC}  dwmblocks.c $(LDFLAGS) -o dwmblocks

clean:
	rm -f dwmblocks

install: output
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install -m 0755 dwmblocks $(DESTDIR)$(PREFIX)/bin/dwmblocks

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/dwmblocks
