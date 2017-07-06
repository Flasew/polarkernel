/* 
 * My first try on kernel programming
 * skeleton.c
 * Apr 16 2017
 * From Freebsd webpage
 */
#include <sys/types.h>
#include <sys/module.h>
#include <sys/systm.h>   /* uprintf */
#include <sys/errno.h>
#include <sys/param.h>   /* defined used in kernel.h */
#include <sys/kernel.h>  /* types used in module init. */

/*
 * Load handler that deals with the loading and unloading of KLD
 * (Dynamic Kernal Linker Facility 
 */

static int 
skel_loader( struct module * m, int what, void * arg ){
    int err = 0; 

    switch(what){
        case MOD_LOAD:   /* kldload */
            uprintf("Skeleton KLD loaded.\n");
            break;
        case MOD_UNLOAD:
            uprintf("Skeleton KLD unloaded.\n");
            break;
        default:
            err = EOPNOTSUPP;
            break;
    }
    return (err);
}

/* declare this modult to the rest of the kernel */
static moduledata_t skel_mod = {
    "skel",
    skel_loader,
    NULL
};

DECLARE_MODULE (skeleton, skel_mod, SI_SUB_KLD, SI_ORDER_ANY);
