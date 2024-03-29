// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2017 NXP
 *
 * Author: Fabio Estevam <fabio.estevam@freescale.com>
 */

#include <init.h>
#include <net.h>
#include <asm/arch/clock.h>
#include <asm/arch/crm_regs.h>
#include <asm/arch/iomux.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/mx6-pins.h>
#include <asm/arch/sys_proto.h>
#include <asm/global_data.h>
#include <asm/gpio.h>
#include <asm/mach-imx/iomux-v3.h>
#include <asm/mach-imx/boot_mode.h>
#include <asm/io.h>
#include <asm/mach-imx/mxc_i2c.h>
#include <env.h>
#include <linux/delay.h>
#include <linux/sizes.h>
#include <common.h>
#include <fsl_esdhc_imx.h>
#include <mmc.h>
#include <i2c.h>
#include <miiphy.h>
#include <netdev.h>
#include <power/pmic.h>
#include <power/pfuze100_pmic.h>
#include <usb.h>
#include <usb/ehci-ci.h>
#include <asm/mach-imx/video.h>
#include <power/regulator.h>

#ifdef CONFIG_IMX_RDC
#include <asm/mach-imx/rdc-sema.h>
#include <asm/arch/imx-rdc.h>
#endif
DECLARE_GLOBAL_DATA_PTR;

#define UART_PAD_CTRL  (PAD_CTL_PKE | PAD_CTL_PUE |		\
	PAD_CTL_PUS_100K_UP | PAD_CTL_SPEED_MED |		\
	PAD_CTL_DSE_40ohm   | PAD_CTL_SRE_FAST  | PAD_CTL_HYS)

#define USDHC_PAD_CTRL (PAD_CTL_PKE | PAD_CTL_PUE |		\
	PAD_CTL_PUS_22K_UP  | PAD_CTL_SPEED_LOW |		\
	PAD_CTL_DSE_80ohm   | PAD_CTL_SRE_FAST  | PAD_CTL_HYS)

#define I2C_PAD_CTRL    (PAD_CTL_PKE | PAD_CTL_PUE |            \
	PAD_CTL_PUS_100K_UP | PAD_CTL_SPEED_MED |               \
	PAD_CTL_DSE_40ohm | PAD_CTL_HYS |			\
	PAD_CTL_ODE)

#define ENET_PAD_CTRL  (PAD_CTL_PUS_100K_UP | PAD_CTL_PUE |     \
	PAD_CTL_SPEED_HIGH   |                                   \
	PAD_CTL_DSE_48ohm   | PAD_CTL_SRE_FAST)

#define ENET_CLK_PAD_CTRL  (PAD_CTL_SPEED_MED | \
	PAD_CTL_DSE_120ohm   | PAD_CTL_SRE_FAST)

#define ENET_RX_PAD_CTRL  (PAD_CTL_PKE | PAD_CTL_PUE |          \
	PAD_CTL_SPEED_HIGH   | PAD_CTL_SRE_FAST)

#define ENET_RX_PAD_CTRL_PRE  (PAD_CTL_PKE | PAD_CTL_PUE |          \
	PAD_CTL_SPEED_HIGH   | PAD_CTL_PUS_100K_DOWN | PAD_CTL_SRE_FAST)

#define I2C_PAD_CTRL    (PAD_CTL_PKE | PAD_CTL_PUE |            \
	PAD_CTL_PUS_100K_UP | PAD_CTL_SPEED_MED |               \
	PAD_CTL_DSE_40ohm | PAD_CTL_HYS |			\
	PAD_CTL_ODE)

#define LCD_PAD_CTRL    (PAD_CTL_HYS | PAD_CTL_PUS_100K_UP | PAD_CTL_PUE | \
	PAD_CTL_PKE | PAD_CTL_SPEED_MED | PAD_CTL_DSE_40ohm)

#define WDOG_PAD_CTRL (PAD_CTL_PUE | PAD_CTL_PKE | PAD_CTL_SPEED_MED |	\
	PAD_CTL_DSE_40ohm)

#define OTG_ID_PAD_CTRL (PAD_CTL_PKE | PAD_CTL_PUE |		\
	PAD_CTL_PUS_47K_UP  | PAD_CTL_SPEED_LOW |		\
	PAD_CTL_DSE_80ohm   | PAD_CTL_SRE_FAST  | PAD_CTL_HYS)

int dram_init(void)
{
	gd->ram_size = (get_ram_size((long *)PHYS_SDRAM, MAX_SDRAM_SIZE));

	return 0;
}

static iomux_v3_cfg_t const uart5_pads[] = {
	MX6_PAD_KEY_COL3__UART5_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
	MX6_PAD_KEY_ROW3__UART5_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

static iomux_v3_cfg_t const usdhc2_pads[] = {
	MX6_PAD_SD2_CLK__USDHC2_CLK | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD2_CMD__USDHC2_CMD | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD2_DATA0__USDHC2_DATA0 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD2_DATA1__USDHC2_DATA1 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD2_DATA2__USDHC2_DATA2 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD2_DATA3__USDHC2_DATA3 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
};

static iomux_v3_cfg_t const usdhc3_pads[] = {
	MX6_PAD_SD3_CLK__USDHC3_CLK | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD3_CMD__USDHC3_CMD | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD3_DATA0__USDHC3_DATA0 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD3_DATA1__USDHC3_DATA1 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD3_DATA2__USDHC3_DATA2 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD3_DATA3__USDHC3_DATA3 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD3_DATA4__USDHC3_DATA4 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD3_DATA5__USDHC3_DATA5 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD3_DATA6__USDHC3_DATA6 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD3_DATA7__USDHC3_DATA7 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
    MX6_PAD_NAND_CE1_B__USDHC3_RESET_B | MUX_PAD_CTRL(USDHC_PAD_CTRL),
};

#ifdef CONFIG_MX6SXSABRESD_EMMC_REWORK
static iomux_v3_cfg_t const usdhc4_emmc_pads[] = {
	MX6_PAD_SD4_CLK__USDHC4_CLK | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD4_CMD__USDHC4_CMD | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD4_DATA0__USDHC4_DATA0 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD4_DATA1__USDHC4_DATA1 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD4_DATA2__USDHC4_DATA2 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD4_DATA3__USDHC4_DATA3 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD4_DATA4__USDHC4_DATA4 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD4_DATA5__USDHC4_DATA5 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD4_DATA6__USDHC4_DATA6 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD4_DATA7__USDHC4_DATA7 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD4_RESET_B__USDHC4_RESET_B | MUX_PAD_CTRL(NO_PAD_CTRL),
};
#else
static iomux_v3_cfg_t const usdhc4_pads[] = {
	MX6_PAD_SD4_CLK__USDHC4_CLK | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD4_CMD__USDHC4_CMD | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD4_DATA0__USDHC4_DATA0 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD4_DATA1__USDHC4_DATA1 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD4_DATA2__USDHC4_DATA2 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD4_DATA3__USDHC4_DATA3 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD4_DATA7__GPIO6_IO_21 | MUX_PAD_CTRL(NO_PAD_CTRL),
};
#endif

static iomux_v3_cfg_t const wdog_b_pad = {
	MX6_PAD_GPIO1_IO13__GPIO1_IO_13 | MUX_PAD_CTRL(WDOG_PAD_CTRL),
};

static iomux_v3_cfg_t const peri_3v3_pads[] = {
	MX6_PAD_QSPI1A_DATA0__GPIO4_IO_16 | MUX_PAD_CTRL(NO_PAD_CTRL),
};

static void setup_iomux_uart(void)
{
	imx_iomux_v3_setup_multiple_pads(uart5_pads, ARRAY_SIZE(uart5_pads));
}

static iomux_v3_cfg_t const fec1_pads[] = {
	MX6_PAD_ENET1_COL__ENET2_MDC | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET1_CRS__ENET2_MDIO | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII1_RX_CTL__ENET1_RX_EN | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	MX6_PAD_RGMII1_RD0__ENET1_RX_DATA_0 | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	MX6_PAD_RGMII1_RD1__ENET1_RX_DATA_1 | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	MX6_PAD_RGMII1_RD2__ENET1_RX_DATA_2 | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	MX6_PAD_RGMII1_RD3__ENET1_RX_DATA_3 | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	MX6_PAD_RGMII1_RXC__ENET1_RX_CLK | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	MX6_PAD_RGMII1_TX_CTL__ENET1_TX_EN | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII1_TD0__ENET1_TX_DATA_0 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII1_TD1__ENET1_TX_DATA_1 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII1_TD2__ENET1_TX_DATA_2 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII1_TD3__ENET1_TX_DATA_3 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII1_TXC__ENET1_RGMII_TXC | MUX_PAD_CTRL(ENET_PAD_CTRL),
};

static iomux_v3_cfg_t const fec2_pads[] = {
	MX6_PAD_ENET1_COL__ENET2_MDC | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET1_CRS__ENET2_MDIO | MUX_PAD_CTRL(ENET_PAD_CTRL),


	MX6_PAD_RGMII2_RX_CTL__ENET2_RX_EN | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	MX6_PAD_RGMII2_RD0__ENET2_RX_DATA_0 | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	MX6_PAD_RGMII2_RD1__ENET2_RX_DATA_1 | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	MX6_PAD_RGMII2_RD2__ENET2_RX_DATA_2 | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	MX6_PAD_RGMII2_RD3__ENET2_RX_DATA_3 | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	MX6_PAD_RGMII2_RXC__ENET2_RX_CLK | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	MX6_PAD_RGMII2_TX_CTL__ENET2_TX_EN | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII2_TD0__ENET2_TX_DATA_0 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII2_TD1__ENET2_TX_DATA_1 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII2_TD2__ENET2_TX_DATA_2 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII2_TD3__ENET2_TX_DATA_3 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII2_TXC__ENET2_RGMII_TXC | MUX_PAD_CTRL(ENET_PAD_CTRL),
};

static iomux_v3_cfg_t const phy_control_pads[] = {
	/* Phy 25M Clock */
	MX6_PAD_ENET2_RX_CLK__ENET2_REF_CLK_25M | MUX_PAD_CTRL(ENET_CLK_PAD_CTRL),

	/* PHY1 Reset. */
	MX6_PAD_ENET2_CRS__GPIO2_IO_7 | MUX_PAD_CTRL(NO_PAD_CTRL),
	/* PHY2 Reset. */
	MX6_PAD_ENET1_MDIO__GPIO2_IO_3 | MUX_PAD_CTRL(NO_PAD_CTRL),
};

static iomux_v3_cfg_t const fec_pre_reset_pads[] = {
    MX6_PAD_RGMII1_RXC__ENET1_RX_CLK | MUX_PAD_CTRL(ENET_RX_PAD_CTRL_PRE),
    MX6_PAD_RGMII2_RXC__ENET2_RX_CLK | MUX_PAD_CTRL(ENET_RX_PAD_CTRL_PRE),
};

static iomux_v3_cfg_t const fec_post_reset_pads[] = {
    MX6_PAD_RGMII1_RXC__ENET1_RX_CLK | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
    MX6_PAD_RGMII2_RXC__ENET2_RX_CLK | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
};

static void setup_iomux_fec(int fec_id)
{
	if (0 == fec_id)
		imx_iomux_v3_setup_multiple_pads(fec1_pads, ARRAY_SIZE(fec1_pads));
	else
		imx_iomux_v3_setup_multiple_pads(fec2_pads, ARRAY_SIZE(fec2_pads));
}

int board_eth_init(struct bd_info *bis)
{
	int ret;

	setup_iomux_fec(CONFIG_FEC_ENET_DEV);

	ret = fecmxc_initialize_multi(bis, CONFIG_FEC_ENET_DEV,
		CONFIG_FEC_MXC_PHYADDR, IMX_FEC_BASE);
	
	if (ret)
		printf("FEC%d MXC: %s:failed\n", CONFIG_FEC_ENET_DEV, __func__);

	return 0;
}

static int setup_fec(void)
{
	struct iomuxc *iomuxc_regs = (struct iomuxc *)IOMUXC_BASE_ADDR;
	struct anatop_regs *anatop = (struct anatop_regs *)ANATOP_BASE_ADDR;
	int reg, ret;

	/* Use 125M anatop loopback REF_CLK1 for ENET1, clear gpr1[13], gpr1[17]*/
	clrsetbits_le32(&iomuxc_regs->gpr[1], IOMUX_GPR1_FEC1_MASK, 0);

	/* Use 125M anatop loopback REF_CLK1 for ENET2, clear gpr1[14], gpr1[18]*/
	clrsetbits_le32(&iomuxc_regs->gpr[1], IOMUX_GPR1_FEC2_MASK, 0);

	imx_iomux_v3_setup_multiple_pads(phy_control_pads, ARRAY_SIZE(phy_control_pads));

	ret = enable_fec_anatop_clock(0, ENET_125MHZ);
	if (ret)
		return ret;

	ret = enable_fec_anatop_clock(1, ENET_125MHZ);
	if (ret)
		return ret;

	reg = readl(&anatop->pll_enet);
	reg |= BM_ANADIG_PLL_ENET_REF_25M_ENABLE;
	writel(reg, &anatop->pll_enet);

	imx_iomux_v3_setup_multiple_pads(fec_pre_reset_pads, ARRAY_SIZE(fec_pre_reset_pads));

	gpio_request(IMX_GPIO_NR(2, 7) , "reset_phy1");
	gpio_request(IMX_GPIO_NR(2, 3) , "reset_phy2");
	
	gpio_request(IMX_GPIO_NR(5, 5) , "rx_down_1");
	gpio_request(IMX_GPIO_NR(5, 17) , "rx_down_2");

    /*RXC down for phy address*/
	gpio_direction_output(IMX_GPIO_NR(5, 5) , 0);
	gpio_direction_output(IMX_GPIO_NR(5, 17) , 0);
	gpio_set_value(IMX_GPIO_NR(5, 5), 0);
	gpio_set_value(IMX_GPIO_NR(5, 17), 0);

	/* Reset PHY1 */
	gpio_direction_output(IMX_GPIO_NR(2, 7) , 0);
	/* Reset PHY2 */
	gpio_direction_output(IMX_GPIO_NR(2, 3) , 0);

	udelay(500);

	/* Release Reset PHY1 */
	gpio_set_value(IMX_GPIO_NR(2, 7), 1);
	/* Release Reset PHY2 */
	gpio_set_value(IMX_GPIO_NR(2, 3), 1);

	imx_iomux_v3_setup_multiple_pads(fec_post_reset_pads, ARRAY_SIZE(fec_post_reset_pads));

	return 0;
}

#ifdef CONFIG_SYS_I2C_LEGACY
#define PC MUX_PAD_CTRL(I2C_PAD_CTRL)
/* I2C1 for PMIC */
static struct i2c_pads_info i2c_pad_info1 = {
	.scl = {
		.i2c_mode = MX6_PAD_GPIO1_IO00__I2C1_SCL | PC,
		.gpio_mode = MX6_PAD_GPIO1_IO00__GPIO1_IO_0 | PC,
		.gp = IMX_GPIO_NR(1, 0),
	},
	.sda = {
		.i2c_mode = MX6_PAD_GPIO1_IO01__I2C1_SDA | PC,
		.gpio_mode = MX6_PAD_GPIO1_IO01__GPIO1_IO_1 | PC,
		.gp = IMX_GPIO_NR(1, 1),
	},
};

/* I2C2 */
struct i2c_pads_info i2c_pad_info2 = {
	.scl = {
		.i2c_mode = MX6_PAD_GPIO1_IO02__I2C2_SCL | PC,
		.gpio_mode = MX6_PAD_GPIO1_IO02__GPIO1_IO_2 | PC,
		.gp = IMX_GPIO_NR(1, 2),
	},
	.sda = {
		.i2c_mode = MX6_PAD_GPIO1_IO03__I2C2_SDA | PC,
		.gpio_mode = MX6_PAD_GPIO1_IO03__GPIO1_IO_3 | PC,
		.gp = IMX_GPIO_NR(1, 3),
	},
};
#endif

#ifdef CONFIG_POWER_LEGACY
int power_init_board(void)
{
	return 0;
}
#elif defined(CONFIG_DM_PMIC_PFUZE100)
int power_init_board(void)
{
	return 0;
}
#endif

#ifdef CONFIG_LDO_BYPASS_CHECK
#ifdef CONFIG_POWER_LEGACY
void ldo_mode_set(int ldo_bypass)
{
	unsigned int value;
	int is_400M;
	u32 vddarm;
	struct pmic *p = pmic_get("PFUZE100");

	if (!p) {
		printf("No PMIC found!\n");
		return;
	}

	/* switch to ldo_bypass mode */
	if (ldo_bypass) {
		prep_anatop_bypass();
		/* decrease VDDARM to 1.275V */
		pmic_reg_read(p, PFUZE100_SW1ABVOL, &value);
		value &= ~0x3f;
		value |= PFUZE100_SW1ABC_SETP(12750);
		pmic_reg_write(p, PFUZE100_SW1ABVOL, value);

		/* decrease VDDSOC to 1.3V */
		pmic_reg_read(p, PFUZE100_SW1CVOL, &value);
		value &= ~0x3f;
		value |= PFUZE100_SW1ABC_SETP(13000);
		pmic_reg_write(p, PFUZE100_SW1CVOL, value);

		is_400M = set_anatop_bypass(1);
		if (is_400M)
			vddarm = PFUZE100_SW1ABC_SETP(10750);
		else
			vddarm = PFUZE100_SW1ABC_SETP(11750);

		pmic_reg_read(p, PFUZE100_SW1ABVOL, &value);
		value &= ~0x3f;
		value |= vddarm;
		pmic_reg_write(p, PFUZE100_SW1ABVOL, value);

		pmic_reg_read(p, PFUZE100_SW1CVOL, &value);
		value &= ~0x3f;
		value |= PFUZE100_SW1ABC_SETP(11750);
		pmic_reg_write(p, PFUZE100_SW1CVOL, value);

		finish_anatop_bypass();
		printf("switch to ldo_bypass mode!\n");
	}

}
#elif defined(CONFIG_DM_PMIC_PFUZE100)
void ldo_mode_set(int ldo_bypass)
{
	struct udevice *dev;
	int ret;
	int is_400M;
	u32 vddarm;

	ret = pmic_get("pfuze100@8", &dev);
	if (ret == -ENODEV) {
		printf("No PMIC found!\n");
		return;
	}

	/* switch to ldo_bypass mode , boot on 800Mhz */
	if (ldo_bypass) {
		prep_anatop_bypass();

		/* decrease VDDARM for 400Mhz DQ:1.1V, DL:1.275V */
		pmic_clrsetbits(dev, PFUZE100_SW1ABVOL, 0x3f, PFUZE100_SW1ABC_SETP(12750));

		/* increase VDDSOC to 1.3V */
		pmic_clrsetbits(dev, PFUZE100_SW1CVOL, 0x3f, PFUZE100_SW1ABC_SETP(13000));

		is_400M = set_anatop_bypass(1);
		if (is_400M)
			vddarm = PFUZE100_SW1ABC_SETP(10750);
		else
			vddarm = PFUZE100_SW1ABC_SETP(11750);

		pmic_clrsetbits(dev, PFUZE100_SW1ABVOL, 0x3f, vddarm);

		/* decrease VDDSOC to 1.175V */
		pmic_clrsetbits(dev, PFUZE100_SW1CVOL, 0x3f, PFUZE100_SW1ABC_SETP(11750));

		finish_anatop_bypass();
		printf("switch to ldo_bypass mode!\n");
	}
}
#endif
#endif

#ifdef CONFIG_USB_EHCI_MX6
#ifndef CONFIG_DM_USB

#define USB_OTHERREGS_OFFSET	0x800
#define UCTRL_PWR_POL		(1 << 9)

static iomux_v3_cfg_t const usb_otg_pads[] = {
	/* OGT1 */
	MX6_PAD_GPIO1_IO09__USB_OTG1_PWR | MUX_PAD_CTRL(NO_PAD_CTRL),
	MX6_PAD_GPIO1_IO10__ANATOP_OTG1_ID | MUX_PAD_CTRL(OTG_ID_PAD_CTRL),
	/* OTG2 */
	MX6_PAD_GPIO1_IO12__USB_OTG2_PWR | MUX_PAD_CTRL(NO_PAD_CTRL)
};

static void setup_usb(void)
{
	imx_iomux_v3_setup_multiple_pads(usb_otg_pads,
					 ARRAY_SIZE(usb_otg_pads));
}

int board_usb_phy_mode(int port)
{
	if (port == 1)
		return USB_INIT_HOST;
	else
		return usb_phy_mode(port);
}

int board_ehci_hcd_init(int port)
{
	u32 *usbnc_usb_ctrl;

	if (port > 1)
		return -EINVAL;

	usbnc_usb_ctrl = (u32 *)(USB_BASE_ADDR + USB_OTHERREGS_OFFSET +
				 port * 4);

	/* Set Power polarity */
	setbits_le32(usbnc_usb_ctrl, UCTRL_PWR_POL);

	return 0;
}
#endif
#endif

#define MICREL_KSZ9021_EXTREG_CTRL	0xB
#define MICREL_KSZ9021_EXTREG_DATA_WRITE	0xC
#define MICREL_KSZ9021_RGMII_CLK_CTRL_PAD_SCEW	0x104
#define MICREL_KSZ9021_RGMII_RX_DATA_PAD_SCEW	0x105

void ksz9021rn_phy_fixup(struct phy_device *phydev)
{
  printf("ksz9021rn_phy_fixup\n");
  phy_write(phydev, MDIO_DEVAD_NONE, MICREL_KSZ9021_EXTREG_CTRL, 0x8000 | MICREL_KSZ9021_RGMII_RX_DATA_PAD_SCEW);
  phy_write(phydev, MDIO_DEVAD_NONE, MICREL_KSZ9021_EXTREG_DATA_WRITE, 0x0000);

  phy_write(phydev, MDIO_DEVAD_NONE, MICREL_KSZ9021_EXTREG_CTRL, 0x8000 | MICREL_KSZ9021_RGMII_CLK_CTRL_PAD_SCEW);
  phy_write(phydev, MDIO_DEVAD_NONE, MICREL_KSZ9021_EXTREG_DATA_WRITE, 0xf0f0);

  phy_write(phydev, MDIO_DEVAD_NONE, MICREL_KSZ9021_EXTREG_CTRL, 0x8000 | 0x106);
  phy_write(phydev, MDIO_DEVAD_NONE, MICREL_KSZ9021_EXTREG_DATA_WRITE, 0x0000);
}

static void mmd_write_reg(struct phy_device *dev, int device, int reg, int val)
{
	phy_write(dev, MDIO_DEVAD_NONE, 0x0d, device);
	phy_write(dev, MDIO_DEVAD_NONE, 0x0e, reg);
	phy_write(dev, MDIO_DEVAD_NONE, 0x0d, (1 << 14) | device);
	phy_write(dev, MDIO_DEVAD_NONE, 0x0e, val);
}

static int ksz9031rn_phy_fixup(struct phy_device *phydev)
{
	printf("ksz9031rn_phy_fixup\n");

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
  unsigned short tmp1 = 0;
  unsigned short tmp2 = 0;

	if (miiphy_read("FEC0", CONFIG_FEC_MXC_PHYADDR, MII_PHYSID2, &tmp2) != 0) {
		debug("PHY ID register 3 read failed\n");
	}
	if (miiphy_read("FEC0", CONFIG_FEC_MXC_PHYADDR, MII_PHYSID1, &tmp1) != 0) {
		debug("PHY ID register 2 read failed\n");
	}

	unsigned short model = (tmp2>>4) & 0x3F;
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

#ifdef CONFIG_IMX_RDC
static rdc_peri_cfg_t const shared_resources[] = {
	(RDC_PER_GPIO1 | RDC_DOMAIN(0) | RDC_DOMAIN(1)),
};
#endif

int board_early_init_f(void)
{
#ifdef CONFIG_IMX_RDC
	imx_rdc_setup_peripherals(shared_resources, ARRAY_SIZE(shared_resources));
#endif

#ifdef CONFIG_SYS_AUXCORE_FASTUP
	arch_auxiliary_core_up(0, CONFIG_SYS_AUXCORE_BOOTDATA);
#endif

	setup_iomux_uart();

	/* Enable PERI_3V3, which is used by SD2, ENET, LVDS, BT */
	imx_iomux_v3_setup_multiple_pads(peri_3v3_pads,
					 ARRAY_SIZE(peri_3v3_pads));

	return 0;
}

#ifdef CONFIG_IMX_BOOTAUX
ulong board_get_usable_ram_top(ulong total_size)
{
	/* Reserve top 1M memory used by M core vring/buffer */
	return gd->ram_top - SZ_1M;
}
#endif

static struct fsl_esdhc_cfg usdhc_cfg[3] = {
	{USDHC2_BASE_ADDR, 0, 4},
	{USDHC3_BASE_ADDR, 0, 8},
#ifdef CONFIG_MX6SXSABRESD_EMMC_REWORK
	{USDHC4_BASE_ADDR, 0, 8},
#else
	{USDHC4_BASE_ADDR},
#endif
};

#define USDHC3_CD_GPIO	IMX_GPIO_NR(2, 10)
#define USDHC3_PWR_GPIO	IMX_GPIO_NR(2, 11)
#define USDHC4_CD_GPIO	IMX_GPIO_NR(6, 21)

int board_mmc_getcd(struct mmc *mmc)
{
	struct fsl_esdhc_cfg *cfg = (struct fsl_esdhc_cfg *)mmc->priv;
	int ret = 0;

	switch (cfg->esdhc_base) {
	case USDHC2_BASE_ADDR:
		ret = 1; /* Assume uSDHC2 is always present */
		break;
	case USDHC3_BASE_ADDR:
		ret = 1;
		break;
	case USDHC4_BASE_ADDR:
#ifdef CONFIG_MX6SXSABRESD_EMMC_REWORK
		ret = 1;
#else
		ret = !gpio_get_value(USDHC4_CD_GPIO);
#endif
		break;
	}

	return ret;
}

int board_mmc_init(struct bd_info *bis)
{
	int i, ret;
	/*
	 * According to the board_mmc_init() the following map is done:
	 * (U-Boot device node)    (Physical Port)
	 * mmc0                    USDHC2
	 * mmc1                    USDHC3
	 * mmc2                    USDHC4
	 */
	for (i = 0; i < CONFIG_SYS_FSL_USDHC_NUM; i++) {
		switch (i) {
		case 0:
			imx_iomux_v3_setup_multiple_pads(
				usdhc2_pads, ARRAY_SIZE(usdhc2_pads));
			usdhc_cfg[0].sdhc_clk = mxc_get_clock(MXC_ESDHC2_CLK);
			break;
		case 1:
			imx_iomux_v3_setup_multiple_pads(
				usdhc3_pads, ARRAY_SIZE(usdhc3_pads));
			usdhc_cfg[1].sdhc_clk = mxc_get_clock(MXC_ESDHC3_CLK);
			break;
		case 2:
#ifdef CONFIG_MX6SXSABRESD_EMMC_REWORK
			imx_iomux_v3_setup_multiple_pads(
				usdhc4_emmc_pads, ARRAY_SIZE(usdhc4_emmc_pads));
#else
			imx_iomux_v3_setup_multiple_pads(
				usdhc4_pads, ARRAY_SIZE(usdhc4_pads));
			gpio_request(USDHC4_CD_GPIO, "usdhc4 cd");
			gpio_direction_input(USDHC4_CD_GPIO);
#endif
			usdhc_cfg[2].sdhc_clk = mxc_get_clock(MXC_ESDHC4_CLK);
			break;
		default:
			printf("Warning: you configured more USDHC controllers"
				"(%d) than supported by the board\n", i + 1);
			return -EINVAL;
			}

			ret = fsl_esdhc_initialize(bis, &usdhc_cfg[i]);
			if (ret) {
				printf("Warning: failed to initialize mmc dev %d\n", i);
				return ret;
			}
	}
	return 0;
}

#ifdef CONFIG_FSL_QSPI

int board_qspi_init(void)
{
	/* Set the clock */
	enable_qspi_clk(1);

	return 0;
}
#endif

#ifdef CONFIG_CMD_BMODE
static const struct boot_mode board_boot_modes[] = {
	/* 4 bit bus width */
	{"sd3", MAKE_CFGVAL(0x42, 0x30, 0x00, 0x00)},
	{"sd4", MAKE_CFGVAL(0x40, 0x38, 0x00, 0x00)},
	{"qspi2", MAKE_CFGVAL(0x18, 0x00, 0x00, 0x00)},
	{NULL,	 0},
};
#endif

#ifdef CONFIG_VIDEO_MXS
static iomux_v3_cfg_t const lvds_ctrl_pads[] = {
	/* CABC enable */
	MX6_PAD_QSPI1A_DATA2__GPIO4_IO_18 | MUX_PAD_CTRL(NO_PAD_CTRL),

	/* Use GPIO for Brightness adjustment, duty cycle = period */
	MX6_PAD_SD1_DATA1__GPIO6_IO_3 | MUX_PAD_CTRL(NO_PAD_CTRL),
};

static iomux_v3_cfg_t const lcd_pads[] = {
	MX6_PAD_LCD1_CLK__LCDIF1_CLK | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD1_ENABLE__LCDIF1_ENABLE | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD1_HSYNC__LCDIF1_HSYNC | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD1_VSYNC__LCDIF1_VSYNC | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD1_DATA00__LCDIF1_DATA_0 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD1_DATA01__LCDIF1_DATA_1 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD1_DATA02__LCDIF1_DATA_2 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD1_DATA03__LCDIF1_DATA_3 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD1_DATA04__LCDIF1_DATA_4 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD1_DATA05__LCDIF1_DATA_5 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD1_DATA06__LCDIF1_DATA_6 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD1_DATA07__LCDIF1_DATA_7 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD1_DATA08__LCDIF1_DATA_8 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD1_DATA09__LCDIF1_DATA_9 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD1_DATA10__LCDIF1_DATA_10 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD1_DATA11__LCDIF1_DATA_11 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD1_DATA12__LCDIF1_DATA_12 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD1_DATA13__LCDIF1_DATA_13 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD1_DATA14__LCDIF1_DATA_14 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD1_DATA15__LCDIF1_DATA_15 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD1_DATA16__LCDIF1_DATA_16 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD1_DATA17__LCDIF1_DATA_17 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD1_DATA18__LCDIF1_DATA_18 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD1_DATA19__LCDIF1_DATA_19 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD1_DATA20__LCDIF1_DATA_20 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD1_DATA21__LCDIF1_DATA_21 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD1_DATA22__LCDIF1_DATA_22 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD1_DATA23__LCDIF1_DATA_23 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD1_RESET__GPIO3_IO_27 | MUX_PAD_CTRL(NO_PAD_CTRL),

	/* Use GPIO for Brightness adjustment, duty cycle = period */
	MX6_PAD_SD1_DATA2__GPIO6_IO_4 | MUX_PAD_CTRL(NO_PAD_CTRL),
};

void setup_lvds(void)
{
	int ret;

	ret = enable_lcdif_clock(LCDIF2_BASE_ADDR, 1);
	if (ret) {
		printf("Enable LCDIF clock failed, %d\n", ret);
		return;
	}
	ret = enable_lvds_clock(LCDIF2_BASE_ADDR);
	if (ret) {
		printf("Enable LVDS bridge failed, %d\n", ret);
		return;
	}

	imx_iomux_v3_setup_multiple_pads(lvds_ctrl_pads,
							ARRAY_SIZE(lvds_ctrl_pads));

	/* Enable CABC */
	gpio_request(IMX_GPIO_NR(4, 18), "CABC enable");
	gpio_direction_output(IMX_GPIO_NR(4, 18) , 1);

	/* Set Brightness to high */
	gpio_request(IMX_GPIO_NR(6, 3), "lvds backlight");
	gpio_direction_output(IMX_GPIO_NR(6, 3) , 1);
}

void setup_lcd(void)
{
	int ret;

	ret = enable_lcdif_clock(MX6SX_LCDIF1_BASE_ADDR, 1);
	if (ret) {
		printf("Enable LCDIF clock failed, %d\n", ret);
		return;
	}

	imx_iomux_v3_setup_multiple_pads(lcd_pads, ARRAY_SIZE(lcd_pads));

	/* Reset the LCD */
	gpio_request(IMX_GPIO_NR(3, 27), "lcd_rst");
	gpio_direction_output(IMX_GPIO_NR(3, 27) , 0);
	udelay(500);
	gpio_direction_output(IMX_GPIO_NR(3, 27) , 1);

	/* Set Brightness to high */
	gpio_request(IMX_GPIO_NR(6, 4), "lcd_bright");
	gpio_direction_output(IMX_GPIO_NR(6, 4) , 1);
}
#endif

int board_init(void)
{
	/* Address of boot parameters */
	gd->bd->bi_boot_params = PHYS_SDRAM + 0x100;

	/*
	 * Because kernel set WDOG_B mux before pad with the common pinctrl
	 * framwork now and wdog reset will be triggered once set WDOG_B mux
	 * with default pad setting, we set pad setting here to workaround this.
	 * Since imx_iomux_v3_setup_pad also set mux before pad setting, we set
	 * as GPIO mux firstly here to workaround it.
	 */
	imx_iomux_v3_setup_pad(wdog_b_pad);

#if defined(CONFIG_DM_REGULATOR)
	regulators_enable_boot_on(false);
#endif

#ifdef CONFIG_SYS_I2C_LEGACY
	setup_i2c(0, CONFIG_SYS_I2C_SPEED, 0x7f, &i2c_pad_info1);
	setup_i2c(1, CONFIG_SYS_I2C_SPEED, 0x7f, &i2c_pad_info2);
#endif

#ifdef CONFIG_USB_EHCI_MX6
#ifndef CONFIG_DM_USB
	setup_usb();
#endif
#endif

#ifdef CONFIG_FSL_QSPI
	board_qspi_init();
#endif

	/* Also used for OF_CONTROL enabled */
#ifdef CONFIG_FEC_MXC
	setup_fec();
#endif

#ifdef CONFIG_VIDEO_MXS
	setup_lvds();
	setup_lcd();
#endif

	return 0;
}

static bool is_reva(void)
{
	return (nxp_board_rev() == 1);
}

int board_late_init(void)
{
#ifdef CONFIG_ENV_VARS_UBOOT_RUNTIME_CONFIG
	if (is_reva())
		env_set("board_rev", "REVA");
#endif

#ifdef CONFIG_CMD_BMODE
	add_board_boot_modes(board_boot_modes);
#endif

	env_set("tee", "no");
#ifdef CONFIG_IMX_OPTEE
	env_set("tee", "yes");
#endif

#ifdef CONFIG_ENV_IS_IN_MMC
	board_late_mmc_env_init();
#endif
	return 0;
}

int checkboard(void)
{
#ifdef CONFIG_NXP_BOARD_REVISION
	printf("Board: MX6SX Engicam Smarcore rev%c\n", nxp_board_rev_string());
#else
       puts("Board: MX6SX SABRE SDB");
#endif
	return 0;
}

void board_quiesce_devices(void)
{
#if defined(CONFIG_VIDEO_MXS)
	enable_lcdif_clock(MX6SX_LCDIF1_BASE_ADDR, 0);
	enable_lcdif_clock(LCDIF2_BASE_ADDR, 0);
#endif
}
