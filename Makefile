SRCS = $(wildcard src/*.c) samples/demo.c
OBJS = $(SRCS:.c=.o)

CFLAGS = -Iinclude `pkg-config gtk+-2.0 --cflags`
LDFLAGS = `pkg-config gtk+-2.0 --libs`

PROG = demo

.c.o:
	gcc $(CFLAGS) -c $^ -o $@ -g

all: $(OBJS)
	gcc $(LDFLAGS) -o $(PROG) $(OBJS)

clean:
	rm -f $(OBJS) $(PROG)

cleanall: clean
	rm -f *~ */*~
