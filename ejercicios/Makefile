EXTRA_CFLAGS += -I/usr/realtime/include -D_IN_RTAI_
obj-m = roundrobin.o
startstop-objs := roundrobin.o

all:
	make -C /lib/modules/3.10.32-rtai/build M=$(PWD) modules

clean:
	make -C /lib/modules/3.10.32-rtai/build M=$(PWD) clean

KBUILD_EXTRA_SYMBOLS := $PWD
