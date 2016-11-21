
PKGS    = MagickCore MagickWand
CFLAGS  = -g -Wall -O2 -std=c99 -D_GNU_SOURCE $(shell pkg-config --cflags $(PKGS)) -I.
LDFLAGS = $(shell pkg-config --libs $(PKGS)) shapelib-1.3.0/libshp.a

OBJS = gview.o fileutil.o image.o colors.o paths.o

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.x: $(OBJS)
	$(CC) $^ -o $@ $(LDFLAGS)

all: gview.x

clean:
	rm -f gview.x $(OBJS)

fileutil.o: fileutil.c fileutil.h
image.o: image.c image.h fileutil.h colors.h
colors.o: colors.c colors.h image.h
paths.o: paths.c paths.h image.h
gview.o: gview.c colors.h paths.h image.h
