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

#define FRAME_WIDTH  (480)
#define FRAME_HEIGHT (854)

#define REGFLAG_DELAY             							0XFE
#define REGFLAG_END_OF_TABLE      						0xFFF   // END OF REGISTERS MARKER

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
{0xBF,3,{0x91,0x61,0xF2}},
{0xB3,2,{0x00,0xA5}},
{0xB8,6,{0x00,0xBF,0x01,0x00,0xBF,0x01}},
{0xBA,3,{0x3E,0x23,0x04}},
{0xC3,1,{0x04}},
{0xC4,2,{0x30,0x6A}},
{0xC7,9,{0x00,0x21,0x42,0x05,0x25,0x35,0x12,0xA5,0xA5}},
{0xC8,38,{0x7F,0x68,0x55,0x47,0x40,0x30,0x33,0x1D,0x37,0x37,
0x37,0x53,0x40,0x45,0x35,0x2B,0x1B,0x0B,0x00,0x7F,
0x68,0x55,0x47,0x40,0x30,0x33,0x1D,0x37,0x37,0x37,
0x53,0x40,0x45,0x35,0x2B,0x1B,0x0B,0x00}},
{0xD4,16,{0x1F,0x1F,0x1F,0x03,0x01,0x05,0x07,0x09,0x0B,0x11
,0x13,0x1F,0x1F,0x1F,0x1F,0x1F}}, 
{0xD5,16,{0x1F,0x1F,0x1F,0x02,0x00,0x04,0x06,0x08,0x0A,0x10
,0x12,0x1F,0x1F,0x1F,0x1F,0x1F}},
{0xD6,16,{0x1F,0x1F,0x1F,0x10,0x12,0x04,0x0A,0x08,0x06,0x02
,0x00,0x1F,0x1F,0x1F,0x1F,0x1F}}, 
{0xD7,16,{0x1F,0x1F,0x1F,0x11,0x13,0x05,0x0B,0x09,0x07,0x03
,0x01,0x1F,0x1F,0x1F,0x1F,0x1F}},
{0xD8,20,{0x20,0x00,0x00,0x30,0x03,0x30,0x01,0x02,0x30,0x01
,0x02,0x06,0x70,0x73,0x5D,0x73,0x06,0x06,0x70,0x08}}, 
{0xD9,19,{0x00,0x0A,0x0A,0x88,0x00,0x00,0x06,0x7B,0x00,0x80
,0x00,0x3B,0x33,0x1F,0x00,0x00,0x00,0x06,0x70}},

{0x11,0,{}}, 
{REGFLAG_DELAY, 150, {}},
{0x29,0,{}}, 
{REGFLAG_DELAY, 30, {}},

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
		params->dbi.te_mode 			= LCM_DBI_TE_MODE_VSYNC_ONLY;
	params->dbi.te_edge_polarity		= LCM_POLARITY_RISING;

	params->dsi.mode   = SYNC_PULSE_VDO_MODE; //BURST_VDO_MODE;

	// DSI
	/* Command mode setting */
	params->dsi.LANE_NUM				= LCM_TWO_LANE;

	//The following defined the fomat for data coming from LCD engine.
	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

	params->dsi.packet_size=256;
	params->dsi.intermediat_buffer_num = 2;

	params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;

	params->dsi.word_count=FRAME_WIDTH*3;	//DSI CMD mode need set these two bellow params, different to 6577
	params->dsi.vertical_active_line=FRAME_HEIGHT;
    
	params->dsi.vertical_sync_active				= 4;
	params->dsi.vertical_backporch					= 7;
	params->dsi.vertical_frontporch					= 11;
	params->dsi.vertical_active_line				= FRAME_HEIGHT; 

	params->dsi.horizontal_sync_active				= 20;
	params->dsi.horizontal_backporch				= 20;
	params->dsi.horizontal_frontporch				= 20;
	params->dsi.horizontal_active_pixel				= FRAME_WIDTH;

    

	params->dsi.PLL_CLOCK = 208;///160 173; //dsi clock customization: should config clock value directly
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
}



static void lcm_resume(void)
{
	lcm_init();
}


LCM_DRIVER jd9161ba_dsi_vdo_fwvga_zhongguangdian_ivo_lcm_drv = 
{
	.name			= "jd9161ba_dsi_vdo_fwvga_zhongguangdian_ivo",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
};

