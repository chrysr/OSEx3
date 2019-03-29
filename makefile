CC=g++
CFLAGS=-I.
DEPS=ledger.h port.h shared.h vessel.h
OBJ=myport.o vessel.o port-master.o monitor.o ledger.o port.o shared.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

all: myport vessel port-master monitor

myport: myport.o ledger.o port.o shared.o
	$(CC) -o $@ $^ $(CFLAGS) -lpthread
vessel: vessel.o ledger.o port.o shared.o
	$(CC) -o $@ $^ $(CFLAGS) -lpthread
port-master: port-master.o ledger.o port.o shared.o
	$(CC) -o $@ $^ $(CFLAGS) -lpthread
monitor: monitor.o ledger.o port.o shared.o
	$(CC) -o $@ $^ $(CFLAGS) -lpthread

.Phony: clean
clean:
	-rm $(OBJ)
	-rm myport
	-rm vessel
	-rm port-master
	-rm monitor
