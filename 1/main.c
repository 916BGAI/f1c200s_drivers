#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <asm/io.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <asm-generic/gpio.h>
#include <linux/kdev_t.h>
#include <linux/kern_levels.h>
#include <linux/printk.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/ioport.h>
#include <linux/types.h>

static void __iomem *GPIOE_CFG0;
static void __iomem *GPIOE_CFG1;
static void __iomem *GPIOE_DATA;
static void __iomem *GPIOE_PUL0;

struct led_dev {
	dev_t dev_id;
	int major;
	int minor;
	struct cdev *cdev;
	struct class *class;
	struct device *device;
};

static struct led_dev led;

static void led_board_ctrl(int status)
{
    uint32_t val;

    if (status == 1) {
        // 打开LED
		val = readl(GPIOE_DATA);
		val &= ~(1 << 4);
		writel(val, GPIOE_DATA);
    } else {
        // 关闭LED
        val = readl(GPIOE_DATA);
		val |= (0 << 4);
		writel(val, GPIOE_DATA);
    }
}

static int led_open(struct inode *inode, struct file *file)
{
	uint32_t val;

	val = readl(GPIOE_CFG0);
	val &= ~(7 << 16);
	val |= (1 << 16);
	writel(val, GPIOE_CFG0);

	val = readl(GPIOE_PUL0);
	val &= ~(3 << 8);
	val |= (1 << 8);
	writel(val, GPIOE_PUL0);

	val = readl(GPIOE_DATA);
	val |= (1 << 4);
	writel(val, GPIOE_DATA);

	return 0;
}

static int led_release(struct inode *inode, struct file *file)
{
	return 0;
}

static ssize_t led_read(struct file *file, char __user *buf, size_t size,
			loff_t *ppos)
{
	return 0;
}

static ssize_t led_write(struct file *file, const char __user *buf,
			 size_t count, loff_t *pos)
{
	int ret;
	size_t status;

	ret = copy_from_user(&status,buf,4);
    if (ret < 0) {
        printk("led write failed!\n");
        return -EFAULT;
    }

    if (status == 0) {
        led_board_ctrl(1);
    } else if (status == 1){
        led_board_ctrl(0);
    }

	return 0;
}

static const struct file_operations led_ops = {
	.owner = THIS_MODULE,
	.open = led_open,
	.release = led_release,
	.read = led_read,
	.write = led_write,
};

static int led_probe(struct platform_device *pdev)
{
	int ret;
	struct resource *res;

	if (led.major) {
		led.dev_id = MKDEV(led.major, 0);
		ret = register_chrdev_region(led.dev_id, 1, "LED");
	} else {
		ret = alloc_chrdev_region(&led.dev_id, 0, 1, "LED");
		led.major = MAJOR(led.dev_id);
		led.minor = MINOR(led.dev_id);
	}
	if (ret) {
		pr_err(KERN_WARNING "chrdev region failed!\r\n");
		goto err_dev_id;
	}

	led.cdev = cdev_alloc();
	if (!led.cdev) {
		pr_err("cdev alloc failed!\r\n");
		goto err_cdev;
	}
	led.cdev->owner = THIS_MODULE;
	led.cdev->ops = &led_ops;
	ret = cdev_add(led.cdev, led.dev_id, 1);
	if (ret) {
		pr_err("cdev add failed!\r\n");
		goto err_cdev;
	}

	led.class = class_create(THIS_MODULE, "LED");
	if (!led.class) {
		pr_err("led class failed!\r\n");
		goto err_class;
	}

	led.device = device_create(led.class, NULL, led.dev_id, NULL, "LED");
	if (IS_ERR(led.device)) {
		pr_err("device create failed!\r\n");
		goto err_device;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	GPIOE_CFG0 = ioremap(res->start,(res->end - res->start)+1);
	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	GPIOE_CFG1 = ioremap(res->start,(res->end - res->start)+1);
	res = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	GPIOE_DATA = ioremap(res->start,(res->end - res->start)+1);
	res = platform_get_resource(pdev, IORESOURCE_MEM, 3);
	GPIOE_PUL0 = ioremap(res->start,(res->end - res->start)+1);

	return 0;

err_device:
	class_destroy(led.class);
err_class:
	cdev_del(led.cdev);
err_cdev:
	unregister_chrdev_region(led.dev_id, 1);
err_dev_id:
	return -1;
}

static int led_remove(struct platform_device *pdev)
{
	iounmap(GPIOE_CFG0);
	iounmap(GPIOE_CFG1);
	iounmap(GPIOE_DATA);
	iounmap(GPIOE_PUL0);

	cdev_del(led.cdev);
	unregister_chrdev_region(led.dev_id, 1);
	device_destroy(led.class, led.dev_id);
	class_destroy(led.class);

	return 0;
}

static struct of_device_id led_match_table[] = {
	{
		.compatible = "planck-pi,led",
	},
};
static struct platform_device_id led_device_ids[] = {
	{
		.name = "led",
	},
};

static struct platform_driver led_driver= 
{ 
	.probe = led_probe, 
	.remove = led_remove, 
	.driver={ 
	.name = "led", 
	.of_match_table = led_match_table, 
	}, 
	.id_table = led_device_ids,
};

static __init int led_init(void)
{
	platform_driver_register(&led_driver);
	return 0;
}

static __exit void led_exit(void)
{
	platform_driver_register(&led_driver);
}

module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("zhang <zhang916772719@gmail.com>");
MODULE_DESCRIPTION("LED");
MODULE_VERSION("1.0");