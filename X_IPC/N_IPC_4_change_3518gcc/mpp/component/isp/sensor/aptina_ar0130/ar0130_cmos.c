#if !defined(__AR0130_CMOS_H_)
#define __AR0130_CMOS_H_

#include <stdio.h>
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
    {
        5000,
        {
            0x1cb,0x808f,0x803c,
            0x804b,0x166,0x801b,
            0x8,  0x80a9,0x1a0
        },

        3200,
        {
            0x1da,0x809e,0x803c,
            0x805f,0x175,0x8016,
            0x802c,0x8124,0x250
        },

        2600,
        {
         0x233,  0x80f4, 0x803f,
         0x8041, 0x131,  0xf,
         0x809c, 0x81b3, 0x34f
        }
    },

	// black level for R, Gr, Gb, B channels
	{0xA8,0xA8,0xA8,0xA8},

	//calibration reference color temperature
	5200,

	//WB gain at Macbeth 5000K, must keep consistent with calibration color temperature

    // {0x181,0x100,0x100,0x1b3}, //for demo
    {0x0197,0x0100,0x0100,0x01C4},//for ref

	// WB curve parameters, must keep consistent with reference color temperature.

    // {96,-23,-182,225343,128,-178914}, //for demo
    {66,36,-153,213987,128,-166117},  //for ref


	// hist_thresh
	{0xd,0x28,0x60,0x80},

	0x00,	// iridix_balck
	0x1,	// rggb

	// gain
	//0x8,	0x8, // this is gain target, it will be constricted by sensor-gain.
	0x8,	0x4, /* The purpose of setting max dgain target to 4 is to reduce FPN */

	//wdr_variance_space, wdr_variance_intensity, slope_max_write,  white_level_write
	0x04,	0x01,	0x30, 	0x4FF,

	0x1, 	// balance_fe
	0x80,	// ae compensation
	0x23, 	// sinter threshold

	0x1,     //noise profile=0, use the default noise profile lut, don't need to set nr0 and nr1
	0x0,
	546
};

static cmos_isp_agc_table_t st_isp_agc_table =
{
    //sharpen_alt_d
    {0x50,0x48,0x40,0x38,0x34,0x30,0x28,0x28},

    //sharpen_alt_ud
    {0x90,0x88,0x80,0x70,0x60,0x50,0x40,0x40},

    //snr_thresh
    {0x13,0x19,0x20,0x26,0x2c,0x32,0x38,0x38},

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

static cmos_isp_demosaic_t st_isp_demosaic =
{
    /*vh_slope*/
    0xf5,

    /*aa_slope*/
    0xb4,

    /*va_slope*/
    0xe6,

    /*uu_slope*/
    0x80,

    /*sat_slope*/
    0x5d,

    /*ac_slope*/
    0xcf,

    /*vh_thresh*/
    0x30,

    /*aa_thresh*/
    0x5b,

    /*va_thresh*/
    0x52,

    /*uu_thresh*/
    0x40,

    /*sat_thresh*/
    0x171,

    /*ac_thresh*/
    0x1b3
};

/*
 * This function initialises an instance of cmos_inttime_t.
 */
static /*__inline*/ cmos_inttime_const_ptr_t cmos_inttime_initialize()
{
    //TODO: min/max integration time control.
   	//cmos_inttime.min_lines_std = 128;
	cmos_inttime.full_lines_std = 750;
	cmos_inttime.full_lines_std_30fps = 750;
	cmos_inttime.full_lines = 750;
	cmos_inttime.full_lines_del = 750; //TODO: remove
	cmos_inttime.full_lines_limit = 65535;
	cmos_inttime.max_lines = 748;
	cmos_inttime.min_lines = 2;
	cmos_inttime.vblanking_lines = 0;

	cmos_inttime.exposure_ashort = 0;
	cmos_inttime.exposure_shift = 0;

	cmos_inttime.lines_per_500ms = 750*30/2; // 500ms / 22.22us
	cmos_inttime.flicker_freq = 0;//60*256;//50*256;

	cmos_inttime.max_lines_target = cmos_inttime.max_lines;
	cmos_inttime.min_lines_target = cmos_inttime.min_lines;
	//cmos_inttime.max_flicker_lines = cmos_inttime.max_lines_target;
	//cmos_inttime.min_flicker_lines = cmos_inttime.min_lines_target;
	//cmos_inttime.input_changed = 0;
	return &cmos_inttime;
}

/*
 * This function applies the new integration time to the ISP registers.
 */
static __inline void cmos_inttime_update(cmos_inttime_ptr_t p_inttime)
{
	HI_U16 _time = p_inttime->exposure_ashort >> p_inttime->exposure_shift;
	sensor_write_register(0x3012, _time);
}

/*
 * This function applies the new vert blanking porch to the ISP registers.
 */
static __inline void cmos_vblanking_update(cmos_inttime_const_ptr_t p_inttime)
{
     int  _fulllines= p_inttime->full_lines;

       sensor_write_register(0x300A, _fulllines);

}

static __inline HI_U16 vblanking_calculate(
		cmos_inttime_ptr_t p_inttime)
{
	p_inttime->exposure_along  = p_inttime->exposure_ashort;

	if(p_inttime->exposure_along < p_inttime->full_lines_std - 2)
	{
		p_inttime->full_lines_del = p_inttime->full_lines_std;
	}
	if(p_inttime->exposure_along >= p_inttime->full_lines_std - 2)
	{
		p_inttime->full_lines_del = p_inttime->exposure_along + 2;
	}
#if defined(TRACE_ALL)
	alt_printf("full_lines_del = %x\n", p_inttime->full_lines_del);
#endif
	p_inttime->vblanking_lines = p_inttime->full_lines_del - 720;
#if defined(TRACE_ALL)
	alt_printf("vblanking_lines = %x\n", p_inttime->vblanking_lines);
#endif
	return p_inttime->exposure_ashort;
}

/* Set fps base */
static __inline void cmos_fps_set(
		cmos_inttime_ptr_t p_inttime,
		const HI_U8 fps
		)
{
	switch(fps)
	{
		case 30:
			p_inttime->full_lines_std = 750;
			p_inttime->lines_per_500ms = 750 * 30 / 2;
			sensor_write_register(0x300A, 0x2EE);
		break;

		case 25:
			p_inttime->full_lines_std = 900;
			p_inttime->lines_per_500ms = 900 * 25 / 2;
			sensor_write_register(0x300A, 0x384);
		break;

		default:
		break;
	}
}

/*
 * This function initialises an instance of cmos_gains_t.
 */
static /*__inline*/ cmos_gains_ptr_t cmos_gains_initialize()
{

	cmos_gains.max_again = 8;
	cmos_gains.max_dgain = 255;
	cmos_gains.max_again_target = cmos_gains.max_again;
	cmos_gains.max_dgain_target = cmos_gains.max_dgain;

	cmos_gains.again_shift = 0;
	cmos_gains.dgain_shift = 5;
	cmos_gains.dgain_fine_shift = 0;

	cmos_gains.again = 1;
	cmos_gains.dgain = 1;
	cmos_gains.dgain_fine = 1;
	cmos_gains.again_db = 0;
	cmos_gains.dgain_db = 0;

    cmos_gains.isp_dgain_shift = 4;
    cmos_gains.isp_dgain = 1 << cmos_gains.isp_dgain_shift;
    cmos_gains.max_isp_dgain_target = 1 << cmos_gains.isp_dgain_shift;

//	cmos_gains.input_changed = 0;

	return &cmos_gains;
}

/*
 * This function applies the new gains to the ISP registers.
 */
static __inline void cmos_gains_update(cmos_gains_const_ptr_t p_gains)
{
	int ag = p_gains->again;
	int dg = p_gains->dgain;

	switch(ag)
	{
		case(0):
			sensor_write_register(0x30B0, 0x1300);
			break;
		case(1):
			sensor_write_register(0x30B0, 0x1300);
			break;
		case(2):
			sensor_write_register(0x30B0, 0x1310);
			break;
		case(4):
			sensor_write_register(0x30B0, 0x1320);
			break;
		case(8):
			sensor_write_register(0x30B0, 0x1330);
			break;
	}

	sensor_write_register(0x305E, dg);
}

/* Emulate digital fine gain */
static __inline void em_dgain_fine_update(cmos_gains_ptr_t p_gains)
{
}

static HI_U32 cmos_gains_lin_to_db_convert(HI_U32 data, HI_U32 shift_in)
{
    #define PRECISION 8
	HI_U32 _res = 0;
	if(0 == data)
		return _res;

    data = data << PRECISION; // to ensure precision.
	for(;;)
	{
        /* Note to avoid endless loop here. */
		data = (data * 913) >> 10;
        // data = (data*913 + (1<<9)) >> 10; // endless loop when shift_in is 0. */
		if(data <= ((1<<shift_in) << PRECISION))
		{
			break;
		}
		++_res;
	}
	return _res;
}

static __inline HI_U32 analog_gain_from_exposure_calculate(
		cmos_gains_ptr_t p_gains,
		HI_U32 exposure,
		HI_U32 exposure_max,
		HI_U32 exposure_shift)
{
	HI_U32 _again = 1<<p_gains->again_shift;
	//HI_U32 _ares = 1<<p_gains->again_shift;
	//HI_U32 _lres = 0;
	int shft = 0;

	while (exposure > (1<<20))
	{
		exposure >>= 1;
		exposure_max >>= 1;
		++shft;
	}

	if(exposure > exposure_max)
	{
        //when setting manual exposure line, exposure_max>>shift should not be 0.
        exposure_max = DIV_0_TO_1(exposure_max);
		_again = (exposure  * _again)  / exposure_max;
//		exposure = exposure_max;

		if (_again >= 1<< 3) { _again = 1<<3; }
		else if (_again >= 1<< 2) { _again = 1<<2; }
		else if (_again >= 1<< 1) { _again = 1<<1; }
		else if (_again >= 1)     { _again = 1;    }

		_again = _again < (1<<p_gains->again_shift) ? (1<<p_gains->again_shift) : _again;
		_again = _again > p_gains->max_again_target ? p_gains->max_again_target : _again;

		exposure = (exposure / _again);
	}
	else
	{
		//_again = (_again * exposure) / (exposure / exposure_shift) / exposure_shift;
	}

	p_gains->again = _again;
    p_gains->again_db = cmos_gains_lin_to_db_convert(p_gains->again, p_gains->again_shift);
	return (exposure << shft);
}

static __inline HI_U32 digital_gain_from_exposure_calculate(
		cmos_gains_ptr_t p_gains,
		HI_U32 exposure,
		HI_U32 exposure_max,
		HI_U32 exposure_shift
		)
{
	HI_U32 _dgain = 1<<p_gains->dgain_shift;
	int shft = 0;

	while (exposure > (1<<20))
	{
		exposure >>= 1;
		exposure_max >>= 1;
		++shft;
	}

	if(exposure > exposure_max)
	{
	    //when setting manual exposure line, exposure_max>>shift should not be 0.
            exposure_max = DIV_0_TO_1(exposure_max);
            _dgain = (exposure  * _dgain) / exposure_max;
            exposure = exposure_max;
	}
	else
	{
        //TODO: after anti-flick, dgain may need to decrease.
		//_dgain = (_dgain * exposure) / (exposure / exposure_shift) / exposure_shift;
	}
	_dgain = _dgain < (1<<p_gains->dgain_shift) ? (1<<p_gains->dgain_shift) : _dgain;
	_dgain = _dgain > p_gains->max_dgain_target ? p_gains->max_dgain_target : _dgain;

	p_gains->dgain = _dgain;
    p_gains->dgain_db = cmos_gains_lin_to_db_convert(p_gains->dgain, p_gains->dgain_shift);

	return exposure << shft;
}

static __inline void sensor_update(
	cmos_gains_const_ptr_t p_gains,
	cmos_inttime_ptr_t p_inttime,
	HI_U8 frame
    )
{
	if(frame == 0)
	{
		cmos_inttime_update(p_inttime);
	}
	if(frame == 1)
	{
		cmos_gains_update(p_gains);
	}
}

static __inline HI_U32 cmos_get_ISO(cmos_gains_ptr_t p_gains)
{
	HI_U32 _again = p_gains->again == 0 ? 1 : p_gains->again;
	HI_U32 _dgain = p_gains->dgain == 0 ? 1 : p_gains->dgain;

	p_gains->iso =  ((_again * _dgain * 100) >> (p_gains->again_shift + p_gains->dgain_shift));

	return p_gains->iso;
}

static HI_U8 cmos_get_analog_gain(cmos_gains_ptr_t p_gains)
{
    //return cmos_gains_lin_to_db_convert(p_gains->again, p_gains->again_shift);
    return p_gains->again_db;
}

static HI_U8 cmos_get_digital_gain(cmos_gains_ptr_t p_gains)
{
    //return cmos_gains_lin_to_db_convert(p_gains->dgain, p_gains->dgain_shift);
    return p_gains->dgain_db;
}

#if 0
static HI_U8 cmos_get_digital_fine_gain(cmos_gains_ptr_t p_gains)
{
    return cmos_gains_lin_to_db_convert(p_gains->dgain_fine, p_gains->dgain_shift);
}
#endif

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

static HI_U32 cmos_get_isp_demosaic(cmos_isp_demosaic_ptr_t p_cmos_isp_demosaic)
{
   if (NULL == p_cmos_isp_demosaic)
   {
	    printf("null pointer when get isp demosaic value!\n");
	    return -1;
   }
   memcpy(p_cmos_isp_demosaic, &st_isp_demosaic,sizeof(cmos_isp_demosaic_t));
   return 0;

}
void setup_sensor(int isp_mode)
{
    if(0 == isp_mode) /* ISP 'normal' isp_mode */
    {
        sensor_write_register(0x300C, 0xCE4);	//30fps
    }
    else if(1 == isp_mode) /* ISP pixel calibration isp_mode */
    {
        sensor_write_register(0x300C, 0x4D58);	//5fps
        sensor_write_register(0x3012, 0x05DA);	//max exposure lines
        sensor_write_register(0x30B0, 0x1300);	//AG, Context A
        sensor_write_register(0x305E, 0x0020);	//DG, Context A
    }
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
	.pfn_cmos_get_isp_demosaic = cmos_get_isp_demosaic,
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



#endif // __AR0130_CMOS_H_