MNAME=pci_parallel

SRCS=$(MNAME).c bus_if.h device_if.h pci_if.h 
KMOD=$(MNAME)

.include <bsd.kmod.mk>

config:
	gcc -o $(MNAME)_config $(MNAME)_config.c
