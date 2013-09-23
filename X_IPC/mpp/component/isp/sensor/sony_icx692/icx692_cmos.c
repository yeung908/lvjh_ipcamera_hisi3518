#if !defined(__SONY_ICX692_H_)
#define __SONY_ICX692_H_

#include <stdio.h>
#include <unistd.h>		// usleep
#include <string.h>

#include "hi_comm_sns.h"
#include "hi_sns_ctrl.h"
#include "mpi_isp.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */


/****************************************************************************
 * local variables															*
 ****************************************************************************/

static cmos_inttime_t cmos_inttime;
static cmos_gains_t cmos_gains;
HI_U8 gu8SensorMode = 0;

static cmos_isp_default_t st_coms_isp_default =
{
    // color correction matrix
    {
        5000,
    	{	0x1e5,  0x80ba, 0x802b,
    		0x8024, 0x0183, 0x805e,
    		0x0021, 0x80ac, 0x018a
    	},
    	3200,
        {
            0x0241, 0x80c5, 0x807c,
            0x802c, 0x016f, 0x8043,
            0x003d, 0x80ee, 0x01b0
        },
        2600,
        {
            0x0245, 0x80ce, 0x8077,
            0x802c, 0x0150, 0x8024,
            0x54  , 0x820a, 0x02b6
        }
    },

	// black level for R, Gr, Gb, B channels
	{0xf,0xf,0xf,0xf},

    // calibration reference color temperature
    5000,

    //WB gain at 5000K, must keep consistent with calibration color temperature
	{0x176, 0x100, 0x100, 0x1f1},

    // WB curve parameters, must keep consistent with reference color temperature.
	{33, 113, -110, 186795, 0x80, -138463},

	// hist_thresh
	{0xd,0x28,0x60,0x80},

	0x00,	// iridix_balck
	0x1,	// rggb

	// gain
	0x8,	0x40,

	// iridix space, intensity, slope_max, white level
	0x02,	0x08,	0x80, 	0x8ff,

	0x1, 	// balance_fe
	0x80,	// ae compensation
	0x20, 	// sinter threshold

	0x1,        //0: use default profile table; 1: use calibrated profile lut, the setting for nr0 and nr1 must be correct.
	0,
	1528
};


static cmos_isp_agc_table_t st_isp_agc_table =
{
//sharpen_alt_d
{0x50,0x45,0x40,0x38,0x34,0x30,0x28,0x28},

//sharpen_alt_ud
{0x3b,0x38,0x34,0x30,0x2b,0x28,0x24,0x20},

//snr_thresh
{0x23,0x2c,0x34,0x3e,0x46,0x4e,0x54,0x54},

//demosaic_lum_thresh
{0x60,0x60,0x80,0x80,0x80,0x80,0x80,0x80},

//demosaic_np_offset
{0x0,0xa,0x12,0x1a,0x20,0x28,0x30,0x30},

//ge_strength
{0x55,0x55,0x55,0x55,0x55,0x55,0x37,0x37}

};

static cmos_isp_noise_table_t st_isp_noise_table =
{
  //nosie_profile_weight_lut
    {0,  0,  0,  0,  0,  0,  11, 15, 17, 19, 20, 21, 22, 22, 23, 24,
    25, 25, 26, 26, 26, 27, 27, 27, 28, 28, 28, 29, 29, 29, 29, 29,
    30, 30, 30, 30, 30, 31, 31, 31, 31, 31, 32, 32, 32, 32, 32, 32,
    32, 33, 33, 33, 33, 33, 33, 33, 33, 33, 34, 34, 34, 34, 34, 34,
    34, 34, 34, 34, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 36, 36,
    36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 37, 37,
    37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 38, 38,
    38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38},

  //demosaic_weight_lut
    {0,11,15,17,19,20,21,22,23,23,24,25,25,26,26,26,
    27,27,27,28,28,28,29,29,29,29,29,30,30,30,30,30,
    31,31,31,31,31,32,32,32,32,32,32,32,33,33,33,33,
    33,33,33,33,33,34,34,34,34,34,34,34,34,34,34,35,
    35,35,35,35,35,35,35,35,35,35,35,36,36,36,36,36,
    36,36,36,36,36,36,36,36,36,37,37,37,37,37,37,37,
    37,37,37,37,37,37,37,37,37,38,38,38,38,38,38,38,
    38,38,38,38,38,38,38,38,38,38,38,38,38,38,38,38}
};

/*
 * This function initialises an instance of cmos_inttime_t.
 */
static __inline cmos_inttime_const_ptr_t cmos_inttime_initialize()
{
	cmos_inttime.full_lines_std_30fps = 748;
	cmos_inttime.full_lines_std_25fps = 748;
	cmos_inttime.full_lines_std = 748;
	cmos_inttime.full_lines = 748;
	cmos_inttime.max_lines_target = 746;

	cmos_inttime.full_lines_limit = 65535;
	cmos_inttime.min_lines_target = 1;
	cmos_inttime.vblanking_lines = 748;

	cmos_inttime.exposure_ashort = 0;

	cmos_inttime.lines_per_500ms = cmos_inttime.full_lines_std_30fps*30/2;
	cmos_inttime.flicker_freq = 0;//60*256;//50*256;

	return &cmos_inttime;
}

/*
 * This function applies the new integration time to the ISP registers.
 */
static __inline void cmos_inttime_update(cmos_inttime_ptr_t p_inttime)
{

	HI_U32 eshort = p_inttime->exposure_ashort;
	HI_U32 exp_frames = eshort / p_inttime->full_lines_std;

	eshort = eshort - exp_frames * p_inttime->full_lines_std;
	eshort = p_inttime->full_lines_std - eshort;

	sensor_write_register(0x7c13, (eshort&0xffff));  //SUB Start lines
	sensor_write_register(0x7c14, (eshort&0xffff));  //SUB End lines,only one sub pulse .
	/*
	sensor_write_register(0x7D03, 0x4);  			//Primary field counter enable,bit[2]
	sensor_write_register(0x7C7C, ((exp_frames+1)&0x1fff));  //SGMASK_NUM,bit[12~0]
	sensor_write_register(0x7C7D, ((exp_frames+1)&0x1fff));  //SUBCKMASK_NUM,bit[12~0]
	sensor_write_register(0x7C7B, 0xA);  			//SUBCK_MASK_SKIP1,bit[1]
	*/

}

/*
 * This function applies the new vert blanking porch to the ISP registers.
 */
static __inline void cmos_vblanking_update(cmos_inttime_const_ptr_t p_inttime)
{
	return;
	int full_lines_lo16 = p_inttime->full_lines & 0xffff;
	int full_lines_hi16= (p_inttime->full_lines >> 16) & 0x1;

}

static __inline HI_U16 vblanking_calculate(
		cmos_inttime_ptr_t p_inttime)
{
	if (p_inttime->exposure_ashort >= p_inttime->full_lines - 3)
	{
		p_inttime->exposure_ashort = p_inttime->full_lines - 3;
	}

	p_inttime->vblanking_lines = p_inttime->full_lines - p_inttime->full_lines_std;

	return p_inttime->exposure_ashort;
}

extern void sensor_init_exit(int fps);

/* Set fps base */
static __inline void cmos_fps_set(
		cmos_inttime_ptr_t p_inttime,
		const HI_U8 fps
		)
{
	usleep(40000);
	return;
	if (30 == fps)
	{

		sensor_init_exit(30);
		p_inttime->full_lines_std = 748;
		p_inttime->full_lines = 748;
		p_inttime->max_lines_target = 748;
	}
	else if (25 == fps)
	{
		sensor_init_exit(25);
		p_inttime->full_lines_std = 1350;
		p_inttime->full_lines = 1350;
		p_inttime->max_lines_target = 1347;
	}
}

/*
 * This function initialises an instance of cmos_gains_t.
 */
static __inline cmos_gains_ptr_t cmos_gains_initialize()
{
	cmos_gains.max_again = 0x8;
	cmos_gains.max_dgain = 0x40;

	cmos_gains.again_shift = 0;
	cmos_gains.dgain_shift = 0;
	cmos_gains.dgain_fine = 0;

    cmos_gains.isp_dgain_shift = 4;
    cmos_gains.isp_dgain = 1 << cmos_gains.isp_dgain_shift;
    cmos_gains.max_isp_dgain_target = 1 << cmos_gains.isp_dgain_shift;

	return &cmos_gains;
}

static __inline HI_U32 cmos_get_ISO(cmos_gains_ptr_t p_gains)
{
	HI_U32 _again = p_gains->again == 0 ? 1 : p_gains->again;
	HI_U32 _dgain = p_gains->dgain == 0 ? 1 : p_gains->dgain;

	p_gains->iso =  ((_again * _dgain * 100) >> (p_gains->again_shift + p_gains->dgain_shift));

	return p_gains->iso;
}

/*
 * This function applies the digital gain
 * input and output offset and correction
 */
static __inline void cmos_gains_update(cmos_gains_const_ptr_t p_gains)
{

	switch (p_gains->again_db)
    {
    case 0:
        sensor_write_register(0xC001, 0x0);
		break;

	case 3:
        sensor_write_register(0xC001, 0x1);
        break;

    case 6:
        sensor_write_register(0xC001, 0x2);
        break;

    case 9:
        sensor_write_register(0xC001, 0x3);
        break;

    case 12:
        sensor_write_register(0xC001, 0x4);
        break;

    case 15:
        sensor_write_register(0xC001, 0x5);
        break;

    case 18:
        sensor_write_register(0xC001, 0x6);
        break;

    default:
        break;
    }

	//set the dgain
	 sensor_write_register(0xC002, (p_gains->dgain_db & 0x3ff));

	//printf("again = %d,dgain = %d\n",p_gains->again_db,p_gains->dgain_db);

}

/*
 * This function applies the new gains to the ISP registers.
 */
static __inline HI_U16 cmos_gains_update2(cmos_gains_const_ptr_t p_gains)
{
	return 0;
}

static __inline HI_U32 analog_gain_from_exposure_calculate(
		cmos_gains_ptr_t p_gains,
		HI_U32 exposure,
		HI_U32 exposure_max,
		HI_U32 exposure_shift)
{
	int _i;
	HI_U32 _again = 0;
	HI_U32 _again_db = 9;
	HI_U32 exposure1,exposure0;
	int shft = 0;

	// normalize
	while (exposure > (1<<22))
	{
		exposure >>= 1;
		exposure_max >>= 1;
		++shft;
	}
	exposure0 = exposure>>1;
	exposure0 = (exposure0*725) >> 10;

	   /* again unit: 3db */

	for(_i = 3; _i <6 ; _i++)
	{
		exposure1 = (exposure0*725) >> 10;
		if(exposure1 <= exposure_max)
			break;

		_again_db = _again_db +3;
		exposure0 = exposure1;

	}


	p_gains->again = (exposure << p_gains->again_shift) / exposure0;;
	p_gains->again_db = _again_db;

	return (exposure0 << shft);
}



static __inline HI_U32 digital_gain_from_exposure_calculate(
		cmos_gains_ptr_t p_gains,
		HI_U32 exposure,
		HI_U32 exposure_max,
		HI_U32 exposure_shift)
{
	int _i;
	HI_U32 _dgain = 0x0;
	HI_U32 exposure0, exposure1;
	int shft = 0;
	// normalize
	while (exposure > (1<<20)) /* analog use (1<<22) for analog exposure is bigger. */
	{
		exposure >>= 1;
		exposure_max >>= 1;
		++shft;
	}

	exposure0 = exposure;

	/* unit: 0.035db */
	for(_i = 0x0; _i < 0x3ff; _i++)
	{
		exposure1 = (exposure0*1020) >> 10;
		if(exposure1 <= exposure_max)
			break;
		++_dgain;
		exposure0 = exposure1;
	}
	p_gains->dgain = (exposure << p_gains->dgain_shift) / exposure0;
       p_gains->dgain_db = _dgain;
	return exposure0 << shft;
}

static void setup_sensor(int isp_mode)
{
		return;
	if(0 == isp_mode) /* setup for ISP 'normal mode' */
	{
		sensor_write_register(0x00A5,0x0000);
	}
	else if(1 == isp_mode) /* setup for ISP pixel calibration mode */
	{
        //set the gain to 0
		sensor_write_register(0x0020,0x0080);
		sensor_write_register(0x0021,0x0080);
		sensor_write_register(0x00A1,0x0400);
		sensor_write_register(0x00A2,0x0002);
		sensor_write_register(0x00A5,0x0005);
	}
}

static HI_U8 cmos_get_analog_gain(cmos_gains_ptr_t p_gains)
{
    return p_gains->again_db;
}

static HI_U8 cmos_get_digital_gain(cmos_gains_ptr_t p_gains)
{
	return p_gains->dgain_db;
}
/*
static HI_U8 cmos_get_digital_fine_gain(cmos_gains_ptr_t cmos_gains)
{
    return cmos_gains->dgain_fine;
}
*/
static HI_U32 cmos_get_isp_default(cmos_isp_default_ptr_t p_coms_isp_default)
{
	if (NULL == p_coms_isp_default)
	{
	    printf("null pointer when get isp default value!\n");
	    return -1;
	}
    memcpy(p_coms_isp_default, &st_coms_isp_default, sizeof(cmos_isp_default_t));
    return 0;
}


static HI_U32 cmos_get_isp_agc_table(cmos_isp_agc_table_ptr_t p_cmos_isp_agc_table)
{
	if (NULL == p_cmos_isp_agc_table)
	{
	    printf("null pointer when get isp agc table value!\n");
	    return -1;
	}
    memcpy(p_cmos_isp_agc_table, &st_isp_agc_table, sizeof(cmos_isp_agc_table_t));
    return 0;
}

static HI_U32 cmos_get_isp_noise_table(cmos_isp_noise_table_ptr_t p_cmos_isp_noise_table)
{
	if (NULL == p_cmos_isp_noise_table)
	{
	    printf("null pointer when get isp noise table value!\n");
	    return -1;
	}
    memcpy(p_cmos_isp_noise_table, &st_isp_noise_table, sizeof(cmos_isp_noise_table_t));
    return 0;
}

/****************************************************************************
 * callback structure                                                       *
 ****************************************************************************/

static SENSOR_EXP_FUNC_S stSensorExpFuncs =
{
    .pfn_cmos_inttime_initialize = cmos_inttime_initialize,
    .pfn_cmos_inttime_update = cmos_inttime_update,

    .pfn_cmos_gains_initialize = cmos_gains_initialize,
    .pfn_cmos_gains_update = cmos_gains_update,
    .pfn_cmos_gains_update2 = NULL,
    .pfn_analog_gain_from_exposure_calculate = analog_gain_from_exposure_calculate,
    .pfn_digital_gain_from_exposure_calculate = digital_gain_from_exposure_calculate,

    .pfn_cmos_fps_set = cmos_fps_set,
    .pfn_vblanking_calculate = vblanking_calculate,
    .pfn_cmos_vblanking_front_update = cmos_vblanking_update,

    .pfn_setup_sensor = setup_sensor,

    .pfn_cmos_get_analog_gain = cmos_get_analog_gain,
    .pfn_cmos_get_digital_gain = cmos_get_digital_gain,
    .pfn_cmos_get_digital_fine_gain = NULL,
    .pfn_cmos_get_iso = cmos_get_ISO,

    .pfn_cmos_get_isp_default = cmos_get_isp_default,
    .pfn_cmos_get_isp_special_alg = NULL,
    .pfn_cmos_get_isp_agc_table = cmos_get_isp_agc_table,
	.pfn_cmos_get_isp_noise_table = cmos_get_isp_noise_table,
	.pfn_cmos_get_isp_demosaic = NULL,
	.pfn_cmos_get_isp_shading_table = NULL,

};

int sensor_register_callback(void)
{
	int ret;
	ret = HI_MPI_ISP_SensorRegCallBack(&stSensorExpFuncs);
	if (ret)
	{
	    printf("sensor register callback function failed!\n");
	    return ret;
	}

	return 0;
}

//chang sensor mode
int sensor_mode_set(HI_U8 u8Mode)
{
    switch(u8Mode)
    {
        //sensor mode 0
        case 0:
            gu8SensorMode = 0;
            // TODO:
        break;
        //sensor mode 1
        case 1:
            gu8SensorMode = 1;
             // TODO:
        break;

        default:
            printf("NOT support this mode!\n");
            return -1;
        break;
    }
    return 0;
}


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */


#endif // __SONY_ICX692_H_
