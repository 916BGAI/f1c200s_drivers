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
#include <linux/input.h>

struct planck_pi_key {
	struct gpio_desc *pin;
	struct input_dev *input_dev;
	int irq;
};

static irqreturn_t key_irq_handler(int irq, void *dev_id)
{
	struct planck_pi_key *key = dev_id;

	if (gpiod_get_value(key->pin) == 1) {
		input_report_key(key->input_dev, KEY_0, 0);
		input_sync(key->input_dev);
	} else if (gpiod_get_value(key->pin) == 0) {
		input_report_key(key->input_dev, KEY_0, 1);
		input_sync(key->input_dev);
	}

	return IRQ_RETVAL(IRQ_HANDLED);
}

static int key_probe(struct platform_device *pdev)
{
	struct planck_pi_key *key;
	struct input_dev *input_dev;
	int ret;

	key = devm_kzalloc(&pdev->dev, sizeof(*key), GFP_KERNEL);
	if (!key)
		return -ENOMEM;

	input_dev = input_allocate_device();
	if (!input_dev) {
		devm_free_irq(&pdev->dev, key->irq, key);
		devm_gpiod_put(&pdev->dev, key->pin);
		dev_err(&pdev->dev, "failed to allocate the input device\r\n");
		return -ENOMEM;
	}
	set_bit(EV_KEY, input_dev->evbit);
	set_bit(EV_REP, input_dev->evbit);
	set_bit(KEY_0, input_dev->keybit);
	input_dev->name = dev_name(&pdev->dev);
	input_dev->phys = "key/input0";
	input_dev->open = NULL;
	input_dev->close = NULL;
	input_dev->id.bustype = BUS_HOST;
	input_dev->id.vendor = 0x0001;
	input_dev->id.product = 0x0001;
	input_dev->id.version = 0x0100;
	ret = input_register_device(input_dev);
	if (ret < 0) {
		input_free_device(input_dev);
		devm_free_irq(&pdev->dev, key->irq, key);
		devm_gpiod_put(&pdev->dev, key->pin);
		dev_err(&pdev->dev, "error registering input device\r\n");
		return -ENOMEM;
	}

	key->input_dev = input_dev;

	key->pin = devm_gpiod_get(&pdev->dev, "key", GPIOF_DIR_IN);
	if (IS_ERR(key->pin))
		return PTR_ERR(key->pin);
	gpiod_direction_input(key->pin);

	key->irq = gpiod_to_irq(key->pin);
	if (key->irq < 0) {
		devm_gpiod_put(&pdev->dev, key->pin);
		dev_err(&pdev->dev, "Failed to translate GPIO to IRQ\r\n");
	}

	ret = devm_request_irq(&pdev->dev, key->irq, key_irq_handler,
			       IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
			       dev_name(&pdev->dev), key);

	dev_set_drvdata(&pdev->dev, key);

	return 0;
}

static int key_remove(struct platform_device *pdev)
{
	struct planck_pi_key *key = dev_get_drvdata(&pdev->dev);

	devm_free_irq(&pdev->dev, key->irq, key);
	devm_gpiod_put(&pdev->dev, key->pin);
	input_unregister_device(key->input_dev);

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