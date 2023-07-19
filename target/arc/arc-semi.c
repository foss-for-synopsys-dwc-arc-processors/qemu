#include "qemu/osdep.h"
#include "cpu.h"
#include "qemu/log.h"
#include "chardev/char-fe.h"
#include "qapi/error.h"
#include "exec/helper-proto.h"
#include "semihosting/semihost.h"

enum {
    TARGET_SYS_exit = 1,
    TARGET_SYS_read = 3,
    TARGET_SYS_write = 4,
    TARGET_SYS_open = 5,
    TARGET_SYS_close = 6,
    TARGET_SYS_unlink = 10,
    TARGET_SYS_time = 13,
    TARGET_SYS_lseek = 19,
    TARGET_SYS_times = 43,
    TARGET_SYS_gettimeofday = 78,
    TARGET_SYS_stat = 106, /* nsim stat's is corupted.  */
    TARGET_SYS_fstat = 108,

    TARGET_SYS_argc = 1000,
    TARGET_SYS_argv_sz = 1001,
    TARGET_SYS_argv = 1002,
    TARGET_SYS_memset = 1004,

    TARGET_SYS_errno = 2000,
};

enum {
    TARGET_ERRNO_EPERM = 1,			/* Not owner */
    TARGET_ERRNO_ENOENT = 2,		/* No such file or directory */
    TARGET_ERRNO_ESRCH = 3,			/* No such process */
    TARGET_ERRNO_EINTR = 4,			/* Interrupted system call */
    TARGET_ERRNO_EIO = 5,			/* I/O error */
    TARGET_ERRNO_ENXIO = 6,			/* No such device or address */
    TARGET_ERRNO_E2BIG = 7,			/* Arg list too long */
    TARGET_ERRNO_ENOEXEC = 8,		/* Exec format error */
    TARGET_ERRNO_EBADF = 9,			/* Bad file number */
    TARGET_ERRNO_ECHILD = 10,		/* No children */
    TARGET_ERRNO_EAGAIN = 11,		/* No more processes */
    TARGET_ERRNO_ENOMEM = 12,		/* Not enough space */
    TARGET_ERRNO_EACCES = 13,		/* Permission denied */
    TARGET_ERRNO_EFAULT = 14,		/* Bad address */
    TARGET_ERRNO_ENOTBLK = 15,	    /* Block device required */
    TARGET_ERRNO_EBUSY = 16,		/* Device or resource busy */
    TARGET_ERRNO_EEXIST = 17,		/* File exists */
    TARGET_ERRNO_EXDEV = 18,		/* Cross-device link */
    TARGET_ERRNO_ENODEV = 19,		/* No such device */
    TARGET_ERRNO_ENOTDIR = 20,		/* Not a directory */
    TARGET_ERRNO_EISDIR = 21,		/* Is a directory */
    TARGET_ERRNO_EINVAL = 22,		/* Invalid argument */
    TARGET_ERRNO_ENFILE = 23,		/* Too many open files in system */
    TARGET_ERRNO_EMFILE = 24,		/* File descriptor value too large */
    TARGET_ERRNO_ENOTTY = 25,		/* Not a character device */
    TARGET_ERRNO_ETXTBSY = 26,		/* Text file busy */
    TARGET_ERRNO_EFBIG = 27,		/* File too large */
    TARGET_ERRNO_ENOSPC = 28,		/* No space left on device */
    TARGET_ERRNO_ESPIPE = 29,		/* Illegal seek */
    TARGET_ERRNO_EROFS = 30,		/* Read-only file system */
    TARGET_ERRNO_EMLINK = 31,		/* Too many links */
    TARGET_ERRNO_EPIPE = 32,		/* Broken pipe */
    TARGET_ERRNO_EDOM = 33,			/* Mathematics argument out of domain of function */
    TARGET_ERRNO_ERANGE = 34,		/* Result too large */
    TARGET_ERRNO_ENOMSG = 35,		/* No message of desired type */
    TARGET_ERRNO_EIDRM = 36,		/* Identifier removed */
    TARGET_ERRNO_ECHRNG = 37,       /* Channel number out of range */
    TARGET_ERRNO_EL2NSYNC = 38,     /* Level 2 not synchronized */
    TARGET_ERRNO_EL3HLT = 39,	    /* Level 3 halted */
    TARGET_ERRNO_EL3RST = 40,       /* Level 3 reset */
    TARGET_ERRNO_ELNRNG = 41,	    /* Link number out of range */
    TARGET_ERRNO_EUNATCH = 42,	    /* Protocol driver not attached */
    TARGET_ERRNO_ENOCSI = 43,	    /* No CSI structure available */
    TARGET_ERRNO_EL2HLT = 44,	    /* Level 2 halted */
    TARGET_ERRNO_EDEADLK = 45,		/* Deadlock */
    TARGET_ERRNO_ENOLCK = 46,		/* No lock */
    TARGET_ERRNO_EBADE = 50,		/* Invalid exchange */
    TARGET_ERRNO_EBADR = 51,		/* Invalid request descriptor */
    TARGET_ERRNO_EXFULL = 52,		/* Exchange full */
    TARGET_ERRNO_ENOANO = 53,		/* No anode */
    TARGET_ERRNO_EBADRQC = 54,		/* Invalid request code */
    TARGET_ERRNO_EBADSLT = 55,		/* Invalid slot */
    TARGET_ERRNO_EDEADLOCK = 56,	/* File locking deadlock error */
    TARGET_ERRNO_EBFONT = 57,		/* Bad font file fmt */
    TARGET_ERRNO_ENOSTR = 60,		/* Not a stream */
    TARGET_ERRNO_ENODATA = 61,		/* No data (for no delay io) */
    TARGET_ERRNO_ETIME = 62,		/* Stream ioctl timeout */
    TARGET_ERRNO_ENOSR = 63,		/* No stream resources */
    TARGET_ERRNO_ENONET = 64,		/* Machine is not on the network */
    TARGET_ERRNO_ENOPKG = 65,		/* Package not installed */
    TARGET_ERRNO_EREMOTE = 66,		/* The object is remote */
    TARGET_ERRNO_ENOLINK = 67,		/* Virtual circuit is gone */
    TARGET_ERRNO_EADV = 68,			/* Advertise error */
    TARGET_ERRNO_ESRMNT = 69,		/* Srmount error */
    TARGET_ERRNO_ECOMM = 70,		/* Communication error on send */
    TARGET_ERRNO_EPROTO = 71,		/* Protocol error */
    TARGET_ERRNO_EMULTIHOP = 74,	/* Multihop attempted */
    TARGET_ERRNO_ELBIN = 75,		/* Inode is remote (not really error) */
    TARGET_ERRNO_EDOTDOT = 76,		/* Cross mount point (not really error) */
    TARGET_ERRNO_EBADMSG = 77,		/* Bad message */
    TARGET_ERRNO_EFTYPE = 79,		/* Inappropriate file type or format */
    TARGET_ERRNO_ENOTUNIQ = 80,		/* Given log. name not unique */
    TARGET_ERRNO_EBADFD = 81,		/* File descriptor in bad state */
    TARGET_ERRNO_EREMCHG = 82,		/* Remote address changed */
    TARGET_ERRNO_ELIBACC = 83,		/* Can't access a needed shared lib */
    TARGET_ERRNO_ELIBBAD = 84,		/* Accessing a corrupted shared lib */
    TARGET_ERRNO_ELIBSCN = 85,		/* .lib section in a.out corrupted */
    TARGET_ERRNO_ELIBMAX = 86,		/* Attempting to link in too many libs */
    TARGET_ERRNO_ELIBEXEC = 87,		/* Attempting to exec a shared library */
    TARGET_ERRNO_ENOSYS = 88,		/* Function not implemented */
    TARGET_ERRNO_ENMFILE = 89,      /* No more files */
    TARGET_ERRNO_ENOTEMPTY = 90,	/* Directory not empty */
    TARGET_ERRNO_ENAMETOOLONG = 91,	/* File or path name too long */
    TARGET_ERRNO_ELOOP = 92,		/* Too many symbolic links */
    TARGET_ERRNO_EOPNOTSUPP = 95,	/* Operation not supported on socket */
    TARGET_ERRNO_EPFNOSUPPORT = 96, /* Protocol family not supported */
    TARGET_ERRNO_ECONNRESET = 104,  /* Connection reset by peer */
    TARGET_ERRNO_ENOBUFS = 105,		/* No buffer space available */
    TARGET_ERRNO_EAFNOSUPPORT = 106,/* Address family not supported by protocol family */
    TARGET_ERRNO_EPROTOTYPE = 107,	/* Protocol wrong type for socket */
    TARGET_ERRNO_ENOTSOCK = 108,	/* Socket operation on non-socket */
    TARGET_ERRNO_ENOPROTOOPT = 109,	/* Protocol not available */
    TARGET_ERRNO_ESHUTDOWN = 110,	/* Can't send after socket shutdown */
    TARGET_ERRNO_ECONNREFUSED = 111,/* Connection refused */
    TARGET_ERRNO_EADDRINUSE = 112,	/* Address already in use */
    TARGET_ERRNO_ECONNABORTED = 113,/* Software caused connection abort */
    TARGET_ERRNO_ENETUNREACH = 114,	/* Network is unreachable */
    TARGET_ERRNO_ENETDOWN = 115,	/* Network interface is not configured */
    TARGET_ERRNO_ETIMEDOUT = 116,	/* Connection timed out */
    TARGET_ERRNO_EHOSTDOWN = 117,	/* Host is down */
    TARGET_ERRNO_EHOSTUNREACH = 118,/* Host is unreachable */
    TARGET_ERRNO_EINPROGRESS = 119,	/* Connection already in progress */
    TARGET_ERRNO_EALREADY = 120,	/* Socket already connected */
    TARGET_ERRNO_EDESTADDRREQ = 121,/* Destination address required */
    TARGET_ERRNO_EMSGSIZE = 122,	/* Message too long */
    TARGET_ERRNO_EPROTONOSUPPORT = 123,	/* Unknown protocol */
    TARGET_ERRNO_ESOCKTNOSUPPORT = 124,	/* Socket type not supported */
    TARGET_ERRNO_EADDRNOTAVAIL = 125,	/* Address not available */
    TARGET_ERRNO_ENETRESET = 126,   /* Connection aborted by network */
    TARGET_ERRNO_EISCONN = 127,		/* Socket is already connected */
    TARGET_ERRNO_ENOTCONN = 128,	/* Socket is not connected */
    TARGET_ERRNO_ETOOMANYREFS = 129,/* Too many references: cannot splice */
    TARGET_ERRNO_EPROCLIM = 130,    /* Too many processes */
    TARGET_ERRNO_EUSERS = 131,      /* Too many users */
    TARGET_ERRNO_EDQUOT = 132,      /* Reserved */
    TARGET_ERRNO_ESTALE = 133,      /* Reserved */
    TARGET_ERRNO_ENOTSUP = 134,     /* Not supported */
    TARGET_ERRNO_ENOMEDIUM = 135,   /* No medium found */
    TARGET_ERRNO_ENOSHARE = 136,    /* No such host or network path */
    TARGET_ERRNO_ECASECLASH = 137,  /* Filename exists with different case */
    TARGET_ERRNO_EILSEQ = 138,		/* Illegal byte sequence */
    TARGET_ERRNO_EOVERFLOW = 139,	/* Value too large for defined data type */
    TARGET_ERRNO_ECANCELED = 140,	/* Operation canceled */
    TARGET_ERRNO_ENOTRECOVERABLE = 141,	/* State not recoverable */
    TARGET_ERRNO_EOWNERDEAD = 142,	/* Previous owner died */
    TARGET_ERRNO_ESTRPIPE = 143,	/* Streams pipe error */
    TARGET_ERRNO_EHWPOISON = 144,   /* Memory page has hardware error */
    TARGET_ERRNO_EISNAM = 145,      /* Is a named type file */
    TARGET_ERRNO_EKEYEXPIRED = 146, /* Key has expired */
    TARGET_ERRNO_EKEYREJECTED = 147,/* Key was rejected by service */
    TARGET_ERRNO_EKEYREVOKED = 148, /* Key has been revoked */
};

static const struct {
    int host;
    int target;
} errno_map[] = {
#ifdef EPERM
    { .host = EPERM, .target = TARGET_ERRNO_EPERM },
#endif
#ifdef ENOENT
    { .host = ENOENT, .target = TARGET_ERRNO_ENOENT },
#endif
#ifdef ESRCH
    { .host = ESRCH, .target = TARGET_ERRNO_ESRCH },
#endif
#ifdef EINTR
    { .host = EINTR, .target = TARGET_ERRNO_EINTR },
#endif
#ifdef EIO
    { .host = EIO, .target = TARGET_ERRNO_EIO },
#endif
#ifdef ENXIO
    { .host = ENXIO, .target = TARGET_ERRNO_ENXIO },
#endif
#ifdef E2BIG
    { .host = E2BIG, .target = TARGET_ERRNO_E2BIG },
#endif
#ifdef ENOEXEC
    { .host = ENOEXEC, .target = TARGET_ERRNO_ENOEXEC },
#endif
#ifdef EBADF
    { .host = EBADF, .target = TARGET_ERRNO_EBADF },
#endif
#ifdef ECHILD
    { .host = ECHILD, .target = TARGET_ERRNO_ECHILD },
#endif
#ifdef EAGAIN
    { .host = EAGAIN, .target = TARGET_ERRNO_EAGAIN },
#endif
#ifdef ENOMEM
    { .host = ENOMEM, .target = TARGET_ERRNO_ENOMEM },
#endif
#ifdef EACCES
    { .host = EACCES, .target = TARGET_ERRNO_EACCES },
#endif
#ifdef EFAULT
    { .host = EFAULT, .target = TARGET_ERRNO_EFAULT },
#endif
#ifdef ENOTBLK
    { .host = ENOTBLK, .target = TARGET_ERRNO_ENOTBLK },
#endif
#ifdef EBUSY
    { .host = EBUSY, .target = TARGET_ERRNO_EBUSY },
#endif
#ifdef EEXIST
    { .host = EEXIST, .target = TARGET_ERRNO_EEXIST },
#endif
#ifdef EXDEV
    { .host = EXDEV, .target = TARGET_ERRNO_EXDEV },
#endif
#ifdef ENODEV
    { .host = ENODEV, .target = TARGET_ERRNO_ENODEV },
#endif
#ifdef ENOTDIR
    { .host = ENOTDIR, .target = TARGET_ERRNO_ENOTDIR },
#endif
#ifdef EISDIR
    { .host = EISDIR, .target = TARGET_ERRNO_EISDIR },
#endif
#ifdef EINVAL
    { .host = EINVAL, .target = TARGET_ERRNO_EINVAL },
#endif
#ifdef ENFILE
    { .host = ENFILE, .target = TARGET_ERRNO_ENFILE },
#endif
#ifdef EMFILE
    { .host = EMFILE, .target = TARGET_ERRNO_EMFILE },
#endif
#ifdef ENOTTY
    { .host = ENOTTY, .target = TARGET_ERRNO_ENOTTY },
#endif
#ifdef ETXTBSY
    { .host = ETXTBSY, .target = TARGET_ERRNO_ETXTBSY },
#endif
#ifdef EFBIG
    { .host = EFBIG, .target = TARGET_ERRNO_EFBIG },
#endif
#ifdef ENOSPC
    { .host = ENOSPC, .target = TARGET_ERRNO_ENOSPC },
#endif
#ifdef ESPIPE
    { .host = ESPIPE, .target = TARGET_ERRNO_ESPIPE },
#endif
#ifdef EROFS
    { .host = EROFS, .target = TARGET_ERRNO_EROFS },
#endif
#ifdef EMLINK
    { .host = EMLINK, .target = TARGET_ERRNO_EMLINK },
#endif
#ifdef EPIPE
    { .host = EPIPE, .target = TARGET_ERRNO_EPIPE },
#endif
#ifdef EDOM
    { .host = EDOM, .target = TARGET_ERRNO_EDOM },
#endif
#ifdef ERANGE
    { .host = ERANGE, .target = TARGET_ERRNO_ERANGE },
#endif
#ifdef ENOMSG
    { .host = ENOMSG, .target = TARGET_ERRNO_ENOMSG },
#endif
#ifdef EIDRM
    { .host = EIDRM, .target = TARGET_ERRNO_EIDRM },
#endif
#ifdef ECHRNG
    { .host = ECHRNG, .target = TARGET_ERRNO_ECHRNG },
#endif
#ifdef EL2NSYNC
    { .host = EL2NSYNC, .target = TARGET_ERRNO_EL2NSYNC },
#endif
#ifdef EL3HLT
    { .host = EL3HLT, .target = TARGET_ERRNO_EL3HLT },
#endif
#ifdef EL3RST
    { .host = EL3RST, .target = TARGET_ERRNO_EL3RST },
#endif
#ifdef ELNRNG
    { .host = ELNRNG, .target = TARGET_ERRNO_ELNRNG },
#endif
#ifdef EUNATCH
    { .host = EUNATCH, .target = TARGET_ERRNO_EUNATCH },
#endif
#ifdef ENOCSI
    { .host = ENOCSI, .target = TARGET_ERRNO_ENOCSI },
#endif
#ifdef EL2HLT
    { .host = EL2HLT, .target = TARGET_ERRNO_EL2HLT },
#endif
#ifdef EDEADLK
    { .host = EDEADLK, .target = TARGET_ERRNO_EDEADLK },
#endif
#ifdef ENOLCK
    { .host = ENOLCK, .target = TARGET_ERRNO_ENOLCK },
#endif
#ifdef EBADE
    { .host = EBADE, .target = TARGET_ERRNO_EBADE },
#endif
#ifdef EBADR
    { .host = EBADR, .target = TARGET_ERRNO_EBADR },
#endif
#ifdef EXFULL
    { .host = EXFULL, .target = TARGET_ERRNO_EXFULL },
#endif
#ifdef ENOANO
    { .host = ENOANO, .target = TARGET_ERRNO_ENOANO },
#endif
#ifdef EBADRQC
    { .host = EBADRQC, .target = TARGET_ERRNO_EBADRQC },
#endif
#ifdef EBADSLT
    { .host = EBADSLT, .target = TARGET_ERRNO_EBADSLT },
#endif
#ifdef EDEADLOCK
    { .host = EDEADLOCK, .target = TARGET_ERRNO_EDEADLOCK },
#endif
#ifdef EBFONT
    { .host = EBFONT, .target = TARGET_ERRNO_EBFONT },
#endif
#ifdef ENOSTR
    { .host = ENOSTR, .target = TARGET_ERRNO_ENOSTR },
#endif
#ifdef ENODATA
    { .host = ENODATA, .target = TARGET_ERRNO_ENODATA },
#endif
#ifdef ETIME
    { .host = ETIME, .target = TARGET_ERRNO_ETIME },
#endif
#ifdef ENOSR
    { .host = ENOSR, .target = TARGET_ERRNO_ENOSR },
#endif
#ifdef ENONET
    { .host = ENONET, .target = TARGET_ERRNO_ENONET },
#endif
#ifdef ENOPKG
    { .host = ENOPKG, .target = TARGET_ERRNO_ENOPKG },
#endif
#ifdef EREMOTE
    { .host = EREMOTE, .target = TARGET_ERRNO_EREMOTE },
#endif
#ifdef ENOLINK
    { .host = ENOLINK, .target = TARGET_ERRNO_ENOLINK },
#endif
#ifdef EADV
    { .host = EADV, .target = TARGET_ERRNO_EADV },
#endif
#ifdef ESRMNT
    { .host = ESRMNT, .target = TARGET_ERRNO_ESRMNT },
#endif
#ifdef ECOMM
    { .host = ECOMM, .target = TARGET_ERRNO_ECOMM },
#endif
#ifdef EPROTO
    { .host = EPROTO, .target = TARGET_ERRNO_EPROTO },
#endif
#ifdef EMULTIHOP
    { .host = EMULTIHOP, .target = TARGET_ERRNO_EMULTIHOP },
#endif
#ifdef ELBIN
    { .host = ELBIN, .target = TARGET_ERRNO_ELBIN },
#endif
#ifdef EDOTDOT
    { .host = EDOTDOT, .target = TARGET_ERRNO_EDOTDOT },
#endif
#ifdef EBADMSG
    { .host = EBADMSG, .target = TARGET_ERRNO_EBADMSG },
#endif
#ifdef EFTYPE
    { .host = EFTYPE, .target = TARGET_ERRNO_EFTYPE },
#endif
#ifdef ENOTUNIQ
    { .host = ENOTUNIQ, .target = TARGET_ERRNO_ENOTUNIQ },
#endif
#ifdef EBADFD
    { .host = EBADFD, .target = TARGET_ERRNO_EBADFD },
#endif
#ifdef EREMCHG
    { .host = EREMCHG, .target = TARGET_ERRNO_EREMCHG },
#endif
#ifdef ELIBACC
    { .host = ELIBACC, .target = TARGET_ERRNO_ELIBACC },
#endif
#ifdef ELIBBAD
    { .host = ELIBBAD, .target = TARGET_ERRNO_ELIBBAD },
#endif
#ifdef ELIBSCN
    { .host = ELIBSCN, .target = TARGET_ERRNO_ELIBSCN },
#endif
#ifdef ELIBMAX
    { .host = ELIBMAX, .target = TARGET_ERRNO_ELIBMAX },
#endif
#ifdef ELIBEXEC
    { .host = ELIBEXEC, .target = TARGET_ERRNO_ELIBEXEC },
#endif
#ifdef ENOSYS
    { .host = ENOSYS, .target = TARGET_ERRNO_ENOSYS },
#endif
#ifdef ENMFILE
    { .host = ENMFILE, .target = TARGET_ERRNO_ENMFILE },
#endif
#ifdef ENOTEMPTY
    { .host = ENOTEMPTY, .target = TARGET_ERRNO_ENOTEMPTY },
#endif
#ifdef ENAMETOOLONG
    { .host = ENAMETOOLONG, .target = TARGET_ERRNO_ENAMETOOLONG },
#endif
#ifdef ELOOP
    { .host = ELOOP, .target = TARGET_ERRNO_ELOOP },
#endif
#ifdef EOPNOTSUPP
    { .host = EOPNOTSUPP, .target = TARGET_ERRNO_EOPNOTSUPP },
#endif
#ifdef EPFNOSUPPORT
    { .host = EPFNOSUPPORT, .target = TARGET_ERRNO_EPFNOSUPPORT },
#endif
#ifdef ECONNRESET
    { .host = ECONNRESET, .target = TARGET_ERRNO_ECONNRESET },
#endif
#ifdef ENOBUFS
    { .host = ENOBUFS, .target = TARGET_ERRNO_ENOBUFS },
#endif
#ifdef EAFNOSUPPORT
    { .host = EAFNOSUPPORT, .target = TARGET_ERRNO_EAFNOSUPPORT },
#endif
#ifdef EPROTOTYPE
    { .host = EPROTOTYPE, .target = TARGET_ERRNO_EPROTOTYPE },
#endif
#ifdef ENOTSOCK
    { .host = ENOTSOCK, .target = TARGET_ERRNO_ENOTSOCK },
#endif
#ifdef ENOPROTOOPT
    { .host = ENOPROTOOPT, .target = TARGET_ERRNO_ENOPROTOOPT },
#endif
#ifdef ESHUTDOWN
    { .host = ESHUTDOWN, .target = TARGET_ERRNO_ESHUTDOWN },
#endif
#ifdef ECONNREFUSED
    { .host = ECONNREFUSED, .target = TARGET_ERRNO_ECONNREFUSED },
#endif
#ifdef EADDRINUSE
    { .host = EADDRINUSE, .target = TARGET_ERRNO_EADDRINUSE },
#endif
#ifdef ECONNABORTED
    { .host = ECONNABORTED, .target = TARGET_ERRNO_ECONNABORTED },
#endif
#ifdef ENETUNREACH
    { .host = ENETUNREACH, .target = TARGET_ERRNO_ENETUNREACH },
#endif
#ifdef ENETDOWN
    { .host = ENETDOWN, .target = TARGET_ERRNO_ENETDOWN },
#endif
#ifdef ETIMEDOUT
    { .host = ETIMEDOUT, .target = TARGET_ERRNO_ETIMEDOUT },
#endif
#ifdef EHOSTDOWN
    { .host = EHOSTDOWN, .target = TARGET_ERRNO_EHOSTDOWN },
#endif
#ifdef EHOSTUNREACH
    { .host = EHOSTUNREACH, .target = TARGET_ERRNO_EHOSTUNREACH },
#endif
#ifdef EINPROGRESS
    { .host = EINPROGRESS, .target = TARGET_ERRNO_EINPROGRESS },
#endif
#ifdef EALREADY
    { .host = EALREADY, .target = TARGET_ERRNO_EALREADY },
#endif
#ifdef EDESTADDRREQ
    { .host = EDESTADDRREQ, .target = TARGET_ERRNO_EDESTADDRREQ },
#endif
#ifdef EMSGSIZE
    { .host = EMSGSIZE, .target = TARGET_ERRNO_EMSGSIZE },
#endif
#ifdef EPROTONOSUPPORT
    { .host = EPROTONOSUPPORT, .target = TARGET_ERRNO_EPROTONOSUPPORT },
#endif
#ifdef ESOCKTNOSUPPORT
    { .host = ESOCKTNOSUPPORT, .target = TARGET_ERRNO_ESOCKTNOSUPPORT },
#endif
#ifdef EADDRNOTAVAIL
    { .host = EADDRNOTAVAIL, .target = TARGET_ERRNO_EADDRNOTAVAIL },
#endif
#ifdef ENETRESET
    { .host = ENETRESET, .target = TARGET_ERRNO_ENETRESET },
#endif
#ifdef EISCONN
    { .host = EISCONN, .target = TARGET_ERRNO_EISCONN },
#endif
#ifdef ENOTCONN
    { .host = ENOTCONN, .target = TARGET_ERRNO_ENOTCONN },
#endif
#ifdef ETOOMANYREFS
    { .host = ETOOMANYREFS, .target = TARGET_ERRNO_ETOOMANYREFS },
#endif
#ifdef EPROCLIM
    { .host = EPROCLIM, .target = TARGET_ERRNO_EPROCLIM },
#endif
#ifdef EUSERS
    { .host = EUSERS, .target = TARGET_ERRNO_EUSERS },
#endif
#ifdef EDQUOT
    { .host = EDQUOT, .target = TARGET_ERRNO_EDQUOT },
#endif
#ifdef ESTALE
    { .host = ESTALE, .target = TARGET_ERRNO_ESTALE },
#endif
#ifdef ENOTSUP
    { .host = ENOTSUP, .target = TARGET_ERRNO_ENOTSUP },
#endif
#ifdef ENOMEDIUM
    { .host = ENOMEDIUM, .target = TARGET_ERRNO_ENOMEDIUM },
#endif
#ifdef ENOSHARE
    { .host = ENOSHARE, .target = TARGET_ERRNO_ENOSHARE },
#endif
#ifdef ECASECLASH
    { .host = ECASECLASH, .target = TARGET_ERRNO_ECASECLASH },
#endif
#ifdef EILSEQ
    { .host = EILSEQ, .target = TARGET_ERRNO_EILSEQ },
#endif
#ifdef EOVERFLOW
    { .host = EOVERFLOW, .target = TARGET_ERRNO_EOVERFLOW },
#endif
#ifdef ECANCELED
    { .host = ECANCELED, .target = TARGET_ERRNO_ECANCELED },
#endif
#ifdef ENOTRECOVERABLE
    { .host = ENOTRECOVERABLE, .target = TARGET_ERRNO_ENOTRECOVERABLE },
#endif
#ifdef EOWNERDEAD
    { .host = EOWNERDEAD, .target = TARGET_ERRNO_EOWNERDEAD },
#endif
#ifdef ESTRPIPE
    { .host = ESTRPIPE, .target = TARGET_ERRNO_ESTRPIPE },
#endif
#ifdef EHWPOISON
    { .host = EHWPOISON, .target = TARGET_ERRNO_EHWPOISON },
#endif
#ifdef EISNAM
    { .host = EISNAM, .target = TARGET_ERRNO_EISNAM },
#endif
#ifdef EKEYEXPIRED
    { .host = EKEYEXPIRED, .target = TARGET_ERRNO_EKEYEXPIRED },
#endif
#ifdef EKEYREJECTED
    { .host = EKEYREJECTED, .target = TARGET_ERRNO_EKEYREJECTED },
#endif
#ifdef EKEYREVOKED
    { .host = EKEYREVOKED, .target = TARGET_ERRNO_EKEYREVOKED },
#endif
};

static int
map_errno(int semihosting_errno)
{
    unsigned i;
    for (i = 0; i < sizeof(errno_map)/sizeof(errno_map[0]); i++)
        if (errno_map[i].host == semihosting_errno)
            return errno_map[i].target;
    return TARGET_ERRNO_EINVAL;
}

typedef struct ArcSimConsole {
    CharBackend be;
    struct {
        char buffer[16];
        size_t offset;
    } input;
} ArcSimConsole;

static ArcSimConsole *sim_console;

static IOCanReadHandler sim_console_can_read;

static int sim_console_can_read(void *opaque)
{
    ArcSimConsole *p = opaque;

    return sizeof(p->input.buffer) - p->input.offset;
}

static IOReadHandler sim_console_read;

static void sim_console_read(void *opaque, const uint8_t *buf, int size)
{
    ArcSimConsole *p = opaque;
    size_t copy = sizeof(p->input.buffer) - p->input.offset;

    if (size < copy) {
        copy = size;
    }
    memcpy(p->input.buffer + p->input.offset, buf, copy);
    p->input.offset += copy;
}

/*
 * Conversion between linux's struct stat to newlib's struct stat.
 *
 * The struct stat represents file status information in the Linux system.
 * It contains various fields that describe the attributes and properfites of
 * a file or directory.
 *
 * Path: newlib/libc/include/sys/stat.h
 *
 * struct	stat
 * {
 *   dev_t		st_dev;
 *   ino_t		st_ino;
 *   mode_t	st_mode;
 *   nlink_t	st_nlink;
 *   uid_t		st_uid;
 *   gid_t		st_gid;
 *   dev_t		st_rdev;
 *   off_t		st_size;
 *   ...
 *   struct timespec st_atim;
 *   struct timespec st_mtim;
 *   struct timespec st_ctim;
 *   blksize_t     st_blksize;
 *   blkcnt_t	st_blocks;
 * #if !defined(__rtems__)
 *   long		st_spare4[2];
 * #endif
 * #endif
 * };
 *
 * The struct arc_stat is a modified version of the newlib's stat struct,
 * designed to support both 32-bit and 64-bit systems.
 */
struct arc_stat
{
    uint16_t my_dev;
	uint16_t my___pad1;
	uint32_t my_ino;
	uint16_t my_mode;
	uint16_t my_nlink;
	uint16_t my_uid;
	uint16_t my_gid;
	uint16_t my_rdev;
	uint16_t my___pad2;
	uint32_t my_size;
	uint32_t my_blksize;
	uint32_t my_blocks;
	uint32_t my_atime;
	uint32_t my___unused1;
	uint32_t my_mtime;
	uint32_t my___unused2;
	uint32_t my_ctime;
	uint32_t my___unused3;
	uint32_t my___unused4;
	uint32_t my___unused5;
};

static struct arc_stat *conv_stat(struct stat *st);

/* Converts linux's stat to newlib's stat struct */
static struct arc_stat *conv_stat(struct stat *st)
{
    struct arc_stat *arc_st = malloc (sizeof (struct arc_stat));

    arc_st->my_dev = st->st_dev;
    arc_st->my_ino = st->st_ino;
    arc_st->my_mode = st->st_mode;
    arc_st->my_nlink = st->st_nlink;
    arc_st->my_uid = st->st_uid;
    arc_st->my_gid = st->st_gid;
    arc_st->my_rdev = st->st_rdev;
    arc_st->my_size = st->st_size;
    arc_st->my_blocks = st->st_blocks;
    arc_st->my_blksize = st->st_blksize;
    arc_st->my_atime = st->st_atime;
    arc_st->my_mtime = st->st_mtime;
    arc_st->my_ctime = st->st_ctime;

    return arc_st;
}

void arc_sim_open_console(Chardev *chr)
{
    static ArcSimConsole console;

    qemu_chr_fe_init(&console.be, chr, &error_abort);
    qemu_chr_fe_set_handlers(&console.be,
                             sim_console_can_read,
                             sim_console_read,
                             NULL, NULL, &console,
                             NULL, true);
    sim_console = &console;
}

static int arc_semihosting_errno;

/*
 * SYSCALL0:
 *  - Input: R8 syscall name;
 *  - Output: R0 return code.
 *
 * SYSCALL1:
 *  -Input: R0 Arg0, R8 syscall name;
 *  -OutputL R0 return code.
 */
void do_arc_semihosting(CPUARCState *env)
{
    CPUState *cs = env_cpu(env);
    target_ulong *regs = env->r;

    switch (regs[8]) {
    case TARGET_SYS_exit:
        exit(regs[0]);
        break;

    case TARGET_SYS_read:
    case TARGET_SYS_write:
        {
            bool is_write = regs[8] == TARGET_SYS_write;
            uint32_t fd = (uint32_t) regs[0];
            target_ulong vaddr = regs[1];
            uint32_t len = (uint32_t) regs[2];
            uint32_t len_done = 0;

            while (len > 0) {
                hwaddr paddr = cpu_get_phys_page_debug(cs, vaddr);
                uint32_t page_left =
                    TARGET_PAGE_SIZE - (vaddr & (TARGET_PAGE_SIZE - 1));
                uint32_t io_sz = page_left < len ? page_left : len;
                hwaddr sz = io_sz;
                void *buf = cpu_physical_memory_map(paddr, &sz, !is_write);
                uint32_t io_done;
                bool error = false;

                if (buf) {
                    vaddr += io_sz;
                    len -= io_sz;
                    if (fd < 3 && sim_console) {
                        if (is_write && (fd == 1 || fd == 2)) {
                            /* stdout, stderr */
                            io_done = qemu_chr_fe_write_all(&sim_console->be,
                                                            buf, io_sz);
                        } else if (!is_write && fd == 0) {
                            /* stdin */
                            if (sim_console->input.offset) {
                                io_done = sim_console->input.offset;
                                if (io_sz < io_done) {
                                    io_done = io_sz;
                                }
                                memcpy(buf, sim_console->input.buffer, io_done);
                                memmove(sim_console->input.buffer,
                                        sim_console->input.buffer + io_done,
                                        sim_console->input.offset - io_done);
                                sim_console->input.offset -= io_done;
                                qemu_chr_fe_accept_input(&sim_console->be);
                            } else {
                                io_done = -1;
                            }
                        } else {
                            qemu_log_mask(LOG_GUEST_ERROR,
                                          "%s fd %d is not supported with chardev console\n",
                                          is_write ?
                                          "writing to" : "reading from", fd);
                            io_done = -1;
                        }
                    } else {
                        io_done = is_write ?
                            write(fd, buf, io_sz) :
                            read(fd, buf, io_sz);
                    }
                    if (io_done == -1) {
                        error = true;
                        arc_semihosting_errno = errno;
                        io_done = 0;
                    }
                    cpu_physical_memory_unmap(buf, sz, !is_write, io_done);
                } else {
                    error = true;
                    break;
                }
                if (error) {
                    if (!len_done) {
                        len_done = -1;
                    }
                    break;
                }
                len_done += io_done;
                if (io_done < io_sz) {
                    break;
                }
            }
            regs[0] = len_done;
        }
        break;

    case TARGET_SYS_open:
        {
            char name[1024];
            int rc;
            int i;

            for (i = 0; i < ARRAY_SIZE(name); ++i) {
                rc = cpu_memory_rw_debug(cs, regs[0] + i,
                                         (uint8_t *)name + i, 1, 0);
                if (rc != 0 || name[i] == 0) {
                    break;
                }
            }

            if (rc == 0 && i < ARRAY_SIZE(name)) {
                regs[0] = open(name, regs[1], regs[2]);
                arc_semihosting_errno = errno;
            } else {
                regs[0] = -1;
            }
        }
        break;

    case TARGET_SYS_close:
        if (regs[0] < 3) {
            /* Ignore attempts to close stdin/out/err. */
            regs[0] = 0;
        } else {
            regs[0] = close(regs[0]);
            arc_semihosting_errno = errno;
        }
        break;

    case TARGET_SYS_lseek:
        regs[0] = lseek(regs[0], (off_t)(int32_t)regs[1], regs[2]);
        arc_semihosting_errno = errno;
        break;

    case TARGET_SYS_times:
    case TARGET_SYS_time:
        regs[0] = time (NULL);
        arc_semihosting_errno = errno;
        break;

    case TARGET_SYS_gettimeofday:
    {
        qemu_timeval tv;
        struct timeval p;
        uint32_t result = qemu_gettimeofday(&tv);
        arc_semihosting_errno = errno;
        target_ulong base = regs[0];
        uint32_t sz = sizeof (struct timeval);
        hwaddr len = sz;
        void *buf = cpu_physical_memory_map(base, &len, 1);

        p.tv_sec = tv.tv_sec;
        p.tv_usec = tv.tv_usec;
        if (buf)
        {
            memcpy(buf, &p, sizeof (struct timeval));
            cpu_physical_memory_unmap(buf, len, 1, len);
        }
        regs[0] = result;
        break;
    }

    /*
     * Expected register values:
     * regs[0] (r0) - AFile descriptor
     * regs[1] (r1) - Address of newlib structure buffer
     */
    case TARGET_SYS_fstat:
    {
        /*
         * Initialize a stat struct to store
         * file status information
         */
        struct stat sbuf;
        memset(&sbuf, 0, sizeof(sbuf));

        /*
         * Call the fstat system call with the
         * file descriptor and store the result in regs[0]
         */
        regs[0] = fstat(regs[0], &sbuf);
        arc_semihosting_errno = errno;

        /*
         * Converts linux's stat struct to newlib's stat struct
         */
        struct arc_stat *arc_sbuf = conv_stat(&sbuf);

        /*
         * Map the physical memory to a buffer at the specified address
         */
        hwaddr len = sizeof(*arc_sbuf);
        void *buf = cpu_physical_memory_map(regs[1], &len, 1);

        /*
         * If the buffer mapping is successful,
         * copy the converted stat structure to the buffer
         */
        if (buf)
        {
            memcpy(buf, arc_sbuf, sizeof(*arc_sbuf));
            cpu_physical_memory_unmap(buf, len, 1, len);
        }

        free(arc_sbuf);
        break;
    }

    /*
     * Expected register values:
     * regs[0] (r0) - Address of the file name string in memory
     * regs[1] (r1) - Address of newlib structure buffer
     */
    case TARGET_SYS_stat:
    {
        /*
         *  Create a character array to store the file name
         */
        char name[1024];
        int rc;
        int i;

        /*
         * Read the file name character by character from the memory
         */
        for (i = 0; i < ARRAY_SIZE(name); ++i) {
            rc = cpu_memory_rw_debug(cs, regs[0] + i,
                                     (uint8_t *)name + i, 1, 0);
            if (rc != 0 || name[i] == 0) {
                break;
            }
        }

        /*
         * Initialize a stat struct to store
         * file status information
         */
        struct stat sbuf;
        memset(&sbuf, 0, sizeof(sbuf));

        /*
         * Call the stat system call with the
         * file name and store the result in regs[0]
         */
        regs[0] = stat(name, &sbuf);
        arc_semihosting_errno = errno;

        /*
         * Converts linux's stat struct to newlib's stat struct
         */
        struct arc_stat *arc_sbuf = conv_stat(&sbuf);

        /*
         * Map the physical memory to a buffer at the specified address
         */
        hwaddr len = sizeof(*arc_sbuf);
        void *buf = cpu_physical_memory_map(regs[1], &len, 1);
        /*
         * If the buffer mapping is successful,
         * copy the converted stat structure to the buffer
         */
        if (buf)
        {
            memcpy(buf, arc_sbuf, sizeof(*arc_sbuf));
            cpu_physical_memory_unmap(buf, len, 1, len);
        }

        free(arc_sbuf);
        break;
    }
    case TARGET_SYS_errno:
        regs[0] = map_errno(arc_semihosting_errno);
        break;
#if 0
    case TARGET_SYS_argc:
        regs[2] = semihosting_get_argc();
        regs[3] = 0;
        break;

    case TARGET_SYS_argv_sz:
        {
            int argc = semihosting_get_argc();
            int sz = (argc + 1) * sizeof(uint32_t);
            int i;

            for (i = 0; i < argc; ++i) {
                sz += 1 + strlen(semihosting_get_arg(i));
            }
            regs[2] = sz;
            regs[3] = 0;
        }
        break;

    case TARGET_SYS_argv:
        {
            int argc = semihosting_get_argc();
            int str_offset = (argc + 1) * sizeof(uint32_t);
            int i;
            uint32_t argptr;

            for (i = 0; i < argc; ++i) {
                const char *str = semihosting_get_arg(i);
                int str_size = strlen(str) + 1;

                argptr = tswap32(regs[3] + str_offset);

                cpu_memory_rw_debug(cs,
                                    regs[3] + i * sizeof(uint32_t),
                                    (uint8_t *)&argptr, sizeof(argptr), 1);
                cpu_memory_rw_debug(cs,
                                    regs[3] + str_offset,
                                    (uint8_t *)str, str_size, 1);
                str_offset += str_size;
            }
            argptr = 0;
            cpu_memory_rw_debug(cs,
                                regs[3] + i * sizeof(uint32_t),
                                (uint8_t *)&argptr, sizeof(argptr), 1);
            regs[3] = 0;
        }
        break;

    case TARGET_SYS_memset:
        {
            uint32_t base = regs[3];
            uint32_t sz = regs[5];

            while (sz) {
                hwaddr len = sz;
                void *buf = cpu_physical_memory_map(base, &len, 1);

                if (buf && len) {
                    memset(buf, regs[4], len);
                    cpu_physical_memory_unmap(buf, len, 1, len);
                } else {
                    len = 1;
                }
                base += len;
                sz -= len;
            }
            regs[2] = regs[3];
            regs[3] = 0;
        }
        break;
#endif

    default:
        qemu_log_mask(LOG_GUEST_ERROR, "%s(%d): not implemented\n", __func__,
                      (uint32_t) regs[8]);
        regs[0] = -1;
        break;
    }
}
