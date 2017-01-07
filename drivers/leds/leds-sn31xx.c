/*
 * leds-SN31XX.c - RGB LED Driver
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
*[BUGFIX]-Mod   by TCTNB.XQJ,FR-803287 2014/10/09,add 2nd led ic sn31xx 
*/

#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/leds.h>
#include <linux/of_gpio.h>
#include <linux/slab.h>
#include <linux/pm.h>
#include <linux/regulator/consumer.h>


/* POWER SUPPLY VOLTAGE RANGE */
#define SN31XX_VDD_MIN_UV  2000000
#define SN31XX_VDD_MAX_UV  3300000
#define SN31XX_VIO_MIN_UV        1750000
#define SN31XX_VIO_MAX_UV       1950000

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
    SN31XX_OFF,
    SN31XX_BLINK,
    SN31XX_ON,
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
struct SN31XX_led_platform_data{
	unsigned int	en_gpio;
	
};

struct SN31XX_led {
    struct i2c_client       *client;
    struct rw_semaphore     rwsem;
    struct work_struct      work;

    struct led_state        led[2];
    struct regulator    *vio;
   struct regulator    *vdd;
    int power_enabled;
    unsigned int	en_gpio;
    /*
     * Making led_classdev as array is not recommended, because array
     * members prevent using 'container_of' macro. So repetitive works
     * are needed.
     */
    struct led_classdev     cdev_led1g;
    /*
     * Advanced Configuration Function(ADF) mode:
     * In ADF mode, user can set registers of SN31XXGU directly,
     * therefore SN31XXGU doesn't enter reset state.
     */
    int             adf_on;
   struct pinctrl *sn31xx_pinctrl;
   struct pinctrl_state *gpio_state;
    enum led_ids            led_id;
    enum led_colors         color;
    enum led_bits           state;
};
static int SN31XX_power_init(struct SN31XX_led *data, bool on)
{
    int rc;

    if (!on) {

        if (regulator_count_voltages(data->vio) > 0)
              regulator_set_voltage(data->vio, 0,SN31XX_VIO_MAX_UV);
           regulator_put(data->vio);

        }
       else
       {
	

          data->vio = regulator_get(&data->client->dev, "vio");
          if (IS_ERR(data->vio)) {
              rc = PTR_ERR(data->vio);
              dev_err(&data->client->dev,
                "Regulator get failed vio rc=%d\n", rc);
              return rc;
          }
          if (regulator_count_voltages(data->vio) > 0) {
              rc = regulator_set_voltage(data->vio,
                                        SN31XX_VIO_MIN_UV, SN31XX_VIO_MAX_UV);
              if (rc) {
                dev_err(&data->client->dev,
                "Regulator set failed vio rc=%d\n", rc);
                return rc;
            }
        }
    }

    return 0;



}
static int SN31XX_power_set(struct SN31XX_led *data, bool on)
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
/*  SN31XXGU core functions                    */
/*--------------------------------------------------------------*/

static int SN31XX_write_byte(struct i2c_client *client, u8 reg, u8 val)
{
    int ret = i2c_smbus_write_byte_data(client, reg, val);
    if (ret >= 0)
        return 0;

    dev_err(&client->dev, "%s: reg 0x%x, val 0x%x, err %d\n",
                        __func__, reg, val, ret);

    return ret;
}

#define SN31XX_SET_REGISTER(reg_addr, reg_name)                \
static ssize_t SN31XX_store_reg##reg_addr(struct device *dev,      \
    struct device_attribute *attr, const char *buf, size_t count)   \
{                                   \
    struct SN31XX_led *led = i2c_get_clientdata(to_i2c_client(dev));\
    unsigned long val;                      \
    int ret;                            \
    if (!count)                         \
        return -EINVAL;                     \
    ret = kstrtoul(buf, 16, &val);                  \
    if (ret)                            \
        return ret;                     \
    down_write(&led->rwsem);                    \
    msleep(500);                                                \
    SN31XX_write_byte(led->client, reg_addr, (u8) val);        \
    up_write(&led->rwsem);                      \
    return count;                           \
}                                   \
static struct device_attribute SN31XX_reg##reg_addr##_attr = {     \
    .attr = {.name = reg_name, .mode = 0644},           \
    .store = SN31XX_store_reg##reg_addr,               \
};

SN31XX_SET_REGISTER(0x00, "0x00");
SN31XX_SET_REGISTER(0x01, "0x01");
SN31XX_SET_REGISTER(0x02, "0x02");
SN31XX_SET_REGISTER(0x03, "0x03");
SN31XX_SET_REGISTER(0x04, "0x04");
SN31XX_SET_REGISTER(0x07, "0x07");
SN31XX_SET_REGISTER(0x0a, "0x0a");
SN31XX_SET_REGISTER(0x10, "0x10");
SN31XX_SET_REGISTER(0x16, "0x16");
SN31XX_SET_REGISTER(0x1c, "0x1c");
SN31XX_SET_REGISTER(0x1d, "0x1d");
SN31XX_SET_REGISTER(0x2f, "0x2f");


static struct device_attribute *SN31XX_addr_attributes[] = {
    &SN31XX_reg0x00_attr,
    &SN31XX_reg0x01_attr,
    &SN31XX_reg0x02_attr,
    &SN31XX_reg0x03_attr,
    &SN31XX_reg0x04_attr,
    &SN31XX_reg0x07_attr,
    &SN31XX_reg0x0a_attr,
    &SN31XX_reg0x10_attr,
    &SN31XX_reg0x16_attr,
    &SN31XX_reg0x1c_attr,
    &SN31XX_reg0x1d_attr,
    &SN31XX_reg0x2f_attr,
};

static void SN31XX_enable_adv_conf(struct SN31XX_led *led)
{
    int i, ret;

    for (i = 0; i < ARRAY_SIZE(SN31XX_addr_attributes); i++) {
        ret = device_create_file(&led->client->dev,
                        SN31XX_addr_attributes[i]);
        if (ret) {
            dev_err(&led->client->dev, "failed: sysfs file %s\n",
                    SN31XX_addr_attributes[i]->attr.name);
            goto failed_remove_files;
        }
    }
    led->adf_on = 1;

    return;

failed_remove_files:
    for (i--; i >= 0; i--)
        device_remove_file(&led->client->dev,
                        SN31XX_addr_attributes[i]);
}

static void SN31XX_disable_adv_conf(struct SN31XX_led *led)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(SN31XX_addr_attributes); i++)
        device_remove_file(&led->client->dev,
                        SN31XX_addr_attributes[i]);
    led->adf_on = 0;
}

static ssize_t SN31XX_show_adv_conf(struct device *dev,
    struct device_attribute *attr, char *buf)
{
    struct SN31XX_led *led = i2c_get_clientdata(to_i2c_client(dev));
    ssize_t ret;

    down_read(&led->rwsem);
    if (led->adf_on)
        ret = sprintf(buf, "on\n");
    else
        ret = sprintf(buf, "off\n");
    up_read(&led->rwsem);

    return ret;
}

static ssize_t SN31XX_store_adv_conf(struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    struct SN31XX_led *led = i2c_get_clientdata(to_i2c_client(dev));
    if (!count)
        return -EINVAL;

    down_write(&led->rwsem);
    if (!led->adf_on && !strncmp(buf, "on", 2))
        SN31XX_enable_adv_conf(led);
    else if (led->adf_on && !strncmp(buf, "off", 3))
        SN31XX_disable_adv_conf(led);
    up_write(&led->rwsem);

    return count;
}

static struct device_attribute SN31XX_adv_conf_attr = {
    .attr = {
        .name = "advanced_configuration",
        .mode = 0644,
    },
    .show = SN31XX_show_adv_conf,
    .store = SN31XX_store_adv_conf,
};

#define SN31XX_CONTROL_ATTR(attr_name, name_str)           \
static ssize_t SN31XX_show_##attr_name(struct device *dev,     \
    struct device_attribute *attr, char *buf)           \
{                                   \
    struct SN31XX_led *led = i2c_get_clientdata(to_i2c_client(dev));\
    ssize_t ret;                            \
    down_read(&led->rwsem);                     \
    ret = sprintf(buf, "0x%02x\n", led->attr_name);         \
    up_read(&led->rwsem);                       \
    return ret;                         \
}                                   \
static ssize_t SN31XX_store_##attr_name(struct device *dev,        \
    struct device_attribute *attr, const char *buf, size_t count)   \
{                                   \
    struct SN31XX_led *led = i2c_get_clientdata(to_i2c_client(dev));\
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
static struct device_attribute SN31XX_##attr_name##_attr = {       \
    .attr = {                           \
        .name = name_str,                   \
        .mode = 0644,                       \
    },                              \
    .show = SN31XX_show_##attr_name,               \
    .store = SN31XX_store_##attr_name,             \
};

static struct device_attribute *SN31XX_attributes[] = {
    &SN31XX_adv_conf_attr,
};
static void SN31XX_set_led1g_brightness(struct led_classdev *led_cdev,
                    enum led_brightness value)  
{               
    struct SN31XX_led *led =       
        container_of(led_cdev, struct SN31XX_led, cdev_led1g);
    //led->led_id = id;
    //led->color = clr; 
 
      SN31XX_power_set(led,1);
	 pr_err("  gpio=%d\n",led->en_gpio);
	gpio_direction_output(led->en_gpio, 1);
    if (value == LED_OFF)
       {   
            led->state = SN31XX_OFF;
           SN31XX_write_byte(led->client,0x2f, 0x00); //reset
           SN31XX_write_byte(led->client,0x00, 0x01); //led2 constant on  ,light on
           SN31XX_power_set(led,0);
	   gpio_direction_output(led->en_gpio, 0);
    }
      else
      {
          //  SN31XX_power_set(led,1);
            msleep(100);
            SN31XX_write_byte(led->client,0x2f, 0x00); //reset
            SN31XX_write_byte(led->client,0x00, 0x20); //
            SN31XX_write_byte(led->client,0x03, 0x04); //
            SN31XX_write_byte(led->client,0x02, 0x00); // 
            SN31XX_write_byte(led->client,0x04, 0xff); //10ma
            SN31XX_write_byte(led->client,0x07, 0xff); //update
            led->state = SN31XX_ON;
      }
  //  SN31XX_power_set(led,0);
//  schedule_work(&led->work);
}                                   
static void SN31XX_set_led1g_blink(struct led_classdev *led_cdev,u8 bblink)
{                               
       struct SN31XX_led *led =
               container_of(led_cdev, struct SN31XX_led, cdev_led1g);
       SN31XX_power_set(led,1);
	    pr_err("  gpio=%d\n",led->en_gpio);
       gpio_direction_output(led->en_gpio, 1);
       msleep(100);
       if(bblink==1)
        {
            pr_err("blink on\n");
            SN31XX_write_byte(led->client,0x2f, 0x00); //reset
             SN31XX_write_byte(led->client,0x02, 0x20); //reset
            SN31XX_write_byte(led->client,0x00, 0x20); //reset          

             SN31XX_write_byte(led->client,0x03,0x04); //i max 10ma 
             SN31XX_write_byte(led->client,0x04, 0xff); 
            SN31XX_write_byte(led->client,0x07, 0xff); 

	      SN31XX_write_byte(led->client,0x0a, 0x00); //T0 1S
            SN31XX_write_byte(led->client,0x10, 0x80);//T1,Tt2
            SN31XX_write_byte(led->client,0x16, 0x88);//T3,T4 
          SN31XX_write_byte(led->client,0x1C, 0x00);//UPDATE
  
            led->state = SN31XX_BLINK;
        }
    else    if(bblink==0)//constant 
       {
        //   led->state = SN31XX_OFF;
         
           pr_err("blink off\n");
           msleep(100);
           SN31XX_write_byte(led->client,0x2f, 0x00); //reset
           SN31XX_write_byte(led->client,0x00, 0x20); //
           SN31XX_write_byte(led->client,0x03, 0x04); //
           SN31XX_write_byte(led->client,0x02, 0x00); // 
           SN31XX_write_byte(led->client,0x04, 0xff); //10ma
           SN31XX_write_byte(led->client,0x07, 0xff); //update
          
            led->state = SN31XX_ON;
       }
	else if (bblink==2)
	{
	   SN31XX_write_byte(led->client,0x2f, 0x00); //reset
	   SN31XX_write_byte(led->client,0x02, 0x20); //led mode
	    SN31XX_write_byte(led->client,0x00, 0x20);
	   SN31XX_write_byte(led->client,0x03,0x04); //i max 10ma 
	   SN31XX_write_byte(led->client,0x04, 0xc8); 
	      SN31XX_write_byte(led->client,0x07, 0xff); 

	  SN31XX_write_byte(led->client,0x0a, 0x00); //T0 1S
            SN31XX_write_byte(led->client,0x10, 0x60);//T1,Tt2
            SN31XX_write_byte(led->client,0x16, 0x66);//T3,T4 
          SN31XX_write_byte(led->client,0x1C, 0x00);//UPDATE
            led->state = SN31XX_BLINK;
	   
	}
 // SN31XX_power_set(led,0);
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
      else 
        bblink=1;

      SN31XX_set_led1g_blink(led_cdev,bblink);
      return count;
}


static DEVICE_ATTR(blink, S_IWUSR, NULL, store_blink);


/* TODO: HSB, fade, timeadj, script ... */

static int SN31XX_register_led_classdev(struct SN31XX_led *led)
{
    int ret;
    led->cdev_led1g.name = "led_G";
    led->cdev_led1g.brightness = LED_OFF;
    led->cdev_led1g.brightness_set = SN31XX_set_led1g_brightness;
    //led->cdev_led1g.blink_set = SN31XX_set_led1g_blink;

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

static void SN31XX_unregister_led_classdev(struct SN31XX_led *led)
{

        led_classdev_unregister(&led->cdev_led1g);
}

static int SN31XX_init(struct i2c_client *client)
{
     int ret=0;
 
	ret= SN31XX_write_byte(client,0x2f, 0x00); //reset
  
   

       return ret;
}

static int sn31xx_parse_dt(struct device *dev, struct SN31XX_led *led)
{
	int r = 0;

      led->en_gpio = of_get_named_gpio_flags(dev->of_node,
			"sn31,en-gpio", 0, NULL);

	if ((!gpio_is_valid(led->en_gpio)))
		return -EINVAL;
	return r;
}


extern void qnnp_lbc_enable_led(u8 onoff);
static int sn31xx_probe(struct i2c_client *client,
            const struct i2c_device_id *id)
{
    struct SN31XX_led *led;

    int ret, i;


	
    led = devm_kzalloc(&client->dev, sizeof(struct SN31XX_led), GFP_KERNEL);
    if (!led) {
        qnnp_lbc_enable_led(1);
        dev_err(&client->dev, "failed to allocate driver data\n");
        return -ENOMEM;
    }
      sn31xx_parse_dt(&client->dev, led);
     pr_err(" SN31XX_probe\n");	
       led->client = client;
	   	
  
       i2c_set_clientdata(client, led);
	
	
       pr_err("sn31 gpio=%d\n",led->en_gpio);
	ret = gpio_request(led->en_gpio, "sn31_en");
	if (ret)
		return  -ENODEV;
     
    /* check connection */
	
    ret = SN31XX_power_init(led, 1);
    if (ret < 0)
        goto exit1;
    ret = SN31XX_power_set(led, 1);
    if (ret < 0)
        goto exit2;
    /* Configure RESET GPIO (L: RESET, H: RESET cancel) */
    gpio_direction_output(led->en_gpio, 0);

    /* Tacss = min 0.1ms */
    udelay(100);
    ret=   SN31XX_init(client);
    if (ret < 0)
       goto exit3;

     SN31XX_power_set(led, 0);
    	    
    init_rwsem(&led->rwsem);

    ret = SN31XX_register_led_classdev(led);
	
    if (ret < 0)
        goto  exit4;
	i=0;
	
       for (i = 0; i < ARRAY_SIZE(SN31XX_attributes); i++) {   
        ret = device_create_file(&led->client->dev,
                        SN31XX_attributes[i]);
        if (ret) {
            dev_err(&led->client->dev, "failed: sysfs file %s\n",
                    SN31XX_attributes[i]->attr.name);
            goto exit4;
        }
    }

       ret= sysfs_create_file (&led->cdev_led1g.dev->kobj, &dev_attr_blink.attr);
      qnnp_lbc_enable_led(0);
    return 0;

exit4:
	i=0;
    for (i--; i >= 0; i--)
        device_remove_file(&led->client->dev, SN31XX_attributes[i]);
exit3:
   SN31XX_power_set(led, 0);
exit2:
    SN31XX_power_init(led, 0);
exit1:
  
    qnnp_lbc_enable_led(1);
    return ret;
}

static int SN31XX_remove(struct i2c_client *client)
{
    struct SN31XX_led *led = i2c_get_clientdata(client);
    int i;

    SN31XX_unregister_led_classdev(led);
    if (led->adf_on)
        SN31XX_disable_adv_conf(led);
    for (i = 0; i < ARRAY_SIZE(SN31XX_attributes); i++)
        device_remove_file(&led->client->dev, SN31XX_attributes[i]);
    return 0;
}

 #ifdef CONFIG_PM_SLEEP
 

static int SN31XX_suspend(struct device *dev)
{
    pr_err(" SN31XX_suspend do nothing\n");

    return 0;
}

static int SN31XX_resume(struct device *dev)
{

    pr_err("sn31 resume donothing \n");


    return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(SN31XX_pm, SN31XX_suspend, SN31XX_resume);

static const struct i2c_device_id SN31XX_id[] = {
    { "SN31XX", 0 },
    { }
};




static struct of_device_id SN31XX_match_table[] = {
    { .compatible = "sn31,SN31XX", },
    { },
};

static struct i2c_driver SN31XX_i2c_driver = {
    .probe      = sn31xx_probe,
    .remove      = SN31XX_remove,
    .id_table    = SN31XX_id,
    .driver = {
               .name    = "SN31XX",
               .owner  = THIS_MODULE,
               .of_match_table = SN31XX_match_table,
    .pm = &SN31XX_pm,
      },
};
static int __init sn31xx_led_init(void)
{
       pr_info("SN31XX led driver: initialize.");
       return i2c_add_driver(&SN31XX_i2c_driver);
}
late_initcall(sn31xx_led_init);
MODULE_AUTHOR("xqj<qijun.xu@tcl.com>");
MODULE_DESCRIPTION("SN31XX LED driver");
MODULE_LICENSE("GPL v2");
