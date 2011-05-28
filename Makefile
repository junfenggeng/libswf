SRCS = $(wildcard src/*.c) samples/demo.c
OBJS = $(SRCS:.c=.o)

CFLAGS = -Iinclude

PROG = demo

.c.o:
	gcc $(CFLAGS) -c $^ -o $@ -g

all: $(OBJS)
	gcc -o $(PROG) $(OBJS)

clean:
	rm -f $(OBJS) $(PROG)

cleanall: clean
	rm -f *~ */*~
