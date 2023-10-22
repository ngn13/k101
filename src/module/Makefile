obj-m := vuln.o

all:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules
