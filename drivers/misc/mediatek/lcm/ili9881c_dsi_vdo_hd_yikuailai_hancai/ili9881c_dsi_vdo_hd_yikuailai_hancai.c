/*****************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of MediaTek Inc. (C) 2008
*
*  BY OPENING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
*  THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
*  RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON
*  AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
*  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
*  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
*  NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
*  SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
*  SUPPLIED WITH THE MEDIATEK SOFTWARE, AND BUYER AGREES TO LOOK ONLY TO SUCH
*  THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. MEDIATEK SHALL ALSO
*  NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO BUYER'S
*  SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
*
*  BUYER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE
*  LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
*  AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
*  OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY BUYER TO
*  MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
*
*  THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE
*  WITH THE LAWS OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF
*  LAWS PRINCIPLES.  ANY DISPUTES, CONTROVERSIES OR CLAIMS ARISING THEREOF AND
*  RELATED THERETO SHALL BE SETTLED BY ARBITRATION IN SAN FRANCISCO, CA, UNDER
*  THE RULES OF THE INTERNATIONAL CHAMBER OF COMMERCE (ICC).
*
*****************************************************************************/
#ifndef BUILD_LK
#include <linux/string.h>
#endif
#include "lcm_drv.h"

// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  (720)
#define FRAME_HEIGHT (1280)

#define REGFLAG_DELAY             							0XFE
#define REGFLAG_END_OF_TABLE      						0xFFF   // END OF REGISTERS MARKER

#define LCM_ILI9881C_ID                                      0x9881
// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))

#define UDELAY(n) (lcm_util.udelay(n))
#define MDELAY(n) (lcm_util.mdelay(n))

// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)									lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)				lcm_util.dsi_write_regs(addr, pdata, byte_nums)   

struct LCM_setting_table {
    unsigned cmd;
    unsigned char count;
    unsigned char para_list[64];
};


static struct LCM_setting_table lcm_initialization_setting[] =
{ 
	{0xFF,3,{0x98,0x81,0x03}},
	{0x01,1,{0x00}},
	{0x02,1,{0x00}},
	{0x03,1,{0x72}},
	{0x04,1,{0x00}},
	{0x05,1,{0x00}},
	{0x06,1,{0x09}},
	{0x07,1,{0x00}},
	{0x08,1,{0x00}},
	{0x09,1,{0x01}},
	{0x0a,1,{0x00}},
	{0x0b,1,{0x00}},
	{0x0c,1,{0x01}},
	{0x0d,1,{0x00}},
	{0x0e,1,{0x00}},
	{0x0f,1,{0x00}},
	{0x10,1,{0x00}},
	{0x11,1,{0x00}},
	{0x12,1,{0x00}},
	{0x13,1,{0x00}},
	{0x14,1,{0x00}},
	{0x15,1,{0x00}},
	{0x16,1,{0x00}},
	{0x17,1,{0x00}},
	{0x18,1,{0x00}},
	{0x19,1,{0x00}},
	{0x1a,1,{0x00}},
	{0x1b,1,{0x00}},
	{0x1c,1,{0x00}},
	{0x1d,1,{0x00}},
	{0x1e,1,{0x40}},
	{0x1f,1,{0x80}},
	{0x20,1,{0x05}},
	{0x21,1,{0x02}},
	{0x22,1,{0x00}},
	{0x23,1,{0x00}},
	{0x24,1,{0x00}},
	{0x25,1,{0x00}},
	{0x26,1,{0x00}},
	{0x27,1,{0x00}},
	{0x28,1,{0x33}},
	{0x29,1,{0x02}},
	{0x2a,1,{0x00}},
	{0x2b,1,{0x00}},
	{0x2c,1,{0x00}},
	{0x2d,1,{0x00}},
	{0x2e,1,{0x00}},
	{0x2f,1,{0x00}},
	{0x30,1,{0x00}},
	{0x31,1,{0x00}},
	{0x32,1,{0x00}},
	{0x33,1,{0x00}},
	{0x34,1,{0x04}},
	{0x35,1,{0x00}},
	{0x36,1,{0x00}},
	{0x37,1,{0x00}},
	{0x38,1,{0x3C}},
	{0x39,1,{0x00}},
	{0x3a,1,{0x40}},
	{0x3b,1,{0x40}},
	{0x3c,1,{0x00}},
	{0x3d,1,{0x00}},
	{0x3e,1,{0x00}},
	{0x3f,1,{0x00}},
	{0x40,1,{0x00}},
	{0x41,1,{0x00}},
	{0x42,1,{0x00}},
	{0x43,1,{0x00}},
	{0x44,1,{0x00}},
	{0x50,1,{0x01}},
	{0x51,1,{0x23}},
	{0x52,1,{0x45}},
	{0x53,1,{0x67}},
	{0x54,1,{0x89}},
	{0x55,1,{0xab}},
	{0x56,1,{0x01}},
	{0x57,1,{0x23}},
	{0x58,1,{0x45}},
	{0x59,1,{0x67}},
	{0x5a,1,{0x89}},
	{0x5b,1,{0xab}},
	{0x5c,1,{0xcd}},
	{0x5d,1,{0xef}},
	{0x5e,1,{0x11}},
	{0x5f,1,{0x01}},
	{0x60,1,{0x00}},
	{0x61,1,{0x15}},
	{0x62,1,{0x14}},
	{0x63,1,{0x0E}},
	{0x64,1,{0x0F}},
	{0x65,1,{0x0C}},
	{0x66,1,{0x0D}},
	{0x67,1,{0x06}},
	{0x68,1,{0x02}},
	{0x69,1,{0x07}},
	{0x6a,1,{0x02}},
	{0x6b,1,{0x02}},
	{0x6c,1,{0x02}},
	{0x6d,1,{0x02}},
	{0x6e,1,{0x02}},
	{0x6f,1,{0x02}},
	{0x70,1,{0x02}},
	{0x71,1,{0x02}},
	{0x72,1,{0x02}},
	{0x73,1,{0x02}},
	{0x74,1,{0x02}},
	{0x75,1,{0x01}},
	{0x76,1,{0x00}},
	{0x77,1,{0x14}},
	{0x78,1,{0x15}},
	{0x79,1,{0x0E}},
	{0x7a,1,{0x0F}},
	{0x7b,1,{0x0C}},
	{0x7c,1,{0x0D}},
	{0x7d,1,{0x06}},
	{0x7e,1,{0x02}},
	{0x7f,1,{0x07}},
	{0x80,1,{0x02}},
	{0x81,1,{0x02}},
	{0x82,1,{0x02}},
	{0x83,1,{0x02}},
	{0x84,1,{0x02}},
	{0x85,1,{0x02}},
	{0x86,1,{0x02}},
	{0x87,1,{0x02}},
	{0x88,1,{0x02}},
	{0x89,1,{0x02}},
	{0x8A,1,{0x02}},
	{0xFF,3,{0x98,81,04}},
	{0x00,1,{0x00}},
	{0x6C,1,{0x15}},
	{0x6E,1,{0x2A}},
	{0x6F,1,{0x33}},
	{0x3A,1,{0x94}},
	{0x8D,1,{0x14}},
	{0x87,1,{0xBA}},
	{0x26,1,{0x76}},
	{0xB2,1,{0xD1}},
	{0xB5,1,{0x06}},
	{0xFF,3,{0x98,81,01}},
	{0x22,1,{0x0A}},
	{0x31,1,{0x00}},
	{0x53,1,{0x95}},

	{0x54,1,{0x01}},
	{0x55,1,{0x18}},

	{0x50,1,{0xB7}},
	{0x51,1,{0xB7}},
	{0x60,1,{0x22}},
	{0x61,1,{0x00}},
	{0x62,1,{0x19}},
	{0x63,1,{0x10}},

	{0xA0,1,{0x08}},
	{0xA1,1,{0x15}},
	{0xA2,1,{0x1D}},
	{0xA3,1,{0x0F}},
	{0xA4,1,{0x12}},
	{0xA5,1,{0x23}},
	{0xA6,1,{0x18}},
	{0xA7,1,{0x1C}},
	{0xA8,1,{0x6C}},
	{0xA9,1,{0x1E}},
	{0xAA,1,{0x29}},
	{0xAB,1,{0x61}},
	{0xAC,1,{0x1D}},
	{0xAD,1,{0x1D}},
	{0xAE,1,{0x53}},
	{0xAF,1,{0x21}},
	{0xB0,1,{0x29}},
	{0xB1,1,{0x48}},
	{0xB2,1,{0x69}},
	{0xB3,1,{0x39}},

	{0xC0,1,{0x08}},
	{0xC1,1,{0x12}},
	{0xC2,1,{0x1F}},
	{0xC3,1,{0x0D}},
	{0xC4,1,{0x0D}},
	{0xC5,1,{0x20}},
	{0xC6,1,{0x14}},
	{0xC7,1,{0x19}},
	{0xC8,1,{0x6A}},
	{0xC9,1,{0x1B}},
	{0xCA,1,{0x28}},
	{0xCB,1,{0x62}},
	{0xCC,1,{0x1C}},
	{0xCD,1,{0x1B}},
	{0xCE,1,{0x4D}},
	{0xCF,1,{0x26}},
	{0xD0,1,{0x29}},
	{0xD1,1,{0x44}},
	{0xD2,1,{0x66}},
	{0xD3,1,{0x39}},
	{0xFF,3,{0x98,0x81,0x00}},

	{0x35,1,{0x00}},
	
	{0x11, 0,{0x00}},
	{REGFLAG_DELAY, 120, {}},
	{0x29, 0,{0x00}},
	{REGFLAG_DELAY, 20, {}},

{REGFLAG_END_OF_TABLE, 0x00, {}}
};


static struct LCM_setting_table lcm_deep_sleep_mode_in_setting[] = {
     // Display off sequence
     {0x28, 1, {0x00}},
     {REGFLAG_DELAY, 20, {}},
     // Sleep Mode Ondiv1_real*div2_real
     {0x10, 1, {0x00}},
     {REGFLAG_DELAY, 120, {}},
     {REGFLAG_END_OF_TABLE, 0x00, {}}
};


static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
	unsigned int i;

    for(i = 0; i < count; i++) {
		
        unsigned cmd;
        cmd = table[i].cmd;
		
        switch (cmd) {
			
            case REGFLAG_DELAY :
                MDELAY(table[i].count);
                break;
				
            case REGFLAG_END_OF_TABLE :
                break;
				
            default:
				dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
				MDELAY(2);
       	}
    }
	
}


// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------
static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
    memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}


static void lcm_get_params(LCM_PARAMS *params)
{
	memset(params, 0, sizeof(LCM_PARAMS));

	params->type   = LCM_TYPE_DSI;

	params->width  = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;

	// enable tearing-free
    params->dbi.te_mode = LCM_DBI_TE_MODE_DISABLED;
	params->dbi.te_edge_polarity		= LCM_POLARITY_RISING;

	params->dsi.mode   = SYNC_PULSE_VDO_MODE; //BURST_VDO_MODE;

	// DSI
	/* Command mode setting */
    params->dsi.LANE_NUM				= LCM_THREE_LANE; ///LCM_TWO_LANE

	//The following defined the fomat for data coming from LCD engine.
	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

	params->dsi.packet_size=256;
	params->dsi.intermediat_buffer_num = 2;

	params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;

	params->dsi.word_count=FRAME_WIDTH * 3;	//DSI CMD mode need set these two bellow params, different to 6577
	params->dsi.vertical_active_line=FRAME_HEIGHT;
    

	params->dsi.vertical_sync_active				= 6;
	params->dsi.vertical_backporch					= 15;
	params->dsi.vertical_frontporch					= 19;
	params->dsi.vertical_active_line				= FRAME_HEIGHT; 

	params->dsi.horizontal_sync_active = 40; 
	params->dsi.horizontal_backporch = 120; 
	params->dsi.horizontal_frontporch = 80; 

	params->dsi.horizontal_active_pixel				= FRAME_WIDTH;
	
	params->dsi.PLL_CLOCK = 250;
	params->dsi.ssc_disable = 1;
	params->dsi.ssc_range = 4;
}

static void lcm_init(void)
{
	SET_RESET_PIN(1);
	SET_RESET_PIN(0);
	MDELAY(10);
	SET_RESET_PIN(1);
	MDELAY(120);

	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
}


static void lcm_suspend(void)
{
  push_table(lcm_deep_sleep_mode_in_setting, sizeof(lcm_deep_sleep_mode_in_setting) / sizeof(struct LCM_setting_table), 1);
	
	SET_RESET_PIN(0);
	MDELAY(20);
}



static void lcm_resume(void)
{
	lcm_init();
}

LCM_DRIVER ili9881c_dsi_vdo_hd_yikuailai_hancai_lcm_drv = 
{
	.name			= "ili9881c_dsi_vdo_hd_yikuailai_hancai",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,	
};

