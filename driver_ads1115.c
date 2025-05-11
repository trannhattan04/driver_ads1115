#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#define DRIVER_NAME "ads1115_driver"
#define CLASS_NAME "ads1115"
#define DEVICE_NAME "ads1115"

#define addr_reg_config 0x01      // DIA CHI THANH GHI CAU HINH
#define addr_reg_conv   0x00      // DIA CHI THANH GHI DOC DU LIEU


// IOCTL commands
#define TYPE 'a'
#define READ_AIN0_ADS1115 _IOR(TYPE, 1, int)
#define READ_AIN1_ADS1115 _IOR(TYPE, 2, int)
#define READ_AIN2_ADS1115 _IOR(TYPE, 3, int)
#define READ_AIN3_ADS1115 _IOR(TYPE, 4, int)

static struct i2c_client *ads1115_client;
static struct class* ads1115_class = NULL;
static struct device* ads1115_device = NULL;
static int major_number;
static int device_open_count = 0;

// HAM DOC DU LIEU TU THANH GHI DU LIEU CUA ADS1115
static int ads1115_read_data(struct i2c_client *client)
{
    s32 ret = 0;
    s16 data = 0;
    float V = 0;
    int mV = 0;

    // Đọc 2 byte từ thanh ghi chuyển đổi (0x00)
    ret = i2c_smbus_read_word_data(client, addr_reg_conv);
    if (ret < 0) {
        printk(KERN_ERR "Failed to read data\n");
        return ret;
    }

    data = (ret << 8) | ((ret >> 8) & 0xFF);
    V = ((float)data / 32768.0) * 2.048;
    mV = (int)(V*1000);
    return mV;
}

//  HAM TRAO DOI DATA GIUA USER SPACE VA KERNEL SPACE
static long ads1115_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int data;
    int ret;

    switch (cmd) {
        case READ_AIN0_ADS1115:
            // Cấu hình: Single-shot mode, AIN0-GND, ±2.048V, 128SPS
            // OS = 1 (start), MUX = 100 (AIN0-GND), PGA = 010 (±2.048V), MODE = 1
            // DR = 100 (128SPS), Comparator = default
            // Data can cau hinh cho thanh ghi la: 1100 0100 1000 0011 = 0xC483
            ret = i2c_smbus_write_word_data(ads1115_client, addr_reg_config, cpu_to_be16(0xC483));
            if (ret < 0) {
                printk(KERN_ERR "Failed to config device");
                return ret;
            }
            msleep(10);  // thoi gian cho de thanh ghi duoc cau hinh thanh cong
            data = ads1115_read_data(ads1115_client);
            break;
        case READ_AIN1_ADS1115:
            // Cấu hình: Single-shot mode, AIN1-GND, ±2.048V, 128SPS
            // Data can cau hinh cho thanh ghi la: 1101 0100 1000 0011 = 0xD483
            ret = i2c_smbus_write_word_data(ads1115_client, addr_reg_config, cpu_to_be16(0xD483));
            if (ret < 0) {
                printk(KERN_ERR "Failed to config device");
                return ret;
            }
            msleep(10);
            data = ads1115_read_data(ads1115_client);
            break;
        case READ_AIN2_ADS1115:
            // Cấu hình: Single-shot mode, AIN2-GND, ±2.048V, 128SPS
            // Data can cau hinh cho thanh ghi la: 1110 0100 1000 0011 = 0xE483
            ret = i2c_smbus_write_word_data(ads1115_client, addr_reg_config, cpu_to_be16(0xE483));
            if (ret < 0) {
                printk(KERN_ERR "Failed to config device");
                return ret;
            }
            msleep(10);
            data = ads1115_read_data(ads1115_client);
            break;
        case READ_AIN3_ADS1115:
            // Cấu hình: Single-shot mode, AIN3-GND, ±2.048V, 128SPS
            // Data can cau hinh cho thanh ghi la: 1111 0100 1000 0011 = 0xF483
            ret = i2c_smbus_write_word_data(ads1115_client, addr_reg_config, cpu_to_be16(0xF483));
            if (ret < 0) {
                printk(KERN_ERR "Failed to config device");
                return ret;
            }
            msleep(10);
            data = ads1115_read_data(ads1115_client);
            break;
        default:
            return -EINVAL;
    }

    if (copy_to_user((int __user *)arg, &data, sizeof(data))) {
        printk(KERN_ERR "copy_to_user failed\n");
        return -EFAULT;
    }
    return 0;
}

// ham nay se chay khi nguoi dung thuc hien lenh open() tren user space
static int ads1115_open(struct inode *inodep, struct file *filep)
{
    if (device_open_count > 0)
        return -EBUSY;
    
    device_open_count++;
    try_module_get(THIS_MODULE);
    printk(KERN_INFO "ads1115 device opened\n");
    return 0;
}

//ham nay se chay khi nguoi dung thuc hien lenh close() tren user space
static int ads1115_release(struct inode *inodep, struct file *filep)
{
    device_open_count--;
    module_put(THIS_MODULE);
    printk(KERN_INFO "ads1115 device closed\n");
    return 0;
}

static struct file_operations fops = {
    .open = ads1115_open,
    .unlocked_ioctl = ads1115_ioctl,
    .release = ads1115_release,
};

// ham nay goi khi raspberry tim thay thiet bi tuc la ads1115 duoc ket noi chinh xac
static int ads1115_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int ret;
    ads1115_client = client;

    // Create a char device
    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_number < 0) {
        printk(KERN_ERR "Failed to register a major number\n");
        return major_number;
    }

    ads1115_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(ads1115_class)) {
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ERR "Failed to register device class\n");
        return PTR_ERR(ads1115_class);
    }

    ads1115_device = device_create(ads1115_class, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);
    if (IS_ERR(ads1115_device)) {
        class_destroy(ads1115_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ERR "Failed to create the device\n");
        return PTR_ERR(ads1115_device);
    }
    
    

    ret = i2c_smbus_write_word_data(client, 0x01, cpu_to_be16(0xC483));
    if (ret < 0) {
        device_destroy(ads1115_class, MKDEV(major_number, 0));
        class_unregister(ads1115_class);
        class_destroy(ads1115_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ERR "Failed to config device");
        return ret;
    }

    printk(KERN_INFO "ADS1115 driver installed\n");

    return 0;
}

// ham nay duoc goi khi ta thao go driver
static void ads1115_remove(struct i2c_client *client)
{
    device_destroy(ads1115_class, MKDEV(major_number, 0));
    class_unregister(ads1115_class);
    class_destroy(ads1115_class);
    unregister_chrdev(major_number, DEVICE_NAME);
    printk(KERN_INFO "ADS1115 driver removed\n");
 
}

static const struct i2c_device_id ads1115_id[] = {
    { "ads1115", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, ads1115_id);

static const struct of_device_id ads1115_of_match[] = {
    { .compatible = "ti,ads1115", },
    { },
};
MODULE_DEVICE_TABLE(of, ads1115_of_match);

static struct i2c_driver ads1115_driver = {
    .driver = {
        .name   = DRIVER_NAME,
        .owner  = THIS_MODULE,
        .of_match_table = of_match_ptr(ads1115_of_match),
    },
    .probe      = ads1115_probe,     // ham nay chay khi i2c_add_driver() tim thay duoc thiet bi tuong ung
    .remove     = ads1115_remove,    // khi huy het noi i2c voi driver ads1115
    .id_table   = ads1115_id,
};

static int __init ads1115_init(void)
{
    printk(KERN_INFO "Initializing ads1115 driver\n");  
    return i2c_add_driver(&ads1115_driver);             // đăng ký driver với hệ thống I2C của kernel 
                                                        // Dò các thiết bị hiện có và so khớp với id_table
}

static void __exit ads1115_exit(void)
{
    printk(KERN_INFO "Exiting ads1115 driver\n");
    i2c_del_driver(&ads1115_driver);
}

module_init(ads1115_init);
module_exit(ads1115_exit);

MODULE_AUTHOR("TRAN NHAT TAN");
MODULE_DESCRIPTION("ADS1115 DRIVER");
MODULE_LICENSE("GPL");