/*
 *----------------------------------------------------------------------
 *    micro T-Kernel 3.0 BSP 2.0
 *
 *    Copyright (C) 2023-2025 by Ken Sakamura.
 *    This software is distributed under the T-License 2.1.
 *----------------------------------------------------------------------
 *
 *    Released by TRON Forum(http://www.tron.org) at 2025/05.
 *
 *----------------------------------------------------------------------
 */
#include <sys/machine.h>
#include <config_bsp/stm32_cube/config_bsp.h>

#ifdef MTKBSP_STM32CUBE
#if DEVCNF_USE_HAL_NET

#include <string.h>

#include <tk/tkernel.h>
#include <tk/device.h>
#include <sys/queue.h>

#include <sysdepend/stm32_cube/cpu_status.h>
#include <mtkernel/kernel/knlinc/tstdlib.h>
#include <mtkernel/device/common/drvif/msdrvif.h>
#include "hal_net_cnf.h"

#include <tm/tmonitor.h>

LOCAL __attribute__((__aligned__(ETHER_DRV_BUFF_ALIGNMENT)))uint8_t ether_rx_buffers[DEV_HAL_RBUF_NUM][1536];

#define ETHER_FLGPTN_TX_COMPLETE	(1U << 0)
#define ETHER_FLGPTN_TX_ABORTED		(1U << 1)


/*
 *	hal_net.c
 *	Net device driver (RA FSP)
*/

/*---------------------------------------------------------------------*/
/*Net Device driver Control block
 */
typedef struct {
	ID			devid;		// Device ID
	UINT			omode;		// Open mode
	UW			unit;		// Unit no
	BOOL			initialized;	// Is device initialized.
	ID			evtmbfid;	// MBF ID for event notification
	ID			rxmbfid;	// MBF ID for RX event notification
	QUEUE 			freerxbufq;	// Free RX buffer Queue
	ID			flgid;		// Event flag ID
	BOOL			linkstatus;	// Link status	
} T_HAL_NET_DCB;

/* Interrupt detection flag */
LOCAL const T_CFLG	id_flg	= {
			.flgatr		= TA_TFIFO | TA_WMUL,
			.iflgptn	= 0,
};

#if TK_SUPPORT_MEMLIB
LOCAL T_HAL_NET_DCB	*dev_net_cb[DEV_HAL_NET_UNITNM] = {0};
#define		get_dcb_ptr(unit)	(dev_net_cb[unit])
#else
LOCAL T_HAL_NET_DCB	dev_net_cb[DEV_HAL_NET_UNITNM] = {0};
#define		get_dcb_ptr(unit)	(&dev_net_cb[unit])
#endif

#define netdrv_check_param(req, type)	(((req)->size < (W) sizeof(type)) ? E_PAR : E_OK)

IMPORT ETH_HandleTypeDef heth1;
IMPORT ETH_TxPacketConfig TxConfig;

/*---------------------------------------------------------------------*/
/* Attribute data control
 */
LOCAL ER read_atr(T_HAL_NET_DCB *p_dcb, T_DEVREQ *req)
{
	ER ercd;
	
	switch(req->start) {
	case DN_NETEVENT:
		ercd = netdrv_check_param( req, ID );
		if( ercd == E_OK ) {
			*((ID *)req->buf) = p_dcb->rxmbfid;
		}
		break;
	case DN_NETADDR:
		ercd = netdrv_check_param( req, NetAddr );
		if( ercd == E_OK ) {
			memcpy(req->buf, heth1.Init.MACAddr, sizeof(NetAddr));
		}
		break;
	case DN_NETRXBUFSZ:
	case DN_NETDEVINFO:
	case DN_NETRESET:
	case DN_NETSTINFO:
	case DN_NETCSTINFO:
	case DN_NETWLANCONFIG:
	case DN_NETWLANSTINFO:
	case DN_NETWLANCSTINFO:
		return E_NOSPT;
	default:
		return E_PAR;
	}
	return ercd;
}

LOCAL ER write_atr(T_HAL_NET_DCB *p_dcb, T_DEVREQ *req)
{
	ER ercd;
	
	switch( req->start ) {
	case DN_NETEVENT:
		ercd = netdrv_check_param( req, ID );
		if( ercd == E_OK ) {
			p_dcb->rxmbfid = *((ID *)req->buf);
		}
		break;

	case DN_NETRXBUF:
		ercd = netdrv_check_param( req, void* );
		if( ercd == E_OK ) {
			/* Disable the ether interrupt to achieve synchronization. */
			DisableInt((UINT) ETH1_IRQn);
			QueInsert(*((QUEUE**)req->buf), &p_dcb->freerxbufq);
			EnableInt((UINT) ETH1_IRQn, (INT) 10);
		}
		break;
	case DN_NETRXBUFSZ:
		ercd = netdrv_check_param( req, NetRxBufSz );
		/* Can't set RX buffer size to r_ether module. Always return E_OK. */
		return E_OK;
	case DN_NETRESET:
	case DN_SET_MCAST_LIST:
	case DN_SET_ALL_MCAST:
	case DN_NETWLANCONFIG:
		/* NOT SUPPORTED */
		return E_NOSPT;
	default:
		
		return E_PAR;
	}
	
	return ercd;
}

/*---------------------------------------------------------------------*/
/* Device-specific data control
 */
void HAL_ETH_TxCpltCallback(ETH_HandleTypeDef *heth)
{
	T_HAL_NET_DCB *p_dcb = get_dcb_ptr(0);
	
	tk_set_flg(p_dcb->flgid, ETHER_FLGPTN_TX_COMPLETE);
}

void HAL_ETH_RxCpltCallback(ETH_HandleTypeDef *heth)
{
	void *p;
	
	HAL_ETH_ReadData(&heth1, &p);
	
	UNUSED(p);
}

void HAL_ETH_ErrorCallback(ETH_HandleTypeDef *heth)
{
	UNUSED(heth);
}

void HAL_ETH_RxAllocateCallback(uint8_t **buff)
{
	T_HAL_NET_DCB 	*p_dcb = get_dcb_ptr(0);
	
	*buff = (void *) QueRemoveNext( &p_dcb->freerxbufq );
}

void HAL_ETH_RxLinkCallback(void **pStart, void **pEnd, uint8_t *buff, uint16_t Length)
{
	T_HAL_NET_DCB 	*p_dcb = get_dcb_ptr(0);
	NetEvent	event;
	ER		ercd;
	
	event.buf = buff;
	event.len = (UH) Length;
	ercd = tk_snd_mbf( p_dcb->rxmbfid, &event, sizeof( NetEvent ), TMO_POL );
	if( ercd < E_OK ) {
		/* Reinsert buffer to free RX buffer queue. */
		QueInsert((QUEUE*)buff, &p_dcb->freerxbufq);
	}
	
}

LOCAL ER read_data(T_HAL_NET_DCB *p_dcb, T_DEVREQ *req)
{
	UNUSED(p_dcb);
	UNUSED(req);
	return E_NOSPT;
}

LOCAL ER write_data(T_HAL_NET_DCB *p_dcb, T_DEVREQ *req)
{
	ETH_BufferTypeDef	Txbuffer;
	HAL_StatusTypeDef	sts;
	UINT			flgptn;
	ER			er;
	
	if( req->size < 0 ) {
		return E_PAR;
	}
	if( req->size == 0 ) {
		return ETH_MAX_FRAME_LENGTH;
	}
	else {
		if( p_dcb->linkstatus != TRUE ) {
			/* Check link status */
			//
			if( p_dcb->linkstatus != TRUE ) {
				return E_NOMDA;
			}
		}
		
		Txbuffer.buffer = req->buf;
		Txbuffer.len = req->size;
		Txbuffer.next = NULL;
		
		TxConfig.Length = req->size;
		TxConfig.TxBuffer = &Txbuffer;
		TxConfig.pData = req->buf;
		
		sts = HAL_ETH_Transmit_IT(&heth1, &TxConfig);
		if( sts != HAL_OK ) {
			if(HAL_ETH_GetError(&heth1) & HAL_ETH_ERROR_BUSY)
			{
				HAL_ETH_ReleaseTxPacket(&heth1);
			}
			return E_IO;
		}
		
		er = tk_wai_flg(p_dcb->flgid, 
				ETHER_FLGPTN_TX_COMPLETE | ETHER_FLGPTN_TX_ABORTED, 
				TWF_ORW | TWF_BITCLR, 
				&flgptn, 
				DEV_HAL_NET_TMOUT);
		if( er < E_OK ) {
			/* Check link status */
			//
			return er;
		}
		else if( (flgptn & ETHER_FLGPTN_TX_ABORTED) != 0 ) {
			return E_IO;
		}
		return E_OK;
	}
}

/*----------------------------------------------------------------------
 * mSDI I/F function
 */
/*
 * Open device
 */
LOCAL ER dev_net_openfn( ID devid, UINT omode, T_MSDI *p_msdi)
{
	UNUSED(devid);
	UNUSED(omode);
	UNUSED(p_msdi);
	/* Do Nothing. */
	return E_OK;
}

/*
 * Close Device
 */
LOCAL ER dev_net_closefn( ID devid, UINT option, T_MSDI *p_msdi)
{
	UNUSED(devid);
	UNUSED(option);
	UNUSED(p_msdi);
	/* Do Nothing. */
	return E_OK;
}

/*
 * Read Device
 */
LOCAL ER dev_net_readfn( T_DEVREQ *req, T_MSDI *p_msdi)
{
	T_HAL_NET_DCB	*p_dcb;
	ER		err;

	p_dcb = (T_HAL_NET_DCB*)(p_msdi->dmsdi.exinf);

	if(req->start >= 0) {
		err = read_data( p_dcb, req);	// Device specific data
	} else {
		err = read_atr( p_dcb, req);	// Device attribute data
	}
	return err;
}

/*
 * Write Device
 */
LOCAL ER dev_net_writefn( T_DEVREQ *req, T_MSDI *p_msdi)
{
	T_HAL_NET_DCB	*p_dcb;
	ER		err;

	p_dcb = (T_HAL_NET_DCB*)(p_msdi->dmsdi.exinf);

	if(req->start >= 0) {
		err = write_data( p_dcb, req);	// Device specific data
	} else {
		err = write_atr( p_dcb, req);	// Device attribute data
	}
	return err;
}

/*
 * Event Device
 */
LOCAL ER dev_net_eventfn( INT evttyp, void *evtinf, T_MSDI *p_msdi)
{
	UNUSED(evttyp);
	UNUSED(evtinf);
	UNUSED(p_msdi);
	/* Do Nothing. */
	return E_NOSPT;
}

/*----------------------------------------------------------------------
 * Device driver initialization and registration
 */
EXPORT ER dev_init_hal_net( UW unit )
{
	T_HAL_NET_DCB	*p_dcb;
	T_IDEV		idev;
	T_MSDI		*p_msdi;
	T_DMSDI		dmsdi;
	ER		err;
	INT		i;

	if( unit >= DEV_HAL_NET_UNITNM) return E_PAR;

#if TK_SUPPORT_MEMLIB
	p_dcb = (T_HAL_NET_DCB*)Kmalloc(sizeof(T_HAL_NET_DCB));
	if( p_dcb == NULL) return E_NOMEM;
	dev_net_cb[unit]	= p_dcb;
#else
	p_dcb = &dev_net_cb[unit];
#endif

	p_dcb->flgid = tk_cre_flg(&id_flg);
	if(p_dcb->flgid <= E_OK) {
		err = (ER)p_dcb->flgid;
		goto err_1;
	}

	/* Device registration information */
	dmsdi.exinf	= p_dcb;
	dmsdi.drvatr	= 0;			/* Driver attributes */
	dmsdi.devatr	= TDK_UNDEF;		/* Device attributes */
	dmsdi.nsub	= 0;			/* Number of sub units */
	dmsdi.blksz	= 1;			/* Unique data block size (-1 = unknown) */
	dmsdi.openfn	= dev_net_openfn;
	dmsdi.closefn	= dev_net_closefn;
	dmsdi.readfn	= dev_net_readfn;
	dmsdi.writefn	= dev_net_writefn;
	dmsdi.eventfn	= dev_net_eventfn;
	
	knl_strcpy( (char*)dmsdi.devnm, DEVNAME_HAL_NET);
	i = knl_strlen(DEVNAME_HAL_NET);
	dmsdi.devnm[i] = (UB)('a' + unit);
	dmsdi.devnm[i+1] = 0;

	err = msdi_def_dev( &dmsdi, &idev, &p_msdi);
	if(err != E_OK) goto err_1;

	p_dcb->devid	= p_msdi->devid;
	p_dcb->unit	= unit;
	p_dcb->evtmbfid	= idev.evtmbfid;
	p_dcb->rxmbfid	= -1;
	p_dcb->linkstatus = FALSE;
	p_dcb->initialized = FALSE;
	
	/* Initialize the RX buffer. */
	QueInit( &p_dcb->freerxbufq );
	for( i = 0; i < DEV_HAL_RBUF_NUM; i++ ) {
		QueInsert((QUEUE*)ether_rx_buffers[i], &p_dcb->freerxbufq);
	}
	
	HAL_ETH_Start_IT(&heth1);
	return E_OK;

err_1:
#if TK_SUPPORT_MEMLIB
	Kfree(p_dcb);
#endif
	return err;
}

IMPORT ER hal_net_get_link_status( UW unit )
{
	T_HAL_NET_DCB	*p_dcb = get_dcb_ptr(unit);
	if( p_dcb == NULL || p_dcb->initialized == FALSE ) {
		return E_CTX;
	}
	else {
		return p_dcb->linkstatus ? E_OK : E_NOMDA;
	}
}
	

#endif		/* DEVCNF_USE_HAL_NET */
#endif		/* MTKBSP_STM32CUBE */