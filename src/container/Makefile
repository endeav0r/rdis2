OBJS=buffer.o function.o graph.o index.o instruction.o list.o map.o queue.o tree.o

INCLUDE=-iquote../
CFLAGS=-Wall -Werror -g

all : $(OBJS)

%.o : %.c %.h
	$(CC) -c -o $@ $< $(INCLUDE) $(CFLAGS)

clean :
	rm -f *.o