#include <sys/param.h>
#include <sys/module.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <sys/conf.h>
#include <sys/uio.h>
#include <sys/bus.h>

#include <machine/bus.h>
#include <machine/resource.h>
#include <sys/interrupt.h>
#include <sys/rman.h>
#include <sys/lock.h>
#include <sys/mutex.h>
#include <sys/condvar.h>
#include <sys/malloc.h>
#include <sys/queue.h>
#include <sys/taskqueue.h>

#include "ilog_dev.h"

struct intr_log_softc ilog_sc;

int 
ilog_open(struct cdev * dev, int oflags, int devtype, struct thread * td)
{
    uprintf("Opening ilog device...\n");
    return 0;
}

int
ilog_close(struct cdev * dev, int fflag, int devtype, struct thread * td)
{
    uprintf("Closing ilog device...\n");
    return 0;
}

int
ilog_read(struct cdev * dev, struct uio * uio, int ioflag)
{
    mtx_lock(ilog_sc.ilog_mtx);
    int which = ilog_sc.which;
    ilog_sc.which = !which;
    mtx_unlock(ilog_sc.ilog_mtx);

    int error = 0;
    int amount;
    // uprintf("%d logs on hold\n", ilog_sc.logArr[which].nlogs);
    for (int i = 0; i < ilog_sc.logArr[which].nlogs; i++) {
        // uprintf("LOOP: %d\n", i);
        amount = MIN(uio->uio_resid,
            (strlen(ilog_sc.logArr[which].logs[i]) - uio->uio_offset > 0) ?
             strlen(ilog_sc.logArr[which].logs[i]) - uio->uio_offset : 0) ;

        error = uiomove(ilog_sc.logArr[which].logs[i] + uio->uio_offset,
                 amount, uio);

        if (error != 0) {
            uprintf("Read failed.\n");
            break;
        }
        uio->uio_offset = 0;
    }
    ilog_sc.logArr[which].nlogs = 0;
    return error;
}