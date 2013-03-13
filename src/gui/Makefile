OBJS=gui.o

INCLUDE=`pkg-config --cflags gtk+-3.0` \
        -iquote../ -iquote../container -iquote../arch -iquote../loader
CFLAGS=-Wall -Werror -g

all : $(OBJS)

%.o : %.c %.h
	$(CC) -c -o $@ $< $(INCLUDE) $(CFLAGS)

clean :
	rm -f *.o