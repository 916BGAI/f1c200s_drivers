NAME = demo
ARCH = arm
CROSS_COMPILE = arm-linux-gnueabi-
KERN_DIR = /home/kjlsai/Code/f1c200s/Kernel

obj-m += $(NAME).o
$(NAME)-objs += main.o
PWD := $(shell pwd)

all:
	make -C $(KERN_DIR) M=$(PWD) modules

clean:
	make -C $(KERN_DIR) M=$(PWD) modules clean
	rm -rf modules.order

.PHONY:all clean
