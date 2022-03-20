CONFIG_MODULE_SIG = n
TARGET_MODULE := _fibdrv

obj-m := $(TARGET_MODULE).o
ccflags-y := -std=gnu99 -Wno-declaration-after-statement

_fibdrv-objs := \
	fibdrv.o \
	sstring.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

GIT_HOOKS := .git/hooks/applied

.PHONY: time

all: $(GIT_HOOKS) client
	$(MAKE) -C $(KDIR) M=$(PWD) modules

$(GIT_HOOKS):
	@scripts/install-git-hooks
	@echo

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	$(RM) client out time time.txt
load:
	sudo insmod $(TARGET_MODULE).ko
unload:
	sudo rmmod $(TARGET_MODULE) || true >/dev/null

client: client.c
	$(CC) -o $@ $^

time: client_ktime.c
	sudo sh performance.sh
	$(CC) -o $@ $<
	sudo taskset 0x1 ./time
	gnuplot < runtime.gp

PRINTF = env printf
PASS_COLOR = \e[32;01m
NO_COLOR = \e[0m
pass = $(PRINTF) "$(PASS_COLOR)$1 Passed [-]$(NO_COLOR)\n"

check: all
	$(MAKE) unload
	$(MAKE) load
	sudo ./client > out
	$(MAKE) unload
	@diff -u out scripts/expected.txt && $(call pass)
	@scripts/verify.py
