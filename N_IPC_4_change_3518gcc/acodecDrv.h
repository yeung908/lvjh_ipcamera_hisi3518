#ifndef __ACODEC_H_
#define __ACODEC_H_

typedef enum 
{ 
    acodec_get_samplerate 			= 0,
    acodec_set_samplerate			= 1,   
    acodec_get_analoginputpath		= 2,
    acodec_set_analoginputpath		= 3,
    acodec_get_inputvalume			= 4,
    acodec_set_inputvalume			= 5,
    acodec_get_outputvalume			= 6,
    acodec_set_outputvalume			= 7,
	acodec_get_register				= 8,
	acodec_set_register				= 9,
    
}acodecopt;

typedef struct
{
	unsigned int channel;
	unsigned int samplerate;
}tmAcodec_samplerate;

typedef struct
{
	unsigned int channel;
	unsigned int analoginputpath;
}tmAcodec_analoginputpath;

typedef struct
{
	unsigned int channel;
	unsigned int inputvalume;
}tmAcodec_inputvalume;

typedef struct
{
	unsigned int channel;
	unsigned int outputvalume;
}tmAcodec_outputvalume;

typedef struct
{
	unsigned int page;
	unsigned int address;
	unsigned int value;
}tmAcodec_Register;

typedef struct
{    
	union 
	{
		tmAcodec_samplerate			samplerate;
		tmAcodec_analoginputpath   	analoginputpath;
		tmAcodec_inputvalume   		inputvalume;
		tmAcodec_outputvalume   	outputvalume;

		tmAcodec_Register			singleregister; 
		
    }acodec;
    
    int opt;

}TMACODEC;

int AudioCodecStart(void);
int acodecDrv_SetAnalogInputPath(int channel, int val);
int  SAMPLE_Tlv320_CfgAudio(void);


#endif
