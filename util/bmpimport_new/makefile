CFLAGS=

SRC=	main.c

OBJ=	$(SRC:.c=.o)

all:	bmpimport

bmpimport: $(OBJ)
	cc $(CFLAGS) $(OBJ) -o $@

.c.o:	$(SRC)
	cc -c $(CFLAGS) $< -o $@

clean:
	-rm -f *.o

