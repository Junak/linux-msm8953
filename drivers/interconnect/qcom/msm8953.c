// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2018-2020 Linaro Ltd
 * Based on msm8916.c (author: Georgi Djakov <georgi.djakov@linaro.org>)
 */

#include <linux/clk.h>
#include <linux/device.h>
#include <linux/interconnect-provider.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/slab.h>

#include <dt-bindings/interconnect/qcom,msm8953.h>

#include "smd-rpm.h"

#define RPM_BUS_MASTER_REQ			0x73616d62
#define RPM_BUS_SLAVE_REQ			0x766c7362

#define BIMC_BKE_ENA_REG(qport)			(0x8300 + (qport) * 0x4000)
#define BIMC_BKE_ENA_MASK			GENMASK(1, 0)
#define BIMC_BKE_ENA_SHIFT			0
#define BIMC_BKE_HEALTH_REG(qport, hlvl)	(0x8340 + (qport) * 0x4000 \
						+ (hlvl) * 4)
#define BIMC_BKE_HEALTH_LIMIT_CMDS_MASK		GENMASK(31, 31)
#define BIMC_BKE_HEALTH_LIMIT_CMDS_SHIFT	31
#define BIMC_BKE_HEALTH_AREQPRIO_MASK		GENMASK(9, 8)
#define BIMC_BKE_HEALTH_AREQPRIO_SHIFT		8
#define BIMC_BKE_HEALTH_PRIOLVL_MASK		GENMASK(1, 0)
#define BIMC_BKE_HEALTH_PRIOLVL_SHIFT		0

#define NOC_QOS_PRIO_REG(qport)			(0x7008 + (qport) * 0x1000)
#define NOC_QOS_PRIO_P0_MASK			GENMASK(1, 0)
#define NOC_QOS_PRIO_P0_SHIFT			0
#define NOC_QOS_PRIO_P1_MASK			GENMASK(3, 2)
#define NOC_QOS_PRIO_P1_SHIFT			2

#define NOC_QOS_MODE_REG(qport)			(0x700c + (qport) * 0x1000)
#define NOC_QOS_MODE_MASK			GENMASK(1, 0)
#define NOC_QOS_MODE_SHIFT			0
#define NOC_QOS_MODE_FIXED			0
#define NOC_QOS_MODE_LIMITER			1
#define NOC_QOS_MODE_BYPASS			2
#define NOC_QOS_MODE_REGULATOR			3

#define NUM_BUS_CLKS				2

enum {
	QNOC_NODE_NONE = 0,
	QNOC_MASTER_AMPSS_M0,
	QNOC_MASTER_GRAPHICS_3D,
	QNOC_SNOC_BIMC_0_MAS,
	QNOC_SNOC_BIMC_2_MAS,
	QNOC_SNOC_BIMC_1_MAS,
	QNOC_MASTER_TCU_0,
	QNOC_SLAVE_EBI_CH0,
	QNOC_BIMC_SNOC_SLV,
	QNOC_MASTER_SPDM,
	QNOC_MASTER_BLSP_1,
	QNOC_MASTER_BLSP_2,
	QNOC_MASTER_USB3,
	QNOC_MASTER_CRYPTO_CORE0,
	QNOC_MASTER_SDCC_1,
	QNOC_MASTER_SDCC_2,
	QNOC_SNOC_PNOC_MAS,
	QNOC_PNOC_M_0,
	QNOC_PNOC_M_1,
	QNOC_PNOC_INT_1,
	QNOC_PNOC_INT_2,
	QNOC_PNOC_SLV_0,
	QNOC_PNOC_SLV_1,
	QNOC_PNOC_SLV_2,
	QNOC_PNOC_SLV_3,
	QNOC_PNOC_SLV_4,
	QNOC_PNOC_SLV_6,
	QNOC_PNOC_SLV_7,
	QNOC_PNOC_SLV_8,
	QNOC_PNOC_SLV_9,
	QNOC_SLAVE_SPDM_WRAPPER,
	QNOC_SLAVE_PDM,
	QNOC_SLAVE_TCSR,
	QNOC_SLAVE_SNOC_CFG,
	QNOC_SLAVE_TLMM,
	QNOC_SLAVE_MESSAGE_RAM,
	QNOC_SLAVE_BLSP_1,
	QNOC_SLAVE_BLSP_2,
	QNOC_SLAVE_PRNG,
	QNOC_SLAVE_CAMERA_CFG,
	QNOC_SLAVE_DISPLAY_CFG,
	QNOC_SLAVE_VENUS_CFG,
	QNOC_SLAVE_GRAPHICS_3D_CFG,
	QNOC_SLAVE_SDCC_1,
	QNOC_SLAVE_SDCC_2,
	QNOC_SLAVE_CRYPTO_0_CFG,
	QNOC_SLAVE_PMIC_ARB,
	QNOC_SLAVE_USB3,
	QNOC_SLAVE_IPA_CFG,
	QNOC_SLAVE_TCU,
	QNOC_PNOC_SNOC_SLV,
	QNOC_MASTER_QDSS_BAM,
	QNOC_BIMC_SNOC_MAS,
	QNOC_PNOC_SNOC_MAS,
	QNOC_MASTER_IPA,
	QNOC_MASTER_QDSS_ETR,
	QNOC_SNOC_QDSS_INT,
	QNOC_SNOC_INT_0,
	QNOC_SNOC_INT_1,
	QNOC_SNOC_INT_2,
	QNOC_SLAVE_APPSS,
	QNOC_SLAVE_WCSS,
	QNOC_SNOC_BIMC_1_SLV,
	QNOC_SLAVE_OCIMEM,
	QNOC_SNOC_PNOC_SLV,
	QNOC_SLAVE_QDSS_STM,
	QNOC_SLAVE_OCMEM_64,
	QNOC_SLAVE_LPASS,
	QNOC_MASTER_JPEG,
	QNOC_MASTER_MDP_PORT0,
	QNOC_MASTER_VIDEO_P0,
	QNOC_MASTER_VFE,
	QNOC_MASTER_VFE1,
	QNOC_MASTER_CPP,
	QNOC_SNOC_BIMC_0_SLV,
	QNOC_SNOC_BIMC_2_SLV,
	QNOC_SLAVE_CATS_128,
};

struct msm8953_icc_desc {
	struct msm8953_icc_node *nodes;
	int num_nodes;
	u64 max_bw;
	void (*node_qos_init)(struct msm8953_icc_node *, struct regmap*);
};

#define to_msm8953_provider(_provider) \
	container_of(_provider, struct msm8953_icc_provider, provider)

/**
 * struct msm8953_icc_provider - Qualcomm specific interconnect provider
 * @provider: generic interconnect provider
 * @bus_clks: the clk_bulk_data table of bus clocks
 * @num_bus_clks: the total number of clk_bulk_data
 * @rate: current bus clock rate in Hz
 */
struct msm8953_icc_provider {
	struct icc_provider provider;
	struct clk_bulk_data bus_clks[NUM_BUS_CLKS];
	const struct msm8953_icc_desc *desc;
	u64 bus_rate;
};

enum qos_mode {
	QOS_NONE,
	QOS_BYPASS,
	QOS_FIXED,
};

/**
 * struct msm8953_icc_node - Qualcomm specific interconnect nodes
 * @name: the node name used in debugfs
 * @id: a unique node identifier
 * @links: an array of nodes where we can go next while traversing
 * @qport: the offset index into the masters QoS register space
 * @port0, @port1: priority low/high signal for NoC or prioity level for BIMC
 * @qos_mode: QoS mode to be programmed for this device.
 * @buswidth: width of the interconnect between a node and the bus (bytes)
 * @mas_rpm_id:	RPM ID for devices that are bus masters
 * @slv_rpm_id:	RPM ID for devices that are bus slaves
 */
struct msm8953_icc_node {
	unsigned char *name;
	u16 id;
	u16 *links;
	u16 prio0;
	u16 prio1;
	u16 buswidth;
	u16 qport;
	enum qos_mode qos_mode;
	int mas_rpm_id;
	int slv_rpm_id;
	u64 rate;
};

static void msm8953_bimc_node_init(struct msm8953_icc_node *qn,
				  struct regmap* rmap);

static void msm8953_noc_node_init(struct msm8953_icc_node *qn,
				  struct regmap* rmap);

#define QNODE(_name, _id, _qport, _buswidth, ...)			\
		.name = _name,						\
		.id = _id,						\
		.qport = _qport,					\
		.buswidth = _buswidth,					\
		.links = (u16[]) { __VA_ARGS__, QNOC_NODE_NONE }	\

#define QNODE_AP(_name, _id, _qport, _buswidth, _qos_mode,		\
	      _prio0, _prio1, ...)					\
      {									\
	      QNODE(#_name, _id, _qport, _buswidth, __VA_ARGS__),	\
		.prio0 = _prio0,					\
		.prio1 = _prio1,					\
		.qos_mode = _qos_mode,					\
		.mas_rpm_id = -1,					\
		.slv_rpm_id = -1,					\
      }

#define QNODE_RPM(_name, _id, _qport, _buswidth,			\
		_mas_rpm_id, _slv_rpm_id, ...)				\
      {									\
	      QNODE(#_name, _id, _qport, _buswidth, __VA_ARGS__),	\
		.mas_rpm_id = _mas_rpm_id,				\
		.slv_rpm_id = _slv_rpm_id,				\
      }

static struct msm8953_icc_node msm8953_bimc_nodes[] = {
QNODE_AP(MAS_APPS_PROC, QNOC_MASTER_AMPSS_M0, 0, 8, QOS_FIXED, 0, 0,
	QNOC_SLAVE_EBI_CH0, QNOC_BIMC_SNOC_SLV),
QNODE_AP(MAS_OXILI, QNOC_MASTER_GRAPHICS_3D, 2, 8, QOS_FIXED, 0, 0,
	QNOC_SLAVE_EBI_CH0, QNOC_BIMC_SNOC_SLV),
QNODE_AP(MAS_SNOC_BIMC_0, QNOC_SNOC_BIMC_0_MAS, 3, 8, QOS_BYPASS, 0, 0,
	QNOC_SLAVE_EBI_CH0, QNOC_BIMC_SNOC_SLV),
QNODE_AP(MAS_SNOC_BIMC_2, QNOC_SNOC_BIMC_2_MAS, 4, 8, QOS_BYPASS, 0, 0,
	QNOC_SLAVE_EBI_CH0, QNOC_BIMC_SNOC_SLV),
QNODE_RPM(MAS_SNOC_BIMC_1, QNOC_SNOC_BIMC_1_MAS, 5, 8, 76, -1,
	QNOC_SLAVE_EBI_CH0),
QNODE_AP(MAS_TCU_0, QNOC_MASTER_TCU_0, 6, 8, QOS_FIXED, 2, 2,
	QNOC_SLAVE_EBI_CH0, QNOC_BIMC_SNOC_SLV),
QNODE_RPM(SLV_EBI, QNOC_SLAVE_EBI_CH0, 0, 8, -1, 0,
	QNOC_NODE_NONE),
QNODE_RPM(SLV_BIMC_SNOC, QNOC_BIMC_SNOC_SLV, 0, 8, -1, 2,
	QNOC_BIMC_SNOC_MAS),
};

static struct msm8953_icc_desc msm8953_bimc = {
	.nodes = msm8953_bimc_nodes,
	.num_nodes = ARRAY_SIZE(msm8953_bimc_nodes),
	.max_bw = 7448000000,
	.node_qos_init = msm8953_bimc_node_init,
};

static struct msm8953_icc_node msm8953_pcnoc_nodes[] = {
QNODE_AP(MAS_SPDM, QNOC_MASTER_SPDM, 0, 4, QOS_NONE, 0, 0,
	QNOC_PNOC_M_0),
QNODE_RPM(MAS_BLSP_1, QNOC_MASTER_BLSP_1, 0, 4, 41, -1,
	QNOC_PNOC_M_1),
QNODE_RPM(MAS_BLSP_2, QNOC_MASTER_BLSP_2, 0, 4, 39, -1,
	QNOC_PNOC_M_1),
QNODE_AP(MAS_USB3, QNOC_MASTER_USB3, 11, 8, QOS_FIXED, 1, 1,
	QNOC_PNOC_INT_1),
QNODE_AP(MAS_CRYPTO, QNOC_MASTER_CRYPTO_CORE0, 0, 8, QOS_FIXED, 1, 1,
	QNOC_PNOC_INT_1),
QNODE_RPM(MAS_SDCC_1, QNOC_MASTER_SDCC_1, 7, 8, 33, -1,
	QNOC_PNOC_INT_1),
QNODE_RPM(MAS_SDCC_2, QNOC_MASTER_SDCC_2, 8, 8, 35, -1,
	QNOC_PNOC_INT_1),
QNODE_RPM(MAS_SNOC_PCNOC, QNOC_SNOC_PNOC_MAS, 9, 8, 77, -1,
	QNOC_PNOC_INT_2),
QNODE_AP(PCNOC_M_0, QNOC_PNOC_M_0, 5, 4, QOS_FIXED, 1, 1,
	QNOC_PNOC_INT_1),
QNODE_RPM(PCNOC_M_1, QNOC_PNOC_M_1, 6, 4, 88, 117,
	QNOC_PNOC_INT_1),
QNODE_RPM(PCNOC_INT_1, QNOC_PNOC_INT_1, 0, 8, 86, 115,
	QNOC_PNOC_INT_2, QNOC_PNOC_SNOC_SLV),
QNODE_RPM(PCNOC_INT_2, QNOC_PNOC_INT_2, 0, 8, 124, 184,
	QNOC_PNOC_SLV_1, QNOC_PNOC_SLV_2, QNOC_PNOC_SLV_0,
	QNOC_PNOC_SLV_4, QNOC_PNOC_SLV_6, QNOC_PNOC_SLV_7,
	QNOC_PNOC_SLV_8, QNOC_PNOC_SLV_9, QNOC_SLAVE_TCU,
	QNOC_SLAVE_GRAPHICS_3D_CFG, QNOC_PNOC_SLV_3),
QNODE_RPM(PCNOC_S_0, QNOC_PNOC_SLV_0, 0, 4, 89, 118,
	QNOC_SLAVE_PDM, QNOC_SLAVE_SPDM_WRAPPER),
QNODE_RPM(PCNOC_S_1, QNOC_PNOC_SLV_1, 0, 4, 90, 119,
	QNOC_SLAVE_TCSR),
QNODE_RPM(PCNOC_S_2, QNOC_PNOC_SLV_2, 0, 4, 91, 120,
	QNOC_SLAVE_SNOC_CFG),
QNODE_RPM(PCNOC_S_3, QNOC_PNOC_SLV_3, 0, 4, 92, 121,
	QNOC_SLAVE_TLMM, QNOC_SLAVE_PRNG, QNOC_SLAVE_BLSP_1,
	QNOC_SLAVE_BLSP_2, QNOC_SLAVE_MESSAGE_RAM),
QNODE_AP(PCNOC_S_4, QNOC_PNOC_SLV_4, 0, 4, QOS_NONE, 0, 0,
	QNOC_SLAVE_CAMERA_CFG, QNOC_SLAVE_DISPLAY_CFG, QNOC_SLAVE_VENUS_CFG),
QNODE_RPM(PCNOC_S_6, QNOC_PNOC_SLV_6, 0, 4, 94, 123,
	QNOC_SLAVE_CRYPTO_0_CFG, QNOC_SLAVE_SDCC_2, QNOC_SLAVE_SDCC_1),
QNODE_RPM(PCNOC_S_7, QNOC_PNOC_SLV_7, 0, 4, 95, 124,
	QNOC_SLAVE_PMIC_ARB),
QNODE_AP(PCNOC_S_8, QNOC_PNOC_SLV_8, 0, 4, QOS_NONE, 0, 0,
	QNOC_SLAVE_USB3),
QNODE_AP(PCNOC_S_9, QNOC_PNOC_SLV_9, 0, 4, QOS_NONE, 0, 0,
	QNOC_SLAVE_IPA_CFG),
QNODE_AP(SLV_SPDM, QNOC_SLAVE_SPDM_WRAPPER, 0, 4, QOS_NONE, 0, 0,
	QNOC_NODE_NONE),
QNODE_RPM(SLV_PDM, QNOC_SLAVE_PDM, 0, 4, -1, 41,
	QNOC_NODE_NONE),
QNODE_RPM(SLV_TCSR, QNOC_SLAVE_TCSR, 0, 4, -1, 50,
	QNOC_NODE_NONE),
QNODE_RPM(SLV_SNOC_CFG, QNOC_SLAVE_SNOC_CFG, 0, 4, -1, 70,
	QNOC_NODE_NONE),
QNODE_RPM(SLV_TLMM, QNOC_SLAVE_TLMM, 0, 4, -1, 51,
	QNOC_NODE_NONE),
QNODE_RPM(SLV_MESSAGE_RAM, QNOC_SLAVE_MESSAGE_RAM, 0, 4, -1, 55,
	QNOC_NODE_NONE),
QNODE_RPM(SLV_BLSP_1, QNOC_SLAVE_BLSP_1, 0, 4, -1, 39,
	QNOC_NODE_NONE),
QNODE_RPM(SLV_BLSP_2, QNOC_SLAVE_BLSP_2, 0, 4, -1, 37,
	QNOC_NODE_NONE),
QNODE_RPM(SLV_PRNG, QNOC_SLAVE_PRNG, 0, 4, -1, 44,
	QNOC_NODE_NONE),
QNODE_AP(SLV_CAMERA_SS_CFG, QNOC_SLAVE_CAMERA_CFG, 0, 4, QOS_NONE, 0, 0,
	QNOC_NODE_NONE),
QNODE_AP(SLV_DISP_SS_CFG, QNOC_SLAVE_DISPLAY_CFG, 0, 4, QOS_NONE, 0, 0,
	QNOC_NODE_NONE),
QNODE_AP(SLV_VENUS_CFG, QNOC_SLAVE_VENUS_CFG, 0, 4, QOS_NONE, 0, 0,
	QNOC_NODE_NONE),
QNODE_AP(SLV_GPU_CFG, QNOC_SLAVE_GRAPHICS_3D_CFG, 0, 8, QOS_NONE, 0, 0,
	QNOC_NODE_NONE),
QNODE_RPM(SLV_SDCC_1, QNOC_SLAVE_SDCC_1, 0, 4, -1, 31,
	QNOC_NODE_NONE),
QNODE_RPM(SLV_SDCC_2, QNOC_SLAVE_SDCC_2, 0, 4, -1, 33,
	QNOC_NODE_NONE),
QNODE_AP(SLV_CRYPTO_0_CFG, QNOC_SLAVE_CRYPTO_0_CFG, 0, 4, QOS_NONE, 0, 0,
	QNOC_NODE_NONE),
QNODE_RPM(SLV_PMIC_ARB, QNOC_SLAVE_PMIC_ARB, 0, 4, -1, 59,
	QNOC_NODE_NONE),
QNODE_AP(SLV_USB3, QNOC_SLAVE_USB3, 0, 4, QOS_NONE, 0, 0,
	QNOC_NODE_NONE),
QNODE_AP(SLV_IPA_CFG, QNOC_SLAVE_IPA_CFG, 0, 4, QOS_NONE, 0, 0,
	QNOC_NODE_NONE),
QNODE_AP(SLV_TCU, QNOC_SLAVE_TCU, 0, 8, QOS_NONE, 0, 0,
	QNOC_NODE_NONE),
QNODE_RPM(SLV_PCNOC_SNOC, QNOC_PNOC_SNOC_SLV, 0, 8, -1, 45,
	QNOC_PNOC_SNOC_MAS),
};

static struct msm8953_icc_desc msm8953_pcnoc = {
	.nodes = msm8953_pcnoc_nodes,
	.num_nodes = ARRAY_SIZE(msm8953_pcnoc_nodes),
	.max_bw = 1064000000,
	.node_qos_init = msm8953_noc_node_init,
};

static struct msm8953_icc_node msm8953_snoc_nodes[] = {
QNODE_AP(MAS_QDSS_BAM, QNOC_MASTER_QDSS_BAM, 11, 4, QOS_FIXED, 1, 1,
	QNOC_SNOC_QDSS_INT),
QNODE_RPM(MAS_BIMC_SNOC, QNOC_BIMC_SNOC_MAS, 0, 8, 21, -1,
	QNOC_SNOC_INT_0, QNOC_SNOC_INT_1, QNOC_SNOC_INT_2),
QNODE_RPM(MAS_PCNOC_SNOC, QNOC_PNOC_SNOC_MAS, 5, 8, 29, -1,
	QNOC_SNOC_INT_0, QNOC_SNOC_INT_1, QNOC_SNOC_BIMC_1_SLV),
QNODE_AP(MAS_IPA, QNOC_MASTER_IPA, 14, 8, QOS_FIXED, 0, 0,
	QNOC_SNOC_INT_0, QNOC_SNOC_INT_1, QNOC_SNOC_BIMC_1_SLV),
QNODE_AP(MAS_QDSS_ETR, QNOC_MASTER_QDSS_ETR, 10, 8, QOS_FIXED, 1, 1,
	QNOC_SNOC_QDSS_INT),
QNODE_AP(QDSS_INT, QNOC_SNOC_QDSS_INT, 0, 8, QOS_NONE, 0, 0,
	QNOC_SNOC_INT_1, QNOC_SNOC_BIMC_1_SLV),
QNODE_AP(SNOC_INT_0, QNOC_SNOC_INT_0, 0, 8, QOS_NONE, 0, 0,
	QNOC_SLAVE_LPASS, QNOC_SLAVE_WCSS, QNOC_SLAVE_APPSS),
QNODE_RPM(SNOC_INT_1, QNOC_SNOC_INT_1, 0, 8, 100, 131,
	QNOC_SLAVE_QDSS_STM, QNOC_SLAVE_OCIMEM, QNOC_SNOC_PNOC_SLV),
QNODE_AP(SNOC_INT_2, QNOC_SNOC_INT_2, 0, 8, QOS_NONE, 0, 0,
	QNOC_SLAVE_CATS_128, QNOC_SLAVE_OCMEM_64),
QNODE_AP(SLV_KPSS_AHB, QNOC_SLAVE_APPSS, 0, 4, QOS_NONE, 0, 0,
	QNOC_NODE_NONE),
QNODE_AP(SLV_WCSS, QNOC_SLAVE_WCSS, 0, 4, QOS_NONE, 0, 0,
	QNOC_NODE_NONE),
QNODE_RPM(SLV_SNOC_BIMC_1, QNOC_SNOC_BIMC_1_SLV, 0, 8, -1, 104,
	QNOC_SNOC_BIMC_1_MAS),
QNODE_RPM(SLV_IMEM, QNOC_SLAVE_OCIMEM, 0, 8, -1, 26,
	QNOC_NODE_NONE),
QNODE_RPM(SLV_SNOC_PCNOC, QNOC_SNOC_PNOC_SLV, 0, 8, -1, 28,
	QNOC_SNOC_PNOC_MAS),
QNODE_RPM(SLV_QDSS_STM, QNOC_SLAVE_QDSS_STM, 0, 4, -1, 30,
	QNOC_NODE_NONE),
QNODE_AP(SLV_CATS_1, QNOC_SLAVE_OCMEM_64, 0, 8, QOS_NONE, 0, 0,
	QNOC_NODE_NONE),
QNODE_AP(SLV_LPASS, QNOC_SLAVE_LPASS, 0, 4, QOS_NONE, 0, 0,
	QNOC_NODE_NONE),
};

static struct msm8953_icc_desc msm8953_snoc = {
	.nodes = msm8953_snoc_nodes,
	.num_nodes = ARRAY_SIZE(msm8953_snoc_nodes),
	.max_bw = 2134000000,
	.node_qos_init = msm8953_noc_node_init,
};

static struct msm8953_icc_node msm8953_sysmmnoc_nodes[] = {
QNODE_AP(MAS_JPEG, QNOC_MASTER_JPEG, 6, 16, QOS_BYPASS, 0, 0,
	QNOC_SNOC_BIMC_2_SLV),
QNODE_AP(MAS_MDP, QNOC_MASTER_MDP_PORT0, 7, 16, QOS_BYPASS, 0, 0,
	QNOC_SNOC_BIMC_0_SLV),
QNODE_AP(MAS_VENUS, QNOC_MASTER_VIDEO_P0, 8, 16, QOS_BYPASS, 0, 0,
	QNOC_SNOC_BIMC_2_SLV),
QNODE_AP(MAS_VFE0, QNOC_MASTER_VFE, 9, 16, QOS_BYPASS, 0, 0,
	QNOC_SNOC_BIMC_0_SLV),
QNODE_AP(MAS_VFE1, QNOC_MASTER_VFE1, 13, 16, QOS_BYPASS, 0, 0,
	QNOC_SNOC_BIMC_0_SLV),
QNODE_AP(MAS_CPP, QNOC_MASTER_CPP, 12, 16, QOS_BYPASS, 0, 0,
	QNOC_SNOC_BIMC_2_SLV),
QNODE_AP(SLV_SNOC_BIMC_0, QNOC_SNOC_BIMC_0_SLV, 0, 16, QOS_NONE, 0, 0,
	QNOC_SNOC_BIMC_0_MAS),
QNODE_AP(SLV_SNOC_BIMC_2, QNOC_SNOC_BIMC_2_SLV, 0, 16, QOS_NONE, 0, 0,
	QNOC_SNOC_BIMC_2_MAS),
QNODE_AP(SLV_CATS_0, QNOC_SLAVE_CATS_128, 0, 16, QOS_NONE, 0, 0,
	QNOC_NODE_NONE),
};

static struct msm8953_icc_desc msm8953_sysmmnoc = {
	.nodes = msm8953_sysmmnoc_nodes,
	.num_nodes = ARRAY_SIZE(msm8953_sysmmnoc_nodes),
	.max_bw = 2880000000,
	.node_qos_init = msm8953_noc_node_init,
};

static void msm8953_bimc_node_init(struct msm8953_icc_node *qn,
				  struct regmap* rmap)
{
	int health_lvl;
	u32 bke_en = 0;

	switch (qn->qos_mode) {
	case QOS_FIXED:
		for (health_lvl = 0; health_lvl < 4; health_lvl++) {
			regmap_update_bits(rmap, BIMC_BKE_HEALTH_REG(qn->qport, health_lvl),
						 BIMC_BKE_HEALTH_AREQPRIO_MASK,
						 qn->prio1 << BIMC_BKE_HEALTH_AREQPRIO_SHIFT);

			regmap_update_bits(rmap, BIMC_BKE_HEALTH_REG(qn->qport, health_lvl),
						 BIMC_BKE_HEALTH_PRIOLVL_MASK,
						 qn->prio0 << BIMC_BKE_HEALTH_PRIOLVL_SHIFT);

			if (health_lvl < 3)
				regmap_update_bits(rmap,
						   BIMC_BKE_HEALTH_REG(qn->qport, health_lvl),
						   BIMC_BKE_HEALTH_LIMIT_CMDS_MASK, 0);
		}
		bke_en = 1 << BIMC_BKE_ENA_SHIFT;
		break;
	case QOS_BYPASS:
		break;
	default:
		return;
	}

	regmap_update_bits(rmap, BIMC_BKE_ENA_REG(qn->qport), BIMC_BKE_ENA_MASK, bke_en);
}

static void msm8953_noc_node_init(struct msm8953_icc_node *qn,
				  struct regmap* rmap)
{
	u32 mode = 0;

	switch (qn->qos_mode) {
	case QOS_BYPASS:
		mode = NOC_QOS_MODE_BYPASS;
		break;
	case QOS_FIXED:
		regmap_update_bits(rmap,
				NOC_QOS_PRIO_REG(qn->qport),
				NOC_QOS_PRIO_P0_MASK,
				qn->prio0 & NOC_QOS_PRIO_P0_SHIFT);
		regmap_update_bits(rmap,
				NOC_QOS_PRIO_REG(qn->qport),
				NOC_QOS_PRIO_P1_MASK,
				qn->prio1 & NOC_QOS_PRIO_P0_SHIFT);
		mode = NOC_QOS_MODE_FIXED;
		break;
	default:
		return;
	}

	regmap_update_bits(rmap,
			   NOC_QOS_MODE_REG(qn->qport),
			   NOC_QOS_MODE_MASK,
			   mode);
}

static void msm8953_qnoc_update_bus_clk(struct msm8953_icc_provider *qp,
					struct msm8953_icc_node *qn, u64 rate)
{
	struct icc_node *n;
	int i, ret;

	rate = min(rate, qp->desc->max_bw / 8);

	if (rate == qn->rate)
		return;

	qn->rate = rate;

	if (qp->bus_rate > rate)
		list_for_each_entry(n, &qp->provider.nodes, node_list) {
			struct msm8953_icc_node *qn = n->data;
			rate = max(qn->rate, rate);
		}

	if (qp->bus_rate == rate)
		return;

	for (i = 0; i < NUM_BUS_CLKS; i++) {
		ret = clk_set_rate(qp->bus_clks[i].clk, rate);
		if (ret) {
			dev_err(qp->provider.dev, "clk_set_rate error: %d\n", ret);
			return;
		}
	}

	qp->bus_rate = rate;
}

static int msm8953_get_bw(struct icc_node *node, u32 *avg, u32 *peak)
{
	struct msm8953_icc_provider *qp = to_msm8953_provider(node->provider);

	*avg = qp->desc->max_bw;
	*peak = qp->desc->max_bw;

	return 0;
}

static int msm8953_rpm_send_req(struct device *dev, u32 type, int rpm_id, u64 bw)
{
	int ret;

	if (rpm_id < 0)
		return 0;

	ret = qcom_icc_rpm_smd_send(QCOM_SMD_RPM_ACTIVE_STATE,
			type, rpm_id, bw);
	if (ret)
		dev_err(dev, "qcom_icc_rpm_smd_send (%s) rpm_id=%d error %d\n",
				type == RPM_BUS_MASTER_REQ ? "master" : "slave", rpm_id, ret);
	return ret;
}

static int msm8953_node_set(struct icc_node *node)
{
	struct msm8953_icc_provider *qp = to_msm8953_provider(node->provider);
	struct msm8953_icc_node *qn = node->data;
	struct device *dev = node->provider->dev;
	u64 avg_bw, peak_bw;
	int ret;

	avg_bw = min(qp->desc->max_bw, icc_units_to_bps(node->avg_bw));

	/* send bandwidth request message to the RPM processor */
	ret = msm8953_rpm_send_req(dev, RPM_BUS_MASTER_REQ, qn->mas_rpm_id, avg_bw);
	if (ret)
		return ret;

	ret = msm8953_rpm_send_req(dev, RPM_BUS_SLAVE_REQ, qn->slv_rpm_id, avg_bw);
	if (ret)
		return ret;

	peak_bw = min(qp->desc->max_bw, icc_units_to_bps(node->peak_bw));

	msm8953_qnoc_update_bus_clk(qp, qn, max(avg_bw, peak_bw) / qn->buswidth);

	return 0;
}

static int msm8953_icc_set(struct icc_node *src, struct icc_node *dst)
{
	int ret;

	ret = msm8953_node_set(src);
	if (ret)
		return ret;

	if (src != dst) {
		ret = msm8953_node_set(dst);
		if (ret)
			return ret;
	}

	return 0;
}

static int msm8953_qnoc_probe(struct platform_device *pdev)
{
	struct msm8953_icc_node *qnodes;
	struct msm8953_icc_provider *qp;
	struct device *dev = &pdev->dev;
	struct icc_onecell_data *data;
	struct icc_provider *provider;
	struct icc_node *node;
	struct clk *qos_clk = NULL;
	struct regmap *regmap;
	size_t num_nodes, i;
	void __iomem *base;
	int ret;

	/* wait for the RPM proxy */
	if (!qcom_icc_rpm_smd_available())
		return -EPROBE_DEFER;

	qp = devm_kzalloc(dev, sizeof(*qp),
			  GFP_KERNEL);
	if (!qp)
		return -ENOMEM;

	qp->bus_clks[0].id = "bus";
	qp->bus_clks[1].id = "bus_a";
	qp->desc = of_device_get_match_data(dev);

	if (!qp->desc)
		return -EINVAL;

	if (qp->desc->node_qos_init) {
		struct resource *res;
		struct regmap_config icc_regmap_cfg = {
			.reg_bits = 32,
			.reg_stride = 4,
			.val_bits = 32,
			.fast_io = true,
		};

		res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

		base = devm_ioremap(&pdev->dev, res->start, resource_size(res));
		if (IS_ERR(base))
			return PTR_ERR(base);

		icc_regmap_cfg.max_register = resource_size(res) - 4;

		regmap = regmap_init_mmio(&pdev->dev, base, &icc_regmap_cfg);
		if (IS_ERR(regmap))
			return PTR_ERR(regmap);

		if (of_device_is_compatible(dev->of_node, "qcom,msm8953-pcnoc")) {
			qos_clk = devm_clk_get(dev, "pcnoc_usb3_axi");
			if (IS_ERR(qos_clk)) {
				ret = PTR_ERR(qos_clk);
				goto err_free;
			}
		}
	}

	qnodes = qp->desc->nodes;
	num_nodes = qp->desc->num_nodes;

	data = devm_kzalloc(dev, struct_size(data, nodes, num_nodes),
			    GFP_KERNEL);
	if (!data) {
		ret = -ENOMEM;
		goto err_free;
	}

	ret = devm_clk_bulk_get(dev, NUM_BUS_CLKS, qp->bus_clks);
	if (ret)
		goto err_free;

	ret = clk_bulk_prepare_enable(NUM_BUS_CLKS, qp->bus_clks);
	if (ret)
		goto err_free;

	provider = &qp->provider;
	INIT_LIST_HEAD(&provider->nodes);
	provider->dev = dev;
	provider->set = msm8953_icc_set;
	provider->get_bw = msm8953_get_bw;
	provider->aggregate = icc_std_aggregate;
	provider->xlate = of_icc_xlate_onecell;
	provider->data = data;

	ret = icc_provider_add(provider);
	if (ret) {
		dev_err(dev, "error adding interconnect provider: %d\n", ret);
		clk_bulk_disable_unprepare(NUM_BUS_CLKS, qp->bus_clks);
		goto err_free;
	}

	for (i = 0; i < num_nodes; i++) {
		if (WARN_ON(!qnodes->buswidth))
			return -EINVAL;

		if (!qp->desc->node_qos_init || qnodes[i].qos_mode == QOS_NONE)
			continue;

		clk_prepare_enable(qos_clk);
		qp->desc->node_qos_init(&qnodes[i], regmap);
		clk_disable_unprepare(qos_clk);
	}

	for (i = 0; i < num_nodes; i++) {
		u16 *link;

		node = icc_node_create(qnodes[i].id);
		if (IS_ERR(node)) {
			ret = PTR_ERR(node);
			goto err;
		}

		node->name = qnodes[i].name;
		node->data = &qnodes[i];
		icc_node_add(node, provider);

		for (link = qnodes[i].links; *link != QNOC_NODE_NONE; link++)
			icc_link_create(node, *link);

		data->nodes[i] = node;
	}

	data->num_nodes = num_nodes;

	platform_set_drvdata(pdev, qp);

	kfree(regmap);

	if (!IS_ERR_OR_NULL(base))
		devm_iounmap(dev, base);

	return 0;
err:
	icc_nodes_remove(provider);
	icc_provider_del(provider);
	clk_bulk_disable_unprepare(NUM_BUS_CLKS, qp->bus_clks);

err_free:
	kfree(regmap);

	return ret;
}

static int msm8953_qnoc_remove(struct platform_device *pdev)
{
	struct msm8953_icc_provider *qp = platform_get_drvdata(pdev);

	icc_nodes_remove(&qp->provider);
	clk_bulk_disable_unprepare(NUM_BUS_CLKS, qp->bus_clks);
	return icc_provider_del(&qp->provider);
}

static const struct of_device_id msm8953_noc_of_match[] = {
	{ .compatible = "qcom,msm8953-bimc", .data = &msm8953_bimc },
	{ .compatible = "qcom,msm8953-pcnoc", .data = &msm8953_pcnoc },
	{ .compatible = "qcom,msm8953-snoc", .data = &msm8953_snoc },
	{ .compatible = "qcom,msm8953-sysmmnoc", .data = &msm8953_sysmmnoc },
	{ }
};
MODULE_DEVICE_TABLE(of, msm8953_noc_of_match);

static struct platform_driver msm8953_noc_driver = {
	.probe = msm8953_qnoc_probe,
	.remove = msm8953_qnoc_remove,
	.driver = {
		.name = "qnoc-msm8953",
		.of_match_table = msm8953_noc_of_match,
		.sync_state = icc_sync_state,
	},
};
module_platform_driver(msm8953_noc_driver);
MODULE_DESCRIPTION("Qualcomm MSM8953 NoC driver");
MODULE_LICENSE("GPL v2");
