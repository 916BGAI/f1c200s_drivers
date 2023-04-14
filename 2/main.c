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

struct gpio_desc *led_pin_sys;
struct gpio_desc *led_pin_user;

static ssize_t led_show(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	return sprintf(buf, "The led status is = %d\r\n",
		       gpiod_get_value(led_pin_user));
}

static ssize_t led_store(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count)
{
	if (0 == memcmp(buf, "on", 2)) {
		gpiod_set_value(led_pin_user, 0);
	} else if (0 == memcmp(buf, "off", 3)) {
		gpiod_set_value(led_pin_user, 1);
	} else {
		printk(KERN_INFO "Not support cmd\r\n");
	}
	return count;
}

static DEVICE_ATTR(LED, 0660, led_show, led_store);

static int led_probe(struct platform_device *pdev)
{
	int ret;

	ret = device_create_file(&pdev->dev, &dev_attr_LED);
	if (unlikely(ret)) {
		dev_err(&pdev->dev, "Failed creating device attrs\n");
		return -EINVAL;
	}

	led_pin_sys =
		devm_gpiod_get(&pdev->dev, "led-sys", GPIOF_OUT_INIT_HIGH);
	if (IS_ERR(led_pin_sys))
		return PTR_ERR(led_pin_sys);
	led_pin_user =
		devm_gpiod_get(&pdev->dev, "led-user", GPIOF_OUT_INIT_HIGH);
	if (IS_ERR(led_pin_user))
		return PTR_ERR(led_pin_user);

	gpiod_direction_output(led_pin_sys, 1);
	gpiod_direction_output(led_pin_user, 1);

	return 0;
}

static int led_remove(struct platform_device *pdev)
{
	gpiod_direction_output(led_pin_sys, 1);
	gpiod_direction_output(led_pin_user, 1);

	gpiod_direction_input(led_pin_sys);
	gpiod_direction_input(led_pin_user);

	devm_gpiod_put(&pdev->dev,led_pin_sys);
	devm_gpiod_put(&pdev->dev,led_pin_user);

	device_remove_file(&pdev->dev,&dev_attr_LED);

	return 0;
}

static struct of_device_id led_match_table[] = {
	{
		.compatible = "planck-pi,leds",
	},
};
static struct platform_device_id led_device_ids[] = {
	{
		.name = "leds_gpio",
	},
};

static struct platform_driver led_driver= 
{ 
	.driver={ 
	.name = "leds",
	.owner = THIS_MODULE,
	.of_match_table = led_match_table, 
	}, 
	.probe = led_probe, 
	.remove = led_remove, 
	.id_table = led_device_ids,
};

module_platform_driver(led_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("zhang <zhang916772719@gmail.com>");
MODULE_DESCRIPTION("LED");
MODULE_VERSION("1.0");