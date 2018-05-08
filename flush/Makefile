obj-m+=gf.o

all: 
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules
	$(CC) testgf.c -o test
clean:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) clean
	rm test
load:
	sudo insmod gf.ko
unload:
	sudo rmmod gf.ko
run:
	sudo dmesg -C
	sudo insmod gf.ko
	sudo chmod 777 /dev/global_flush
	./test
	dmesg
	sudo rmmod gf.ko
