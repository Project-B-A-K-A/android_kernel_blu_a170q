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

#define REGFLAG_DELAY             							0XFFE
#define REGFLAG_END_OF_TABLE      							0xFFF   // END OF REGISTERS MARKER

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
{0xFF,5,{0xFF,0x98,0x06,0x04,0x01}},
{0x08,1,{0x10}},
{0x21,1,{0x01}},
{0x30,1,{0x01}},
{0x31,1,{0x00}},
{0x50,1,{0x90}},
{0x51,1,{0x90}},
{0x60,1,{0x07}},
{0x61,1,{0x00}},
{0x62,1,{0x07}},
{0x63,1,{0x00}},
{0x40,1,{0x11}},
{0x41,1,{0x44}},
{0x42,1,{0x03}},
{0x43,1,{0x0A}},
{0x44,1,{0x05}},
{0x52,1,{0x00}},
{0x53,1,{0x06}},
{0x57,1,{0x50}},
{0xFF,5,{0xFF,0x98,0x06,0x04,0x01}},
{0xA0,1,{0x00}},
{0xA1,1,{0x11}},
{0xA2,1,{0x1A}},
{0xA3,1,{0x0E}},
{0xA4,1,{0x07}},
{0xA5,1,{0x0B}},
{0xA6,1,{0x08}},
{0xA7,1,{0x05}},
{0xA8,1,{0x09}},
{0xA9,1,{0x0D}},
{0xAA,1,{0x19}},
{0xAB,1,{0x14}},
{0xAC,1,{0x00}},
{0xAD,1,{0x00}},
{0xAE,1,{0x00}},
{0xAF,1,{0x00}},
{0xC0,1,{0x00}},
{0xC1,1,{0x12}},
{0xC2,1,{0x1C}},
{0xC3,1,{0x0E}},
{0xC4,1,{0x08}},
{0xC5,1,{0x09}},
{0xC6,1,{0x08}},
{0xC7,1,{0x06}},
{0xC8,1,{0x06}},
{0xC9,1,{0x09}},
{0xCA,1,{0x1B}},
{0xCB,1,{0x15}},
{0xCC,1,{0x00}},
{0xCD,1,{0x00}},
{0xCE,1,{0x01}},
{0xCF,1,{0x00}},
{0xFF,5,{0xFF,0x98,0x06,0x04,0x06}},
{0x00,1,{0x21}},
{0x01,1,{0x06}},
{0x02,1,{0xA0}},
{0x03,1,{0x02}},
{0x04,1,{0x01}},
{0x05,1,{0x01}},
{0x06,1,{0x80}},
{0x07,1,{0x03}},
{0x08,1,{0x00}},
{0x09,1,{0x80}},
{0x0A,1,{0x00}},
{0x0B,1,{0x00}},
{0x0C,1,{0x20}},
{0x0D,1,{0x20}},
{0x0E,1,{0x09}},
{0x0F,1,{0x00}},
{0x10,1,{0xFF}},
{0x11,1,{0xE0}},
{0x12,1,{0x00}},
{0x13,1,{0x00}},
{0x14,1,{0x00}},
{0x15,1,{0xC0}},
{0x16,1,{0x08}},
{0x17,1,{0x00}},
{0x18,1,{0x00}},
{0x19,1,{0x00}},
{0x1A,1,{0x00}},
{0x1B,1,{0x00}},
{0x1C,1,{0x00}},
{0x1D,1,{0x00}},
{0x20,1,{0x01}},
{0x21,1,{0x23}},
{0x22,1,{0x45}},
{0x23,1,{0x67}},
{0x24,1,{0x01}},
{0x25,1,{0x23}},
{0x26,1,{0x45}},
{0x27,1,{0x67}},
{0x30,1,{0x12}},
{0x31,1,{0x22}},
{0x32,1,{0x22}},
{0x33,1,{0x22}},
{0x34,1,{0x87}},
{0x35,1,{0x96}},
{0x36,1,{0xAA}},
{0x37,1,{0xDB}},
{0x38,1,{0xCC}},
{0x39,1,{0xBD}},
{0x3A,1,{0x78}},
{0x3B,1,{0x69}},
{0x3C,1,{0x22}},
{0x3D,1,{0x22}},
{0x3E,1,{0x22}},
{0x3F,1,{0x22}},
{0x40,1,{0x22}},
{0x52,1,{0x10}},
{0x53,1,{0x10}},
{0x54,1,{0x13}},
{0xFF,5,{0xFF,0x98,0x06,0x04,0x07}},
{0x02,1,{0x77}},
{0xE1,1,{0x79}},
{0x17,1,{0x22}},
{0xB3,1,{0x10}},
{0xFF,5,{0xFF,0x98,0x06,0x04,0x00}},


	{0x11,0,{}},
	{REGFLAG_DELAY,120,{}},
	{0x29,0,{}},
	{REGFLAG_DELAY,50,{}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct LCM_setting_table lcm_deep_sleep_mode_in_setting[] = {

	// Display off sequence
	{0x28, 1, {0x00}},
	{REGFLAG_DELAY, 50, {}},

    // Sleep Mode On
	{0x10, 1, {0x00}},
	{REGFLAG_DELAY, 200, {}},
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

	params->dsi.vertical_sync_active = 4;
	params->dsi.vertical_backporch = 16;//10 16
	params->dsi.vertical_frontporch = 20;//8  20
	params->dsi.vertical_active_line = FRAME_HEIGHT;

	params->dsi.horizontal_sync_active = 10; //8
	params->dsi.horizontal_backporch = 50; //50
	params->dsi.horizontal_frontporch = 50; //50
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;
	
		params->dsi.PLL_CLOCK = 180;///190 160 173; //dsi clock customization: should config clock value directly
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

	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);///for S4511 82
}

static void lcm_suspend(void)
{
    push_table(lcm_deep_sleep_mode_in_setting, sizeof(lcm_deep_sleep_mode_in_setting) / sizeof(struct LCM_setting_table), 1);
}

static void lcm_resume(void)
{
    lcm_init();
}

// ---------------------------------------------------------------------------
//  Get LCM Driver Hooks
// ---------------------------------------------------------------------------
LCM_DRIVER ili9806e_dsi_vdo_fwvga_gaofeng_ivo_lcm_drv = 
{
    .name			= "ili9806e_dsi_vdo_fwvga_gaofeng_ivo",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
};

