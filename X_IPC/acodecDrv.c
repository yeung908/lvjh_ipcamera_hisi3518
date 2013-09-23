#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>

#include "acodecDrv.h"

#include "tlv320aic31.h"
#include "tlv320aic31_def.h"

static int acodec_fd = 0;
//----mody by lv---start
int  SAMPLE_Tlv320_CfgAudio(void)
{
    //HI_S32 ret;
    int vol = 0x135;
    Audio_Ctrl audio_ctrl;
    int s_fdTlv = -1;
	int s32Samplerate = 0;
    
    s_fdTlv = open("/dev/misc/tlv320aic31", O_RDWR);
    if (s_fdTlv < 0)
	{
		printf("Can not open /dev/misc/tlv320aic31(Error: %s)!!!\n", strerror(errno));
		return -1;
	}
	else
	{
		printf("Open /dev/misc/tlv320aic31 (OK)!!!\n");
	}

    if(s32Samplerate == 0)
    {
        s32Samplerate = AC31_SET_8K_SAMPLERATE;
    }
    else if(s32Samplerate == 1)
    {
        s32Samplerate = AC31_SET_16K_SAMPLERATE;
    }
    else if(s32Samplerate == 2)
    {
        s32Samplerate = AC31_SET_24K_SAMPLERATE;
    }
    else if(s32Samplerate == 3)
    {
        s32Samplerate = AC31_SET_32K_SAMPLERATE;
    }
    else if(s32Samplerate == 4)
    {
        s32Samplerate = AC31_SET_44_1K_SAMPLERATE;
    }
    else 
    {
        return -1;
    }

    /* set transfer mode 0:I2S 1:PCM */
    audio_ctrl.trans_mode = 1;
    if (ioctl(s_fdTlv,SET_TRANSFER_MODE,&audio_ctrl))
    {
        printf("set tlv320aic31 trans_mode err\n");
        return -1;
    }

    #if 1
    {
        /*set sample of DAC and ADC */
        audio_ctrl.sample = AC31_SET_16K_SAMPLERATE;
        if (ioctl(s_fdTlv,SET_DAC_SAMPLE,&audio_ctrl))
        {
            printf("ioctl err1\n");
            return -1;
        }
        
        if (ioctl(s_fdTlv,SET_ADC_SAMPLE,&audio_ctrl))
        {
            printf("ioctl err2\n");
            return -1;
        }   
        printf("set tlv320aic31 sample 0x%x\n",s32Samplerate);
    }
	#endif
        
    /*set volume control of left and right DAC */
    audio_ctrl.chip_num = 0;
    audio_ctrl.if_mute_route = 0;
    audio_ctrl.input_level = 0;
    ioctl(s_fdTlv,LEFT_DAC_VOL_CTRL,&audio_ctrl);
    ioctl(s_fdTlv,RIGHT_DAC_VOL_CTRL,&audio_ctrl);

    /*Right/Left DAC Datapath Control */
    audio_ctrl.if_powerup = 1;/*Left/Right DAC datapath plays left/right channel input data*/
    ioctl(s_fdTlv,LEFT_DAC_POWER_SETUP,&audio_ctrl);
    ioctl(s_fdTlv,RIGHT_DAC_POWER_SETUP,&audio_ctrl);
     
    audio_ctrl.ctrl_mode = 1;
    ioctl(s_fdTlv,SET_CTRL_MODE,&audio_ctrl); 
    printf("set tlv320aic31 master:%d\n",1);
    
    /* (0:16bit 1:20bit 2:24bit 3:32bit) */
    audio_ctrl.data_length = 1;
    ioctl(s_fdTlv,SET_DATA_LENGTH,&audio_ctrl);
  
    
    /*DACL1 TO LEFT_LOP/RIGHT_LOP VOLUME CONTROL 82 92*/
    audio_ctrl.if_mute_route = 1;/* route*/
    audio_ctrl.input_level = vol; /*level control*/
    ioctl(s_fdTlv,DACL1_2_LEFT_LOP_VOL_CTRL,&audio_ctrl);
    ioctl(s_fdTlv,DACR1_2_RIGHT_LOP_VOL_CTRL,&audio_ctrl);
    
#if 0
    if (HI_TRUE == bMaster)
    {
        /* DAC OUTPUT SWITCHING CONTROL 41*/
        audio_ctrl.dac_path = 0;
        ioctl(s_fdTlv,DAC_OUT_SWITCH_CTRL,&audio_ctrl);
    }
#endif

    /* LEFT_LOP/RIGHT_LOP OUTPUT LEVEL CONTROL 86 93*/
    audio_ctrl.if_mute_route = 1;
    audio_ctrl.if_powerup = 1;
    audio_ctrl.input_level = 0;
    ioctl(s_fdTlv,LEFT_LOP_OUTPUT_LEVEL_CTRL,&audio_ctrl);
    ioctl(s_fdTlv,RIGHT_LOP_OUTPUT_LEVEL_CTRL,&audio_ctrl);

    /* LEFT/RIGHT ADC PGA GAIN CONTROL 15 16*/    
    audio_ctrl.if_mute_route =0;    
    audio_ctrl.input_level = 0x76;    
    ioctl(s_fdTlv,LEFT_ADC_PGA_CTRL,&audio_ctrl);    
    ioctl(s_fdTlv,RIGHT_ADC_PGA_CTRL,&audio_ctrl);        
    /*INT2L TO LEFT/RIGTH ADCCONTROL 17 18*/    
    audio_ctrl.input_level = 0;    
    ioctl(s_fdTlv,IN2LR_2_LEFT_ADC_CTRL,&audio_ctrl);    
    ioctl(s_fdTlv,IN2LR_2_RIGTH_ADC_CTRL,&audio_ctrl);    
    /*IN1L_2_LEFT/RIGTH_ADC_CTRL 19 22*/    
    audio_ctrl.input_level = 0xf;  
    audio_ctrl.if_powerup = 1;    
    ioctl(s_fdTlv,IN1L_2_LEFT_ADC_CTRL,&audio_ctrl);    
    ioctl(s_fdTlv,IN1R_2_RIGHT_ADC_CTRL,&audio_ctrl); 

    close(s_fdTlv);
    return 0;
}
//----mody by lv---end

int SAMPLE_Tlv320_Set(int s32Samplerate)
{

    int vol = 0x180;
    Audio_Ctrl audio_ctrl;
    audio_ctrl.chip_num = 0;

	//printf("teset audio input \n");
	
    if(s32Samplerate == 0)
    {
        s32Samplerate = AC31_SET_8K_SAMPLERATE;
    }
    else if(s32Samplerate == 1)
    {
        s32Samplerate = AC31_SET_16K_SAMPLERATE;
    }
    else if(s32Samplerate == 2)
    {
        s32Samplerate = AC31_SET_24K_SAMPLERATE;
    }
    else if(s32Samplerate == 3)
    {
        s32Samplerate = AC31_SET_32K_SAMPLERATE;
    }
    else if(s32Samplerate == 4)
    {
        s32Samplerate = AC31_SET_44_1K_SAMPLERATE;
    }
    else 
    {
        return -1;
    }

	/* set transfer mode 0:I2S 1:PCM */
	//----mody by lv---start
    audio_ctrl.trans_mode = 0;
    if (ioctl(acodec_fd,SET_TRANSFER_MODE,&audio_ctrl))
    {
        printf("set tlv320aic31 trans_mode err\n");
        return -1;
    } 
    // 原来屏去的，现在新打开
	//----mody by lv---end
#if 1	//----mody by lv
	// Add the code by lvjh, 2009-01-20
    /*set sample of DAC and ADC */
	//    audio_ctrl.sample = AC31_SET_9K_SAMPLERATE;
//    audio_ctrl.sample = AC31_SET_12K_SAMPLERATE;
    audio_ctrl.sample = s32Samplerate;

    if (ioctl(acodec_fd,SET_DAC_SAMPLE,&audio_ctrl))
    {
        printf("ioctl err1\n");
        return -1;
    }
    
    if (ioctl(acodec_fd,SET_ADC_SAMPLE,&audio_ctrl))
    {
        printf("ioctl err2\n");
        return -1;
    }
#endif

    //set volume control of left and right DAC 
    audio_ctrl.chip_num = 0;
    audio_ctrl.if_mute_route = 0;
    audio_ctrl.input_level = 0;
    ioctl(acodec_fd,LEFT_DAC_VOL_CTRL,&audio_ctrl);
    ioctl(acodec_fd,RIGHT_DAC_VOL_CTRL,&audio_ctrl);

    // DAC POWER AND OUTPUT DRIVER CONTROL 7 37
    audio_ctrl.if_powerup = 1;
    ioctl(acodec_fd,LEFT_DAC_POWER_SETUP,&audio_ctrl);
    ioctl(acodec_fd,RIGHT_DAC_POWER_SETUP,&audio_ctrl);
     
	// 原来打开的，现在屏去
    // 设置主从模式
#if 0    	
    audio_ctrl.ctrl_mode = 1;/*0:master 1:slave*/
    ioctl(acodec_fd,SET_CTRL_MODE,&audio_ctrl); 
#else
    audio_ctrl.ctrl_mode = 0;/*0:master 1:slave*/
    ioctl(acodec_fd,SET_CTRL_MODE,&audio_ctrl); 
#endif
    
    // 数据位宽
    audio_ctrl.data_length = 1;       //mody by lv old value:0
    ioctl(acodec_fd,SET_DATA_LENGTH,&audio_ctrl);
  
    
    //DACL1 TO LEFT_LOP/RIGHT_LOP VOLUME CONTROL 82 92
    audio_ctrl.if_mute_route = 1;/* route*/
    //audio_ctrl.input_level = vol; /*level control*/
   // vol = 127;
	audio_ctrl.input_level = vol; /*level control*/
	
    ioctl(acodec_fd,DACL1_2_LEFT_LOP_VOL_CTRL,&audio_ctrl);
    ioctl(acodec_fd,DACR1_2_RIGHT_LOP_VOL_CTRL,&audio_ctrl);

#if 0
    // DAC OUTPUT SWITCHING CONTROL 41
    audio_ctrl.dac_path = 1;
    ioctl(acodec_fd,DAC_OUT_SWITCH_CTRL,&audio_ctrl);
#endif
    // LEFT_LOP/RIGHT_LOP OUTPUT LEVEL CONTROL 86 93
    audio_ctrl.if_mute_route = 1;
    audio_ctrl.if_powerup = 1;
    audio_ctrl.input_level = 0;
    ioctl(acodec_fd,LEFT_LOP_OUTPUT_LEVEL_CTRL,&audio_ctrl);
    ioctl(acodec_fd,RIGHT_LOP_OUTPUT_LEVEL_CTRL,&audio_ctrl);
	
    //配置AD
    // LEFT/RIGHT ADC PGA GAIN CONTROL 15 16   
     audio_ctrl.if_mute_route =0;    
//     audio_ctrl.input_level = 0x34;    
     audio_ctrl.input_level =0x76;//0x76;    //mody by lv old value:0
     ioctl(acodec_fd,LEFT_ADC_PGA_CTRL,&audio_ctrl);    
     ioctl(acodec_fd,RIGHT_ADC_PGA_CTRL,&audio_ctrl);  


    //INT2L TO LEFT/RIGTH ADCCONTROL 17 18    
	audio_ctrl.input_level = 0x00;    
	//audio_ctrl.input_level = 0x0F;    	// Add the code by lvjh, 2009-07-29
    ioctl(acodec_fd,IN2LR_2_LEFT_ADC_CTRL,&audio_ctrl);    
    ioctl(acodec_fd,IN2LR_2_RIGTH_ADC_CTRL,&audio_ctrl);    
    //IN1L_2_LEFT/RIGTH_ADC_CTRL 19 22    
	//audio_ctrl.input_level = 0xf;
	//audio_ctrl.input_level = 0x80;		// Add the code by lvjh, 2009-07-29
    audio_ctrl.reserved = 0; 			// Add the code by lvjh, 2009-07-29//mody by lv old nothing
    audio_ctrl.input_level = 0xf;//0x0f;               //mody by lv 
    audio_ctrl.if_powerup = 1;    

    ioctl(acodec_fd,IN1L_2_LEFT_ADC_CTRL,&audio_ctrl);    
    ioctl(acodec_fd,IN1R_2_RIGHT_ADC_CTRL,&audio_ctrl);
	//printf("////////////////////\n");
	//sleep(2);	
	return 0;
}

int acodecDrv_Open(char *devName)
{
	char name[64];
	
	if (devName == NULL)
	{
		return -1;
	}

	sprintf(name, "/dev/misc/%s", devName);	    
    acodec_fd = open(name, O_RDWR);
    if (acodec_fd < 0)
	{
		printf("Can not open %s(Error: %s)!!!\n", name, strerror(errno));
		return -1;
	}
	else
	{
		printf("Open %s(OK)!!!\n", name);
	}


	SAMPLE_Tlv320_Set(3);              //mody by lv old value:0
	
	//printf("Open %s OK!!!\n", name);
	
    return 0;
}

int acodecDrv_Close()
{
	close(acodec_fd);
	acodec_fd = -1;
	
	return 0;	
}

int acodecDrv_SetAnalogInputPath(int channel, int val)
{
	int	ret = -1;
	Audio_Ctrl audio_ctrl;

	//printf("<<<<<<<<<<<<<<<<<<<<<<>>>>>>>>>\n");
	//printf("acodecDrv_SetAnalogInputPath: %d\n", val);
	//sleep(10);
	
	if (val)
	{
		//配置AD
		// LEFT/RIGHT ADC PGA GAIN CONTROL 15 16   
		audio_ctrl.if_mute_route =0;    
		audio_ctrl.input_level =0x76;        //mody by lv old value:0
		ioctl(acodec_fd,LEFT_ADC_PGA_CTRL,&audio_ctrl);    
		ioctl(acodec_fd,RIGHT_ADC_PGA_CTRL,&audio_ctrl);        
		//INT2L TO LEFT/RIGTH ADCCONTROL 17 18    
		//audio_ctrl.input_level = 0;    
		audio_ctrl.input_level = 0x0F;    	// Add the code by lvjh, 2009-07-29
		ioctl(acodec_fd,IN2LR_2_LEFT_ADC_CTRL,&audio_ctrl);    
		ioctl(acodec_fd,IN2LR_2_RIGTH_ADC_CTRL,&audio_ctrl);    
		//IN1L_2_LEFT/RIGTH_ADC_CTRL 19 22    
		//audio_ctrl.input_level = 0xf;
		audio_ctrl.input_level = 0x80;		// Add the code by lvjh, 2009-07-29
		audio_ctrl.reserved = 1; 			// Add the code by lvjh, 2009-07-29
    
		audio_ctrl.if_powerup = 1;    
		ioctl(acodec_fd,IN1L_2_LEFT_ADC_CTRL,&audio_ctrl);    
		ioctl(acodec_fd,IN1R_2_RIGHT_ADC_CTRL,&audio_ctrl); 
	}
	else
	{
		//配置AD
		// LEFT/RIGHT ADC PGA GAIN CONTROL 15 16   
		audio_ctrl.if_mute_route =0;    
		audio_ctrl.input_level = 0x76;    //mody by lv old value:0
		ioctl(acodec_fd,LEFT_ADC_PGA_CTRL,&audio_ctrl);    
		ioctl(acodec_fd,RIGHT_ADC_PGA_CTRL,&audio_ctrl);        
		//INT2L TO LEFT/RIGTH ADCCONTROL 17 18    
		audio_ctrl.input_level = 0x00;  
		//audio_ctrl.input_level = 0x2B;  
		//audio_ctrl.input_level = 0x0F;    	// Add the code by lvjh, 2009-07-29
		ioctl(acodec_fd,IN2LR_2_LEFT_ADC_CTRL,&audio_ctrl);    
		ioctl(acodec_fd,IN2LR_2_RIGTH_ADC_CTRL,&audio_ctrl);    
		//IN1L_2_LEFT/RIGTH_ADC_CTRL 19 22    
		//audio_ctrl.input_level = 0x2B;
		//audio_ctrl.input_level = 0x80;		// Add the code by lvjh, 2009-07-29
		audio_ctrl.input_level = 0x0F;
		//audio_ctrl.input_level = 0x47;
	 	//audio_ctrl.reserved = 1; 			// Add the code by lvjh, 2009-07-29
    	audio_ctrl.if_powerup = 1;  
		audio_ctrl.reserved = 1;             //mody by lv old value:0
		ioctl(acodec_fd,IN1L_2_LEFT_ADC_CTRL,&audio_ctrl);    

//		audio_ctrl.input_level = 0x40;
		ioctl(acodec_fd,IN1R_2_RIGHT_ADC_CTRL,&audio_ctrl); 

		//----mody by lv---start		
		#if 0
			//��Ƶ������ƣ� ȥ����
		    audio_ctrl.input_level = 0x8F;
		    ioctl(acodec_fd,AGC_L_CTL,&audio_ctrl);    
		    ioctl(acodec_fd,AGC_R_CTL,&audio_ctrl); 

			audio_ctrl.input_level = 0x38;
		    ioctl(acodec_fd,AGC_L_GAIN,&audio_ctrl);    
		    ioctl(acodec_fd,AGC_R_GAIN,&audio_ctrl); 

			audio_ctrl.input_level = 0x7F;
		    ioctl(acodec_fd,AGC_L_NOISE,&audio_ctrl);    
		    ioctl(acodec_fd,AGC_R_NOISE,&audio_ctrl); 

			audio_ctrl.input_level = 0x9A;
		    ioctl(acodec_fd,AGC_L_MAX,&audio_ctrl);    
		    ioctl(acodec_fd,AGC_R_MAX,&audio_ctrl); 
			
		#endif

		#if 0
		audio_ctrl.input_level = 0X2B;    
		//audio_ctrl.input_level = 0x0F;    	// Add the code by lvjh, 2009-07-29
		ioctl(acodec_fd,LEFT_DAC_VOL_CTRL,&audio_ctrl);    
		ioctl(acodec_fd,RIGHT_DAC_VOL_CTRL,&audio_ctrl);
		printf("audio control test>>>>>>>>\n");
		

		audio_ctrl.input_level = 0X2B;    
		//audio_ctrl.input_level = 0x0F;    	// Add the code by lvjh, 2009-07-29
		ioctl(acodec_fd,PGAL_2_HPLOUT_VOL_CTRL,&audio_ctrl);    
		ioctl(acodec_fd,DACL1_2_HPLOUT_VOL_CTRL,&audio_ctrl);
		printf("audio control test>>>>>>>>\n");

		audio_ctrl.input_level = 0X49;    
		//audio_ctrl.input_level = 0x0F;    	// Add the code by lvjh, 2009-07-29
		ioctl(acodec_fd,HPLOUT_OUTPUT_LEVEL_CTRL,&audio_ctrl); 
		#endif
	 
	}
	acodecDrv_Close();
	//----mody by lv---end
	return 0;
	
	/*
	TMACODEC tmacodec;

	if (acodec_fd > 0)
	{
		tmacodec.opt = acodec_set_analoginputpath;
		tmacodec.acodec.analoginputpath.channel = channel;
		tmacodec.acodec.analoginputpath.analoginputpath = val;
	
		ret = write(acodec_fd, (void *)&tmacodec, sizeof(TMACODEC));

		if (ret == -1)
		{
			return -1;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return -1;
	}
	*/
}

int AudioCodecStart(void)
{
	acodecDrv_Open("tlv320aic31");

	return 0;
}

