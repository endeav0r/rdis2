OBJS=x86.o recursive_dis.o

INCLUDE=-iquote../ -iquote../container -iquote../arch
CFLAGS=-Wall -Werror -g

all : $(OBJS)

%.o : %.c %.h
	$(CC) -c -o $@ $< $(INCLUDE) $(CFLAGS)

clean :
	rm -f *.o