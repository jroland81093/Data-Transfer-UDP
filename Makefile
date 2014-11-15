CC=gcc
CFLAGS=-I.
DEPS=utils.c

all: sender receiver

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

sender: sender.o
	$(CC) -o $@ $^ $(CFLAGS)

receiver: receiver.o
	$(CC) -o $@ $^ $(CFLAGS)



