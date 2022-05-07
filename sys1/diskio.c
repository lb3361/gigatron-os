/*-----------------------------------------------------------------------*/
/* Low level disk I/O module for FatFs on the Gigatron                   */
/*-----------------------------------------------------------------------*/

#include "ff.h"         /* Obtains integer types */
#include "diskio.h"     /* Declarations of disk functions */
#include "spi.h"

#include <string.h>
#include <gigatron/sys.h>
#include <gigatron/libc.h>
#include <gigatron/console.h>

#define CMDVERBOSE 0
#define INITVERBOSE 0
#define MULTIPLE 0

/*-----------------------------------------------------------------------*/
/* Private                                                               */
/*-----------------------------------------------------------------------*/


#if FF_MAX_SS != 512 || FF_MIN_SS != 512
#error Sector size must be 512 bytes
#endif


/* MMC/SD command (SPI mode) */
#define CMD0    (0)         /* GO_IDLE_STATE */
#define CMD1    (1)         /* SEND_OP_COND */
#define ACMD41  (0x80+41)   /* SEND_OP_COND (SDC) */
#define CMD8    (8)         /* SEND_IF_COND */
#define CMD9    (9)         /* SEND_CSD */
#define CMD10   (10)        /* SEND_CID */
#define CMD12   (12)        /* STOP_TRANSMISSION */
#define CMD13   (13)        /* SEND_STATUS */
#define ACMD13  (0x80+13)   /* SD_STATUS (SDC) */
#define CMD16   (16)        /* SET_BLOCKLEN */
#define CMD17   (17)        /* READ_SINGLE_BLOCK */
#define CMD18   (18)        /* READ_MULTIPLE_BLOCK */
#define CMD23   (23)        /* SET_BLOCK_COUNT */
#define ACMD23  (0x80+23)   /* SET_WR_BLK_ERASE_COUNT (SDC) */
#define CMD24   (24)        /* WRITE_BLOCK */
#define CMD25   (25)        /* WRITE_MULTIPLE_BLOCK */
#define CMD32   (32)        /* ERASE_ER_BLK_START */
#define CMD33   (33)        /* ERASE_ER_BLK_END */
#define CMD38   (38)        /* ERASE */
#define CMD55   (55)        /* APP_CMD */
#define CMD58   (58)        /* READ_OCR */

static DSTATUS Stat[2] = { STA_NOINIT, STA_NOINIT };
static BYTE CardType[2];  /* b0:MMC, b1:SDv1, b2:SDv2, b3:Block addressing */
static BYTE CID[2][16];
static BYTE Drive;


static void deselect (void)
{
    register BYTE ctrl = ctrlBits_v5 | 0xc;
    SYS_ExpanderControl(ctrl);
}

static void select (void)
{
    register BYTE ctrl = ctrlBits_v5 | 0xc;
    if (Drive == 0)
        ctrl ^= 4;
    else if (Drive == 1)
        ctrl ^= 8;
    SYS_ExpanderControl(ctrl);
}

static int wait_ready(void) /* 1:OK, 0:Timeout */
{
    BYTE d;
    register UINT tmr;
    for (tmr = 5000; tmr; tmr--) {  /* Wait for ready in timeout of 500ms */
        spi_recv(&d, 1);
        if (d == 0xFF)
            break;
    }
    return tmr ? 1 : 0;
}

static BYTE reselect(void)  /* 1:OK, 0:Timeout */
{
    deselect();
    select();
    if (wait_ready())
        return 1;
    deselect();
    return 0;
}

static
int recv_datablock(register BYTE *buff, register UINT btr)
{
    BYTE d[2];
    register UINT tmr;
    for (tmr = 1000; tmr; tmr--) {  /* Wait for data packet in timeout of 100ms */
        spi_recv(d, 1);
        if (d[0] != 0xFF)
            break;
    }
    if (d[0] != 0xFE)               /* If not valid data token, return with error */
        return 0;
    spi_recv(buff, btr);            /* Receive the data block into buffer */
    spi_recv(d, 2);                 /* Discard CRC */
    return 1;                       /* Return with success */
}

#if ! FF_READONLY

static
int xmit_datablock(register const BYTE *buff, register BYTE token)
{
    BYTE d[2];
    if (!wait_ready())
        return 0;
    d[0] = token;
    spi_send(d, 1);             /* Xmit a token */
    if (token != 0xFD) {    /* Is it data token? */
        spi_send(buff, 512);    /* Xmit the 512 byte data block to MMC */
        spi_recv(d, 2);         /* Xmit dummy CRC (0xFF,0xFF) */
        spi_recv(d, 1);         /* Receive data response */
        if ((d[0] & 0x1F) != 0x05)  /* If not accepted, return with error */
            return 0;
    }
    return 1;
}

#endif

#if CMDVERBOSE
static BYTE lastc;
#endif

static void cmdsend(register BYTE c, register DWORD arg)
{
    BYTE buf[6];    
    register BYTE n;
#if CMDVERBOSE
    cprintf("CMD %d: ", lastc = c);
#endif
    buf[0] = 0x40 | c;              /* Start + Command index */
    buf[1] = (BYTE)(arg >> 24);     /* Argument[31..24] */
    buf[2] = (BYTE)(arg >> 16);     /* Argument[23..16] */
    buf[3] = (BYTE)(arg >> 8);      /* Argument[15..8] */
    buf[4] = (BYTE)arg;             /* Argument[7..0] */
    n = 0x01;                       /* Dummy CRC + Stop */
    if (c == CMD0) n = 0x95;        /* (valid CRC for CMD0(0)) */
    if (c == CMD8) n = 0x87;        /* (valid CRC for CMD8(0x1AA)) */
    buf[5] = n;
    spi_send(buf, 6);
}

static BYTE cmdreply(void)
{
    BYTE d;
    register BYTE n = 10;
    do {
        spi_recv(&d, 1);
    } while ((d & 0x80) && --n);
#if CMDVERBOSE
    n = d;
    if (lastc == CMD0 || lastc == CMD8 || lastc == CMD55)
        n &= 0xfe;
    cprintf("%d %s\n", d, (n) ? "FAIL" : "OK");
#endif
    return d;
}

static BYTE send_cmd0(void)
{
    BYTE d;
    register BYTE n;
    frameCount = 0;
    for(;;) {
        deselect();
        for (n = 10; n; n--)
            spi_recv(&d, 1);    /* Apply 80 dummy clocks, CS high */
        select();
        spi_recv(&d, 1);        /* Stuff byte, CS low   */
        cmdsend(CMD0, 0);       /* CMD0 command, CS low */
        if ((n = cmdreply()) == 1) {
            wait_ready();
            return n;
        }
        if (frameCount >= 60)   /* No more than 1 second */
            return 0xff;
    }
}

static BYTE send_cmd12(void)
{
    BYTE d;
    cmdsend(CMD12, 0);
    spi_recv(&d, 1);    /* skip stuff byte */
    return cmdreply();
}

static BYTE send_cmd(register BYTE c, register DWORD arg)
{
    register BYTE n;
    if (! reselect())
        return 0xff;
    if (c & 0x80) {     /* ACMD<n> */
        c &= 0x7F;
        cmdsend(CMD55, 0);
        n = cmdreply();
        if (n & 0xfe)
            return n;
        wait_ready();
    }
    cmdsend(c, arg);
    n = cmdreply();
    if (n & 0x80)       /* Timeout */
        Stat[Drive] = STA_NOINIT;
    return n;
}

static BYTE read_cid(BYTE *buf16)
{
    register BYTE d;
    if ((d = send_cmd(CMD10, 0)))
        return d;
    if (!recv_datablock(buf16, 16))
        return 0xff;
    return 0;
}


/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (BYTE drv)
{
    if (drv & 0xfe)
        return RES_NOTRDY;
    return Stat[drv];
}


/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (BYTE drv)
{
    register BYTE n, ty, c;
    register UINT tmr;
    register DSTATUS s;
    BYTE buf[4];
    
    if (drv & 0xfe)
        return RES_NOTRDY;
    Drive = drv;
    ty = 0;
    if (send_cmd0() == 1) { /* Enter Idle state in SPI mode */

        if (send_cmd(CMD8, 0x1AA) == 1) {   /* SDv2? */
            spi_recv(buf, 4);                           /* Get trailing return value of R7 resp */
            if (buf[2] == 0x01 && buf[3] == 0xAA) {     /* The card can work at vdd range of 2.7-3.6V */
                for (tmr = 2000; tmr; tmr--)            /* Wait for leaving idle state (ACMD41 with HCS bit) */
                    if (send_cmd(ACMD41, 1UL << 30) == 0)
                        break;
                if (tmr && send_cmd(CMD58, 0) == 0) {   /* Check CCS bit in the OCR */
                    spi_recv(buf, 4);
                    ty = (buf[0] & 0x40) ? 4 : 2;
                }
            }
        } else {            /* SDv1 or MMCv3 */
            if (send_cmd(ACMD41, 0) <= 1) {
                ty = 2; c = ACMD41; /* SDv1 */
            } else {
                ty = 1; c = CMD1;   /* MMCv3 */
            }
            for (tmr = 200; tmr; tmr--)             /* Wait for leaving idle state */
                if (send_cmd(c, 0) == 0)
                    break;
            if (!tmr || send_cmd(CMD16, 512) != 0)  /* Set R/W block length to 512 */
                ty = 0;
        }
    }
    CardType[drv] = ty;
#if INITVERBOSE
    cprintf("CardType=%d\n", ty);
#endif
    s = STA_NOINIT;
    if (ty && !read_cid(CID[drv]))
        s = 0;
    Stat[drv] = s;
    deselect();
    return s;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (BYTE drv,        /* Physical drive nmuber to identify the drive */
                   BYTE *buff,      /* Data buffer to store read data */
                   LBA_t sector,    /* Start sector in LBA */
                   UINT count)      /* Number of sectors to read */
{
    register BYTE c;
    register DWORD sect = (DWORD)sector;
    
    if (drv & 0xfe)
        return RES_PARERR;
    if (Stat[drv] & (STA_NOINIT|STA_NODISK))
        return RES_NOTRDY;
    Drive = drv;
    if (CardType[drv] < 4)
        sect *= 512;                        /* Convert LBA to byte address if needed */
    if (count == 1) {
        if (send_cmd(CMD17, sect) == 0) {   /* Read block */
            if (recv_datablock((BYTE*)buff, 512))
                count = 0;
        }
    } else {
#if ! MULTIPLE
        return RES_PARERR;
#else
        if (send_cmd(CMD18, sect) == 0) {   /* Read multiple blocks */
            while (count > 0 && recv_datablock((BYTE*)buff, 512)) {
                buff += 512;
                count -= 1;
            }
            (void) send_cmd12();            /* Stop transmission */
        }
#endif
    }
    deselect();
    return count ? RES_ERROR : RES_OK;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if !FF_FS_READONLY

DRESULT disk_write (BYTE drv,          /* Physical drive nmuber to identify the drive */
                    const BYTE *buff,  /* Data to be written */
                    LBA_t sector,      /* Start sector in LBA */
                    UINT count)        /* Number of sectors to write */
{
    register DWORD sect = (DWORD)sector;

    if (drv & 0xfe)
        return RES_PARERR;
    if (Stat[drv] & (STA_NOINIT|STA_NODISK))
        return RES_NOTRDY;
    Drive = drv;
    if (CardType[drv] < 4)
        sect *= 512;                      /* Convert LBA to byte address if needed */
    if (count == 1) {
        if (send_cmd(CMD24, sect) == 0)   /* Write single sector */
            if (xmit_datablock(buff, 0xFE))
                count = 0;
    } else {
#if ! MULTIPLE
        return RES_PARERR;
#else
        if (CardType[drv] >= 2)
            (void) send_cmd(ACMD23, count);      /* Announce erase count */
        if (send_cmd(CMD25, sect) == 0) {        /* Write multiple sector */
            while (count > 0 && xmit_datablock(buff, 0xfc)) {
                buff += 512;
                count -= 1;
            }
            if (! xmit_datablock(0, 0xfd))       /* Stop tran */
                count = -1;
        }
#endif
    }
    deselect();
    return count ? RES_ERROR : RES_OK;
}

#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (BYTE drv,       /* Physical drive nmuber (0..) */
                    BYTE cmd,       /* Control code */
                    void *buff)     /* Buffer to send/receive control data */
{
    if (drv & 0xfe)
        return RES_NOTRDY;
    if (Stat[drv] & (STA_NOINIT|STA_NODISK))
        return RES_NOTRDY;
    Drive = drv;

    switch(cmd)
        {
        case DISK_CHANGED: {
            BYTE ncid[16];
            register BYTE res = read_cid(ncid);
            deselect();
            if (res || memcmp(CID[drv], ncid, 16))
                return RES_ERROR;
            return RES_OK;
        }
        }
    return RES_PARERR;
}


/* Local Variables: */
/* mode: c */
/* c-basic-offset: 4 */
/* indent-tabs-mode: () */
/* End: */
