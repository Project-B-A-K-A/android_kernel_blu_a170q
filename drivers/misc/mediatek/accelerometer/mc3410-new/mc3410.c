/*
* Copyright(C)2014 MediaTek Inc.
* Modification based on code covered by the below mentioned copyright
* and/or permission notice(S).
*/

/*****************************************************************************
 *
 * Copyright (c) 2014 mCube, Inc.  All rights reserved.
 *
 * This source is subject to the mCube Software License.
 * This software is protected by Copyright and the information and source code
 * contained herein is confidential. The software including the source code
 * may not be copied and the information contained herein may not be used or
 * disclosed except with the written permission of mCube Inc.
 *
 * All other rights reserved.
 *
 * This code and information are provided "as is" without warranty of any
 * kind, either expressed or implied, including but not limited to the
 * implied warranties of merchantability and/or fitness for a
 * particular purpose.
 *
 * The following software/firmware and/or related documentation ("mCube Software")
 * have been modified by mCube Inc. All revisions are subject to any receiver's
 * applicable license agreements with mCube Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *
 *****************************************************************************/

/*****************************************************************************
 *** HEADER FILES
 *****************************************************************************/

#include "cust_acc.h"
#include "accel.h"
#include "mc3410.h"


/*****************************************************************************
 *** CONFIGURATION
 *****************************************************************************/
/* #define _MC3XXX_SUPPORT_DOT_CALIBRATION_  *//* Just for mcube's calibration APK*/
#define _MC3XXX_SUPPORT_LPF_
#define _MC3XXX_SUPPORT_CONCURRENCY_PROTECTION_
/* #define _MC3XXX_SUPPORT_APPLY_AVERAGE_AGORITHM_ */
/* #define _MC3XXX_SUPPORT_PERIODIC_DOC_ */
/* #define _MC3XXX_SUPPORT_VPROXIMITY_SENSOR_ */
#define _MC3XXX_SUPPORT_LRF_
/* #define _MC3XXX_SUPPORT_POWER_SAVING_SHUTDOWN_POWER_ */
#define C_MAX_FIR_LENGTH	(32)
#define VIRTUAL_Z	0
#define MC3XXX_SAME_NUM    4  /*4 data is same*/

/*****************************************************************************
 *** CONSTANT / DEFINITION
 *****************************************************************************/
/**************************
 *** CONFIGURATION
 **************************/
#define MC3XXX_DEV_NAME						"MC3XXX"
#define MC3XXX_DEV_DRIVER_VERSION			  "2.1.6"
#define MC3XXX_DEV_DRIVER_VERSION_VIRTUAL_Z	"1.0.1"

/**************************
 *** COMMON
 **************************/
#define MC3XXX_AXIS_X	  0
#define MC3XXX_AXIS_Y	  1
#define MC3XXX_AXIS_Z	  2
#define MC3XXX_AXES_NUM	3
#define MC3XXX_DATA_LEN	6
#define MC3XXX_RESOLUTION_LOW	 1
#define MC3XXX_RESOLUTION_HIGH	2
#define MC3XXX_LOW_REOLUTION_DATA_SIZE	 3
#define MC3XXX_HIGH_REOLUTION_DATA_SIZE	6
#define MC3XXX_INIT_SUCC	(0)
#define MC3XXX_INIT_FAIL	(-1)
#define MC3XXX_REGMAP_LENGTH	(64)
#define DEBUG_SWITCH		0
#define C_I2C_FIFO_SIZE	 8


#if DEBUG_SWITCH
#define GSE_TAG				  "[Gsensor] "
#define GSE_ERR(fmt, args...)	pr_err(GSE_TAG fmt, ##args)
#define GSE_LOG(fmt, args...)	pr_debug(GSE_TAG fmt, ##args)
#else
#define GSE_TAG
#define GSE_ERR(fmt, args...)	do {} while (0)
#define GSE_LOG(fmt, args...)	do {} while (0)
#endif


/* Maintain  cust info here */
static struct acc_hw accel_cust;
static struct acc_hw *hw = &accel_cust;

/* For  driver get cust info */
static struct acc_hw *get_cust_acc(void)
{
	return &accel_cust;
}

static int g_samedataCounter = 0;/*count the same data number*/
static int g_predata[3] = {0,0,0};/*save the pre data of acc*/

/*****************************************************************************
 *** DATA TYPE / STRUCTURE DEFINITION / ENUM
 *****************************************************************************/
enum MCUBE_TRC {
	MCUBE_TRC_FILTER  = 0x01,
	MCUBE_TRC_RAWDATA = 0x02,
	MCUBE_TRC_IOCTL   = 0x04,
	MCUBE_TRC_CALI	= 0X08,
	MCUBE_TRC_INFO	= 0X10,
	MCUBE_TRC_REGXYZ  = 0X20,
};

struct scale_factor {
	u8	whole;
	u8	fraction;
};

struct data_resolution {
	struct scale_factor	scalefactor;
	int					sensitivity;
};

struct data_filter {
	s16	raw[C_MAX_FIR_LENGTH][MC3XXX_AXES_NUM];
	int	sum[MC3XXX_AXES_NUM];
	int	num;
	int	idx;
};

struct mc3xxx_i2c_data {
	/* ================================================ */
	struct i2c_client		  *client;
	struct acc_hw			  *hw;
	struct hwmsen_convert	   cvt;

	/* ================================================ */
	struct data_resolution	 *reso;
	atomic_t					trace;
	atomic_t					suspend;
	atomic_t					selftest;
	atomic_t					filter;
	s16						 cali_sw[MC3XXX_AXES_NUM + 1];

	/* ================================================ */
	s16						 offset[MC3XXX_AXES_NUM + 1];
	s16						 data[MC3XXX_AXES_NUM + 1];

	/* ================================================ */
	#if defined(_MC3XXX_SUPPORT_LPF_)
	atomic_t				firlen;
	atomic_t				fir_en;
	struct data_filter	  fir;
	#endif

};

#ifdef _MC3XXX_SUPPORT_LRF_
struct S_LRF_CB {
	s16	nIsNewRound;
	s16	nPreDiff;
	s16	nPreValue;
	s16	nMaxValue;
	s16	nMinValue;
	s16	nRepValue;
	s16	nNewDataMonitorCount;
};
#endif

/*****************************************************************************
 *** EXTERNAL FUNCTION
 *****************************************************************************/
/* extern struct acc_hw*	mc3xxx_get_cust_acc_hw(void); */

/*****************************************************************************
 *** STATIC FUNCTION
 *****************************************************************************/
static int mc3xxx_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int mc3xxx_i2c_remove(struct i2c_client *client);
static int _mc3xxx_i2c_auto_probe(struct i2c_client *client);
static int mc3xxx_suspend(struct i2c_client *client, pm_message_t msg);
static int mc3xxx_resume(struct i2c_client *client);
static int mc3xxx_local_init(void);
static int mc3xxx_remove(void);
static int MC3XXX_SetPowerMode(struct i2c_client *client, bool enable);
static int MC3XXX_WriteCalibration(struct i2c_client *client, int dat[MC3XXX_AXES_NUM]);
static void MC3XXX_SetGain(void);

/*****************************************************************************
 *** STATIC VARIBLE & CONTROL BLOCK DECLARATION
 *****************************************************************************/
static unsigned char	s_bResolution = 0x00;
static unsigned char	s_bPCODE	  = 0x00;
static unsigned char	s_bPCODER	 = 0x00;
static unsigned char	s_bHWID	   = 0x00;
static unsigned char	s_bMPOL	   = 0x00;
static int	s_nInitFlag = MC3XXX_INIT_FAIL;
static struct acc_init_info  mc3xxx_init_info = {
	.name   = MC3XXX_DEV_NAME,
	.init   = mc3xxx_local_init,
	.uninit = mc3xxx_remove,
	};
#ifdef CONFIG_OF
static const struct of_device_id accel_of_match[] = {
	{.compatible = "mediatek,gsensor"},
	{},
};
#endif
static const struct i2c_device_id mc3xxx_i2c_id[] = { {MC3XXX_DEV_NAME, 0}, {} };
/* static struct i2c_board_info __initdata mc3xxx_i2c_board_info = { I2C_BOARD_INFO(MC3XXX_DEV_NAME, 0x4C) }; */
static unsigned short mc3xxx_i2c_auto_probe_addr[] = { 0x4C, 0x6C, 0x4E, 0x6D, 0x6E, 0x6F };
static struct i2c_driver	mc3xxx_i2c_driver = {
							.driver = {
								  .name = MC3XXX_DEV_NAME,
						   #ifdef CONFIG_OF
								  .of_match_table = accel_of_match,
						   #endif
								  },
							.probe  = mc3xxx_i2c_probe,
							.remove = mc3xxx_i2c_remove,

							.suspend = mc3xxx_suspend,
							.resume  = mc3xxx_resume,

							.id_table = mc3xxx_i2c_id,
						};

static struct i2c_client		*mc3xxx_i2c_client;
static struct mc3xxx_i2c_data   *mc3xxx_obj_i2c_data;
static struct data_resolution	mc3xxx_offset_resolution = { {7, 8}, 256 };
static bool	mc3xxx_sensor_power;
static struct GSENSOR_VECTOR3D	gsensor_gain, gsensor_offset;
static char	selftestRes[10] = {0};
static struct file *fd_file;
static mm_segment_t oldfs;
static unsigned char offset_buf[6];
static signed int	offset_data[3];
static signed int	gain_data[3];
static unsigned char	s_baOTP_OffsetData[6] = { 0 };
static signed int	s_nIsRBM_Enabled;
static DEFINE_MUTEX(MC3XXX_i2c_mutex);

#ifdef _MC3XXX_SUPPORT_LRF_
	static struct S_LRF_CB	s_taLRF_CB[MC3XXX_AXES_NUM];
#endif

#ifdef _MC3XXX_SUPPORT_CONCURRENCY_PROTECTION_
	static struct semaphore	s_tSemaProtect;
#endif

#ifdef _MC3XXX_SUPPORT_DOT_CALIBRATION_
	static int	s_nIsCaliLoaded;
	static int	LPF_FirstRun = 1;
#endif

static int			 LPF_SamplingRate = 5;
static int			 LPF_CutoffFrequency = 0x00000004;
static unsigned int	iAReal0_X;
static unsigned int	iAcc0Lpf0_X;
static unsigned int	iAcc0Lpf1_X;
static unsigned int	iAcc1Lpf0_X;
static unsigned int	iAcc1Lpf1_X;

static unsigned int	iAReal0_Y;
static unsigned int	iAcc0Lpf0_Y;
static unsigned int	iAcc0Lpf1_Y;
static unsigned int	iAcc1Lpf0_Y;
static unsigned int	iAcc1Lpf1_Y;

static unsigned int	iAReal0_Z;
static unsigned int	iAcc0Lpf0_Z;
static unsigned int	iAcc0Lpf1_Z;
static unsigned int	iAcc1Lpf0_Z;
static unsigned int	iAcc1Lpf1_Z;

static signed char	s_bAccuracyStatus = SENSOR_STATUS_ACCURACY_MEDIUM;
static int mc3xxx_mutex_lock(void);
static void mc3xxx_mutex_unlock(void);
static void mc3xxx_mutex_init(void);


#ifdef _MC3XXX_SUPPORT_PERIODIC_DOC_
	static DECLARE_WAIT_QUEUE_HEAD(wq_mc3xxx_open_status);

	static atomic_t	s_t_mc3xxx_open_status = ATOMIC_INIT(0);

	static unsigned char	s_bIsPDOC_Enabled;
#endif

#ifdef _MC3XXX_SUPPORT_VPROXIMITY_SENSOR_
	static int	P_STATUS;
	static int	prev_P_STATUS;
#endif

/*****************************************************************************
 *** MACRO
 *****************************************************************************/
#ifdef _MC3XXX_SUPPORT_CONCURRENCY_PROTECTION_
static void mc3xxx_mutex_init(void)
{
	sema_init(&s_tSemaProtect, 1);
}

static int mc3xxx_mutex_lock(void)
{
	if (down_interruptible(&s_tSemaProtect))
			return (-ERESTARTSYS);
	return 0;
}

static void mc3xxx_mutex_unlock(void)
{
	up(&s_tSemaProtect);
}
#else
	#define mc3xxx_mutex_lock()				do {} while (0)
	#define mc3xxx_mutex_lock()				do {} while (0)
	#define mc3xxx_mutex_unlock()			  do {} while (0)
#endif

#define IS_MCFM12()	((0xC0 <= s_bHWID) && (s_bHWID <= 0xCF))
#define IS_MCFM3X()	((0x20 == s_bHWID) || ((0x22 <= s_bHWID) && (s_bHWID <= 0x2F)))

/*****************************************************************************
 *** TODO
 *****************************************************************************/
#define DATA_PATH			  "/sdcard2/mcube-register-map.txt"

#ifdef _MC3XXX_SUPPORT_DOT_CALIBRATION_
	static char	file_path[MC3XXX_BUF_SIZE]	= "/data/data/com.mcube.acc/files/mcube-calib.txt";
	static char	backup_file_path[MC3XXX_BUF_SIZE] = "/data/misc/sensors/mcube-calib.txt";
#endif

/*****************************************************************************
 *** FUNCTION
 *****************************************************************************/

/**************I2C operate API*****************************/
static int MC3XXX_i2c_read_block(struct i2c_client *client, u8 addr, u8 *data, u8 len)
{
	u8 beg = addr;
	int err;
	struct i2c_msg msgs[2] = {{0}, {0} };

	mutex_lock(&MC3XXX_i2c_mutex);

	msgs[0].addr = client->addr;
	msgs[0].flags = 0;
	msgs[0].len = 1;
	msgs[0].buf = &beg;

	msgs[1].addr = client->addr;
	msgs[1].flags = I2C_M_RD;
	msgs[1].len = len;
	msgs[1].buf = data;

	if (!client) {
		mutex_unlock(&MC3XXX_i2c_mutex);
		return -EINVAL;
	} else if (len > C_I2C_FIFO_SIZE) {
		GSE_ERR(" length %d exceeds %d\n", len, C_I2C_FIFO_SIZE);
		mutex_unlock(&MC3XXX_i2c_mutex);
		return -EINVAL;
	}
	err = i2c_transfer(client->adapter, msgs, sizeof(msgs)/sizeof(msgs[0]));
	if (err != 2) {
		GSE_ERR("i2c_transfer error: (%d %p %d) %d\n", addr, data, len, err);
		err = -EIO;
	} else
		err = 0;

	mutex_unlock(&MC3XXX_i2c_mutex);
	return err;

}

static int MC3XXX_i2c_write_block(struct i2c_client *client, u8 addr, u8 *data, u8 len)
{   /*because address also occupies one byte, the maximum length for write is 7 bytes*/
	int err, idx, num;
	char buf[C_I2C_FIFO_SIZE];

	err = 0;
	mutex_lock(&MC3XXX_i2c_mutex);
	if (!client) {
		mutex_unlock(&MC3XXX_i2c_mutex);
		return -EINVAL;
	} else if (len >= C_I2C_FIFO_SIZE) {
		GSE_ERR(" length %d exceeds %d\n", len, C_I2C_FIFO_SIZE);
		mutex_unlock(&MC3XXX_i2c_mutex);
		return -EINVAL;
	}

	num = 0;
	buf[num++] = addr;
	for (idx = 0; idx < len; idx++)
		buf[num++] = data[idx];

	err = i2c_master_send(client, buf, num);
	if (err < 0) {
		GSE_ERR("send command error!!\n");
		mutex_unlock(&MC3XXX_i2c_mutex);
		return -EFAULT;
	}
	err = 0;

	mutex_unlock(&MC3XXX_i2c_mutex);
	return err;
}


/*****************************************
 *** GetLowPassFilter
 *****************************************/
static unsigned int GetLowPassFilter(unsigned int X0, unsigned int Y1)
{
	unsigned int lTemp;

	lTemp = Y1;
	lTemp *= LPF_CutoffFrequency;	/* 4HZ LPF RC=0.04 */
	X0 *= LPF_SamplingRate;
	lTemp += X0;
	lTemp += LPF_CutoffFrequency;
	lTemp /= (LPF_CutoffFrequency + LPF_SamplingRate);
	Y1 = lTemp;

	return Y1;
}

/*****************************************
 *** openFile
 *****************************************/
static struct file *openFile(char *path, int flag, int mode)
{
	struct file *fp = NULL;

	fp = filp_open(path, flag, mode);

	if (IS_ERR(fp) || !fp->f_op) {
		GSE_LOG("Calibration File filp_open return NULL\n");
		return NULL;
	} else
		return fp;

}

/*****************************************
 *** readFile
 *****************************************/
#ifdef _MC3XXX_SUPPORT_DOT_CALIBRATION_
static int readFile(struct file *fp, char *buf, int readlen)
{
	if (fp->f_op && fp->f_op->read)
		return fp->f_op->read(fp, buf, readlen, &fp->f_pos);
	else
		return -1;
}
#endif

/*****************************************
 *** writeFile
 *****************************************/
static int writeFile(struct file *fp, char *buf, int writelen)
{
	if (fp->f_op && fp->f_op->write)
		return fp->f_op->write(fp, buf, writelen, &fp->f_pos);
	else
		return -1;
}

/*****************************************
 *** closeFile
 *****************************************/
static int closeFile(struct file *fp)
{
	filp_close(fp, NULL);
	return 0;
}

/*****************************************
 *** initKernelEnv
 *****************************************/
static void initKernelEnv(void)
{
	oldfs = get_fs();
	set_fs(KERNEL_DS);
	GSE_LOG("initKernelEnv\n");
}

/*****************************************
 *** mcube_write_log_data
 *****************************************/
static int mcube_write_log_data(struct i2c_client *client, u8 data[0x3f])
{
	#define _WRT_LOG_DATA_BUFFER_SIZE	(66 * 50)

	s16 rbm_data[3] = {0}, raw_data[3] = {0};
	int err = 0;
	char *_pszBuffer = NULL;
	int n = 0, i = 0;

	initKernelEnv();
	fd_file = openFile(DATA_PATH , O_RDWR | O_CREAT, 0);
	if (fd_file == NULL)
		GSE_LOG("mcube_write_log_data fail to open\n");
	else {
		rbm_data[MC3XXX_AXIS_X] = (s16)((data[0x0d]) | (data[0x0e] << 8));
		rbm_data[MC3XXX_AXIS_Y] = (s16)((data[0x0f]) | (data[0x10] << 8));
		rbm_data[MC3XXX_AXIS_Z] = (s16)((data[0x11]) | (data[0x12] << 8));

		raw_data[MC3XXX_AXIS_X] = (rbm_data[MC3XXX_AXIS_X] + offset_data[0]/2)*gsensor_gain.x/gain_data[0];
		raw_data[MC3XXX_AXIS_Y] = (rbm_data[MC3XXX_AXIS_Y] + offset_data[1]/2)*gsensor_gain.y/gain_data[1];
		raw_data[MC3XXX_AXIS_Z] = (rbm_data[MC3XXX_AXIS_Z] + offset_data[2]/2)*gsensor_gain.z/gain_data[2];

		_pszBuffer = kzalloc(_WRT_LOG_DATA_BUFFER_SIZE, GFP_KERNEL);

	if (NULL == _pszBuffer) {
		GSE_ERR("fail to allocate memory for buffer\n");
		return -1;
	}

	memset(_pszBuffer, 0, _WRT_LOG_DATA_BUFFER_SIZE);

	n += sprintf(_pszBuffer+n, "G-sensor RAW X = %d  Y = %d  Z = %d\n", raw_data[0] , raw_data[1] , raw_data[2]);
	n += sprintf(_pszBuffer+n, "G-sensor RBM X = %d  Y = %d  Z = %d\n", rbm_data[0] , rbm_data[1] , rbm_data[2]);

	for (i = 0; i < 63; i++)
		n += sprintf(_pszBuffer+n, "mCube register map Register[%x] = 0x%x\n", i, data[i]);

	mdelay(50);

	err = writeFile(fd_file, _pszBuffer, n);
	if (err > 0)
		GSE_LOG("buf:%s\n", _pszBuffer);
	else
		GSE_LOG("write file error %d\n", err);

	kfree(_pszBuffer);
	set_fs(oldfs);
	closeFile(fd_file);
	}

	return 0;
}

#ifdef _MC3XXX_SUPPORT_DOT_CALIBRATION_
/*****************************************
 *** mcube_read_cali_file
 *****************************************/
static int mcube_read_cali_file(struct i2c_client *client)
{
	int cali_data[3] = {0}, cali_data1[3] = {0};
	int err = 0;
	char buf[64] = {0};

	GSE_LOG("[%s]\n", __func__);

	initKernelEnv();
	fd_file = openFile(file_path, O_RDONLY, 0);

	if (fd_file == NULL) {
		GSE_LOG("%s:fail to open calibration file: %s\n", __func__, file_path);
		fd_file = openFile(backup_file_path, O_RDONLY, 0);

		if (fd_file == NULL) {
			GSE_LOG("%s:fail to open calibration file: %s\n", __func__, backup_file_path);
			cali_data[0] = 0;
			cali_data[1] = 0;
			cali_data[2] = 0;
			return -1;
		}
		GSE_LOG("Open backup calibration file successfully: %s\n", backup_file_path);
	} else
		GSE_LOG("Open calibration file successfully: %s\n", file_path);


	memset(buf, 0, sizeof(buf));

	err = readFile(fd_file, buf, sizeof(buf));
	if (err > 0)
		GSE_LOG("cali_file: buf:%s\n", buf);
	else
		GSE_LOG("read file error %d\n", err);

	set_fs(oldfs);
	closeFile(fd_file);

	err = sscanf(buf, "%d %d %d", &cali_data[MC3XXX_AXIS_X], &cali_data[MC3XXX_AXIS_Y], &cali_data[MC3XXX_AXIS_Z]);
	GSE_LOG("cali_data: %d %d %d\n", cali_data[MC3XXX_AXIS_X], cali_data[MC3XXX_AXIS_Y], cali_data[MC3XXX_AXIS_Z]);

	cali_data1[MC3XXX_AXIS_X] = cali_data[MC3XXX_AXIS_X] * gsensor_gain.x / GRAVITY_EARTH_1000;
	cali_data1[MC3XXX_AXIS_Y] = cali_data[MC3XXX_AXIS_Y] * gsensor_gain.y / GRAVITY_EARTH_1000;
	cali_data1[MC3XXX_AXIS_Z] = cali_data[MC3XXX_AXIS_Z] * gsensor_gain.z / GRAVITY_EARTH_1000;

	GSE_LOG("cali_data1: %d %d %d\n", cali_data1[MC3XXX_AXIS_X],
		cali_data1[MC3XXX_AXIS_Y], cali_data1[MC3XXX_AXIS_Z]);

	MC3XXX_WriteCalibration(client, cali_data1);

	return 0;
}

/*****************************************
 *** mcube_load_cali
 *****************************************/
static void	mcube_load_cali(struct i2c_client *pt_i2c_client)
{
	if (false == s_nIsCaliLoaded) {
		GSE_LOG("[%s] loading cali file...\n", __func__);

		if (MC3XXX_RETCODE_SUCCESS == mcube_read_cali_file(pt_i2c_client))
			s_nIsCaliLoaded = true;
	}
}

#endif /* _MC3XXX_SUPPORT_DOT_CALIBRATION_ */

/*****************************************
 *** mcube_psensor_ioctl
 *****************************************/
#ifdef _MC3XXX_SUPPORT_VPROXIMITY_SENSOR_
static long mcube_psensor_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int err = 0;
	void __user *data;
	int pSensor = 0;

	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));

	if (err) {
		PS_ERR("access error: %08X, (%2d, %2d)\n", cmd, _IOC_DIR(cmd), _IOC_SIZE(cmd));
		return -EFAULT;
	}

	switch (cmd) {
	case PSENSOR_IOCTL_SET_POSTURE:
		data = (void __user *)arg;
		if (data == NULL) {
			err = -EINVAL;
			break;
		}

		if (copy_from_user(&pSensor, data, sizeof(int))) {
			err = -EFAULT;
			break;
		}

		P_STATUS = pSensor;
		PS_LOG("IOCTL Get P_STATUS = %d", pSensor);
		break;
	default:

		PS_ERR("unknown IOCTL: 0x%08x\n", cmd);
		/* err = -ENOIOCTLCMD; */
		break;
	}
	return err;
}

/*****************************************
 *** STATIC STRUCTURE:: fops
 *****************************************/
static const struct file_operations mcube_psensor_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = mcube_psensor_ioctl,
};

/*****************************************
 *** STATIC STRUCTURE:: misc-device
 *****************************************/
static struct miscdevice mcube_psensor_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "psensor",
	.fops  = &mcube_psensor_fops,
};

/*****************************************
 *** psensor_ps_operate
 *****************************************/
static int psensor_ps_operate(void *self, uint32_t command, void *buff_in, int size_in,
	void *buff_out /*sensor_data*/, int size_out, int *actualout)
{
	int err = 0;
	int value;
	int val =  -1;
	hwm_sensor_data *sensor_data;

	switch (command) {
	case SENSOR_DELAY:
		if ((buff_in == NULL) || (size_in < sizeof(int))) {
			PS_ERR("Set delay parameter error!\n");
			err = -EINVAL;
		}
		/* Do nothing */
		break;

	case SENSOR_ENABLE:
		value = *(int *)buff_in;
		if (value) {
			if ((buff_in == NULL) || (size_in < sizeof(int))) {
				PS_ERR("Enable sensor parameter error!\n");
				err = -EINVAL;
			} else {
				value = *(int *)buff_in;
				PS_ERR("enable ps\n");
			}
		} else
			PS_ERR("disable ps\n");

		break;

	case SENSOR_GET_DATA:
		PS_LOG("fwq get ps data !!!!!!\n");
		if ((buff_out == NULL) || (size_out < sizeof(hwm_sensor_data))) {
			PS_ERR("get sensor data parameter error!\n");
			err = -EINVAL;
		} else {
			sensor_data = (hwm_sensor_data *)buff_out;
			PS_LOG("mcube sensor data P_STATUS=%d\n", P_STATUS);

		if (prev_P_STATUS != P_STATUS) {
			if (P_STATUS == 0) {
				PS_LOG("TURN ON LCD\n");
				val = 1;
				PS_LOG("Set val = 1, Proximity sensor far away\n");
			} else if (P_STATUS == 1) {
				PS_LOG("TURN OFF LCD\n");
				val = 0;
				PS_LOG("Set val = 0, Proximity sensor close\n");
			}
			PS_LOG("mcube sensor data prev_P_STATUS=%d, P_STATUS=%d\n", prev_P_STATUS, P_STATUS);
			prev_P_STATUS = P_STATUS;
			} else {
				PS_LOG("P_STATUS %5d=>%5d\n", prev_P_STATUS, P_STATUS);
				prev_P_STATUS = P_STATUS;
				val = (P_STATUS == 0) ? (1):(0);
		}
		sensor_data->values[0] = val;
		sensor_data->value_divide = 1;
		sensor_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;
		prev_P_STATUS = P_STATUS;
		}
		break;

	default:
		PS_ERR("proxmy sensor operate function has no this command %d!\n", command);
		err = -1;
	}

	return err;
}
#endif  /* end of _MC3XXX_SUPPORT_VPROXIMITY_SENSOR_ */

/*****************************************
 *** MC3XXX_power
 *****************************************/
static void MC3XXX_power(struct acc_hw *hw, unsigned int on)
{
}

#ifdef _MC3XXX_SUPPORT_DOT_CALIBRATION_
/*****************************************
 *** MC3XXX_rbm
 *****************************************/
static void MC3XXX_rbm(struct i2c_client *client, int enable)
{
	u8	_baDataBuf[3] = { 0 };

	_baDataBuf[0] = 0x43;
	MC3XXX_i2c_write_block(client, 0x07, _baDataBuf, 0x01);

	MC3XXX_i2c_read_block(client, 0x04, _baDataBuf, 0x01);

	/* GSE_LOG("[%s] REG(0x04): 0x%X, enable: %d\n", __FUNCTION__, _baDataBuf[0], enable); */

	if (0x00 == (_baDataBuf[0] & 0x40)) {
		_baDataBuf[0] = 0x6D;
		MC3XXX_i2c_write_block(client, 0x1B, _baDataBuf, 0x01);

		_baDataBuf[0] = 0x43;
		MC3XXX_i2c_write_block(client, 0x1B, _baDataBuf, 0x01);
	}

	if (1 == enable) {
		_baDataBuf[0] = 0x00;
		MC3XXX_i2c_write_block(client, 0x3B, _baDataBuf, 0x01);

		_baDataBuf[0] = 0x02;
		MC3XXX_i2c_write_block(client, 0x14, _baDataBuf, 0x01);

		if (MC3XXX_RESOLUTION_LOW == s_bResolution)
			gsensor_gain.x = gsensor_gain.y = gsensor_gain.z = 1024;

		s_nIsRBM_Enabled = 1;
		LPF_FirstRun = 1;

		GSE_LOG("set rbm!!\n");
	} else if (0 == enable) {
		_baDataBuf[0] = 0x00;
		MC3XXX_i2c_write_block(client, 0x14, _baDataBuf, 0x01);

		_baDataBuf[0] = s_bPCODER;
		MC3XXX_i2c_write_block(client, 0x3B, _baDataBuf, 0x01);

		MC3XXX_SetGain();

		s_nIsRBM_Enabled = 0;

		GSE_LOG("clear rbm!!\n");
	}

	MC3XXX_i2c_read_block(client, 0x04, _baDataBuf, 0x01);

	/* GSE_LOG("RBM CONTROL DONE - REG(0x04): 0x%X\n", _baDataBuf[0]); */

	if (_baDataBuf[0] & 0x40) {
		_baDataBuf[0] = 0x6D;
		MC3XXX_i2c_write_block(client, 0x1B, _baDataBuf, 0x01);

		_baDataBuf[0] = 0x43;
		MC3XXX_i2c_write_block(client, 0x1B, _baDataBuf, 0x01);
	}

	/* MC3XXX_i2c_read_block(client, 0x04, _baDataBuf, 0x01); */
	/* GSE_LOG("END - REG(0x04): 0x%X\n", _baDataBuf[0]); */

	_baDataBuf[0] = 0x41;
	MC3XXX_i2c_write_block(client, 0x07, _baDataBuf, 0x01);

	mdelay(220);
}

/*****************************************
 *** MC3XXX_ReadData_RBM
 *****************************************/
static int MC3XXX_ReadData_RBM(struct i2c_client *client, int data[MC3XXX_AXES_NUM])
{
	u8 addr = 0x0d;
	u8 rbm_buf[MC3XXX_DATA_LEN] = {0};
	int err = 0;
	int _nTemp = 0;

	if (NULL == client) {
		err = -EINVAL;
		return err;
	}

	err = MC3XXX_i2c_read_block(client, addr, rbm_buf, 0x06);
	if (err) {
		GSE_ERR("error: %d\n", err);
		return err;
	}

	data[MC3XXX_AXIS_X] = (s16)((rbm_buf[0]) | (rbm_buf[1] << 8));
	data[MC3XXX_AXIS_Y] = (s16)((rbm_buf[2]) | (rbm_buf[3] << 8));
	data[MC3XXX_AXIS_Z] = (s16)((rbm_buf[4]) | (rbm_buf[5] << 8));

	GSE_LOG("rbm_buf<<<<<[%02x %02x %02x %02x %02x %02x]\n",
		rbm_buf[0], rbm_buf[2], rbm_buf[2], rbm_buf[3], rbm_buf[4], rbm_buf[5]);
	GSE_LOG("RBM<<<<<[%04x %04x %04x]\n", data[MC3XXX_AXIS_X], data[MC3XXX_AXIS_Y], data[MC3XXX_AXIS_Z]);
	GSE_LOG("RBM<<<<<[%04d %04d %04d]\n", data[MC3XXX_AXIS_X], data[MC3XXX_AXIS_Y], data[MC3XXX_AXIS_Z]);

	if (MC3XXX_PCODE_3250 == s_bPCODE) {
		_nTemp = data[MC3XXX_AXIS_X];
		data[MC3XXX_AXIS_X] = data[MC3XXX_AXIS_Y];
		data[MC3XXX_AXIS_Y] = -_nTemp;
		GSE_LOG("[%s] 3250 read remap\n", __func__);
	} else {
		if (s_bMPOL & 0x01)
			data[MC3XXX_AXIS_X] = -data[MC3XXX_AXIS_X];
		if (s_bMPOL & 0x02)
			data[MC3XXX_AXIS_Y] = -data[MC3XXX_AXIS_Y]);
	}
	return err;
}
#endif /* _MC3XXX_SUPPORT_DOT_CALIBRATION_ */

/*****************************************
 *** MC3XXX_ValidateSensorIC
 *****************************************/
static int MC3XXX_ValidateSensorIC(unsigned char *pbPCode, unsigned char *pbHwID)
{
	GSE_LOG("[%s] *pbPCode: 0x%02X, *pbHwID: 0x%02X\n", __func__, *pbPCode, *pbHwID);

	if ((0x01 == *pbHwID) || (0x03 == *pbHwID)
		|| ((0x04 <= *pbHwID) && (*pbHwID <= 0x0F))) {
		if ((MC3XXX_PCODE_3210 == *pbPCode) || (MC3XXX_PCODE_3230 == *pbPCode)
			|| (MC3XXX_PCODE_3250 == *pbPCode))
			return MC3XXX_RETCODE_SUCCESS;
	} else if ((0x02 == *pbHwID) || (0x21 == *pbHwID)
		 || ((0x10 <= *pbHwID) && (*pbHwID <= 0x1F))) {
		if ((MC3XXX_PCODE_3210 == *pbPCode) || (MC3XXX_PCODE_3230  == *pbPCode)
			|| (MC3XXX_PCODE_3250 == *pbPCode)
				|| (MC3XXX_PCODE_3410 == *pbPCode) || (MC3XXX_PCODE_3410N == *pbPCode)
					|| (MC3XXX_PCODE_3430 == *pbPCode) || (MC3XXX_PCODE_3430N == *pbPCode)) {
			return MC3XXX_RETCODE_SUCCESS;
		}
	} else if ((0xC0 <= *pbHwID) && (*pbHwID <= 0xCF)) {
		*pbPCode = (*pbPCode & 0x71);

	if ((MC3XXX_PCODE_3510 == *pbPCode) || (MC3XXX_PCODE_3530 == *pbPCode))
		return MC3XXX_RETCODE_SUCCESS;
	} else if ((0x20 == *pbHwID) || ((0x22 <= *pbHwID) && (*pbHwID <= 0x2F))) {
		*pbPCode = (*pbPCode & 0xF1);

		if ((MC3XXX_PCODE_3210 == *pbPCode) || (MC3XXX_PCODE_3216 == *pbPCode)
			|| (MC3XXX_PCODE_3236 == *pbPCode) || (MC3XXX_PCODE_RESERVE_1 == *pbPCode)
			|| (MC3XXX_PCODE_RESERVE_2 == *pbPCode) || (MC3XXX_PCODE_RESERVE_3 == *pbPCode)
			|| (MC3XXX_PCODE_RESERVE_4 == *pbPCode) || (MC3XXX_PCODE_RESERVE_5 == *pbPCode)
			|| (MC3XXX_PCODE_RESERVE_6 == *pbPCode) || (MC3XXX_PCODE_RESERVE_7 == *pbPCode)
			|| (MC3XXX_PCODE_RESERVE_8 == *pbPCode) || (MC3XXX_PCODE_RESERVE_9 == *pbPCode))
			return MC3XXX_RETCODE_SUCCESS;

	}

	return MC3XXX_RETCODE_ERROR_IDENTIFICATION;
}

/*****************************************
 *** MC3XXX_Read_Chip_ID
 *****************************************/
static int MC3XXX_Read_Chip_ID(struct i2c_client *client, char *buf)
{
	u8	 _bChipID[4] = { 0 };

	GSE_LOG("[%s]\n", __func__);

	if (!buf || !client)
		return -EINVAL;

	if (MC3XXX_i2c_read_block(client, 0x3C, _bChipID, 4)) {
		GSE_ERR("[%s] i2c read fail\n", __func__);
		_bChipID[0] = 0;
		_bChipID[1] = 0;
		_bChipID[2] = 0;
		_bChipID[3] = 0;
	}

	GSE_LOG("[%s] %02X-%02X-%02X-%02X\n", __func__, _bChipID[3], _bChipID[2], _bChipID[1], _bChipID[0]);

	return sprintf(buf, "%02X-%02X-%02X-%02X\n", _bChipID[3], _bChipID[2], _bChipID[1], _bChipID[0]);
}

/*****************************************
 *** MC3XXX_Read_Reg_Map
 *****************************************/
static int MC3XXX_Read_Reg_Map(struct i2c_client *p_i2c_client, u8 *pbUserBuf)
{
	u8	 _baData[MC3XXX_REGMAP_LENGTH] = { 0 };
	int	_nIndex = 0;

	GSE_LOG("[%s]\n", __func__);

	if (NULL == p_i2c_client)
		return (-EINVAL);

	for (_nIndex = 0; _nIndex < MC3XXX_REGMAP_LENGTH; _nIndex++) {
		MC3XXX_i2c_read_block(p_i2c_client, _nIndex, &_baData[_nIndex], 1);

		if (NULL != pbUserBuf)
			pbUserBuf[_nIndex] = _baData[_nIndex];

		GSE_LOG("[Gsensor] REG[0x%02X] = 0x%02X\n", _nIndex, _baData[_nIndex]);
	}

	mcube_write_log_data(p_i2c_client, _baData);

	return 0;
}

/*****************************************
 *** MC3XXX_SaveDefaultOffset
 *****************************************/
static void MC3XXX_SaveDefaultOffset(struct i2c_client *p_i2c_client)
{
	GSE_LOG("[%s]\n", __func__);

	MC3XXX_i2c_read_block(p_i2c_client, 0x21, &s_baOTP_OffsetData[0], 3);
	MC3XXX_i2c_read_block(p_i2c_client, 0x24, &s_baOTP_OffsetData[3], 3);

	GSE_LOG("s_baOTP_OffsetData: 0x%02X - 0x%02X - 0x%02X - 0x%02X - 0x%02X - 0x%02X\n",
		s_baOTP_OffsetData[0], s_baOTP_OffsetData[1], s_baOTP_OffsetData[2],
		s_baOTP_OffsetData[3], s_baOTP_OffsetData[4], s_baOTP_OffsetData[5]);
}

/*****************************************
 *** MC3XXX_LPF
 *****************************************/
#ifdef _MC3XXX_SUPPORT_LPF_
static void MC3XXX_LPF(struct mc3xxx_i2c_data *priv, s16 data[MC3XXX_AXES_NUM])
{
	if (atomic_read(&priv->filter)) {
		if (atomic_read(&priv->fir_en) && !atomic_read(&priv->suspend)) {
			int idx, firlen = atomic_read(&priv->firlen);

			if (priv->fir.num < firlen) {
				priv->fir.raw[priv->fir.num][MC3XXX_AXIS_X] = data[MC3XXX_AXIS_X];
				priv->fir.raw[priv->fir.num][MC3XXX_AXIS_Y] = data[MC3XXX_AXIS_Y];
				priv->fir.raw[priv->fir.num][MC3XXX_AXIS_Z] = data[MC3XXX_AXIS_Z];
				priv->fir.sum[MC3XXX_AXIS_X] += data[MC3XXX_AXIS_X];
				priv->fir.sum[MC3XXX_AXIS_Y] += data[MC3XXX_AXIS_Y];
				priv->fir.sum[MC3XXX_AXIS_Z] += data[MC3XXX_AXIS_Z];

				if (atomic_read(&priv->trace) & MCUBE_TRC_FILTER) {
					GSE_LOG("add [%2d] [%5d %5d %5d] => [%5d %5d %5d]\n", priv->fir.num,
						priv->fir.raw[priv->fir.num][MC3XXX_AXIS_X],
						priv->fir.raw[priv->fir.num][MC3XXX_AXIS_Y],
						priv->fir.raw[priv->fir.num][MC3XXX_AXIS_Z],
						priv->fir.sum[MC3XXX_AXIS_X],
						priv->fir.sum[MC3XXX_AXIS_Y], priv->fir.sum[MC3XXX_AXIS_Z]);
				}

				priv->fir.num++;
				priv->fir.idx++;
			} else {
				idx = priv->fir.idx % firlen;
				priv->fir.sum[MC3XXX_AXIS_X] -= priv->fir.raw[idx][MC3XXX_AXIS_X];
				priv->fir.sum[MC3XXX_AXIS_Y] -= priv->fir.raw[idx][MC3XXX_AXIS_Y];
				priv->fir.sum[MC3XXX_AXIS_Z] -= priv->fir.raw[idx][MC3XXX_AXIS_Z];
				priv->fir.raw[idx][MC3XXX_AXIS_X] = data[MC3XXX_AXIS_X];
				priv->fir.raw[idx][MC3XXX_AXIS_Y] = data[MC3XXX_AXIS_Y];
				priv->fir.raw[idx][MC3XXX_AXIS_Z] = data[MC3XXX_AXIS_Z];
				priv->fir.sum[MC3XXX_AXIS_X] += data[MC3XXX_AXIS_X];
				priv->fir.sum[MC3XXX_AXIS_Y] += data[MC3XXX_AXIS_Y];
				priv->fir.sum[MC3XXX_AXIS_Z] += data[MC3XXX_AXIS_Z];
				priv->fir.idx++;
				data[MC3XXX_AXIS_X] = priv->fir.sum[MC3XXX_AXIS_X]/firlen;
				data[MC3XXX_AXIS_Y] = priv->fir.sum[MC3XXX_AXIS_Y]/firlen;
				data[MC3XXX_AXIS_Z] = priv->fir.sum[MC3XXX_AXIS_Z]/firlen;

				if (atomic_read(&priv->trace) & MCUBE_TRC_FILTER) {
					GSE_LOG("add [%2d] [%5d %5d %5d] => [%5d %5d %5d] : [%5d %5d %5d]\n", idx,
					priv->fir.raw[idx][MC3XXX_AXIS_X], priv->fir.raw[idx][MC3XXX_AXIS_Y],
					priv->fir.raw[idx][MC3XXX_AXIS_Z], priv->fir.sum[MC3XXX_AXIS_X],
					priv->fir.sum[MC3XXX_AXIS_Y], priv->fir.sum[MC3XXX_AXIS_Z],
					data[MC3XXX_AXIS_X], data[MC3XXX_AXIS_Y], data[MC3XXX_AXIS_Z]);
				}
			}
		}
	}
}
#endif	/* END OF #ifdef _MC3XXX_SUPPORT_LPF_ */

#ifdef _MC3XXX_SUPPORT_LRF_
/*****************************************
 *** _MC3XXX_LowResFilter
 *****************************************/
static void _MC3XXX_LowResFilter(s16 nAxis, s16 naData[MC3XXX_AXES_NUM])
{
	#define _LRF_DIFF_COUNT_POS				  2
	#define _LRF_DIFF_COUNT_NEG				  (-_LRF_DIFF_COUNT_POS)
	#define _LRF_DIFF_BOUNDARY_POS			   (_LRF_DIFF_COUNT_POS + 1)
	#define _LRF_DIFF_BOUNDARY_NEG			   (_LRF_DIFF_COUNT_NEG - 1)
	#define _LRF_DIFF_DATA_UNCHANGE_MAX_COUNT	11

	signed int	_nCurrDiff = 0;
	signed int	_nSumDiff  = 0;
	s16		   _nCurrData = naData[nAxis];

	_nCurrDiff = (_nCurrData - s_taLRF_CB[nAxis].nRepValue);

	if ((_LRF_DIFF_COUNT_NEG < _nCurrDiff) && (_nCurrDiff < _LRF_DIFF_COUNT_POS)) {
		if (s_taLRF_CB[nAxis].nIsNewRound) {
			s_taLRF_CB[nAxis].nMaxValue = _nCurrData;
			s_taLRF_CB[nAxis].nMinValue = _nCurrData;

			s_taLRF_CB[nAxis].nIsNewRound = 0;
			s_taLRF_CB[nAxis].nNewDataMonitorCount = 0;
		} else {
			if (_nCurrData > s_taLRF_CB[nAxis].nMaxValue)
				s_taLRF_CB[nAxis].nMaxValue = _nCurrData;
			else if (_nCurrData < s_taLRF_CB[nAxis].nMinValue)
				s_taLRF_CB[nAxis].nMinValue = _nCurrData;

			if (s_taLRF_CB[nAxis].nMinValue != s_taLRF_CB[nAxis].nMaxValue) {
				if (_nCurrData == s_taLRF_CB[nAxis].nPreValue)
					s_taLRF_CB[nAxis].nNewDataMonitorCount++;
				else
				s_taLRF_CB[nAxis].nNewDataMonitorCount = 0;
			}
		}

		if (1 != (s_taLRF_CB[nAxis].nMaxValue - s_taLRF_CB[nAxis].nMinValue))
			s_taLRF_CB[nAxis].nRepValue = ((s_taLRF_CB[nAxis].nMaxValue + s_taLRF_CB[nAxis]
				.nMinValue) / 2);

		_nSumDiff = (_nCurrDiff + s_taLRF_CB[nAxis].nPreDiff);

		if (_nCurrDiff)
			s_taLRF_CB[nAxis].nPreDiff = _nCurrDiff;

		if ((_LRF_DIFF_BOUNDARY_NEG < _nSumDiff) && (_nSumDiff < _LRF_DIFF_BOUNDARY_POS)) {
			if (_LRF_DIFF_DATA_UNCHANGE_MAX_COUNT > s_taLRF_CB[nAxis].nNewDataMonitorCount) {
				naData[nAxis] = s_taLRF_CB[nAxis].nRepValue;
				goto _LRF_RETURN;
			}
		}
	}

	s_taLRF_CB[nAxis].nRepValue   = _nCurrData;
	s_taLRF_CB[nAxis].nPreDiff	= 0;
	s_taLRF_CB[nAxis].nIsNewRound = 1;

_LRF_RETURN:

	GSE_LOG(">>>>> [_MC3XXX_LowResFilter][%d] _nCurrDiff: %4d _nSumDiff: %4d _nCurrData:%4d	 Rep:%4d\n",
		nAxis, _nCurrDiff, _nSumDiff, _nCurrData, s_taLRF_CB[nAxis].nRepValue);

	s_taLRF_CB[nAxis].nPreValue = _nCurrData;

	#undef _LRF_DIFF_COUNT_POS
	#undef _LRF_DIFF_COUNT_NEG
	#undef _LRF_DIFF_BOUNDARY_POS
	#undef _LRF_DIFF_BOUNDARY_NEG
	#undef _LRF_DIFF_DATA_UNCHANGE_MAX_COUNT
}
#endif	/* END OF #ifdef _MC3XXX_SUPPORT_LRF_ */

/*****************************************
 *** _MC3XXX_ReadData_RBM2RAW
 *****************************************/
static void	_MC3XXX_ReadData_RBM2RAW(s16 waData[MC3XXX_AXES_NUM])
{
	struct mc3xxx_i2c_data   *_pt_i2c_obj = mc3xxx_obj_i2c_data;

	waData[MC3XXX_AXIS_X] = (waData[MC3XXX_AXIS_X] + offset_data[MC3XXX_AXIS_X] / 2)
		* 1024 / gain_data[MC3XXX_AXIS_X] + 8096;
	waData[MC3XXX_AXIS_Y] = (waData[MC3XXX_AXIS_Y] + offset_data[MC3XXX_AXIS_Y] / 2)
		* 1024 / gain_data[MC3XXX_AXIS_Y] + 8096;
	waData[MC3XXX_AXIS_Z] = (waData[MC3XXX_AXIS_Z] + offset_data[MC3XXX_AXIS_Z] / 2)
		* 1024 / gain_data[MC3XXX_AXIS_Z] + 8096;

	if (atomic_read(&_pt_i2c_obj->trace) & MCUBE_TRC_INFO)
		GSE_LOG("RBM->RAW <<<<<[%08d %08d %08d]\n", waData[MC3XXX_AXIS_X],
			waData[MC3XXX_AXIS_Y], waData[MC3XXX_AXIS_Z]);


	iAReal0_X			 = (0x0010 * waData[MC3XXX_AXIS_X]);
	iAcc1Lpf0_X		   = GetLowPassFilter(iAReal0_X, iAcc1Lpf1_X);
	iAcc0Lpf0_X		   = GetLowPassFilter(iAcc1Lpf0_X, iAcc0Lpf1_X);
	waData[MC3XXX_AXIS_X] = (iAcc0Lpf0_X / 0x0010);

	iAReal0_Y			 = (0x0010 * waData[MC3XXX_AXIS_Y]);
	iAcc1Lpf0_Y		   = GetLowPassFilter(iAReal0_Y, iAcc1Lpf1_Y);
	iAcc0Lpf0_Y		   = GetLowPassFilter(iAcc1Lpf0_Y, iAcc0Lpf1_Y);
	waData[MC3XXX_AXIS_Y] = (iAcc0Lpf0_Y / 0x0010);

	iAReal0_Z			 = (0x0010 * waData[MC3XXX_AXIS_Z]);
	iAcc1Lpf0_Z		   = GetLowPassFilter(iAReal0_Z, iAcc1Lpf1_Z);
	iAcc0Lpf0_Z		   = GetLowPassFilter(iAcc1Lpf0_Z, iAcc0Lpf1_Z);
	waData[MC3XXX_AXIS_Z] = (iAcc0Lpf0_Z / 0x0010);
	if (atomic_read(&_pt_i2c_obj->trace) & MCUBE_TRC_INFO)
		GSE_LOG("RBM->RAW->LPF <<<<<[%08d %08d %08d]\n", waData[MC3XXX_AXIS_X],
			waData[MC3XXX_AXIS_Y], waData[MC3XXX_AXIS_Z]);

	waData[MC3XXX_AXIS_X] = (waData[MC3XXX_AXIS_X] - 8096) * gsensor_gain.x / 1024;
	waData[MC3XXX_AXIS_Y] = (waData[MC3XXX_AXIS_Y] - 8096) * gsensor_gain.y / 1024;
	waData[MC3XXX_AXIS_Z] = (waData[MC3XXX_AXIS_Z] - 8096) * gsensor_gain.z / 1024;
	if (atomic_read(&_pt_i2c_obj->trace) & MCUBE_TRC_INFO)
		GSE_LOG("RBM->RAW->LPF->RAW <<<<<[%08d %08d %08d]\n", waData[MC3XXX_AXIS_X],
			waData[MC3XXX_AXIS_Y], waData[MC3XXX_AXIS_Z]);

	iAcc0Lpf1_X = iAcc0Lpf0_X;
	iAcc1Lpf1_X = iAcc1Lpf0_X;
	iAcc0Lpf1_Y = iAcc0Lpf0_Y;
	iAcc1Lpf1_Y = iAcc1Lpf0_Y;
	iAcc0Lpf1_Z = iAcc0Lpf0_Z;
	iAcc1Lpf1_Z = iAcc1Lpf0_Z;
}

/*****************************************
 *** MC3XXX_ReadData
 *****************************************/
static int	MC3XXX_ReadData(struct i2c_client *pt_i2c_client, s16 waData[MC3XXX_AXES_NUM])
{
	u8	_baData[MC3XXX_DATA_LEN] = { 0 };
	s16 _nTemp = 0;
	#ifdef _MC3XXX_SUPPORT_LPF_
		struct mc3xxx_i2c_data   *_ptPrivData = NULL;
	#endif
	struct mc3xxx_i2c_data   *_pt_i2c_obj = ((struct mc3xxx_i2c_data *) i2c_get_clientdata(pt_i2c_client));

	if (atomic_read(&_pt_i2c_obj->trace) & MCUBE_TRC_INFO)
		GSE_LOG("[%s] s_nIsRBM_Enabled: %d\n", __func__, s_nIsRBM_Enabled);

	if (NULL == pt_i2c_client) {
		GSE_ERR("ERR: Null Pointer\n");

	return MC3XXX_RETCODE_ERROR_NULL_POINTER;
	}

	if (!s_nIsRBM_Enabled) {
		if (MC3XXX_RESOLUTION_LOW == s_bResolution) {
			if (MC3XXX_i2c_read_block(pt_i2c_client, MC3XXX_REG_XOUT, _baData,
			MC3XXX_LOW_REOLUTION_DATA_SIZE)) {
				GSE_ERR("ERR: fail to read data via I2C!\n");

			return MC3XXX_RETCODE_ERROR_I2C;
			}

			waData[MC3XXX_AXIS_X] = ((s8) _baData[0]);
			waData[MC3XXX_AXIS_Y] = ((s8) _baData[1]);
			waData[MC3XXX_AXIS_Z] = ((s8) _baData[2]);

			if (atomic_read(&_pt_i2c_obj->trace) & MCUBE_TRC_INFO)
				GSE_LOG("[%s][low] X: %d, Y: %d, Z: %d\n",
					__func__, waData[MC3XXX_AXIS_X], waData[MC3XXX_AXIS_Y], waData[MC3XXX_AXIS_Z]);

		#ifdef _MC3XXX_SUPPORT_LRF_
			_MC3XXX_LowResFilter(MC3XXX_AXIS_X, waData);
			_MC3XXX_LowResFilter(MC3XXX_AXIS_Y, waData);
			_MC3XXX_LowResFilter(MC3XXX_AXIS_Z, waData);
		#endif
		} else if (MC3XXX_RESOLUTION_HIGH == s_bResolution) {
			if (MC3XXX_i2c_read_block(pt_i2c_client, MC3XXX_REG_XOUT_EX_L, _baData,
			MC3XXX_HIGH_REOLUTION_DATA_SIZE)) {
				GSE_ERR("ERR: fail to read data via I2C!\n");

			return MC3XXX_RETCODE_ERROR_I2C;
			}

			waData[MC3XXX_AXIS_X] = ((signed short) ((_baData[0]) | (_baData[1]<<8)));
			waData[MC3XXX_AXIS_Y] = ((signed short) ((_baData[2]) | (_baData[3]<<8)));
			waData[MC3XXX_AXIS_Z] = ((signed short) ((_baData[4]) | (_baData[5]<<8)));
			if (atomic_read(&_pt_i2c_obj->trace) & MCUBE_TRC_INFO)
				GSE_LOG("[%s][high] X: %d, Y: %d, Z: %d\n",
					__func__, waData[MC3XXX_AXIS_X], waData[MC3XXX_AXIS_Y], waData[MC3XXX_AXIS_Z]);

		}

		if (atomic_read(&_pt_i2c_obj->trace) & MCUBE_TRC_INFO)
			GSE_LOG("RAW<<<<<[%04d %04d %04d]\n",
				waData[MC3XXX_AXIS_X], waData[MC3XXX_AXIS_Y], waData[MC3XXX_AXIS_Z]);

	#ifdef _MC3XXX_SUPPORT_LPF_
		_ptPrivData = i2c_get_clientdata(pt_i2c_client);

		MC3XXX_LPF(_ptPrivData, waData);
		if (atomic_read(&_pt_i2c_obj->trace) & MCUBE_TRC_INFO)
			GSE_LOG("LPF<<<<<[%04d %04d %04d]\n", waData[MC3XXX_AXIS_X], waData[MC3XXX_AXIS_Y],
				waData[MC3XXX_AXIS_Z]);
	#endif
	} else {
		if (MC3XXX_i2c_read_block(pt_i2c_client, MC3XXX_REG_XOUT_EX_L, _baData,
		MC3XXX_HIGH_REOLUTION_DATA_SIZE)) {
			GSE_ERR("ERR: fail to read data via I2C!\n");

		return MC3XXX_RETCODE_ERROR_I2C;
		}

		waData[MC3XXX_AXIS_X] = ((s16)((_baData[0]) | (_baData[1] << 8)));
		waData[MC3XXX_AXIS_Y] = ((s16)((_baData[2]) | (_baData[3] << 8)));
		waData[MC3XXX_AXIS_Z] = ((s16)((_baData[4]) | (_baData[5] << 8)));

		if (atomic_read(&_pt_i2c_obj->trace) & MCUBE_TRC_INFO)
			GSE_LOG("RBM<<<<<[%08d %08d %08d]\n", waData[MC3XXX_AXIS_X], waData[MC3XXX_AXIS_Y],
				waData[MC3XXX_AXIS_Z]);

		_MC3XXX_ReadData_RBM2RAW(waData);
	}

	if (MC3XXX_PCODE_3250 == s_bPCODE) {
		_nTemp = waData[MC3XXX_AXIS_X];
		waData[MC3XXX_AXIS_X] = waData[MC3XXX_AXIS_Y];
		waData[MC3XXX_AXIS_Y] = -_nTemp;
		GSE_LOG("[%s] 3250 read remap\n", __func__);
	} else {
		if (s_bMPOL & 0x01)
			waData[MC3XXX_AXIS_X] = -waData[MC3XXX_AXIS_X];
		if (s_bMPOL & 0x02)
			waData[MC3XXX_AXIS_Y] = -waData[MC3XXX_AXIS_Y];
	}
	return MC3XXX_RETCODE_SUCCESS;
}

/*****************************************
 *** MC3XXX_ReadOffset
 *****************************************/
static int MC3XXX_ReadOffset(struct i2c_client *client, s16 ofs[MC3XXX_AXES_NUM])
{
	int err = 0;
	u8 off_data[6] = {0};
	s16 _nTemp = 0;

	if (MC3XXX_RESOLUTION_HIGH == s_bResolution) {
		err = MC3XXX_i2c_read_block(client, MC3XXX_REG_XOUT_EX_L, off_data, MC3XXX_DATA_LEN);
		if (err) {
			GSE_ERR("error: %d\n", err);
			return err;
		}
		ofs[MC3XXX_AXIS_X] = ((s16)(off_data[0]))|((s16)(off_data[1])<<8);
		ofs[MC3XXX_AXIS_Y] = ((s16)(off_data[2]))|((s16)(off_data[3])<<8);
		ofs[MC3XXX_AXIS_Z] = ((s16)(off_data[4]))|((s16)(off_data[5])<<8);
	} else if (MC3XXX_RESOLUTION_LOW == s_bResolution) {
		err = MC3XXX_i2c_read_block(client, 0, off_data, 3);
		if (err) {
			GSE_ERR("error: %d\n", err);
			return err;
		}
		ofs[MC3XXX_AXIS_X] = (s8)off_data[0];
		ofs[MC3XXX_AXIS_Y] = (s8)off_data[1];
		ofs[MC3XXX_AXIS_Z] = (s8)off_data[2];
	}

	GSE_LOG("MC3XXX_ReadOffset %d %d %d\n", ofs[MC3XXX_AXIS_X] , ofs[MC3XXX_AXIS_Y], ofs[MC3XXX_AXIS_Z]);

	if (MC3XXX_PCODE_3250 == s_bPCODE) {
		_nTemp = ofs[0];
		ofs[0] = ofs[1];
		ofs[1] = -_nTemp;
		GSE_LOG("[%s] 3250 read remap\n", __func__);
	} else {
		if (s_bMPOL & 0x01)
			ofs[0] = -ofs[0];
		if (s_bMPOL & 0x02)
			ofs[1] = -ofs[1];
	}
	return err;
}

/*****************************************
 *** MC3XXX_ResetCalibration
 *****************************************/
static int MC3XXX_ResetCalibration(struct i2c_client *client)
{
	struct mc3xxx_i2c_data *obj = i2c_get_clientdata(client);
	u8 buf[MC3XXX_AXES_NUM] = {0x00, 0x00, 0x00};
	s16 tmp = 0;
	int err = 0;
	u8  bMsbFilter	   = 0x3F;
	s16 wSignBitMask	 = 0x2000;
	s16 wSignPaddingBits = 0xC000;

	buf[0] = 0x43;
	err = MC3XXX_i2c_write_block(client, 0x07, buf, 1);
	if (err)
		GSE_ERR("error 0x07: %d\n", err);

	err = MC3XXX_i2c_write_block(client, 0x21, offset_buf, 6);
	if (err)
		GSE_ERR("error: %d\n", err);

	buf[0] = 0x41;
	err = MC3XXX_i2c_write_block(client, 0x07, buf, 1);
	if (err)
		GSE_ERR("error: %d\n", err);

	mdelay(20);

	if (IS_MCFM12() || IS_MCFM3X()) {
		bMsbFilter	   = 0x7F;
		wSignBitMask	 = 0x4000;
		wSignPaddingBits = 0x8000;
	}

	tmp = ((offset_buf[1] & bMsbFilter) << 8) + offset_buf[0];
	if (tmp & wSignBitMask)
		tmp |= wSignPaddingBits;
	offset_data[0] = tmp;

	tmp = ((offset_buf[3] & bMsbFilter) << 8) + offset_buf[2];
	if (tmp & wSignBitMask)
		tmp |= wSignPaddingBits;
	offset_data[1] = tmp;

	tmp = ((offset_buf[5] & bMsbFilter) << 8) + offset_buf[4];
	if (tmp & wSignBitMask)
		tmp |= wSignPaddingBits;
	offset_data[2] = tmp;

	memset(obj->cali_sw, 0x00, sizeof(obj->cali_sw));

	return err;
}

/*****************************************
 *** MC3XXX_ReadCalibration
 *****************************************/
static int MC3XXX_ReadCalibration(struct i2c_client *client, int dat[MC3XXX_AXES_NUM])
{
	struct mc3xxx_i2c_data *obj = i2c_get_clientdata(client);
	int err = 0;

	err = MC3XXX_ReadOffset(client, obj->offset);
	if (err) {
		GSE_ERR("read offset fail, %d\n", err);
		return err;
	}

	dat[MC3XXX_AXIS_X] = obj->offset[MC3XXX_AXIS_X];
	dat[MC3XXX_AXIS_Y] = obj->offset[MC3XXX_AXIS_Y];
	dat[MC3XXX_AXIS_Z] = obj->offset[MC3XXX_AXIS_Z];

	GSE_LOG("MC3XXX_ReadCalibration %d %d %d\n",
		dat[obj->cvt.map[MC3XXX_AXIS_X]] , dat[obj->cvt.map[MC3XXX_AXIS_Y]], dat[obj->cvt.map[MC3XXX_AXIS_Z]]);

	return 0;
}

/*****************************************
 *** MC3XXX_WriteCalibration
 *****************************************/
static int MC3XXX_WriteCalibration(struct i2c_client *client, int dat[MC3XXX_AXES_NUM])
{
	struct mc3xxx_i2c_data *obj = i2c_get_clientdata(client);
	int err = 0;
	u8 buf[9] = { 0};
	s16 tmp = 0, x_gain = 0, y_gain = 0, z_gain = 0;
	s32 x_off = 0, y_off = 0, z_off = 0;
	int cali[MC3XXX_AXES_NUM] = {0};
	int _nTemp = 0;

	u8  bMsbFilter	   = 0x3F;
	s16 wSignBitMask	 = 0x2000;
	s16 wSignPaddingBits = 0xC000;
	s32 dwRangePosLimit  = 0x1FFF;
	s32 dwRangeNegLimit  = -0x2000;

	GSE_LOG("UPDATE dat: (%+3d %+3d %+3d)\n", dat[MC3XXX_AXIS_X], dat[MC3XXX_AXIS_Y], dat[MC3XXX_AXIS_Z]);

	cali[MC3XXX_AXIS_X] = obj->cvt.sign[MC3XXX_AXIS_X]*(dat[obj->cvt.map[MC3XXX_AXIS_X]]);
	cali[MC3XXX_AXIS_Y] = obj->cvt.sign[MC3XXX_AXIS_Y]*(dat[obj->cvt.map[MC3XXX_AXIS_Y]]);
	cali[MC3XXX_AXIS_Z] = obj->cvt.sign[MC3XXX_AXIS_Z]*(dat[obj->cvt.map[MC3XXX_AXIS_Z]]);

	if (MC3XXX_PCODE_3250 == s_bPCODE) {
		_nTemp = cali[MC3XXX_AXIS_X];
		cali[MC3XXX_AXIS_X] = -cali[MC3XXX_AXIS_Y];
		cali[MC3XXX_AXIS_Y] = _nTemp;
		GSE_LOG("[%s] 3250 write remap\n", __func__);
	} else {
		if (s_bMPOL & 0x01)
			cali[MC3XXX_AXIS_X] = -cali[MC3XXX_AXIS_X];
		if (s_bMPOL & 0x02)
			cali[MC3XXX_AXIS_Y] = -cali[MC3XXX_AXIS_Y];
		GSE_LOG("[%s] 35X0 remap [s_bMPOL: %d]\n", __func__, s_bMPOL);
	}

	GSE_LOG("UPDATE dat: (%+3d %+3d %+3d)\n", cali[MC3XXX_AXIS_X], cali[MC3XXX_AXIS_Y], cali[MC3XXX_AXIS_Z]);

	/* read registers 0x21~0x29 */
	err = MC3XXX_i2c_read_block(client, 0x21, buf, 3);
	if (err) {
		GSE_ERR("error: %d\n", err);
		return err;
	}
	err = MC3XXX_i2c_read_block(client, 0x24, &buf[3], 3);
	if (err) {
		GSE_ERR("error: %d\n", err);
		return err;
	}
	err = MC3XXX_i2c_read_block(client, 0x27, &buf[6], 3);
	if (err) {
		GSE_ERR("error: %d\n", err);
		return err;
	}

	if (IS_MCFM12() || IS_MCFM3X()) {
		bMsbFilter	   = 0x7F;
		wSignBitMask	 = 0x4000;
		wSignPaddingBits = 0x8000;
		dwRangePosLimit  = 0x3FFF;
		dwRangeNegLimit  = -0x4000;
	}

	/* get x,y,z offset */
	tmp = ((buf[1] & bMsbFilter) << 8) + buf[0];
	if (tmp & wSignBitMask)
		tmp |= wSignPaddingBits;
	x_off = tmp;

	tmp = ((buf[3] & bMsbFilter) << 8) + buf[2];
	if (tmp & wSignBitMask)
		tmp |= wSignPaddingBits;
	y_off = tmp;

	tmp = ((buf[5] & bMsbFilter) << 8) + buf[4];
	if (tmp & wSignBitMask)
		tmp |= wSignPaddingBits;
	z_off = tmp;

	/* get x,y,z gain */
	x_gain = ((buf[1] >> 7) << 8) + buf[6];
	y_gain = ((buf[3] >> 7) << 8) + buf[7];
	z_gain = ((buf[5] >> 7) << 8) + buf[8];

	/* prepare new offset */
	x_off = x_off + 16 * cali[MC3XXX_AXIS_X] * 256 * 128 / 3 / gsensor_gain.x / (40 + x_gain);
	y_off = y_off + 16 * cali[MC3XXX_AXIS_Y] * 256 * 128 / 3 / gsensor_gain.y / (40 + y_gain);
	z_off = z_off + 16 * cali[MC3XXX_AXIS_Z] * 256 * 128 / 3 / gsensor_gain.z / (40 + z_gain);

	/* add for over range */
	if (x_off > dwRangePosLimit)
		x_off = dwRangePosLimit;
	else if (x_off < dwRangeNegLimit)
		x_off = dwRangeNegLimit;


	if (y_off > dwRangePosLimit)
		y_off = dwRangePosLimit;
	else if (y_off < dwRangeNegLimit)
		y_off = dwRangeNegLimit;

	if (z_off > dwRangePosLimit)
		z_off = dwRangePosLimit;
	else if (z_off < dwRangeNegLimit)
		z_off = dwRangeNegLimit;

	/* storege the cerrunt offset data with DOT format */
	offset_data[0] = x_off;
	offset_data[1] = y_off;
	offset_data[2] = z_off;

	/* storege the cerrunt Gain data with GOT format */
	gain_data[0] = 256*8*128/3/(40+x_gain);
	gain_data[1] = 256*8*128/3/(40+y_gain);
	gain_data[2] = 256*8*128/3/(40+z_gain);

	buf[0] = 0x43;
	MC3XXX_i2c_write_block(client, 0x07, buf, 1);

	buf[0] = x_off & 0xff;
	buf[1] = ((x_off >> 8) & bMsbFilter) | (x_gain & 0x0100 ? 0x80 : 0);
	buf[2] = y_off & 0xff;
	buf[3] = ((y_off >> 8) & bMsbFilter) | (y_gain & 0x0100 ? 0x80 : 0);
	buf[4] = z_off & 0xff;
	buf[5] = ((z_off >> 8) & bMsbFilter) | (z_gain & 0x0100 ? 0x80 : 0);

	MC3XXX_i2c_write_block(client, 0x21, buf, 6);

	buf[0] = 0x41;
	MC3XXX_i2c_write_block(client, 0x07, buf, 1);

	mdelay(50);

	return err;
}

/*****************************************
 *** MC3XXX_SetPowerMode
 *****************************************/
static int MC3XXX_SetPowerMode(struct i2c_client *client, bool enable)
{
	u8 databuf[2] = {0};
	int res = 0;
	u8 addr = MC3XXX_REG_MODE_FEATURE;
	struct mc3xxx_i2c_data *obj = i2c_get_clientdata(client);

	if (enable == mc3xxx_sensor_power)
		GSE_LOG("Sensor power status should not be set again!!!\n");

	if (MC3XXX_i2c_read_block(client, addr, databuf, 1)) {
		GSE_ERR("read power ctl register err!\n");
		return MC3XXX_RETCODE_ERROR_I2C;
	}

	/* GSE_LOG("set power read MC3XXX_REG_MODE_FEATURE =%x\n", databuf[0]); */

	if (enable) {
		databuf[0] = 0x41;
		res = MC3XXX_i2c_write_block(client, MC3XXX_REG_MODE_FEATURE, databuf, 1);
	#ifdef _MC3XXX_SUPPORT_DOT_CALIBRATION_
		mcube_load_cali(client);
	#endif
	} else {
		databuf[0] = 0x43;
		res = MC3XXX_i2c_write_block(client, MC3XXX_REG_MODE_FEATURE, databuf, 1);
	}

	if (res < 0) {
		GSE_LOG("fwq set power mode failed!\n");
		return MC3XXX_RETCODE_ERROR_I2C;
	} else if (atomic_read(&obj->trace) & MCUBE_TRC_INFO)
		GSE_LOG("fwq set power mode ok %d!\n", databuf[1]);

	mc3xxx_sensor_power = enable;

	return MC3XXX_RETCODE_SUCCESS;
}

/*****************************************
 *** MC3XXX_SetResolution
 *****************************************/
static void MC3XXX_SetResolution(void)
{
	/* GSE_LOG("[%s]\n", __func__); */

	switch (s_bPCODE) {
	case MC3XXX_PCODE_3230:
	case MC3XXX_PCODE_3430:
	case MC3XXX_PCODE_3430N:
	case MC3XXX_PCODE_3530:
	case MC3XXX_PCODE_3236:
		s_bResolution = MC3XXX_RESOLUTION_LOW;
		break;

	case MC3XXX_PCODE_3210:
	case MC3XXX_PCODE_3250:
	case MC3XXX_PCODE_3410:
	case MC3XXX_PCODE_3410N:
	case MC3XXX_PCODE_3510:
	case MC3XXX_PCODE_3216:
		s_bResolution = MC3XXX_RESOLUTION_HIGH;
		break;

	case MC3XXX_PCODE_RESERVE_10:
		GSE_ERR("RESERVED ONLINE!\n");
	 /* TODO: should have a default configuration... */
		break;

	case MC3XXX_PCODE_RESERVE_1:
	case MC3XXX_PCODE_RESERVE_3:
	case MC3XXX_PCODE_RESERVE_4:
	case MC3XXX_PCODE_RESERVE_5:
	case MC3XXX_PCODE_RESERVE_6:
	case MC3XXX_PCODE_RESERVE_8:
	case MC3XXX_PCODE_RESERVE_9:
		GSE_ERR("RESERVED ONLINE!\n");
		s_bResolution = MC3XXX_RESOLUTION_LOW;
		break;

	case MC3XXX_PCODE_RESERVE_2:
	case MC3XXX_PCODE_RESERVE_7:
		GSE_ERR("RESERVED ONLINE!\n");
		s_bResolution = MC3XXX_RESOLUTION_HIGH;
		break;

	default:
		GSE_ERR("ERR: no resolution assigned!\n");
	}

	/* GSE_LOG("[%s] s_bResolution: %d\n", __func__, s_bResolution); */
}

/*****************************************
 *** MC3XXX_SetSampleRate
 *****************************************/
static void MC3XXX_SetSampleRate(struct i2c_client *pt_i2c_client)
{
	unsigned char	_baDataBuf[2] = { 0 };

	/* GSE_LOG("[%s]\n", __func__); */

	_baDataBuf[0] = MC3XXX_REG_SAMPLE_RATE;
	_baDataBuf[1] = 0x00;

	if (IS_MCFM12() || IS_MCFM3X()) {
		unsigned char	_baData2Buf[2] = { 0 };

		_baData2Buf[0] = 0x2A;
		MC3XXX_i2c_read_block(pt_i2c_client, 0x2A, _baData2Buf, 1);

		/* GSE_LOG("[%s] REG(0x2A) = 0x%02X\n", __func__, _baData2Buf[0]); */

		_baData2Buf[0] = (_baData2Buf[0] & 0xC0);

		switch (_baData2Buf[0]) {
		case 0x00:
			_baDataBuf[0] = 0x00;
			break;
		case 0x40:
			_baDataBuf[0] = 0x08;
			break;
		case 0x80:
			_baDataBuf[0] = 0x09;
			break;
		case 0xC0:
			_baDataBuf[0] = 0x0A;
			break;
		default:
			GSE_ERR("[%s] no chance to get here... check code!\n", __func__);
			break;
		}
	} else
		_baDataBuf[0] = 0x00;

	MC3XXX_i2c_write_block(pt_i2c_client, MC3XXX_REG_SAMPLE_RATE, _baDataBuf, 1);
}

/*****************************************
 *** MC3XXX_ConfigRegRange
 *****************************************/
static void MC3XXX_ConfigRegRange(struct i2c_client *pt_i2c_client)
{
	unsigned char	_baDataBuf[2] = { 0 };
	int res = 0;

	/* _baDataBuf[0] = 0x3F; */
	/* Modify low pass filter bandwidth to 512hz, for solving sensor data don't change issue */
	_baDataBuf[0] = 0x0F;

	if (MC3XXX_RESOLUTION_LOW == s_bResolution)
		_baDataBuf[0] = 0x32;

	if (IS_MCFM12() || IS_MCFM3X()) {
		if (MC3XXX_RESOLUTION_LOW == s_bResolution)
			_baDataBuf[0] = 0x02;
		else
			_baDataBuf[0] = 0x25;
	}
	res = MC3XXX_i2c_write_block(pt_i2c_client, MC3XXX_REG_RANGE_CONTROL, _baDataBuf, 1);
	if (res < 0)
		GSE_ERR("MC3XXX_ConfigRegRange fail\n");

	/* GSE_LOG("[%s] set 0x%X\n", __func__, _baDataBuf[1]); */
}

/*****************************************
 *** MC3XXX_SetGain
 *****************************************/
static void MC3XXX_SetGain(void)
{
	gsensor_gain.x = gsensor_gain.y = gsensor_gain.z = 1024;

	if (MC3XXX_RESOLUTION_LOW == s_bResolution) {
		gsensor_gain.x = gsensor_gain.y = gsensor_gain.z = 86;

		if (IS_MCFM12() || IS_MCFM3X())
			gsensor_gain.x = gsensor_gain.y = gsensor_gain.z = 64;

	}

	/* GSE_LOG("[%s] gain: %d / %d / %d\n", __func__, gsensor_gain.x, gsensor_gain.y, gsensor_gain.z); */
}

/*****************************************
 *** MC3XXX_Init
 *****************************************/
static int MC3XXX_Init(struct i2c_client *client, int reset_cali)
{
	unsigned char	_baDataBuf[2] = { 0 };

	/* GSE_LOG("[%s]\n", __func__); */

	#ifdef _MC3XXX_SUPPORT_POWER_SAVING_SHUTDOWN_POWER_
	if (MC3XXX_RETCODE_SUCCESS != _mc3xxx_i2c_auto_probe(client))
		return MC3XXX_RETCODE_ERROR_I2C;

	/* GSE_LOG("[%s] confirmed i2c addr: 0x%X\n", __FUNCTION__, client->addr); */
	#endif

	_baDataBuf[0] = 0x43;
	MC3XXX_i2c_write_block(client, MC3XXX_REG_MODE_FEATURE, _baDataBuf, 1);

	MC3XXX_SetResolution();
	MC3XXX_SetSampleRate(client);
	MC3XXX_ConfigRegRange(client);
	MC3XXX_SetGain();

	_baDataBuf[0] = 0x00;
	MC3XXX_i2c_write_block(client, MC3XXX_REG_TAP_DETECTION_ENABLE, _baDataBuf, 1);

	_baDataBuf[0] = 0x00;
	MC3XXX_i2c_write_block(client, MC3XXX_REG_INTERRUPT_ENABLE, _baDataBuf, 1);

	_baDataBuf[0] = 0;
	MC3XXX_i2c_read_block(client, 0x2A, _baDataBuf, 1);
	s_bMPOL = (_baDataBuf[0] & 0x03);

	#ifdef _MC3XXX_SUPPORT_DOT_CALIBRATION_
	MC3XXX_rbm(client, 0);
	#endif

	#ifdef _MC3XXX_SUPPORT_LPF_
	{
	struct mc3xxx_i2c_data   *_pt_i2c_data = i2c_get_clientdata(client);

	memset(&_pt_i2c_data->fir, 0x00, sizeof(_pt_i2c_data->fir));
	}
	#endif

	#ifdef _MC3XXX_SUPPORT_LRF_
	memset(&s_taLRF_CB, 0, sizeof(s_taLRF_CB));
	#endif

	#ifdef _MC3XXX_SUPPORT_PERIODIC_DOC_
	init_waitqueue_head(&wq_mc3xxx_open_status);
	#endif

	/* GSE_LOG("[%s] init ok.\n", __func__); */

	return MC3XXX_RETCODE_SUCCESS;
}

/*****************************************
 *** MC3XXX_ReadChipInfo
 *****************************************/
static int MC3XXX_ReadChipInfo(struct i2c_client *client, char *buf, int bufsize)
{
	if ((NULL == buf) || (bufsize <= 30))
		return -1;

	if (NULL == client) {
		*buf = 0;
		return -2;
	}

	sprintf(buf, "MC3XXX Chip");
	return 0;
}

/*****************************************
 *** MC3XXX_ReadSensorData
 *****************************************/
static int MC3XXX_ReadSensorData(struct i2c_client *pt_i2c_client, char *pbBuf, int nBufSize)
{
	int					   _naAccelData[MC3XXX_AXES_NUM] = { 0 };
	struct mc3xxx_i2c_data   *_pt_i2c_obj = ((struct mc3xxx_i2c_data *) i2c_get_clientdata(pt_i2c_client));

	if ((NULL == pt_i2c_client) || (NULL == pbBuf)) {
		GSE_ERR("ERR: Null Pointer\n");
		return MC3XXX_RETCODE_ERROR_NULL_POINTER;
	}

	if (false == mc3xxx_sensor_power) {
		if (MC3XXX_RETCODE_SUCCESS != MC3XXX_SetPowerMode(pt_i2c_client, true))
			GSE_ERR("ERR: fail to set power mode!\n");
	}

	#ifdef _MC3XXX_SUPPORT_DOT_CALIBRATION_
	mcube_load_cali(pt_i2c_client);

	if ((s_nIsRBM_Enabled) && (1 == LPF_FirstRun)) {
		int	_nLoopIndex = 0;

		LPF_FirstRun = 0;

		for (_nLoopIndex = 0; _nLoopIndex < (LPF_SamplingRate + LPF_CutoffFrequency); _nLoopIndex++)
			MC3XXX_ReadData(pt_i2c_client, _pt_i2c_obj->data);
	}
	#endif

	if (MC3XXX_RETCODE_SUCCESS != MC3XXX_ReadData(pt_i2c_client, _pt_i2c_obj->data)) {
		GSE_ERR("ERR: fail to read data!\n");

		return MC3XXX_RETCODE_ERROR_I2C;
	}

	/* output format: mg */
	if (atomic_read(&_pt_i2c_obj->trace) & MCUBE_TRC_INFO)
		GSE_LOG("[%s] raw data: %d, %d, %d\n", __func__, _pt_i2c_obj->data[MC3XXX_AXIS_X],
			_pt_i2c_obj->data[MC3XXX_AXIS_Y], _pt_i2c_obj->data[MC3XXX_AXIS_Z]);

	_naAccelData[(_pt_i2c_obj->cvt.map[MC3XXX_AXIS_X])] = (_pt_i2c_obj->cvt.sign[MC3XXX_AXIS_X]
		* _pt_i2c_obj->data[MC3XXX_AXIS_X]);
	_naAccelData[(_pt_i2c_obj->cvt.map[MC3XXX_AXIS_Y])] = (_pt_i2c_obj->cvt.sign[MC3XXX_AXIS_Y]
		* _pt_i2c_obj->data[MC3XXX_AXIS_Y]);
	_naAccelData[(_pt_i2c_obj->cvt.map[MC3XXX_AXIS_Z])] = (_pt_i2c_obj->cvt.sign[MC3XXX_AXIS_Z]
		* _pt_i2c_obj->data[MC3XXX_AXIS_Z]);

	if (atomic_read(&_pt_i2c_obj->trace) & MCUBE_TRC_INFO)
		GSE_LOG("[%s] map data: %d, %d, %d!\n", __func__, _naAccelData[MC3XXX_AXIS_X],
			_naAccelData[MC3XXX_AXIS_Y], _naAccelData[MC3XXX_AXIS_Z]);

	_naAccelData[MC3XXX_AXIS_X] = (_naAccelData[MC3XXX_AXIS_X] * GRAVITY_EARTH_1000 / gsensor_gain.x);
	_naAccelData[MC3XXX_AXIS_Y] = (_naAccelData[MC3XXX_AXIS_Y] * GRAVITY_EARTH_1000 / gsensor_gain.y);
	_naAccelData[MC3XXX_AXIS_Z] = (_naAccelData[MC3XXX_AXIS_Z] * GRAVITY_EARTH_1000 / gsensor_gain.z);

	if (atomic_read(&_pt_i2c_obj->trace) & MCUBE_TRC_INFO)
		GSE_LOG("[%s] accel data: %d, %d, %d!\n", __func__, _naAccelData[MC3XXX_AXIS_X],
			_naAccelData[MC3XXX_AXIS_Y], _naAccelData[MC3XXX_AXIS_Z]);

	sprintf(pbBuf, "%04x %04x %04x",
		_naAccelData[MC3XXX_AXIS_X], _naAccelData[MC3XXX_AXIS_Y], _naAccelData[MC3XXX_AXIS_Z]);

	return MC3XXX_RETCODE_SUCCESS;
}

/*****************************************
 *** _MC3XXX_ReadAverageData
 *****************************************/
#ifdef _MC3XXX_SUPPORT_APPLY_AVERAGE_AGORITHM_
static int _MC3XXX_ReadAverageData(struct i2c_client *client, char *buf)
{
	struct mc3xxx_i2c_data *obj = (struct mc3xxx_i2c_data *)i2c_get_clientdata(client);
	int acc[MC3XXX_AXES_NUM] = {0};
	s16 sensor_data[3] = {0};
	s16 sensor_data_max[3] = {0};
	s16 sensor_data_mini[3] = {0};
	s32 sensor_data_sum[3] = {0};

	int i = 0, j = 0;

	MC3XXX_ReadData(client, sensor_data);
	GSE_LOG("MC3XXX_ReadRawData MC3XXX_ReadData: %d, %d, %d!\n",
		sensor_data[MC3XXX_AXIS_X], sensor_data[MC3XXX_AXIS_Y], sensor_data[MC3XXX_AXIS_Z]);
	sensor_data_max[MC3XXX_AXIS_X] = sensor_data[MC3XXX_AXIS_X];
	sensor_data_max[MC3XXX_AXIS_Y] = sensor_data[MC3XXX_AXIS_Y];
	sensor_data_max[MC3XXX_AXIS_Z] = sensor_data[MC3XXX_AXIS_Z];

	sensor_data_mini[MC3XXX_AXIS_X] = sensor_data[MC3XXX_AXIS_X];
	sensor_data_mini[MC3XXX_AXIS_Y] = sensor_data[MC3XXX_AXIS_Y];
	sensor_data_mini[MC3XXX_AXIS_Z] = sensor_data[MC3XXX_AXIS_Z];

	sensor_data_sum[MC3XXX_AXIS_X] += sensor_data[MC3XXX_AXIS_X];
	sensor_data_sum[MC3XXX_AXIS_Y] += sensor_data[MC3XXX_AXIS_Y];
	sensor_data_sum[MC3XXX_AXIS_Z] += sensor_data[MC3XXX_AXIS_Z];

	for (i = 0; i < 11 ; i++) {
		MC3XXX_ReadData(client, sensor_data);
		GSE_LOG("MC3XXX_ReadRawData MC3XXX_ReadData: %d, %d, %d!\n",
			sensor_data[MC3XXX_AXIS_X], sensor_data[MC3XXX_AXIS_Y], sensor_data[MC3XXX_AXIS_Z]);

		sensor_data_sum[MC3XXX_AXIS_X] += sensor_data[MC3XXX_AXIS_X];
		sensor_data_sum[MC3XXX_AXIS_Y] += sensor_data[MC3XXX_AXIS_Y];
		sensor_data_sum[MC3XXX_AXIS_Z] += sensor_data[MC3XXX_AXIS_Z];
		for (j = 0; j < 3 ; j++) {
			if (sensor_data[j] > sensor_data_max[j])
				sensor_data_max[j] = sensor_data[j];

			if (sensor_data[j] < sensor_data_mini[j])
				sensor_data_mini[j] = sensor_data[j];
		}
	}
	GSE_LOG("MC3XXX_ReadRawData sensor_data_max: %d, %d, %d!\n",
		sensor_data_max[MC3XXX_AXIS_X], sensor_data_max[MC3XXX_AXIS_Y], sensor_data_max[MC3XXX_AXIS_Z]);
	GSE_LOG("MC3XXX_ReadRawData sensor_data_mini: %d, %d, %d!\n",
		sensor_data_mini[MC3XXX_AXIS_X], sensor_data_mini[MC3XXX_AXIS_Y], sensor_data_mini[MC3XXX_AXIS_Z]);
	sensor_data[MC3XXX_AXIS_X] = (s16)((sensor_data_sum[MC3XXX_AXIS_X]-sensor_data_max[MC3XXX_AXIS_X]
		-sensor_data_mini[MC3XXX_AXIS_X])/10);
	sensor_data[MC3XXX_AXIS_Y] = (s16)((sensor_data_sum[MC3XXX_AXIS_Y]-sensor_data_max[MC3XXX_AXIS_Y]
		-sensor_data_mini[MC3XXX_AXIS_Y])/10);
	sensor_data[MC3XXX_AXIS_Z] = (s16)((sensor_data_sum[MC3XXX_AXIS_Z]-sensor_data_max[MC3XXX_AXIS_Z]
		-sensor_data_mini[MC3XXX_AXIS_Z])/10);
	GSE_LOG("MC3XXX_ReadRawData sensor_data: %d, %d, %d!\n",
		sensor_data[MC3XXX_AXIS_X], sensor_data[MC3XXX_AXIS_Y], sensor_data[MC3XXX_AXIS_Z]);

	acc[(obj->cvt.map[MC3XXX_AXIS_X])] = obj->cvt.sign[MC3XXX_AXIS_X] * sensor_data[MC3XXX_AXIS_X];
	acc[(obj->cvt.map[MC3XXX_AXIS_Y])] = obj->cvt.sign[MC3XXX_AXIS_Y] * sensor_data[MC3XXX_AXIS_Y];
	acc[(obj->cvt.map[MC3XXX_AXIS_Z])] = obj->cvt.sign[MC3XXX_AXIS_Z] * sensor_data[MC3XXX_AXIS_Z];

	GSE_LOG("MC3XXX_ReadRawData mapdata: %d, %d, %d!\n",
		acc[MC3XXX_AXIS_X], acc[MC3XXX_AXIS_Y], acc[MC3XXX_AXIS_Z]);

	acc[MC3XXX_AXIS_X] = (acc[MC3XXX_AXIS_X]*GRAVITY_EARTH_1000/gsensor_gain.x);
	acc[MC3XXX_AXIS_Y] = (acc[MC3XXX_AXIS_Y]*GRAVITY_EARTH_1000/gsensor_gain.y);
	acc[MC3XXX_AXIS_Z] = (acc[MC3XXX_AXIS_Z]*GRAVITY_EARTH_1000/gsensor_gain.z);

	GSE_LOG("MC3XXX_ReadRawData mapdata1: %d, %d, %d!\n",
		acc[MC3XXX_AXIS_X], acc[MC3XXX_AXIS_Y], acc[MC3XXX_AXIS_Z]);

	sprintf(buf, "%04x %04x %04x", acc[MC3XXX_AXIS_X], acc[MC3XXX_AXIS_Y], acc[MC3XXX_AXIS_Z]);

	return 0;
}
#endif	/* END OF #ifdef _MC3XXX_SUPPORT_APPLY_AVERAGE_AGORITHM_ */

/*****************************************
 *** MC3XXX_ReadRawData
 *****************************************/
static int MC3XXX_ReadRawData(struct i2c_client *client, char *buf)
{
	int res = 0;

	if (!buf || !client)
		return -EINVAL;

	if (mc3xxx_sensor_power == false) {
		res = MC3XXX_SetPowerMode(client, true);
		if (res)
			GSE_ERR("Power on mc3xxx error %d!\n", res);
	}

	#ifdef _MC3XXX_SUPPORT_APPLY_AVERAGE_AGORITHM_
	return _MC3XXX_ReadAverageData(client, buf);
	#else
	{
	s16 sensor_data[3] = {0};

	res = MC3XXX_ReadData(client, sensor_data);
	if (res) {
		GSE_ERR("I2C error: ret value=%d", res);
		return -EIO;
	}
	sprintf(buf, "%04x %04x %04x", sensor_data[MC3XXX_AXIS_X],
	sensor_data[MC3XXX_AXIS_Y], sensor_data[MC3XXX_AXIS_Z]);
	}
	#endif

	return 0;
}

#ifdef _MC3XXX_SUPPORT_DOT_CALIBRATION_
/*****************************************
 *** MC3XXX_ReadRBMData
 *****************************************/
static int MC3XXX_ReadRBMData(struct i2c_client *client, char *buf)
{
	int res = 0;
	int data[3] = { 0 };

	if (!buf || !client)
		return -EINVAL;

	if (mc3xxx_sensor_power == false) {
		res = MC3XXX_SetPowerMode(client, true);
		if (res)
			GSE_ERR("Power on mc3xxx error %d!\n", res);
	}

	res = MC3XXX_ReadData_RBM(client, data);
	if (res) {
		GSE_ERR("I2C error: ret value=%d", res);
		return -EIO;
	}

	sprintf(buf, "%04x %04x %04x", data[MC3XXX_AXIS_X],
	data[MC3XXX_AXIS_Y], data[MC3XXX_AXIS_Z]);

	return res;
}
#endif /* _MC3XXX_SUPPORT_DOT_CALIBRATION_ */

/*****************************************
 *** MC3XXX_JudgeTestResult
 *****************************************/
static int MC3XXX_JudgeTestResult(struct i2c_client *client)
{
	int	res				  = 0;
	int	self_result		  = 0;
	s16	acc[MC3XXX_AXES_NUM] = { 0 };

	res = MC3XXX_ReadData(client, acc);
	if (res) {
		GSE_ERR("I2C error: ret value=%d", res);
		return -EIO;
	}

	acc[MC3XXX_AXIS_X] = acc[MC3XXX_AXIS_X] * 1000 / gsensor_gain.x;
	acc[MC3XXX_AXIS_Y] = acc[MC3XXX_AXIS_Y] * 1000 / gsensor_gain.y;
	acc[MC3XXX_AXIS_Z] = acc[MC3XXX_AXIS_Z] * 1000 / gsensor_gain.z;

	self_result = ((acc[MC3XXX_AXIS_X] * acc[MC3XXX_AXIS_X])
		   + (acc[MC3XXX_AXIS_Y] * acc[MC3XXX_AXIS_Y])
		   + (acc[MC3XXX_AXIS_Z] * acc[MC3XXX_AXIS_Z]));

	if ((self_result > 475923) && (self_result < 2185360)) {
		GSE_ERR("MC3XXX_JudgeTestResult successful\n");
		return MC3XXX_RETCODE_SUCCESS;
	}

	GSE_ERR("MC3XXX_JudgeTestResult failt\n");
	return -EINVAL;
}

/*****************************************
 *** MC3XXX_SelfCheck
 *****************************************/
static void MC3XXX_SelfCheck(struct i2c_client *client, u8 *pUserBuf)
{
	u8	_bRData1 = 0;
	u8	_bRData2 = 0;
	u8	_bRData3 = 0;
	u8	_baDataBuf[2] = { 0 };

	MC3XXX_i2c_read_block(client, 0x20, &_bRData1, 1);
	MC3XXX_i2c_read_block(client, 0x3B, &_bRData2, 1);

	_baDataBuf[0] = 0x43;
	MC3XXX_i2c_write_block(client, 0x07, _baDataBuf, 1);

	mdelay(10);

	for ( ; ; ) {
		_baDataBuf[0] = 0x6D;
		MC3XXX_i2c_write_block(client, 0x1B, _baDataBuf, 1);

		_baDataBuf[0] = 0x43;
		MC3XXX_i2c_write_block(client, 0x1B, _baDataBuf, 1);

		_bRData3 = 0x00;
		MC3XXX_i2c_read_block(client, 0x04, &_bRData3, 1);

		if (_bRData3 & 0x40)
			break;
	}

	_baDataBuf[0] = (_bRData2 & 0xFE);
	MC3XXX_i2c_write_block(client, 0x3B, _baDataBuf, 1);

	_baDataBuf[0] = 0x03;
	MC3XXX_i2c_write_block(client, 0x20, _baDataBuf, 1);

	_baDataBuf[0] = 0x40;
	MC3XXX_i2c_write_block(client, 0x14, _baDataBuf, 1);

	mdelay(10);

	_baDataBuf[0] = pUserBuf[0];
	MC3XXX_i2c_write_block(client, 0x00, _baDataBuf, 1);

	_baDataBuf[0] = 0x41;
	MC3XXX_i2c_write_block(client, 0x07, _baDataBuf, 1);

	mdelay(10);

	_baDataBuf[0] = 0x43;
	MC3XXX_i2c_write_block(client, 0x07, _baDataBuf, 1);

	mdelay(10);

	MC3XXX_Read_Reg_Map(client, pUserBuf);

	mdelay(10);

	_baDataBuf[0] = 0x00;
	MC3XXX_i2c_write_block(client, 0x14, _baDataBuf, 1);

	_baDataBuf[0] = _bRData1;
	MC3XXX_i2c_write_block(client, 0x20, _baDataBuf, 1);

	_baDataBuf[0] = _bRData2;
	MC3XXX_i2c_write_block(client, 0x3B, _baDataBuf, 1);

	mdelay(10);

	for ( ; ; ) {
		_baDataBuf[0] = 0x6D;
		MC3XXX_i2c_write_block(client, 0x1B, _baDataBuf, 1);

		_baDataBuf[0] = 0x43;
		MC3XXX_i2c_write_block(client, 0x1B, _baDataBuf, 1);

		_bRData3 = 0xFF;
		MC3XXX_i2c_read_block(client, 0x04, &_bRData3, 1);

		if (!(_bRData3 & 0x40))
			break;
	}

	mdelay(10);
}

/*****************************************
 *** MC3XXX_GetOpenStatus
 *****************************************/
#ifdef _MC3XXX_SUPPORT_PERIODIC_DOC_
static int	MC3XXX_GetOpenStatus(void)
{
	/* GSE_LOG("[%s] %d\n", __FUNCTION__, atomic_read(&s_t_mc3xxx_open_status)); */

	wait_event_interruptible(wq_mc3xxx_open_status, (atomic_read(&s_t_mc3xxx_open_status) != 0));

	/* GSE_LOG("[%s] pass wait_event_interruptible: %d\n", __FUNCTION__, atomic_read(&s_t_mc3xxx_open_status)); */

	return atomic_read(&s_t_mc3xxx_open_status);
}
#endif

/*****************************************
 *** show_chipinfo_value
 *****************************************/
static ssize_t show_chipinfo_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = mc3xxx_i2c_client;
	char strbuf[MC3XXX_BUF_SIZE] = {0};

	GSE_LOG("fwq show_chipinfo_value\n");
	if (NULL == client) {
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}

	MC3XXX_ReadChipInfo(client, strbuf, MC3XXX_BUF_SIZE);
	return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);
}

/*****************************************
 *** show_sensordata_value
 *****************************************/
static ssize_t show_sensordata_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = mc3xxx_i2c_client;
	char strbuf[MC3XXX_BUF_SIZE] = { 0 };

	if (NULL == client) {
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}

	mc3xxx_mutex_lock();
	MC3XXX_ReadSensorData(client, strbuf, MC3XXX_BUF_SIZE);
	mc3xxx_mutex_unlock();
	return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);
}

/*****************************************
 *** show_cali_value
 *****************************************/
static ssize_t show_cali_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = mc3xxx_i2c_client;
	struct mc3xxx_i2c_data *obj;
	int err = 0;
	int len = 0;
	int mul = 0;
	int tmp[MC3XXX_AXES_NUM] = { 0 };

	GSE_LOG("fwq show_cali_value\n");

	if (NULL == client) {
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}

	obj = (struct mc3xxx_i2c_data *) i2c_get_clientdata(client);

	err = MC3XXX_ReadOffset(client, obj->offset);
	if (err)
		return -EINVAL;

	err = MC3XXX_ReadCalibration(client, tmp);
	if (err)
		return -EINVAL;

	mul = gsensor_gain.x / mc3xxx_offset_resolution.sensitivity;
	len += snprintf(buf+len, PAGE_SIZE-len, "[HW] (%+3d, %+3d, %+3d) : (0x%02X, 0x%02X, 0x%02X)\n",
			obj->offset[MC3XXX_AXIS_X], obj->offset[MC3XXX_AXIS_Y], obj->offset[MC3XXX_AXIS_Z],
			obj->offset[MC3XXX_AXIS_X], obj->offset[MC3XXX_AXIS_Y], obj->offset[MC3XXX_AXIS_Z]);
	len += snprintf(buf+len, PAGE_SIZE-len, "[SW][%d] (%+3d, %+3d, %+3d)\n", 1,
			obj->cali_sw[MC3XXX_AXIS_X], obj->cali_sw[MC3XXX_AXIS_Y], obj->cali_sw[MC3XXX_AXIS_Z]);
	len += snprintf(buf+len, PAGE_SIZE-len, "[ALL]  (%+3d, %+3d, %+3d) : (%+3d, %+3d, %+3d)\n",
			obj->offset[MC3XXX_AXIS_X]*mul + obj->cali_sw[MC3XXX_AXIS_X],
			obj->offset[MC3XXX_AXIS_Y]*mul + obj->cali_sw[MC3XXX_AXIS_Y],
			obj->offset[MC3XXX_AXIS_Z]*mul + obj->cali_sw[MC3XXX_AXIS_Z],
			tmp[MC3XXX_AXIS_X], tmp[MC3XXX_AXIS_Y], tmp[MC3XXX_AXIS_Z]);

		return len;
}

/*****************************************
 *** store_cali_value
 *****************************************/
static ssize_t store_cali_value(struct device_driver *ddri, const char *buf, size_t count)
{
	struct i2c_client *client = mc3xxx_i2c_client;
	int err = 0;
	int x = 0;
	int y = 0;
	int z = 0;
	int dat[MC3XXX_AXES_NUM] = {0};

	if (!strncmp(buf, "rst", 3)) {
		mc3xxx_mutex_lock();
		err = MC3XXX_ResetCalibration(client);
		mc3xxx_mutex_unlock();

		if (err)
			GSE_ERR("reset offset err = %d\n", err);
	} else if (3 == sscanf(buf, "0x%02X 0x%02X 0x%02X", &x, &y, &z)) {
		dat[MC3XXX_AXIS_X] = x;
		dat[MC3XXX_AXIS_Y] = y;
		dat[MC3XXX_AXIS_Z] = z;

		mc3xxx_mutex_lock();
		err = MC3XXX_WriteCalibration(client, dat);
		mc3xxx_mutex_unlock();
		if (err)
			GSE_ERR("write calibration err = %d\n", err);
	} else
		GSE_ERR("invalid format\n");

	return count;
}

/*****************************************
 *** show_selftest_value
 *****************************************/
static ssize_t show_selftest_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = mc3xxx_i2c_client;

	if (NULL == client) {
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}

	return snprintf(buf, 8, "%s\n", selftestRes);
}

/*****************************************
 *** store_selftest_value
 *****************************************/
static ssize_t store_selftest_value(struct device_driver *ddri, const char *buf, size_t count)
{   /*write anything to this register will trigger the process*/
	struct i2c_client *client = mc3xxx_i2c_client;
	int num = 0;
	int ret = 0;

	ret = kstrtoint(buf, 10, &num);
	if (ret != 0) {
		GSE_ERR("parse number fail\n");
		return count;
	} else if (0 == num) {
		GSE_ERR("invalid data count\n");
		return count;
	}

	GSE_LOG("NORMAL:\n");
	mc3xxx_mutex_lock();
	MC3XXX_SetPowerMode(client, true);
	mc3xxx_mutex_unlock();
	GSE_LOG("SELFTEST:\n");

	if (!MC3XXX_JudgeTestResult(client)) {
		GSE_LOG("SELFTEST : PASS\n");
		strcpy(selftestRes, "y");
	} else {
		GSE_LOG("SELFTEST : FAIL\n");
		strcpy(selftestRes, "n");
	}

	return count;
}

/*****************************************
 *** show_firlen_value
 *****************************************/
static ssize_t show_firlen_value(struct device_driver *ddri, char *buf)
{
	#ifdef _MC3XXX_SUPPORT_LPF_
	struct i2c_client *client = mc3xxx_i2c_client;
	struct mc3xxx_i2c_data *obj = i2c_get_clientdata(client);

	GSE_LOG("fwq show_firlen_value\n");
	if (atomic_read(&obj->firlen)) {
		int idx = 0, len = atomic_read(&obj->firlen);

		GSE_LOG("len = %2d, idx = %2d\n", obj->fir.num, obj->fir.idx);

		for (idx = 0; idx < len; idx++)
			GSE_LOG("[%5d %5d %5d]\n", obj->fir.raw[idx][MC3XXX_AXIS_X],
				obj->fir.raw[idx][MC3XXX_AXIS_Y], obj->fir.raw[idx][MC3XXX_AXIS_Z]);

		GSE_LOG("sum = [%5d %5d %5d]\n", obj->fir.sum[MC3XXX_AXIS_X],
			obj->fir.sum[MC3XXX_AXIS_Y], obj->fir.sum[MC3XXX_AXIS_Z]);
		GSE_LOG("avg = [%5d %5d %5d]\n", obj->fir.sum[MC3XXX_AXIS_X]/len,
			obj->fir.sum[MC3XXX_AXIS_Y]/len, obj->fir.sum[MC3XXX_AXIS_Z]/len);
	}
	return snprintf(buf, PAGE_SIZE, "%d\n", atomic_read(&obj->firlen));
	#else
	GSE_LOG("fwq show_firlen_value\n");
	return snprintf(buf, PAGE_SIZE, "not support\n");
	#endif
}

/*****************************************
 *** store_firlen_value
 *****************************************/
static ssize_t store_firlen_value(struct device_driver *ddri, const char *buf, size_t count)
{
	#ifdef _MC3XXX_SUPPORT_LPF_
	struct i2c_client *client = mc3xxx_i2c_client;
	struct mc3xxx_i2c_data *obj = i2c_get_clientdata(client);
	int firlen = 0;
	int ret = 0;

	GSE_LOG("fwq store_firlen_value\n");

	ret = kstrtoint(buf, 10, &firlen);
	if (ret != 0)
		GSE_ERR("invallid format\n");
	else if (firlen > C_MAX_FIR_LENGTH)
		GSE_ERR("exceeds maximum filter length\n");
	else {
		atomic_set(&obj->firlen, firlen);
		if (0 == firlen)
			atomic_set(&obj->fir_en, 0);
		else {
			memset(&obj->fir, 0x00, sizeof(obj->fir));
			atomic_set(&obj->fir_en, 1);
		}
	}
	#endif
	return count;
}

/*****************************************
 *** show_trace_value
 *****************************************/
static ssize_t show_trace_value(struct device_driver *ddri, char *buf)
{
	ssize_t res = 0;
	struct mc3xxx_i2c_data *obj = mc3xxx_obj_i2c_data;

	GSE_LOG("fwq show_trace_value\n");

	if (obj == NULL) {
		GSE_ERR("i2c_data obj is null!!\n");
		return 0;
	}

	res = snprintf(buf, PAGE_SIZE, "0x%04X\n", atomic_read(&obj->trace));
	return res;
}

/*****************************************
 *** store_trace_value
 *****************************************/
static ssize_t store_trace_value(struct device_driver *ddri, const char *buf, size_t count)
{
	struct mc3xxx_i2c_data *obj = mc3xxx_obj_i2c_data;
	int trace = 0;

	GSE_LOG("fwq store_trace_value\n");

	if (obj == NULL) {
		GSE_ERR("i2c_data obj is null!!\n");
		return 0;
	}

	if (1 == sscanf(buf, "0x%x", &trace))
		atomic_set(&obj->trace, trace);
	else
		GSE_ERR("invalid content: '%s', length = %zu\n", buf, count);

	return count;
}

/*****************************************
 *** show_status_value
 *****************************************/
static ssize_t show_status_value(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	struct mc3xxx_i2c_data *obj = mc3xxx_obj_i2c_data;

	GSE_LOG("fwq show_status_value\n");

	if (obj == NULL) {
		GSE_ERR("i2c_data obj is null!!\n");
		return 0;
	}

	if (obj->hw)
		len += snprintf(buf+len, PAGE_SIZE-len, "CUST: %d %d (%d %d)\n",
			obj->hw->i2c_num, obj->hw->direction, obj->hw->power_id, obj->hw->power_vol);
	else
		len += snprintf(buf+len, PAGE_SIZE-len, "CUST: NULL\n");

	return len;
}

/*****************************************
 *** show_power_status
 *****************************************/
static ssize_t show_power_status(struct device_driver *ddri, char *buf)
{
	ssize_t res = 0;
	u8 uData = 0;
	struct mc3xxx_i2c_data *obj = mc3xxx_obj_i2c_data;

	if (obj == NULL) {
		GSE_ERR("i2c_data obj is null!!\n");
		return 0;
	}
	MC3XXX_i2c_read_block(obj->client, MC3XXX_REG_MODE_FEATURE, &uData, 1);

	res = snprintf(buf, PAGE_SIZE, "0x%04X\n", uData);
	return res;
}

/*****************************************
 *** show_version_value
 *****************************************/
static ssize_t show_version_value(struct device_driver *ddri, char *buf)
{
	if (1 == VIRTUAL_Z)
		return snprintf(buf, PAGE_SIZE, "%s\n", MC3XXX_DEV_DRIVER_VERSION_VIRTUAL_Z);
	else
		return snprintf(buf, PAGE_SIZE, "%s\n", MC3XXX_DEV_DRIVER_VERSION);
}

/*****************************************
 *** show_chip_id
 *****************************************/
static ssize_t show_chip_id(struct device_driver *ddri, char *buf)
{
	struct mc3xxx_i2c_data   *_pt_i2c_data = mc3xxx_obj_i2c_data;

	return MC3XXX_Read_Chip_ID(_pt_i2c_data->client, buf);
}

/*****************************************
 *** show_virtual_z
 *****************************************/
static ssize_t show_virtual_z(struct device_driver *ddri, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", VIRTUAL_Z == 1 ? "Virtual Z Support" : "No Virtual Z Support");
}

/*****************************************
 *** show_regiter_map
 *****************************************/
static ssize_t show_regiter_map(struct device_driver *ddri, char *buf)
{
	u8		 _bIndex	   = 0;
	u8		 _baRegMap[64] = { 0 };
	ssize_t	_tLength	  = 0;

	struct i2c_client *client = mc3xxx_i2c_client;

	if ((0xA5 == buf[0]) && (0x7B == buf[1]) && (0x40 == buf[2])) {
		mc3xxx_mutex_lock();
		MC3XXX_Read_Reg_Map(client, buf);
		mc3xxx_mutex_unlock();

		buf[0x21] = s_baOTP_OffsetData[0];
		buf[0x22] = s_baOTP_OffsetData[1];
		buf[0x23] = s_baOTP_OffsetData[2];
		buf[0x24] = s_baOTP_OffsetData[3];
		buf[0x25] = s_baOTP_OffsetData[4];
		buf[0x26] = s_baOTP_OffsetData[5];

		_tLength = 64;
	} else {
		mc3xxx_mutex_lock();
		MC3XXX_Read_Reg_Map(client, _baRegMap);
		mc3xxx_mutex_unlock();

	for (_bIndex = 0; _bIndex < 64; _bIndex++)
		_tLength += snprintf((buf + _tLength), (PAGE_SIZE - _tLength), "Reg[0x%02X]: 0x%02X\n",
			_bIndex, _baRegMap[_bIndex]);
	}

	return _tLength;
}

/*****************************************
 *** store_regiter_map
 *****************************************/
static ssize_t store_regiter_map(struct device_driver *ddri, const char *buf, size_t count)
{
	return count;
}

/*****************************************
 *** show_chip_orientation
 *****************************************/
static ssize_t show_chip_orientation(struct device_driver *ptDevDrv, char *pbBuf)
{
	ssize_t		  _tLength = 0;
	struct acc_hw   *_ptAccelHw = get_cust_acc();

	GSE_LOG("[%s] default direction: %d\n", __func__, _ptAccelHw->direction);

	_tLength = snprintf(pbBuf, PAGE_SIZE, "default direction = %d\n", _ptAccelHw->direction);

	return _tLength;
}

/*****************************************
 *** store_chip_orientation
 *****************************************/
static ssize_t store_chip_orientation(struct device_driver *ptDevDrv, const char *pbBuf, size_t tCount)
{
	int _nDirection = 0;
	int ret = 0;
	struct mc3xxx_i2c_data   *_pt_i2c_obj = mc3xxx_obj_i2c_data;

	if (NULL == _pt_i2c_obj)
		return 0;

	ret = kstrtoint(pbBuf, 10, &_nDirection);
	if (ret != 0) {
		if (hwmsen_get_convert(_nDirection, &_pt_i2c_obj->cvt))
			GSE_ERR("ERR: fail to set direction\n");
	}

	GSE_LOG("[%s] set direction: %d\n", __func__, _nDirection);

	return tCount;
}

/*****************************************
 *** show_accuracy_status
 *****************************************/
static ssize_t show_accuracy_status(struct device_driver *ddri, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", s_bAccuracyStatus);
}

/*****************************************
 *** store_accuracy_status
 *****************************************/
static ssize_t store_accuracy_status(struct device_driver *ddri, const char *buf, size_t count)
{
	int	_nAccuracyStatus = 0;
	int ret = 0;

	ret = kstrtoint(buf, 10, &_nAccuracyStatus);
	if (ret != 0) {
		GSE_ERR("incorrect argument\n");
		return count;
	}

	if (SENSOR_STATUS_ACCURACY_HIGH < _nAccuracyStatus) {
		GSE_ERR("illegal accuracy status\n");
		return count;
	}

	s_bAccuracyStatus = ((int8_t) _nAccuracyStatus);

	return count;
}

/*****************************************
 *** show_selfcheck_value
 *****************************************/
static ssize_t show_selfcheck_value(struct device_driver *ptDevDriver, char *pbBuf)
{
	struct i2c_client   *_pt_i2c_client = mc3xxx_i2c_client;

	/* GSE_LOG("[%s] 0x%02X\n", __FUNCTION__, pbBuf[0]); */

	mc3xxx_mutex_lock();
	MC3XXX_SelfCheck(_pt_i2c_client, pbBuf);
	MC3XXX_Init(_pt_i2c_client, 0);
	mc3xxx_mutex_unlock();

	return 64;
}

/*****************************************
 *** store_selfcheck_value
 *****************************************/
static ssize_t store_selfcheck_value(struct device_driver *ddri, const char *buf, size_t count)
{
	/* reserved */
	/* GSE_LOG("[%s] buf[0]: 0x%02X\n", __FUNCTION__, buf[0]); */

	return count;
}

/*****************************************
 *** show_chip_validate_value
 *****************************************/
static ssize_t show_chip_validate_value(struct device_driver *ptDevDriver, char *pbBuf)
{
	unsigned char	_bChipValidation = 0;

	_bChipValidation = MC3XXX_ValidateSensorIC(&s_bPCODE, &s_bHWID);

	return snprintf(pbBuf, PAGE_SIZE, "%d\n", _bChipValidation);
}

/*****************************************
 *** show_pdoc_enable_value
 *****************************************/
static ssize_t show_pdoc_enable_value(struct device_driver *ptDevDriver, char *pbBuf)
{
	#ifdef _MC3XXX_SUPPORT_PERIODIC_DOC_
	return snprintf(pbBuf, PAGE_SIZE, "%d\n", s_bIsPDOC_Enabled);
	#else
	unsigned char	_bIsPDOC_Enabled = false;

	return snprintf(pbBuf, PAGE_SIZE, "%d\n", _bIsPDOC_Enabled);
	#endif
}

/*****************************************
 *** DRIVER ATTRIBUTE LIST TABLE
 *****************************************/
static DRIVER_ATTR(chipinfo   ,		   S_IRUGO, show_chipinfo_value,   NULL);
static DRIVER_ATTR(sensordata ,		   S_IRUGO, show_sensordata_value, NULL);
static DRIVER_ATTR(cali	   , S_IWUSR | S_IRUGO, show_cali_value,	   store_cali_value);
static DRIVER_ATTR(selftest   , S_IWUSR | S_IRUGO, show_selftest_value,   store_selftest_value);
static DRIVER_ATTR(firlen	 , S_IWUSR | S_IRUGO, show_firlen_value,	 store_firlen_value);
static DRIVER_ATTR(trace	  , S_IWUSR | S_IRUGO, show_trace_value,	  store_trace_value);
static DRIVER_ATTR(status	 ,		   S_IRUGO, show_status_value,	 NULL);
static DRIVER_ATTR(power	  ,		   S_IRUGO, show_power_status,	 NULL);
static DRIVER_ATTR(version	,		   S_IRUGO, show_version_value,	NULL);
static DRIVER_ATTR(chipid	 ,		   S_IRUGO, show_chip_id,		  NULL);
static DRIVER_ATTR(virtualz   ,		   S_IRUGO, show_virtual_z,		NULL);
static DRIVER_ATTR(regmap	 , S_IWUSR | S_IRUGO, show_regiter_map,	  store_regiter_map);
static DRIVER_ATTR(orientation, S_IWUSR | S_IRUGO, show_chip_orientation, store_chip_orientation);
static DRIVER_ATTR(accuracy   , S_IWUSR | S_IRUGO, show_accuracy_status , store_accuracy_status);
static DRIVER_ATTR(selfcheck  , S_IWUSR | S_IRUGO, show_selfcheck_value , store_selfcheck_value);
static DRIVER_ATTR(validate   ,		   S_IRUGO, show_chip_validate_value, NULL);
static DRIVER_ATTR(pdoc	   ,		   S_IRUGO, show_pdoc_enable_value  , NULL);

static struct driver_attribute   *mc3xxx_attr_list[] = {
							   &driver_attr_chipinfo,
							   &driver_attr_sensordata,
							   &driver_attr_cali,
							   &driver_attr_selftest,
							   &driver_attr_firlen,
							   &driver_attr_trace,
							   &driver_attr_status,
							   &driver_attr_power,
							   &driver_attr_version,
							   &driver_attr_chipid,
							   &driver_attr_virtualz,
							   &driver_attr_regmap,
							   &driver_attr_orientation,
							   &driver_attr_accuracy,
							   &driver_attr_selfcheck,
							   &driver_attr_validate,
							   &driver_attr_pdoc,
							   };

/*****************************************
 *** mc3xxx_create_attr
 *****************************************/
static int mc3xxx_create_attr(struct device_driver *driver)
{
	int idx, err = 0;
	int num = (int)(sizeof(mc3xxx_attr_list)/sizeof(mc3xxx_attr_list[0]));

	if (driver == NULL)
		return -EINVAL;

	for (idx = 0; idx < num; idx++) {
		err = driver_create_file(driver, mc3xxx_attr_list[idx]);
		if (err) {
			GSE_ERR("driver_create_file (%s) = %d\n", mc3xxx_attr_list[idx]->attr.name, err);
			break;
		}
	}
	return err;
}

/*****************************************
 *** mc3xxx_delete_attr
 *****************************************/
static int mc3xxx_delete_attr(struct device_driver *driver)
{
	int idx , err = 0;
	int num = (int)(sizeof(mc3xxx_attr_list)/sizeof(mc3xxx_attr_list[0]));

	if (driver == NULL)
		return -EINVAL;

	for (idx = 0; idx < num; idx++)
		driver_remove_file(driver, mc3xxx_attr_list[idx]);

	return err;
}


/*****************************************
 *** mc3xxx_open
 *****************************************/
static int mc3xxx_open(struct inode *inode, struct file *file)
{
	file->private_data = mc3xxx_i2c_client;

	if (file->private_data == NULL) {
		GSE_ERR("null pointer!!\n");
		return -EINVAL;
	}
	return nonseekable_open(inode, file);
}

/*****************************************
 *** mc3xxx_release
 *****************************************/
static int mc3xxx_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}

/*****************************************
 *** mc3xxx_ioctl
 *****************************************/
static long mc3xxx_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct i2c_client *client = (struct i2c_client *)file->private_data;
	struct mc3xxx_i2c_data *obj = (struct mc3xxx_i2c_data *)i2c_get_clientdata(client);
	char strbuf[MC3XXX_BUF_SIZE] = {0};
	void __user *data = NULL;
	struct SENSOR_DATA sensor_data = {0};
	long err = 0;
	int cali[3] = {0};

	#ifdef _MC3XXX_SUPPORT_DOT_CALIBRATION_
		int prod = -1;
		int tempZ = 0;
		unsigned char _bTempPCode = 0x00;
	#endif

	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));


	if (err) {
		GSE_ERR("access error: %08X, (%2d, %2d)\n", cmd, _IOC_DIR(cmd), _IOC_SIZE(cmd));
		return -EFAULT;
	}

	switch (cmd) {
	case GSENSOR_IOCTL_INIT:
		GSE_LOG("fwq GSENSOR_IOCTL_INIT\n");
		mc3xxx_mutex_lock();
		MC3XXX_Init(client, 0);
		mc3xxx_mutex_unlock();
		break;

	case GSENSOR_IOCTL_READ_CHIPINFO:
		GSE_LOG("fwq GSENSOR_IOCTL_READ_CHIPINFO\n");
		data = (void __user *) arg;
		if (data == NULL) {
			err = -EINVAL;
			break;
		}

		MC3XXX_ReadChipInfo(client, strbuf, MC3XXX_BUF_SIZE);
		if (copy_to_user(data, strbuf, strlen(strbuf)+1)) {
			err = -EFAULT;
			break;
		}
		break;

	case GSENSOR_IOCTL_READ_SENSORDATA:
		data = (void __user *) arg;
		if (data == NULL) {
			err = -EINVAL;
			break;
		}
		mc3xxx_mutex_lock();
		#ifdef _MC3XXX_SUPPORT_APPLY_AVERAGE_AGORITHM_
		MC3XXX_ReadRawData(client, strbuf);
		#else
		MC3XXX_ReadSensorData(client, strbuf, MC3XXX_BUF_SIZE);
		#endif
		mc3xxx_mutex_unlock();
		if (copy_to_user(data, strbuf, strlen(strbuf)+1)) {
			err = -EFAULT;
			break;
		}
		break;

	case GSENSOR_IOCTL_READ_GAIN:
		GSE_LOG("fwq GSENSOR_IOCTL_READ_GAIN\n");
		data = (void __user *) arg;
		if (data == NULL) {
			err = -EINVAL;
			break;
		}

		if (copy_to_user(data, &gsensor_gain, sizeof(struct GSENSOR_VECTOR3D))) {
			err = -EFAULT;
			break;
		}
		break;

	case GSENSOR_IOCTL_READ_OFFSET:
		GSE_LOG("fwq GSENSOR_IOCTL_READ_OFFSET\n");
		data = (void __user *) arg;
		if (data == NULL) {
			err = -EINVAL;
			break;
		}

		if (copy_to_user(data, &gsensor_offset, sizeof(struct GSENSOR_VECTOR3D))) {
			err = -EFAULT;
			break;
		}
		break;

	case GSENSOR_IOCTL_READ_RAW_DATA:
		GSE_LOG("fwq GSENSOR_IOCTL_READ_RAW_DATA\n");
		data = (void __user *) arg;
		if (data == NULL) {
			err = -EINVAL;
			break;
		}
		mc3xxx_mutex_lock();
		MC3XXX_ReadRawData(client, strbuf);
		mc3xxx_mutex_unlock();
		if (copy_to_user(data, strbuf, strlen(strbuf)+1)) {
			err = -EFAULT;
			break;
		}
		break;

	case GSENSOR_IOCTL_SET_CALI:
		GSE_LOG("fwq GSENSOR_IOCTL_SET_CALI!!\n");
		data = (void __user *)arg;
		if (data == NULL) {
			err = -EINVAL;
			break;
		}
		if (copy_from_user(&sensor_data, data, sizeof(sensor_data))) {
			err = -EFAULT;
			break;
		}
		if (atomic_read(&obj->suspend)) {
			GSE_ERR("Perform calibration in suspend state!!\n");
			err = -EINVAL;
		} else {
			obj->cali_sw[MC3XXX_AXIS_X] += sensor_data.x;
			obj->cali_sw[MC3XXX_AXIS_Y] += sensor_data.y;
			obj->cali_sw[MC3XXX_AXIS_Z] += sensor_data.z;

			cali[MC3XXX_AXIS_X] = sensor_data.x * gsensor_gain.x / GRAVITY_EARTH_1000;
			cali[MC3XXX_AXIS_Y] = sensor_data.y * gsensor_gain.y / GRAVITY_EARTH_1000;
			cali[MC3XXX_AXIS_Z] = sensor_data.z * gsensor_gain.z / GRAVITY_EARTH_1000;

			mc3xxx_mutex_lock();
			err = MC3XXX_WriteCalibration(client, cali);
			mc3xxx_mutex_unlock();
		}
		break;

	case GSENSOR_IOCTL_CLR_CALI:
		GSE_LOG("fwq GSENSOR_IOCTL_CLR_CALI!!\n");
		mc3xxx_mutex_lock();
		err = MC3XXX_ResetCalibration(client);
		mc3xxx_mutex_unlock();
		break;

	case GSENSOR_IOCTL_GET_CALI:
		GSE_LOG("fwq mc3xxx GSENSOR_IOCTL_GET_CALI\n");
		data = (void __user *)arg;
		if (data == NULL) {
			err = -EINVAL;
			break;
		}

		sensor_data.x = obj->cali_sw[MC3XXX_AXIS_X];
		sensor_data.y = obj->cali_sw[MC3XXX_AXIS_Y];
		sensor_data.z = obj->cali_sw[MC3XXX_AXIS_Z];
		if (copy_to_user(data, &sensor_data, sizeof(sensor_data))) {
			err = -EFAULT;
			break;
		}
		break;

	/* add in Sensors_io.h */
	#ifdef _MC3XXX_SUPPORT_DOT_CALIBRATION_

	case GSENSOR_MCUBE_IOCTL_SET_CALI:
		GSE_LOG("fwq GSENSOR_MCUBE_IOCTL_SET_CALI!!\n");
		data = (void __user *)arg;
		if (data == NULL) {
			err = -EINVAL;
			break;
		}
		if (copy_from_user(&sensor_data, data, sizeof(sensor_data))) {
			err = -EFAULT;
			break;
		}
		if (atomic_read(&obj->suspend)) {
			GSE_ERR("Perform calibration in suspend state!!\n");
			err = -EINVAL;
		} else {
			cali[MC3XXX_AXIS_X] = sensor_data.x * gsensor_gain.x / GRAVITY_EARTH_1000;
			cali[MC3XXX_AXIS_Y] = sensor_data.y * gsensor_gain.y / GRAVITY_EARTH_1000;
			cali[MC3XXX_AXIS_Z] = sensor_data.z * gsensor_gain.z / GRAVITY_EARTH_1000;

			mc3xxx_mutex_lock();
			err = MC3XXX_WriteCalibration(client, cali);
			mc3xxx_mutex_unlock();
		}
		break;

	case GSENSOR_IOCTL_SET_CALI_MODE:
		GSE_LOG("fwq mc3xxx GSENSOR_IOCTL_SET_CALI_MODE\n");
		break;

	case GSENSOR_MCUBE_IOCTL_READ_RBM_DATA:
		GSE_LOG("fwq GSENSOR_MCUBE_IOCTL_READ_RBM_DATA\n");
		data = (void __user *) arg;
		if (data == NULL) {
			err = -EINVAL;
			break;
		}
		mc3xxx_mutex_lock();
		MC3XXX_ReadRBMData(client, strbuf);
		mc3xxx_mutex_unlock();
		if (copy_to_user(data, strbuf, strlen(strbuf)+1)) {
			err = -EFAULT;
			break;
		}
		break;

	case GSENSOR_MCUBE_IOCTL_SET_RBM_MODE:
		GSE_LOG("fwq GSENSOR_MCUBE_IOCTL_SET_RBM_MODE\n");
		mc3xxx_mutex_lock();
		MC3XXX_rbm(client, 1);
		mc3xxx_mutex_unlock();
		break;

	case GSENSOR_MCUBE_IOCTL_CLEAR_RBM_MODE:
		GSE_LOG("fwq GSENSOR_MCUBE_IOCTL_CLEAR_RBM_MODE\n");
		mc3xxx_mutex_lock();
		MC3XXX_rbm(client, 0);
		mc3xxx_mutex_unlock();
		break;

	case GSENSOR_MCUBE_IOCTL_REGISTER_MAP:
		GSE_LOG("fwq GSENSOR_MCUBE_IOCTL_REGISTER_MAP\n");

		MC3XXX_Read_Reg_Map(client, NULL);

		break;

	case GSENSOR_MCUBE_IOCTL_READ_PRODUCT_ID:
		GSE_LOG("fwq GSENSOR_MCUBE_IOCTL_READ_PRODUCT_ID\n");
		data = (void __user *) arg;
		if (data == NULL) {
			err = -EINVAL;
			break;
		}

		prod = MC3XXX_ValidateSensorIC(&s_bPCODE, &s_bHWID);
		if (MC3XXX_RETCODE_SUCCESS != prod)
			GSE_LOG("Not mCube accelerometers!\n");

		if (copy_to_user(data, &prod, sizeof(prod))) {
			err = -EFAULT;
			break;
		}
		break;

	case GSENSOR_MCUBE_IOCTL_READ_FILEPATH:
		GSE_LOG("fwq GSENSOR_MCUBE_IOCTL_READ_FILEPATH\n");
		data = (void __user *) arg;
		if (data == NULL) {
			err = -EINVAL;
			break;
		}

		if (copy_to_user(data, file_path, (strlen(file_path)+1))) {
			err = -EFAULT;
			break;
		}
		break;

	case GSENSOR_MCUBE_IOCTL_VIRTUAL_Z:
		GSE_LOG("fwq GSENSOR_MCUBE_IOCTL_VIRTUAL_Z\n");
		data = (void __user *) arg;
		tempZ = VIRTUAL_Z;
		if (data == NULL) {
			err = -EINVAL;
			break;
		}
		if (copy_to_user(data, &tempZ, sizeof(tempZ))) {
			err = -EFAULT;
			break;
		}
		break;

	case GSENSOR_MCUBE_IOCTL_READ_PCODE:
		GSE_LOG("fwq GSENSOR_MCUBE_IOCTL_READ_PCODE\n");
		data = (void __user *) arg;
		if (data == NULL) {
			err = -EINVAL;
			break;
		}

		_bTempPCode = s_bPCODE;
		GSE_LOG("mCube PCode = %2x!\n", _bTempPCode);
		if (copy_to_user(data, &_bTempPCode, sizeof(_bTempPCode))) {
			err = -EFAULT;
			break;
		}
		break;

	#endif	/* END of _MC3XXX_SUPPORT_DOT_CALIBRATION_ */

	#ifdef _MC3XXX_SUPPORT_PERIODIC_DOC_

	case GSENSOR_MCUBE_IOCTL_GET_OFLAG:
		int			_nSensorsOpenStatus = 0;
		void __user   *_pArg = ((void __user *) arg);

		GSE_LOG("[%s] GSENSOR_MCUBE_IOCTL_GET_OFLAG\n", __func__);

		_nSensorsOpenStatus = MC3XXX_GetOpenStatus();

		if (copy_to_user(_pArg, &_nSensorsOpenStatus, sizeof(_nSensorsOpenStatus)))
			return (-EFAULT);
		break;

	#endif	/* END of _MC3XXX_SUPPORT_PERIODIC_DOC_ */

	default:
		GSE_ERR("unknown IOCTL: 0x%08x\n", cmd);
		err = -ENOIOCTLCMD;
		break;

	}

	return err;
}

#ifdef CONFIG_COMPAT
static long mc3xxx_compat_ioctl(struct file *file, unsigned int cmd,
	   unsigned long arg)
{
	long err = 0;

	void __user *arg32 = compat_ptr(arg);

	if (!file->f_op || !file->f_op->unlocked_ioctl)
		return -ENOTTY;

	switch (cmd) {
	case COMPAT_GSENSOR_IOCTL_READ_SENSORDATA:
		if (arg32 == NULL) {
			err = -EINVAL;
			break;
		}

		err = file->f_op->unlocked_ioctl(file, GSENSOR_IOCTL_READ_SENSORDATA, (unsigned long)arg32);
		if (err) {
			GSE_ERR("GSENSOR_IOCTL_READ_SENSORDATA unlocked_ioctl failed.");
			return err;
		}
		break;

	case COMPAT_GSENSOR_IOCTL_SET_CALI:
		if (arg32 == NULL) {
			err = -EINVAL;
			break;
		}

		err = file->f_op->unlocked_ioctl(file, GSENSOR_IOCTL_SET_CALI, (unsigned long)arg32);
		if (err) {
			GSE_ERR("GSENSOR_IOCTL_SET_CALI unlocked_ioctl failed.");
			return err;
		}
		break;

	case COMPAT_GSENSOR_IOCTL_GET_CALI:
		if (arg32 == NULL) {
			err = -EINVAL;
			break;
		}

		err = file->f_op->unlocked_ioctl(file, GSENSOR_IOCTL_GET_CALI, (unsigned long)arg32);
		if (err) {
			GSE_ERR("GSENSOR_IOCTL_GET_CALI unlocked_ioctl failed.");
			return err;
		}
	break;

	case COMPAT_GSENSOR_IOCTL_CLR_CALI:
		if (arg32 == NULL) {
			err = -EINVAL;
			break;
		}

		err = file->f_op->unlocked_ioctl(file, GSENSOR_IOCTL_CLR_CALI, (unsigned long)arg32);
		if (err) {
			GSE_ERR("GSENSOR_IOCTL_CLR_CALI unlocked_ioctl failed.");
			return err;
		}
		break;

	default:
		GSE_ERR("unknown IOCTL: 0x%08x\n", cmd);
		err = -ENOIOCTLCMD;

	}

	return err;
}
#endif
/*****************************************
 *** MC3XXX_reset
 *****************************************/
static void MC3XXX_reset(struct i2c_client *client)
{
	unsigned char	_baBuf[2] = { 0 };

	_baBuf[0] = 0x43;

	MC3XXX_i2c_write_block(client, MC3XXX_REG_MODE_FEATURE, _baBuf, 0x01);

	MC3XXX_i2c_read_block(client, 0x04, _baBuf, 0x01);

	if (0x00 == (_baBuf[0] & 0x40)) {
		_baBuf[0] = 0x6D;
		MC3XXX_i2c_write_block(client, 0x1B, _baBuf, 0x01);

		_baBuf[0] = 0x43;
		MC3XXX_i2c_write_block(client, 0x1B, _baBuf, 0x01);
	}

	_baBuf[0] = 0x43;
	MC3XXX_i2c_write_block(client, 0x07, _baBuf, 1);

	_baBuf[0] = 0x80;
	MC3XXX_i2c_write_block(client, 0x1C, _baBuf, 1);

	_baBuf[0] = 0x80;
	MC3XXX_i2c_write_block(client, 0x17, _baBuf, 1);

	mdelay(5);

	_baBuf[0] = 0x00;
	MC3XXX_i2c_write_block(client, 0x1C, _baBuf, 1);

	_baBuf[0] = 0x00;
	MC3XXX_i2c_write_block(client, 0x17, _baBuf, 1);

	mdelay(5);

	MC3XXX_i2c_read_block(client, 0x21, offset_buf, 6);

	MC3XXX_i2c_read_block(client, 0x04, _baBuf, 0x01);

	if (_baBuf[0] & 0x40) {
		_baBuf[0] = 0x6D;
		MC3XXX_i2c_write_block(client, 0x1B, _baBuf, 0x01);

		_baBuf[0] = 0x43;
		MC3XXX_i2c_write_block(client, 0x1B, _baBuf, 0x01);
	}

	_baBuf[0] = 0x41;

	MC3XXX_i2c_write_block(client, MC3XXX_REG_MODE_FEATURE, _baBuf, 0x01);
}

/*****************************************
 *** STRUCT:: mc3xxx_fops
 *****************************************/
static const struct file_operations	mc3xxx_fops = {
						   .owner		= THIS_MODULE,
						   .open		= mc3xxx_open,
						   .release		= mc3xxx_release,
						   .unlocked_ioctl	= mc3xxx_ioctl,
						#ifdef CONFIG_COMPAT
						   .compat_ioctl	= mc3xxx_compat_ioctl,
						#endif
						   };

/*****************************************
 *** STRUCT:: mc3xxx_device
 *****************************************/
static struct miscdevice mc3xxx_device = {
						.minor = MISC_DYNAMIC_MINOR,
						.name  = "gsensor",
						.fops  = &mc3xxx_fops,
						};

/*****************************************
 *** mc3xxx_suspend
 *****************************************/
static int mc3xxx_suspend(struct i2c_client *client, pm_message_t msg)
{
	struct mc3xxx_i2c_data *obj = i2c_get_clientdata(client);
	int err = 0;

	GSE_LOG("mc3xxx_suspend\n");

	if (msg.event == PM_EVENT_SUSPEND) {
		if (obj == NULL) {
			GSE_ERR("null pointer!!\n");
			return -EINVAL;
		}

		atomic_set(&obj->suspend, 1);
		mc3xxx_mutex_lock();
		err = MC3XXX_SetPowerMode(client, false);
		mc3xxx_mutex_unlock();
		if (err) {
			GSE_ERR("write power control fail!!\n");
			return err;
		}
		MC3XXX_power(obj->hw, 0);
	}
	return err;
}

/*****************************************
 *** mc3xxx_resume
 *****************************************/
static int mc3xxx_resume(struct i2c_client *client)
{
	struct mc3xxx_i2c_data *obj = i2c_get_clientdata(client);
	int err;

	GSE_LOG("mc3xxx_resume\n");
	if (obj == NULL) {
		GSE_ERR("null pointer!!\n");
		return -EINVAL;
	}

	MC3XXX_power(obj->hw, 1);
	mc3xxx_mutex_lock();
	err = MC3XXX_Init(client, 0);
	if (err) {
		mc3xxx_mutex_unlock();
		GSE_ERR("initialize client fail!!\n");
		return err;
	}

	err = MC3XXX_SetPowerMode(client, true);
	mc3xxx_mutex_unlock();
	if (err) {
		GSE_ERR("write power control fail!!\n");
		return err;
	}
	atomic_set(&obj->suspend, 0);
	return 0;
}


/*****************************************
 *** _mc3xxx_i2c_auto_probe
 *****************************************/
static int _mc3xxx_i2c_auto_probe(struct i2c_client *client)
{
	#define _MC3XXX_I2C_PROBE_ADDR_COUNT_	\
		(sizeof(mc3xxx_i2c_auto_probe_addr) / sizeof(mc3xxx_i2c_auto_probe_addr[0]))
	unsigned char	_baData1Buf[2] = { 0 };
	unsigned char	_baData2Buf[2] = { 0 };
	int _nCount = 0;
	int _naCheckCount[_MC3XXX_I2C_PROBE_ADDR_COUNT_] = { 0 };

	memset(_naCheckCount, 0, sizeof(_naCheckCount));
_I2C_AUTO_PROBE_RECHECK_:
	s_bPCODE  = 0x00;
	s_bPCODER = 0x00;
	s_bHWID   = 0x00;

	for (_nCount = 0; _nCount < _MC3XXX_I2C_PROBE_ADDR_COUNT_; _nCount++) {
		client->addr = mc3xxx_i2c_auto_probe_addr[_nCount];
		_baData1Buf[0] = 0;
		if (0 > MC3XXX_i2c_read_block(client, 0x3B, _baData1Buf, 1))
			continue;
		_naCheckCount[_nCount]++;

		if (0x00 == _baData1Buf[0]) {
			if (1 == _naCheckCount[_nCount]) {
				MC3XXX_reset(client);
				mdelay(3);
				goto _I2C_AUTO_PROBE_RECHECK_;
			} else
				continue;
		}

		_baData2Buf[0] = 0;
		MC3XXX_i2c_read_block(client, 0x18, _baData2Buf, 1);
		s_bPCODER = _baData1Buf[0];
		if (MC3XXX_RETCODE_SUCCESS == MC3XXX_ValidateSensorIC(&_baData1Buf[0], &_baData2Buf[0])) {
			s_bPCODE = _baData1Buf[0];
			s_bHWID  = _baData2Buf[0];
			MC3XXX_SaveDefaultOffset(client);

			return MC3XXX_RETCODE_SUCCESS;
		}
	}

	return MC3XXX_RETCODE_ERROR_I2C;
	#undef _MC3XXX_I2C_PROBE_ADDR_COUNT_
}

/* if use  this typ of enable , Gsensor should report inputEvent(x, y, z ,stats, div) to HAL */
static int mc3xxx_open_report_data(int open)
{
	/* should queuq work to report event if  is_report_input_direct=true */
	return 0;
}

/* if use  this typ of enable , Gsensor only enabled but not report inputEvent to HAL */
static int mc3xxx_enable_nodata(int en)
{
	int res = 0;
	int retry = 0;
	bool power = false;

	if (1 == en)
		power = true;
	if (0 == en)
		power = false;

	for (retry = 0; retry < 3; retry++) {
		res = MC3XXX_SetPowerMode(mc3xxx_obj_i2c_data->client, power);
		if (res == 0) {
			GSE_LOG("MC3XXX_SetPowerMode done\n");
			break;
		}
		GSE_LOG("MC3XXX_SetPowerMode fail\n");
	}

	if (res != 0) {
		GSE_LOG("MC3XXX_SetPowerMode fail!\n");
		return -1;
	}
	GSE_LOG("mc3xxx_enable_nodata OK!\n");
	return 0;
}

static int mc3xxx_set_delay(u64 ns)
{
	int value = 0;

	value = (int)ns/1000/1000;
	GSE_LOG("mc3xxx_set_delay (%d), chip only use 1024HZ\n", value);
	return 0;
}

static int mc3xxx_get_data(int *x , int *y, int *z, int *status)
{
	int err = 0;
	char buff[MC3XXX_BUF_SIZE];
	int ret;
	u8 databuf[2] = {0};
	MC3XXX_ReadSensorData(mc3xxx_obj_i2c_data->client, buff, MC3XXX_BUF_SIZE);
	ret = sscanf(buff, "%x %x %x", x, y, z);
	*status = SENSOR_STATUS_ACCURACY_MEDIUM;

/*Judge the same data*/
	if((g_predata[0] == *x) && (g_predata[1] == *y) && (g_predata[2] == *z)) {
		if(MC3XXX_SAME_NUM > g_samedataCounter) {
			g_samedataCounter++;
		}
		else {
			GSE_LOG("samedata[%d]:%x,%x,%x\n", g_samedataCounter, *x, *y, *z);
			g_samedataCounter = 0;
#if 0
			if(MC3XXX_i2c_read_block(mc3xxx_i2c_client, MC3XXX_REG_MODE_FEATURE, databuf,1))
   			 {
      			  GSE_ERR("read power ctl register err!\n");
     			  return MC3XXX_RETCODE_ERROR_I2C;
  			  }
    			GSE_ERR("0,setpower read MC3XXX_REG_MODE_FEATURE =%x,i2c_addr:%x\n", databuf[0],mc3xxx_i2c_client->addr);
#endif
			/*MC3XXX_reset(mc3xxx_i2c_client);*/
			err = MC3XXX_Init(mc3xxx_i2c_client, 0);/*init acc hw*/

#if 0
			if(MC3XXX_i2c_read_block(mc3xxx_i2c_client, MC3XXX_REG_MODE_FEATURE, databuf,1)) {
      				GSE_ERR("read power ctl register err!\n");
     				return MC3XXX_RETCODE_ERROR_I2C;
  			  }
    			GSE_ERR("1,setpower read MC3XXX_REG_MODE_FEATURE =%x\n", databuf[0]);
#endif

			databuf[0] = 0x41;/*set power on*/
			err = MC3XXX_i2c_write_block(mc3xxx_i2c_client, MC3XXX_REG_MODE_FEATURE, databuf, 1);/*set power mode*/
			if (err){
				GSE_ERR("write power control fail!!\n");
			}
#if 0
			if(MC3XXX_i2c_read_block(mc3xxx_i2c_client, MC3XXX_REG_MODE_FEATURE, databuf,1)){
				GSE_ERR("read power ctl register err!\n");
     				return MC3XXX_RETCODE_ERROR_I2C;
  			  }
			 GSE_ERR("2,setpower read MC3XXX_REG_MODE_FEATURE =%x\n", databuf[0]);
#endif
		}
	}
	else {
		g_predata[0] = *x;
		g_predata[1] = *y;
		g_predata[2] = *z;
		g_samedataCounter = 0;
	}
	return 0;
}


/*****************************************
 *** mc3xxx_i2c_probe
 *****************************************/
static int mc3xxx_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct i2c_client *new_client;
	struct mc3xxx_i2c_data *obj;
	struct acc_control_path ctl = {0};
	struct acc_data_path data = {0};
	int err = 0;

	GSE_LOG("mc3xxx_i2c_probe\n");

	if (MC3XXX_RETCODE_SUCCESS != _mc3xxx_i2c_auto_probe(client))
	{
		err = -1;
		goto exit;
	}

	obj = kzalloc(sizeof(*obj), GFP_KERNEL);
	if (!obj) {
		err = -ENOMEM;
		goto exit;
	}

	obj->hw = hw;
	err = hwmsen_get_convert(obj->hw->direction, &obj->cvt);
	if (err) {
		GSE_ERR("invalid direction: %d\n", obj->hw->direction);
		goto exit_kfree;
	}

	mc3xxx_obj_i2c_data = obj;
	obj->client = client;
	new_client = obj->client;
	i2c_set_clientdata(new_client, obj);
	atomic_set(&obj->trace, 0);
	atomic_set(&obj->suspend, 0);

	#ifdef _MC3XXX_SUPPORT_LPF_
	if (obj->hw->firlen > C_MAX_FIR_LENGTH)
		atomic_set(&obj->firlen, C_MAX_FIR_LENGTH);
	else
		atomic_set(&obj->firlen, obj->hw->firlen);

	if (atomic_read(&obj->firlen) > 0)
		atomic_set(&obj->fir_en, 1);
	#endif
	mc3xxx_i2c_client = new_client;
	MC3XXX_reset(new_client);

	if (MC3XXX_RETCODE_SUCCESS != _mc3xxx_i2c_auto_probe(client))
	{
		err = -1;
		goto exit_init_failed;
	}

	MC3XXX_i2c_read_block(client, 0x21, offset_buf, 6);
	err = MC3XXX_Init(new_client, 1);
	if (err)
		goto exit_init_failed;
	mc3xxx_mutex_init();
	err = misc_register(&mc3xxx_device);
	if (err) {
		GSE_ERR("mc3xxx_device register failed\n");
		goto exit_misc_device_register_failed;
	}
	ctl.is_use_common_factory = false;
	err = mc3xxx_create_attr(&(mc3xxx_init_info.platform_diver_addr->driver));
	if (err) {
		GSE_ERR("create attribute err = %d\n", err);
		goto exit_create_attr_failed;
	}

	ctl.open_report_data = mc3xxx_open_report_data;
	ctl.enable_nodata = mc3xxx_enable_nodata;
	ctl.set_delay  = mc3xxx_set_delay;
	ctl.is_report_input_direct = false;
	ctl.is_support_batch = obj->hw->is_batch_supported;
	err = acc_register_control_path(&ctl);
	if (err) {
		GSE_ERR("register acc control path err\n");
		goto exit_kfree;
	}
	data.get_data = mc3xxx_get_data;
	data.vender_div = 1000;
	err = acc_register_data_path(&data);
	if (err) {
		GSE_ERR("register acc data path err= %d\n", err);
		goto exit_kfree;
	}
	GSE_LOG("%s: OK\n", __func__);
	s_nInitFlag = MC3XXX_INIT_SUCC;
	return 0;

exit_create_attr_failed:
	misc_deregister(&mc3xxx_device);
exit_misc_device_register_failed:
exit_init_failed:
exit_kfree:
	kfree(obj);
	obj = NULL;
exit:
	GSE_ERR("%s: err = %d\n", __func__, err);
	s_nInitFlag = MC3XXX_INIT_FAIL;

	return err;
}

/*****************************************
 *** mc3xxx_i2c_remove
 *****************************************/
static int mc3xxx_i2c_remove(struct i2c_client *client)
{
	int err = 0;

	err = mc3xxx_delete_attr(&(mc3xxx_init_info.platform_diver_addr->driver));
	if (err)
		GSE_ERR("mc3xxx_delete_attr fail: %d\n", err);

	err = misc_deregister(&mc3xxx_device);
	if (err)
		GSE_ERR("misc_deregister fail: %d\n", err);

	#ifdef _MC3XXX_SUPPORT_VPROXIMITY_SENSOR_
	misc_deregister(&mcube_psensor_device);
	#endif
	mc3xxx_i2c_client = NULL;
	i2c_unregister_device(client);
	kfree(i2c_get_clientdata(client));

	return 0;
}


/*****************************************
 *** mc3xxx_remove
 *****************************************/
static int mc3xxx_remove(void)
{
	MC3XXX_power(hw, 0);
	i2c_del_driver(&mc3xxx_i2c_driver);

	return 0;
}

/*****************************************
 *** mc3xxx_local_init
 *****************************************/
static int  mc3xxx_local_init(void)
{
	GSE_LOG("mc3xxx_local_init\n");
	MC3XXX_power(hw, 1);

	if (i2c_add_driver(&mc3xxx_i2c_driver)) {
		GSE_ERR("add driver error\n");
		return -1;
	}

	if (MC3XXX_INIT_FAIL == s_nInitFlag)
		return -1;
	return 0;
}

/*****************************************
 *** mc3xxx_init
 *****************************************/
static int __init mc3xxx_init(void)
{
	const char *name = "mediatek,mc3410";

	hw = get_accel_dts_func(name, hw);
	if (!hw)
		GSE_ERR("get dts info fail\n");

	acc_driver_add(&mc3xxx_init_info);
	return 0;
}

/*****************************************
 *** mc3xxx_exit
 *****************************************/
static void __exit mc3xxx_exit(void)
{
	GSE_LOG("mc3xxx_exit\n");
}

/*----------------------------------------------------------------------------*/
module_init(mc3xxx_init);
module_exit(mc3xxx_exit);
/*----------------------------------------------------------------------------*/
MODULE_DESCRIPTION("mc3XXX G-Sensor Driver");
MODULE_AUTHOR("Mediatek");
MODULE_LICENSE("GPL");
MODULE_VERSION(MC3XXX_DEV_DRIVER_VERSION);
