#include        <linux/err.h>
#include        <linux/errno.h>
#include        <linux/delay.h>
#include        <linux/fs.h>
#include        <linux/i2c.h>
#include        <linux/input.h>
#include        <linux/uaccess.h>
#include        <linux/workqueue.h>
#include	    <linux/earlysuspend.h>
#include        <linux/irq.h>
#include        <linux/gpio.h>
#include        <linux/interrupt.h>
#include        <linux/slab.h>
#include        <linux/miscdevice.h>
#include        <linux/linux_sensors.h>
#include        <linux/gsensor.h>
#include        <linux/hwmon-sysfs.h>
#include        "gsensor_dmard06.h"
//#define DEBUG
#ifdef DEBUG
#define dprintk(x...)	do{printk("~~~~~ryder: %s~~~~~\n",__FUNCTION__);printk(x);}while(0)
#else
#define dprintk(x...)
#endif
#define SENSOR_DATA_SIZE 3

#define SAMPLE_COUNT (1)
#ifdef CONFIG_SENSORS_ORI
extern void orientation_report_values(int x,int y,int z);
#endif
struct dmard06_acc_data *dmard06_acc;
struct {
    unsigned int cutoff_ms;
    unsigned int mask;
} dmard06_acc_odr_table[] = {
    { 4,	ODR342 },
    { 15,	ODR85  },
    { 30,	ODR42  },
    { 50,	ODR21  },
};

struct dmard06_acc_data {
    struct i2c_client *client;
    struct gsensor_platform_data *pdata;

    struct mutex	lock_rw;
    struct mutex	lock;
    struct delayed_work input_work;
    struct delayed_work dmard06_acc_delayed_work;

    struct input_dev *input_dev;

    int hw_initialized;
    /* hw_working=-1 means not tested yet */
    int hw_working;
    atomic_t enabled;
    atomic_t regulator_enabled;
    int power_tag;
    int is_suspend;
    u8 sensitivity;
    u8 resume_state[RESUME_ENTRIES];
    int irq;
    struct work_struct irq_work;
    struct workqueue_struct *irq_work_queue;
    struct workqueue_struct *work_queue;
    struct early_suspend early_suspend;

    struct miscdevice dmard06_misc_device;

    struct regulator *power;
};

static int dmard06_i2c_read(struct dmard06_acc_data *acc,
        u8 * buf, int len)
{
    int err;
    struct i2c_msg  msgs[] = {
        {
            .addr = acc->client->addr,
            .flags = 0 ,
            .len = 1,
            .buf = buf,
        },
        {
            .addr = acc->client->addr,
            .flags = 1,
            .len = len,
            .buf = buf,
        },
    };
    err = i2c_transfer(acc->client->adapter, msgs, 2);
    if (err < 0) {
        dev_err(&acc->client->dev, "gsensor dmard06 i2c read error\n");
    }
    return err;
}

static int dmard06_i2c_write(struct dmard06_acc_data *acc, u8 * buf, int len)
{
    int err;
    struct i2c_msg msgs[] = {
        {
            .addr = acc->client->addr,
            .flags = acc->client->flags ,
            .len = len + 1,
            .buf = buf,
        },
    };
    err = i2c_transfer(acc->client->adapter, msgs, 1);
    if (err < 0)
        dev_err(&acc->client->dev, "gsensor dmard06 i2c write error\n");

    return err;
}

static int dmard06_acc_i2c_read(struct dmard06_acc_data *acc,u8 * buf, int len)
{
    int ret;
    mutex_lock(&acc->lock_rw);
    ret = dmard06_i2c_read(acc,buf,len);
    mutex_unlock(&acc->lock_rw);
    return ret;
}
static int dmard06_acc_i2c_write(struct dmard06_acc_data *acc, u8 * buf, int len)
{
    int ret;
    mutex_lock(&acc->lock_rw);
    ret = dmard06_i2c_write(acc,buf,len);
    mutex_unlock(&acc->lock_rw);
    return ret;
}
int dmard06_acc_update_odr(struct dmard06_acc_data *acc, int poll_interval_ms)
{
    int err = -1;
    int i;
    u8 config[2];
    u8 tmp;
    for(i = 0;i < ARRAY_SIZE(dmard06_acc_odr_table);i++){
        config[1] = dmard06_acc_odr_table[i].mask;
        if(poll_interval_ms < dmard06_acc_odr_table[i].cutoff_ms){
            break;
        }
    }
    switch (config[1])
    {
        case ODR342:	  tmp = 0;break;//INT_DUR1 register set to 0x2d irq rate is:11Hz
        case ODR85:	      tmp= 0x1;break;//set to 0x0e irq rate:23Hz
        case ODR42:	      tmp = 0x2;break;//set to 0x06 irq rate:42Hz
        default:	      tmp= 0x3;break;//default situation set to 0x1D:irq rate:11Hz
    }
    config[0]=0x44;
    err = dmard06_acc_i2c_read(acc, config, 1);
    config[1]&=0xe7;
    config[1]|=tmp<<3;
    err = dmard06_acc_i2c_write(acc,config,1);
    flush_delayed_work(&acc->dmard06_acc_delayed_work);
    cancel_delayed_work_sync(&acc->dmard06_acc_delayed_work);
    queue_delayed_work(dmard06_acc->work_queue,&dmard06_acc->dmard06_acc_delayed_work,msecs_to_jiffies(dmard06_acc->pdata->poll_interval));
    return err;
}
static int dmard06_acc_device_power_off(struct dmard06_acc_data *acc)
{
    int err;
    if (acc->pdata->power_off) {
        acc->pdata->power_off();
        acc->hw_initialized = 0;
    }else{
        u8 buf[2] = { CTRL_REG1, DMARD06_ACC_PM_OFF };
        err = dmard06_acc_i2c_write(acc, buf, 1);
        if (err < 0){
            dev_err(&acc->client->dev,
                    "soft power off failed: %d\n", err);
            return err;
        }
    }
    if (atomic_cmpxchg(&acc->regulator_enabled, 1, 0)) {
        regulator_disable(acc->power);
    }

    return 0;
}

static int dmard06_acc_device_power_on(struct dmard06_acc_data *acc)
{
    int err = -1;
    u8 buf[2];
    int result=-1;
    if (acc->power && (atomic_cmpxchg(&acc->regulator_enabled, 0, 1) == 0)){
        atomic_set(&acc->regulator_enabled,1);      
        result=regulator_enable(acc->power);
        if (result < 0){
            dev_err(&acc->client->dev,"power_off regulator failed: %d\n", result);
            return result;
        }
    }
    if (acc->pdata->power_on) {
        err = acc->pdata->power_on();
        if (err < 0) {
            dev_err(&acc->client->dev,
                    "power_on failed: %d\n", err);
            return err;
        }
    }else{
        if (atomic_read(&acc->enabled)) {
            buf[0] = CTRL_REG1;
            err = dmard06_acc_i2c_read(acc, buf, 1);
            if (err < 0)
                return err;
            buf[1] = buf[0]|0x20;
            err = dmard06_acc_i2c_write(acc, buf, 1);
            if (err < 0){
                dev_err(&acc->client->dev,"power_on failed: %d\n", err);
                return err;
            }
        }
    }
    return 0;
}

static int dmard06_acc_enable(struct dmard06_acc_data *acc);
static int dmard06_acc_disable(struct dmard06_acc_data *acc);

static int dmard06_acc_enable(struct dmard06_acc_data *acc)
{
    int err;
    if ((acc->is_suspend == 0) && !atomic_cmpxchg(&acc->enabled, 0, 1)) {
        err = dmard06_acc_device_power_on(acc);
        if (err < 0) {
            atomic_set(&acc->enabled, 0);
            return err;
        }
    }
    return 0;
}

static int dmard06_acc_disable(struct dmard06_acc_data *acc)
{
    if (atomic_cmpxchg(&acc->enabled, 1, 0)) {
        dmard06_acc_device_power_off(acc);
    }
    return 0;
}

struct linux_sensor_t hardware_data_dmard06 = {
    "dmard06 3-axis Accelerometer",
    "ST sensor",
    SENSOR_TYPE_ACCELEROMETER,1,32,1, 1, 
    { }//modify version from 0 to 1 for cts
};
static int dmard06_acc_validate_pdata(struct dmard06_acc_data *acc)
{
    acc->pdata->poll_interval = max(acc->pdata->poll_interval,acc->pdata->min_interval);
    if (acc->pdata->axis_map_x > 2 || acc->pdata->axis_map_y > 2 || acc->pdata->axis_map_z > 2) {
        dev_err(&acc->client->dev,
                "invalid axis_map value "
                "x:%u y:%u z%u\n", 
                acc->pdata->axis_map_x,
                acc->pdata->axis_map_y,
                acc->pdata->axis_map_z);
        return -EINVAL;
    }
    if (acc->pdata->negate_x > 1 || acc->pdata->negate_y > 1 || acc->pdata->negate_z > 1) {
        dev_err(&acc->client->dev,
                "invalid negate value " 
                "x:%u y:%u z:%u\n",
                acc->pdata->negate_x,
                acc->pdata->negate_y,
                acc->pdata->negate_z);
        return -EINVAL;
    }
    if (acc->pdata->poll_interval < acc->pdata->min_interval) {
        dev_err(&acc->client->dev, "minimum poll interval violated\n");
        return -EINVAL;
    }
    return 0;
}

void  acc_input_close(struct input_dev *input)
{
    struct  dmard06_acc_data *acc = input_get_drvdata(input);
    cancel_delayed_work_sync(&acc->dmard06_acc_delayed_work);
}

int acc_input_open(struct input_dev *input)
{
    struct dmard06_acc_data  *acc = input_get_drvdata(input);
    if (atomic_read(&acc->enabled))
    {
        printk("Dmard Sensor has already enable!\n");
        return -1;
    }
    else
    {
        atomic_set(&acc->enabled, 1);
        queue_delayed_work(acc->work_queue,&acc->dmard06_acc_delayed_work,msecs_to_jiffies(100));
    }
    return 0;
}
static int dmard06_acc_input_init(struct dmard06_acc_data *acc)
{
    int err;
    acc->input_dev = input_allocate_device();
    if (!acc->input_dev) {
        err = -ENOMEM;
        dev_err(&acc->client->dev, "input device allocation failed\n");
        goto err0;
    }
    acc->input_dev->open = acc_input_open;
    acc->input_dev->close =acc_input_close;
    acc->input_dev->name = "g_sensor";
    acc->input_dev->id.bustype = BUS_I2C;
    acc->input_dev->dev.parent = &acc->client->dev;
    input_set_drvdata(acc->input_dev, acc);
    set_bit(EV_ABS, acc->input_dev->evbit);
    input_set_abs_params(acc->input_dev, ABS_X, -G_MAX, G_MAX, 0, 0);
    input_set_abs_params(acc->input_dev, ABS_Y, -G_MAX, G_MAX, 0, 0);
    input_set_abs_params(acc->input_dev, ABS_Z, -G_MAX, G_MAX, 0, 0);
    err = input_register_device(acc->input_dev);
    if (err) {
        dev_err(&acc->client->dev,"unable to register input device %s\n",acc->input_dev->name);
        goto err1;
    }
    return 0;
err1:
    input_free_device(acc->input_dev);
err0:
    return err;
}

static void dmard06_acc_input_cleanup(struct dmard06_acc_data *acc)
{
    input_unregister_device(acc->input_dev);
    input_free_device(acc->input_dev);
}

static int dmard06_misc_open(struct inode *inode, struct file *file)
{
    int err;
    err = nonseekable_open(inode, file);
    if (err < 0)
        return err;
    return 0;
}

long dmard06_misc_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    void __user *argp = (void __user *)arg;
    int interval;
    struct miscdevice *dev = file->private_data;
    struct dmard06_acc_data *dmard06 = container_of(dev, struct dmard06_acc_data,dmard06_misc_device);

    switch (cmd) {
        case SENSOR_IOCTL_GET_DELAY:
            interval = dmard06->pdata->poll_interval;
            if (copy_to_user(argp, &interval, sizeof(interval)))
                return -EFAULT;
            break;
        case SENSOR_IOCTL_SET_DELAY:
            if (atomic_read(&dmard06->enabled)) {
                mutex_lock(&dmard06->lock);
                if (copy_from_user(&interval, argp, sizeof(interval)))
                    return -EFAULT;
                if (interval < dmard06->pdata->min_interval )
                    interval = dmard06->pdata->min_interval;
                else if (interval > dmard06->pdata->max_interval)
                    interval = dmard06->pdata->max_interval;
                dmard06->pdata->poll_interval = interval;
                dmard06_acc_update_odr(dmard06, dmard06->pdata->poll_interval);
                mutex_unlock(&dmard06->lock);
            }
            break;
        case SENSOR_IOCTL_SET_ACTIVE:
            mutex_lock(&dmard06->lock);
            if (copy_from_user(&interval, argp, sizeof(interval)))
                return -EFAULT;
            if (interval > 1)
                return -EINVAL;

            if (interval) {
                dmard06->power_tag = interval;
                dmard06_acc_enable(dmard06);
                acc_input_open(dmard06->input_dev);
            } else {
                dmard06->power_tag = interval;
                acc_input_close(dmard06->input_dev);
                dmard06_acc_disable(dmard06);
                mdelay(2);
            }
            mutex_unlock(&dmard06->lock);
            break;
        case SENSOR_IOCTL_GET_ACTIVE:
            interval = atomic_read(&dmard06->enabled);
            if (copy_to_user(argp, &interval, sizeof(interval)))
                return -EINVAL;
            break;
        case SENSOR_IOCTL_GET_DATA:
            if (copy_to_user(argp, &hardware_data_dmard06, sizeof(hardware_data_dmard06)))
                return -EINVAL;
            break;

        case SENSOR_IOCTL_GET_DATA_MAXRANGE:
            if (copy_to_user(argp, &dmard06->pdata->g_range, sizeof(dmard06->pdata->g_range)))
                return -EFAULT;
            break;
        case SENSOR_IOCTL_WAKE:
            input_event(dmard06->input_dev, EV_SYN,SYN_CONFIG, 0);
            break;
        default:
            return -EINVAL;
    }

    return 0;
}


static const struct file_operations dmard06_misc_fops = {
    .owner = THIS_MODULE,
    .open = dmard06_misc_open,
    .unlocked_ioctl = dmard06_misc_ioctl,
};

static int dmard06_acc_miscdev_init(struct dmard06_acc_data *acc)
{
    acc->dmard06_misc_device.minor = MISC_DYNAMIC_MINOR,
    acc->dmard06_misc_device.name =  "g_sensor",
    acc->dmard06_misc_device.fops = &dmard06_misc_fops;
    return misc_register(&acc->dmard06_misc_device);
}

s16 filter_call(s8* data, int size)
{
    int index;
    s8 max, min;
    int count = 0;
    s8 value = 0;
    max = min = data[0];
    for (index=0; index < size; index++) {
        if (data[index] > max) {
            max = data[index];
        }
        if (data[index] < min) {
            min = data[index];
        }
        count += data[index];
    }

    if (size <= 3) {
        value = count / size;
    } else {
        value = (count - max - min) / (size - 2);
    }
    return value;
}
static void device_i2c_xyz_read_reg(struct i2c_client *client,u8 *buffer, int length)
{
    int i = 0;
    u8 cAddress = 0;
    cAddress = 0x41;
    for(i=0;i<SENSOR_DATA_SIZE;i++)
    {
        buffer[i] = i2c_smbus_read_byte_data(client,cAddress+i);
    }
}

static int get_flag_bit( u8 value)
{
    s8 num = 0;
    int aaa = value & (1<<6);
    num = value & 0x3f;
    if(aaa == 0)
    {
        num = num;
    }
    else
    {
        num=num-64;
    }
    return num;
}
void device_i2c_read_xyz(struct i2c_client *client, s8 *xyz_p)
{
    s8 xyzTmp[SENSOR_DATA_SIZE*SAMPLE_COUNT];
    s8 xyzTmp2[SENSOR_DATA_SIZE]={0,0,0};
    int i, j;
    u8 buffer[3];
    for (j=0;j<SAMPLE_COUNT;j++){
        device_i2c_xyz_read_reg(client, buffer, 3); 
        for(i = 0; i < SENSOR_DATA_SIZE; i++){
            xyzTmp[i*SAMPLE_COUNT+j] = get_flag_bit((buffer[i] >> 1));
        }
    }
    xyzTmp2[0]=filter_call(xyzTmp,SAMPLE_COUNT);
    xyzTmp2[1]=filter_call(xyzTmp+SAMPLE_COUNT,SAMPLE_COUNT);
    xyzTmp2[2]=filter_call(xyzTmp+2*SAMPLE_COUNT,SAMPLE_COUNT);
    xyz_p[0] =((dmard06_acc->pdata->negate_x) ? (-xyzTmp2[dmard06_acc->pdata->axis_map_x]): (xyzTmp2[dmard06_acc->pdata->axis_map_x]));
    xyz_p[1] =((dmard06_acc->pdata->negate_y) ? (-xyzTmp2[dmard06_acc->pdata->axis_map_y]): (xyzTmp2[dmard06_acc->pdata->axis_map_y]));
    xyz_p[2] =((dmard06_acc->pdata->negate_z) ? (-xyzTmp2[dmard06_acc->pdata->axis_map_z]): (xyzTmp2[dmard06_acc->pdata->axis_map_z]));
    /*for(i=0;i<3;i++)//2G mode
        xyz_p[i]*=2;
        */
}
static void dmard06_acc_delayed_work_fun(struct work_struct *work)
{
    s8 xyz[3],x,y,z;
    device_i2c_read_xyz( dmard06_acc->client, (s8 *)&xyz);
    x=xyz[0];
    y=xyz[1];
    z=xyz[2];
    input_report_abs(dmard06_acc->input_dev, ABS_X, x);
    input_report_abs(dmard06_acc->input_dev, ABS_Y, y);
    input_report_abs(dmard06_acc->input_dev, ABS_Z, z);
    input_sync(dmard06_acc->input_dev);
#ifdef CONFIG_SENSORS_ORI
    x = (dmard06_acc->pdata->ori_roll_negate) ? (-x): x;
    y = (dmard06_acc->pdata->ori_pith_negate) ? (-y): y;
    orientation_report_values(x,y,z);
#endif
    queue_delayed_work(dmard06_acc->work_queue,&dmard06_acc->dmard06_acc_delayed_work,1);
}
#ifdef CONFIG_HAS_EARLYSUSPEND
static void dmard06_acc_late_resume(struct early_suspend *handler);
static void dmard06_acc_early_suspend(struct early_suspend *handler);
#endif


int dmard06_acc_hw_init(struct dmard06_acc_data *acc)
{
    char cAddress = 0 , cData = 0;
    int result=-1;
    acc->power= regulator_get(&acc->client->dev, "vgsensor");
    atomic_set(&acc->regulator_enabled,1);      
    result=regulator_enable(acc->power);
    if (result < 0){
        dev_err(&acc->client->dev,"power_off regulator failed: %d\n", result);
        return result;
    }
    cAddress = SW_RESET;
    result = i2c_smbus_read_byte_data(acc->client,cAddress);
    dprintk(KERN_INFO "i2c Read SW_RESET = %x \n", result);
    cAddress = WHO_AM_I;
    result = i2c_smbus_read_byte_data(acc->client,cAddress);
    dprintk( "i2c Read WHO_AM_I = %d \n", result);
    cData=result;
    if(( cData&0x00FF) == WHO_AM_I_VALUE) //read 0Fh should be 06, else some err there
    {
        dprintk( "@@@ %s gsensor registered I2C driver!\n",__FUNCTION__);
    }
    else
    {
        dprintk( "@@@ %s gsensor I2C err = %d!\n",__FUNCTION__,cData);
        return -1;
    }
    return 0;
}
static int dmard06_acc_probe(struct i2c_client *client,		const struct i2c_device_id *id)
{
    struct dmard06_acc_data *acc;
    int err = -1,result=0;
    dprintk("%s: probe start.\n", DMARD06_ACC_DEV_NAME);
    if (client->dev.platform_data == NULL) {
        dev_err(&client->dev, "platform data is NULL. exiting.\n");
        err = -ENODEV;
        goto exit_check_functionality_failed;
    }
    result = i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE|I2C_FUNC_SMBUS_BYTE_DATA);
    if(!result)	{
        dev_err(&client->dev, "client not i2c capable\n");
        err = -ENODEV;
        goto exit_check_functionality_failed;
    }
    dmard06_acc=acc=kzalloc(sizeof(struct dmard06_acc_data), GFP_KERNEL);
    if (acc == NULL) {
        err = -ENOMEM;
        dev_err(&client->dev,"failed to allocate memory for module data: %d\n", err);
        goto err_free_acc;
    }
    mutex_init(&acc->lock_rw);
    mutex_init(&acc->lock);
    acc->client = client;
    acc->pdata = kmalloc(sizeof(*acc->pdata), GFP_KERNEL);
    if (acc->pdata == NULL) {
        err = -ENOMEM;
        dev_err(&client->dev,"failed to allocate memory for pdata: %d\n",err);
        goto err_free_pdata;
    }
    memcpy(acc->pdata, client->dev.platform_data, sizeof(*acc->pdata));
    dmard06_acc_validate_pdata(acc);
    if (acc->pdata->init) {
        err = acc->pdata->init();
        if (err < 0) {
            dev_err(&client->dev, "init failed: %d\n", err);
            goto err_pdata_init;
        }
    }
    dmard06_acc_hw_init(acc);
    err = dmard06_acc_input_init(acc);
    if (err < 0) {
        dev_err(&client->dev, "input init failed\n");
        goto err_power_off;
    }
    err = dmard06_acc_miscdev_init(acc);
    if(err) dprintk("ryder : failed to regist msic dev");
    acc->work_queue=create_workqueue("my_devpoll");
    INIT_DELAYED_WORK(&acc->dmard06_acc_delayed_work,dmard06_acc_delayed_work_fun);
    if(!(acc->work_queue)){
        dprintk("creating workqueue failed\n");
    }
#ifdef CONFIG_HAS_EARLYSUSPEND
    acc->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
    acc->early_suspend.suspend = dmard06_acc_early_suspend;
    acc->early_suspend.resume = dmard06_acc_late_resume;
    register_early_suspend(&acc->early_suspend);
#endif
    return 0;
err_power_off:
err_pdata_init:
    if (acc->pdata->exit)
        acc->pdata->exit();
err_free_acc:
    kfree(acc->pdata);
err_free_pdata:
    kfree(acc);
exit_check_functionality_failed:
    dprintk(KERN_ERR "%s: Driver Init failed\n", DMARD06_ACC_DEV_NAME);
    return err;
}

static int __devexit dmard06_acc_remove(struct i2c_client *client)
{
    struct dmard06_acc_data *acc = i2c_get_clientdata(client);
    if(acc->pdata->gpio_int >= 0){
        destroy_workqueue(acc->irq_work_queue);
    }
    dmard06_acc_input_cleanup(acc);
    dmard06_acc_device_power_off(acc);
    if (acc->pdata->exit)
        acc->pdata->exit();
    kfree(acc->pdata);
    kfree(acc);
    return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void dmard06_acc_late_resume(struct early_suspend *handler)
{
    /*struct dmard06_acc_data *acc;
      dprintk("~~~~~~~~late resume~~~~~~enable %d~~~",atomic_read(&acc->enabled));
      acc = container_of(handler, struct dmard06_acc_data, early_suspend);
      acc->is_suspend = 0;
      if (!atomic_read(&acc->enabled)) {
      mutex_lock(&acc->lock);
      dmard06_acc_enable(acc);
      acc_input_open(acc->input_dev);
      mutex_unlock(&acc->lock);
      }
      */
}

static void dmard06_acc_early_suspend(struct early_suspend *handler)
{
    /*dprintk("~~~~~~~~early suspend~~~~~~~~~");
      struct dmard06_acc_data *acc ;
      acc = container_of(handler, struct dmard06_acc_data, early_suspend);
      acc->is_suspend = 1;
      if (atomic_read(&acc->enabled)) {
      acc_input_close(acc->input_dev);
      dmard06_acc_disable(acc);
      }
      */
}
#endif

static const struct i2c_device_id dmard06_acc_id[] = { { DMARD06_ACC_DEV_NAME, 0 }, { }, };

MODULE_DEVICE_TABLE(i2c, dmard06_acc_id);

static struct i2c_driver dmard06_acc_driver = {
    .driver = {
        .owner = THIS_MODULE,
        .name = DMARD06_ACC_DEV_NAME,
    },
    .probe = dmard06_acc_probe,
    .remove = __devexit_p(dmard06_acc_remove),
    .id_table = dmard06_acc_id,
};

static int __init dmard06_acc_init(void)
{
    dprintk(KERN_INFO "%s accelerometer driver: init\n",DMARD06_ACC_DEV_NAME);
    return i2c_add_driver(&dmard06_acc_driver);
}

static void __exit dmard06_acc_exit(void)
{
    dprintk(KERN_INFO "%s accelerometer driver exit\n",DMARD06_ACC_DEV_NAME);
    i2c_del_driver(&dmard06_acc_driver);
    return;
}

module_init(dmard06_acc_init);
module_exit(dmard06_acc_exit);

MODULE_DESCRIPTION("Dmard06  digital accelerometer  kernel driver");
MODULE_AUTHOR("dwjia,Ingenic");
MODULE_LICENSE("GPL");

