// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2018 NXP
 */
#include <common.h>
#include <env.h>
#include <init.h>
#include <miiphy.h>
#include <netdev.h>
#include <asm/global_data.h>
#include <asm/mach-imx/iomux-v3.h>
#include <asm-generic/gpio.h>
#include <asm/arch/imx8mm_pins.h>
#include <asm/arch/clock.h>
#include <asm/arch/sys_proto.h>
#include <asm/mach-imx/gpio.h>
#include <asm/mach-imx/mxc_i2c.h>
#include <i2c.h>
#include <asm/io.h>
#include "../../freescale/common/tcpc.h"
#include <usb.h>
#include <imx_sip.h>
#include <linux/arm-smccc.h>

DECLARE_GLOBAL_DATA_PTR;

#define UART_PAD_CTRL	(PAD_CTL_DSE6 | PAD_CTL_FSEL1)
#define WDOG_PAD_CTRL	(PAD_CTL_DSE6 | PAD_CTL_ODE | PAD_CTL_PUE | PAD_CTL_PE)

static iomux_v3_cfg_t const uart_pads[] = {
	IMX8MM_PAD_UART2_RXD_UART2_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
	IMX8MM_PAD_UART2_TXD_UART2_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

static iomux_v3_cfg_t const wdog_pads[] = {
	IMX8MM_PAD_GPIO1_IO02_WDOG1_WDOG_B  | MUX_PAD_CTRL(WDOG_PAD_CTRL),
};


#define FEC_RST_PAD IMX_GPIO_NR(3, 7)
#define MDIOAADR2 IMX_GPIO_NR(1, 25)
#define FEC_RST_PAD_KSZ9021 IMX_GPIO_NR(2, 9)
#define RGMII_RXDV IMX_GPIO_NR(1, 24)
static iomux_v3_cfg_t const fec1_rst_pads[] = {
        IMX8MM_PAD_NAND_DATA01_GPIO3_IO7 | MUX_PAD_CTRL(NO_PAD_CTRL),
		IMX8MM_PAD_ENET_RXC_GPIO1_IO25 | MUX_PAD_CTRL(NO_PAD_CTRL),
        IMX8MM_PAD_SD1_DATA7_GPIO2_IO9 | MUX_PAD_CTRL(NO_PAD_CTRL),
        IMX8MM_PAD_ENET_RX_CTL_GPIO1_IO24 | MUX_PAD_CTRL(NO_PAD_CTRL),
};

static iomux_v3_cfg_t const fec1_rgmii_rxdv_pads[] = {
        IMX8MM_PAD_ENET_RX_CTL_ENET1_RGMII_RX_CTL | MUX_PAD_CTRL(NO_PAD_CTRL),
};

int board_early_init_f(void)
{
	struct wdog_regs *wdog = (struct wdog_regs *)WDOG1_BASE_ADDR;

	imx_iomux_v3_setup_multiple_pads(wdog_pads, ARRAY_SIZE(wdog_pads));

	set_wdog_reset(wdog);

	imx_iomux_v3_setup_multiple_pads(uart_pads, ARRAY_SIZE(uart_pads));

	init_uart_clk(1);

	imx_iomux_v3_setup_multiple_pads(fec1_rst_pads,
				 ARRAY_SIZE(fec1_rst_pads));

	gpio_request(FEC_RST_PAD, "fec1_rst");
	gpio_request(MDIOAADR2, "mdioaddr2");
	gpio_request(RGMII_RXDV, "rgmii_rxdv");

	udelay(1000);
	gpio_direction_output(MDIOAADR2, 0);
	gpio_direction_output(FEC_RST_PAD, 0);
	gpio_direction_output(RGMII_RXDV, 0);
	gpio_direction_output(FEC_RST_PAD_KSZ9021, 0);
	udelay(10000);
	gpio_direction_output(FEC_RST_PAD, 1);
	gpio_direction_output(FEC_RST_PAD_KSZ9021, 1);
	udelay(100);

	imx_iomux_v3_setup_multiple_pads(fec1_rgmii_rxdv_pads, ARRAY_SIZE(fec1_rgmii_rxdv_pads));

	return 0;
}

#if IS_ENABLED(CONFIG_FEC_MXC)


static int setup_fec(void)
{
	struct iomuxc_gpr_base_regs *gpr =
			(struct iomuxc_gpr_base_regs *)IOMUXC_GPR_BASE_ADDR;
	/* Use 125M anatop REF_CLK1 for ENET1, not from external */
	clrsetbits_le32(&gpr->gpr[1],
			IOMUXC_GPR_GPR1_GPR_ENET1_TX_CLK_SEL_MASK, 0);
	return set_clk_enet(ENET_125MHZ);

}

#define MICREL_KSZ9021_EXTREG_CTRL	0xB
#define MICREL_KSZ9021_EXTREG_DATA_WRITE	0xC
#define MICREL_KSZ9021_RGMII_CLK_CTRL_PAD_SCEW	0x104
#define MICREL_KSZ9021_RGMII_RX_DATA_PAD_SCEW	0x105

void ksz9021rn_phy_fixup(struct phy_device *phydev)
{
	printf("KSZ9021\n");
	phy_write(phydev, MDIO_DEVAD_NONE, MICREL_KSZ9021_EXTREG_CTRL, 0x8000 | MICREL_KSZ9021_RGMII_RX_DATA_PAD_SCEW);
	phy_write(phydev, MDIO_DEVAD_NONE, MICREL_KSZ9021_EXTREG_DATA_WRITE, 0x0000);

	phy_write(phydev, MDIO_DEVAD_NONE, MICREL_KSZ9021_EXTREG_CTRL, 0x8000 | MICREL_KSZ9021_RGMII_CLK_CTRL_PAD_SCEW);
	phy_write(phydev, MDIO_DEVAD_NONE, MICREL_KSZ9021_EXTREG_DATA_WRITE, 0xf0f0);

	phy_write(phydev, MDIO_DEVAD_NONE, MICREL_KSZ9021_EXTREG_CTRL, MICREL_KSZ9021_RGMII_CLK_CTRL_PAD_SCEW);
}

void mmd_write_reg(struct phy_device *phydev, int device, int reg, int val)
{
	phy_write(phydev, MDIO_DEVAD_NONE, 0x0d, device);
	phy_write(phydev, MDIO_DEVAD_NONE, 0x0e, reg);
	phy_write(phydev, MDIO_DEVAD_NONE, 0x0d, (1 << 14) | device);
	phy_write(phydev, MDIO_DEVAD_NONE, 0x0e, val);
}

static int ksz9031rn_phy_fixup(struct phy_device *phydev)
{
	printf("KSZ9031\n");

	//write register 6 addr 2 TXD[0:3] skew
	mmd_write_reg(phydev, 2, 6, 0x4111);

	//write register 5 addr 2 RXD[0:3] skew
	mmd_write_reg(phydev, 2, 5, 0x47a7);

	//write register 4 addr 2 RX_DV TX_EN skew
	mmd_write_reg(phydev, 2, 4, 0x004A);

	//write register 8 addr 2 RX_CLK GTX_CLK skew
	mmd_write_reg(phydev, 2, 8, 0x0273);

	return 0;
}

int board_phy_config(struct phy_device *phydev)
{
	/* enable rgmii rxc skew and phy mode select to RGMII copper */
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1d, 0x1f);
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1e, 0x8);

	phy_write(phydev, MDIO_DEVAD_NONE, 0x1d, 0x00);
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1e, 0x82ee);
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1d, 0x05);
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1e, 0x100);

	unsigned short tmp = 0;

	if (miiphy_read("FEC0", CONFIG_FEC_MXC_PHYADDR, MII_PHYSID2, &tmp) != 0)
		debug("PHY ID register 3 read failed\n");

	unsigned short model = (tmp>>4) & 0x3F;

	if (model == 0x21) // KSZ9021
	{
		ksz9021rn_phy_fixup(phydev);
	}
	else if (model == 0x22) // KSZ9031
	{
		ksz9031rn_phy_fixup(phydev);
	}

	if (phydev->drv->config)
		phydev->drv->config(phydev);
	return 0;
}
#endif


int board_init(void)
{
	struct arm_smccc_res res;

	gpio_request(IMX_GPIO_NR(1, 2), "RESET");
	gpio_direction_output(IMX_GPIO_NR(1, 2), 1);

	if (IS_ENABLED(CONFIG_FEC_MXC))
		setup_fec();

	return 0;
}

int board_late_init(void)
{
#ifdef CONFIG_ENV_IS_IN_MMC
	board_late_mmc_env_init();
#endif

#ifdef CONFIG_ENV_VARS_UBOOT_RUNTIME_CONFIG
	env_set("board_name", "ICORE");
	env_set("board_rev", "iMX8MM");
#endif
	return 0;
}

#ifdef CONFIG_ANDROID_SUPPORT
bool is_power_key_pressed(void) {
	return (bool)(!!(readl(SNVS_HPSR) & (0x1 << 6)));
}
#endif

#ifdef CONFIG_FSL_FASTBOOT
#ifdef CONFIG_ANDROID_RECOVERY
int is_recovery_key_pressing(void)
{
	return 0; /* TODO */
}
#endif /* CONFIG_ANDROID_RECOVERY */
#endif /* CONFIG_FSL_FASTBOOT */
