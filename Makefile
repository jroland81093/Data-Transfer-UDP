CC=gcc
CFLAGS=-I.
DEPS=utils.c

all: Sender Receiver

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

Sender: sender.o
	$(CC) -o $@ $^ $(CFLAGS)

Receiver: receiver.o
	$(CC) -o $@ $^ $(CFLAGS)



