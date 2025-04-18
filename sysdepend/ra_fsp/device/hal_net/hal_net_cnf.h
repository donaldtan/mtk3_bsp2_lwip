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

#endif	/* _DEV_HAL_NET_CNF_H_ */