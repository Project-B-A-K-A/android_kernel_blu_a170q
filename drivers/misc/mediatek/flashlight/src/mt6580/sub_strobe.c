#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/time.h>
#include "kd_flashlight.h"
#include <asm/io.h>
#include <asm/uaccess.h>
#include "kd_camera_typedef.h"
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/version.h>
#include <linux/mutex.h>
#include <linux/i2c.h>
#include <linux/leds.h>
#include <mt-plat/mt_boot_common.h>

/******************************************************************************
 * Debug configuration
******************************************************************************/
/* availible parameter */
/* ANDROID_LOG_ASSERT */
/* ANDROID_LOG_ERROR */
/* ANDROID_LOG_WARNING */
/* ANDROID_LOG_INFO */
/* ANDROID_LOG_DEBUG */
/* ANDROID_LOG_VERBOSE */

#define TAG_NAME "[leds_strobe.c]"
#define PK_DBG_NONE(fmt, arg...)    do {} while (0)
#define PK_DBG_FUNC(fmt, arg...)    pr_debug(TAG_NAME "%s: " fmt, __func__ , ##arg)

/*#define DEBUG_LEDS_STROBE*/
#ifdef DEBUG_LEDS_STROBE
#define PK_DBG PK_DBG_FUNC
#else
#define PK_DBG(a, ...)
#endif
/******************************************************************************
 * local variables
******************************************************************************/
static DEFINE_SPINLOCK(g_strobeSMPLock);	/* cotta-- SMP proection */


static u32 strobe_Res;
static u32 strobe_Timeus;
static BOOL g_strobe_On;

static int g_duty = -1;
static int g_timeOutTimeMs;


static struct work_struct workTimeOut;
/*****************************************************************************
Functions
*****************************************************************************/
static void work_timeOutFunc(struct work_struct *data);

struct pinctrl *substrobectrl = NULL;
struct pinctrl_state *sub_strobe_en_h = NULL;
struct pinctrl_state *sub_strobe_en_l = NULL;

void sub_strobe_gpio_init(struct platform_device *pdev)
{
	substrobectrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(substrobectrl)) 
	{
		PK_DBG("Cannot find strobe pinctrl!");
	}
	
	sub_strobe_en_h = pinctrl_lookup_state(substrobectrl, "sub_strobe_en_h");
	if (IS_ERR(sub_strobe_en_h)) 
	{
		PK_DBG("Cannot find sub_strobe_en_h pinctrl!");
	}
	
	sub_strobe_en_l = pinctrl_lookup_state(substrobectrl, "sub_strobe_en_l");
	if (IS_ERR(sub_strobe_en_l)) 
	{
		PK_DBG("Cannot find sub_strobe_en_l pinctrl!");
	}
}

int Sub_FL_Init(void)
{
	pinctrl_select_state(substrobectrl, sub_strobe_en_l);
		
	return 0;
}

int Sub_FL_Enable(void)
{
	pinctrl_select_state(substrobectrl, sub_strobe_en_h);
		
	return 0;
}



int Sub_FL_Disable(void)
{
	pinctrl_select_state(substrobectrl, sub_strobe_en_l);
	
	return 0;
}

int Sub_FL_dim_duty(kal_uint32 duty)
{
	PK_DBG(" Sub_FL_dim_duty line=%d\n", __LINE__);
	g_duty = duty;
	return 0;
}

int Sub_FL_Uninit(void)
{
	Sub_FL_Disable();
	return 0;
}
/*****************************************************************************
User interface
*****************************************************************************/
static void work_timeOutFunc(struct work_struct *data)
{
	Sub_FL_Disable();
	PK_DBG("ledTimeOut_callback\n");
}

enum hrtimer_restart subledTimeOutCallback(struct hrtimer *timer)
{
	schedule_work(&workTimeOut);
	return HRTIMER_NORESTART;
}

static struct hrtimer g_timeOutTimer;
void SubtimerInit(void)
{
	INIT_WORK(&workTimeOut, work_timeOutFunc);
	g_timeOutTimeMs = 1000;
	hrtimer_init(&g_timeOutTimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	g_timeOutTimer.function = subledTimeOutCallback;
}



static int sub_strobe_ioctl(unsigned int cmd, unsigned long arg)
{
	int i4RetValue = 0;
	int ior_shift;
	int iow_shift;
	int iowr_shift;

	ior_shift = cmd - (_IOR(FLASHLIGHT_MAGIC, 0, int));
	iow_shift = cmd - (_IOW(FLASHLIGHT_MAGIC, 0, int));
	iowr_shift = cmd - (_IOWR(FLASHLIGHT_MAGIC, 0, int));
/*	PK_DBG
	    ("LM3642 sub_strobe_ioctl() line=%d ior_shift=%d, iow_shift=%d iowr_shift=%d arg=%d\n",
	     __LINE__, ior_shift, iow_shift, iowr_shift, (int)arg);
*/
	switch (cmd) 
	{
	case FLASH_IOC_SET_TIME_OUT_TIME_MS:
		PK_DBG("FLASH_IOC_SET_TIME_OUT_TIME_MS: %d\n", (int)arg);
		g_timeOutTimeMs = arg;
		break;


	case FLASH_IOC_SET_DUTY:
		PK_DBG("FLASHLIGHT_DUTY: %d\n", (int)arg);
		Sub_FL_dim_duty(arg);
		break;


	case FLASH_IOC_SET_STEP:
		PK_DBG("FLASH_IOC_SET_STEP: %d\n", (int)arg);

		break;

	case FLASH_IOC_SET_ONOFF:
		PK_DBG("FLASHLIGHT_ONOFF: %d\n", (int)arg);
		if (arg == 1) 
		{

			int s;
			int ms;

			if (g_timeOutTimeMs > 1000) 
			{
				s = g_timeOutTimeMs / 1000;
				ms = g_timeOutTimeMs - s * 1000;
			} 
			else 
			{
				s = 0;
				ms = g_timeOutTimeMs;
			}

			//if(g_timeOutTimeMs!=0)
			if((g_timeOutTimeMs!=0) && (FACTORY_BOOT != get_boot_mode()))
            {
				ktime_t ktime;

				ktime = ktime_set(s, ms * 1000000);
				hrtimer_start(&g_timeOutTimer, ktime, HRTIMER_MODE_REL);
			}
			Sub_FL_Enable();
		} 
		else 
		{
			Sub_FL_Disable();
			hrtimer_cancel(&g_timeOutTimer);
		}
		break;
		
	default:
		PK_DBG(" No such command\n");
		i4RetValue = -EPERM;
		break;
	}
	
	return i4RetValue;
}

static int sub_strobe_open(void *pArg)
{
	int i4RetValue = 0;

	PK_DBG("sub_strobe_open line=%d\n", __LINE__);

	if (0 == strobe_Res) 
	{
		Sub_FL_Init();
		SubtimerInit();
	}
	PK_DBG("sub_strobe_open line=%d\n", __LINE__);
	spin_lock_irq(&g_strobeSMPLock);


	if (strobe_Res) 
	{
		PK_DBG(" busy!\n");
		i4RetValue = -EBUSY;
	} 
	else 
	{
		strobe_Res += 1;
	}


	spin_unlock_irq(&g_strobeSMPLock);
	PK_DBG("sub_strobe_open line=%d\n", __LINE__);

	return i4RetValue;
}

static int sub_strobe_release(void *pArg)
{
	PK_DBG(" sub_strobe_release\n");

	if (strobe_Res) 
	{
		spin_lock_irq(&g_strobeSMPLock);

		strobe_Res = 0;
		strobe_Timeus = 0;

		/* LED On Status */
		g_strobe_On = FALSE;

		spin_unlock_irq(&g_strobeSMPLock);

		Sub_FL_Uninit();
	}

	PK_DBG(" Done\n");

	return 0;
}

FLASHLIGHT_FUNCTION_STRUCT subStrobeFunc = 
{
	sub_strobe_open,
	sub_strobe_release,
	sub_strobe_ioctl
};

MUINT32 subStrobeInit(PFLASHLIGHT_FUNCTION_STRUCT *pfFunc)
{
	if (pfFunc != NULL)
		*pfFunc = &subStrobeFunc;
	return 0;
}