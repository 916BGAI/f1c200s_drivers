#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <asm-generic/gpio.h>
#include <linux/kdev_t.h>
#include <linux/kern_levels.h>
#include <linux/printk.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/ioport.h>
#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/export.h>
#include <linux/interrupt.h>
#include <linux/delay.h>

struct gpio_desc *key_pin;
int key_irq;

static irqreturn_t key_irq_handler(int irq, void *dev_id)
{
	int val;

	val = gpiod_get_value(key_pin);
	printk("key press on, val is %d!\r\n", val);
	return IRQ_RETVAL(IRQ_HANDLED);
}

static int key_probe(struct platform_device *pdev)
{
	int ret;

	key_pin = devm_gpiod_get(&pdev->dev, "key", GPIOF_DIR_IN);
	if (IS_ERR(key_pin))
		return PTR_ERR(key_pin);
	gpiod_direction_input(key_pin);

	key_irq = gpiod_to_irq(key_pin);
	if (key_irq < 0) {
		devm_gpiod_put(&pdev->dev, key_pin);
		dev_err(&pdev->dev, "Failed to translate GPIO to IRQ\r\n");
	}

	ret = devm_request_irq(&pdev->dev, key_irq, key_irq_handler,
			       IRQF_TRIGGER_RISING, dev_name(&pdev->dev),
			       pdev);
	if (ret) {
		devm_gpiod_put(&pdev->dev, key_pin);
		dev_err(&pdev->dev, "failed to request IRQ %u (%d)\n", key_irq,
			ret);
		return ret;
	}

	return 0;
}

static int key_remove(struct platform_device *pdev)
{
	devm_free_irq(&pdev->dev, key_irq, pdev);
	devm_gpiod_put(&pdev->dev, key_pin);

	return 0;
}

static struct of_device_id key_match_table[] = {
	{
		.compatible = "planck-pi,key",
	},
};

static struct platform_device_id key_device_ids[] = {
	{
		.name = "key",
	},
};

static struct platform_driver key_driver= 
{ 
	.driver={ 
	.name = "key",
	.owner = THIS_MODULE,
	.of_match_table = key_match_table, 
	}, 
	.probe = key_probe, 
	.remove = key_remove, 
	.id_table = key_device_ids,
};

module_platform_driver(key_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("zhang <zhang916772719@gmail.com>");
MODULE_DESCRIPTION("LED");
MODULE_VERSION("1.0");