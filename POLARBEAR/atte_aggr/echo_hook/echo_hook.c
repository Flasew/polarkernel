#include <sys/param.h> 
#include <sys/proc.h> 
#include <sys/module.h>

#include <sys/kernel.h> 
#include <sys/systm.h> 
#include <sys/conf.h> 
#include <sys/queue.h> 
#include <sys/lock.h> 
#include <sys/mutex.h>

#include <fs/devfs/devfs_int.h>

#include <sys/malloc.h>
#include <sys/eventhandler.h>
#include <sys/kthread.h>
#include <sys/proc.h>
#include <sys/sched.h>
#include <sys/unistd.h>
#include <sys/condvar.h>
#include <sys/bus.h>
#include <sys/interrupt.h>

#include <machine/bus.h>
#include <machine/resource.h>
#include <sys/rman.h>
#include <sys/kbio.h>
#include <dev/kbd/kbdreg.h>
#include <dev/kbd/kbdtables.h>
#include <dev/atkbdc/atkbdreg.h>
#include <dev/atkbdc/atkbdc_subr.h>
#include <dev/atkbdc/atkbdcreg.h>
#include <dev/atkbdc/psm.h>

//extern TAILQ_HEAD(cdev_priv_list,cdev_priv) cdevp_list;
static struct cv read_cv;
static struct mtx read_mtx;

typedef struct {
    struct resource *intr;
    void            *ih;
} atkbd_softc_t;

device_t * devList;
int devcount;

static int atkbdattach(device_t);


d_read_t read_hook;
d_read_t * read;

d_write_t write_hook;
d_write_t * write;

int
write_hook(struct cdev * dev, struct uio * uio, int ioflag)
{
    mtx_lock(&read_mtx);
    cv_wait(&read_cv, &read_mtx);
    mtx_unlock(&read_mtx);
	uprintf("I'm going to hold thid message until interupted.\n");
    return (*write)(dev, uio, ioflag);
}

int 
read_hook(struct cdev * dev, struct uio * uio, int ioflag)
{
    uprintf("Caught read\n");
	return (*read)(dev, uio, ioflag);
}

static void
read_intr(void * arg)
{
    cv_signal(&read_cv);
}

static int
atkbdattach(device_t dev)
{
    int error = 0;
    device_t * dlist;
    int count;
    device_busy(dev);
    device_disable(dev);
    device_get_children(dev, &dlist, &count);
    dev = dlist[0];
    if (error){
        uprintf("detach\n");
        return error;
    }
    atkbd_softc_t *sc = NULL;
    // keyboard_t *kbd;
    // u_long irq;
    // int flags;
    int rid;

    sc = device_get_softc(dev);

    rid = KBDC_RID_KBD;
    //irq = bus_get_resource_start(dev, SYS_RES_IRQ, rid);
    //flags = device_get_flags(dev);
    //error = atkbd_attach_unit(dev, &kbd, irq, flags);
    if (error)
        return error;

    /* declare our interrupt handler */
    error = bus_teardown_intr(dev, sc -> intr, sc->ih);
    if (error){
        uprintf("teardown\n");
        return error;
    }

    error = bus_deactivate_resource(dev, SYS_RES_IRQ, rid,
                 sc->intr);
    if (error)
        return error;

    error = bus_release_resource(dev, SYS_RES_IRQ, rid,
                sc->intr);
    if (error) {
        uprintf("rel\n");
        return error;
    }

    sc->intr = bus_alloc_resource_any(dev, SYS_RES_IRQ, &rid, RF_ACTIVE);
    if (error) 
        return error;

    error = bus_setup_intr(dev, sc->intr, INTR_TYPE_TTY, NULL, read_intr,
            NULL, &sc->ih);
    if (error)
        bus_release_resource(dev, SYS_RES_IRQ, rid, sc->intr);
    device_attach(dev);

    return error;
}

static int 
load(struct module * module, int cmd, void * arg)
{
	int error = 0;
	struct cdev_priv * cdp;

	switch(cmd){
		case MOD_LOAD:
            mtx_init(&read_mtx, "read event", NULL, MTX_DEF);
            cv_init(&read_cv, "read cv");

            if (error)
                return error;

            device_get_children(root_bus, &devList, &devcount);
            uprintf("Count: %d\n", devcount);
            for (int i = 0; i < devcount; i++){
                device_print_prettyname(devList[i]);
            }
            device_get_children(devList[0], &devList, &devcount);
            uprintf("Count: %d\n", devcount);
            for (int i = 0; i < devcount; i++){
                device_print_prettyname(devList[i]);
            }
            device_get_children(devList[3], &devList, &devcount);
            uprintf("Count: %d\n", devcount);
            for (int i = 0; i < devcount; i++){
                device_print_prettyname(devList[i]);
            }
            error = atkbdattach(devList[20]);
            device_get_children(devList[20], &devList, &devcount);
            uprintf("Count: %d\n", devcount);
            for (int i = 0; i < devcount; i++){
                device_print_prettyname(devList[i]);
            }
            if (error)
                uprintf("fucked up\n");
            
			mtx_lock(&devmtx);
			TAILQ_FOREACH(cdp, &cdevp_list, cdp_list) {
				if (strcmp(cdp->cdp_c.si_name, "echo") == 0) {
					read = cdp->cdp_c.si_devsw->d_read;
					cdp->cdp_c.si_devsw-> d_read = read_hook;
                    write = cdp->cdp_c.si_devsw->d_write;
                    cdp->cdp_c.si_devsw-> d_write = write_hook;
					break;
				}
			}
			mtx_unlock(&devmtx);
			break;
		case MOD_UNLOAD:
            mtx_destroy(&read_mtx);
            cv_destroy(&read_cv);

			mtx_lock(&devmtx);
			TAILQ_FOREACH(cdp, &cdevp_list, cdp_list) {
				if (strcmp(cdp->cdp_c.si_name, "echo") == 0) {
					cdp->cdp_c.si_devsw->d_read = read;
                    cdp->cdp_c.si_devsw->d_write = write;
					break;
				}
			}
			mtx_unlock(&devmtx);
			break;
		default:
			error = EOPNOTSUPP;
			break;
	}
	return error;
}


static moduledata_t echo_hook_mod = {
	"echo_hook",
	load,
	NULL
};

DECLARE_MODULE(echo_hook, echo_hook_mod, SI_SUB_DRIVERS, 
	SI_ORDER_MIDDLE);


