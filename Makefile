CC=gcc
LD=gcc

CFLAGS = -Wall -c -g -O3 `pkg-config --cflags gtk+-2.0`
LDFLAGS = `pkg-config --libs gtk+-2.0` -lz -lpng -ljpeg

CFILES = main.c memory.c png.c editor.c common.c palette.c tiled.c jpg.c bmp.c string.c pcx.c prefs.c exit.c tga.c
HFILES = main.h memory.h png.h editor.h common.h palette.h tiled.h jpg.h bmp.h string.h pcx.h prefs.h exit.h tga.h
OFILES = main.o memory.o png.o editor.o common.o palette.o tiled.o jpg.o bmp.o string.o pcx.o prefs.o exit.o tga.o
EXECUT = tiledriller

ifeq ($(PREFIX),)
	PREFIX := /usr/local
endif


all: $(OFILES) Makefile
	$(LD) $(OFILES) -o $(EXECUT) -lm $(LDFLAGS) ; strip $(EXECUT)


main.o: main.c defines.h
	$(CC) $(CFLAGS) main.c

memory.o: memory.c defines.h
	$(CC) $(CFLAGS) memory.c

png.o: png.c defines.h
	$(CC) $(CFLAGS) png.c

editor.o: editor.c defines.h
	$(CC) $(CFLAGS) editor.c

common.o: common.c defines.h
	$(CC) $(CFLAGS) common.c

palette.o: palette.c defines.h
	$(CC) $(CFLAGS) palette.c

tiled.o: tiled.c defines.h
	$(CC) $(CFLAGS) tiled.c

jpg.o: jpg.c defines.h
	$(CC) $(CFLAGS) jpg.c

bmp.o: bmp.c defines.h
	$(CC) $(CFLAGS) bmp.c

string.o: string.c defines.h
	$(CC) $(CFLAGS) string.c

pcx.o: pcx.c defines.h
	$(CC) $(CFLAGS) pcx.c

prefs.o: prefs.c defines.h
	$(CC) $(CFLAGS) prefs.c

exit.o: exit.c defines.h
	$(CC) $(CFLAGS) exit.c

tga.o: tga.c defines.h
	$(CC) $(CFLAGS) tga.c


$(OFILES): $(HFILES)


clean:
	rm -f $(OFILES) core *~ $(EXECUT)

nice:
	rm -r *~

install: all
	install -Dm755 $(EXECUT) $(DESTDIR)$(PREFIX)/bin/$(EXECUT)
	install -Dm644 tiledriller.desktop $(DESTDIR)$(PREFIX)/share/applications/tiledriller.desktop
