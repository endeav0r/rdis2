OBJS=object.o util.o rdg.o rdg_node.o rdis.o

CFLAGS=-Wall -Werror -g
INCLUDE=-iquotecontainer/ -iquote./ -iquotearch/ -iquoteloader/ `pkg-config --cflags cairo`
LIBS=-ludis86 `pkg-config --libs cairo gtk+-3.0` ../darm/libdarm.a -lm

all : $(OBJS)
	make -C container
	make -C loader
	make -C arch
	make -C gui

	gcc *.o container/*.o arch/*.o loader/*.o -o rdis $(LIBS) $(CFLAGS)
	gcc object.o util.o rdg.o rdg_node.o gui/*.o container/*.o arch/*.o loader/*.o -o rdis_gui $(LIBS) $(CFLAGS)

%.o : %.c %.h
	$(CC) -c -o $@ $< $(INCLUDE) $(CFLAGS)

%.o : %.c
	$(CC) -c -o $@ $< $(INCLUDE) $(CFLAGS)

clean :
	make -C container clean
	make -C loader clean
	make -C arch clean
	make -C gui clean
	rm -f *.o
	rm -f rdis
	rm -f rdis_gui