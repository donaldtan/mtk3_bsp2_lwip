/*
 *----------------------------------------------------------------------
 *    micro T-Kernel 3.0 BSP 2.0
 *
 *    Copyright (C) 2025 by Ken Sakamura.
 *    This software is distributed under the T-License 2.1.
 *----------------------------------------------------------------------
 *
 *    Released by TRON Forum(http://www.tron.org) at 2025/05.
 *
 *----------------------------------------------------------------------
 */

#ifndef _DEV_NET_PHY_H_
#define _DEV_NET_PHY_H_

/*
 *	phy.h
 *
 *	PHY chips definition (STM32N6570-DK)
 */

#define PHY_ADDRESS	(0U)

#define LAN8742_BCR      ((uint16_t)0x0000U)
#define LAN8742_BSR      ((uint16_t)0x0001U)
#define LAN8742_PHYI1R   ((uint16_t)0x0002U)
#define LAN8742_PHYI2R   ((uint16_t)0x0003U)
#define LAN8742_ANAR     ((uint16_t)0x0004U)
#define LAN8742_ANLPAR   ((uint16_t)0x0005U)
#define LAN8742_ANER     ((uint16_t)0x0006U)
#define LAN8742_ANNPTR   ((uint16_t)0x0007U)
#define LAN8742_ANNPRR   ((uint16_t)0x0008U)
#define LAN8742_MMDACR   ((uint16_t)0x000DU)
#define LAN8742_MMDAADR  ((uint16_t)0x000EU)
#define LAN8742_ENCTR    ((uint16_t)0x0010U)
#define LAN8742_MCSR     ((uint16_t)0x0011U)
#define LAN8742_SMR      ((uint16_t)0x0012U)
#define LAN8742_TPDCR    ((uint16_t)0x0018U)
#define LAN8742_TCSR     ((uint16_t)0x0019U)
#define LAN8742_SECR     ((uint16_t)0x001AU)
#define LAN8742_SCSIR    ((uint16_t)0x001BU)
#define LAN8742_CLR      ((uint16_t)0x001CU)
#define LAN8742_ISFR     ((uint16_t)0x001DU)
#define LAN8742_IMR      ((uint16_t)0x001EU)
#define LAN8742_PHYSCSR  ((uint16_t)0x001FU)


#define LAN8742_BSR_100BASE_T4       ((uint16_t)0x8000U)
#define LAN8742_BSR_100BASE_TX_FD    ((uint16_t)0x4000U)
#define LAN8742_BSR_100BASE_TX_HD    ((uint16_t)0x2000U)
#define LAN8742_BSR_10BASE_T_FD      ((uint16_t)0x1000U)
#define LAN8742_BSR_10BASE_T_HD      ((uint16_t)0x0800U)
#define LAN8742_BSR_100BASE_T2_FD    ((uint16_t)0x0400U)
#define LAN8742_BSR_100BASE_T2_HD    ((uint16_t)0x0200U)
#define LAN8742_BSR_EXTENDED_STATUS  ((uint16_t)0x0100U)
#define LAN8742_BSR_AUTONEGO_CPLT    ((uint16_t)0x0020U)
#define LAN8742_BSR_REMOTE_FAULT     ((uint16_t)0x0010U)
#define LAN8742_BSR_AUTONEGO_ABILITY ((uint16_t)0x0008U)
#define LAN8742_BSR_LINK_STATUS      ((uint16_t)0x0004U)
#define LAN8742_BSR_JABBER_DETECT    ((uint16_t)0x0002U)
#define LAN8742_BSR_EXTENDED_CAP     ((uint16_t)0x0001U)

#define LAN8742_BCR_SOFT_RESET         ((uint16_t)0x8000U)
#define LAN8742_BCR_LOOPBACK           ((uint16_t)0x4000U)
#define LAN8742_BCR_SPEED_SELECT       ((uint16_t)0x2000U)
#define LAN8742_BCR_AUTONEGO_EN        ((uint16_t)0x1000U)
#define LAN8742_BCR_POWER_DOWN         ((uint16_t)0x0800U)
#define LAN8742_BCR_ISOLATE            ((uint16_t)0x0400U)
#define LAN8742_BCR_RESTART_AUTONEGO   ((uint16_t)0x0200U)
#define LAN8742_BCR_DUPLEX_MODE        ((uint16_t)0x0100U)

#define LAN8742_PHYSCSR_AUTONEGO_DONE   ((uint16_t)0x1000U)
#define LAN8742_PHYSCSR_HCDSPEEDMASK    ((uint16_t)0x001CU)
#define LAN8742_PHYSCSR_10BT_HD         ((uint16_t)0x0004U)
#define LAN8742_PHYSCSR_10BT_FD         ((uint16_t)0x0014U)
#define LAN8742_PHYSCSR_100BTX_HD       ((uint16_t)0x0008U)
#define LAN8742_PHYSCSR_100BTX_FD       ((uint16_t)0x0018U) 

BOOL PHY_GetLinkStatus(ETH_HandleTypeDef *heth)
{
	uint32_t reg;
	HAL_StatusTypeDef sts;
	uint32_t duplex, speed = 0;
	ETH_MACConfigTypeDef MACConf = {0};
	
	sts = HAL_ETH_ReadPHYRegister(heth, PHY_ADDRESS, LAN8742_BSR, &reg);
	if( (sts == HAL_OK) && ((reg & LAN8742_BSR_LINK_STATUS) != 0) ) {
		/* Check Auto negotiation */
		sts = HAL_ETH_ReadPHYRegister(heth, PHY_ADDRESS, LAN8742_BCR, &reg);
		if( sts == HAL_OK ) {
			if( (reg & LAN8742_BCR_AUTONEGO_EN) != LAN8742_BCR_AUTONEGO_EN ) {
				if( (reg & LAN8742_BCR_SPEED_SELECT) == LAN8742_BCR_SPEED_SELECT ) {
					speed = ETH_SPEED_100M;
				}
				else {
					speed = ETH_SPEED_10M;
				}
				
				if( (reg & LAN8742_BCR_DUPLEX_MODE) == LAN8742_BCR_DUPLEX_MODE ) {
					duplex = ETH_FULLDUPLEX_MODE;
				}
				else {
					duplex = ETH_HALFDUPLEX_MODE;
				}
			}
			else {	/* Auto Nego enabled */
				if( HAL_ETH_ReadPHYRegister(heth, PHY_ADDRESS, LAN8742_PHYSCSR, &reg) != HAL_OK || ((reg & LAN8742_PHYSCSR_AUTONEGO_DONE) == 0) ) {
					return FALSE;
				}

				if((reg & LAN8742_PHYSCSR_HCDSPEEDMASK) == LAN8742_PHYSCSR_100BTX_FD) {
					duplex = ETH_FULLDUPLEX_MODE;
					speed = ETH_SPEED_100M;
				}
				else if( (reg & LAN8742_PHYSCSR_HCDSPEEDMASK) == LAN8742_PHYSCSR_100BTX_HD ) {
					duplex = ETH_HALFDUPLEX_MODE;
					speed = ETH_SPEED_100M;
				}
				else if( (reg & LAN8742_PHYSCSR_HCDSPEEDMASK) == LAN8742_PHYSCSR_10BT_FD ) {
					duplex = ETH_FULLDUPLEX_MODE;
					speed = ETH_SPEED_10M;
				}
				else
				{
					duplex = ETH_HALFDUPLEX_MODE;
					speed = ETH_SPEED_10M;
				}
			}
			
			/* Get MAC Config MAC */
			HAL_ETH_GetMACConfig(heth, &MACConf);
			MACConf.DuplexMode = duplex;
			MACConf.Speed = speed;
			HAL_ETH_SetMACConfig(heth, &MACConf);
			
			return TRUE;
		}
	}
	
	return FALSE;
}

void PHY_Init(ETH_HandleTypeDef *heth)
{
	uint32_t reg;
	
	/* Initialize MDIO Clock. */
	HAL_ETH_SetMDIOClockRange(heth);
	
	/* Start autonegotiation. */
	if( HAL_ETH_ReadPHYRegister(heth, PHY_ADDRESS, LAN8742_BCR, &reg) >= 0 ) {
		reg |= LAN8742_BCR_AUTONEGO_EN;
		HAL_ETH_WritePHYRegister(heth, PHY_ADDRESS, LAN8742_BCR, reg);
	}
}

#endif /* _DEV_NET_PHY_H_ */