ifneq ($(KERNELRELEASE),)

# 设置编内核译版本
KERNEL_VIERSION := 2_6_32
KBUILD_CFLAGS += -fno-pic -Wno-declaration-after-statement
EXTRA_CFLAGS += -O0
# 定义模块名称
MODULE_NAME := tracepoint-probe

# 根据编译参数设置驱动模块版本信息
ifeq ($(version), )
NERV_VERSION=\"3.5.0\"
else
NERV_DESCRIPTION=\"$(version)\"
endif
ifeq ($(description), )
NERV_DESCRIPTION=\"nerv\"
else
NERV_DESCRIPTION=\"$(description)\"
endif

KBUILD_CFLAGS += -DNERV_VERSION=$(NERV_VERSION) -DNERV_DESCRIPTION=$(NERV_DESCRIPTION)

# 指定驱动模块的核心文件（有init 和 exit）
RESMAIN_CORE_OBJS := main.o

EXTRA_CFLAGS = -I$(PWD)

# 指定编译文件
RESMAIN_GLUE_OBJS = tracepoint.o \


$(MODULE_NAME)-objs := $(RESMAIN_GLUE_OBJS) $(RESMAIN_CORE_OBJS)

obj-m := $(MODULE_NAME).o
else
PWD := $(shell pwd)
# 设置内核版本，默认当前运行内核版本
ifeq ($(KVER), )
KVER=$(shell uname -r)
endif
KDIR := /lib/modules/$(KVER)/build
all:
	$(MAKE) -C $(KDIR) M=$(PWD) 
clean:
	rm -rf *.mod.c *.mk *.ko .tmp_versions *.symvers *.order
	find . -type f -name "*.o" -or -name "*.cmd" | xargs rm -fv
.PHONY: all clean
endif # ($(KERNELRELEASE),)
