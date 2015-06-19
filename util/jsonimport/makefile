CFLAGS=	-D USE_MYSQL -I $(MYSQLINCLUDE) -L $(MYSQLLIBPATH)

SRC=	main.c	\
	sha1.c

OBJ=	$(SRC:.c=.o)

all:	jsonimport

jsonimport: $(OBJ)
	gcc $(CFLAGS) $(OBJ) -lmysqlclient -lcrypt -o $@

.c.o:	$(SRC)
	gcc -c $(CFLAGS) $< -o $@

clean:
	-rm -f *.o

