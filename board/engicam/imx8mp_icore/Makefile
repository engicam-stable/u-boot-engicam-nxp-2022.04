#
# Copyright 2019 NXP
#
# SPDX-License-Identifier:      GPL-2.0+
#

obj-y += imx8mp_icore.o
obj-y += ../../freescale/common/

ifdef CONFIG_SPL_BUILD
obj-y += spl.o
ifdef CONFIG_IMX8MP_1GB_LPDDR4
	obj-$(CONFIG_IMX8M_LPDDR4) += lpddr4_1gb_timing.o
else ifdef CONFIG_IMX8MP_4GB_LPDDR4
	obj-$(CONFIG_IMX8M_LPDDR4) += lpddr4_4gb_timing.o
else
	obj-$(CONFIG_IMX8M_LPDDR4) += lpddr4_timing.o
endif

endif
