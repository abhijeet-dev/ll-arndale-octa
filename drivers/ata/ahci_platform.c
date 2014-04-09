/*
 * AHCI SATA platform driver
 *
 * Copyright 2004-2005  Red Hat, Inc.
 *   Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2010  MontaVista Software, LLC.
 *   Anton Vorontsov <avorontsov@ru.mvista.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pm.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/libata.h>
#include <linux/ahci_platform.h>
#include "ahci.h"

enum ahci_type {
	AHCI,		/* standard platform ahci */
	HIP04_AHCI,	/* ahci on HiP04 */
};

static const struct ata_port_info ahci_port_info[] = {
	[AHCI] = {
		.flags		= AHCI_FLAG_COMMON,
		.pio_mask	= ATA_PIO4,
		.udma_mask	= ATA_UDMA6,
		.port_ops	= &ahci_platform_ops,
	},
	[HIP04_AHCI] = {
		AHCI_HFLAGS	(AHCI_HFLAG_NO_FBS),
		.flags		= AHCI_FLAG_COMMON,
		.pio_mask	= ATA_PIO4,
		.udma_mask	= ATA_UDMA6,
		.port_ops	= &ahci_platform_ops,
	},
};

static const struct of_device_id ahci_of_match[] = {
	{ .compatible = "snps,spear-ahci", },
	{ .compatible = "snps,exynos5440-ahci", },
	{ .compatible = "ibm,476gtr-ahci", },
	{ .compatible = "snps,dwc-ahci", },
	{ .compatible = "hisilicon,hisi-ahci", .data = (void *)HIP04_AHCI, },
	{},
};
MODULE_DEVICE_TABLE(of, ahci_of_match);

static int ahci_match_of_id(struct platform_device *pdev,
			    enum ahci_type *ahci_type)
{
	const struct of_device_id *of_id = of_match_device(ahci_of_match,
							   &pdev->dev);
	if (!of_id)
		return -ENODEV;
	*ahci_type = (unsigned long)(of_id->data);
	return 0;
}

static int ahci_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct ahci_platform_data *pdata = dev_get_platdata(dev);
	struct ahci_host_priv *hpriv;
	struct ata_port_info pi;
	enum ahci_type ahci_type;
	int rc;

	hpriv = ahci_platform_get_resources(pdev);
	if (IS_ERR(hpriv))
		return PTR_ERR(hpriv);

	rc = ahci_platform_enable_resources(hpriv);
	if (rc)
		return rc;

	/*
	 * Some platforms might need to prepare for mmio region access,
	 * which could be done in the following init call. So, the mmio
	 * region shouldn't be accessed before init (if provided) has
	 * returned successfully.
	 */
	if (pdata && pdata->init) {
		rc = pdata->init(dev, hpriv->mmio);
		if (rc)
			goto disable_resources;
	}

	if (!ahci_match_of_id(pdev, &ahci_type))
		pi = ahci_port_info[ahci_type];
	else
		pi = ahci_port_info[AHCI];

	rc = ahci_platform_init_host(pdev, hpriv, &pi, 0, 0);
	if (rc)
		goto pdata_exit;

	return 0;
pdata_exit:
	if (pdata && pdata->exit)
		pdata->exit(dev);
disable_resources:
	ahci_platform_disable_resources(hpriv);
	return rc;
}

static SIMPLE_DEV_PM_OPS(ahci_pm_ops, ahci_platform_suspend,
			 ahci_platform_resume);

static struct platform_driver ahci_driver = {
	.probe = ahci_probe,
	.remove = ata_platform_remove_one,
	.driver = {
		.name = "ahci",
		.owner = THIS_MODULE,
		.of_match_table = ahci_of_match,
		.pm = &ahci_pm_ops,
	},
};
module_platform_driver(ahci_driver);

MODULE_DESCRIPTION("AHCI SATA platform driver");
MODULE_AUTHOR("Anton Vorontsov <avorontsov@ru.mvista.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:ahci");
