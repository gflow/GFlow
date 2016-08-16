
CFLAGS  = -g -Wall -O2 -std=c99 -D_GNU_SOURCE
LDFLAGS = -lz

OBJS = gview.o

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.x: $(OBJS)
	$(CC) $^ -o $@ $(LDFLAGS)

all: gview.x

clean:
	rm -f gview.x $(OBJS)
