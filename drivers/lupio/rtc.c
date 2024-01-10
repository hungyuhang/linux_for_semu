// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020 Joël Porquet-Lupine <joel@porquet.org>
 */
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/rtc.h>

#include <asm/io.h>

/*
 * Driver for LupIO-RTC
 */

struct lupio_rtc_data {
	void __iomem *virt_base;
	struct rtc_device *rtc;
};

/* LupIO-RTC register map */
#define LUPIO_RTC_SECD	0x0
#define LUPIO_RTC_MINT	0x1
#define LUPIO_RTC_HOUR	0x2
#define LUPIO_RTC_DYMO	0x3
#define LUPIO_RTC_MNTH	0x4
#define LUPIO_RTC_YEAR	0x5
#define LUPIO_RTC_CENT	0x6
#define LUPIO_RTC_DYWK	0x7
#define LUPIO_RTC_DYYR	0x8

/*
 * RTC driver
 */

static int lupio_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
	struct lupio_rtc_data *rtc = dev_get_drvdata(dev);
	void __iomem *base = rtc->virt_base;

	/* Fill out `tm` object */
	tm->tm_sec	= readb(base + LUPIO_RTC_SECD);
	tm->tm_min 	= readb(base + LUPIO_RTC_MINT);
	tm->tm_hour	= readb(base + LUPIO_RTC_HOUR);
	tm->tm_mday 	= readb(base + LUPIO_RTC_DYMO);
	tm->tm_mon	= readb(base + LUPIO_RTC_MNTH) - 1;
	tm->tm_year	= readb(base + LUPIO_RTC_YEAR)
		+ readb(base + LUPIO_RTC_CENT) * 100
		- 1900;
	tm->tm_wday	= readb(base + LUPIO_RTC_DYWK) % 6;
	tm->tm_yday	= readb(base + LUPIO_RTC_DYYR) - 1;
	tm->tm_isdst	= -1; /* Unavailable */

	return 0;
}

static int lupio_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
	return -EOPNOTSUPP;
}

static const struct rtc_class_ops lupio_rtc_ops = {
	.read_time = lupio_rtc_read_time,
	.set_time = lupio_rtc_set_time,
};

/*
 * Platform driver
 */
static int lupio_rtc_pf_probe(struct platform_device *pdev)
{
	struct lupio_rtc_data *lupio_rtc;

	lupio_rtc = devm_kzalloc(&pdev->dev, sizeof(struct lupio_rtc_data),
				 GFP_KERNEL);
	if (!lupio_rtc) {
		dev_err(&pdev->dev, "failed to allocate memory for %s node\n",
			pdev->name);
		return -ENOMEM;
	}

	lupio_rtc->virt_base = devm_platform_ioremap_resource(pdev, 0);
	if (!lupio_rtc->virt_base) {
		dev_err(&pdev->dev, "failed to ioremap_resource for %s node\n",
			pdev->name);
		return -EADDRNOTAVAIL;
	}

	lupio_rtc->rtc = devm_rtc_allocate_device(&pdev->dev);
	if (IS_ERR(lupio_rtc->rtc))
		return PTR_ERR(lupio_rtc->rtc);

	lupio_rtc->rtc->ops = &lupio_rtc_ops;

	platform_set_drvdata(pdev, lupio_rtc);

	return devm_rtc_register_device(lupio_rtc->rtc);
}

static const struct of_device_id lupio_rtc_pf_of_ids[] = {
	{ .compatible = "lupio,rtc" },
	{}
};
MODULE_DEVICE_TABLE(of, lupio_rtc_pf_of_ids);

static struct platform_driver lupio_rtc_pf_driver = {
	.driver = {
		.owner 		= THIS_MODULE,
		.name		= "lupio_rtc",
		.of_match_table = lupio_rtc_pf_of_ids,
	},
	.probe	= lupio_rtc_pf_probe,
};

module_platform_driver(lupio_rtc_pf_driver);

/* MODULE information */
MODULE_AUTHOR("Joël Porquet-Lupine <joel@porquet.org>");
MODULE_DESCRIPTION("LupIO-RTC driver");
MODULE_LICENSE("GPL");
