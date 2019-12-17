CC = gcc
CFILE = $(wildcard *.c)
TARGETS = $(patsubst %.c, %, $(CFILE))
CFLAGS = -Wall -Os
LIBS = -lpthread

all: $(TARGETS)

$(TARGETS):%:%.c
	$(CC) $< $(CFLAGS) -o $@ $(LIBS)

.PHONT:clean all

#server: *.c
#	$(CC) $(FLAGS) *.c -o server $(LIBS)

#client: *.c
#	$(CC) $(FLAGS) *.c -o client $(LIBS)

clean:
	rm -f $(TARGETS)
