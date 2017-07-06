/* 
 * I just can't resist using this traditional Hellow Word thing lol.
 * hello.c
 * Weiyang Wang Apr 16 2017
 * print HW by loading this file into kernel
 */
#include <sys/types.h>
#include <sys/module.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/param.h>
#include <sys/kernel.h>

static int 
hello_loader( struct module * m, int what, void * arg ){
    int err = 0;

    switch(what){
        case MOD_LOAD:
            uprintf("Hello World!\n");
            break;
        case MOD_UNLOAD:
            uprintf("Goodbye World...\n");
            break;
        default:
            err = EOPNOTSUPP;
            break;
    }
    return err;
}

/* declare module to the rest of kernal */
static moduledata_t hello_mod = {
    "hello",
    hello_loader,
    NULL,
};

DECLARE_MODULE( hello, hello_mod, SI_SUB_KLD, SI_ORDER_ANY );
