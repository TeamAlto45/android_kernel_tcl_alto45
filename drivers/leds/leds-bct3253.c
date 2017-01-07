/*
 * leds-bct3253.c - RGB LED Driver
 *
 * Copyright (C) 2014 TCL Electronics
 * Qijun.xu <qijun.xu@tcl.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 * now is only green led is used,so disalbe other led,but ic is support RGB
 * Datasheet:
 * ===================================================================================
*                              EDIT HISTORY FOR MODULE
*
* This section contains comments describing changes made to the module.
* Notice that changes are listed in reverse chronological order.
*
*  when       who        what, where, why
*------------------------------------------------------------------------------------
*07/24/2014  XQJ      |FR-742098, add bct3253 extern led ic chip
 */

#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/leds.h>
#include <linux/leds-bct3253.h>
#include <linux/slab.h>
#include <linux/pm.h>
#include <linux/regulator/consumer.h>

#define BCT3253_CURRENT_637     0x11 /* 63.75mA */
#define BCT3253_CURRENT_318     0x10 /* 31.875mA */
#define BCT3253_CURRENT_255     0x01 /* 25.5mA */
#define BCT3253_CURRENT_127     0x00 /* 12.7mA */
/* POWER SUPPLY VOLTAGE RANGE */
#define BCT3253_VDD_MIN_UV  2000000
#define BCT3253_VDD_MAX_UV  3300000
#define BCT3253_VIO_MIN_UV        1750000
#define BCT3253_VIO_MAX_UV       1950000

enum led_ids {
    LED1,
       LED2,
    LED_NUM,
};

enum led_colors {
    RED,
    GREEN,
    BLUE,
};

enum led_bits {
    BCT3253_OFF,
    BCT3253_BLINK,
    BCT3253_ON,
};

/*
 * State '0' : 'off'
 * State '1' : 'blink'
 * State '2' : 'on'.
 */
struct led_state {
    unsigned r:2;
    unsigned g:2;
    unsigned b:2;
};

struct bct3253_led {
    struct bct3253_led_platform_data    *pdata;
    struct i2c_client       *client;
    struct rw_semaphore     rwsem;
    struct work_struct      work;

    struct led_state        led[2];
    struct regulator    *vio;
    int power_enabled;
    /*
     * Making led_classdev as array is not recommended, because array
     * members prevent using 'container_of' macro. So repetitive works
     * are needed.
     */
    struct led_classdev     cdev_led1g;
    /*
     * Advanced Configuration Function(ADF) mode:
     * In ADF mode, user can set registers of BCT3253GU directly,
     * therefore BCT3253GU doesn't enter reset state.
     */
    int             adf_on;

    enum led_ids            led_id;
    enum led_colors         color;
    enum led_bits           state;
};
static int bct3253_power_init(struct bct3253_led *data, bool on)
{
    int rc;

    if (!on) {

        if (regulator_count_voltages(data->vio) > 0)
              regulator_set_voltage(data->vio, 0,BCT3253_VIO_MAX_UV);
           regulator_put(data->vio);

        }
    else {

        data->vio = regulator_get(&data->client->dev, "vio");
        if (IS_ERR(data->vio)) {
            rc = PTR_ERR(data->vio);
            dev_err(&data->client->dev,
                "Regulator get failed vio rc=%d\n", rc);
            return rc;
        }
        if (regulator_count_voltages(data->vio) > 0) {
            rc = regulator_set_voltage(data->vio,
                BCT3253_VIO_MIN_UV, BCT3253_VIO_MAX_UV);
            if (rc) {
                dev_err(&data->client->dev,
                "Regulator set failed vio rc=%d\n", rc);
                return rc;
            }
        }
    }

    return 0;



}
static int bct3253_power_set(struct bct3253_led *data, bool on)
{
    int rc = 0;
    //if(on==0)
       //    return rc;
    if (!on && data->power_enabled) {


        rc = regulator_disable(data->vio);
        if (rc) {
            dev_err(&data->client->dev,
                "Regulator vio disable failed rc=%d\n", rc);
            return rc;
        }
        data->power_enabled = false;
        return rc;
    } else if (on && !data->power_enabled) {

        rc = regulator_enable(data->vio);
        if (rc) {
            dev_err(&data->client->dev,
                "Regulator vio enable failed rc=%d\n", rc);
            return rc;
        }
        data->power_enabled = true;

        /*
         * The max time for the power supply rise time is 50ms.
         * Use 80ms to make sure it meets the requirements.
         */
        msleep(80);
        return rc;
    } else {
        dev_warn(&data->client->dev,
                "Power on=%d. enabled=%d\n",
                on, data->power_enabled);
        return rc;
    }
}
/*--------------------------------------------------------------*/
/*  BCT3253GU core functions                    */
/*--------------------------------------------------------------*/

static int bct3253_write_byte(struct i2c_client *client, u8 reg, u8 val)
{
    int ret = i2c_smbus_write_byte_data(client, reg, val);
    if (ret >= 0)
        return 0;

    dev_err(&client->dev, "%s: reg 0x%x, val 0x%x, err %d\n",
                        __func__, reg, val, ret);

    return ret;
}

#define BCT3253_SET_REGISTER(reg_addr, reg_name)                \
static ssize_t bct3253_store_reg##reg_addr(struct device *dev,      \
    struct device_attribute *attr, const char *buf, size_t count)   \
{                                   \
    struct bct3253_led *led = i2c_get_clientdata(to_i2c_client(dev));\
    unsigned long val;                      \
    int ret;                            \
    if (!count)                         \
        return -EINVAL;                     \
    ret = kstrtoul(buf, 16, &val);                  \
    if (ret)                            \
        return ret;                     \
    down_write(&led->rwsem);                    \
    msleep(500);                                                \
    bct3253_write_byte(led->client, reg_addr, (u8) val);        \
    up_write(&led->rwsem);                      \
    return count;                           \
}                                   \
static struct device_attribute bct3253_reg##reg_addr##_attr = {     \
    .attr = {.name = reg_name, .mode = 0644},           \
    .store = bct3253_store_reg##reg_addr,               \
};

BCT3253_SET_REGISTER(0x00, "0x00");
BCT3253_SET_REGISTER(0x01, "0x01");
BCT3253_SET_REGISTER(0x02, "0x02");
BCT3253_SET_REGISTER(0x03, "0x03");
BCT3253_SET_REGISTER(0x04, "0x04");
BCT3253_SET_REGISTER(0x05, "0x05");
BCT3253_SET_REGISTER(0x06, "0x06");
BCT3253_SET_REGISTER(0x07, "0x07");
BCT3253_SET_REGISTER(0x08, "0x08");
BCT3253_SET_REGISTER(0x09, "0x09");
BCT3253_SET_REGISTER(0x0a, "0x0a");
BCT3253_SET_REGISTER(0x0b, "0x0b");
BCT3253_SET_REGISTER(0x0c, "0x0c");
BCT3253_SET_REGISTER(0x0d, "0x0d");
BCT3253_SET_REGISTER(0x0e, "0x0e");
BCT3253_SET_REGISTER(0x0f, "0x0f");
BCT3253_SET_REGISTER(0x10, "0x10");
BCT3253_SET_REGISTER(0x11, "0x11");
BCT3253_SET_REGISTER(0x12, "0x12");
BCT3253_SET_REGISTER(0x13, "0x13");
BCT3253_SET_REGISTER(0x14, "0x14");

static struct device_attribute *bct3253_addr_attributes[] = {
    &bct3253_reg0x00_attr,
    &bct3253_reg0x01_attr,
    &bct3253_reg0x02_attr,
    &bct3253_reg0x03_attr,
    &bct3253_reg0x04_attr,
    &bct3253_reg0x05_attr,
    &bct3253_reg0x06_attr,
    &bct3253_reg0x07_attr,
    &bct3253_reg0x08_attr,
    &bct3253_reg0x09_attr,
    &bct3253_reg0x0a_attr,
    &bct3253_reg0x0b_attr,
    &bct3253_reg0x0c_attr,
    &bct3253_reg0x0d_attr,
    &bct3253_reg0x0e_attr,
    &bct3253_reg0x0f_attr,
    &bct3253_reg0x10_attr,
    &bct3253_reg0x11_attr,
    &bct3253_reg0x12_attr,
    &bct3253_reg0x13_attr,
    &bct3253_reg0x14_attr,
};

static void bct3253_enable_adv_conf(struct bct3253_led *led)
{
    int i, ret;

    for (i = 0; i < ARRAY_SIZE(bct3253_addr_attributes); i++) {
        ret = device_create_file(&led->client->dev,
                        bct3253_addr_attributes[i]);
        if (ret) {
            dev_err(&led->client->dev, "failed: sysfs file %s\n",
                    bct3253_addr_attributes[i]->attr.name);
            goto failed_remove_files;
        }
    }
    led->adf_on = 1;

    return;

failed_remove_files:
    for (i--; i >= 0; i--)
        device_remove_file(&led->client->dev,
                        bct3253_addr_attributes[i]);
}

static void bct3253_disable_adv_conf(struct bct3253_led *led)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(bct3253_addr_attributes); i++)
        device_remove_file(&led->client->dev,
                        bct3253_addr_attributes[i]);
    led->adf_on = 0;
}

static ssize_t bct3253_show_adv_conf(struct device *dev,
    struct device_attribute *attr, char *buf)
{
    struct bct3253_led *led = i2c_get_clientdata(to_i2c_client(dev));
    ssize_t ret;

    down_read(&led->rwsem);
    if (led->adf_on)
        ret = sprintf(buf, "on\n");
    else
        ret = sprintf(buf, "off\n");
    up_read(&led->rwsem);

    return ret;
}

static ssize_t bct3253_store_adv_conf(struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    struct bct3253_led *led = i2c_get_clientdata(to_i2c_client(dev));
    if (!count)
        return -EINVAL;

    down_write(&led->rwsem);
    if (!led->adf_on && !strncmp(buf, "on", 2))
        bct3253_enable_adv_conf(led);
    else if (led->adf_on && !strncmp(buf, "off", 3))
        bct3253_disable_adv_conf(led);
    up_write(&led->rwsem);

    return count;
}

static struct device_attribute bct3253_adv_conf_attr = {
    .attr = {
        .name = "advanced_configuration",
        .mode = 0644,
    },
    .show = bct3253_show_adv_conf,
    .store = bct3253_store_adv_conf,
};

#define BCT3253_CONTROL_ATTR(attr_name, name_str)           \
static ssize_t bct3253_show_##attr_name(struct device *dev,     \
    struct device_attribute *attr, char *buf)           \
{                                   \
    struct bct3253_led *led = i2c_get_clientdata(to_i2c_client(dev));\
    ssize_t ret;                            \
    down_read(&led->rwsem);                     \
    ret = sprintf(buf, "0x%02x\n", led->attr_name);         \
    up_read(&led->rwsem);                       \
    return ret;                         \
}                                   \
static ssize_t bct3253_store_##attr_name(struct device *dev,        \
    struct device_attribute *attr, const char *buf, size_t count)   \
{                                   \
    struct bct3253_led *led = i2c_get_clientdata(to_i2c_client(dev));\
    unsigned long val;                      \
    int ret;                            \
    if (!count)                         \
        return -EINVAL;                     \
    ret = kstrtoul(buf, 16, &val);                  \
    if (ret)                            \
        return ret;                     \
    down_write(&led->rwsem);                    \
    led->attr_name = val;                       \
    up_write(&led->rwsem);                      \
    return count;                           \
}                                   \
static struct device_attribute bct3253_##attr_name##_attr = {       \
    .attr = {                           \
        .name = name_str,                   \
        .mode = 0644,                       \
    },                              \
    .show = bct3253_show_##attr_name,               \
    .store = bct3253_store_##attr_name,             \
};

static struct device_attribute *bct3253_attributes[] = {
    &bct3253_adv_conf_attr,
};
static void bct3253_set_led1g_brightness(struct led_classdev *led_cdev,
                    enum led_brightness value)  
{               
    struct bct3253_led *led =       
        container_of(led_cdev, struct bct3253_led, cdev_led1g);
    //led->led_id = id;
    //led->color = clr; 
      bct3253_power_set(led,1);
    if (value == LED_OFF)
       {   
            led->state = BCT3253_OFF;
            bct3253_write_byte(led->client,0x01, 0x0); //led2 constant on  ,light on
            bct3253_power_set(led,0);
    }
      else
      {
          //  bct3253_power_set(led,1);
         msleep(100);
            bct3253_write_byte(led->client,0x00, 0x01); //reset
            bct3253_write_byte(led->client,0x00, 0x01); //reset
             bct3253_write_byte(led->client,0x02, BCT3253_CURRENT_127); //i max 12.57ma
             bct3253_write_byte(led->client,0x04, 0xb5); //  green brightness 128 total 255 ,12.5
            bct3253_write_byte(led->client,0x01, 0x02); //led2 constant on  ,light on
            led->state = BCT3253_ON;
      }
  //  bct3253_power_set(led,0);
//  schedule_work(&led->work);
}
/* [BUGFIX]-Mod-BEGIN by TCTNB.XQJ, PR-772363, 2014/08/25,modify blink time*/
static void bct3253_set_led1g_blink(struct led_classdev *led_cdev,u8 bblink)
{                               
       struct bct3253_led *led =
               container_of(led_cdev, struct bct3253_led, cdev_led1g);
       bct3253_power_set(led,1);
       msleep(100);
       if(bblink==1)
        {
            pr_err("blink on\n");
             bct3253_write_byte(led->client,0x00, 0x01); //reset
             bct3253_write_byte(led->client,0x00, 0x01); //reset

             bct3253_write_byte(led->client,0x02, BCT3253_CURRENT_127); //i max 12.57ma
             bct3253_write_byte(led->client,0x04, 0xb5); //  green brightness 128 total 255 ,12.5
             bct3253_write_byte(led->client,0x07, 0x64); //TT1 2.5s  TT1 2.5S 0.5*5
             bct3253_write_byte(led->client,0x0d, 0xa4); // led 1 mid 0x0c,0x0c/0x0f
             bct3253_write_byte(led->client,0x0e, 0x10); // 0x0100 ,duty min 0x04 ratioo=0x04/0xf
             bct3253_write_byte(led->client,0x0f, 0x22); //STEP dt2 4ms dt1 16ms
             bct3253_write_byte(led->client,0x10, 0x22);//dt4 12ms dt3 8ms
             bct3253_write_byte(led->client,0x01, 0x22); //led2 slop ,light on
            led->state = BCT3253_BLINK;
        }
	 else  if(bblink==2)
        {
             pr_err("2, blink on\n");
             bct3253_write_byte(led->client,0x00, 0x01); //reset
             bct3253_write_byte(led->client,0x00, 0x01); //reset
            bct3253_write_byte(led->client,0x01, 0x20); //reset
             bct3253_write_byte(led->client,0x02, BCT3253_CURRENT_127); //i max 12.57ma 
             bct3253_write_byte(led->client,0x04, 0xb5); //  green brightness 128 total 255 ,12.5  

             bct3253_write_byte(led->client,0x07, 0x42); //TT1.s  TT2 1s+1.0 min
             bct3253_write_byte(led->client,0x0d, 0xa4); // led 1 mid 0x0c,0x0c/0x0f
             bct3253_write_byte(led->client,0x0e, 0x00); // 0x0100 ,duty min 0x04 ratioo=0x04/0xf
             bct3253_write_byte(led->client,0x0f, 0x11); //STEP dt2 4ms dt1 16ms
             bct3253_write_byte(led->client,0x10, 0x11);//dt4 12ms dt3 8ms
             bct3253_write_byte(led->client,0x01, 0x22); //led2 slop ,light on
             led->state = BCT3253_BLINK;
        }
      else //constant
       {
            pr_err("blink off\n");
            bct3253_write_byte(led->client,0x01, 0x0); //led2 constant on  ,light on
            bct3253_write_byte(led->client,0x00, 0x01); //reset
            bct3253_write_byte(led->client,0x00, 0x01); //reset

             bct3253_write_byte(led->client,0x02, BCT3253_CURRENT_127); //i max 12.57ma 
             bct3253_write_byte(led->client,0x04, 0xb5); //  green brightness 128 total 255 ,12.5  
         bct3253_write_byte(led->client,0x01, 0x02); //led2 constant on  ,light on
            led->state = BCT3253_ON;
       }
 // bct3253_power_set(led,0);
    //  schedule_work(&led->work);              
}


static ssize_t store_blink(struct device *dev, struct device_attribute *attr,
              const char *buf, size_t count)
{

      struct led_classdev *led_cdev = dev_get_drvdata(dev);
      u8 bblink;
      pr_err("in %s,name=%s\n",__func__,led_cdev->name);
    if(*buf=='0')
        bblink=0;
    else  if(*buf=='2')
        bblink=2;
   else
        bblink=1;
    bct3253_set_led1g_blink(led_cdev,bblink);
    return count;
}
/* [BUGFIX]-Mod-END by TCTNB.XQJ*/
static DEVICE_ATTR(blink, S_IWUSR, NULL, store_blink);


/* TODO: HSB, fade, timeadj, script ... */

static int bct3253_register_led_classdev(struct bct3253_led *led)
{
    int ret;
    led->cdev_led1g.name = "led_G";
    led->cdev_led1g.brightness = LED_OFF;
    led->cdev_led1g.brightness_set = bct3253_set_led1g_brightness;
    //led->cdev_led1g.blink_set = bct3253_set_led1g_blink;

    ret = led_classdev_register(&led->client->dev, &led->cdev_led1g);
    if (ret < 0) {
        dev_err(&led->client->dev, "couldn't register LED %s\n",
                            led->cdev_led1g.name);
        goto failed_unregister_led1_G;
    }
    return 0;
#if 0
failed_unregister_led2_B:
    led_classdev_unregister(&led->cdev_led2g);
failed_unregister_led2_G:
    led_classdev_unregister(&led->cdev_led2r);
failed_unregister_led2_R:
    led_classdev_unregister(&led->cdev_led1b);
failed_unregister_led1_B:
    led_classdev_unregister(&led->cdev_led1g);
#endif
failed_unregister_led1_G:
    led_classdev_unregister(&led->cdev_led1g);

    return ret;
}

static void bct3253_unregister_led_classdev(struct bct3253_led *led)
{

        led_classdev_unregister(&led->cdev_led1g);
}

static int bct3253_init(struct i2c_client *client)
{
     int ret=0;
     bct3253_write_byte(client,0x00, 0x01); //reset

    
      ret= bct3253_write_byte(client,0x00, 0x01); //reset
#if 0
        bct3253_write_byte(client,0x02, 0x00); //i max 12.57ma 
        bct3253_write_byte(client,0x03, 0x00); //
     bct3253_write_byte(client,0x04, 0xff); // brightness 128 total 255 ,12.5

         bct3253_write_byte(client,0x05, 0x00); //



         bct3253_write_byte(client,0x06, 0x75);//0x55); 

         bct3253_write_byte(client,0x07, 0x75);//0x55); //TT1 2.5s  TT1 2.5S 0.5*5
         bct3253_write_byte(client,0x08, 0x75);//0x55); //
      bct3253_write_byte(client,0x09, 0xf7);//0xfc); // led 1 mid 0x0c,0x0c/0x0f

         bct3253_write_byte(client,0x0a, 0x20);//0xf1); //delay 2s 0x0100 ,duty min 0x04 ratioo=0x04/0xf
         bct3253_write_byte(client,0x0b, 0x18);//0x33); //STEP ALL 12MS
         bct3253_write_byte(client,0x0c, 0x43);//0x33); 

         bct3253_write_byte(client,0x0d, 0xf7);//0xfc); // led 1 mid 0x0c,0x0c/0x0f
         bct3253_write_byte(client,0x0e, 0x20);//0xf1); //delay 2s 0x0100 ,duty min 0x04 ratioo=0x04/0xf
      bct3253_write_byte(client,0x0f, 0x18);//0x33); //STEP ALL 12MS
         bct3253_write_byte(client,0x10, 0x43);//0x33); 
         bct3253_write_byte(client,0x11, 0xf7);//0xfc); // led 1 mid 0x0c,0x0c/0x0f
         bct3253_write_byte(client,0x12, 0x20);//0xf1); //delay 2s 0x0100 ,duty min 0x04 ratioo=0x04/0xf
      bct3253_write_byte(client,0x13, 0x18);//0x33); //STEP ALL 12MS
      bct3253_write_byte(client,0x14, 0x43);//0x33);    
      ret=bct3253_write_byte(client,0x01, 0x72);  //0x77 --slope mode on,0x70,slop mode ,led off ,0x07 ,constant on ,0x00 constant led off 
#endif
       return ret;
}
extern void qnnp_lbc_enable_led(u8 onff);
static int bct3253_probe(struct i2c_client *client,
            const struct i2c_device_id *id)
{
    struct bct3253_led *led;
    struct bct3253_led_platform_data *pdata;
    int ret, i;

    
    led = devm_kzalloc(&client->dev, sizeof(struct bct3253_led), GFP_KERNEL);
    if (!led) {
        qnnp_lbc_enable_led(1);/*[BUGFIX]-Mod   by TCTNB.XQJ,FR-803287 2014/10/09,add 2nd led ic sn31xx ,for compitable*/
        dev_err(&client->dev, "failed to allocate driver data\n");
        return -ENOMEM;
    }
    pr_err("xqj bct3253_probe\n");
    led->client = client;
    pdata = led->pdata = client->dev.platform_data;
    i2c_set_clientdata(client, led);
    /* check connection */
    ret = bct3253_power_init(led, 1);
    if (ret < 0)
        goto exit1;
    ret = bct3253_power_set(led, 1);
    if (ret < 0)
        goto exit2;
    /* Configure RESET GPIO (L: RESET, H: RESET cancel) */
    //gpio_request_one(pdata->reset_gpio, GPIOF_OUT_INIT_HIGH, "RGB_RESETB");

    /* Tacss = min 0.1ms */
    udelay(100);
       ret=   bct3253_init(client);
    if (ret < 0)
        goto exit3;

     bct3253_power_set(led, 0);
    
    init_rwsem(&led->rwsem);
    ret = bct3253_register_led_classdev(led);
    if (ret < 0)
        goto  exit4;;
       for (i = 0; i < ARRAY_SIZE(bct3253_attributes); i++) {   
        ret = device_create_file(&led->client->dev,
                        bct3253_attributes[i]);
        if (ret) {
            dev_err(&led->client->dev, "failed: sysfs file %s\n",
                    bct3253_attributes[i]->attr.name);
            goto exit4;
        }
    }
       ret= sysfs_create_file (&led->cdev_led1g.dev->kobj, &dev_attr_blink.attr);
      qnnp_lbc_enable_led(0);
    return 0;

exit4:
    for (i--; i >= 0; i--)
        device_remove_file(&led->client->dev, bct3253_attributes[i]);
exit3:
    bct3253_power_set(led, 0);
exit2:
    bct3253_power_init(led, 0);
exit1:
    qnnp_lbc_enable_led(1);/*[BUGFIX]-Mod   by TCTNB.XQJ,FR-803287 2014/10/09,add 2nd led ic sn31xx ,for compitable*/
    return ret;
}

static int bct3253_remove(struct i2c_client *client)
{
    struct bct3253_led *led = i2c_get_clientdata(client);
    int i;

    bct3253_unregister_led_classdev(led);
    if (led->adf_on)
        bct3253_disable_adv_conf(led);
    for (i = 0; i < ARRAY_SIZE(bct3253_attributes); i++)
        device_remove_file(&led->client->dev, bct3253_attributes[i]);
    return 0;
}

 #ifdef CONFIG_PM_SLEEP
 

static int bct3253_suspend(struct device *dev)
{
    pr_err(" bct3253_suspend do nothing\n");

    return 0;
}

static int bct3253_resume(struct device *dev)
{

    pr_err("bct resume donothing \n");


    return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(bct3253_pm, bct3253_suspend, bct3253_resume);

static const struct i2c_device_id bct3253_id[] = {
    { "BCT3253", 0 },
    { }
};




static struct of_device_id bct3253_match_table[] = {
    { .compatible = "bct,bct3253", },
    { },
};

static struct i2c_driver bct3253_i2c_driver = {
    .probe      = bct3253_probe,
       .remove      = bct3253_remove,
       .id_table    = bct3253_id,
       .driver = {
               .name    = "BCT3253",
               .owner  = THIS_MODULE,
               .of_match_table = bct3253_match_table,
    .pm = &bct3253_pm,
      },
};
static int __init bct_led_init(void)
{
       pr_info("bct3253 led driver: initialize.");
       return i2c_add_driver(&bct3253_i2c_driver);
}
late_initcall(bct_led_init);
MODULE_AUTHOR("xqj<qijun.xu@tcl.com>");
MODULE_DESCRIPTION("BCT3253 LED driver");
MODULE_LICENSE("GPL v2");
