
CC = gcc
CFLAGS = -static -O2   -std=c99
LINK =	gcc
LFLAGS = 

emhttp: obj/emhttp.o obj/main.o	
	$(LINK)  -o  emhttp  obj/emhttp.o obj/main.o  $(LFLAGS)

obj/emhttp.o:	src/emhttp.c 
	$(CC) -c $(CFLAGS) -o $@ $<
obj/main.o:	src/main.c 
	$(CC) -c $(CFLAGS) -o $@ $<

clean:
	rm obj/*.o
	rm emhttp





