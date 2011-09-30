/*
 * This is a kernel module to be able to dump PIN configs and drive strengths
 *  pn realtime in .32 kernel
 *
 * Copyright (C) 2011 Eduardo José Tagle <ejtagle@tutopia.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */  

#include <linux/list.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/debugfs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <asm/io.h>



/*Physical address of Tegra EMC */
#define TEGRA_EMC_BASE 			0x7000F400
#define TEGRA_EMC_SIZE 			SZ_1K

#define TEGRA_APB_MISC_BASE		0x70000000
#define TEGRA_APB_MISC_SIZE		SZ_4K 

#define TEGRA_CLK_RESET_BASE	0x60006000
#define TEGRA_CLK_RESET_SIZE	SZ_4K
 
#define TEGRA_PMC_BASE			0x7000E400
#define TEGRA_PMC_SIZE			SZ_256 

#define TEGRA_FUSE_BASE			0x7000F800
#define TEGRA_FUSE_SIZE			SZ_1K
 
static void __iomem * reg_emc_base; 
static void __iomem * reg_apb_base; 
static void __iomem * reg_clk_base;
static void __iomem * reg_pmc_base;
static void __iomem * reg_apb_base;
static void __iomem * reg_fuse_base;

//

static inline unsigned long emc_pg_readl(unsigned long offset)
{
	return readl(reg_emc_base + offset);
}

static inline void emc_pg_write(unsigned long val,unsigned long offset)
{
	writel(val, reg_emc_base + offset);
}

static u32 tegra_fuse_readl(unsigned long offset)
{
	return readl(reg_fuse_base + offset);
}

#define clk_writel(value, reg) \
	__raw_writel(value, (u32)reg_clk_base + (reg))
#define clk_readl(reg) \
	__raw_readl((u32)reg_clk_base + (reg))
#define pmc_writel(value, reg) \
	__raw_writel(value, (u32)reg_pmc_base + (reg))
#define pmc_readl(reg) \
	__raw_readl((u32)reg_pmc_base + (reg))

static inline unsigned long pg_readl(unsigned long offset)
{
	return readl(reg_apb_base + offset);
}

static inline void pg_writel(unsigned long value, unsigned long offset)
{
	writel(value, reg_apb_base + offset);
}



/// Tegra2 pinmux

enum tegra_pingroup {
	TEGRA_PINGROUP_ATA = 0,
	TEGRA_PINGROUP_ATB,
	TEGRA_PINGROUP_ATC,
	TEGRA_PINGROUP_ATD,
	TEGRA_PINGROUP_ATE,
	TEGRA_PINGROUP_CDEV1,
	TEGRA_PINGROUP_CDEV2,
	TEGRA_PINGROUP_CRTP,
	TEGRA_PINGROUP_CSUS,
	TEGRA_PINGROUP_DAP1,
	TEGRA_PINGROUP_DAP2,
	TEGRA_PINGROUP_DAP3,
	TEGRA_PINGROUP_DAP4,
	TEGRA_PINGROUP_DDC,
	TEGRA_PINGROUP_DTA,
	TEGRA_PINGROUP_DTB,
	TEGRA_PINGROUP_DTC,
	TEGRA_PINGROUP_DTD,
	TEGRA_PINGROUP_DTE,
	TEGRA_PINGROUP_DTF,
	TEGRA_PINGROUP_GMA,
	TEGRA_PINGROUP_GMB,
	TEGRA_PINGROUP_GMC,
	TEGRA_PINGROUP_GMD,
	TEGRA_PINGROUP_GME,
	TEGRA_PINGROUP_GPU,
	TEGRA_PINGROUP_GPU7,
	TEGRA_PINGROUP_GPV,
	TEGRA_PINGROUP_HDINT,
	TEGRA_PINGROUP_I2CP,
	TEGRA_PINGROUP_IRRX,
	TEGRA_PINGROUP_IRTX,
	TEGRA_PINGROUP_KBCA,
	TEGRA_PINGROUP_KBCB,
	TEGRA_PINGROUP_KBCC,
	TEGRA_PINGROUP_KBCD,
	TEGRA_PINGROUP_KBCE,
	TEGRA_PINGROUP_KBCF,
	TEGRA_PINGROUP_LCSN,
	TEGRA_PINGROUP_LD0,
	TEGRA_PINGROUP_LD1,
	TEGRA_PINGROUP_LD10,
	TEGRA_PINGROUP_LD11,
	TEGRA_PINGROUP_LD12,
	TEGRA_PINGROUP_LD13,
	TEGRA_PINGROUP_LD14,
	TEGRA_PINGROUP_LD15,
	TEGRA_PINGROUP_LD16,
	TEGRA_PINGROUP_LD17,
	TEGRA_PINGROUP_LD2,
	TEGRA_PINGROUP_LD3,
	TEGRA_PINGROUP_LD4,
	TEGRA_PINGROUP_LD5,
	TEGRA_PINGROUP_LD6,
	TEGRA_PINGROUP_LD7,
	TEGRA_PINGROUP_LD8,
	TEGRA_PINGROUP_LD9,
	TEGRA_PINGROUP_LDC,
	TEGRA_PINGROUP_LDI,
	TEGRA_PINGROUP_LHP0,
	TEGRA_PINGROUP_LHP1,
	TEGRA_PINGROUP_LHP2,
	TEGRA_PINGROUP_LHS,
	TEGRA_PINGROUP_LM0,
	TEGRA_PINGROUP_LM1,
	TEGRA_PINGROUP_LPP,
	TEGRA_PINGROUP_LPW0,
	TEGRA_PINGROUP_LPW1,
	TEGRA_PINGROUP_LPW2,
	TEGRA_PINGROUP_LSC0,
	TEGRA_PINGROUP_LSC1,
	TEGRA_PINGROUP_LSCK,
	TEGRA_PINGROUP_LSDA,
	TEGRA_PINGROUP_LSDI,
	TEGRA_PINGROUP_LSPI,
	TEGRA_PINGROUP_LVP0,
	TEGRA_PINGROUP_LVP1,
	TEGRA_PINGROUP_LVS,
	TEGRA_PINGROUP_OWC,
	TEGRA_PINGROUP_PMC,
	TEGRA_PINGROUP_PTA,
	TEGRA_PINGROUP_RM,
	TEGRA_PINGROUP_SDB,
	TEGRA_PINGROUP_SDC,
	TEGRA_PINGROUP_SDD,
	TEGRA_PINGROUP_SDIO1,
	TEGRA_PINGROUP_SLXA,
	TEGRA_PINGROUP_SLXC,
	TEGRA_PINGROUP_SLXD,
	TEGRA_PINGROUP_SLXK,
	TEGRA_PINGROUP_SPDI,
	TEGRA_PINGROUP_SPDO,
	TEGRA_PINGROUP_SPIA,
	TEGRA_PINGROUP_SPIB,
	TEGRA_PINGROUP_SPIC,
	TEGRA_PINGROUP_SPID,
	TEGRA_PINGROUP_SPIE,
	TEGRA_PINGROUP_SPIF,
	TEGRA_PINGROUP_SPIG,
	TEGRA_PINGROUP_SPIH,
	TEGRA_PINGROUP_UAA,
	TEGRA_PINGROUP_UAB,
	TEGRA_PINGROUP_UAC,
	TEGRA_PINGROUP_UAD,
	TEGRA_PINGROUP_UCA,
	TEGRA_PINGROUP_UCB,
	TEGRA_PINGROUP_UDA,
	/* these pin groups only have pullup and pull down control */
	TEGRA_PINGROUP_CK32,
	TEGRA_PINGROUP_DDRC,
	TEGRA_PINGROUP_PMCA,
	TEGRA_PINGROUP_PMCB,
	TEGRA_PINGROUP_PMCC,
	TEGRA_PINGROUP_PMCD,
	TEGRA_PINGROUP_PMCE,
	TEGRA_PINGROUP_XM2C,
	TEGRA_PINGROUP_XM2D,
	TEGRA_MAX_PINGROUP,
};

enum tegra_drive_pingroup {
	TEGRA_DRIVE_PINGROUP_AO1 = 0,
	TEGRA_DRIVE_PINGROUP_AO2,
	TEGRA_DRIVE_PINGROUP_AT1,
	TEGRA_DRIVE_PINGROUP_AT2,
	TEGRA_DRIVE_PINGROUP_CDEV1,
	TEGRA_DRIVE_PINGROUP_CDEV2,
	TEGRA_DRIVE_PINGROUP_CSUS,
	TEGRA_DRIVE_PINGROUP_DAP1,
	TEGRA_DRIVE_PINGROUP_DAP2,
	TEGRA_DRIVE_PINGROUP_DAP3,
	TEGRA_DRIVE_PINGROUP_DAP4,
	TEGRA_DRIVE_PINGROUP_DBG,
	TEGRA_DRIVE_PINGROUP_LCD1,
	TEGRA_DRIVE_PINGROUP_LCD2,
	TEGRA_DRIVE_PINGROUP_SDMMC2,
	TEGRA_DRIVE_PINGROUP_SDMMC3,
	TEGRA_DRIVE_PINGROUP_SPI,
	TEGRA_DRIVE_PINGROUP_UAA,
	TEGRA_DRIVE_PINGROUP_UAB,
	TEGRA_DRIVE_PINGROUP_UART2,
	TEGRA_DRIVE_PINGROUP_UART3,
	TEGRA_DRIVE_PINGROUP_VI1,
	TEGRA_DRIVE_PINGROUP_VI2,
	TEGRA_DRIVE_PINGROUP_XM2A,
	TEGRA_DRIVE_PINGROUP_XM2C,
	TEGRA_DRIVE_PINGROUP_XM2D,
	TEGRA_DRIVE_PINGROUP_XM2CLK,
	TEGRA_DRIVE_PINGROUP_MEMCOMP,
	TEGRA_DRIVE_PINGROUP_SDIO1,
	TEGRA_DRIVE_PINGROUP_CRT,
	TEGRA_DRIVE_PINGROUP_DDC,
	TEGRA_DRIVE_PINGROUP_GMA,
	TEGRA_DRIVE_PINGROUP_GMB,
	TEGRA_DRIVE_PINGROUP_GMC,
	TEGRA_DRIVE_PINGROUP_GMD,
	TEGRA_DRIVE_PINGROUP_GME,
	TEGRA_DRIVE_PINGROUP_OWR,
	TEGRA_DRIVE_PINGROUP_UAD,
	TEGRA_MAX_DRIVE_PINGROUP,
};
  
enum tegra_mux_func {
	TEGRA_MUX_RSVD = 0x8000,
	TEGRA_MUX_RSVD1 = 0x8000,
	TEGRA_MUX_RSVD2 = 0x8001,
	TEGRA_MUX_RSVD3 = 0x8002,
	TEGRA_MUX_RSVD4 = 0x8003,
	TEGRA_MUX_NONE = -1,
	TEGRA_MUX_AHB_CLK,
	TEGRA_MUX_APB_CLK,
	TEGRA_MUX_AUDIO_SYNC,
	TEGRA_MUX_CRT,
	TEGRA_MUX_DAP1,
	TEGRA_MUX_DAP2,
	TEGRA_MUX_DAP3,
	TEGRA_MUX_DAP4,
	TEGRA_MUX_DAP5,
	TEGRA_MUX_DISPLAYA,
	TEGRA_MUX_DISPLAYB,
	TEGRA_MUX_EMC_TEST0_DLL,
	TEGRA_MUX_EMC_TEST1_DLL,
	TEGRA_MUX_GMI,
	TEGRA_MUX_GMI_INT,
	TEGRA_MUX_HDMI,
	TEGRA_MUX_I2C,
	TEGRA_MUX_I2C2,
	TEGRA_MUX_I2C3,
	TEGRA_MUX_IDE,
	TEGRA_MUX_IRDA,
	TEGRA_MUX_KBC,
	TEGRA_MUX_MIO,
	TEGRA_MUX_MIPI_HS,
	TEGRA_MUX_NAND,
	TEGRA_MUX_OSC,
	TEGRA_MUX_OWR,
	TEGRA_MUX_PCIE,
	TEGRA_MUX_PLLA_OUT,
	TEGRA_MUX_PLLC_OUT1,
	TEGRA_MUX_PLLM_OUT1,
	TEGRA_MUX_PLLP_OUT2,
	TEGRA_MUX_PLLP_OUT3,
	TEGRA_MUX_PLLP_OUT4,
	TEGRA_MUX_PWM,
	TEGRA_MUX_PWR_INTR,
	TEGRA_MUX_PWR_ON,
	TEGRA_MUX_RTCK,
	TEGRA_MUX_SDIO1,
	TEGRA_MUX_SDIO2,
	TEGRA_MUX_SDIO3,
	TEGRA_MUX_SDIO4,
	TEGRA_MUX_SFLASH,
	TEGRA_MUX_SPDIF,
	TEGRA_MUX_SPI1,
	TEGRA_MUX_SPI2,
	TEGRA_MUX_SPI2_ALT,
	TEGRA_MUX_SPI3,
	TEGRA_MUX_SPI4,
	TEGRA_MUX_TRACE,
	TEGRA_MUX_TWC,
	TEGRA_MUX_UARTA,
	TEGRA_MUX_UARTB,
	TEGRA_MUX_UARTC,
	TEGRA_MUX_UARTD,
	TEGRA_MUX_UARTE,
	TEGRA_MUX_ULPI,
	TEGRA_MUX_VI,
	TEGRA_MUX_VI_SENSOR_CLK,
	TEGRA_MUX_XIO,
	TEGRA_MUX_SAFE,
	TEGRA_MAX_MUX,
};

enum tegra_pullupdown {
	TEGRA_PUPD_NORMAL = 0,
	TEGRA_PUPD_PULL_DOWN,
	TEGRA_PUPD_PULL_UP,
};

enum tegra_tristate {
	TEGRA_TRI_NORMAL = 0,
	TEGRA_TRI_TRISTATE = 1,
};

enum tegra_vddio {
	TEGRA_VDDIO_BB = 0,
	TEGRA_VDDIO_LCD,
	TEGRA_VDDIO_VI,
	TEGRA_VDDIO_UART,
	TEGRA_VDDIO_DDR,
	TEGRA_VDDIO_NAND,
	TEGRA_VDDIO_SYS,
	TEGRA_VDDIO_AUDIO,
	TEGRA_VDDIO_SD,
};

struct tegra_pingroup_config {
	enum tegra_pingroup		pingroup;
	enum tegra_mux_func		func;
	enum tegra_pullupdown	pupd;
	enum tegra_tristate		tristate;
};

enum tegra_slew {
	TEGRA_SLEW_FASTEST = 0,
	TEGRA_SLEW_FAST,
	TEGRA_SLEW_SLOW,
	TEGRA_SLEW_SLOWEST,
	TEGRA_MAX_SLEW,
};

enum tegra_pull_strength {
	TEGRA_PULL_0 = 0,
	TEGRA_PULL_1,
	TEGRA_PULL_2,
	TEGRA_PULL_3,
	TEGRA_PULL_4,
	TEGRA_PULL_5,
	TEGRA_PULL_6,
	TEGRA_PULL_7,
	TEGRA_PULL_8,
	TEGRA_PULL_9,
	TEGRA_PULL_10,
	TEGRA_PULL_11,
	TEGRA_PULL_12,
	TEGRA_PULL_13,
	TEGRA_PULL_14,
	TEGRA_PULL_15,
	TEGRA_PULL_16,
	TEGRA_PULL_17,
	TEGRA_PULL_18,
	TEGRA_PULL_19,
	TEGRA_PULL_20,
	TEGRA_PULL_21,
	TEGRA_PULL_22,
	TEGRA_PULL_23,
	TEGRA_PULL_24,
	TEGRA_PULL_25,
	TEGRA_PULL_26,
	TEGRA_PULL_27,
	TEGRA_PULL_28,
	TEGRA_PULL_29,
	TEGRA_PULL_30,
	TEGRA_PULL_31,
	TEGRA_MAX_PULL,
};

enum tegra_drive {
	TEGRA_DRIVE_DIV_8 = 0,
	TEGRA_DRIVE_DIV_4,
	TEGRA_DRIVE_DIV_2,
	TEGRA_DRIVE_DIV_1,
	TEGRA_MAX_DRIVE,
};

enum tegra_hsm {
	TEGRA_HSM_DISABLE = 0,
	TEGRA_HSM_ENABLE,
};

enum tegra_schmitt {
	TEGRA_SCHMITT_DISABLE = 0,
	TEGRA_SCHMITT_ENABLE,
};

struct tegra_drive_pingroup_config {
	enum tegra_drive_pingroup 	pingroup;
	enum tegra_hsm 				hsm;
	enum tegra_schmitt 			schmitt;
	enum tegra_drive 			drive;
	enum tegra_pull_strength 	pull_down;
	enum tegra_pull_strength 	pull_up;
	enum tegra_slew 			slew_rising;
	enum tegra_slew 			slew_falling;
};

struct tegra_drive_pingroup_desc {
	const char *name;
	s16 reg;
};

struct tegra_pingroup_desc {
	const char *name;
	int 		funcs[4];
	int 		func_safe;
	int 		vddio;
	s16 		tri_reg; 	/* offset into the TRISTATE_REG_* register bank */
	s16 		mux_reg;	/* offset into the PIN_MUX_CTL_* register bank */
	s16 		pupd_reg;	/* offset into the PULL_UPDOWN_REG_* register bank */
	s8 			tri_bit; 	/* offset into the TRISTATE_REG_* register bit */
	s8 			mux_bit;	/* offset into the PIN_MUX_CTL_* register bit */
	s8 			pupd_bit;	/* offset into the PULL_UPDOWN_REG_* register bit */
};

#define DRIVE_PINGROUP(pg_name, r)				\
	[TEGRA_DRIVE_PINGROUP_ ## pg_name] = {			\
		.name = #pg_name,				\
		.reg = r					\
	}

static const struct tegra_drive_pingroup_desc tegra_soc_drive_pingroups[TEGRA_MAX_DRIVE_PINGROUP] = {
	DRIVE_PINGROUP(AO1,		0x868),
	DRIVE_PINGROUP(AO2,		0x86c),
	DRIVE_PINGROUP(AT1,		0x870),
	DRIVE_PINGROUP(AT2,		0x874),
	DRIVE_PINGROUP(CDEV1,		0x878),
	DRIVE_PINGROUP(CDEV2,		0x87c),
	DRIVE_PINGROUP(CSUS,		0x880),
	DRIVE_PINGROUP(DAP1,		0x884),
	DRIVE_PINGROUP(DAP2,		0x888),
	DRIVE_PINGROUP(DAP3,		0x88c),
	DRIVE_PINGROUP(DAP4,		0x890),
	DRIVE_PINGROUP(DBG,		0x894),
	DRIVE_PINGROUP(LCD1,		0x898),
	DRIVE_PINGROUP(LCD2,		0x89c),
	DRIVE_PINGROUP(SDMMC2,		0x8a0),
	DRIVE_PINGROUP(SDMMC3,		0x8a4),
	DRIVE_PINGROUP(SPI,		0x8a8),
	DRIVE_PINGROUP(UAA,		0x8ac),
	DRIVE_PINGROUP(UAB,		0x8b0),
	DRIVE_PINGROUP(UART2,		0x8b4),
	DRIVE_PINGROUP(UART3,		0x8b8),
	DRIVE_PINGROUP(VI1,		0x8bc),
	DRIVE_PINGROUP(VI2,		0x8c0),
	DRIVE_PINGROUP(XM2A,		0x8c4),
	DRIVE_PINGROUP(XM2C,		0x8c8),
	DRIVE_PINGROUP(XM2D,		0x8cc),
	DRIVE_PINGROUP(XM2CLK,		0x8d0),
	DRIVE_PINGROUP(MEMCOMP,		0x8d4),
	DRIVE_PINGROUP(SDIO1,		0x8e0),
	DRIVE_PINGROUP(CRT,		0x8ec),
	DRIVE_PINGROUP(DDC,		0x8f0),
	DRIVE_PINGROUP(GMA,		0x8f4),
	DRIVE_PINGROUP(GMB,		0x8f8),
	DRIVE_PINGROUP(GMC,		0x8fc),
	DRIVE_PINGROUP(GMD,		0x900),
	DRIVE_PINGROUP(GME,		0x904),
	DRIVE_PINGROUP(OWR,		0x908),
	DRIVE_PINGROUP(UAD,		0x90c),
};

#define PINGROUP(pg_name, vdd, f0, f1, f2, f3, f_safe,		\
		 tri_r, tri_b, mux_r, mux_b, pupd_r, pupd_b)	\
	[TEGRA_PINGROUP_ ## pg_name] = {			\
		.name = #pg_name,				\
		.vddio = TEGRA_VDDIO_ ## vdd,			\
		.funcs = {					\
			TEGRA_MUX_ ## f0,			\
			TEGRA_MUX_ ## f1,			\
			TEGRA_MUX_ ## f2,			\
			TEGRA_MUX_ ## f3,			\
		},						\
		.func_safe = TEGRA_MUX_ ## f_safe,		\
		.tri_reg = tri_r,				\
		.tri_bit = tri_b,				\
		.mux_reg = mux_r,				\
		.mux_bit = mux_b,				\
		.pupd_reg = pupd_r,				\
		.pupd_bit = pupd_b,				\
	}

static const struct tegra_pingroup_desc tegra_soc_pingroups[TEGRA_MAX_PINGROUP] = {
	PINGROUP(ATA,   NAND,  IDE,       NAND,      GMI,       RSVD,          IDE,       0x14, 0,  0x80, 24, 0xA0, 0),
	PINGROUP(ATB,   NAND,  IDE,       NAND,      GMI,       SDIO4,         IDE,       0x14, 1,  0x80, 16, 0xA0, 2),
	PINGROUP(ATC,   NAND,  IDE,       NAND,      GMI,       SDIO4,         IDE,       0x14, 2,  0x80, 22, 0xA0, 4),
	PINGROUP(ATD,   NAND,  IDE,       NAND,      GMI,       SDIO4,         IDE,       0x14, 3,  0x80, 20, 0xA0, 6),
	PINGROUP(ATE,   NAND,  IDE,       NAND,      GMI,       RSVD,          IDE,       0x18, 25, 0x80, 12, 0xA0, 8),
	PINGROUP(CDEV1, AUDIO, OSC,       PLLA_OUT,  PLLM_OUT1, AUDIO_SYNC,    OSC,       0x14, 4,  0x88, 2,  0xA8, 0),
	PINGROUP(CDEV2, AUDIO, OSC,       AHB_CLK,   APB_CLK,   PLLP_OUT4,     OSC,       0x14, 5,  0x88, 4,  0xA8, 2),
	PINGROUP(CRTP,  LCD,   CRT,       RSVD,      RSVD,      RSVD,          RSVD,      0x20, 14, 0x98, 20, 0xA4, 24),
	PINGROUP(CSUS,  VI,    PLLC_OUT1, PLLP_OUT2, PLLP_OUT3, VI_SENSOR_CLK, PLLC_OUT1, 0x14, 6,  0x88, 6,  0xAC, 24),
	PINGROUP(DAP1,  AUDIO, DAP1,      RSVD,      GMI,       SDIO2,         DAP1,      0x14, 7,  0x88, 20, 0xA0, 10),
	PINGROUP(DAP2,  AUDIO, DAP2,      TWC,       RSVD,      GMI,           DAP2,      0x14, 8,  0x88, 22, 0xA0, 12),
	PINGROUP(DAP3,  BB,    DAP3,      RSVD,      RSVD,      RSVD,          DAP3,      0x14, 9,  0x88, 24, 0xA0, 14),
	PINGROUP(DAP4,  UART,  DAP4,      RSVD,      GMI,       RSVD,          DAP4,      0x14, 10, 0x88, 26, 0xA0, 16),
	PINGROUP(DDC,   LCD,   I2C2,      RSVD,      RSVD,      RSVD,          RSVD4,     0x18, 31, 0x88, 0,  0xB0, 28),
	PINGROUP(DTA,   VI,    RSVD,      SDIO2,     VI,        RSVD,          RSVD4,     0x14, 11, 0x84, 20, 0xA0, 18),
	PINGROUP(DTB,   VI,    RSVD,      RSVD,      VI,        SPI1,          RSVD1,     0x14, 12, 0x84, 22, 0xA0, 20),
	PINGROUP(DTC,   VI,    RSVD,      RSVD,      VI,        RSVD,          RSVD1,     0x14, 13, 0x84, 26, 0xA0, 22),
	PINGROUP(DTD,   VI,    RSVD,      SDIO2,     VI,        RSVD,          RSVD1,     0x14, 14, 0x84, 28, 0xA0, 24),
	PINGROUP(DTE,   VI,    RSVD,      RSVD,      VI,        SPI1,          RSVD1,     0x14, 15, 0x84, 30, 0xA0, 26),
	PINGROUP(DTF,   VI,    I2C3,      RSVD,      VI,        RSVD,          RSVD4,     0x20, 12, 0x98, 30, 0xA0, 28),
	PINGROUP(GMA,   NAND,  UARTE,     SPI3,      GMI,       SDIO4,         SPI3,      0x14, 28, 0x84, 0,  0xB0, 20),
	PINGROUP(GMB,   NAND,  IDE,       NAND,      GMI,       GMI_INT,       GMI,       0x18, 29, 0x88, 28, 0xB0, 22),
	PINGROUP(GMC,   NAND,  UARTD,     SPI4,      GMI,       SFLASH,        SPI4,      0x14, 29, 0x84, 2,  0xB0, 24),
	PINGROUP(GMD,   NAND,  RSVD,      NAND,      GMI,       SFLASH,        GMI,       0x18, 30, 0x88, 30, 0xB0, 26),
	PINGROUP(GME,   NAND,  RSVD,      DAP5,      GMI,       SDIO4,         GMI,       0x18, 0,  0x8C, 0,  0xA8, 24),
	PINGROUP(GPU,   UART,  PWM,       UARTA,     GMI,       RSVD,          RSVD4,     0x14, 16, 0x8C, 4,  0xA4, 20),
	PINGROUP(GPU7,  SYS,   RTCK,      RSVD,      RSVD,      RSVD,          RTCK,      0x20, 11, 0x98, 28, 0xA4, 6),
	PINGROUP(GPV,   SD,    PCIE,      RSVD,      RSVD,      RSVD,          PCIE,      0x14, 17, 0x8C, 2,  0xA0, 30),
	PINGROUP(HDINT, LCD,   HDMI,      RSVD,      RSVD,      RSVD,          HDMI,      0x1C, 23, 0x84, 4,  0xAC, 22),
	PINGROUP(I2CP,  SYS,   I2C,       RSVD,      RSVD,      RSVD,          RSVD4,     0x14, 18, 0x88, 8,  0xA4, 2),
	PINGROUP(IRRX,  UART,  UARTA,     UARTB,     GMI,       SPI4,          UARTB,     0x14, 20, 0x88, 18, 0xA8, 22),
	PINGROUP(IRTX,  UART,  UARTA,     UARTB,     GMI,       SPI4,          UARTB,     0x14, 19, 0x88, 16, 0xA8, 20),
	PINGROUP(KBCA,  SYS,   KBC,       NAND,      SDIO2,     EMC_TEST0_DLL, KBC,       0x14, 22, 0x88, 10, 0xA4, 8),
	PINGROUP(KBCB,  SYS,   KBC,       NAND,      SDIO2,     MIO,           KBC,       0x14, 21, 0x88, 12, 0xA4, 10),
	PINGROUP(KBCC,  SYS,   KBC,       NAND,      TRACE,     EMC_TEST1_DLL, KBC,       0x18, 26, 0x88, 14, 0xA4, 12),
	PINGROUP(KBCD,  SYS,   KBC,       NAND,      SDIO2,     MIO,           KBC,       0x20, 10, 0x98, 26, 0xA4, 14),
	PINGROUP(KBCE,  SYS,   KBC,       NAND,      OWR,       RSVD,          KBC,       0x14, 26, 0x80, 28, 0xB0, 2),
	PINGROUP(KBCF,  SYS,   KBC,       NAND,      TRACE,     MIO,           KBC,       0x14, 27, 0x80, 26, 0xB0, 0),
	PINGROUP(LCSN,  LCD,   DISPLAYA,  DISPLAYB,  SPI3,      RSVD,          RSVD4,     0x1C, 31, 0x90, 12, 0xAC, 20),
	PINGROUP(LD0,   LCD,   DISPLAYA,  DISPLAYB,  XIO,       RSVD,          RSVD4,     0x1C, 0,  0x94, 0,  0xAC, 12),
	PINGROUP(LD1,   LCD,   DISPLAYA,  DISPLAYB,  XIO,       RSVD,          RSVD4,     0x1C, 1,  0x94, 2,  0xAC, 12),
	PINGROUP(LD10,  LCD,   DISPLAYA,  DISPLAYB,  XIO,       RSVD,          RSVD4,     0x1C, 10, 0x94, 20, 0xAC, 12),
	PINGROUP(LD11,  LCD,   DISPLAYA,  DISPLAYB,  XIO,       RSVD,          RSVD4,     0x1C, 11, 0x94, 22, 0xAC, 12),
	PINGROUP(LD12,  LCD,   DISPLAYA,  DISPLAYB,  XIO,       RSVD,          RSVD4,     0x1C, 12, 0x94, 24, 0xAC, 12),
	PINGROUP(LD13,  LCD,   DISPLAYA,  DISPLAYB,  XIO,       RSVD,          RSVD4,     0x1C, 13, 0x94, 26, 0xAC, 12),
	PINGROUP(LD14,  LCD,   DISPLAYA,  DISPLAYB,  XIO,       RSVD,          RSVD4,     0x1C, 14, 0x94, 28, 0xAC, 12),
	PINGROUP(LD15,  LCD,   DISPLAYA,  DISPLAYB,  XIO,       RSVD,          RSVD4,     0x1C, 15, 0x94, 30, 0xAC, 12),
	PINGROUP(LD16,  LCD,   DISPLAYA,  DISPLAYB,  XIO,       RSVD,          RSVD4,     0x1C, 16, 0x98, 0,  0xAC, 12),
	PINGROUP(LD17,  LCD,   DISPLAYA,  DISPLAYB,  RSVD,      RSVD,          RSVD4,     0x1C, 17, 0x98, 2,  0xAC, 12),
	PINGROUP(LD2,   LCD,   DISPLAYA,  DISPLAYB,  XIO,       RSVD,          RSVD4,     0x1C, 2,  0x94, 4,  0xAC, 12),
	PINGROUP(LD3,   LCD,   DISPLAYA,  DISPLAYB,  XIO,       RSVD,          RSVD4,     0x1C, 3,  0x94, 6,  0xAC, 12),
	PINGROUP(LD4,   LCD,   DISPLAYA,  DISPLAYB,  XIO,       RSVD,          RSVD4,     0x1C, 4,  0x94, 8,  0xAC, 12),
	PINGROUP(LD5,   LCD,   DISPLAYA,  DISPLAYB,  XIO,       RSVD,          RSVD4,     0x1C, 5,  0x94, 10, 0xAC, 12),
	PINGROUP(LD6,   LCD,   DISPLAYA,  DISPLAYB,  XIO,       RSVD,          RSVD4,     0x1C, 6,  0x94, 12, 0xAC, 12),
	PINGROUP(LD7,   LCD,   DISPLAYA,  DISPLAYB,  XIO,       RSVD,          RSVD4,     0x1C, 7,  0x94, 14, 0xAC, 12),
	PINGROUP(LD8,   LCD,   DISPLAYA,  DISPLAYB,  XIO,       RSVD,          RSVD4,     0x1C, 8,  0x94, 16, 0xAC, 12),
	PINGROUP(LD9,   LCD,   DISPLAYA,  DISPLAYB,  XIO,       RSVD,          RSVD4,     0x1C, 9,  0x94, 18, 0xAC, 12),
	PINGROUP(LDC,   LCD,   DISPLAYA,  DISPLAYB,  RSVD,      RSVD,          RSVD4,     0x1C, 30, 0x90, 14, 0xAC, 20),
	PINGROUP(LDI,   LCD,   DISPLAYA,  DISPLAYB,  RSVD,      RSVD,          RSVD4,     0x20, 6,  0x98, 16, 0xAC, 18),
	PINGROUP(LHP0,  LCD,   DISPLAYA,  DISPLAYB,  RSVD,      RSVD,          RSVD4,     0x1C, 18, 0x98, 10, 0xAC, 16),
	PINGROUP(LHP1,  LCD,   DISPLAYA,  DISPLAYB,  RSVD,      RSVD,          RSVD4,     0x1C, 19, 0x98, 4,  0xAC, 14),
	PINGROUP(LHP2,  LCD,   DISPLAYA,  DISPLAYB,  RSVD,      RSVD,          RSVD4,     0x1C, 20, 0x98, 6,  0xAC, 14),
	PINGROUP(LHS,   LCD,   DISPLAYA,  DISPLAYB,  XIO,       RSVD,          RSVD4,     0x20, 7,  0x90, 22, 0xAC, 22),
	PINGROUP(LM0,   LCD,   DISPLAYA,  DISPLAYB,  SPI3,      RSVD,          RSVD4,     0x1C, 24, 0x90, 26, 0xAC, 22),
	PINGROUP(LM1,   LCD,   DISPLAYA,  DISPLAYB,  RSVD,      CRT,           RSVD3,     0x1C, 25, 0x90, 28, 0xAC, 22),
	PINGROUP(LPP,   LCD,   DISPLAYA,  DISPLAYB,  RSVD,      RSVD,          RSVD4,     0x20, 8,  0x98, 14, 0xAC, 18),
	PINGROUP(LPW0,  LCD,   DISPLAYA,  DISPLAYB,  SPI3,      HDMI,          DISPLAYA,  0x20, 3,  0x90, 0,  0xAC, 20),
	PINGROUP(LPW1,  LCD,   DISPLAYA,  DISPLAYB,  RSVD,      RSVD,          RSVD4,     0x20, 4,  0x90, 2,  0xAC, 20),
	PINGROUP(LPW2,  LCD,   DISPLAYA,  DISPLAYB,  SPI3,      HDMI,          DISPLAYA,  0x20, 5,  0x90, 4,  0xAC, 20),
	PINGROUP(LSC0,  LCD,   DISPLAYA,  DISPLAYB,  XIO,       RSVD,          RSVD4,     0x1C, 27, 0x90, 18, 0xAC, 22),
	PINGROUP(LSC1,  LCD,   DISPLAYA,  DISPLAYB,  SPI3,      HDMI,          DISPLAYA,  0x1C, 28, 0x90, 20, 0xAC, 20),
	PINGROUP(LSCK,  LCD,   DISPLAYA,  DISPLAYB,  SPI3,      HDMI,          DISPLAYA,  0x1C, 29, 0x90, 16, 0xAC, 20),
	PINGROUP(LSDA,  LCD,   DISPLAYA,  DISPLAYB,  SPI3,      HDMI,          DISPLAYA,  0x20, 1,  0x90, 8,  0xAC, 20),
	PINGROUP(LSDI,  LCD,   DISPLAYA,  DISPLAYB,  SPI3,      RSVD,          DISPLAYA,  0x20, 2,  0x90, 6,  0xAC, 20),
	PINGROUP(LSPI,  LCD,   DISPLAYA,  DISPLAYB,  XIO,       HDMI,          DISPLAYA,  0x20, 0,  0x90, 10, 0xAC, 22),
	PINGROUP(LVP0,  LCD,   DISPLAYA,  DISPLAYB,  RSVD,      RSVD,          RSVD4,     0x1C, 21, 0x90, 30, 0xAC, 22),
	PINGROUP(LVP1,  LCD,   DISPLAYA,  DISPLAYB,  RSVD,      RSVD,          RSVD4,     0x1C, 22, 0x98, 8,  0xAC, 16),
	PINGROUP(LVS,   LCD,   DISPLAYA,  DISPLAYB,  XIO,       RSVD,          RSVD4,     0x1C, 26, 0x90, 24, 0xAC, 22),
	PINGROUP(OWC,   SYS,   OWR,       RSVD,      RSVD,      RSVD,          OWR,       0x14, 31, 0x84, 8,  0xB0, 30),
	PINGROUP(PMC,   SYS,   PWR_ON,    PWR_INTR,  RSVD,      RSVD,          PWR_ON,    0x14, 23, 0x98, 18, -1,   -1),
	PINGROUP(PTA,   NAND,  I2C2,      HDMI,      GMI,       RSVD,          RSVD4,     0x14, 24, 0x98, 22, 0xA4, 4),
	PINGROUP(RM,    UART,  I2C,       RSVD,      RSVD,      RSVD,          RSVD4,     0x14, 25, 0x80, 14, 0xA4, 0),
	PINGROUP(SDB,   SD,    UARTA,     PWM,       SDIO3,     SPI2,          PWM,       0x20, 15, 0x8C, 10, -1,   -1),
	PINGROUP(SDC,   SD,    PWM,       TWC,       SDIO3,     SPI3,          TWC,       0x18, 1,  0x8C, 12, 0xAC, 28),
	PINGROUP(SDD,   SD,    UARTA,     PWM,       SDIO3,     SPI3,          PWM,       0x18, 2,  0x8C, 14, 0xAC, 30),
	PINGROUP(SDIO1, BB,    SDIO1,     RSVD,      UARTE,     UARTA,         RSVD2,     0x14, 30, 0x80, 30, 0xB0, 18),
	PINGROUP(SLXA,  SD,    PCIE,      SPI4,      SDIO3,     SPI2,          PCIE,      0x18, 3,  0x84, 6,  0xA4, 22),
	PINGROUP(SLXC,  SD,    SPDIF,     SPI4,      SDIO3,     SPI2,          SPI4,      0x18, 5,  0x84, 10, 0xA4, 26),
	PINGROUP(SLXD,  SD,    SPDIF,     SPI4,      SDIO3,     SPI2,          SPI4,      0x18, 6,  0x84, 12, 0xA4, 28),
	PINGROUP(SLXK,  SD,    PCIE,      SPI4,      SDIO3,     SPI2,          PCIE,      0x18, 7,  0x84, 14, 0xA4, 30),
	PINGROUP(SPDI,  AUDIO, SPDIF,     RSVD,      I2C,       SDIO2,         RSVD2,     0x18, 8,  0x8C, 8,  0xA4, 16),
	PINGROUP(SPDO,  AUDIO, SPDIF,     RSVD,      I2C,       SDIO2,         RSVD2,     0x18, 9,  0x8C, 6,  0xA4, 18),
	PINGROUP(SPIA,  AUDIO, SPI1,      SPI2,      SPI3,      GMI,           GMI,       0x18, 10, 0x8C, 30, 0xA8, 4),
	PINGROUP(SPIB,  AUDIO, SPI1,      SPI2,      SPI3,      GMI,           GMI,       0x18, 11, 0x8C, 28, 0xA8, 6),
	PINGROUP(SPIC,  AUDIO, SPI1,      SPI2,      SPI3,      GMI,           GMI,       0x18, 12, 0x8C, 26, 0xA8, 8),
	PINGROUP(SPID,  AUDIO, SPI2,      SPI1,      SPI2_ALT,  GMI,           GMI,       0x18, 13, 0x8C, 24, 0xA8, 10),
	PINGROUP(SPIE,  AUDIO, SPI2,      SPI1,      SPI2_ALT,  GMI,           GMI,       0x18, 14, 0x8C, 22, 0xA8, 12),
	PINGROUP(SPIF,  AUDIO, SPI3,      SPI1,      SPI2,      RSVD,          RSVD4,     0x18, 15, 0x8C, 20, 0xA8, 14),
	PINGROUP(SPIG,  AUDIO, SPI3,      SPI2,      SPI2_ALT,  I2C,           SPI2_ALT,  0x18, 16, 0x8C, 18, 0xA8, 16),
	PINGROUP(SPIH,  AUDIO, SPI3,      SPI2,      SPI2_ALT,  I2C,           SPI2_ALT,  0x18, 17, 0x8C, 16, 0xA8, 18),
	PINGROUP(UAA,   BB,    SPI3,      MIPI_HS,   UARTA,     ULPI,          MIPI_HS,   0x18, 18, 0x80, 0,  0xAC, 0),
	PINGROUP(UAB,   BB,    SPI2,      MIPI_HS,   UARTA,     ULPI,          MIPI_HS,   0x18, 19, 0x80, 2,  0xAC, 2),
	PINGROUP(UAC,   BB,    OWR,       RSVD,      RSVD,      RSVD,          RSVD4,     0x18, 20, 0x80, 4,  0xAC, 4),
	PINGROUP(UAD,   UART,  IRDA,      SPDIF,     UARTA,     SPI4,          SPDIF,     0x18, 21, 0x80, 6,  0xAC, 6),
	PINGROUP(UCA,   UART,  UARTC,     RSVD,      GMI,       RSVD,          RSVD4,     0x18, 22, 0x84, 16, 0xAC, 8),
	PINGROUP(UCB,   UART,  UARTC,     PWM,       GMI,       RSVD,          RSVD4,     0x18, 23, 0x84, 18, 0xAC, 10),
	PINGROUP(UDA,   BB,    SPI1,      RSVD,      UARTD,     ULPI,          RSVD2,     0x20, 13, 0x80, 8,  0xB0, 16),
	/* these pin groups only have pullup and pull down control */
	PINGROUP(CK32,  SYS,   RSVD,      RSVD,      RSVD,      RSVD,          RSVD,      -1,   -1, -1,   -1, 0xB0, 14),
	PINGROUP(DDRC,  DDR,   RSVD,      RSVD,      RSVD,      RSVD,          RSVD,      -1,   -1, -1,   -1, 0xAC, 26),
	PINGROUP(PMCA,  SYS,   RSVD,      RSVD,      RSVD,      RSVD,          RSVD,      -1,   -1, -1,   -1, 0xB0, 4),
	PINGROUP(PMCB,  SYS,   RSVD,      RSVD,      RSVD,      RSVD,          RSVD,      -1,   -1, -1,   -1, 0xB0, 6),
	PINGROUP(PMCC,  SYS,   RSVD,      RSVD,      RSVD,      RSVD,          RSVD,      -1,   -1, -1,   -1, 0xB0, 8),
	PINGROUP(PMCD,  SYS,   RSVD,      RSVD,      RSVD,      RSVD,          RSVD,      -1,   -1, -1,   -1, 0xB0, 10),
	PINGROUP(PMCE,  SYS,   RSVD,      RSVD,      RSVD,      RSVD,          RSVD,      -1,   -1, -1,   -1, 0xB0, 12),
	PINGROUP(XM2C,  DDR,   RSVD,      RSVD,      RSVD,      RSVD,          RSVD,      -1,   -1, -1,   -1, 0xA8, 30),
	PINGROUP(XM2D,  DDR,   RSVD,      RSVD,      RSVD,      RSVD,          RSVD,      -1,   -1, -1,   -1, 0xA8, 28),
};


#define HSM_EN(reg)	(((reg) >> 2) & 0x1)
#define SCHMT_EN(reg)	(((reg) >> 3) & 0x1)
#define LPMD(reg)	(((reg) >> 4) & 0x3)
#define DRVDN(reg)	(((reg) >> 12) & 0x1f)
#define DRVUP(reg)	(((reg) >> 20) & 0x1f)
#define SLWR(reg)	(((reg) >> 28) & 0x3)
#define SLWF(reg)	(((reg) >> 30) & 0x3)

static const struct tegra_pingroup_desc *const pingroups = tegra_soc_pingroups;
static const struct tegra_drive_pingroup_desc *const drive_pingroups = tegra_soc_drive_pingroups;

static char *tegra_mux_names[TEGRA_MAX_MUX] = {
	[TEGRA_MUX_AHB_CLK] = "AHB_CLK",
	[TEGRA_MUX_APB_CLK] = "APB_CLK",
	[TEGRA_MUX_AUDIO_SYNC] = "AUDIO_SYNC",
	[TEGRA_MUX_CRT] = "CRT",
	[TEGRA_MUX_DAP1] = "DAP1",
	[TEGRA_MUX_DAP2] = "DAP2",
	[TEGRA_MUX_DAP3] = "DAP3",
	[TEGRA_MUX_DAP4] = "DAP4",
	[TEGRA_MUX_DAP5] = "DAP5",
	[TEGRA_MUX_DISPLAYA] = "DISPLAYA",
	[TEGRA_MUX_DISPLAYB] = "DISPLAYB",
	[TEGRA_MUX_EMC_TEST0_DLL] = "EMC_TEST0_DLL",
	[TEGRA_MUX_EMC_TEST1_DLL] = "EMC_TEST1_DLL",
	[TEGRA_MUX_GMI] = "GMI",
	[TEGRA_MUX_GMI_INT] = "GMI_INT",
	[TEGRA_MUX_HDMI] = "HDMI",
	[TEGRA_MUX_I2C] = "I2C",
	[TEGRA_MUX_I2C2] = "I2C2",
	[TEGRA_MUX_I2C3] = "I2C3",
	[TEGRA_MUX_IDE] = "IDE",
	[TEGRA_MUX_IRDA] = "IRDA",
	[TEGRA_MUX_KBC] = "KBC",
	[TEGRA_MUX_MIO] = "MIO",
	[TEGRA_MUX_MIPI_HS] = "MIPI_HS",
	[TEGRA_MUX_NAND] = "NAND",
	[TEGRA_MUX_OSC] = "OSC",
	[TEGRA_MUX_OWR] = "OWR",
	[TEGRA_MUX_PCIE] = "PCIE",
	[TEGRA_MUX_PLLA_OUT] = "PLLA_OUT",
	[TEGRA_MUX_PLLC_OUT1] = "PLLC_OUT1",
	[TEGRA_MUX_PLLM_OUT1] = "PLLM_OUT1",
	[TEGRA_MUX_PLLP_OUT2] = "PLLP_OUT2",
	[TEGRA_MUX_PLLP_OUT3] = "PLLP_OUT3",
	[TEGRA_MUX_PLLP_OUT4] = "PLLP_OUT4",
	[TEGRA_MUX_PWM] = "PWM",
	[TEGRA_MUX_PWR_INTR] = "PWR_INTR",
	[TEGRA_MUX_PWR_ON] = "PWR_ON",
	[TEGRA_MUX_RTCK] = "RTCK",
	[TEGRA_MUX_SDIO1] = "SDIO1",
	[TEGRA_MUX_SDIO2] = "SDIO2",
	[TEGRA_MUX_SDIO3] = "SDIO3",
	[TEGRA_MUX_SDIO4] = "SDIO4",
	[TEGRA_MUX_SFLASH] = "SFLASH",
	[TEGRA_MUX_SPDIF] = "SPDIF",
	[TEGRA_MUX_SPI1] = "SPI1",
	[TEGRA_MUX_SPI2] = "SPI2",
	[TEGRA_MUX_SPI2_ALT] = "SPI2_ALT",
	[TEGRA_MUX_SPI3] = "SPI3",
	[TEGRA_MUX_SPI4] = "SPI4",
	[TEGRA_MUX_TRACE] = "TRACE",
	[TEGRA_MUX_TWC] = "TWC",
	[TEGRA_MUX_UARTA] = "UARTA",
	[TEGRA_MUX_UARTB] = "UARTB",
	[TEGRA_MUX_UARTC] = "UARTC",
	[TEGRA_MUX_UARTD] = "UARTD",
	[TEGRA_MUX_UARTE] = "UARTE",
	[TEGRA_MUX_ULPI] = "ULPI",
	[TEGRA_MUX_VI] = "VI",
	[TEGRA_MUX_VI_SENSOR_CLK] = "VI_SENSOR_CLK",
	[TEGRA_MUX_XIO] = "XIO",
	[TEGRA_MUX_SAFE] = "<safe>",
};

static const char *tegra_drive_names[TEGRA_MAX_DRIVE] = {
	[TEGRA_DRIVE_DIV_8] = "DIV_8",
	[TEGRA_DRIVE_DIV_4] = "DIV_4",
	[TEGRA_DRIVE_DIV_2] = "DIV_2",
	[TEGRA_DRIVE_DIV_1] = "DIV_1",
};

static const char *tegra_slew_names[TEGRA_MAX_SLEW] = {
	[TEGRA_SLEW_FASTEST] = "FASTEST",
	[TEGRA_SLEW_FAST] = "FAST",
	[TEGRA_SLEW_SLOW] = "SLOW",
	[TEGRA_SLEW_SLOWEST] = "SLOWEST",
};

static const char *tri_name(unsigned long val)
{
	return val ? "TRISTATE" : "NORMAL";
}

static const char *pupd_name(unsigned long val)
{
	switch (val) {
	case 0:
		return "NORMAL";

	case 1:
		return "PULL_DOWN";

	case 2:
		return "PULL_UP";

	default:
		return "RSVD";
	}
}

static const char *drive_name(unsigned long val)
{
	if (val >= TEGRA_MAX_DRIVE)
		return "<UNKNOWN>";

	return tegra_drive_names[val];
} 

static const char *slew_name(unsigned long val)
{
	if (val >= TEGRA_MAX_SLEW)
		return "<UNKNOWN>";

	return tegra_slew_names[val];
} 


static void dbg_pad_field(struct seq_file *s, int len)
{
	seq_putc(s, ',');

	while (len-- > -1)
		seq_putc(s, ' ');
}

static int dbg_pinmux_show(struct seq_file *s, void *unused)
{
	int i;
	int len;

	for (i = 0; i < TEGRA_MAX_PINGROUP; i++) {
		unsigned long tri;
		unsigned long mux;
		unsigned long pupd;

		seq_printf(s, "\t{TEGRA_PINGROUP_%s", pingroups[i].name);
		len = strlen(pingroups[i].name);
		dbg_pad_field(s, 5 - len);

		if (pingroups[i].mux_reg < 0) {
			seq_printf(s, "TEGRA_MUX_NONE");
			len = strlen("NONE");
		} else {
			mux = (pg_readl(pingroups[i].mux_reg) >>
			       pingroups[i].mux_bit) & 0x3;
			if (pingroups[i].funcs[mux] == TEGRA_MUX_RSVD) {
				seq_printf(s, "TEGRA_MUX_RSVD%1lu", mux+1);
				len = 5;
			} else {
				seq_printf(s, "TEGRA_MUX_%s",
					   tegra_mux_names[pingroups[i].funcs[mux]]);
				len = strlen(tegra_mux_names[pingroups[i].funcs[mux]]);
			}
		}
		dbg_pad_field(s, 13-len);

		if (pingroups[i].pupd_reg < 0) {
			seq_printf(s, "TEGRA_PUPD_NORMAL");
			len = strlen("NORMAL");
		} else {
			pupd = (pg_readl(pingroups[i].pupd_reg) >>
				pingroups[i].pupd_bit) & 0x3;
			seq_printf(s, "TEGRA_PUPD_%s", pupd_name(pupd));
			len = strlen(pupd_name(pupd));
		}
		dbg_pad_field(s, 9 - len);

		if (pingroups[i].tri_reg < 0) {
			seq_printf(s, "TEGRA_TRI_NORMAL");
		} else {
			tri = (pg_readl(pingroups[i].tri_reg) >>
			       pingroups[i].tri_bit) & 0x1;

			seq_printf(s, "TEGRA_TRI_%s", tri_name(tri));
		}
		seq_printf(s, "},\n");
	}
	return 0;
}

static int dbg_pinmux_open(struct inode *inode, struct file *file)
{
	return single_open(file, dbg_pinmux_show, &inode->i_private);
}

static const struct file_operations debug_fops = {
	.open		= dbg_pinmux_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int dbg_drive_pinmux_show(struct seq_file *s, void *unused)
{
	int i;
	int len;

	for (i = 0; i < TEGRA_MAX_DRIVE_PINGROUP; i++) {
		u32 reg;

		seq_printf(s, "\t{TEGRA_DRIVE_PINGROUP_%s",
			drive_pingroups[i].name);
		len = strlen(drive_pingroups[i].name);
		dbg_pad_field(s, 7 - len);


		reg = pg_readl(drive_pingroups[i].reg);
		if (HSM_EN(reg)) {
			seq_printf(s, "TEGRA_HSM_ENABLE");
			len = 16;
		} else {
			seq_printf(s, "TEGRA_HSM_DISABLE");
			len = 17;
		}
		dbg_pad_field(s, 17 - len);

		if (SCHMT_EN(reg)) {
			seq_printf(s, "TEGRA_SCHMITT_ENABLE");
			len = 21;
		} else {
			seq_printf(s, "TEGRA_SCHMITT_DISABLE");
			len = 22;
		}
		dbg_pad_field(s, 22 - len);

		seq_printf(s, "TEGRA_DRIVE_%s", drive_name(LPMD(reg)));
		len = strlen(drive_name(LPMD(reg)));
		dbg_pad_field(s, 5 - len);

		seq_printf(s, "TEGRA_PULL_%d", DRVDN(reg));
		len = DRVDN(reg) < 10 ? 1 : 2;
		dbg_pad_field(s, 2 - len);

		seq_printf(s, "TEGRA_PULL_%d", DRVUP(reg));
		len = DRVUP(reg) < 10 ? 1 : 2;
		dbg_pad_field(s, 2 - len);

		seq_printf(s, "TEGRA_SLEW_%s", slew_name(SLWR(reg)));
		len = strlen(slew_name(SLWR(reg)));
		dbg_pad_field(s, 7 - len);

		seq_printf(s, "TEGRA_SLEW_%s", slew_name(SLWF(reg)));

		seq_printf(s, "},\n");
	}
	return 0;
}

static int dbg_drive_pinmux_open(struct inode *inode, struct file *file)
{
	return single_open(file, dbg_drive_pinmux_show, &inode->i_private);
}

static const struct file_operations debug_drive_fops = {
	.open		= dbg_drive_pinmux_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};


/// Tegra2 emc


#define TEGRA_MRR_DIVLD        (1<<20)
#define TEGRA_EMC_STATUS       0x02b4
#define TEGRA_EMC_MRR          0x00ec
 
static const struct {
	unsigned long addr;
	const char* name;
} emc_reg_addr[] = {	
	{0x2c,	"RC"},
	{0x30,	"RFC"},
	{0x34,	"RAS"},
	{0x38,	"RP"},
	{0x3c,	"R2W"},
	{0x40,	"W2R"},
	{0x44,	"R2P"},
	{0x48,	"W2P"},
	{0x4c,	"RD_RCD"},
	{0x50,	"WR_RCD"},
	{0x54,	"RRD"},
	{0x58,	"REXT"},
	{0x5c,	"WDV"},
	{0x60,	"QUSE"},
	{0x64,	"QRST"},
	{0x68,	"QSAFE"},
	{0x6c,	"RDV"},
	{0x70,	"REFRESH"},
	{0x74,	"BURST_REFRESH_NUM"},
	{0x78,	"PDEX2WR"},
	{0x7c,	"PDEX2RD"},
	{0x80,	"PCHG2PDEN"},
	{0x84,	"ACT2PDEN"},
	{0x88,	"AR2PDEN"},
	{0x8c,	"RW2PDEN"},
	{0x90,	"TXSR"},
	{0x94,	"TCKE"},
	{0x98,	"TFAW"},
	{0x9c,	"TRPAB"},
	{0xa0,	"TCLKSTABLE"},
	{0xa4,	"TCLKSTOP"},
	{0xa8,	"TREFBW"},
	{0xac,	"QUSE_EXTRA"},
	{0x114,	"FBIO_CFG6"},
	{0xb0,	"ODT_WRITE"},
	{0xb4,	"ODT_READ"},
	{0x104,	"FBIO_CFG5"},
	{0x2bc,	"CFG_DIG_DLL"},
	{0x2c0,	"DLL_XFORM_DQS"},
	{0x2c4,	"DLL_XFORM_QUSE"},
	{0x2e0,	"ZCAL_REF_CNT"},
	{0x2e4,	"ZCAL_WAIT_CNT"},
	{0x2a8,	"AUTO_CAL_INTERVAL"},
	{0x2d0,	"CFG_CLKTRIM_0"},
	{0x2d4,	"CFG_CLKTRIM_1"},
	{0x2d8,	"CFG_CLKTRIM_2"}
};

#if 0
/* read LPDDR2 memory modes */
static u32 tegra_emc_read_mrr(unsigned long addr)
{
	u32 value;
	int count = 100;

	do {
		emc_pg_readl(TEGRA_EMC_MRR);
	} while (--count && (emc_pg_readl(TEGRA_EMC_STATUS) & TEGRA_MRR_DIVLD));
	if (count == 0) {
		pr_err("ERROR: Failed to read memory type\n");
		return 0;
	}
	value = (1 << 30) | (addr << 16);
	emc_pg_write(value, TEGRA_EMC_MRR);

	count = 100;
	while (--count && !(emc_pg_readl(TEGRA_EMC_STATUS) & TEGRA_MRR_DIVLD));
	if (count == 0) {
		pr_err("ERROR: Failed to read memory type\n");
		return 0;
	}
	value = emc_pg_readl(TEGRA_EMC_MRR) & 0xFFFF;

	return value;
}  
#endif

static int dbg_emc_show(struct seq_file *s, void *unused)
{
	int i;
	
#if 0
	/* Tegra2 locks up if we try to access this */ 
	unsigned long vid;
	unsigned long rev_id1;
	unsigned long rev_id2;
	unsigned long pid;

	/* Read memory information */
	vid = tegra_emc_read_mrr(5);
	seq_printf(s,"/* Memory vid     = 0x%04lx */", vid);
	rev_id1 = tegra_emc_read_mrr(6);
	seq_printf(s,"/* Memory rev_id1 = 0x%04lx */", rev_id1);
	rev_id2 = tegra_emc_read_mrr(7);
	seq_printf(s,"/* Memory rev_id2 = 0x%04lx */", rev_id2);
	pid = tegra_emc_read_mrr(8); 	
	seq_printf(s,"/* Memory pid     = 0x%04lx */", pid);  
#endif

	for (i = 0; i < sizeof(emc_reg_addr)/sizeof(emc_reg_addr[0]); i++) {
		u32 reg    = emc_pg_readl(emc_reg_addr[i].addr);
		seq_printf(s, "\t0x%08x, /*%s*/\n",reg,emc_reg_addr[i].name);
	}

	return 0;
}

static int dbg_emc_open(struct inode *inode, struct file *file)
{
	return single_open(file, dbg_emc_show, &inode->i_private);
}

static const struct file_operations debug_emc_fops = {
	.open		= dbg_emc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

/// Tegra2 clocks

#define FUSE_UID_LOW		0x108
#define FUSE_UID_HIGH		0x10c
#define FUSE_SKU_INFO		0x110
#define FUSE_SPARE_BIT		0x200 


#define SKU_ID_T20	8
#define SKU_ID_T25SE	20
#define SKU_ID_AP25	23
#define SKU_ID_T25	24
#define SKU_ID_AP25E	27
#define SKU_ID_T25E	28  

static int tegra_sku_id;
 
static void tegra_init_fuse(void)
{
	u32 reg = readl(reg_clk_base + 0x48);
	reg |= 1 << 28;
	writel(reg, reg_clk_base + 0x48);

	tegra_sku_id = tegra_fuse_readl(FUSE_SKU_INFO) & 0xff;
} 


static int tegra_pinmux_get_func(enum tegra_pingroup pg,
	enum tegra_mux_func *func)
{
	int mux;
	unsigned long reg;

	if (pg < 0 || pg >=  TEGRA_MAX_PINGROUP)
		return -ERANGE;

	if (pingroups[pg].mux_reg < 0)
		return -EINVAL;

	reg = pg_readl(pingroups[pg].mux_reg);
	mux = (reg >> pingroups[pg].mux_bit) & 3;
	*func = pingroups[pg].funcs[mux];

	return 0;
} 

#define DIV_BUS			(1 << 0)
#define DIV_U71			(1 << 1)
#define DIV_U71_FIXED		(1 << 2)
#define DIV_2			(1 << 3)
#define DIV_U16			(1 << 4)
#define PLL_FIXED		(1 << 5)
#define PLL_HAS_CPCON		(1 << 6)
#define MUX			(1 << 7)
#define PLLD			(1 << 8)
#define PERIPH_NO_RESET		(1 << 9)
#define PERIPH_NO_ENB		(1 << 10)
#define PERIPH_EMC_ENB		(1 << 11)
#define PERIPH_MANUAL_RESET	(1 << 12)
#define PLL_ALT_MISC_REG	(1 << 13)
#define PLLU			(1 << 14)
#define PERIPH_SOURCE_CLK_4BIT	(1 << 15)
#define ENABLE_ON_INIT		(1 << 28)

struct clk;

struct clk_mux_sel {
	struct clk	*input;
	u32		value;
};

struct clk_pll_freq_table {
	unsigned long	input_rate;
	unsigned long	output_rate;
	u16		n;
	u16		m;
	u8		p;
	u8		cpcon;
};

struct clk_ops {
	void		(*init)(struct clk *);
};

enum clk_state {
	UNINITIALIZED = 0,
	ON,
	OFF,
};

struct clk {
	/* node for master clocks list */
	struct list_head	node;		/* node for list of all clocks */

	bool			set;
	struct clk_ops		*ops;
	unsigned long		rate;
	unsigned long		max_rate;
	unsigned long		min_rate;
	u32			flags;
	const char		*name;

	enum clk_state		state;
	struct clk		*parent;
	u32			div;
	u32			mul;

	const struct clk_mux_sel	*inputs;
	u32				reg;
	u32				reg_shift;

	struct list_head		shared_bus_list;

	union {
		struct {
			unsigned int			clk_num;
		} periph;
		struct {
			unsigned long			input_min;
			unsigned long			input_max;
			unsigned long			cf_min;
			unsigned long			cf_max;
			unsigned long			vco_min;
			unsigned long			vco_max;
			const struct clk_pll_freq_table	*freq_table;
			int				lock_delay;
		} pll;
		struct {
			u32				sel;
			u32				reg_mask;
		} mux;
		struct {
			struct clk			*main;
			struct clk			*backup;
		} cpu;
		struct {
			struct list_head		node;
			bool				enabled;
			unsigned long			rate;
		} shared_bus_user;
	} u;

};

static LIST_HEAD(clocks);

static struct clk *tegra_get_clock_by_name(const char *name)
{
	struct clk *c;
	struct clk *ret = NULL;
	list_for_each_entry(c, &clocks, node) {
		if (strcmp(c->name, name) == 0) {
			ret = c;
			break;
		}
	}
	return ret;
}

static unsigned long clk_get_rate(struct clk *c);

static unsigned long clk_predict_rate_from_parent(struct clk *c, struct clk *p)
{
	u64 rate;

	rate = clk_get_rate(p);

	if (c->mul != 0 && c->div != 0) {
		rate *= c->mul;
		rate += c->div / 2; /* round up */
		do_div(rate, c->div);
	}

	return rate;
}

static unsigned long clk_get_rate_locked(struct clk *c)
{
	unsigned long rate;

	if (c->parent)
		rate = clk_predict_rate_from_parent(c, c->parent);
	else
		rate = c->rate;

	return rate;
}

static unsigned long clk_get_rate(struct clk *c)
{
	unsigned long rate;

	rate = clk_get_rate_locked(c);

	return rate;
}

static void clk_init(struct clk *c)
{

	if (c->ops && c->ops->init)
		c->ops->init(c);

	if (!c->ops) {
		c->set = true;
		if (c->parent)
			c->state = c->parent->state;
		else
			c->state = ON;
	}

	list_add(&c->node, &clocks);
}


static unsigned long clk_get_rate_all_locked(struct clk *c)
{
	u64 rate;
	int mul = 1;
	int div = 1;
	struct clk *p = c;

	while (p) {
		c = p;
		if (c->mul != 0 && c->div != 0) {
			mul *= c->mul;
			div *= c->div;
		}
		p = c->parent;
	}

	rate = c->rate;
	rate *= mul;
	do_div(rate, div);

	return rate;
}


#define RST_DEVICES			0x004
#define RST_DEVICES_SET			0x300
#define RST_DEVICES_CLR			0x304
#define RST_DEVICES_NUM			3

#define CLK_OUT_ENB			0x010
#define CLK_OUT_ENB_SET			0x320
#define CLK_OUT_ENB_CLR			0x324
#define CLK_OUT_ENB_NUM			3

#define CLK_MASK_ARM			0x44
#define MISC_CLK_ENB			0x48

#define OSC_CTRL			0x50
#define OSC_CTRL_OSC_FREQ_MASK		(3<<30)
#define OSC_CTRL_OSC_FREQ_13MHZ		(0<<30)
#define OSC_CTRL_OSC_FREQ_19_2MHZ	(1<<30)
#define OSC_CTRL_OSC_FREQ_12MHZ		(2<<30)
#define OSC_CTRL_OSC_FREQ_26MHZ		(3<<30)
#define OSC_CTRL_MASK			(0x3f2 | OSC_CTRL_OSC_FREQ_MASK)

#define OSC_FREQ_DET			0x58
#define OSC_FREQ_DET_TRIG		(1<<31)

#define OSC_FREQ_DET_STATUS		0x5C
#define OSC_FREQ_DET_BUSY		(1<<31)
#define OSC_FREQ_DET_CNT_MASK		0xFFFF

#define PERIPH_CLK_SOURCE_I2S1		0x100
#define PERIPH_CLK_SOURCE_EMC		0x19c
#define PERIPH_CLK_SOURCE_OSC		0x1fc
#define PERIPH_CLK_SOURCE_NUM \
	((PERIPH_CLK_SOURCE_OSC - PERIPH_CLK_SOURCE_I2S1) / 4)

#define PERIPH_CLK_SOURCE_MASK		(3<<30)
#define PERIPH_CLK_SOURCE_SHIFT		30
#define PERIPH_CLK_SOURCE_4BIT_MASK	(7<<28)
#define PERIPH_CLK_SOURCE_4BIT_SHIFT	28
#define PERIPH_CLK_SOURCE_ENABLE	(1<<28)
#define PERIPH_CLK_SOURCE_DIVU71_MASK	0xFF
#define PERIPH_CLK_SOURCE_DIVU16_MASK	0xFFFF
#define PERIPH_CLK_SOURCE_DIV_SHIFT	0

#define SDMMC_CLK_INT_FB_SEL		(1 << 23)
#define SDMMC_CLK_INT_FB_DLY_SHIFT	16
#define SDMMC_CLK_INT_FB_DLY_MASK	(0xF << SDMMC_CLK_INT_FB_DLY_SHIFT)

#define PLL_BASE			0x0
#define PLL_BASE_BYPASS			(1<<31)
#define PLL_BASE_ENABLE			(1<<30)
#define PLL_BASE_REF_ENABLE		(1<<29)
#define PLL_BASE_OVERRIDE		(1<<28)
#define PLL_BASE_DIVP_MASK		(0x7<<20)
#define PLL_BASE_DIVP_SHIFT		20
#define PLL_BASE_DIVN_MASK		(0x3FF<<8)
#define PLL_BASE_DIVN_SHIFT		8
#define PLL_BASE_DIVM_MASK		(0x1F)
#define PLL_BASE_DIVM_SHIFT		0

#define PLL_OUT_RATIO_MASK		(0xFF<<8)
#define PLL_OUT_RATIO_SHIFT		8
#define PLL_OUT_OVERRIDE		(1<<2)
#define PLL_OUT_CLKEN			(1<<1)
#define PLL_OUT_RESET_DISABLE		(1<<0)

#define PLL_MISC(c)			(((c)->flags & PLL_ALT_MISC_REG) ? 0x4 : 0xc)

#define PLL_MISC_DCCON_SHIFT		20
#define PLL_MISC_CPCON_SHIFT		8
#define PLL_MISC_CPCON_MASK		(0xF<<PLL_MISC_CPCON_SHIFT)
#define PLL_MISC_LFCON_SHIFT		4
#define PLL_MISC_LFCON_MASK		(0xF<<PLL_MISC_LFCON_SHIFT)
#define PLL_MISC_VCOCON_SHIFT		0
#define PLL_MISC_VCOCON_MASK		(0xF<<PLL_MISC_VCOCON_SHIFT)

#define PLLU_BASE_POST_DIV		(1<<20)

#define PLLD_MISC_CLKENABLE		(1<<30)
#define PLLD_MISC_DIV_RST		(1<<23)
#define PLLD_MISC_DCCON_SHIFT		12

#define PLLE_MISC_READY			(1 << 15)

#define PERIPH_CLK_TO_ENB_REG(c)	((c->u.periph.clk_num / 32) * 4)
#define PERIPH_CLK_TO_ENB_SET_REG(c)	((c->u.periph.clk_num / 32) * 8)
#define PERIPH_CLK_TO_ENB_BIT(c)	(1 << (c->u.periph.clk_num % 32))

#define SUPER_CLK_MUX			0x00
#define SUPER_STATE_SHIFT		28
#define SUPER_STATE_MASK		(0xF << SUPER_STATE_SHIFT)
#define SUPER_STATE_STANDBY		(0x0 << SUPER_STATE_SHIFT)
#define SUPER_STATE_IDLE		(0x1 << SUPER_STATE_SHIFT)
#define SUPER_STATE_RUN			(0x2 << SUPER_STATE_SHIFT)
#define SUPER_STATE_IRQ			(0x3 << SUPER_STATE_SHIFT)
#define SUPER_STATE_FIQ			(0x4 << SUPER_STATE_SHIFT)
#define SUPER_SOURCE_MASK		0xF
#define	SUPER_FIQ_SOURCE_SHIFT		12
#define	SUPER_IRQ_SOURCE_SHIFT		8
#define	SUPER_RUN_SOURCE_SHIFT		4
#define	SUPER_IDLE_SOURCE_SHIFT		0

#define SUPER_CLK_DIVIDER		0x04

#define BUS_CLK_DISABLE			(1<<3)
#define BUS_CLK_DIV_MASK		0x3

#define PMC_CTRL			0x0
# define PMC_CTRL_BLINK_ENB		(1 << 7)

#define PMC_DPD_PADS_ORIDE		0x1c
# define PMC_DPD_PADS_ORIDE_BLINK_ENB	(1 << 20)

#define PMC_BLINK_TIMER_DATA_ON_SHIFT	0
#define PMC_BLINK_TIMER_DATA_ON_MASK	0x7fff
#define PMC_BLINK_TIMER_ENB		(1 << 15)
#define PMC_BLINK_TIMER_DATA_OFF_SHIFT	16
#define PMC_BLINK_TIMER_DATA_OFF_MASK	0xffff


unsigned long clk_measure_input_freq(void)
{
	u32 clock_autodetect;
	clk_writel(OSC_FREQ_DET_TRIG | 1, OSC_FREQ_DET);
	do {} while (clk_readl(OSC_FREQ_DET_STATUS) & OSC_FREQ_DET_BUSY);
	clock_autodetect = clk_readl(OSC_FREQ_DET_STATUS);
	if (clock_autodetect >= 732 - 3 && clock_autodetect <= 732 + 3) {
		return 12000000;
	} else if (clock_autodetect >= 794 - 3 && clock_autodetect <= 794 + 3) {
		return 13000000;
	} else if (clock_autodetect >= 1172 - 3 && clock_autodetect <= 1172 + 3) {
		return 19200000;
	} else if (clock_autodetect >= 1587 - 3 && clock_autodetect <= 1587 + 3) {
		return 26000000;
	} else {
		pr_err("%s: Unexpected clock autodetect value %d", __func__, clock_autodetect);
		return 0;
	}
}

/* clk_m functions */
static unsigned long tegra2_clk_m_autodetect_rate(struct clk *c)
{
	u32 auto_clock_control = clk_readl(OSC_CTRL) & ~OSC_CTRL_OSC_FREQ_MASK;

	c->rate = clk_measure_input_freq();
	switch (c->rate) {
	case 12000000:
		auto_clock_control |= OSC_CTRL_OSC_FREQ_12MHZ;
		break;
	case 13000000:
		auto_clock_control |= OSC_CTRL_OSC_FREQ_13MHZ;
		break;
	case 19200000:
		auto_clock_control |= OSC_CTRL_OSC_FREQ_19_2MHZ;
		break;
	case 26000000:
		auto_clock_control |= OSC_CTRL_OSC_FREQ_26MHZ;
		break;
	default:
		pr_err("%s: Unexpected clock rate %ld", __func__, c->rate);
		return 1;
	}
	clk_writel(auto_clock_control, OSC_CTRL);
	return c->rate;
}

static void tegra2_clk_m_init(struct clk *c)
{
	pr_debug("%s on clock %s\n", __func__, c->name);
	tegra2_clk_m_autodetect_rate(c);
}

static struct clk_ops tegra_clk_m_ops = {
	.init		= tegra2_clk_m_init,
};


/* super clock functions */
/* "super clocks" on tegra have two-stage muxes and a clock skipping
 * super divider.  We will ignore the clock skipping divider, since we
 * can't lower the voltage when using the clock skip, but we can if we
 * lower the PLL frequency.
 */
static void tegra2_super_clk_init(struct clk *c)
{
	u32 val;
	int source;
	int shift;
	const struct clk_mux_sel *sel;
	val = clk_readl(c->reg + SUPER_CLK_MUX);
	c->state = ON;
	shift = ((val & SUPER_STATE_MASK) == SUPER_STATE_IDLE) ?
		SUPER_IDLE_SOURCE_SHIFT : SUPER_RUN_SOURCE_SHIFT;
	source = (val >> shift) & SUPER_SOURCE_MASK;
	for (sel = c->inputs; sel->input != NULL; sel++) {
		if (sel->value == source)
			break;
	}
	c->parent = sel->input;
}

/*
 * Super clocks have "clock skippers" instead of dividers.  Dividing using
 * a clock skipper does not allow the voltage to be scaled down, so instead
 * adjust the rate of the parent clock.  This requires that the parent of a
 * super clock have no other children, otherwise the rate will change
 * underneath the other children.
 */

static struct clk_ops tegra_super_ops = {
	.init			= tegra2_super_clk_init,
};

/* virtual cpu clock functions */
/* some clocks can not be stopped (cpu, memory bus) while the SoC is running.
   To change the frequency of these clocks, the parent pll may need to be
   reprogrammed, so the clock must be moved off the pll, the pll reprogrammed,
   and then the clock moved back to the pll.  To hide this sequence, a virtual
   clock handles it.
 */
static void tegra2_cpu_clk_init(struct clk *c)
{
}


static struct clk_ops tegra_cpu_ops = {
	.init     = tegra2_cpu_clk_init,
};

static struct clk_ops tegra_cop_ops = {
};

/* bus clock functions */
static void tegra2_bus_clk_init(struct clk *c)
{
	u32 val = clk_readl(c->reg);
	c->state = ((val >> c->reg_shift) & BUS_CLK_DISABLE) ? OFF : ON;
	c->div = ((val >> c->reg_shift) & BUS_CLK_DIV_MASK) + 1;
	c->mul = 1;
}


static struct clk_ops tegra_bus_ops = {
	.init			= tegra2_bus_clk_init,
};

/* Blink output functions */
static void tegra2_blink_clk_init(struct clk *c)
{
	u32 val;

	val = pmc_readl(PMC_CTRL);
	c->state = (val & PMC_CTRL_BLINK_ENB) ? ON : OFF;
	c->mul = 1;
	val = pmc_readl(c->reg);

	if (val & PMC_BLINK_TIMER_ENB) {
		unsigned int on_off;

		on_off = (val >> PMC_BLINK_TIMER_DATA_ON_SHIFT) &
			PMC_BLINK_TIMER_DATA_ON_MASK;
		val >>= PMC_BLINK_TIMER_DATA_OFF_SHIFT;
		val &= PMC_BLINK_TIMER_DATA_OFF_MASK;
		on_off += val;
		/* each tick in the blink timer is 4 32KHz clocks */
		c->div = on_off * 4;
	} else {
		c->div = 1;
	}
}

static struct clk_ops tegra_blink_clk_ops = {
	.init			= &tegra2_blink_clk_init,
};


static void tegra2_pll_clk_init(struct clk *c)
{
	u32 val = clk_readl(c->reg + PLL_BASE);

	c->state = (val & PLL_BASE_ENABLE) ? ON : OFF;

	if (c->flags & PLL_FIXED && !(val & PLL_BASE_OVERRIDE)) {
		pr_warning("Clock %s has unknown fixed frequency\n", c->name);
		c->mul = 1;
		c->div = 1;
	} else if (val & PLL_BASE_BYPASS) {
		c->mul = 1;
		c->div = 1;
	} else {
		c->mul = (val & PLL_BASE_DIVN_MASK) >> PLL_BASE_DIVN_SHIFT;
		c->div = (val & PLL_BASE_DIVM_MASK) >> PLL_BASE_DIVM_SHIFT;
		if (c->flags & PLLU)
			c->div *= (val & PLLU_BASE_POST_DIV) ? 1 : 2;
		else
			c->div *= (val & PLL_BASE_DIVP_MASK) ? 2 : 1;
	}
}

static struct clk_ops tegra_pll_ops = {
	.init			= tegra2_pll_clk_init,
};

static void tegra2_pllx_clk_init(struct clk *c)
{
	tegra2_pll_clk_init(c);

	if (tegra_sku_id == 7) {
		c->max_rate = 750000000;
	} else if (tegra_sku_id == SKU_ID_T20) {
		/* make adjustment for T20 */
		/* the default max_rate is set at 1.2GHz for T25 */
		c->max_rate = 1000000000;
	}
}

static struct clk_ops tegra_pllx_ops = {
	.init     = tegra2_pllx_clk_init,
};

static struct clk_ops tegra_plle_ops = {
	.init       = tegra2_pll_clk_init,
};

/* Clock divider ops */
static void tegra2_pll_div_clk_init(struct clk *c)
{
	u32 val = clk_readl(c->reg);
	u32 divu71;
	val >>= c->reg_shift;
	c->state = (val & PLL_OUT_CLKEN) ? ON : OFF;
	if (!(val & PLL_OUT_RESET_DISABLE))
		c->state = OFF;

	if (c->flags & DIV_U71) {
		divu71 = (val & PLL_OUT_RATIO_MASK) >> PLL_OUT_RATIO_SHIFT;
		c->div = (divu71 + 2);
		c->mul = 2;
	} else if (c->flags & DIV_2) {
		c->div = 2;
		c->mul = 1;
	} else {
		c->div = 1;
		c->mul = 1;
	}
}


static struct clk_ops tegra_pll_div_ops = {
	.init			= tegra2_pll_div_clk_init,
};

/* Periph clk ops */

static void tegra2_periph_clk_init(struct clk *c)
{
	u32 val = clk_readl(c->reg);
	const struct clk_mux_sel *mux = 0;
	const struct clk_mux_sel *sel;
	u32 shift;

	if (c->flags & PERIPH_SOURCE_CLK_4BIT)
		shift = PERIPH_CLK_SOURCE_4BIT_SHIFT;
	else
		shift = PERIPH_CLK_SOURCE_SHIFT;

	if (c->flags & MUX) {
		for (sel = c->inputs; sel->input != NULL; sel++) {
			if (val >> shift == sel->value)
				mux = sel;
		}

		c->parent = mux->input;
	} else {
		c->parent = c->inputs[0].input;
	}

	if (c->flags & DIV_U71) {
		u32 divu71 = val & PERIPH_CLK_SOURCE_DIVU71_MASK;
		c->div = divu71 + 2;
		c->mul = 2;
	} else if (c->flags & DIV_U16) {
		u32 divu16 = val & PERIPH_CLK_SOURCE_DIVU16_MASK;
		c->div = divu16 + 1;
		c->mul = 1;
	} else {
		c->div = 1;
		c->mul = 1;
	}

	c->state = ON;

	if (!c->u.periph.clk_num)
		return;

	if (!(clk_readl(CLK_OUT_ENB + PERIPH_CLK_TO_ENB_REG(c)) &
			PERIPH_CLK_TO_ENB_BIT(c)))
		c->state = OFF;

	if (!(c->flags & PERIPH_NO_RESET))
		if (clk_readl(RST_DEVICES + PERIPH_CLK_TO_ENB_REG(c)) &
				PERIPH_CLK_TO_ENB_BIT(c))
			c->state = OFF;
}

static struct clk_ops tegra_periph_clk_ops = {
	.init			= &tegra2_periph_clk_init,
};


/* External memory controller clock ops */
static void tegra2_emc_clk_init(struct clk *c)
{
	tegra2_periph_clk_init(c);
	c->max_rate = clk_get_rate_locked(c);
}

static struct clk_ops tegra_emc_clk_ops = {
	.init			= &tegra2_emc_clk_init,
};

/* Clock doubler ops */
static void tegra2_clk_double_init(struct clk *c)
{
	c->mul = 2;
	c->div = 1;
	c->state = ON;

	if (!c->u.periph.clk_num)
		return;

	if (!(clk_readl(CLK_OUT_ENB + PERIPH_CLK_TO_ENB_REG(c)) &
			PERIPH_CLK_TO_ENB_BIT(c)))
		c->state = OFF;
};

static struct clk_ops tegra_clk_double_ops = {
	.init			= &tegra2_clk_double_init,
};

/* Audio sync clock ops */
static void tegra2_audio_sync_clk_init(struct clk *c)
{
	int source;
	const struct clk_mux_sel *sel;
	u32 val = clk_readl(c->reg);
	c->state = (val & (1<<4)) ? OFF : ON;
	source = val & 0xf;
	for (sel = c->inputs; sel->input != NULL; sel++)
		if (sel->value == source)
			break;
	c->parent = sel->input;
}

static struct clk_ops tegra_audio_sync_clk_ops = {
	.init       = tegra2_audio_sync_clk_init,
};

/* cdev1 and cdev2 (dap_mclk1 and dap_mclk2) ops */
static void tegra2_cdev_clk_init(struct clk *c)
{
	int ret;
	enum tegra_mux_func func;
	const struct clk_mux_sel *sel;

	c->state = ON;

	if (!(clk_readl(CLK_OUT_ENB + PERIPH_CLK_TO_ENB_REG(c)) &
			PERIPH_CLK_TO_ENB_BIT(c)))
		c->state = OFF;

	ret = tegra_pinmux_get_func(c->reg, &func);
	if (ret)
		func = -1;

	for (sel = c->inputs; sel->input != NULL; sel++) {
		if (sel->value == func) {
			c->parent = sel->input;
			break;
		}
	}

	if (!c->parent)
		c->state = OFF;
}


static struct clk_ops tegra_cdev_clk_ops = {
	.init			= &tegra2_cdev_clk_init,
};

/* shared bus ops */
static void tegra_clk_shared_bus_init(struct clk *c)
{

	c->max_rate = c->parent->max_rate;
	c->u.shared_bus_user.rate = c->parent->max_rate;
	c->state = OFF;
	c->set = true;

	list_add_tail(&c->u.shared_bus_user.node,
		&c->parent->shared_bus_list);

}

static struct clk_ops tegra_clk_shared_bus_ops = {
	.init = tegra_clk_shared_bus_init,
};


/* Clock definitions */
static struct clk tegra_clk_32k = {
	.name = "clk_32k",
	.rate = 32768,
	.ops  = NULL,
	.max_rate = 32768,
};

static struct clk_pll_freq_table tegra_pll_s_freq_table[] = {
	{32768, 12000000, 366, 1, 1, 0},
	{32768, 13000000, 397, 1, 1, 0},
	{32768, 19200000, 586, 1, 1, 0},
	{32768, 26000000, 793, 1, 1, 0},
	{0, 0, 0, 0, 0, 0},
};

static struct clk tegra_pll_s = {
	.name      = "pll_s",
	.flags     = PLL_ALT_MISC_REG,
	.ops       = &tegra_pll_ops,
	.parent    = &tegra_clk_32k,
	.max_rate  = 26000000,
	.reg       = 0xf0,
	.u.pll = {
		.input_min = 32768,
		.input_max = 32768,
		.cf_min    = 0, /* FIXME */
		.cf_max    = 0, /* FIXME */
		.vco_min   = 12000000,
		.vco_max   = 26000000,
		.freq_table = tegra_pll_s_freq_table,
		.lock_delay = 300,
	},
};

static struct clk_mux_sel tegra_clk_m_sel[] = {
	{ .input = &tegra_clk_32k, .value = 0},
	{ .input = &tegra_pll_s,  .value = 1},
	{ 0, 0},
};

static struct clk tegra_clk_m = {
	.name      = "clk_m",
	.flags     = ENABLE_ON_INIT,
	.ops       = &tegra_clk_m_ops,
	.inputs    = tegra_clk_m_sel,
	.reg       = 0x1fc,
	.reg_shift = 28,
	.max_rate  = 26000000,
};

static struct clk_pll_freq_table tegra_pll_c_freq_table[] = {
	{ 12000000, 600000000, 600, 12, 1, 8 },
	{ 13000000, 600000000, 600, 13, 1, 8 },
	{ 19200000, 600000000, 500, 16, 1, 6 },
	{ 26000000, 600000000, 600, 26, 1, 8 },
	{ 0, 0, 0, 0, 0, 0 },
};

static struct clk tegra_pll_c = {
	.name      = "pll_c",
	.flags	   = PLL_HAS_CPCON,
	.ops       = &tegra_pll_ops,
	.reg       = 0x80,
	.parent    = &tegra_clk_m,
	.max_rate  = 600000000,
	.u.pll = {
		.input_min = 2000000,
		.input_max = 31000000,
		.cf_min    = 1000000,
		.cf_max    = 6000000,
		.vco_min   = 20000000,
		.vco_max   = 1400000000,
		.freq_table = tegra_pll_c_freq_table,
		.lock_delay = 300,
	},
};

static struct clk tegra_pll_c_out1 = {
	.name      = "pll_c_out1",
	.ops       = &tegra_pll_div_ops,
	.flags     = DIV_U71,
	.parent    = &tegra_pll_c,
	.reg       = 0x84,
	.reg_shift = 0,
	.max_rate  = 600000000,
};

static struct clk_pll_freq_table tegra_pll_m_freq_table[] = {
	{ 12000000, 666000000, 666, 12, 1, 8},
	{ 13000000, 666000000, 666, 13, 1, 8},
	{ 19200000, 666000000, 555, 16, 1, 8},
	{ 26000000, 666000000, 666, 26, 1, 8},
	{ 12000000, 600000000, 600, 12, 1, 8},
	{ 13000000, 600000000, 600, 13, 1, 8},
	{ 19200000, 600000000, 375, 12, 1, 6},
	{ 26000000, 600000000, 600, 26, 1, 8},
	{ 0, 0, 0, 0, 0, 0 },
};

static struct clk tegra_pll_m = {
	.name      = "pll_m",
	.flags     = PLL_HAS_CPCON,
	.ops       = &tegra_pll_ops,
	.reg       = 0x90,
	.parent    = &tegra_clk_m,
	.max_rate  = 800000000,
	.u.pll = {
		.input_min = 2000000,
		.input_max = 31000000,
		.cf_min    = 1000000,
		.cf_max    = 6000000,
		.vco_min   = 20000000,
		.vco_max   = 1200000000,
		.freq_table = tegra_pll_m_freq_table,
		.lock_delay = 300,
	},
};

static struct clk tegra_pll_m_out1 = {
	.name      = "pll_m_out1",
	.ops       = &tegra_pll_div_ops,
	.flags     = DIV_U71,
	.parent    = &tegra_pll_m,
	.reg       = 0x94,
	.reg_shift = 0,
	.max_rate  = 600000000,
};

static struct clk_pll_freq_table tegra_pll_p_freq_table[] = {
	{ 12000000, 216000000, 432, 12, 2, 8},
	{ 13000000, 216000000, 432, 13, 2, 8},
	{ 19200000, 216000000, 90,   4, 2, 1},
	{ 26000000, 216000000, 432, 26, 2, 8},
	{ 12000000, 432000000, 432, 12, 1, 8},
	{ 13000000, 432000000, 432, 13, 1, 8},
	{ 19200000, 432000000, 90,   4, 1, 1},
	{ 26000000, 432000000, 432, 26, 1, 8},
	{ 0, 0, 0, 0, 0, 0 },
};

static struct clk tegra_pll_p = {
	.name      = "pll_p",
	.flags     = ENABLE_ON_INIT | PLL_FIXED | PLL_HAS_CPCON,
	.ops       = &tegra_pll_ops,
	.reg       = 0xa0,
	.parent    = &tegra_clk_m,
	.max_rate  = 432000000,
	.u.pll = {
		.input_min = 2000000,
		.input_max = 31000000,
		.cf_min    = 1000000,
		.cf_max    = 6000000,
		.vco_min   = 20000000,
		.vco_max   = 1400000000,
		.freq_table = tegra_pll_p_freq_table,
		.lock_delay = 300,
	},
};

static struct clk tegra_pll_p_out1 = {
	.name      = "pll_p_out1",
	.ops       = &tegra_pll_div_ops,
	.flags     = ENABLE_ON_INIT | DIV_U71 | DIV_U71_FIXED,
	.parent    = &tegra_pll_p,
	.reg       = 0xa4,
	.reg_shift = 0,
	.max_rate  = 432000000,
};

static struct clk tegra_pll_p_out2 = {
	.name      = "pll_p_out2",
	.ops       = &tegra_pll_div_ops,
	.flags     = ENABLE_ON_INIT | DIV_U71 | DIV_U71_FIXED,
	.parent    = &tegra_pll_p,
	.reg       = 0xa4,
	.reg_shift = 16,
	.max_rate  = 432000000,
};

static struct clk tegra_pll_p_out3 = {
	.name      = "pll_p_out3",
	.ops       = &tegra_pll_div_ops,
	.flags     = ENABLE_ON_INIT | DIV_U71 | DIV_U71_FIXED,
	.parent    = &tegra_pll_p,
	.reg       = 0xa8,
	.reg_shift = 0,
	.max_rate  = 432000000,
};

static struct clk tegra_pll_p_out4 = {
	.name      = "pll_p_out4",
	.ops       = &tegra_pll_div_ops,
	.flags     = ENABLE_ON_INIT | DIV_U71 | DIV_U71_FIXED,
	.parent    = &tegra_pll_p,
	.reg       = 0xa8,
	.reg_shift = 16,
	.max_rate  = 432000000,
};

static struct clk_pll_freq_table tegra_pll_a_freq_table[] = {
	{ 28800000, 56448000, 49, 25, 1, 1},
	{ 28800000, 73728000, 64, 25, 1, 1},
	{ 28800000, 24000000,  5,  6, 1, 1},
	{ 0, 0, 0, 0, 0, 0 },
};

static struct clk tegra_pll_a = {
	.name      = "pll_a",
	.flags     = PLL_HAS_CPCON,
	.ops       = &tegra_pll_ops,
	.reg       = 0xb0,
	.parent    = &tegra_pll_p_out1,
	.max_rate  = 73728000,
	.u.pll = {
		.input_min = 2000000,
		.input_max = 31000000,
		.cf_min    = 1000000,
		.cf_max    = 6000000,
		.vco_min   = 20000000,
		.vco_max   = 1400000000,
		.freq_table = tegra_pll_a_freq_table,
		.lock_delay = 300,
	},
};

static struct clk tegra_pll_a_out0 = {
	.name      = "pll_a_out0",
	.ops       = &tegra_pll_div_ops,
	.flags     = DIV_U71,
	.parent    = &tegra_pll_a,
	.reg       = 0xb4,
	.reg_shift = 0,
	.max_rate  = 73728000,
};

static struct clk_pll_freq_table tegra_pll_d_freq_table[] = {
	{ 12000000, 216000000, 216, 12, 1, 4},
	{ 13000000, 216000000, 216, 13, 1, 4},
	{ 19200000, 216000000, 135, 12, 1, 3},
	{ 26000000, 216000000, 216, 26, 1, 4},

	{ 12000000, 252000000, 252, 12, 1, 4},
	{ 13000000, 252000000, 252, 13, 1, 4},
	{ 19200000, 252000000, 210, 16, 1, 3},
	{ 26000000, 252000000, 252, 26, 1, 4},

	{ 12000000, 594000000, 594, 12, 1, 8},
	{ 13000000, 594000000, 594, 13, 1, 8},
	{ 19200000, 594000000, 495, 16, 1, 8},
	{ 26000000, 594000000, 594, 26, 1, 8},

	{ 12000000, 1000000000, 1000, 12, 1, 12},
	{ 13000000, 1000000000, 1000, 13, 1, 12},
	{ 19200000, 1000000000, 625,  12, 1, 8},
	{ 26000000, 1000000000, 1000, 26, 1, 12},

	{ 0, 0, 0, 0, 0, 0 },
};

static struct clk tegra_pll_d = {
	.name      = "pll_d",
	.flags     = PLL_HAS_CPCON | PLLD,
	.ops       = &tegra_pll_ops,
	.reg       = 0xd0,
	.parent    = &tegra_clk_m,
	.max_rate  = 1000000000,
	.u.pll = {
		.input_min = 2000000,
		.input_max = 40000000,
		.cf_min    = 1000000,
		.cf_max    = 6000000,
		.vco_min   = 40000000,
		.vco_max   = 1000000000,
		.freq_table = tegra_pll_d_freq_table,
		.lock_delay = 1000,
	},
};

static struct clk tegra_pll_d_out0 = {
	.name      = "pll_d_out0",
	.ops       = &tegra_pll_div_ops,
	.flags     = DIV_2 | PLLD,
	.parent    = &tegra_pll_d,
	.max_rate  = 500000000,
};

static struct clk_pll_freq_table tegra_pll_u_freq_table[] = {
	{ 12000000, 480000000, 960, 12, 2, 0},
	{ 13000000, 480000000, 960, 13, 2, 0},
	{ 19200000, 480000000, 200, 4,  2, 0},
	{ 26000000, 480000000, 960, 26, 2, 0},
	{ 0, 0, 0, 0, 0, 0 },
};

static struct clk tegra_pll_u = {
	.name      = "pll_u",
	.flags     = PLLU,
	.ops       = &tegra_pll_ops,
	.reg       = 0xc0,
	.parent    = &tegra_clk_m,
	.max_rate  = 480000000,
	.u.pll = {
		.input_min = 2000000,
		.input_max = 40000000,
		.cf_min    = 1000000,
		.cf_max    = 6000000,
		.vco_min   = 480000000,
		.vco_max   = 960000000,
		.freq_table = tegra_pll_u_freq_table,
		.lock_delay = 1000,
	},
};

static struct clk_pll_freq_table tegra_pll_x_freq_table[] = {
	/* 1.2 GHz */
	{ 12000000, 1200000000,  600,  6, 1, 12},
	{ 13000000, 1200000000,  923, 10, 1, 12},
	{ 19200000, 1200000000,  750, 12, 1,  8},
	{ 26000000, 1200000000,  600, 13, 1, 12},

	/* 1 GHz */
	{ 12000000, 1000000000, 1000, 12, 1, 12},
	{ 13000000, 1000000000, 1000, 13, 1, 12},
	{ 19200000, 1000000000, 625,  12, 1, 8},
	{ 26000000, 1000000000, 1000, 26, 1, 12},

	/* 912 MHz */
	{ 12000000, 912000000,  912,  12, 1, 12},
	{ 13000000, 912000000,  912,  13, 1, 12},
	{ 19200000, 912000000,  760,  16, 1, 8},
	{ 26000000, 912000000,  912,  26, 1, 12},

	/* 816 MHz */
	{ 12000000, 816000000,  816,  12, 1, 12},
	{ 13000000, 816000000,  816,  13, 1, 12},
	{ 19200000, 816000000,  680,  16, 1, 8},
	{ 26000000, 816000000,  816,  26, 1, 12},

	/* 760 MHz */
	{ 12000000, 760000000,  760,  12, 1, 12},
	{ 13000000, 760000000,  760,  13, 1, 12},
	{ 19200000, 760000000,  950,  24, 1, 8},
	{ 26000000, 760000000,  760,  26, 1, 12},

	/* 608 MHz */
	{ 12000000, 608000000,  608,  12, 1, 12},
	{ 13000000, 608000000,  608,  13, 1, 12},
	{ 19200000, 608000000,  380,  12, 1, 8},
	{ 26000000, 608000000,  608,  26, 1, 12},

	/* 456 MHz */
	{ 12000000, 456000000,  456,  12, 1, 12},
	{ 13000000, 456000000,  456,  13, 1, 12},
	{ 19200000, 456000000,  380,  16, 1, 8},
	{ 26000000, 456000000,  456,  26, 1, 12},

	/* 312 MHz */
	{ 12000000, 312000000,  312,  12, 1, 12},
	{ 13000000, 312000000,  312,  13, 1, 12},
	{ 19200000, 312000000,  260,  16, 1, 8},
	{ 26000000, 312000000,  312,  26, 1, 12},

	{ 0, 0, 0, 0, 0, 0 },
};

static struct clk tegra_pll_x = {
	.name      = "pll_x",
	.flags     = PLL_HAS_CPCON | PLL_ALT_MISC_REG,
	.ops       = &tegra_pllx_ops,
	.reg       = 0xe0,
	.parent    = &tegra_clk_m,
	.max_rate  = 1200000000,
	.u.pll = {
		.input_min = 2000000,
		.input_max = 31000000,
		.cf_min    = 1000000,
		.cf_max    = 6000000,
		.vco_min   = 20000000,
		.vco_max   = 1200000000,
		.freq_table = tegra_pll_x_freq_table,
		.lock_delay = 300,
	},
};

static struct clk_pll_freq_table tegra_pll_e_freq_table[] = {
	{ 12000000, 100000000,  200,  24, 1, 0 },
	{ 0, 0, 0, 0, 0, 0 },
};

static struct clk tegra_pll_e = {
	.name      = "pll_e",
	.flags	   = PLL_ALT_MISC_REG,
	.ops       = &tegra_plle_ops,
	.parent    = &tegra_clk_m,
	.reg       = 0xe8,
	.max_rate  = 100000000,
	.u.pll = {
		.input_min = 12000000,
		.input_max = 12000000,
		.freq_table = tegra_pll_e_freq_table,
	},
};

static struct clk tegra_clk_d = {
	.name      = "clk_d",
	.flags     = PERIPH_NO_RESET,
	.ops       = &tegra_clk_double_ops,
	.reg       = 0x34,
	.reg_shift = 12,
	.parent    = &tegra_clk_m,
	.max_rate  = 52000000,
	.u.periph  = {
		.clk_num = 90,
	},
};

/* initialized before peripheral clocks */
static struct clk_mux_sel mux_audio_sync_clk[8+1];
static const struct audio_sources {
	const char *name;
	int value;
} mux_audio_sync_clk_sources[] = {
	{ .name = "spdif_in", .value = 0 },
	{ .name = "i2s1", .value = 1 },
	{ .name = "i2s2", .value = 2 },
	{ .name = "pll_a_out0", .value = 4 },
#if 0 /* FIXME: not implemented */
	{ .name = "ac97", .value = 3 },
	{ .name = "ext_audio_clk2", .value = 5 },
	{ .name = "ext_audio_clk1", .value = 6 },
	{ .name = "ext_vimclk", .value = 7 },
#endif
	{ 0, 0 }
};

static struct clk tegra_clk_audio = {
	.name      = "audio",
	.inputs    = mux_audio_sync_clk,
	.reg       = 0x38,
	.max_rate  = 73728000,
	.ops       = &tegra_audio_sync_clk_ops
};

static struct clk tegra_clk_audio_2x = {
	.name      = "audio_2x",
	.flags     = PERIPH_NO_RESET,
	.max_rate  = 48000000,
	.ops       = &tegra_clk_double_ops,
	.reg       = 0x34,
	.reg_shift = 8,
	.parent    = &tegra_clk_audio,
	.u.periph = {
		.clk_num = 89,
	},
};

static struct clk_mux_sel mux_cdev1[] = {
	{ .input = &tegra_clk_m, .value = TEGRA_MUX_OSC},
	{ .input = &tegra_pll_a_out0, .value = TEGRA_MUX_PLLA_OUT},
	{ .input = &tegra_pll_m_out1, .value = TEGRA_MUX_PLLM_OUT1},
	{ .input = &tegra_clk_audio, .value = TEGRA_MUX_AUDIO_SYNC},
	{ 0, 0},
};

/* dap_mclk1, belongs to the cdev1 pingroup. */
static struct clk tegra_clk_cdev1 = {
	.name      = "cdev1",
	.inputs    = mux_cdev1,
	.ops       = &tegra_cdev_clk_ops,
	.u.periph  = {
		.clk_num = 94,
	},
	.max_rate  = 73728000,
};

static struct clk_mux_sel mux_cdev2[] = {
	{ .input = &tegra_clk_m, .value = TEGRA_MUX_OSC},
#if 0 /* FIXME: not implemented */
	{ .input = &tegra_clk_, .value = TEGRA_MUX_AHB_CLK},
	{ .input = &tegra_clk_, .value = TEGRA_MUX_APB_CLK},
#endif
	{ .input = &tegra_pll_p_out4, .value = TEGRA_MUX_PLLP_OUT4},
	{ 0, 0},
};

/* dap_mclk2, belongs to the cdev2 pingroup. */
static struct clk tegra_clk_cdev2 = {
	.name      = "cdev2",
	.inputs    = mux_cdev2,
	.ops       = &tegra_cdev_clk_ops,
	.u.periph  = {
		.clk_num = 93,
	},
	.max_rate  = 73728000,
};

/* This is called after peripheral clocks are initialized, as the
 * audio_sync clock depends on some of the peripheral clocks.
 */

static void init_audio_sync_clock_mux(void)
{
	int i;
	struct clk_mux_sel *sel = mux_audio_sync_clk;
	const struct audio_sources *src = mux_audio_sync_clk_sources;

	for (i = 0; src->name; i++, sel++, src++) {
		sel->input = tegra_get_clock_by_name(src->name);
		if (!sel->input)
			pr_err("%s: could not find clk %s\n", __func__,
				src->name);
		sel->value = src->value;
	}

	clk_init(&tegra_clk_audio);
	clk_init(&tegra_clk_audio_2x);

}

static struct clk_mux_sel mux_cclk[] = {
	{ .input = &tegra_clk_m,	.value = 0},
	{ .input = &tegra_pll_c,	.value = 1},
	{ .input = &tegra_clk_32k,	.value = 2},
	{ .input = &tegra_pll_m,	.value = 3},
	{ .input = &tegra_pll_p,	.value = 4},
	{ .input = &tegra_pll_p_out4,	.value = 5},
	{ .input = &tegra_pll_p_out3,	.value = 6},
	{ .input = &tegra_clk_d,	.value = 7},
	{ .input = &tegra_pll_x,	.value = 8},
	{ 0, 0},
};

static struct clk_mux_sel mux_sclk[] = {
	{ .input = &tegra_clk_m,	.value = 0},
	{ .input = &tegra_pll_c_out1,	.value = 1},
	{ .input = &tegra_pll_p_out4,	.value = 2},
	{ .input = &tegra_pll_p_out3,	.value = 3},
	{ .input = &tegra_pll_p_out2,	.value = 4},
	{ .input = &tegra_clk_d,	.value = 5},
	{ .input = &tegra_clk_32k,	.value = 6},
	{ .input = &tegra_pll_m_out1,	.value = 7},
	{ 0, 0},
};

static struct clk tegra_clk_cclk = {
	.name	= "cclk",
	.inputs	= mux_cclk,
	.reg	= 0x20,
	.ops	= &tegra_super_ops,
	.max_rate = 1200000000,
};

static struct clk tegra_clk_sclk = {
	.name	= "sclk",
	.inputs	= mux_sclk,
	.reg	= 0x28,
	.ops	= &tegra_super_ops,
	.max_rate = 240000000,
	.min_rate = 120000000,
};

static struct clk tegra_clk_virtual_cpu = {
	.name      = "cpu",
	.parent    = &tegra_clk_cclk,
	.ops       = &tegra_cpu_ops,
	.max_rate  = 1200000000,
	.u.cpu = {
		.main      = &tegra_pll_x,
		.backup    = &tegra_pll_p,
	},
};

static struct clk tegra_clk_cop = {
	.name      = "cop",
	.parent    = &tegra_clk_sclk,
	.ops       = &tegra_cop_ops,
	.max_rate  = 240000000,
};

static struct clk tegra_clk_hclk = {
	.name		= "hclk",
	.flags		= DIV_BUS,
	.parent		= &tegra_clk_sclk,
	.reg		= 0x30,
	.reg_shift	= 4,
	.ops		= &tegra_bus_ops,
	.max_rate       = 240000000,
};

static struct clk tegra_clk_pclk = {
	.name		= "pclk",
	.flags		= DIV_BUS,
	.parent		= &tegra_clk_hclk,
	.reg		= 0x30,
	.reg_shift	= 0,
	.ops		= &tegra_bus_ops,
	.max_rate       = 120000000,
};

static struct clk tegra_clk_blink = {
	.name		= "blink",
	.parent		= &tegra_clk_32k,
	.reg		= 0x40,
	.ops		= &tegra_blink_clk_ops,
	.max_rate	= 32768,
};

static struct clk_mux_sel mux_pllm_pllc_pllp_plla[] = {
	{ .input = &tegra_pll_m, .value = 0},
	{ .input = &tegra_pll_c, .value = 1},
	{ .input = &tegra_pll_p, .value = 2},
	{ .input = &tegra_pll_a_out0, .value = 3},
	{ 0, 0},
};

static struct clk_mux_sel mux_pllm_pllc_pllp_clkm[] = {
	{ .input = &tegra_pll_m, .value = 0},
	{ .input = &tegra_pll_c, .value = 1},
	{ .input = &tegra_pll_p, .value = 2},
	{ .input = &tegra_clk_m, .value = 3},
	{ 0, 0},
};

static struct clk_mux_sel mux_pllp_pllc_pllm_clkm[] = {
	{ .input = &tegra_pll_p, .value = 0},
	{ .input = &tegra_pll_c, .value = 1},
	{ .input = &tegra_pll_m, .value = 2},
	{ .input = &tegra_clk_m, .value = 3},
	{ 0, 0},
};

static struct clk_mux_sel mux_pllaout0_audio2x_pllp_clkm[] = {
	{.input = &tegra_pll_a_out0, .value = 0},
	{.input = &tegra_clk_audio_2x, .value = 1},
	{.input = &tegra_pll_p, .value = 2},
	{.input = &tegra_clk_m, .value = 3},
	{ 0, 0},
};

static struct clk_mux_sel mux_pllp_plld_pllc_clkm[] = {
	{.input = &tegra_pll_p, .value = 0},
	{.input = &tegra_pll_d_out0, .value = 1},
	{.input = &tegra_pll_c, .value = 2},
	{.input = &tegra_clk_m, .value = 3},
	{ 0, 0},
};

static struct clk_mux_sel mux_pllp_pllc_audio_clkm_clk32[] = {
	{.input = &tegra_pll_p,     .value = 0},
	{.input = &tegra_pll_c,     .value = 1},
	{.input = &tegra_clk_audio,     .value = 2},
	{.input = &tegra_clk_m,     .value = 3},
	{.input = &tegra_clk_32k,   .value = 4},
	{ 0, 0},
};

static struct clk_mux_sel mux_pllp_pllc_pllm[] = {
	{.input = &tegra_pll_p,     .value = 0},
	{.input = &tegra_pll_c,     .value = 1},
	{.input = &tegra_pll_m,     .value = 2},
	{ 0, 0},
};

static struct clk_mux_sel mux_clk_m[] = {
	{ .input = &tegra_clk_m, .value = 0},
	{ 0, 0},
};

static struct clk_mux_sel mux_pllp_out3[] = {
	{ .input = &tegra_pll_p_out3, .value = 0},
	{ 0, 0},
};

static struct clk_mux_sel mux_plld[] = {
	{ .input = &tegra_pll_d, .value = 0},
	{ 0, 0},
};

static struct clk_mux_sel mux_clk_32k[] = {
	{ .input = &tegra_clk_32k, .value = 0},
	{ 0, 0},
};

static struct clk_mux_sel mux_pclk[] = {
	{ .input = &tegra_clk_pclk, .value = 0},
	{ 0, 0},
};

static struct clk tegra_clk_emc = {
	.name = "emc",
	.ops = &tegra_emc_clk_ops,
	.reg = 0x19c,
	.max_rate = 800000000,
	.inputs = mux_pllm_pllc_pllp_clkm,
	.flags = MUX | DIV_U71 | PERIPH_EMC_ENB,
	.u.periph = {
		.clk_num = 57,
	},
};

#define PERIPH_CLK(_name, _dev, _con, _clk_num, _reg, _max, _inputs, _flags) \
	{						\
		.name      = _name,			\
		.ops       = &tegra_periph_clk_ops,	\
		.reg       = _reg,			\
		.inputs    = _inputs,			\
		.flags     = _flags,			\
		.max_rate  = _max,			\
		.u.periph = {				\
			.clk_num   = _clk_num,		\
		},					\
	}

#define SHARED_CLK(_name, _dev, _con, _parent)		\
	{						\
		.name      = _name,			\
		.ops       = &tegra_clk_shared_bus_ops,	\
		.parent = _parent,			\
	}

struct clk tegra_list_clks[] = {
	PERIPH_CLK("apbdma",	"tegra-dma",		NULL,	34,	0,	108000000, mux_pclk,			0),
	PERIPH_CLK("kbc",	"tegra-kbc",		NULL,	36,	0,	32768,     mux_clk_32k,			PERIPH_NO_RESET),
	PERIPH_CLK("rtc",	"rtc-tegra",		NULL,	4,	0,	32768,     mux_clk_32k,			PERIPH_NO_RESET),
	PERIPH_CLK("timer",	"timer",		NULL,	5,	0,	26000000,  mux_clk_m,			0),
	PERIPH_CLK("kfuse",	"kfuse-tegra",		NULL,	40,	0,	26000000,  mux_clk_m,			0),
	PERIPH_CLK("i2s1",	"tegra-i2s.0",		NULL,	11,	0x100,	26000000,  mux_pllaout0_audio2x_pllp_clkm,	MUX | DIV_U71),
	PERIPH_CLK("i2s2",	"tegra-i2s.1",		NULL,	18,	0x104,	26000000,  mux_pllaout0_audio2x_pllp_clkm,	MUX | DIV_U71),
	PERIPH_CLK("spdif_out",	"spdif_out",		NULL,	10,	0x108,	100000000, mux_pllaout0_audio2x_pllp_clkm,	MUX | DIV_U71),
	PERIPH_CLK("spdif_in",	"spdif_in",		NULL,	10,	0x10c,	100000000, mux_pllp_pllc_pllm,		MUX | DIV_U71),
	PERIPH_CLK("pwm",	"pwm",			NULL,	17,	0x110,	432000000, mux_pllp_pllc_audio_clkm_clk32,	MUX | DIV_U71 | PERIPH_SOURCE_CLK_4BIT),
	PERIPH_CLK("spi",	"spi",			NULL,	43,	0x114,	40000000,  mux_pllp_pllc_pllm_clkm,	MUX | DIV_U71),
	PERIPH_CLK("xio",	"xio",			NULL,	45,	0x120,	150000000, mux_pllp_pllc_pllm_clkm,	MUX | DIV_U71),
	PERIPH_CLK("twc",	"twc",			NULL,	16,	0x12c,	150000000, mux_pllp_pllc_pllm_clkm,	MUX | DIV_U71),
	PERIPH_CLK("sbc1",	"spi_tegra.0",		NULL,	41,	0x134,	160000000, mux_pllp_pllc_pllm_clkm,	MUX | DIV_U71),
	PERIPH_CLK("sbc2",	"spi_tegra.1",		NULL,	44,	0x118,	160000000, mux_pllp_pllc_pllm_clkm,	MUX | DIV_U71),
	PERIPH_CLK("sbc3",	"spi_tegra.2",		NULL,	46,	0x11c,	160000000, mux_pllp_pllc_pllm_clkm,	MUX | DIV_U71),
	PERIPH_CLK("sbc4",	"spi_tegra.3",		NULL,	68,	0x1b4,	160000000, mux_pllp_pllc_pllm_clkm,	MUX | DIV_U71),
	PERIPH_CLK("ide",	"ide",			NULL,	25,	0x144,	100000000, mux_pllp_pllc_pllm_clkm,	MUX | DIV_U71), /* requires min voltage */
	PERIPH_CLK("ndflash",	"tegra_nand",		NULL,	13,	0x160,	164000000, mux_pllp_pllc_pllm_clkm,	MUX | DIV_U71), /* scales with voltage */
	PERIPH_CLK("vfir",	"vfir",			NULL,	7,	0x168,	72000000,  mux_pllp_pllc_pllm_clkm,	MUX | DIV_U71),
	PERIPH_CLK("sdmmc1",	"sdhci-tegra.0",	NULL,	14,	0x150,	52000000,  mux_pllp_pllc_pllm_clkm,	MUX | DIV_U71), /* scales with voltage */
	PERIPH_CLK("sdmmc2",	"sdhci-tegra.1",	NULL,	9,	0x154,	52000000,  mux_pllp_pllc_pllm_clkm,	MUX | DIV_U71), /* scales with voltage */
	PERIPH_CLK("sdmmc3",	"sdhci-tegra.2",	NULL,	69,	0x1bc,	52000000,  mux_pllp_pllc_pllm_clkm,	MUX | DIV_U71), /* scales with voltage */
	PERIPH_CLK("sdmmc4",	"sdhci-tegra.3",	NULL,	15,	0x164,	52000000,  mux_pllp_pllc_pllm_clkm,	MUX | DIV_U71), /* scales with voltage */
	PERIPH_CLK("vcp",	"tegra-avp",		"vcp",	29,	0,	250000000, mux_clk_m,			0),
	PERIPH_CLK("bsea",	"tegra-avp",		"bsea",	62,	0,	250000000, mux_clk_m,			0),
	PERIPH_CLK("bsev",	"tegra-aes",		"bsev",	63,	0,	250000000, mux_clk_m,			0),
	PERIPH_CLK("vde",	"tegra-avp",		"vde",	61,	0x1c8,	250000000, mux_pllp_pllc_pllm_clkm,	MUX | DIV_U71), /* scales with voltage and process_id */
	PERIPH_CLK("csite",	"csite",		NULL,	73,	0x1d4,	144000000, mux_pllp_pllc_pllm_clkm,	MUX | DIV_U71), /* max rate ??? */
	/* FIXME: what is la? */
	PERIPH_CLK("la",	"la",			NULL,	76,	0x1f8,	26000000,  mux_pllp_pllc_pllm_clkm,	MUX | DIV_U71),
	PERIPH_CLK("owr",	"tegra_w1",		NULL,	71,	0x1cc,	26000000,  mux_pllp_pllc_pllm_clkm,	MUX | DIV_U71),
	PERIPH_CLK("nor",	"nor",			NULL,	42,	0x1d0,	92000000,  mux_pllp_pllc_pllm_clkm,	MUX | DIV_U71), /* requires min voltage */
	PERIPH_CLK("mipi",	"mipi",			NULL,	50,	0x174,	60000000,  mux_pllp_pllc_pllm_clkm,	MUX | DIV_U71), /* scales with voltage */
	PERIPH_CLK("i2c1",	"tegra-i2c.0",		NULL,	12,	0x124,	26000000,  mux_pllp_pllc_pllm_clkm,	MUX | DIV_U16),
	PERIPH_CLK("i2c2",	"tegra-i2c.1",		NULL,	54,	0x198,	26000000,  mux_pllp_pllc_pllm_clkm,	MUX | DIV_U16),
	PERIPH_CLK("i2c3",	"tegra-i2c.2",		NULL,	67,	0x1b8,	26000000,  mux_pllp_pllc_pllm_clkm,	MUX | DIV_U16),
	PERIPH_CLK("dvc",	"tegra-i2c.3",		NULL,	47,	0x128,	26000000,  mux_pllp_pllc_pllm_clkm,	MUX | DIV_U16),
	PERIPH_CLK("i2c1_i2c",	"tegra-i2c.0",		"i2c",	0,	0,	72000000,  mux_pllp_out3,			0),
	PERIPH_CLK("i2c2_i2c",	"tegra-i2c.1",		"i2c",	0,	0,	72000000,  mux_pllp_out3,			0),
	PERIPH_CLK("i2c3_i2c",	"tegra-i2c.2",		"i2c",	0,	0,	72000000,  mux_pllp_out3,			0),
	PERIPH_CLK("dvc_i2c",	"tegra-i2c.3",		"i2c",	0,	0,	72000000,  mux_pllp_out3,			0),
	PERIPH_CLK("uarta",	"uart.0",		NULL,	6,	0x178,	600000000, mux_pllp_pllc_pllm_clkm,	MUX),
	PERIPH_CLK("uartb",	"uart.1",		NULL,	7,	0x17c,	600000000, mux_pllp_pllc_pllm_clkm,	MUX),
	PERIPH_CLK("uartc",	"uart.2",		NULL,	55,	0x1a0,	600000000, mux_pllp_pllc_pllm_clkm,	MUX),
	PERIPH_CLK("uartd",	"uart.3",		NULL,	65,	0x1c0,	600000000, mux_pllp_pllc_pllm_clkm,	MUX),
	PERIPH_CLK("uarte",	"uart.4",		NULL,	66,	0x1c4,	600000000, mux_pllp_pllc_pllm_clkm,	MUX),
	PERIPH_CLK("3d",	"3d",			NULL,	24,	0x158,	300000000, mux_pllm_pllc_pllp_plla,	MUX | DIV_U71 | PERIPH_MANUAL_RESET), /* scales with voltage and process_id */
	PERIPH_CLK("2d",	"2d",			NULL,	21,	0x15c,	300000000, mux_pllm_pllc_pllp_plla,	MUX | DIV_U71), /* scales with voltage and process_id */
	PERIPH_CLK("vi",	"tegra_camera",		"vi",	20,	0x148,	150000000, mux_pllm_pllc_pllp_plla,	MUX | DIV_U71), /* scales with voltage and process_id */
	PERIPH_CLK("vi_sensor",	"tegra_camera",		"vi_sensor",	20,	0x1a8,	150000000, mux_pllm_pllc_pllp_plla,	MUX | DIV_U71 | PERIPH_NO_RESET), /* scales with voltage and process_id */
	PERIPH_CLK("epp",	"epp",			NULL,	19,	0x16c,	300000000, mux_pllm_pllc_pllp_plla,	MUX | DIV_U71), /* scales with voltage and process_id */
	PERIPH_CLK("mpe",	"mpe",			NULL,	60,	0x170,	250000000, mux_pllm_pllc_pllp_plla,	MUX | DIV_U71), /* scales with voltage and process_id */
	PERIPH_CLK("host1x",	"host1x",		NULL,	28,	0x180,	166000000, mux_pllm_pllc_pllp_plla,	MUX | DIV_U71), /* scales with voltage and process_id */
	PERIPH_CLK("cve",	"cve",			NULL,	49,	0x140,	250000000, mux_pllp_plld_pllc_clkm,	MUX | DIV_U71), /* requires min voltage */
	PERIPH_CLK("tvo",	"tvo",			NULL,	49,	0x188,	250000000, mux_pllp_plld_pllc_clkm,	MUX | DIV_U71), /* requires min voltage */
	PERIPH_CLK("hdmi",	"hdmi",			NULL,	51,	0x18c,	600000000, mux_pllp_plld_pllc_clkm,	MUX | DIV_U71), /* requires min voltage */
	PERIPH_CLK("tvdac",	"tvdac",		NULL,	53,	0x194,	250000000, mux_pllp_plld_pllc_clkm,	MUX | DIV_U71), /* requires min voltage */
	PERIPH_CLK("disp1",	"tegradc.0",		NULL,	27,	0x138,	600000000, mux_pllp_plld_pllc_clkm,	MUX | DIV_U71), /* scales with voltage and process_id */
	PERIPH_CLK("disp2",	"tegradc.1",		NULL,	26,	0x13c,	600000000, mux_pllp_plld_pllc_clkm,	MUX | DIV_U71), /* scales with voltage and process_id */
	PERIPH_CLK("usbd",	"fsl-tegra-udc",	NULL,	22,	0,	480000000, mux_clk_m,			0), /* requires min voltage */
	PERIPH_CLK("usb2",	"tegra-ehci.1",		NULL,	58,	0,	480000000, mux_clk_m,			0), /* requires min voltage */
	PERIPH_CLK("usb3",	"tegra-ehci.2",		NULL,	59,	0,	480000000, mux_clk_m,			0), /* requires min voltage */
	PERIPH_CLK("dsi",	"dsi",			NULL,	48,	0,	500000000, mux_plld,			0), /* scales with voltage */
	PERIPH_CLK("csi",	"tegra_camera",		"csi",	52,	0,	72000000,  mux_pllp_out3,		0),
	PERIPH_CLK("isp",	"tegra_camera",		"isp",	23,	0,	150000000, mux_clk_m,			0), /* same frequency as VI */
	PERIPH_CLK("csus",	"tegra_camera",		"csus",	92,	0,	150000000, mux_clk_m,			PERIPH_NO_RESET),
	PERIPH_CLK("pex",       NULL,			"pex",  70,     0,	26000000,  mux_clk_m,			PERIPH_MANUAL_RESET),
	PERIPH_CLK("afi",       NULL,			"afi",  72,     0,	26000000,  mux_clk_m,			PERIPH_MANUAL_RESET),
	PERIPH_CLK("pcie_xclk", NULL,		  "pcie_xclk",  74,     0,	26000000,  mux_clk_m,			PERIPH_MANUAL_RESET),

	SHARED_CLK("avp.sclk",	"tegra-avp",		"sclk",	&tegra_clk_sclk),
	SHARED_CLK("avp.emc",	"tegra-avp",		"emc",	&tegra_clk_emc),
	SHARED_CLK("cpu.emc",	"cpu",			"emc",	&tegra_clk_emc),
	SHARED_CLK("disp1.emc",	"tegradc.0",		"emc",	&tegra_clk_emc),
	SHARED_CLK("disp2.emc",	"tegradc.1",		"emc",	&tegra_clk_emc),
	SHARED_CLK("hdmi.emc",	"hdmi",			"emc",	&tegra_clk_emc),
	SHARED_CLK("host.emc",	"tegra_grhost",		"emc",	&tegra_clk_emc),
	SHARED_CLK("usbd.emc",	"fsl-tegra-udc",	"emc",	&tegra_clk_emc),
	SHARED_CLK("usb1.emc",	"tegra-ehci.0",		"emc",	&tegra_clk_emc),
	SHARED_CLK("usb2.emc",	"tegra-ehci.1",		"emc",	&tegra_clk_emc),
	SHARED_CLK("usb3.emc",	"tegra-ehci.2",		"emc",	&tegra_clk_emc),
};

struct clk *tegra_ptr_clks[] = {
	&tegra_clk_32k,
	&tegra_pll_s,
	&tegra_clk_m,
	&tegra_pll_m,
	&tegra_pll_m_out1,
	&tegra_pll_c,
	&tegra_pll_c_out1,
	&tegra_pll_p,
	&tegra_pll_p_out1,
	&tegra_pll_p_out2,
	&tegra_pll_p_out3,
	&tegra_pll_p_out4,
	&tegra_pll_a,
	&tegra_pll_a_out0,
	&tegra_pll_d,
	&tegra_pll_d_out0,
	&tegra_pll_u,
	&tegra_pll_x,
	&tegra_pll_e,
	&tegra_clk_cclk,
	&tegra_clk_sclk,
	&tegra_clk_hclk,
	&tegra_clk_pclk,
	&tegra_clk_d,
	&tegra_clk_cdev1,
	&tegra_clk_cdev2,
	&tegra_clk_virtual_cpu,
	&tegra_clk_blink,
	&tegra_clk_cop,
	&tegra_clk_emc,
};

static void tegra2_init_one_clock(struct clk *c)
{
	INIT_LIST_HEAD(&c->node);
	clk_init(c);
	INIT_LIST_HEAD(&c->shared_bus_list);
}

static void tegra2_init_clocks(void)
{
	int i;
	INIT_LIST_HEAD(&clocks);

	for (i = 0; i < ARRAY_SIZE(tegra_ptr_clks); i++)
		tegra2_init_one_clock(tegra_ptr_clks[i]);

	for (i = 0; i < ARRAY_SIZE(tegra_list_clks); i++)
		tegra2_init_one_clock(&tegra_list_clks[i]);

	init_audio_sync_clock_mux();
}


static void clock_tree_show_one(struct seq_file *s, struct clk *c, int level)
{
	struct clk *child;
	const char *state = "uninit";
	char div[8] = {0};

	if (c->state == ON)
		state = "on";
	else if (c->state == OFF)
		state = "off";

	if (c->mul != 0 && c->div != 0) {
		if (c->mul > c->div) {
			int mul = c->mul / c->div;
			int mul2 = (c->mul * 10 / c->div) % 10;
			int mul3 = (c->mul * 10) % c->div;
			if (mul2 == 0 && mul3 == 0)
				snprintf(div, sizeof(div), "x%d", mul);
			else if (mul3 == 0)
				snprintf(div, sizeof(div), "x%d.%d", mul, mul2);
			else
				snprintf(div, sizeof(div), "x%d.%d..", mul, mul2);
		} else {
			snprintf(div, sizeof(div), "%d%s", c->div / c->mul,
				(c->div % c->mul) ? ".5" : "");
		}
	}

	seq_printf(s, "%*s%c%c%-*s %-6s %-3d %-8s %-10lu\n",
		level * 3 + 1, "",
		c->rate > c->max_rate ? '!' : ' ',
		!c->set ? '*' : ' ',
		30 - level * 3, c->name,
		state, 1, div, clk_get_rate_all_locked(c));

	list_for_each_entry(child, &clocks, node) {
		if (child->parent != c)
			continue;

		clock_tree_show_one(s, child, level + 1);
	}
}

static int clock_tree_show(struct seq_file *s, void *data)
{
	struct clk *c;
	seq_printf(s, "   clock                          state  ref div      rate\n");
	seq_printf(s, "--------------------------------------------------------------\n");

	list_for_each_entry(c, &clocks, node)
		if (c->parent == NULL)
			clock_tree_show_one(s, c, 0);

	return 0;
}

static int clock_tree_open(struct inode *inode, struct file *file)
{
	// Read tegra fuses
	tegra_init_fuse();
	
	// Actualize clock info
	tegra2_init_clocks();
	
	// Do the open
	return single_open(file, clock_tree_show, inode->i_private);
}

static const struct file_operations debug_clock_tree_fops = {
	.open		= clock_tree_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};


///

struct tegra_pinmux_dump_data {
	void __iomem * reg_apb_base; 
	void __iomem * reg_emc_base; 
	void __iomem * reg_clk_base;
	void __iomem * reg_pmc_base;
	void __iomem * reg_fuse_base;
	
	struct dentry * dbg_pinmux;
	struct dentry * dbg_pinmux_drive;
	struct dentry * dbg_emc;
	struct dentry * dbg_clk;
	
	struct proc_dir_entry * proc_pinmux;
	struct proc_dir_entry * proc_pinmux_drive;
	struct proc_dir_entry * proc_emc;
	struct proc_dir_entry * proc_clk;
}; 


struct tegra_pinmux_dump_data *g_data;

static int __devinit tegra_pinmux_dump_init(void)
{
	struct tegra_pinmux_dump_data *data;
	
	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (!data) {
		pr_err("no memory for context\n");
		return -ENOMEM;
	}
	g_data = data;
	
	data->reg_apb_base = ioremap(TEGRA_APB_MISC_BASE, TEGRA_APB_MISC_SIZE);
	if (!data->reg_apb_base) {
		kfree(data);
		g_data = NULL;
		
		pr_err("Failed to map PINMUX registers\n");
		return -ENODEV;
	} 
	reg_apb_base = data->reg_apb_base;

	data->reg_emc_base = ioremap(TEGRA_EMC_BASE, TEGRA_EMC_SIZE);
	if (!data->reg_emc_base) {
		iounmap(data->reg_apb_base); 
		kfree(data);
		g_data = NULL;
		
		pr_err("Failed to map EMC registers\n");
		return -ENODEV;
	} 
	reg_emc_base = data->reg_emc_base;
	
	data->reg_clk_base = ioremap(TEGRA_CLK_RESET_BASE, TEGRA_CLK_RESET_SIZE);
	if (!data->reg_clk_base) {
		iounmap(data->reg_emc_base);
		iounmap(data->reg_apb_base); 
		kfree(data);
		g_data = NULL;
		
		pr_err("Failed to map CLK_RST registers\n");
		return -ENODEV;
	} 
	reg_clk_base = data->reg_clk_base;

	
	data->reg_pmc_base = ioremap(TEGRA_PMC_BASE, TEGRA_PMC_SIZE);
	if (!data->reg_pmc_base) {
		iounmap(data->reg_clk_base);
		iounmap(data->reg_emc_base);
		iounmap(data->reg_apb_base); 
		kfree(data);
		g_data = NULL;
		
		pr_err("Failed to map PMC registers\n");
		return -ENODEV;
	} 
	reg_pmc_base = data->reg_pmc_base;

	data->reg_fuse_base = ioremap(TEGRA_FUSE_BASE, TEGRA_FUSE_SIZE);
	if (!data->reg_fuse_base) {
		iounmap(data->reg_pmc_base);
		iounmap(data->reg_clk_base);
		iounmap(data->reg_emc_base);
		iounmap(data->reg_apb_base); 
		
		kfree(data);
		g_data = NULL;
		
		pr_err("Failed to map FUSE registers\n");
		return -ENODEV;
	} 
	reg_fuse_base = data->reg_fuse_base;
		
	
	data->dbg_pinmux = debugfs_create_file("realtime_tegra_pinmux", S_IRUGO,
					NULL, NULL, &debug_fops);
	data->dbg_pinmux_drive = debugfs_create_file("realtime_tegra_pinmux_drive", S_IRUGO,
					NULL, NULL, &debug_drive_fops);
	data->dbg_emc = debugfs_create_file("realtime_tegra_emc", S_IRUGO,
					NULL, NULL, &debug_emc_fops);
	data->dbg_clk = debugfs_create_file("realtime_tegra_clock", S_IRUGO, 
					NULL, NULL, &debug_clock_tree_fops);

					
					
	data->proc_pinmux = create_proc_entry("tegra-pinmux", 0666, NULL) ;
	if (!data->proc_pinmux)	{
	
		debugfs_remove(data->dbg_clk);
		debugfs_remove(data->dbg_emc);
		debugfs_remove(data->dbg_pinmux_drive);
		debugfs_remove(data->dbg_pinmux);
		
		iounmap(data->reg_fuse_base);
		iounmap(data->reg_pmc_base);
		iounmap(data->reg_clk_base);
		iounmap(data->reg_emc_base); 
		iounmap(data->reg_apb_base); 
		
		kfree(data);
		g_data = NULL;
		
		pr_err("unable to register tegra-pinmux procfs entry. Unable to continue\n");
		return -ENODEV;
	
	}
	data->proc_pinmux->proc_fops = &debug_fops;

	data->proc_pinmux_drive = create_proc_entry("tegra-pinmux-drive", 0666, NULL) ;
	if (!data->proc_pinmux_drive)	{
		remove_proc_entry("tegra-pinmux", data->proc_pinmux->parent) ; 
		
		debugfs_remove(data->dbg_clk);
		debugfs_remove(data->dbg_emc);
		debugfs_remove(data->dbg_pinmux_drive);
		debugfs_remove(data->dbg_pinmux);
		
		iounmap(data->reg_fuse_base);
		iounmap(data->reg_pmc_base);
		iounmap(data->reg_clk_base);
		iounmap(data->reg_emc_base); 
		iounmap(data->reg_apb_base); 
		
		kfree(data);
		g_data = NULL;
		
		pr_err("unable to register tegra-pinmux-drive procfs entry. Unable to continue\n");
		return -ENODEV;
	
	}
	data->proc_pinmux_drive->proc_fops = &debug_drive_fops;

	data->proc_emc = create_proc_entry("tegra-emc", 0666, NULL) ;
	if (!data->proc_emc)	{
		remove_proc_entry("tegra-pinmux-drive", data->proc_pinmux_drive->parent) ; 
		remove_proc_entry("tegra-pinmux", data->proc_pinmux->parent) ; 
		
		debugfs_remove(data->dbg_clk);
		debugfs_remove(data->dbg_emc);
		debugfs_remove(data->dbg_pinmux_drive);
		debugfs_remove(data->dbg_pinmux);
		
		iounmap(data->reg_fuse_base);
		iounmap(data->reg_pmc_base);
		iounmap(data->reg_clk_base);
		iounmap(data->reg_emc_base); 
		iounmap(data->reg_apb_base); 
		
		kfree(data);
		g_data = NULL;
		
		pr_err("unable to register tegra-emc procfs entry. Unable to continue\n");
		return -ENODEV;
	
	}
	data->proc_emc->proc_fops = &debug_emc_fops;
	
	data->proc_clk = create_proc_entry("tegra-clock", 0666, NULL) ;
	if (!data->proc_clk)	{
		remove_proc_entry("tegra-emc", data->proc_emc->parent) ; 
		remove_proc_entry("tegra-pinmux-drive", data->proc_pinmux_drive->parent) ; 
		remove_proc_entry("tegra-pinmux", data->proc_pinmux->parent) ; 
		
		debugfs_remove(data->dbg_clk);
		debugfs_remove(data->dbg_emc);
		debugfs_remove(data->dbg_pinmux_drive);
		debugfs_remove(data->dbg_pinmux);
		
		iounmap(data->reg_fuse_base);
		iounmap(data->reg_pmc_base);
		iounmap(data->reg_clk_base);
		iounmap(data->reg_emc_base); 
		iounmap(data->reg_apb_base); 
		
		kfree(data);
		g_data = NULL;
		
		pr_err("unable to register tegra-clk procfs entry. Unable to continue\n");
		return -ENODEV;
	
	}
	data->proc_clk->proc_fops = &debug_clock_tree_fops;
	
	pr_info("Realtime pinmux config dump module loaded\n");
	
	return 0;
}

static void tegra_pinmux_dump_exit(void)
{
	
	struct tegra_pinmux_dump_data *data = g_data;
	if (!data)
		return;

	remove_proc_entry("tegra-clock", data->proc_clk->parent) ; 
	remove_proc_entry("tegra-emc", data->proc_emc->parent) ; 
	remove_proc_entry("tegra-pinmux-drive", data->proc_pinmux_drive->parent) ; 
	remove_proc_entry("tegra-pinmux", data->proc_pinmux->parent) ; 

	debugfs_remove(data->dbg_clk);
	debugfs_remove(data->dbg_emc);
	debugfs_remove(data->dbg_pinmux_drive);
	debugfs_remove(data->dbg_pinmux);

	iounmap(data->reg_fuse_base);
	iounmap(data->reg_pmc_base);
	iounmap(data->reg_clk_base);
	iounmap(data->reg_emc_base); 
	iounmap(data->reg_apb_base); 
	
	kfree(data);
	g_data = NULL;
	
	return ;
}

module_init(tegra_pinmux_dump_init);
module_exit(tegra_pinmux_dump_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Eduardo José Tagle <ejtagle@tutopia.com>");
MODULE_DESCRIPTION("Realtime pinmux configuration dump"); 