/*
 *----------------------------------------------------------------------
 *    micro T-Kernel 3.0 BSP 2.0
 *
 *    Copyright (C) 2023-2025 by Ken Sakamura.
 *    This software is distributed under the T-License 2.1.
 *----------------------------------------------------------------------
 *
 *    Released by TRON Forum(http://www.tron.org) at 2025/04.
 *
 *----------------------------------------------------------------------
 */

/*
 *	hal_net_cnf.h 
 *	Net device driver  (RA FSP)
 *		Device configuration file
 */
#ifndef	_DEV_HAL_NET_CNF_H_
#define	_DEV_HAL_NET_CNF_H_

#define DEVNAME_HAL_NET		"net"
#define DEV_HAL_NET_TMOUT	(2000)
#define DEV_HAL_RBUF_NUM	8

#define ETH_MAX_FRAME_LENGTH	(1514)


#define DEV_HAL_NET_UNITNM	(1)	// Number of Net units

/* 
 * This controller assumes that the maximum size of an Ethernet packet without
 * jumbo frame support can reach up to 1,536 bytes 
 */
#define ETHER_DRV_BUFF_SIZE		(1536U)

#define ETHER_DRV_BUFF_ALIGNMENT	(32U)

#define ETHER_DRV_MAX_RBUFF		(16U)

/* UTK lwIP options: */
/**
 * LWIP_NETDRV_NO_SETUP_RXBUF==1: Net device driver do not need RX buffer set.
 */
#define ETHER_DRV_NO_SETUP_RXBUF		1

/* Reserved space in the TX buffer. */
#define ETHER_DRV_TXBUF_RESERVED_SIZE		0

#endif	/* _DEV_HAL_NET_CNF_H_ */