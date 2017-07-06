MNAME=

SRCS=$(MNAME).c
KMOD=$(MNAME)

.include <bsd.kmod.mk>

config:
	gcc -o $(MNAME)_config $(MNAME)_config.c
