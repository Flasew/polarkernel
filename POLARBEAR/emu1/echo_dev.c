#include <sys/param.h>
#include <sys/module.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <sys/conf.h>
#include <sys/uio.h>
#include <sys/malloc.h>
#include <sys/ioccom.h>
#include <sys/sysctl.h>

#include "echo_dev.h"

#define BUF_SIZE 512
#define FILE_LOC "/home/POLARBEAR/fakeout.txt"

struct nameidata nd;

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
    // int amount;

    // error = copyin(uio->uio_iov->iov_base, echo_message->buffer,
    //        MIN(uio->uio_iov->iov_len, BUFFER_SIZE - 1));

    vn_start_write(nd.ni_vp, nd.ni_vp->vu_mount, V_NOWAIT);
    VOP_WRITE(nd.ni_vp, &fuio, IO_UNIT | IO_NODELOCKED, curthread->td_ucred);
    vn_finished_write(nd.ni_vp->vu_mount);
 
    // amount = MIN(uio->uio_resid,
    //             (echo_message->buffer_size - 1 - uio->uio_offset > 0) ?
    //              echo_message->buffer_size - 1 - uio->uio_offset : 0) ;
    // if (amount == 0)
    //     return error;

    // error = uiomove(echo_message->buffer, amount, uio);
    // if (error != 0){
    //     uprintf("Write failed.\n");
    //     return error;
    // }

    // echo_message->buffer[amount] = '\0';
    // echo_message->length = amount;

    return error;
}

// static int 
// echo_read(struct cdev * dev, struct uio * uio, int ioflag){
//     int error = 0;
//     int amount;

//     amount = MIN(uio->uio_resid,
//                 (echo_message->length - uio->uio_offset > 0) ?
//                  echo_message->length - uio->uio_offset : 0) ;
    
//     error = uiomove(echo_message->buffer + uio->uio_offset, amount, uio);
//     if (error != 0)
//         uprintf("Read failed.\n");

//     return error;
// }

// static int 
// sysctl_set_buffer_size(SYSCTL_HANDLER_ARGS){
//     int error = 0;
//     int size = echo_message->buffer_size;

//     error = sysctl_handle_int(poid, &size, 0, req);
//     if (error || !req->newptr || echo_message->buffer_size == size)
//         return error;

//     if (size >= 128 && size <= 512){
//         echo_message->buffer = realloc(echo_message->buffer, size,
//             M_ECHO, M_WAITOK);
//         echo_message->buffer_size = size;

//         if (echo_message->length >= size){
//             echo_message->length = size - 1;
//             echo_message->buffer[size - 1] = '\0';
//         }
//     }
//     else
//         error = EINVAL;
//     return error;
// }

// static int
// echo_ioctl(struct cdev * dev, u_long cmd, caddr_t data, int fflag, 
//     struct thread * td){

//     int error = 0;

//     switch (cmd){
//         case ECHO_CLEAR_BUFFER:
//             memset(echo_message->buffer, '\0', echo_message->buffer_size);
//             echo_message->buffer_size = 0;
//             uprintf("Buffer cleared.\n");
//             break;
    
//         default:
//             error = ENOTTY;
//             break;
//     }
//     return error;
// }
