
CC      = mpicc
LD      = mpicc
CFLAGS  = -g -Wall -O2 -std=c11 -D_GNU_SOURCE -isystem $(PETSC_DIR)/include
LDFLAGS = -lpthread -lz -lm -L$(PETSC_DIR)/lib -lpetsc

PETSC_DIR=/usr/local/Cellar/petsc/3.7.2/real

OBJS = util.o habitat.o gflow.o nodelist.o output.o

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.x: %.o
	$(LD) $^ -o $@ $(LDFLAGS)

all: gflow.x

clean:
	rm -f gflow.x $(OBJS)


util.o: util.h
nodelist.o: nodelist.h habitat.h util.h
habitat.o: habitat.h util.h
output.o: output.h habitat.h conductance.h util.h
gflow.o: nodelist.h habitat.h util.h conductance.h output.h

gflow.x: $(OBJS)
