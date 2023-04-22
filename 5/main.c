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
#include <linux/spi/spi.h>

#define ERASE_OPCODE 0x20
#define R_STS_OPCODE 0x35
#define JEDEC_OPCODE 0x9F

struct w25q128_state {
	struct spi_device *spi;
	struct spi_transfer xfer;
	char tx_data[16];
	char rx_data[16];
};

void w25q128_init(struct w25q128_state *st)
{
	st->xfer.tx_buf = st->tx_data;
	st->xfer.rx_buf = st->rx_data;
	st->xfer.len = sizeof(st->tx_data);
	st->xfer.bits_per_word = 8;
	memset(st->rx_data,0,16);
}

static ssize_t w25q128_show(struct device *dev, struct device_attribute *attr,
			    char *buf)
{
	struct w25q128_state *st = dev_get_drvdata(dev);
	return sprintf(buf,"%#x\n%#x%x\n",st->rx_data[0], st->rx_data[1], st->rx_data[2]);
	return 0;
}

static ssize_t w25q128_store(struct device *dev, struct device_attribute *attr,
			     const char *buf, size_t count)
{
	int ret;
	struct w25q128_state *st = dev_get_drvdata(dev);

	st->tx_data[0] = JEDEC_OPCODE;
	st->xfer.len = 1;
	ret = spi_write_then_read(st->spi, st->tx_data, 1, st->rx_data, 2);
	if (ret < 0) {
		dev_warn(dev, "spi_write_then_read failed with status %d\n",
			 ret);
	}

	return count;
}

static DEVICE_ATTR(w25q128, 0660, w25q128_show, w25q128_store);

static int w25q128_probe(struct spi_device *spi)
{
	int ret;
	struct w25q128_state *st;

	ret = device_create_file(&spi->dev, &dev_attr_w25q128);
	if (ret)
		dev_err(&spi->dev, "couldn't create device file for status\n");

	st = devm_kzalloc(&spi->dev, sizeof(*st), GFP_KERNEL);
	st->spi = spi_dev_get(spi);

	w25q128_init(st);

	dev_set_drvdata(&spi->dev, st);

	return 0;
}

static int w25q128_remove(struct spi_device *spi)
{
	device_remove_file(&spi->dev,&dev_attr_w25q128);

	return 0;
}

static struct of_device_id w25qxx_match_table[] = {  
    {.compatible = "planck-pi,w25qxx",},  
}; 

static struct spi_device_id w25qxx_ids[] = {
    {.name = "w25q128",},
};

static struct spi_driver w25qxx_driver = {  
    .driver = {  
        .name =  "w25q128",  
        .owner = THIS_MODULE,  
        .of_match_table = w25qxx_match_table,
    },  
    .probe  = w25q128_probe,  
    .remove = w25q128_remove,  
    .id_table = w25qxx_ids,
};  

module_spi_driver(w25qxx_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("zhang <zhang916772719@gmail.com>");
MODULE_DESCRIPTION("LED");
MODULE_VERSION("1.0");