
LDFLAGS=-lrt -lpcap
CPPFLAGS=-Wall

all: rx tx rx_status_test



%.o: %.c
	gcc -c -o $@ $< $(CPPFLAGS)


rx: rx.o lib.o radiotap.o fec.o
	gcc -o $@ $^ $(LDFLAGS)


tx: tx.o lib.o fec.o
	gcc -o $@ $^ $(LDFLAGS)


rx_status_test: rx_status_test.o
	gcc -o $@ $^ $(LDFLAGS)

clean:
	rm -f rx tx *~ *.o

