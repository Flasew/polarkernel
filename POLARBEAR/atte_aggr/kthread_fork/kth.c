#include <sys/param.h>
#include <sys/module.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <sys/conf.h>
#include <sys/uio.h>
#include <sys/malloc.h>
#include <sys/ioccom.h>
#include <sys/sysctl.h>

#include <sys/mutex.h>
#include <sys/condvar.h>
#include <sys/proc.h>
#include <sys/kthread.h>
#include <sys/sched.h>

int cont;
int numFork = 0;

static struct mtx t_mtx;
static struct cv  t_cv;

static d_read_t     kth_read;

static struct cdevsw kth_cdevsw = {
    .d_version =    D_VERSION,
    .d_read =       kth_read,
    .d_name =       "kth"
};

static void kth_fork(void*);

static struct cdev * kth_dev;

static void
kth_fork(void * arg)
{
	uprintf("FORKED.\n");	
	mtx_lock(&t_mtx);
	cv_wait(&t_cv, &t_mtx);
	mtx_unlock(&t_mtx);
	int go = *(int*)arg;
	uprintf("UNLOCKED.\n");
	if (go) {
		struct thread * td;
		int error = kthread_add(kth_fork, &cont, NULL, &td, 0, 0, "kth %dth fork", ++numFork);
		if (error) {
			uprintf ("error forking!\n");
		}
	} 
	else {
		kthread_exit();
	}
	uprintf("This is the %dth fork of kth. Now exiting.\n", numFork);
	kthread_exit();
}

static int 
kth_read(struct cdev * dev, struct uio * uio, int ioflag){
	cv_signal(&t_cv);
    return 0;
}

static int 
kth_modevent(module_t mod __unused, int event, void *arg __unused){
    int error = 0;

    switch (event) {
        case MOD_LOAD:
        	mtx_init(&t_mtx, "kth mtx", NULL, MTX_DEF);
            cv_init(&t_cv, "kth cv");

            cont = 1;
            // struct thread * td;

            // error = kthread_add(kth_fork, &cont, NULL, &td, 0, 0, "kth %dth fork", ++numFork);
           	// if (error) {
           	// 	uprintf("can't add initial thread\n");
           	// 	return error;
           	// } 
           	// thread_lock(td);
           	// TD_SET_CAN_RUN(td);
           	// sched_add(td,SRQ_BORING);
           	// thread_unlock(td);


           	kth_dev = make_dev(&kth_cdevsw, 0, UID_ROOT, GID_WHEEL, 0644, "kth");
           	kth_fork(&cont);
           	uprintf("kth loaded\n");
            break;
        case MOD_UNLOAD:
            
            cont = 0;
            cv_broadcastpri(&t_cv, 64);

            destroy_dev(kth_dev);
            mtx_destroy(&t_mtx);
            cv_destroy(&t_cv);

           	uprintf("kth unloaded\n");
            break;
        default:
            error = EOPNOTSUPP;
            break;
    }

    return error;
}

DEV_MODULE(kth, kth_modevent, NULL);

