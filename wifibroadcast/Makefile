
LDFLAGS=-lrt -lpcap
CPPFLAGS=-Wall -O2

all: rx tx rx_status_test



%.o: %.c
	gcc -c -o $@ $< $(CPPFLAGS)

rx: rx.o rx_lib.o tx_lib.o lib.o radiotap.o fec.o
	gcc -o $@ $^ $(LDFLAGS)


tx: tx.o tx_lib.o rx_lib.o lib.o radiotap.o fec.o
	gcc -o $@ $^ $(LDFLAGS)


rx_status_test: rx_status_test.o
	gcc -o $@ $^ $(LDFLAGS)

clean:
	rm -f rx tx *~ *.o

