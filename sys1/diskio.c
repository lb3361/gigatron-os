/*-----------------------------------------------------------------------*/
/* Low level disk I/O module for FatFs on the Gigatron                   */
/*-----------------------------------------------------------------------*/

#include "ff.h"			/* Obtains integer types */
#include "diskio.h"		/* Declarations of disk functions */
#include "spi.h"

#include <gigatron/sys.h>
#include <gigatron/libc.h>
#include <gigatron/console.h>

#define CMDVERBOSE 0
#define INITVERBOSE 0

/*-----------------------------------------------------------------------*/
/* Private                                                               */
/*-----------------------------------------------------------------------*/


#if FF_MAX_SS != 512 || FF_MIN_SS != 512
#error Sector size must be 512 bytes
#endif


/* MMC/SD command (SPI mode) */
#define CMD0	(0)			/* GO_IDLE_STATE */
#define CMD1	(1)			/* SEND_OP_COND */
#define	ACMD41	(0x80+41)	/* SEND_OP_COND (SDC) */
#define CMD8	(8)			/* SEND_IF_COND */
#define CMD9	(9)			/* SEND_CSD */
#define CMD10	(10)		/* SEND_CID */
#define CMD12	(12)		/* STOP_TRANSMISSION */
#define CMD13	(13)		/* SEND_STATUS */
#define ACMD13	(0x80+13)	/* SD_STATUS (SDC) */
#define CMD16	(16)		/* SET_BLOCKLEN */
#define CMD17	(17)		/* READ_SINGLE_BLOCK */
#define CMD18	(18)		/* READ_MULTIPLE_BLOCK */
#define CMD23	(23)		/* SET_BLOCK_COUNT */
#define	ACMD23	(0x80+23)	/* SET_WR_BLK_ERASE_COUNT (SDC) */
#define CMD24	(24)		/* WRITE_BLOCK */
#define CMD25	(25)		/* WRITE_MULTIPLE_BLOCK */
#define CMD32	(32)		/* ERASE_ER_BLK_START */
#define CMD33	(33)		/* ERASE_ER_BLK_END */
#define CMD38	(38)		/* ERASE */
#define CMD55	(55)		/* APP_CMD */
#define CMD58	(58)		/* READ_OCR */

static DSTATUS Stat = STA_NOINIT;	/* Disk status */
static BYTE Drive = 0;

static BYTE CardType;   /* b0:MMC, b1:SDv1, b2:SDv2, b3:Block addressing */

static char cbuf[16];

static int wait_ready (void)	/* 1:OK, 0:Timeout */
{
	BYTE d;
	UINT tmr;
	for (tmr = 5000; tmr; tmr--) {	/* Wait for ready in timeout of 500ms */
		spi_recv(&d, 1);
		if (d == 0xFF)
			break;
	}
	return tmr ? 1 : 0;
}

static void deselect (void)
{
	BYTE ctrl = ctrlBits_v5 | 0xc;
	SYS_ExpanderControl(ctrl);
}

static int select (void)	/* 1:OK, 0:Timeout */
{
	BYTE d;
	BYTE ctrl = ctrlBits_v5 | 0xc;
	if (Drive == 0)
		ctrl ^= 4;
	else if (Drive == 1)
		ctrl ^= 8;
	SYS_ExpanderControl(ctrl);
	spi_recv(&d, 1);
	if (wait_ready())
		return 1;	    /* Wait for card ready */
	SYS_ExpanderControl(ctrl);
	return 0;			/* Failed */
}

static
int recv_datablock(BYTE *buff, 	UINT btr)
{
	BYTE d[2];
	UINT tmr;
	for (tmr = 1000; tmr; tmr--) {	/* Wait for data packet in timeout of 100ms */
		spi_recv(d, 1);
		if (d[0] != 0xFF)
			break;
	}
	if (d[0] != 0xFE) 		/* If not valid data token, return with error */
		return 0;
	spi_recv(buff, btr);		/* Receive the data block into buffer */
	spi_recv(d, 2);				/* Discard CRC */
	return 1;				/* Return with success */
}

#if ! FF_READONLY

static
int xmit_datablock(BYTE *buff, BYTE token)
{
	BYTE d[2];
	if (!wait_ready())
		return 0;
	d[0] = token;
	spi_send(d, 1);				/* Xmit a token */
	if (token != 0xFD) {	/* Is it data token? */
		spi_send(buff, 512);	/* Xmit the 512 byte data block to MMC */
		spi_recv(d, 2);			/* Xmit dummy CRC (0xFF,0xFF) */
		spi_recv(d, 1);			/* Receive data response */
		if ((d[0] & 0x1F) != 0x05)	/* If not accepted, return with error */
			return 0;
	}
	return 1;
}

#endif

static BYTE send_cmd(BYTE c, DWORD arg)
{
	BYTE n, d, buf[6];
	
	if (c & 0x80) {	/* ACMD<n> is the command sequense of CMD55-CMD<n> */
		c &= 0x7F;
		n = send_cmd(CMD55, 0);
		if (n > 1)
			return n;
	}
	/* Select the card and wait for ready except to stop multiple block read */
	if (c != CMD12) {
		deselect();
		if (!select())
			return 0xFF;
	}
#if CMDVERBOSE
	cprintf("CMD %d: ", c);
#endif
	/* Send a command packet */
	buf[0] = 0x40 | c;				/* Start + Command index */
	buf[1] = (BYTE)(arg >> 24);		/* Argument[31..24] */
	buf[2] = (BYTE)(arg >> 16);		/* Argument[23..16] */
	buf[3] = (BYTE)(arg >> 8);		/* Argument[15..8] */
	buf[4] = (BYTE)arg;				/* Argument[7..0] */
	n = 0x01;						/* Dummy CRC + Stop */
	if (c == CMD0) n = 0x95;		/* (valid CRC for CMD0(0)) */
	if (c == CMD8) n = 0x87;		/* (valid CRC for CMD8(0x1AA)) */
	buf[5] = n;
	spi_send(buf, 6);
	/* Receive command response */
	if (c == CMD12)		/* Skip a stuff byte when stop reading */
		spi_recv(&d, 1);
	n = 10;				/* Wait for a valid response in timeout of 10 attempts */
	do {
		spi_recv(&d, 1);
	} while ((d & 0x80) && --n);
#if CMDVERBOSE
	if (c == CMD0 || c == CMD8 || c == CMD55) { n = d & 0xfe; } else { n = d; }
	cprintf("%d %s\n", d, (n) ? "FAIL" : "OK");
#endif
	return d;			/* Return with the response value */
}



/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (BYTE drv)
{
	if (drv != Drive)
		return RES_NOTRDY;
	return Stat;
}


/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (BYTE drv)
{
	BYTE n, ty, c, buf[4];
	UINT tmr;
	DSTATUS s;

	if (drv != 0 && drv != 1)
		return RES_NOTRDY;
	Drive = drv;
	
	for (n = 10; n; n--)
		spi_recv(buf, 1);	/* Apply 80 dummy clocks and the card gets ready to receive command */

	ty = 0;
	if (send_cmd(CMD0, 0) == 1) {			/* Enter Idle state */

		if (send_cmd(CMD8, 0x1AA) == 1) {	/* SDv2? */
			spi_recv(buf, 4);								/* Get trailing return value of R7 resp */
			if (buf[2] == 0x01 && buf[3] == 0xAA) {		/* The card can work at vdd range of 2.7-3.6V */
				for (tmr = 2000; tmr; tmr--) 			/* Wait for leaving idle state (ACMD41 with HCS bit) */
					if (send_cmd(ACMD41, 1UL << 30) == 0)
						break;
				if (tmr && send_cmd(CMD58, 0) == 0) {	/* Check CCS bit in the OCR */
					spi_recv(buf, 4);
					ty = (buf[0] & 0x40) ? 4 : 2;
				}
			}
		} else {							/* SDv1 or MMCv3 */
			if (send_cmd(ACMD41, 0) <= 1) {
				ty = 2; c = ACMD41;	/* SDv1 */
			} else {
				ty = 1; c = CMD1;	/* MMCv3 */
			}
			for (tmr = 2000; tmr; tmr--) 		/* Wait for leaving idle state */
				if (send_cmd(c, 0) == 0)
					break;
			if (!tmr || send_cmd(CMD16, 512) != 0)	/* Set R/W block length to 512 */
				ty = 0;
		}
	}
	CardType = ty;
#if INITVERBOSE
	cprintf("CardType=%d\n", ty);
#endif
	s = ty ? 0 : STA_NOINIT;
	Stat = s;
	deselect();

	return s;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (BYTE drv,       /* Physical drive nmuber to identify the drive */
				   BYTE *buff,      /* Data buffer to store read data */
				   LBA_t sector,    /* Start sector in LBA */
				   UINT count)      /* Number of sectors to read */
{
	BYTE c;
	DWORD sect = (DWORD)sector;
	
	if (disk_status(drv) & STA_NOINIT)
		return RES_NOTRDY;
	if (CardType < 4)
		sect *= 512;	/* Convert LBA to byte address if needed */
	c = (count > 1) ? CMD18 : CMD17;	/*  READ_MULTIPLE_BLOCK : READ_SINGLE_BLOCK */
	if (send_cmd(c, sect) == 0) {
		do {
			if (! recv_datablock((BYTE*)buff, 512))
				break;
			buff += 512;
		} while (--count);
		if (c == CMD18)
			(void)send_cmd(CMD12, 0);	/* STOP_TRANSMISSION */
	}
	deselect();
	return count ? RES_ERROR : RES_OK;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if !FF_FS_READONLY

DRESULT disk_write (BYTE drv,          /* Physical drive nmuber to identify the drive */
					const BYTE *buff,   /* Data to be written */
					LBA_t sector,       /* Start sector in LBA */
					UINT count)         /* Number of sectors to write */
{
	DRESULT res = RES_PARERR;
	return res;
}

#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

#if !FF_FS_READONLY

DRESULT disk_ioctl (BYTE pdrv,      /* Physical drive nmuber (0..) */
					BYTE cmd,       /* Control code */
					void *buff)	    /* Buffer to send/receive control data */
{
	return RES_PARERR;
}

#endif

/* Local Variables: */
/* mode: c */
/* tab-width: 4 */
/* c-basic-offset: 4 */
/* indent-tabs-mode: t */
/* End: */
