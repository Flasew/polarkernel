#include <sys/errno.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/vnode.h>
#include <sys/fcntl.h>
#include <sys/module.h>
#include <sys/kernel.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/sbuf.h>
#include <sys/event.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/conf.h>

//  catfile get from stack overflow
static int catfile(const char *filename) {
    struct sbuf *sb;
    static char buf[128] = { 0 };
    struct nameidata nd;
    off_t ofs;
    // ssize_t resid;
    int error, flags, len;

    NDINIT(&nd, LOOKUP, FOLLOW, UIO_SYSSPACE, filename, curthread);
    flags = FWRITE;
    error = vn_open(&nd, &flags, 0, NULL);
    if (error) {
        uprintf("error opening\n");
        return (error);
    }

    NDFREE(&nd, NDF_ONLY_PNBUF);
    

    ofs = 0;
    len = sizeof(buf) - 1;
    sb = sbuf_new_auto();
    // int loopc = 0;

    struct iovec iodata = {
        buf,
        128
    };

    struct uio fuio = {
        &iodata, 
        1,
        0,
        4096,
        UIO_SYSSPACE,
        UIO_READ,
        curthread
    };

    nd.ni_vp->v_un.vu_cdev->si_devsw->
        d_read(nd.ni_vp->v_un.vu_cdev, &fuio, 0);

<<<<<<< HEAD:POLARBEAR/write_vn_start/twrite.c
    uprintf("buf contains: %s", buf);
=======
    uprintf("buf contains: %s\n", buf)
>>>>>>> e747f44ac1716f0ac08f6a7dff9afd2305550d5a:POLARBEAR/rw_d_foo/twrite.c
    // while (1) {
    //     uprintf("loop %d: \n", loopc);
    //     error = vn_rdwr_inchunks(UIO_READ, nd.ni_vp, buf, len, ofs,
    //             UIO_SYSSPACE, IO_NODELOCKED, curthread->td_ucred,
    //             NULL, &resid, curthread);
    //     if (error) {
    //         uprintf("error reading, %d\n", error);
    //         break;
    //     }
    //     if (resid == len)
    //         break;
    //     buf[len - resid] = 0;
    //     sbuf_printf(sb, "%s", buf);
    //     ofs += len - resid;
    // }

    VOP_UNLOCK(nd.ni_vp, 0);
    vn_close(nd.ni_vp, FREAD, curthread->td_ucred, curthread);
    // vput(nd.ni_vp);
    // if (!error)
    //     uprintf("%s", sbuf_data(sb));
    return error;
}

static int echofile(const char *filename, char * mes) {
    struct nameidata nd;
    off_t ofs;
    ssize_t resid;
    int error, flags, len;

    NDINIT(&nd, LOOKUP, FOLLOW, UIO_SYSSPACE, filename, curthread);
    flags = FWRITE;
    error = vn_open(&nd, &flags, 0, NULL);
    if (error) {
        uprintf("error open\n");
        return (error);
    }

    NDFREE(&nd, NDF_ONLY_PNBUF);

    struct iovec iodata = {
        mes,
        strlen(mes)
    };

    struct uio fuio = {
        &iodata, 
        1,
        0,
        strlen(mes),
        UIO_SYSSPACE,
        UIO_WRITE,
        curthread
    };

    error = nd.ni_vp->v_un.vu_cdev->si_devsw->
        d_write(nd.ni_vp->v_un.vu_cdev, &fuio, 0);

    if (!error)
        uprintf("write succeed\n");

    // resid = strlen(mes);
    // ofs = 0;
    // len = strlen(mes);
    // int loopc = 0;
    // while (1) {
    //     uprintf("loop %d: \n", loopc);
    //     error = vn_rdwr(UIO_WRITE, nd.ni_vp, mes, len, ofs,
    //             UIO_SYSSPACE, IO_NODELOCKED | IO_UNIT | IO_APPEND ,
    //             curthread->td_ucred, NOCRED, &resid, curthread);
    //     if (error) {
    //         uprintf("error writing, %d\n", error);
    //         break;
    //     }
    //     if (resid == 0)
    //         break;
    //     ofs += len - resid;
    // }
    // VOP_UNLOCK(nd.ni_vp, 0);
    // vn_close(nd.ni_vp, FWRITE, curthread->td_ucred, curthread);
    // return error;
}

static int EventHandler(struct module *inModule, int inEvent, void *inArg) {
    switch (inEvent) {
        case MOD_LOAD:
            uprintf("module loading.\n");
            if (catfile("/dev/echo") != 0)
                uprintf("Error reading /dev/xxx.\n");
            if (echofile("/dev/echo", "message") != 0)
               uprintf("Error writing /dev/xxx\n");
            return 0;
        case MOD_UNLOAD:
            uprintf("MOTD module unloading.\n");
            return 0;
        default:
            return EOPNOTSUPP;
    }
}

static moduledata_t  moduleData = {
    "motd_kmod",
    EventHandler,
    NULL
};

DECLARE_MODULE(motd_kmod, moduleData, SI_SUB_DRIVERS, SI_ORDER_MIDDLE);
