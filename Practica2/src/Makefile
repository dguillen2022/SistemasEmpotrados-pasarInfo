CC=gcc
CFLAGS=-I.
OBJ = cyclictestURJC.o

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

cyclictestURJC: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)
	
clean:
	rm -f *.o cyclictestURJC
