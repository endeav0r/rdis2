OBJS=arm.o x86.o recursive_dis.o

INCLUDE=-iquote../ -iquote../container -iquote../arch
CFLAGS=-Wall -Werror -g

all : copy_darm $(OBJS)

copy_darm :
	mkdir -p darm
	cp ../../darm/darm.h darm/
	cp ../../darm/armv7-tbl.h darm/

%.o : %.c %.h
	$(CC) -c -o $@ $< $(INCLUDE) $(CFLAGS)

clean :
	rm -f *.o
	rm -rf darm