#
# Copyright 2021 Engicam SRL
#
# SPDX-License-Identifier:      GPL-2.0+
#

obj-y += imx8mm_icore.o
obj-y += ../../freescale/common/

ifdef CONFIG_SPL_BUILD
obj-y += spl.o
ifdef CONFIG_IMX8M_2G_LPDDR4
obj-y += lpddr4_timing_2gb.o
else
obj-$(CONFIG_IMX8M_LPDDR4) += lpddr4_timing.o
endif
endif
