MNAME=echo_hook

SRCS=$(MNAME).c
KMOD=$(MNAME)

.include <bsd.kmod.mk>

config: 
	gcc echo_config.c -o echo_config
