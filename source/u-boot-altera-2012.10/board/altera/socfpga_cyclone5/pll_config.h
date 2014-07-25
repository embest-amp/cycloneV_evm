
#ifndef _PRELOADER_PLL_CONFIG_H_
#define _PRELOADER_PLL_CONFIG_H_

/* PLL configuration data */
/* Main PLL */
#define CONFIG_HPS_MAINPLLGRP_VCO_DENOM			(0)
#define CONFIG_HPS_MAINPLLGRP_VCO_NUMER			(63)
#define CONFIG_HPS_MAINPLLGRP_MPUCLK_CNT		(0)
#define CONFIG_HPS_MAINPLLGRP_MAINCLK_CNT		(0)
#define CONFIG_HPS_MAINPLLGRP_DBGATCLK_CNT		(0)
#define CONFIG_HPS_MAINPLLGRP_MAINQSPICLK_CNT		(3)
#define CONFIG_HPS_MAINPLLGRP_MAINNANDSDMMCCLK_CNT	(3)
#define CONFIG_HPS_MAINPLLGRP_CFGS2FUSER0CLK_CNT	(15)
#define CONFIG_HPS_MAINPLLGRP_MAINDIV_L3MPCLK		(1)
#define CONFIG_HPS_MAINPLLGRP_MAINDIV_L3SPCLK		(1)
#define CONFIG_HPS_MAINPLLGRP_MAINDIV_L4MPCLK		(1)
#define CONFIG_HPS_MAINPLLGRP_MAINDIV_L4SPCLK		(1)
#define CONFIG_HPS_MAINPLLGRP_DBGDIV_DBGATCLK		(0)
#define CONFIG_HPS_MAINPLLGRP_DBGDIV_DBGCLK		(1)
#define CONFIG_HPS_MAINPLLGRP_TRACEDIV_TRACECLK		(0)
/*
 * To tell where is the clock source:
 * 0 = MAINPLL
 * 1 = PERIPHPLL
 */
#define CONFIG_HPS_MAINPLLGRP_L4SRC_L4MP		(1)
#define CONFIG_HPS_MAINPLLGRP_L4SRC_L4SP		(1)

/* Peripheral PLL */
#define CONFIG_HPS_PERPLLGRP_VCO_DENOM			(1)
#define CONFIG_HPS_PERPLLGRP_VCO_NUMER			(79)
/*
 * To tell where is the VCOs source:
 * 0 = EOSC1
 * 1 = EOSC2
 * 2 = F2S
 */
#define CONFIG_HPS_PERPLLGRP_VCO_PSRC			(0)
#define CONFIG_HPS_PERPLLGRP_EMAC0CLK_CNT		(3)
#define CONFIG_HPS_PERPLLGRP_EMAC1CLK_CNT		(3)
#define CONFIG_HPS_PERPLLGRP_PERQSPICLK_CNT		(1)
/*
***************************************************************************
*                       Embest Tech co., ltd
*                        www.embest-tech.com
***************************************************************************
*there is a fixed 4 divisor for sd card, 1GHz/(per_nand_sdmmc_cnt+1)
*if CONFIG_HPS_PERPLLGRP_PERNANDSDMMCCLK_CNT is 19, then sd card clock is 50MHz/4 = 12.5MHz
*we must promote the frequency to 200MHz, then we can  get the 200MHz/4MHz = 50MHz
*/
#define CONFIG_HPS_PERPLLGRP_PERNANDSDMMCCLK_CNT	(4)
#define CONFIG_HPS_PERPLLGRP_PERBASECLK_CNT		(4)
#define CONFIG_HPS_PERPLLGRP_S2FUSER1CLK_CNT		(9)
#define CONFIG_HPS_PERPLLGRP_DIV_USBCLK			(0)
#define CONFIG_HPS_PERPLLGRP_DIV_SPIMCLK		(0)
#define CONFIG_HPS_PERPLLGRP_DIV_CAN0CLK		(1)
#define CONFIG_HPS_PERPLLGRP_DIV_CAN1CLK		(1)
#define CONFIG_HPS_PERPLLGRP_GPIODIV_GPIODBCLK		(6249)
/*
 * To tell where is the clock source:
 * 0 = F2S_PERIPH_REF_CLK
 * 1 = MAIN_CLK
 * 2 = PERIPH_CLK
 */
#define CONFIG_HPS_PERPLLGRP_SRC_SDMMC			(2)
#define CONFIG_HPS_PERPLLGRP_SRC_NAND			(2)
#define CONFIG_HPS_PERPLLGRP_SRC_QSPI			(1)

/* SDRAM PLL */
#define CONFIG_HPS_SDRPLLGRP_VCO_DENOM			(1)
#define CONFIG_HPS_SDRPLLGRP_VCO_NUMER			(63)
/*
 * To tell where is the VCOs source:
 * 0 = EOSC1
 * 1 = EOSC2
 * 2 = F2S
 */
#define CONFIG_HPS_SDRPLLGRP_VCO_SSRC			(0)
#define CONFIG_HPS_SDRPLLGRP_DDRDQSCLK_CNT		(1)
#define CONFIG_HPS_SDRPLLGRP_DDRDQSCLK_PHASE		(0)
#define CONFIG_HPS_SDRPLLGRP_DDR2XDQSCLK_CNT		(0)
#define CONFIG_HPS_SDRPLLGRP_DDR2XDQSCLK_PHASE		(0)
#define CONFIG_HPS_SDRPLLGRP_DDRDQCLK_CNT		(1)
#define CONFIG_HPS_SDRPLLGRP_DDRDQCLK_PHASE		(4)
#define CONFIG_HPS_SDRPLLGRP_S2FUSER2CLK_CNT		(5)
#define CONFIG_HPS_SDRPLLGRP_S2FUSER2CLK_PHASE		(0)

/* Info for driver */
#define CONFIG_HPS_CLK_OSC1_HZ			(25000000)
#define CONFIG_HPS_CLK_MAINVCO_HZ		(1600000000)
#define CONFIG_HPS_CLK_PERVCO_HZ		(1000000000)
#define CONFIG_HPS_CLK_SDRVCO_HZ		(600000000)
#define CONFIG_HPS_CLK_EMAC0_HZ			(50000000)
#define CONFIG_HPS_CLK_EMAC1_HZ			(50000000)
#define CONFIG_HPS_CLK_USBCLK_HZ		(200000000)
#define CONFIG_HPS_CLK_NAND_HZ			(100000000)
/*
***************************************************************************
*                       Embest Tech co., ltd
*                        www.embest-tech.com
***************************************************************************
*there is a fixed 4 divisor for sd card, 1GHz/(per_nand_sdmmc_cnt+1)
*if CONFIG_HPS_PERPLLGRP_PERNANDSDMMCCLK_CNT is 19, then sd card clock is 50MHz/4 = 12.5MHz
*we must promote the frequency to 200MHz, then we can  get the 200MHz/4MHz = 50MHz
*/
#define CONFIG_HPS_CLK_SDMMC_HZ			(200000000)
#define CONFIG_HPS_CLK_QSPI_HZ			(400000000)
#define CONFIG_HPS_CLK_SPIM_HZ			(200000000)
#define CONFIG_HPS_CLK_CAN0_HZ			(100000000)
#define CONFIG_HPS_CLK_CAN1_HZ			(100000000)
#define CONFIG_HPS_CLK_GPIODB_HZ		(32000)
#define CONFIG_HPS_CLK_L4_MP_HZ			(100000000)
#define CONFIG_HPS_CLK_L4_SP_HZ			(100000000)

#endif /* _PRELOADER_PLL_CONFIG_H_ */
