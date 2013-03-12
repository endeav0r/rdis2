OBJS=object.o util.o rdis.o

CFLAGS=-Wall -Werror -g
INCLUDE=-iquotecontainer/ -iquote./ -iquotearch/ -iquoteloader/
LIBS=-ludis86

all : $(OBJS)
	make -C container
	make -C loader
	make -C arch

	gcc *.o container/*.o arch/*.o loader/*.o -o rdis $(LIBS) $(CFLAGS)

%.o : %.c %.h
	$(CC) -c -o $@ $< $(INCLUDE) $(CFLAGS)

%.o : %.c
	$(CC) -c -o $@ $< $(INCLUDE) $(CFLAGS)

clean :
	make -C container clean
	make -C loader clean
	make -C arch clean
	rm -f *.o
	rm -f rdis