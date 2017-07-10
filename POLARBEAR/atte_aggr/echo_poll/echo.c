/*
 * Filename: echo.c
 * Author: not me anyway...
 * Char driver that emulates system "echo" command
 * Improved memory management.
 * Improved added d_ioctl
 */
#include <sys/param.h>
#include <sys/module.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <sys/conf.h>
#include <sys/uio.h>
#include <sys/malloc.h>
#include <sys/ioccom.h>
#include <sys/sysctl.h>

#include <sys/kernel.h>
#include <sys/queue.h>
#include <sys/lock.h>
#include <sys/mutex.h>
#include <sys/condvar.h>

#include <sys/poll.h>
MALLOC_DECLARE(M_ECHO);
MALLOC_DEFINE(M_ECHO, "echo_buffer", "buffer for echo driver");

#define ECHO_CLEAR_BUFFER       _IO('E', 1)
// #define ECHO_SET_BUFFER_SIZE    _IOW('E', 2, int)

// Declarations
static d_open_t     echo_open;
static d_close_t    echo_close;
static d_read_t     echo_read;
static d_write_t    echo_write;
static d_ioctl_t    echo_ioctl;
static d_poll_t     echo_poll;

static struct cv rw_cv;
static struct mtx rw_mtx;

static struct cdevsw echo_cdevsw = {
    .d_version =    D_VERSION,
    .d_open =       echo_open,
    .d_close =      echo_close,
    .d_read =       echo_read,
    .d_write =      echo_write,
    .d_ioctl =      echo_ioctl,
    .d_poll =       echo_poll,
    .d_name =       "echo"
};

typedef struct echo {
    int buffer_size;
    char * buffer;
    int length;
} echo_t;

static echo_t * echo_message;
static struct cdev * echo_dev;

static struct sysctl_ctx_list clist;
static struct sysctl_oid * poid;

static int
echo_open(struct cdev * dev, int oflags, int devtype, struct thread * td){
    uprintf("Opening echo device.\n");
    return 0;
}

static int
echo_close(struct cdev * dev, int fflag, int devtype, struct thread * td){
    uprintf("Closing echo device.\n");
    return 0;
}

static int 
echo_write(struct cdev * dev, struct uio * uio, int ioflag){
    int error = 0;
    int amount;

    cv_signal(&rw_cv);
    uprintf("Read called.\n");

    // error = copyin(uio->uio_iov->iov_base, echo_message->buffer,
    //        MIN(uio->uio_iov->iov_len, BUFFER_SIZE - 1));

    amount = MIN(uio->uio_resid,
                (echo_message->buffer_size - 1 - uio->uio_offset > 0) ?
                 echo_message->buffer_size - 1 - uio->uio_offset : 0) ;
    if (amount == 0)
        return error;

    error = uiomove(echo_message->buffer, amount, uio);
    if (error != 0){
        uprintf("Write failed.\n");
        return error;
    }

    //*(echo_message->buffer + 
    //    MIN(uio->uio_iov->iov_len, BUFFER_SIZE - 1)) = 0;

    //echo_message->length = MIN(uio->uio_iov->iov_len, BUFFER_SIZE - 1);
    echo_message->buffer[amount] = '\0';
    echo_message->length = amount;

    return error;
}

static int 
echo_read(struct cdev * dev, struct uio * uio, int ioflag){
    int error = 0;
    int amount;

    cv_signal(&rw_cv);
    uprintf("Read called.\n");

    amount = MIN(uio->uio_resid,
                (echo_message->length - uio->uio_offset > 0) ?
                 echo_message->length - uio->uio_offset : 0) ;
    
    error = uiomove(echo_message->buffer + uio->uio_offset, amount, uio);
    if (error != 0)
        uprintf("Read failed.\n");

    return error;
}

// static int
// echo_set_buffer_size(int size){
//     int error = 0;
// 
//     if (echo_message->buffer_size == size)
//         return error;
// 
//     if (size >= 128 && size <= 512){
//         echo_message->buffer = realloc(echo_message->buffer, size,
//             M_ECHO, M_WAITOK);
//         echo_message->buffer_size = size;
// 
//         if (echo_message->length >= size){
//             echo_message->length = size - 1;
//             echo_message->buffer[size - 1] = '\0';
//         }
//     }
//     else
//         error = EINVAL;
//     return error;
// }

static int 
echo_poll(struct cdev *dev, int events, struct thread * td){
    int error = 0;
    uprintf("poll function called, event number %x\n", events);
    if (events == 0x58) {
        uprintf("read poll\n");
        return POLLIN;
        // mtx_lock(&rw_mtx);
        // cv_wait(&rw_cv, &rw_mtx);
        // mtx_unlock(&rw_mtx);
    }
    else if (events == 0x1c) {
        uprintf("write poll\n");
        return POLLOUT;
    }
    else {
        error = EINVAL;
    }
    uprintf("falling out of poll function\n");

    return POLLERR;
}

static int 
sysctl_set_buffer_size(SYSCTL_HANDLER_ARGS){
    int error = 0;
    int size = echo_message->buffer_size;

    error = sysctl_handle_int(poid, &size, 0, req);
    if (error || !req->newptr || echo_message->buffer_size == size)
        return error;

    if (size >= 128 && size <= 512){
        echo_message->buffer = realloc(echo_message->buffer, size,
            M_ECHO, M_WAITOK);
        echo_message->buffer_size = size;

        if (echo_message->length >= size){
            echo_message->length = size - 1;
            echo_message->buffer[size - 1] = '\0';
        }
    }
    else
        error = EINVAL;
    return error;
}

static int
echo_ioctl(struct cdev * dev, u_long cmd, caddr_t data, int fflag, 
    struct thread * td){

    int error = 0;

    switch (cmd){
        case ECHO_CLEAR_BUFFER:
            memset(echo_message->buffer, '\0', echo_message->buffer_size);
            echo_message->buffer_size = 0;
            uprintf("Buffer cleared.\n");
            break;
    //    case ECHO_SET_BUFFER_SIZE:
    //        error = echo_set_buffer_size(*(int*)data);
    //        if (error == 0)
    //            uprintf("Buffer resized.\n");
    //        break;
        default:
            error = ENOTTY;
            break;
    }
    return error;
}

static int 
echo_modevent(module_t mod __unused, int event, void *arg __unused){
    int error = 0;

    switch (event) {
        case MOD_LOAD:

            mtx_init(&rw_mtx, "rw event", NULL, MTX_DEF);
            cv_init(&rw_cv, "rw cond var");

            echo_message = malloc(sizeof(echo_t), M_ECHO, M_WAITOK);
            echo_message->buffer_size = 256;
            echo_message->buffer = malloc(echo_message->buffer_size, 
                M_ECHO, M_WAITOK);
            sysctl_ctx_init(&clist);
            SYSCTL_DECL(_echo);
            poid = SYSCTL_ADD_ROOT_NODE(&clist,
                    OID_AUTO,
                    "echo", CTLFLAG_RW, 0, "echo root node");
            SYSCTL_ADD_PROC(&clist, SYSCTL_CHILDREN(poid), OID_AUTO,
                    "buffer_size", CTLTYPE_INT | CTLFLAG_RW,
                    &echo_message->buffer_size, 0, sysctl_set_buffer_size,
                    "I", "echo buffer size");
            echo_dev = make_dev(&echo_cdevsw, 0, 1001, GID_WHEEL,
                        0600, "echo");
            uprintf("Echo driver loaded.\n");
            break;
        case MOD_UNLOAD:
            mtx_destroy(&rw_mtx);
            cv_destroy(&rw_cv);

            destroy_dev(echo_dev);
            sysctl_ctx_free(&clist);
            free(echo_message->buffer, M_ECHO);
            free(echo_message, M_ECHO);
            uprintf("Echo driver unloaded.\n");
            break;
        default:
            error = EOPNOTSUPP;
            break;
    }

    return error;
}

DEV_MODULE(echo, echo_modevent, NULL);

