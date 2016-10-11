
LDFLAGS=-lrt -lpcap
CPPFLAGS=-Wall

all: rx joystick joystick_receive tx tx_alternate tx_duplicate rx_status_test



%.o: %.c
	gcc -c -o $@ $< $(CPPFLAGS)


rx: rx.o rx_lib.o lib.o radiotap.o fec.o
	gcc -o $@ $^ $(LDFLAGS)

joystick: send_joystick_command.o tx_lib.o lib.o fec.o
	gcc -o $@ $^ $(LDFLAGS)

joystick_receive: receive_joystick_command.o rx_lib.o lib.o radiotap.o fec.o
	gcc -o $@ $^ $(LDFLAGS)	

tx: tx.o tx_lib.o lib.o fec.o
	gcc -o $@ $^ $(LDFLAGS)

tx_alternate: tx_alternate.o lib.o fec.o
	gcc -o $@ $^ $(LDFLAGS)

tx_duplicate: tx_duplicate.o lib.o fec.o
	gcc -o $@ $^ $(LDFLAGS)


rx_status_test: rx_status_test.o
	gcc -o $@ $^ $(LDFLAGS)

clean:
	rm -f joystick joystick_receive rx tx tx_alternate tx_duplicate *~ *.o

