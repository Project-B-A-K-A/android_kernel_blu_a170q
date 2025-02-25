#ifndef _CUST_BATTERY_METER_H
#define _CUST_BATTERY_METER_H

/* ============================================================*/
/* define*/
/* ============================================================*/
/*#define SOC_BY_AUXADC*/
/*#define SOC_BY_HW_FG*/
/*#define HW_FG_FORCE_USE_SW_OCV*/
#define SOC_BY_SW_FG

/*#define CONFIG_DIS_CHECK_BATTERY*/
/*#define FIXED_TBAT_25*/

/* ADC resistor  */
#define R_BAT_SENSE 4
#define R_I_SENSE 4
#define R_CHARGER_1 330
#define R_CHARGER_2 39

#define TEMPERATURE_T0	110
#define TEMPERATURE_T1	0
#define TEMPERATURE_T2	25
#define TEMPERATURE_T3	50
#define TEMPERATURE_T	255 /* This should be fixed, never change the value*/

#define FG_METER_RESISTANCE	0

/* Qmax for battery  */
#define Q_MAX_POS_50		2208
#define Q_MAX_POS_25		2258
#define Q_MAX_POS_0			1926
#define Q_MAX_NEG_10		1944

#define Q_MAX_POS_50_H_CURRENT		2184
#define Q_MAX_POS_25_H_CURRENT		2217
#define Q_MAX_POS_0_H_CURRENT			1260
#define Q_MAX_NEG_10_H_CURRENT		641


/* Discharge Percentage */
#define OAM_D5	1 ///0,modified by maxyu 170318	/*  1 : D5,   0: D2*/


/* battery meter parameter */
#define CHANGE_TRACKING_POINT
#define CUST_TRACKING_POINT	0
#define CUST_R_SENSE	200
#define CUST_HW_CC	0
#define AGING_TUNING_VALUE	103
#define CUST_R_FG_OFFSET	0

#define OCV_BOARD_COMPESATE	0 /*mV */
#define R_FG_BOARD_BASE	1000
#define R_FG_BOARD_SLOPE	1000 /*slope*/
#define CAR_TUNE_VALUE	100 /*1.00*/


/* HW Fuel gague  */
#define CURRENT_DETECT_R_FG	10  /*1mA*/
#define MinErrorOffset	1000
#define FG_VBAT_AVERAGE_SIZE	18
#define R_FG_VALUE	10 /* mOhm, base is 20*/

/* fg 2.0 */
#define DIFFERENCE_HWOCV_RTC	30
#define DIFFERENCE_HWOCV_SWOCV	15
#define DIFFERENCE_SWOCV_RTC	10
#define MAX_SWOCV	3

#define DIFFERENCE_VOLTAGE_UPDATE	20
#define AGING1_LOAD_SOC	70
#define AGING1_UPDATE_SOC	30
#define BATTERYPSEUDO100	90
#define BATTERYPSEUDO1	4

/*#define Q_MAX_BY_SYS	8. Qmax variant by system drop voltage.*/
/*#define SHUTDOWN_GAUGE0*/
#define SHUTDOWN_GAUGE1_XMINS
#define SHUTDOWN_GAUGE1_MINS	60

#define SHUTDOWN_SYSTEM_VOLTAGE	3400
#define CHARGE_TRACKING_TIME	60
#define DISCHARGE_TRACKING_TIME	10

#define RECHARGE_TOLERANCE	10
/* SW Fuel Gauge */
#define MAX_HWOCV	5
#define MAX_VBAT	90
#define DIFFERENCE_HWOCV_VBAT	30

/* fg 1.0 */
#define CUST_POWERON_DELTA_CAPACITY_TOLRANCE	40
#define CUST_POWERON_LOW_CAPACITY_TOLRANCE	5
#define CUST_POWERON_MAX_VBAT_TOLRANCE	90
#define CUST_POWERON_DELTA_VBAT_TOLRANCE	30
#define CUST_POWERON_DELTA_HW_SW_OCV_CAPACITY_TOLRANCE	30 //10,modified by maxyu 170323 for charge


/* Disable Battery check for HQA */
#ifdef CONFIG_MTK_DISABLE_POWER_ON_OFF_VOLTAGE_LIMITATION
#define FIXED_TBAT_25
#endif

#define SW_OAM_INIT_V2 //added by maxyu 170318
/* Dynamic change wake up period of battery thread when suspend*/
#define VBAT_NORMAL_WAKEUP	3600	/*3.6V*/
#define VBAT_LOW_POWER_WAKEUP	3500	/*3.5v*/
#define NORMAL_WAKEUP_PERIOD	1800 //5400	/*90 * 60 = 90 min*/
#define LOW_POWER_WAKEUP_PERIOD	300	/*5 * 60 = 5 min*/
#define CLOSE_POWEROFF_WAKEUP_PERIOD	30	/*30 s*/

#define INIT_SOC_BY_SW_SOC
/*#define SYNC_UI_SOC_IMM			3. UI SOC sync to FG SOC immediately*/
#define MTK_ENABLE_AGING_ALGORITHM	/*6. Q_MAX aging algorithm*/
#define MD_SLEEP_CURRENT_CHECK	/*5. Gauge Adjust by OCV 9. MD sleep current check*/
/*#define Q_MAX_BY_CURRENT		7. Qmax variant by current loading.*/

/*#define FG_BAT_INT*/
/*#define IS_BATTERY_REMOVE_BY_PMIC*/


#define FG_CURRENT_INIT_VALUE 3500



#endif
