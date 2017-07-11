/*
 * Filename: d_hand.c
 * Author: Weiyang Wang UCSD
 * FreeBSD kernel module, produce a pesudo-device that allows user
 * to write byte-strings to the device, and wait for interuption. 
 * Upon interuped, send a signal (write the message) to another 
 * driver file, which is intended to be an output port.
 *
 * Version: 0.0.1 alpha
 *  - just to tryout some thoughts...
 *  - intended to use uiomove to read from the user space (same as the 
 *    echo device), but use pwrite to write to the "echo" driver 
 *    which is mounted on my VM. 
 *  - Interuption not implenmemted.
 *
 * Date: June 23, 2017
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

#include <sys/unistd.h>
#include <sys/fcntl.h>

#include <sys/namei.h>
#include <sys/vnode.h>


MALLOC_DECLARE(M_DHANDLE);
MALLOC_DEFINE(M_DHANDLE, "data_handler_buf", "buffer for data handler driver");

// Declarations
static d_open_t     data_handler_open;
static d_close_t    data_handler_close;
static d_read_t     data_handler_read;
static d_write_t    data_handler_write;
static d_ioctl_t    data_handler_ioctl;

static int          open_file(const char *, struct thread *);
static int          close_file(struct nameidata, struct thread *);

static struct cdevsw data_handler_cdevsw = {
    .d_version =    D_VERSION,
    .d_open =       data_handler_open,
    .d_close =      data_handler_close,
    .d_read =       data_handler_read,
    .d_write =      data_handler_write,
    .d_ioctl =      data_handler_ioctl,
    .d_name =       "data_handler"
};

// file
static struct vnode * file_vnodep;
static struct nameidata file_namei;

typedef struct data_handler {
    int buffer_size;
    char * buffer;
    int length;
} data_handler_t;
// ioctl options
// clears the buffer
#define DHANDLE_CLEAR_BUFFER    _IO('D', 1)
// write to the echo device
#define DHANDLE_WRITE_ECHO      _IOR('D', 2, char*)


static data_handler_t * data_handler_message;
static struct cdev * data_handler_dev;

static struct sysctl_ctx_list clist;
static struct sysctl_oid * poid;

static int
open_file(const char * name, struct thread * td)
{
    int error = 0;

    NDINIT(&file_namei, LOOKUP, LOCKLEAF, UIO_SYSSPACE,
             name, td);
    error = namei(&file_namei);
    if (error){
        NDFREE(file_namei);
        return error;
    }
    file_vnodep = file_namei->ni_vp;
    error = VOP_OPEN(file_vnodep, FWRITE, NULL, td, NULL);
    return error;
}

static int 
close_file(struct nameidata f_namei, struct thread * td){
    int error = 0;
    vput(f_namei->ni_vp);
    error = VOP_CLOSE(f_namei->ni_vp, FWRITE, NULL, td);
    if (error) 
        return error;
    NDFREE(&f_namei, 0);
    return error;
}
    

static int
data_handler_open(struct cdev * dev, int oflags, int devtype, struct thread * td){
    uprintf("Opening data_handler device.\n");
    return 0;
}

static int
data_handler_close(struct cdev * dev, int fflag, int devtype, struct thread * td){
    uprintf("Closing data_handler device.\n");
    return 0;
}

static int 
data_handler_write(struct cdev * dev, struct uio * uio, int ioflag){
    int error = 0;
    int amount;

    // error = copyin(uio->uio_iov->iov_base, data_handler_message->buffer,
    //        MIN(uio->uio_iov->iov_len, BUFFER_SIZE - 1));

    amount = MIN(uio->uio_resid,
                (data_handler_message->buffer_size - 1 - uio->uio_offset > 0) ?
                 data_handler_message->buffer_size - 1 - uio->uio_offset : 0) ;
    if (amount == 0)
        return error;

    error = uiomove(data_handler_message->buffer, amount, uio);

    if (error != 0){
        uprintf("Write failed.\n");
        return error;
    }

    data_handler_message->buffer[amount] = '\0';
    data_handler_message->length = amount;

    return error;
}

static int 
data_handler_read(struct cdev * dev, struct uio * uio, int ioflag){
    int error = 0;
    int amount;

    amount = MIN(uio->uio_resid,
                (data_handler_message->length - uio->uio_offset > 0) ?
                 data_handler_message->length - uio->uio_offset : 0) ;
    
    error = uiomove(data_handler_message->buffer + uio->uio_offset, amount, uio);
    if (error != 0)
        uprintf("Read failed.\n");

    return error;
}

// static int
// data_handler_set_buffer_size(int size){
//     int error = 0;
// 
//     if (data_handler_message->buffer_size == size)
//         return error;
// 
//     if (size >= 128 && size <= 512){
//         data_handler_message->buffer = realloc(data_handler_message->buffer, size,
//             M_data_handler, M_WAITOK);
//         data_handler_message->buffer_size = size;
// 
//         if (data_handler_message->length >= size){
//             data_handler_message->length = size - 1;
//             data_handler_message->buffer[size - 1] = '\0';
//         }
//     }
//     else
//         error = EINVAL;
//     return error;
// }

static int 
sysctl_set_buffer_size(SYSCTL_HANDLER_ARGS){
    int error = 0;
    int size = data_handler_message->buffer_size;

    error = sysctl_handle_int(poid, &size, 0, req);
    if (error || !req->newptr || data_handler_message->buffer_size == size)
        return error;

    if (size >= 128 && size <= 512){
        data_handler_message->buffer = realloc(data_handler_message->buffer, size,
            M_DHANDLE, M_WAITOK);
        data_handler_message->buffer_size = size;

        if (data_handler_message->length >= size){
            data_handler_message->length = size - 1;
            data_handler_message->buffer[size - 1] = '\0';
        }
    }
    else
        error = EINVAL;
    return error;
}

static int
data_handler_ioctl(struct cdev * dev, u_long cmd, caddr_t data, int fflag, 
    struct thread * td){

    int error = 0;

    switch (cmd){
        case DHANDLE_CLEAR_BUFFER:
            memset(data_handler_message->buffer, 
                '\0', data_handler_message->buffer_size);
            data_handler_message->buffer_size = 0;
            uprintf("Buffer cleared.\n");
            break;
        default:
            error = ENOTTY;
            break;
    }
    return error;
}

static int 
data_handler_modevent(module_t mod __unused, int event, void *arg __unused){
    int error = 0;
    int fd = 0;
    switch (event) {
        case MOD_LOAD:
            fd = open("/dev/echo", O_RDWR);
            uprintf("fd is %d\n", fd);
            data_handler_message = malloc(sizeof(data_handler_t), 
                    M_DHANDLE, M_WAITOK);
            data_handler_message->buffer_size = 256;
            data_handler_message->buffer = 
                malloc(data_handler_message->buffer_size, 
                M_DHANDLE, M_WAITOK);
            sysctl_ctx_init(&clist);
            SYSCTL_DECL(_data_handler);
            poid = SYSCTL_ADD_ROOT_NODE(&clist,
                    OID_AUTO,
                    "data_handler", CTLFLAG_RW, 0, "data_handler root node");
            SYSCTL_ADD_PROC(&clist, SYSCTL_CHILDREN(poid), OID_AUTO,
                    "buffer_size", CTLTYPE_INT | CTLFLAG_RW,
                    &data_handler_message->buffer_size, 0, sysctl_set_buffer_size,
                    "I", "data_handler buffer size");
            data_handler_dev = make_dev(&data_handler_cdevsw, 0, UID_ROOT, GID_WHEEL,
                        0600, "data_handler");
            uprintf("data_handler driver loaded.\n");
            break;
        case MOD_UNLOAD:
            destroy_dev(data_handler_dev);
            sysctl_ctx_free(&clist);
            free(data_handler_message->buffer, M_DHANDLE);
            free(data_handler_message, M_DHANDLE);
            uprintf("data_handler driver unloaded.\n");
            break;
        default:
            error = EOPNOTSUPP;
            break;
    }

    return error;
}

DEV_MODULE(data_handler, data_handler_modevent, NULL);

