MNAME=twrite

SRCS=$(MNAME).c vnode_if.h
KMOD=$(MNAME)

.include <bsd.kmod.mk>

config:
	gcc -o $(MNAME)_config $(MNAME)_config.c
