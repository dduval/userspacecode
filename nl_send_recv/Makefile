obj-m += nl_sender.o

default:

all:
	make -C/lib/modules/`uname -r`/build M=$$PWD
	gcc -o nl_receive_test nl_receive_test.c
clean:
	rm -f *.o *.ko *.mod.c nl_receive_test

