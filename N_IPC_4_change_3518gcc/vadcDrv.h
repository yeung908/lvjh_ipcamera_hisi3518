#ifndef __VADC_H_
#define __VADC_H_

#define QQVGA				1
#define QVGA				2
#define VGA					3
#define XXVGA				4
#define UXVGA				5


#define DC_SET_IMAGESIZE       0x01    /* imagesize£¬value_range*/

#define    DC_VAL_VGA          0x01    /* imagesize£¬VGA value*/
#define    DC_VAL_XXGA          0x03    /* imagesize£¬VGA value*/
#define    DC_VAL_UXGA         0x04   /* imagesize£¬UXGA value*/


#if 0
typedef unsigned char           HI_U8;
typedef unsigned short          HI_U16;
typedef unsigned int            HI_U32;

typedef signed char             HI_S8;
typedef short                   HI_S16;
typedef int                     HI_S32;
#endif

#define HI_NULL     0L
#define HI_SUCCESS  0
#define HI_FAILURE  (-1)


typedef enum 
{ 
    vadc_get_standard 			= 0,
    vadc_set_standard			= 1,   
    vadc_get_brightness			= 2,
    vadc_set_brightness			= 3,
    vadc_get_hue				= 4,
    vadc_set_hue				= 5,
    vadc_get_saturation			= 6,
    vadc_set_saturation			= 7,
    vadc_get_contrast			= 8,
    vadc_set_contrast			= 9,	 
    vadc_get_videostatus		= 10,

	vadc_set_full				= 11,
    vadc_set_quad				= 12,
    vadc_get_mode				= 13,
    vadc_set_mode				= 14,
    vadc_disable_channel		= 15,
    vadc_enable_channel			= 16,   
    
	vadc_set_osd				= 17, 	
	vadc_set_logo				= 18,
	
	vadc_motionarea_setup		= 19,
	vadc_motiondetect_setup		= 20,
	vadc_maskarea_setup			= 21,
	
	vadc_get_register			= 22,
	vadc_set_register			= 23,
	
	vadc_set_flip				= 24,
	vadc_set_mirror				= 25,
	vadc_set_hz					= 26,
	vadc_set_format				= 27,
	vadc_set_back				= 28,
	vadc_set_color 				= 29,
	

}vadcopt;

typedef struct
{
	unsigned int channel;
	unsigned int standard;
}tmVadc_standard;

typedef struct
{
	unsigned int channel;
	unsigned int contrast;
}tmVadc_contrast;

typedef struct
{
	unsigned int channel;
	unsigned int brightness;
}tmVadc_brightness;

typedef struct
{
	unsigned int channel;
	unsigned int saturation;
}tmVadc_saturation;

typedef struct
{
	unsigned int channel;
	unsigned int hue;
}tmVadc_hue;

typedef struct
{
	unsigned int channel;
	unsigned int status;
}tmVadc_status;

typedef struct
{
	unsigned int channel;
	unsigned int flip;
}tmVadc_flip;

typedef struct
{
	unsigned int channel;
	unsigned int mirror;
}tmVadc_mirror;

typedef struct
{
	unsigned int channel;
	unsigned int hz;
}tmVadc_hz;

typedef struct
{
	unsigned int channel;
	unsigned int format;
}tmVadc_format;


typedef struct
{
	unsigned int page;
	unsigned int address;
	unsigned int value;
	unsigned int lowValue;
}tmVadc_Register;


typedef struct tmvadc 
{    
	union 
	{
		tmVadc_standard			standard;
        tmVadc_contrast   		contrast;
        tmVadc_brightness   	brightness;
        tmVadc_saturation   	saturation;
        tmVadc_hue   			hue;
        tmVadc_Register			singleregister;
        tmVadc_status			status;
        tmVadc_flip				flip;
        tmVadc_mirror			mirror;
        tmVadc_hz				hz;
        tmVadc_format			format;

    }vadc;
    
    int opt;
}TMVADC;

int vadcDrv_Open(char *devName);
int vadcDrv_Close();

int vadcDrv_SetStandard(int channel, int val);
int vadcDrv_SetContrast(int channel, int val);
int vadcDrv_SetSaturation(int channel, int val);
int vadcDrv_SetHue(int channel, int val);
int vadcDrv_SetBack(void);
int vadcDrv_SetColor(void);
int vadcDrv_SetBrightness(int channel, int val);
int vadcDrv_GetStatus(int channel);

int vadcDrv_SetImageFlip(int channel, int flag);
int vadcDrv_SetImageMirror(int channel, int flag);
int vadcDrv_SetImageHz(int channel, int hz);

int videoLostDetectStart();
int videoLostDetectStop();
int videoLostDetectPause();
int videoLostDetectResume();
int getVideoStatus(int channel);
int  startIsp(void);
#endif
