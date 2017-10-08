obj-m += hello-1.o
obj-m += hello-2.o
obj-m += procfs1.o
obj-m += hw1.o
obj-m += q1.o
obj-m += q2.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
