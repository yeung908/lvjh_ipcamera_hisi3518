//------------------------------------------------------------------------------
// File Name: siHdmiTx_902x_TPI.h
// File Description: this file is a header file.
//
// Create Date: 07/22/2010 
//
// Modification history:
//			 - 07/22/2010  Add file desription and comment
//
//
// Copyright © 2002-2010, Silicon Image, Inc.  All rights reserved.
//
// No part of this work may be reproduced, modified, distributed, transmitted,
// transcribed, or translated into any language or computer format, in any form
// or by any means without written permission of: Silicon Image, Inc.,
// 1060 East Arques Avenue, Sunnyvale, California 94085
//------------------------------------------------------------------------------
//#include <stdio.h>

#ifndef _SIHDMITX_902X_TPI_H_
#define _SIHDMITX_902X_TPI_H_

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

//--------------------------------------------------------------------
// typedef
//--------------------------------------------------------------------
typedef unsigned char		byte;
//typedef unsigned short		word;
//typedef unsigned long		dword;

//--------------------------------------------------------------------
// System Macro definition
//--------------------------------------------------------------------
#define DEV_SUPPORT_EDID
//#define DEV_SUPPORT_HDCP
//#define DEV_SUPPORT_CEC
//#define DEV_SUPPORT_3D

#define CLOCK_EDGE_RISING
//#define CLOCK_EDGE_FALLING

#define F_9022A_9334
//#define HW_INT_ENABLE
//--------------------------------------------------------------------
// typedef
//--------------------------------------------------------------------
//typedef unsigned char		byte;
typedef unsigned short		word;
typedef unsigned long		dword;

#define FALSE                0
#define TRUE                  1

#define OFF                    0
#define ON                      1

#define SET 1
#define CLR 0
// I2C Slave Addresses
//====================================================
#define TX_SLAVE_ADDR       	0x72
#define CBUS_SLAVE_ADDR     	0xC8
#define HDCP_SLAVE_ADDR     	0x74
#define EDID_ROM_ADDR       	0xA0
#define EDID_SEG_ADDR	    	0x60

#define WRITE_FLGA  0x00
#define READ_FLGA   0x01

#define IIC_CAPTURED  1
#define IIC_NOACK     2
#define IIC_OK 0

//--------------------------------------------------------------------
// TPI Firmware Version
//--------------------------------------------------------------------
static const char TPI_FW_VERSION[] = "TPI Firmware v6.6.3_APP v1.3";

// Generic Constants
//====================================================
#define FALSE                0
#define TRUE                  1

#define OFF                    0
#define ON                      1

#define LOW                   0
#define HIGH                  1

#define DISABLE 	0x00
#define ENABLE  	0xFF


#define MAX_V_DESCRIPTORS				20
#define MAX_A_DESCRIPTORS				10
#define MAX_SPEAKER_CONFIGURATIONS	4
#define AUDIO_DESCR_SIZE			 	3

#define RGB					0
#define YCBCR444			1
#define YCBCR422_16BITS		2
#define YCBCR422_8BITS		3
#define XVYCC444			4

#define EXTERNAL_HSVSDE	0
#define INTERNAL_DE			1
#define EMBEDDED_SYNC		2

#define COLORIMETRY_601	0
#define COLORIMETRY_709	1

//====================================================
#define MCLK128FS 			0
#define MCLK256FS 			1
#define MCLK384FS 			2
#define MCLK512FS 			3
#define MCLK768FS 			4
#define MCLK1024FS 			5
#define MCLK1152FS 			6
#define MCLK192FS 			7

#define SCK_SAMPLE_FALLING_EDGE 	0x00
#define SCK_SAMPLE_RISING_EDGE 	0x80
//====================================================
// CMD
#define HDMI_CMD_SETMODE 1

//====================================================
// Video mode define
#define HDMI_480I60_4X3	1
#define HDMI_576I50_4X3	2
#define HDMI_480P60_4X3	3
#define HDMI_576P50_4X3	4
#define HDMI_720P60			5
#define HDMI_720P50			6
#define HDMI_1080I60		7
#define HDMI_1080I50		8
#define HDMI_1080P60		9
#define HDMI_1080P50		10
//zhy+ Begin
#define HDMI_1080P25		11
#define HDMI_1080P30		12
#define HDMI_1080P24		13


//zhy+ end
//====================================================
#define AMODE_I2S 			0
#define AMODE_SPDIF 		1
#define AMODE_HBR 			2
#define AMODE_DSD 			3

#define ACHANNEL_2CH		1
#define ACHANNEL_3CH 		2
#define ACHANNEL_4CH 		3
#define ACHANNEL_5CH 		4
#define ACHANNEL_6CH 		5
#define ACHANNEL_7CH 		6
#define ACHANNEL_8CH 		7

#define AFS_44K1			0x00
#define AFS_48K				0x02
#define AFS_32K				0x03
#define AFS_88K2			0x08
#define AFS_768K			0x09
#define AFS_96K				0x0a
#define AFS_176K4			0x0c
#define AFS_192K			0x0e

#define ALENGTH_16BITS		0x02
#define ALENGTH_17BITS		0x0c
#define ALENGTH_18BITS		0x04
#define ALENGTH_19BITS		0x08
#define ALENGTH_20BITS		0x0a
#define ALENGTH_21BITS		0x0d
#define ALENGTH_22BITS		0x05
#define ALENGTH_23BITS		0x09
#define ALENGTH_24BITS		0x0b

//====================================================
typedef struct
{
    	byte HDMIVideoFormat;	// 0 = CEA-861 VIC; 1 = HDMI_VIC; 2 = 3D
    	byte VIC;				// VIC or the HDMI_VIC
    	byte AspectRatio;			// 4x3 or 16x9
    	byte ColorSpace;			// 0 = RGB; 1 = YCbCr4:4:4; 2 = YCbCr4:2:2_16bits; 3 = YCbCr4:2:2_8bits; 4 = xvYCC4:4:4
    	byte ColorDepth;			// 0 = 8bits; 1 = 10bits; 2 = 12bits
    	byte Colorimetry;			// 0 = 601; 1 = 709
	byte SyncMode;			// 0 = external HS/VS/DE; 1 = external HS/VS and internal DE; 2 = embedded sync  
	byte TclkSel;				// 0 = x0.5CLK; 1 = x1CLK; 2 = x2CLK; 3 = x4CLK
    	byte ThreeDStructure;	// Valid when (HDMIVideoFormat == VMD_HDMIFORMAT_3D)
	byte ThreeDExtData;		// Valid when (HDMIVideoFormat == VMD_HDMIFORMAT_3D) && (ThreeDStructure == VMD_3D_SIDEBYSIDEHALF)

	byte AudioMode;			// 0 = I2S; 1 = S/PDIF; 2 = HBR; 3 = DSD;
	byte AudioChannels;		// 1 = 2chs; 2 = 3chs; 3 = 4chs; 4 = 5chs; 5 = 6chs; 6 = 7chs; 7 = 8chs;
	byte AudioFs;			// 0-44.1kHz; 2-48kHz; 3-32kHz; 8-88.2kHz; 9-768kHz; A-96kHz; C-176.4kHz; E-192kHz; 1/4/5/6/7/B/D/F-not indicated
     	byte AudioWordLength; 	// 0/1-not available; 2-16 bit; 4-18 bit; 8-19 bit; A-20 bit; C-17 bit; 5-22 bit; 9-23 bit; B-24 bit; D-21 bit
     	byte AudioI2SFormat;   	// Please refer to TPI reg0x20 for detailed. 
     						 	//[7]_SCK Sample Edge: 0 = Falling; 1 = Rising
     						 	//[6:4]_MCLK Multiplier: 000:MCLK=128Fs; 001:MCLK=256Fs; 010:MCLK=384Fs; 011:MCLK=512Fs; 100:MCLK=768Fs; 101:MCLK=1024Fs; 110:MCLK=1152Fs; 111:MCLK=192Fs;   
     						 	//[3]_WS Polarity-Left when: 0 = WS is low when Left; 1 = WS is high when Left
     						 	//[2]_SD Justify Data is justified: 0 = Left; 1 = Right
     						 	//[1]_SD Direction Byte shifted first: 0 = MSB; 1 = LSB
     						 	//[0]_WS to SD First Bit Shift: 0 = Yes; 1 = No

}SIHDMITX_CONFIG;

//====================================================
typedef struct
{
	byte txPowerState;
	byte tmdsPoweredUp;
	byte hdmiCableConnected;
	byte dsRxPoweredUp;
	
}GLOBAL_SYSTEM;

//====================================================
typedef struct
{
	byte HDCP_TxSupports;
	byte HDCP_AksvValid;
	byte HDCP_Started;
	byte HDCP_LinkProtectionLevel;
	byte HDCP_Override;
	byte HDCPAuthenticated;
	
}GLOBAL_HDCP;

//====================================================
typedef struct
{												// for storing EDID parsed data
	byte edidDataValid;
	byte VideoDescriptor[MAX_V_DESCRIPTORS];	// maximum number of video descriptors
	byte AudioDescriptor[MAX_A_DESCRIPTORS][3];	// maximum number of audio descriptors
	byte SpkrAlloc[MAX_SPEAKER_CONFIGURATIONS];	// maximum number of speaker configurations
	byte UnderScan;								// "1" if DTV monitor underscans IT video formats by default
	byte BasicAudio;								// Sink supports Basic Audio
	byte YCbCr_4_4_4;							// Sink supports YCbCr 4:4:4
	byte YCbCr_4_2_2;							// Sink supports YCbCr 4:2:2
	byte HDMI_Sink;								// "1" if HDMI signature found
	byte CEC_A_B;								// CEC Physical address. See HDMI 1.3 Table 8-6
	byte CEC_C_D;
	byte ColorimetrySupportFlags;				// IEC 61966-2-4 colorimetry support: 1 - xvYCC601; 2 - xvYCC709 
	byte MetadataProfile;
	byte _3D_Supported;
	
} GLOBAL_EDID;

enum EDID_ErrorCodes
{
	EDID_OK,
	EDID_INCORRECT_HEADER,
	EDID_CHECKSUM_ERROR,
	EDID_NO_861_EXTENSIONS,
	EDID_SHORT_DESCRIPTORS_OK,
	EDID_LONG_DESCRIPTORS_OK,
	EDID_EXT_TAG_ERROR,
	EDID_REV_ADDR_ERROR,
	EDID_V_DESCR_OVERFLOW,
	EDID_UNKNOWN_TAG_CODE,
	EDID_NO_DETAILED_DESCRIPTORS,
	EDID_DDC_BUS_REQ_FAILURE,
	EDID_DDC_BUS_RELEASE_FAILURE
};


#ifdef DEV_SUPPORT_EDID
#define IsHDMI_Sink()		(g_edid.HDMI_Sink)
#define IsCEC_DEVICE()		(((g_edid.CEC_A_B != 0xFF) && (g_edid.CEC_C_D != 0xFF)) ? TRUE : FALSE)

#else
#define IsHDMI_Sink()		(FALSE)
#define IsCEC_DEVICE()		(FALSE)
#endif



//--------------------------------------------------------------------
// Debug Definitions
//--------------------------------------------------------------------
// Compile debug prints inline or not
#define CONF__TPI_TRACE_PRINT		(DISABLE)
#define CONF__TPI_DEBUG_PRINT   	(DISABLE)
#define CONF__TPI_EDID_PRINT    	(DISABLE)	//(DISABLE)
#define CONF__CPI_DEBUG_PRINT   	(DISABLE)


// Trace Print Macro
// Note: TPI_TRACE_PRINT Requires double parenthesis
// Example:  TPI_TRACE_PRINT(("hello, world!\n"));
#if (CONF__TPI_TRACE_PRINT == ENABLE)
   // #define TPI_TRACE_PRINT(x)	do {printk(KERN_INFO "%s [%s ,%d]: " fmt "\n",DEV_NAME,__FUNCTION__,__LINE__,##args);} while(0)
    #define TPI_TRACE_PRINT(x)	do{printk x;}while(0)//do {printk(KERN_INFO "%s [%s ,%d]: %s\n",DEV_NAME,__FUNCTION__,__LINE__,(x));} while(0)
#else
    #define TPI_TRACE_PRINT(x)
#endif

// Debug Print Macro
// Note: TPI_DEBUG_PRINT Requires double parenthesis
// Example:  TPI_DEBUG_PRINT(("hello, world!\n"));
#if (CONF__TPI_DEBUG_PRINT == ENABLE)
    //#define TPI_DEBUG_PRINT(fmt,args...)	do {printk(KERN_INFO "%s [%s ,%d]: " fmt "\n",DEV_NAME,__FUNCTION__,__LINE__,args);} while(0)
    #define TPI_DEBUG_PRINT(x)	do{printk x;}while(0)//TPI_TRACE_PRINT(fmt)
#else
    #define TPI_DEBUG_PRINT(x)
#endif
// EDID Print Macro
// Note: To enable EDID description printing, both CONF__TPI_EDID_PRINT and CONF__TPI_DEBUG_PRINT must be enabled
// Note: TPI_EDID_PRINT Requires double parenthesis
// Example:  TPI_EDID_PRINT(("hello, world!\n"));
#if (CONF__TPI_EDID_PRINT == ENABLE)
    #define TPI_EDID_PRINT(x)		do{printk x;}while(0)//TPI_DEBUG_PRINT(x)
#else
    #define TPI_EDID_PRINT(x)
#endif

// CPI Debug Print Macro
// Note: To enable CPI description printing, both CONF__CPI_DEBUG_PRINT and CONF__TPI_DEBUG_PRINT must be enabled
// Note: CPI_DEBUG_PRINT Requires double parenthesis
// Example:  CPI_DEBUG_PRINT(("hello, world!\n"));
#if (CONF__CPI_DEBUG_PRINT == ENABLE)
    #define CPI_DEBUG_PRINT(x)	TPI_DEBUG_PRINT(x)
#else
    #define CPI_DEBUG_PRINT(x)
#endif



enum AV_ConfigErrorCodes
{
    DE_CANNOT_BE_SET_WITH_EMBEDDED_SYNC,
    V_MODE_NOT_SUPPORTED,
    SET_EMBEDDED_SYC_FAILURE,
    I2S_MAPPING_SUCCESSFUL,
    I2S_INPUT_CONFIG_SUCCESSFUL,
    I2S_HEADER_SET_SUCCESSFUL,
    EHDMI_ARC_SINGLE_SET_SUCCESSFUL,
    EHDMI_ARC_COMMON_SET_SUCCESSFUL,
    EHDMI_HEC_SET_SUCCESSFUL,
    EHDMI_ARC_CM_WITH_HEC_SET_SUCCESSFUL,
    AUD_MODE_NOT_SUPPORTED,
    I2S_NOT_SET,
    DE_SET_OK,
    VIDEO_MODE_SET_OK,
    AUDIO_MODE_SET_OK,
    GBD_SET_SUCCESSFULLY,
    DE_CANNOT_BE_SET_WITH_3D_MODE,
};


#define ClearInterrupt(x)       WriteByteTPI(TPI_INTERRUPT_STATUS_REG, x)   // write "1" to clear interrupt bit

// Generic Masks
//====================================================
#define LOW_BYTE      0x00FF

#define LOW_NIBBLE	0x0F
#define HI_NIBBLE  	0xF0

#define MSBIT       	0x80
#define LSBIT          	0x01

#define BIT_0                   0x01
#define BIT_1                   0x02
#define BIT_2                   0x04
#define BIT_3                   0x08
#define BIT_4                   0x10
#define BIT_5                   0x20
#define BIT_6                   0x40
#define BIT_7                   0x80

#define TWO_LSBITS        	0x03
#define THREE_LSBITS   	0x07
#define FOUR_LSBITS    	0x0F
#define FIVE_LSBITS    	0x1F
#define SEVEN_LSBITS    	0x7F
#define TWO_MSBITS     	0xC0
#define EIGHT_BITS      	0xFF
#define BYTE_SIZE        	0x08
#define BITS_1_0          	0x03
#define BITS_2_1          	0x06
#define BITS_2_1_0        	0x07
#define BITS_3_2              	0x0C
#define BITS_4_3_2       	0x1C  
#define BITS_5_4              	0x30
#define BITS_5_4_3		0x38
#define BITS_6_5             	0x60
#define BITS_6_5_4        	0x70
#define BITS_7_6            	0xC0

#define TPI_INTERNAL_PAGE_REG		0xBC
#define TPI_INDEXED_OFFSET_REG	0xBD
#define TPI_INDEXED_VALUE_REG		0xBE

// Interrupt Masks
//====================================================
#define HOT_PLUG_EVENT          0x01
#define RX_SENSE_EVENT          0x02
#define HOT_PLUG_STATE          0x04
#define RX_SENSE_STATE          0x08

#define AUDIO_ERROR_EVENT       0x10
#define SECURITY_CHANGE_EVENT   0x20
#define V_READY_EVENT           0x40
#define HDCP_CHANGE_EVENT       0x80

#define NON_MASKABLE_INT		0xFF

// TPI Control Masks
//====================================================

#define CS_HDMI_RGB         0x00
#define CS_DVI_RGB          0x03

#define ENABLE_AND_REPEAT	0xC0
#define EN_AND_RPT_MPEG	0xC3
#define DISABLE_MPEG		0x03	// Also Vendor Specific InfoFrames

// Pixel Repetition Masks
//====================================================
#define BIT_BUS_24          0x20
#define BIT_EDGE_RISE       0x10

//Audio Maps
//====================================================
#define BIT_AUDIO_MUTE      0x10

// Input/Output Format Masks
//====================================================
#define BITS_IN_RGB         0x00
#define BITS_IN_YCBCR444    0x01
#define BITS_IN_YCBCR422    0x02

#define BITS_IN_AUTO_RANGE  0x00
#define BITS_IN_FULL_RANGE  0x04
#define BITS_IN_LTD_RANGE   0x08

#define BIT_EN_DITHER_10_8  0x40
#define BIT_EXTENDED_MODE   0x80

#define BITS_OUT_RGB        0x00
#define BITS_OUT_YCBCR444   0x01
#define BITS_OUT_YCBCR422   0x02

#define BITS_OUT_AUTO_RANGE 0x00
#define BITS_OUT_FULL_RANGE 0x04
#define BITS_OUT_LTD_RANGE  0x08

#define BIT_BT_709          0x10


// DE Generator Masks
//====================================================
#define BIT_EN_DE_GEN       0x40
#define DE 					0x00
#define DeDataNumBytes 		12

// Embedded Sync Masks
//====================================================
#define BIT_EN_SYNC_EXTRACT 0x40
#define EMB                 0x80
#define EmbDataNumBytes     8


// Audio Modes
//====================================================
#define AUD_PASS_BASIC      0x00
#define AUD_PASS_ALL        0x01
#define AUD_DOWN_SAMPLE     0x02
#define AUD_DO_NOT_CHECK    0x03

#define REFER_TO_STREAM_HDR     0x00
#define TWO_CHANNELS            	0x00
#define EIGHT_CHANNELS          	0x01
#define AUD_IF_SPDIF            		0x40
#define AUD_IF_I2S              		0x80
#define AUD_IF_DSD				0xC0
#define AUD_IF_HBR				0x04

#define TWO_CHANNEL_LAYOUT      0x00
#define EIGHT_CHANNEL_LAYOUT    0x20

#if 0
// I2C Slave Addresses
//====================================================
#define TX_SLAVE_ADDR       	0x72
#define CBUS_SLAVE_ADDR     	0xC8
#define HDCP_SLAVE_ADDR     	0x74
#define EDID_ROM_ADDR       	0xA0
#define EDID_SEG_ADDR	    	0x60
#endif

// Indexed Register Offsets, Constants
//====================================================
#define INDEXED_PAGE_0		0x01
#define INDEXED_PAGE_1		0x02
#define INDEXED_PAGE_2		0x03

#define TMDS_CONT_REG          0x82

// DDC Bus Addresses
//====================================================
#define DDC_BSTATUS_ADDR_L  0x41
#define DDC_BSTATUS_ADDR_H  0x42
#define DDC_KSV_FIFO_ADDR   0x43
#define KSV_ARRAY_SIZE      128

// DDC Bus Bit Masks
//====================================================
#define BIT_DDC_HDMI        0x80
#define BIT_DDC_REPEATER    0x40
#define BIT_DDC_FIFO_RDY    0x20
#define DEVICE_COUNT_MASK   0x7F

// KSV Buffer Size
//====================================================
#define DEVICE_COUNT         128    // May be tweaked as needed

// InfoFrames
//====================================================
#define SIZE_AVI_INFOFRAME      0x0E     // including checksum byte
#define BITS_OUT_FORMAT         0x60    // Y1Y0 field

#define _4_To_3                 0x10    // Aspect ratio - 4:3  in InfoFrame DByte 1
#define _16_To_9                0x20    // Aspect ratio - 16:9 in InfoFrame DByte 1
#define SAME_AS_AR              0x08    // R3R2R1R0 - in AVI InfoFrame DByte 2

#define BT_601                  0x40
#define BT_709                  0x80

//#define EN_AUDIO_INFOFRAMES         0xC2
#define TYPE_AUDIO_INFOFRAMES       0x84
#define AUDIO_INFOFRAMES_VERSION    0x01
#define AUDIO_INFOFRAMES_LENGTH     0x0A

#define TYPE_GBD_INFOFRAME       	0x0A

#define ENABLE_AND_REPEAT			0xC0

#define EN_AND_RPT_MPEG				0xC3
#define DISABLE_MPEG				0x03	// Also Vendor Specific InfoFrames

#define EN_AND_RPT_AUDIO			0xC2
#define DISABLE_AUDIO				0x02

#define EN_AND_RPT_AVI				0xC0	// Not normally used.  Write to TPI 0x19 instead
#define DISABLE_AVI					0x00	// But this is used to Disable

#define NEXT_FIELD					0x80
#define GBD_PROFILE					0x00
#define AFFECTED_GAMUT_SEQ_NUM		0x01

#define ONLY_PACKET					0x30
#define CURRENT_GAMUT_SEQ_NUM		0x01

// FPLL Multipliers:
//====================================================

#define X0d5                      				0x00
#define X1                      					0x01
#define X2                      					0x02
#define X4                      					0x03

// 3D Constants
//====================================================

#define _3D_STRUC_PRESENT				0x02

// 3D_Stucture Constants
//====================================================
#define FRAME_PACKING					0x00
#define FIELD_ALTERNATIVE				0x01
#define LINE_ALTERNATIVE				0x02
#define SIDE_BY_SIDE_FULL				0x03
#define L_PLUS_DEPTH					0x04
#define L_PLUS_DEPTH_PLUS_GRAPHICS	0x05
#define SIDE_BY_SIDE_HALF				0x08

// 3D_Ext_Data Constants
//====================================================
#define HORIZ_ODD_LEFT_ODD_RIGHT		0x00
#define HORIZ_ODD_LEFT_EVEN_RIGHT		0x01
#define HORIZ_EVEN_LEFT_ODD_RIGHT  		0x02
#define HORIZ_EVEN_LEFT_EVEN_RIGHT		0x03

#define QUINCUNX_ODD_LEFT_EVEN_RIGHT	0x04
#define QUINCUNX_ODD_LEFT_ODD_RIGHT		0x05
#define QUINCUNX_EVEN_LEFT_ODD_RIGHT	0x06
#define QUINCUNX_EVEN_LEFT_EVEN_RIGHT	0x07

#define NO_3D_SUPPORT					0x0F

// InfoFrame Type Code
//====================================================
#define AVI  						0x00 
#define SPD  						0x01  
#define AUDIO 						0x02   
#define MPEG 						0x03 
#define GEN_1	 					0x04
#define GEN_2 						0x05  
#define HDMI_VISF 					0x06 
#define GBD 						0x07 

// Size of InfoFrame Data types
#define MAX_SIZE_INFOFRAME_DATA     0x22
#define SIZE_AVI_INFOFRAME			0x0E	// 14 bytes
#define SIZE_SPD_INFOFRAME 			0x19	// 25 bytes
#define SISE_AUDIO_INFOFRAME_IFORM  0x0A    // 10 bytes
#define SIZE_AUDIO_INFOFRAME		0x0F	// 15 bytes
#define SIZE_MPRG_HDMI_INFOFRAME    0x1B	// 27 bytes		
#define SIZE_MPEG_INFOFRAME 		0x0A	// 10 bytes
#define SIZE_GEN_1_INFOFRAME    	0x1F	// 31 bytes
#define SIZE_GEN_2_INFOFRAME    	0x1F	// 31 bytes
#define SIZE_HDMI_VISF_INFOFRAME    0x1E	// 31 bytes
#define SIZE_GBD_INFOFRAME     		0x1C  	// 28 bytes

#define AVI_INFOFRM_OFFSET          0x0C
#define OTHER_INFOFRM_OFFSET        0xC4
#define TPI_INFOFRAME_ACCESS_REG    0xBF

// Serial Communication Buffer constants
#define MAX_COMMAND_ARGUMENTS       50
#define GLOBAL_BYTE_BUF_BLOCK_SIZE  131


// Video Mode Constants
//====================================================
#define VMD_ASPECT_RATIO_4x3			0x01
#define VMD_ASPECT_RATIO_16x9			0x02

#define VMD_COLOR_SPACE_RGB			0x00
#define VMD_COLOR_SPACE_YCBCR422		0x01
#define VMD_COLOR_SPACE_YCBCR444		0x02

#define VMD_COLOR_DEPTH_8BIT			0x00
#define VMD_COLOR_DEPTH_10BIT			0x01
#define VMD_COLOR_DEPTH_12BIT			0x02
#define VMD_COLOR_DEPTH_16BIT			0x03

#define VMD_HDCP_NOT_AUTHENTICATED	0x00
#define VMD_HDCP_AUTHENTICATED		0x01

#define VMD_HDMIFORMAT_CEA_VIC          	0x00
#define VMD_HDMIFORMAT_HDMI_VIC         	0x01
#define VMD_HDMIFORMAT_3D               	0x02
#define VMD_HDMIFORMAT_PC               	0x03

// These values are from HDMI Spec 1.4 Table H-2
#define VMD_3D_FRAMEPACKING			0
#define VMD_3D_FIELDALTERNATIVE		1
#define VMD_3D_LINEALTERNATIVE		2              
#define VMD_3D_SIDEBYSIDEFULL			3
#define VMD_3D_LDEPTH					4
#define VMD_3D_LDEPTHGRAPHICS			5
#define VMD_3D_SIDEBYSIDEHALF			8


//--------------------------------------------------------------------
// System Macro Definitions
//--------------------------------------------------------------------
#define TX_HW_RESET_PERIOD      200
#define SII902XA_DEVICE_ID         0xB0

#define T_HPD_DELAY    			10

//--------------------------------------------------------------------
// HDCP Macro Definitions
//--------------------------------------------------------------------
#define AKSV_SIZE              		5
#define NUM_OF_ONES_IN_KSV	20

//--------------------------------------------------------------------
// EDID Constants Definition
//--------------------------------------------------------------------
#define EDID_BLOCK_0_OFFSET 0x00
#define EDID_BLOCK_1_OFFSET 0x80

#define EDID_BLOCK_SIZE      128
#define EDID_HDR_NO_OF_FF   0x06
#define NUM_OF_EXTEN_ADDR   0x7E

#define EDID_TAG_ADDR       0x00
#define EDID_REV_ADDR       0x01
#define EDID_TAG_IDX        0x02
#define LONG_DESCR_PTR_IDX  0x02
#define MISC_SUPPORT_IDX    0x03

#define ESTABLISHED_TIMING_INDEX        35      // Offset of Established Timing in EDID block
#define NUM_OF_STANDARD_TIMINGS          8
#define STANDARD_TIMING_OFFSET          38
#define LONG_DESCR_LEN                  18
#define NUM_OF_DETAILED_DESCRIPTORS      4

#define DETAILED_TIMING_OFFSET        0x36

// Offsets within a Long Descriptors Block
//====================================================
#define PIX_CLK_OFFSET                	 	0
#define H_ACTIVE_OFFSET               	2
#define H_BLANKING_OFFSET          	3
#define V_ACTIVE_OFFSET                  	5
#define V_BLANKING_OFFSET                6
#define H_SYNC_OFFSET                    	8
#define H_SYNC_PW_OFFSET                 9
#define V_SYNC_OFFSET                   	10
#define V_SYNC_PW_OFFSET                	10
#define H_IMAGE_SIZE_OFFSET            	12
#define V_IMAGE_SIZE_OFFSET           	13
#define H_BORDER_OFFSET                 	15
#define V_BORDER_OFFSET                 	16
#define FLAGS_OFFSET                    	17

#define AR16_10                          		0
#define AR4_3                            		1
#define AR5_4                            		2
#define AR16_9                           		3

// Data Block Tag Codes
//====================================================
#define AUDIO_D_BLOCK       0x01
#define VIDEO_D_BLOCK       0x02
#define VENDOR_SPEC_D_BLOCK 0x03
#define SPKR_ALLOC_D_BLOCK  0x04
#define USE_EXTENDED_TAG    0x07

// Extended Data Block Tag Codes
//====================================================
#define COLORIMETRY_D_BLOCK 0x05

#define HDMI_SIGNATURE_LEN  0x03

#define CEC_PHYS_ADDR_LEN   0x02
#define EDID_EXTENSION_TAG  0x02
#define EDID_REV_THREE      0x03
#define EDID_DATA_START     0x04

#define EDID_BLOCK_0        0x00
#define EDID_BLOCK_2_3      0x01

#define VIDEO_CAPABILITY_D_BLOCK 0x00





//--------------------------------------------------------------------
// TPI Register Definition
//--------------------------------------------------------------------

// TPI Video Mode Data
#define TPI_PIX_CLK_LSB							(0x00)
#define TPI_PIX_CLK_MSB							(0x01)
#define TPI_VERT_FREQ_LSB						(0x02)
#define TPI_VERT_FREQ_MSB						(0x03)
#define TPI_TOTAL_PIX_LSB						(0x04)
#define TPI_TOTAL_PIX_MSB						(0x05)
#define TPI_TOTAL_LINES_LSB					(0x06)
#define TPI_TOTAL_LINES_MSB					(0x07)

// Pixel Repetition Data
#define TPI_PIX_REPETITION						(0x08)

// TPI AVI Input and Output Format Data
/// AVI Input Format Data
#define TPI_INPUT_FORMAT_REG					(0x09)
#define INPUT_COLOR_SPACE_MASK				(BIT_1 | BIT_0)
#define INPUT_COLOR_SPACE_RGB					(0x00)
#define INPUT_COLOR_SPACE_YCBCR444			(0x01)
#define INPUT_COLOR_SPACE_YCBCR422			(0x02)
#define INPUT_COLOR_SPACE_BLACK_MODE			(0x03)

/// AVI Output Format Data
#define TPI_OUTPUT_FORMAT_REG					(0x0A)
#define TPI_YC_Input_Mode						(0x0B)

// TPI InfoFrame related constants
#define TPI_AVI_INFO_REG_ADDR					(0x0C) // AVI InfoFrame Checksum
#define TPI_OTHER_INFO_REG_ADDR		  		(0xBF)
#define TPI_INFO_FRAME_REG_OFFSET				(0xC4)

// TPI AVI InfoFrame Data
#define TPI_AVI_BYTE_0							(0x0C)
#define TPI_AVI_BYTE_1							(0x0D)
#define TPI_AVI_BYTE_2							(0x0E)
#define TPI_AVI_BYTE_3							(0x0F)
#define TPI_AVI_BYTE_4							(0x10)
#define TPI_AVI_BYTE_5							(0x11)

#define TPI_INFO_FRM_DBYTE5					(0xC8)
#define TPI_INFO_FRM_DBYTE6					(0xC9)

#define TPI_END_TOP_BAR_LSB					(0x12)
#define TPI_END_TOP_BAR_MSB					(0x13)

#define TPI_START_BTM_BAR_LSB					(0x14)
#define TPI_START_BTM_BAR_MSB					(0x15)

#define TPI_END_LEFT_BAR_LSB					(0x16)
#define TPI_END_LEFT_BAR_MSB					(0x17)

#define TPI_END_RIGHT_BAR_LSB					(0x18)
#define TPI_END_RIGHT_BAR_MSB					(0x19)

// Colorimetry
#define SET_EX_COLORIMETRY						(0x0C)	// Set TPI_AVI_BYTE_2 to extended colorimetry and use 
													//TPI_AVI_BYTE_3

#define TPI_SYSTEM_CONTROL_DATA_REG			(0x1A)

#define LINK_INTEGRITY_MODE_MASK				(BIT_6)
#define LINK_INTEGRITY_STATIC					(0x00)
#define LINK_INTEGRITY_DYNAMIC					(0x40)

#define TMDS_OUTPUT_CONTROL_MASK				(BIT_4)
#define TMDS_OUTPUT_CONTROL_ACTIVE			(0x00)
#define TMDS_OUTPUT_CONTROL_POWER_DOWN	(0x10)

#define AV_MUTE_MASK							(BIT_3)
#define AV_MUTE_NORMAL						(0x00)
#define AV_MUTE_MUTED							(0x08)

#define DDC_BUS_REQUEST_MASK					(BIT_2)
#define DDC_BUS_REQUEST_NOT_USING			(0x00)
#define DDC_BUS_REQUEST_REQUESTED			(0x04)

#define DDC_BUS_GRANT_MASK					(BIT_1)
#define DDC_BUS_GRANT_NOT_AVAILABLE			(0x00)
#define DDC_BUS_GRANT_GRANTED				(0x02)

#define OUTPUT_MODE_MASK						(BIT_0)
#define OUTPUT_MODE_DVI						(0x00)
#define OUTPUT_MODE_HDMI						(0x01)

// TPI Identification Registers
#define TPI_DEVICE_ID							(0x1B)
#define TPI_DEVICE_REV_ID						(0x1C)

#define TPI_RESERVED2							(0x1D)

#define TPI_DEVICE_POWER_STATE_CTRL_REG		(0x1E)

#define CTRL_PIN_CONTROL_MASK					(BIT_4)
#define CTRL_PIN_TRISTATE						(0x00)
#define CTRL_PIN_DRIVEN_TX_BRIDGE				(0x10)

#define TX_POWER_STATE_MASK					(BIT_1 | BIT_0)
#define TX_POWER_STATE_D0						(0x00)
#define TX_POWER_STATE_D1						(0x01)
#define TX_POWER_STATE_D2						(0x02)
#define TX_POWER_STATE_D3						(0x03)

// Configuration of I2S Interface
#define TPI_I2S_EN								(0x1F)
#define TPI_I2S_IN_CFG							(0x20)
#define SCK_SAMPLE_EDGE						(BIT_7)

// Available only when TPI 0x26[7:6]=10 to select I2S input
#define TPI_I2S_CHST_0							(0x21)
#define TPI_I2S_CHST_1							(0x22)
#define TPI_I2S_CHST_2							(0x23)
#define TPI_I2S_CHST_3							(0x24)
#define TPI_I2S_CHST_4							(0x25)

#define AUDIO_INPUT_LENGTH					(0x24)

// Available only when 0x26[7:6]=01
#define TPI_SPDIF_HEADER						(0x24)
#define TPI_AUDIO_HANDLING						(0x25)

// Audio Configuration Regiaters
#define TPI_AUDIO_INTERFACE_REG				(0x26)
#define AUDIO_MUTE_MASK						(BIT_4)
#define AUDIO_MUTE_NORMAL						(0x00)
#define AUDIO_MUTE_MUTED						(0x10)

#define AUDIO_SEL_MASK							(BITS_7_6)


#define TPI_AUDIO_SAMPLE_CTRL					(0x27)
#define TPI_SPEAKER_CFG						(0xC7)
#define TPI_CODING_TYPE_CHANNEL_COUNT		(0xC4)

//--------------------------------------------------------------------
// HDCP Implementation
// HDCP link security logic is implemented in certain transmitters; unique
//   keys are embedded in each chip as part of the solution. The security 
//   scheme is fully automatic and handled completely by the hardware.
//--------------------------------------------------------------------

/// HDCP Query Data Register
#define TPI_HDCP_QUERY_DATA_REG				(0x29)

#define EXTENDED_LINK_PROTECTION_MASK		(BIT_7)
#define EXTENDED_LINK_PROTECTION_NONE		(0x00)
#define EXTENDED_LINK_PROTECTION_SECURE		(0x80)

#define LOCAL_LINK_PROTECTION_MASK			(BIT_6)
#define LOCAL_LINK_PROTECTION_NONE			(0x00)
#define LOCAL_LINK_PROTECTION_SECURE			(0x40)

#define LINK_STATUS_MASK						(BIT_5 | BIT_4)
#define LINK_STATUS_NORMAL					(0x00)
#define LINK_STATUS_LINK_LOST					(0x10)
#define LINK_STATUS_RENEGOTIATION_REQ		(0x20)
#define LINK_STATUS_LINK_SUSPENDED			(0x30)

#define HDCP_REPEATER_MASK					(BIT_3)
#define HDCP_REPEATER_NO						(0x00)
#define HDCP_REPEATER_YES						(0x08)

#define CONNECTOR_TYPE_MASK					(BIT_2 | BIT_0)
#define CONNECTOR_TYPE_DVI						(0x00)
#define CONNECTOR_TYPE_RSVD					(0x01)
#define CONNECTOR_TYPE_HDMI					(0x04)
#define CONNECTOR_TYPE_FUTURE					(0x05)

#define PROTECTION_TYPE_MASK					(BIT_1)
#define PROTECTION_TYPE_NONE					(0x00)
#define PROTECTION_TYPE_HDCP					(0x02)

/// HDCP Control Data Register
#define TPI_HDCP_CONTROL_DATA_REG			(0x2A)
#define PROTECTION_LEVEL_MASK					(BIT_0)
#define PROTECTION_LEVEL_MIN					(0x00)
#define PROTECTION_LEVEL_MAX					(0x01)

#define KSV_FORWARD_MASK						(BIT_4)
#define KSV_FORWARD_ENABLE					(0x10)
#define KSV_FORWARD_DISABLE					(0x00)

/// HDCP BKSV Registers
#define TPI_BKSV_1_REG							(0x2B)
#define TPI_BKSV_2_REG							(0x2C)
#define TPI_BKSV_3_REG							(0x2D)
#define TPI_BKSV_4_REG							(0x2E)
#define TPI_BKSV_5_REG							(0x2F)

/// HDCP Revision Data Register
#define TPI_HDCP_REVISION_DATA_REG			(0x30)

#define HDCP_MAJOR_REVISION_MASK				(BIT_7 | BIT_6 | BIT_5 | BIT_4)
#define HDCP_MAJOR_REVISION_VALUE				(0x10)

#define HDCP_MINOR_REVISION_MASK				(BIT_3 | BIT_2 | BIT_1 | BIT_0)
#define HDCP_MINOR_REVISION_VALUE				(0x02)

/// HDCP KSV and V' Value Data Register
#define TPI_V_PRIME_SELECTOR_REG				(0x31)

/// V' Value Readback Registers
#define TPI_V_PRIME_7_0_REG					(0x32)
#define TPI_V_PRIME_15_9_REG					(0x33)
#define TPI_V_PRIME_23_16_REG					(0x34)
#define TPI_V_PRIME_31_24_REG					(0x35)

/// HDCP AKSV Registers
#define TPI_AKSV_1_REG							(0x36)
#define TPI_AKSV_2_REG							(0x37)
#define TPI_AKSV_3_REG							(0x38)
#define TPI_AKSV_4_REG							(0x39)
#define TPI_AKSV_5_REG							(0x3A)

#define TPI_DEEP_COLOR_GCP						(0x40)

//--------------------------------------------------------------------
// Interrupt Service
// TPI can be configured to generate an interrupt to the host to notify it of
//   various events. The host can either poll for activity or use an interrupt
//   handler routine. TPI generates on a single interrupt (INT) to the host.
//--------------------------------------------------------------------

/// Interrupt Enable Register
#define TPI_INTERRUPT_ENABLE_REG				(0x3C)

#define HDCP_AUTH_STATUS_CHANGE_EN_MASK	(BIT_7)
#define HDCP_AUTH_STATUS_CHANGE_DISABLE		(0x00)
#define HDCP_AUTH_STATUS_CHANGE_ENABLE		(0x80)

#define HDCP_VPRIME_VALUE_READY_EN_MASK		(BIT_6)
#define HDCP_VPRIME_VALUE_READY_DISABLE		(0x00)
#define HDCP_VPRIME_VALUE_READY_ENABLE		(0x40)

#define HDCP_SECURITY_CHANGE_EN_MASK		(BIT_5)
#define HDCP_SECURITY_CHANGE_DISABLE		    	(0x00)
#define HDCP_SECURITY_CHANGE_ENABLE			(0x20)

#define AUDIO_ERROR_EVENT_EN_MASK			(BIT_4)
#define AUDIO_ERROR_EVENT_DISABLE				(0x00)
#define AUDIO_ERROR_EVENT_ENABLE				(0x10)

#define CPI_EVENT_NO_RX_SENSE_MASK			(BIT_3)
#define CPI_EVENT_NO_RX_SENSE_DISABLE		(0x00)
#define CPI_EVENT_NO_RX_SENSE_ENABLE			(0x08)

#define RECEIVER_SENSE_EVENT_EN_MASK			(BIT_1)
#define RECEIVER_SENSE_EVENT_DISABLE			(0x00)
#define RECEIVER_SENSE_EVENT_ENABLE			(0x02)

#define HOT_PLUG_EVENT_EN_MASK				(BIT_0)
#define HOT_PLUG_EVENT_DISABLE				(0x00)
#define HOT_PLUG_EVENT_ENABLE					(0x01)

/// Interrupt Status Register
#define TPI_INTERRUPT_STATUS_REG				(0x3D)

#define HDCP_AUTH_STATUS_CHANGE_EVENT_MASK	(BIT_7)
#define HDCP_AUTH_STATUS_CHANGE_EVENT_NO	(0x00)
#define HDCP_AUTH_STATUS_CHANGE_EVENT_YES	(0x80)

#define HDCP_VPRIME_VALUE_READY_EVENT_MASK	(BIT_6)
#define HDCP_VPRIME_VALUE_READY_EVENT_NO	(0x00)
#define HDCP_VPRIME_VALUE_READY_EVENT_YES	(0x40)

#define HDCP_SECURITY_CHANGE_EVENT_MASK		(BIT_5)
#define HDCP_SECURITY_CHANGE_EVENT_NO		(0x00)
#define HDCP_SECURITY_CHANGE_EVENT_YES		(0x20)

#define AUDIO_ERROR_EVENT_MASK				(BIT_4)
#define AUDIO_ERROR_EVENT_NO					(0x00)
#define AUDIO_ERROR_EVENT_YES					(0x10)

#define CPI_EVENT_MASK							(BIT_3)
#define CPI_EVENT_NO							(0x00)
#define CPI_EVENT_YES							(0x08)
#define RX_SENSE_MASK							(BIT_3)		// This bit is dual purpose depending on the value of 0x3C[3]
#define RX_SENSE_NOT_ATTACHED					(0x00)
#define RX_SENSE_ATTACHED						(0x08)

#define HOT_PLUG_PIN_STATE_MASK				(BIT_2)
#define HOT_PLUG_PIN_STATE_LOW				(0x00)
#define HOT_PLUG_PIN_STATE_HIGH				(0x04)

#define RECEIVER_SENSE_EVENT_MASK				(BIT_1)
#define RECEIVER_SENSE_EVENT_NO				(0x00)
#define RECEIVER_SENSE_EVENT_YES				(0x02)

#define HOT_PLUG_EVENT_MASK					(BIT_0)
#define HOT_PLUG_EVENT_NO						(0x00)
#define HOT_PLUG_EVENT_YES					(0x01)

/// KSV FIFO First Status Register
#define TPI_KSV_FIFO_READY_INT					(0x3E)

#define KSV_FIFO_READY_MASK					(BIT_1)
#define KSV_FIFO_READY_NO						(0x00)
#define KSV_FIFO_READY_YES						(0x02)

#define TPI_KSV_FIFO_READY_INT_EN				(0x3F)

#define KSV_FIFO_READY_EN_MASK				(BIT_1)
#define KSV_FIFO_READY_DISABLE				(0x00)
#define KSV_FIFO_READY_ENABLE					(0x02)

/// KSV FIFO Last Status Register
#define TPI_KSV_FIFO_STATUS_REG				(0x41)
#define TPI_KSV_FIFO_VALUE_REG					(0x42)

#define KSV_FIFO_LAST_MASK						(BIT_7)
#define KSV_FIFO_LAST_NO						(0x00)
#define KSV_FIFO_LAST_YES						(0x80)

#define KSV_FIFO_COUNT_MASK					(BIT_4 | BIT_3 | BIT_2 | BIT_1 | BIT_0)

// Sync Register Configuration and Sync Monitoring Registers
#define TPI_SYNC_GEN_CTRL						(0x60)
#define TPI_SYNC_POLAR_DETECT					(0x61)

// Explicit Sync DE Generator Registers (TPI 0x60[7]=0)
#define TPI_DE_DLY								(0x62)
#define TPI_DE_CTRL								(0x63)
#define TPI_DE_TOP								(0x64)

#define TPI_RESERVED4							(0x65)

#define TPI_DE_CNT_7_0							(0x66)
#define TPI_DE_CNT_11_8						(0x67)

#define TPI_DE_LIN_7_0							(0x68)
#define TPI_DE_LIN_10_8							(0x69)

#define TPI_DE_H_RES_7_0						(0x6A)
#define TPI_DE_H_RES_10_8						(0x6B)

#define TPI_DE_V_RES_7_0						(0x6C)
#define TPI_DE_V_RES_10_8						(0x6D)

// Embedded Sync Register Set (TPI 0x60[7]=1)
#define TPI_HBIT_TO_HSYNC_7_0					(0x62)
#define TPI_HBIT_TO_HSYNC_9_8					(0x63)
#define TPI_FIELD_2_OFFSET_7_0					(0x64)
#define TPI_FIELD_2_OFFSET_11_8				(0x65)
#define TPI_HWIDTH_7_0							(0x66)
#define TPI_HWIDTH_8_9							(0x67)
#define TPI_VBIT_TO_VSYNC						(0x68)
#define TPI_VWIDTH								(0x69)

// H/W Optimization Control Registers
#define TPI_HW_OPT_CTRL_1						(0xB9)
#define TPI_HW_OPT_CTRL_2						(0xBA)
#define TPI_HW_OPT_CTRL_3						(0xBB)

// H/W Optimization Control Register #3 Set 
#define DDC_DELAY_BIT9_MASK					(BIT_7)
#define DDC_DELAY_BIT9_NO						(0x00)
#define DDC_DELAY_BIT9_YES						(0x80)
#define RI_CHECK_SKIP_MASK						(BIT_3)
#define RI_CHECK_SKIP_NO						(0x00)
#define RI_CHECK_SKIP_YES						(0x08)

// TPI Enable Register
#define TPI_ENABLE								(0xC7)

// Misc InfoFrames
#define MISC_INFO_FRAMES_CTRL					(0xBF)
#define MISC_INFO_FRAMES_TYPE					(0xC0)
#define MISC_INFO_FRAMES_VER					(0xC1)
#define MISC_INFO_FRAMES_LEN					(0xC2)
#define MISC_INFO_FRAMES_CHKSUM				(0xC3)
//--------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////*************************///////////////////////////////
///////////////////////             AV CONFIG              ///////////////////////////////
///////////////////////*************************///////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------
// Video mode table
//------------------------------------------------------------------------------
typedef struct
{
    byte Mode_C1;
    byte Mode_C2;
    byte SubMode;
} ModeIdType;

typedef struct
{
    word Pixels;
    word Lines;
} PxlLnTotalType;

typedef struct
{
    word H;
    word V;
} HVPositionType;

typedef struct
{
    word H;
    word V;
} HVResolutionType;

typedef struct
{
    byte           	RefrTypeVHPol;
    word           	VFreq;
    PxlLnTotalType 	Total;
} TagType;

typedef struct
{
    byte IntAdjMode;
    word HLength;
    byte VLength;
    word Top;
    word Dly;
    word HBit2HSync;
    byte VBit2VSync;
    word Field2Offset;
}  _656Type;

typedef struct
{
    byte VactSpace1;
    byte VactSpace2;
    byte Vblank1;
    byte Vblank2;
    byte Vblank3;
} Vspace_Vblank;

//
// WARNING!  The entries in this enum must remian in the samre order as the PC Codes part
// of the VideoModeTable[].
//
typedef	enum
{
    PC_640x350_85_08 = 0,
    PC_640x400_85_08,
    PC_720x400_70_08,
    PC_720x400_85_04,
    PC_640x480_59_94,
    PC_640x480_72_80,
    PC_640x480_75_00,
    PC_640x480_85_00,
    PC_800x600_56_25,
    PC_800x600_60_317,
    PC_800x600_72_19,
    PC_800x600_75,
    PC_800x600_85_06,
    PC_1024x768_60,
    PC_1024x768_70_07,
    PC_1024x768_75_03,
    PC_1024x768_85,
    PC_1152x864_75,
    PC_1600x1200_60,
    PC_1280x768_59_95,
    PC_1280x768_59_87,
    PC_280x768_74_89,
    PC_1280x768_85,
    PC_1280x960_60,
    PC_1280x960_85,
    PC_1280x1024_60,
    PC_1280x1024_75,
    PC_1280x1024_85,
    PC_1360x768_60,
    PC_1400x105_59_95,
    PC_1400x105_59_98,
    PC_1400x105_74_87,
    PC_1400x105_84_96,
    PC_1600x1200_65,
    PC_1600x1200_70,
    PC_1600x1200_75,
    PC_1600x1200_85,
    PC_1792x1344_60,
    PC_1792x1344_74_997,
    PC_1856x1392_60,
    PC_1856x1392_75,
    PC_1920x1200_59_95,
    PC_1920x1200_59_88,
    PC_1920x1200_74_93,
    PC_1920x1200_84_93,
    PC_1920x1440_60,
    PC_1920x1440_75,
    PC_12560x1440_60,
    PC_SIZE			// Must be last
} PcModeCode_t;

typedef struct
{
    ModeIdType       	ModeId;
    word             		PixClk;
    TagType          		Tag;
    HVPositionType  	Pos;
    HVResolutionType 	Res;
    byte             		AspectRatio;
    _656Type         		_656;
    byte             		PixRep;
    Vspace_Vblank 		VsVb;
    byte             		_3D_Struct;
} VModeInfoType;

#define NSM                     0   // No Sub-Mode

#define	DEFAULT_VIDEO_MODE		0	// 640  x 480p @ 60 VGA

#define ProgrVNegHNeg           0x00
#define ProgrVNegHPos           	0x01
#define ProgrVPosHNeg           	0x02
#define ProgrVPosHPos           	0x03

#define InterlaceVNegHNeg   	0x04
#define InterlaceVPosHNeg      0x05
#define InterlaceVNgeHPos    	0x06
#define InterlaceVPosHPos     	0x07

#define VIC_BASE                	0
#define HDMI_VIC_BASE           43
#define VIC_3D_BASE             	47
#define PC_BASE                 	64

// Aspect ratio
//=================================================
#define R_4                      		0   // 4:3
#define R_4or16                  	1   // 4:3 or 16:9
#define R_16                     		2   // 16:9

//
// These are the VIC codes that we support in a 3D mode
//
#define VIC_FOR_480P_60Hz_4X3			2		// 720p x 480p @60Hz
#define VIC_FOR_480P_60Hz_16X9			3		// 720p x 480p @60Hz
#define VIC_FOR_720P_60Hz				4		// 1280 x 720p @60Mhz
#define VIC_FOR_1080i_60Hz				5		// 1920 x 1080i @60Mhz
#define VIC_FOR_1080p_60Hz				16		// 1920 x 1080i @60hz
#define VIC_FOR_720P_50Hz				19		// 1280 x 720p @50Mhz
#define VIC_FOR_1080i_50Hz				20		// 1920 x 1080i @50Mhz
#define VIC_FOR_1080p_50Hz				31		// 1920 x 720p @50Hz
#define VIC_FOR_1080p_24Hz				32		// 1920 x 720p @24Hz


const VModeInfoType VModesTable[] =
{
	//===================================================================================================================================================================================================================================
    //         VIC                  Refresh type Refresh-Rate Pixel-Totals  Position     Active     Aspect   Int  Length          Hbit  Vbit  Field  Pixel          Vact Space/Blank
    //        1   2  SubM   PixClk  V/H Position       VFreq   H      V      H    V       H    V    Ratio    Adj  H   V  Top  Dly HSync VSync Offset Repl  Space1 Space2 Blank1 Blank2 Blank3  3D
    //===================================================================================================================================================================================================================================
    {{        1,  0, NSM},  2517,  {ProgrVNegHNeg,     6000, { 800,  525}}, {144, 35}, { 640, 480}, R_4,     {0,  96, 2, 33,  48,  16,  10,    0},    0,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 0 - 1.       640  x 480p @ 60 VGA
	{{        2,  3, NSM},  2700,  {ProgrVNegHNeg,     6000, { 858,  525}}, {122, 36}, { 720, 480}, R_4or16, {0,  62, 6, 30,  60,  19,   9,    0},    0,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 1 - 2,3      720  x 480p
    {{        4,  0, NSM},  7425,  {ProgrVPosHPos,     6000, {1650,  750}}, {260, 25}, {1280, 720}, R_16,    {0,  40, 5, 20, 220, 110,   5,    0},    0,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 2 - 4        1280 x 720p@60Hz
    {{        5,  0, NSM},  7425,  {InterlaceVPosHPos, 6000, {2200,  562}}, {192, 20}, {1920,1080}, R_16,    {0,  44, 5, 15, 148,  88,   2, 1100},    0,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 3 - 5        1920 x 1080i
    {{        6,  7, NSM},  2700,  {InterlaceVNegHNeg, 6000, {1716,  264}}, {119, 18}, { 720, 480}, R_4or16, {3,  62, 3, 15, 114,  17,   5,  429},    1,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 4 - 6,7      1440 x 480i,pix repl
    {{        8,  9,   1},  2700,  {ProgrVNegHNeg,     6000, {1716,  262}}, {119, 18}, {1440, 240}, R_4or16, {0, 124, 3, 15, 114,  38,   4,    0},    1,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 5 - 8,9(1)   1440 x 240p
    {{        8,  9,   2},  2700,  {ProgrVNegHNeg,     6000, {1716,  263}}, {119, 18}, {1440, 240}, R_4or16, {0, 124, 3, 15, 114,  38,   4,    0},    1,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 6 - 8,9(2)   1440 x 240p
    {{       10, 11, NSM},  5400,  {InterlaceVNegHNeg, 6000, {3432,  525}}, {238, 18}, {2880, 480}, R_4or16, {0, 248, 3, 15, 228,  76,   4, 1716},    0,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 7 - 10,11    2880 x 480i
    {{       12, 13,   1},  5400,  {ProgrVNegHNeg,     6000, {3432,  262}}, {238, 18}, {2880, 240}, R_4or16, {0, 248, 3, 15, 228,  76,   4,    0},    1,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 8 - 12,13(1) 2880 x 240p
    {{       12, 13,   2},  5400,  {ProgrVNegHNeg,     6000, {3432,  263}}, {238, 18}, {2880, 240}, R_4or16, {0, 248, 3, 15, 228,  76,   4,    0},    1,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 9 - 12,13(2) 2880 x 240p
    {{       14, 15, NSM},  5400,  {ProgrVNegHNeg,     6000, {1716,  525}}, {244, 36}, {1440, 480}, R_4or16, {0, 124, 6, 30, 120,  32,   9,    0},    0,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 10 - 14,15    1440 x 480p
    {{       16,  0, NSM}, 14835,  {ProgrVPosHPos,     6000, {2200, 1125}}, {192, 41}, {1920,1080}, R_16,    {0,  44, 5, 36, 148,  88,   4,    0},    0,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 11 - 16       1920 x 1080p
    {{       17, 18, NSM},  2700,  {ProgrVNegHNeg,     5000, { 864,  625}}, {132, 44}, { 720, 576}, R_4or16, {0,  64, 5, 39,  68,  12,   5,    0},    0,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 12 - 17,18    720  x 576p
    {{       19,  0, NSM},  7425,  {ProgrVPosHPos,     5000, {1980,  750}}, {260, 25}, {1280, 720}, R_16,    {0,  40, 5, 20, 220, 440,   5,    0},    0,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 13 - 19       1280 x 720p@50Hz
    {{       20,  0, NSM},  7425,  {InterlaceVPosHPos, 5000, {2640, 1125}}, {192, 20}, {1920,1080}, R_16,    {0,  44, 5, 15, 148, 528,   2, 1320},    0,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 14 - 20       1920 x 1080i
    {{       21, 22, NSM},  2700,  {InterlaceVNegHNeg, 5000, {1728,  625}}, {132, 22}, { 720, 576}, R_4,     {3,  63, 3, 19, 138,  24,   2,  432},    1,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 15 - 21,22    1440 x 576i
    {{       23, 24,   1},  2700,  {ProgrVNegHNeg,     5000, {1728,  312}}, {132, 22}, {1440, 288}, R_4or16, {0, 126, 3, 19, 138,  24,   2,    0},    1,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 16 - 23,24(1) 1440 x 288p
    {{       23, 24,   2},  2700,  {ProgrVNegHNeg,     5000, {1728,  313}}, {132, 22}, {1440, 288}, R_4or16, {0, 126, 3, 19, 138,  24,   2,    0},    1,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 17 - 23,24(2) 1440 x 288p
    {{       23, 24,   3},  2700,  {ProgrVNegHNeg,     5000, {1728,  314}}, {132, 22}, {1440, 288}, R_4or16, {0, 126, 3, 19, 138,  24,   2,    0},    1,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 18 - 23,24(3) 1440 x 288p
    {{       25, 26, NSM},  5400,  {InterlaceVNegHNeg, 5000, {3456,  625}}, {264, 22}, {2880, 576}, R_4or16, {0, 252, 3, 19, 276,  48,   2, 1728},    1,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 19 - 25,26    2880 x 576i
    {{       27, 28,   1},  5400,  {ProgrVNegHNeg,     5000, {3456,  312}}, {264, 22}, {2880, 288}, R_4or16, {0, 252, 3, 19, 276,  48,   2,    0},    1,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 20 - 27,28(1) 2880 x 288p
    {{       27, 28,   2},  5400,  {ProgrVNegHNeg,     5000, {3456,  313}}, {264, 22}, {2880, 288}, R_4or16, {0, 252, 3, 19, 276,  48,   3,    0},    1,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 21 - 27,28(2) 2880 x 288p
    {{       27, 28,   3},  5400,  {ProgrVNegHNeg,     5000, {3456,  314}}, {264, 22}, {2880, 288}, R_4or16, {0, 252, 3, 19, 276,  48,   4,    0},    1,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 22 - 27,28(3) 2880 x 288p
    {{       29, 30, NSM},  5400,  {ProgrVPosHNeg,     5000, {1728,  625}}, {264, 44}, {1440, 576}, R_4or16, {0, 128, 5, 39, 136,  24,   5,    0},    0,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 23 - 29,30    1440 x 576p
    {{       31,  0, NSM}, 14850,  {ProgrVPosHPos,     5000, {2640, 1125}}, {192, 41}, {1920,1080}, R_16,    {0,  44, 5, 36, 148, 528,   4,    0},    0,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 24 - 31(1)    1920 x 1080p
    {{       32,  0, NSM},  7417,  {ProgrVPosHPos,     2400, {2750, 1125}}, {192, 41}, {1920,1080}, R_16,    {0,  44, 5, 36, 148, 638,   4,    0},    0,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 25 - 32(2)    1920 x 1080p@24Hz
    {{       33,  0, NSM},  7425,  {ProgrVPosHPos,     2500, {2640, 1125}}, {192, 41}, {1920,1080}, R_16,    {0,  44, 5, 36, 148, 528,   4,    0},    0,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 26 - 33(3)    1920 x 1080p
    {{       34,  0, NSM},  7417,  {ProgrVPosHPos,     3000, {2200, 1125}}, {192, 41}, {1920,1080}, R_16,    {0,  44, 5, 36, 148, 88,   4,    0},    0,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 27 - 34(4)    1920 x 1080p@30HZ
    {{       35, 36, NSM}, 10800,  {ProgrVNegHNeg,     5994, {3432,  525}}, {488, 36}, {2880, 480}, R_4or16, {0, 248, 6, 30, 240,  64,  10,    0},    0,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 28 - 35, 36   2880 x 480p@59.94/60Hz
    {{       37, 38, NSM}, 10800,  {ProgrVNegHNeg,     5000, {3456,  625}}, {272, 39}, {2880, 576}, R_4or16, {0, 256, 5, 40, 272,  48,   5,    0},    0,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 29 - 37, 38   2880 x 576p@50Hz
    {{       39,  0, NSM},  7200,  {InterlaceVNegHNeg, 5000, {2304, 1250}}, {352, 62}, {1920,1080}, R_16,    {0, 168, 5, 87, 184,  32,  24,    0},    0,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 30 - 39       1920 x 1080i@50Hz
    {{       40,  0, NSM}, 14850,  {InterlaceVPosHPos, 10000,{2640, 1125}}, {192, 20}, {1920,1080}, R_16,    {0,  44, 5, 15, 148, 528,   2, 1320},    0,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 31 - 40       1920 x 1080i@100Hz
    {{       41,  0, NSM}, 14850,  {InterlaceVPosHPos, 10000,{1980,  750}}, {260, 25}, {1280, 720}, R_16,    {0,  40, 5, 20, 220, 400,   5,    0},    0,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 32 - 41       1280 x 720p@100Hz
    {{       42, 43, NSM},  5400,  {ProgrVNegHNeg,     10000,{ 864,  144}}, {132, 44}, { 720, 576}, R_4or16, {0,  64, 5, 39,  68,  12,   5,    0},    0,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 33 - 42, 43,  720p x 576p@100Hz
    {{       44, 45, NSM},  5400,  {InterlaceVNegHNeg, 10000,{ 864,  625}}, {132, 22}, { 720, 576}, R_4or16, {0,  63, 3, 19,  69,  12,   2,  432},    1,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 34 - 44, 45,  720p x 576i@100Hz, pix repl
    {{       46,  0, NSM}, 14835,  {InterlaceVPosHPos, 11988,{2200, 1125}}, {192, 20}, {1920,1080}, R_16,    {0,  44, 5, 15, 149,  88,   2, 1100},    0,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 35 - 46,      1920 x 1080i@119.88/120Hz
    {{       47,  0, NSM}, 14835,  {ProgrVPosHPos,     11988,{1650,  750}}, {260, 25}, {1280, 720}, R_16,    {0,  40, 5, 20, 220, 110,   5, 1100},    0,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 36 - 47,      1280 x 720p@119.88/120Hz
    {{       48, 49, NSM},  5400,  {ProgrVNegHNeg,     11988,{ 858,  525}}, {122, 36}, { 720, 480}, R_4or16, {0,  62, 6, 30,  60,  16,  10,    0},    0,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 37 - 48, 49   720  x 480p@119.88/120Hz
    {{       50, 51, NSM},  5400,  {InterlaceVNegHNeg, 11988,{ 858,  525}}, {119, 18}, { 720, 480}, R_4or16, {0,  62, 3, 15,  57,  19,   4,  429},    1,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 38 - 50, 51   720  x 480i@119.88/120Hz
    {{       52, 53, NSM}, 10800,  {ProgrVNegHNeg,     20000,{ 864,  625}}, {132, 44}, { 720, 576}, R_4or16, {0,  64, 5, 39,  68,  12,   5,    0},    0,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 39 - 52, 53,  720  x 576p@200Hz
    {{       54, 55, NSM}, 10800,  {InterlaceVNegHNeg, 20000,{ 864,  625}}, {132, 22}, { 720, 576}, R_4or16, {0,  63, 3, 19,  69,  12,   2,  432},    1,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 40 - 54, 55,  1440 x 576i @200Hz, pix repl
    {{       56, 57, NSM}, 10800,  {ProgrVNegHNeg,     24000,{ 858,  525}}, {122, 42}, { 720, 480}, R_4or16, {0,  62, 6, 30,  60,  16,   9,    0},    0,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 41 - 56, 57,  720  x 480p @239.76/240Hz
    {{       58, 59, NSM}, 10800,  {InterlaceVNegHNeg, 24000,{ 858,  525}}, {119, 18}, { 720, 480}, R_4or16, {0,  62, 3, 15,  57,  19,   4,  429},    1,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 42 - 58, 59,  1440 x 480i @239.76/240Hz, pix repl

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 4K x 2K Modes:
	//===================================================================================================================================================================================================================================
    //                                                                                                            Pulse
    //         VIC                  Refresh type Refresh-Rate Pixel-Totals   Position     Active    Aspect   Int  Width           Hbit  Vbit  Field  Pixel          Vact Space/Blank
    //        1   2  SubM   PixClk  V/H Position       VFreq   H      V      H    V       H    V    Ratio    Adj  H   V  Top  Dly HSync VSync Offset Repl  Space1 Space2 Blank1 Blank2 Blank3  3D
    //===================================================================================================================================================================================================================================
    {{        1,  0, NSM}, 297000, {ProgrVNegHNeg,     30000,{4400, 2250}}, {384, 82}, {3840,2160}, R_16,    {0,  88, 10, 72, 296, 176,  8,    0},   0,     {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 43 - 4k x 2k 29.97/30Hz (297.000 MHz)
    {{        2,  0, NSM}, 297000, {ProgrVNegHNeg,     29700,{5280, 2250}}, {384, 82}, {3840,2160}, R_16,    {0,  88, 10, 72, 296,1056,  8,    0},   0,     {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 44 - 4k x 2k 25Hz
    {{        3,  0, NSM}, 297000, {ProgrVNegHNeg,     24000,{5500, 2250}}, {384, 82}, {3840,2160}, R_16,    {0,  88, 10, 72, 296,1276,  8,    0},   0,     {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 45 - 4k x 2k 24Hz (297.000 MHz)
    {{        4  ,0, NSM}, 297000, {ProgrVNegHNeg,     24000,{6500, 2250}}, {384, 82}, {4096,2160}, R_16,    {0,  88, 10, 72, 296,1020,  8,    0},   0,     {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 46 - 4k x 2k 24Hz (SMPTE)

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 3D Modes:
	//===================================================================================================================================================================================================================================
    //                                                                                                            Pulse
    //         VIC                  Refresh type Refresh-Rate Pixel-Totals  Position      Active    Aspect   Int  Width           Hbit  Vbit  Field  Pixel          Vact Space/Blank
    //        1   2  SubM   PixClk  V/H Position       VFreq   H      V      H    V       H    V    Ratio    Adj  H   V  Top  Dly HSync VSync Offset Repl  Space1 Space2 Blank1 Blank2 Blank3  3D
    //===================================================================================================================================================================================================================================
    {{        2,  3, NSM},  2700,  {ProgrVPosHPos,     6000, {858,   525}}, {122, 36}, { 720, 480}, R_4or16, {0,  62, 6, 30,  60,  16,   9,    0},    0,    {0,     0,     0,     0,    0},     8}, // 47 - 3D, 2,3 720p x 480p /60Hz, Side-by-Side (Half)
    {{        4,  0, NSM}, 14850,  {ProgrVPosHPos,     6000, {1650,  750}}, {260, 25}, {1280, 720}, R_16,    {0,  40, 5, 20, 220, 110,   5,    0},    0,    {0,     0,     0,     0,    0},     0}, // 48 - 3D  4   1280 x 720p@60Hz,  Frame Packing
    {{        5,  0, NSM}, 14850,  {InterlaceVPosHPos, 6000, {2200,  562}}, {192, 20}, {1920, 540}, R_16,    {0,  44, 5, 15, 148,  88,   2, 1100},    0,    {23,   22,     0,     0,    0},     0}, // 49 - 3D, 5,  1920 x 1080i/60Hz, Frame Packing
    {{        5,  0, NSM}, 14850,  {InterlaceVPosHPos, 6000, {2200,  562}}, {192, 20}, {1920, 540}, R_16,    {0,  44, 5, 15, 148,  88,   2, 1100},    0,    {0,     0,    22,    22,   23},     1}, // 50 - 3D, 5,  1920 x 1080i/60Hz, Field Alternative
    {{       16,  0, NSM}, 29700,  {ProgrVPosHPos,     6000, {2200, 1125}}, {192, 41}, {1920,1080}, R_16,    {0,  44, 5, 36, 148,  88,   4,    0},    0,    {0,     0,     0,     0,    0},     0}, // 51 - 3D, 16, 1920 x 1080p/60Hz, Frame Packing
    {{       16,  0, NSM}, 29700,  {ProgrVPosHPos,     6000, {2200, 1125}}, {192, 41}, {1920,1080}, R_16,    {0,  44, 5, 36, 148,  88,   4,    0},    0,    {0,     0,     0,     0,    0},     2}, // 52 - 3D, 16, 1920 x 1080p/60Hz, Line Alternative
    {{       16,  0, NSM}, 29700,  {ProgrVPosHPos,     6000, {2200, 1125}}, {192, 41}, {1920,1080}, R_16,    {0,  44, 5, 36, 148,  88,   4,    0},    0,    {0,     0,     0,     0,    0},     3}, // 53 - 3D, 16, 1920 x 1080p/60Hz, Side-by-Side (Full)
    {{       16,  0, NSM}, 14850,  {ProgrVPosHPos,     6000, {2200, 1125}}, {192, 41}, {1920,1080}, R_16,    {0,  44, 5, 36, 148,  88,   4,    0},    0,    {0,     0,     0,     0,    0},     8}, // 54 - 3D, 16, 1920 x 1080p/60Hz, Side-by-Side (Half)
    {{       19,  0, NSM}, 14850,  {ProgrVPosHPos,     5000, {1980,  750}}, {260, 25}, {1280, 720}, R_16,    {0,  40, 5, 20, 220, 440,   5,    0},    0,    {0,     0,     0,     0,    0},     0}, // 55 - 3D, 19, 1280 x 720p@50Hz,  Frame Packing
    {{       19,  0, NSM}, 14850,  {ProgrVPosHPos,     5000, {1980,  750}}, {260, 25}, {1280, 720}, R_16,    {0,  40, 5, 20, 220, 440,   5,    0},    0,    {0,     0,     0,     0,    0},     4}, // 56 - 3D, 19, 1280 x 720p/50Hz,  (L + depth)
    {{       19,  0, NSM}, 29700,  {ProgrVPosHPos,     5000, {1980,  750}}, {260, 25}, {1280, 720}, R_16,    {0,  40, 5, 20, 220, 440,   5,    0},    0,    {0,     0,     0,     0,    0},     5}, // 57 - 3D, 19, 1280 x 720p/50Hz,  (L + depth + Gfx + G-depth)
    {{       20,  0, NSM}, 14850,  {InterlaceVPosHPos, 5000, {2640,  562}}, {192, 20}, {1920, 540}, R_16,    {0,  44, 5, 15, 148, 528,   2, 1220},    0,    {23,   22,     0,     0,    0},     0}, // 58 - 3D, 20, 1920 x 1080i/50Hz, Frame Packing
    {{       20,  0, NSM}, 14850,  {InterlaceVPosHPos, 5000, {2640,  562}}, {192, 20}, {1920, 540}, R_16,    {0,  44, 5, 15, 148, 528,   2, 1220},    0,    {0,     0,    22,    22,   23},     1}, // 59 - 3D, 20, 1920 x 1080i/50Hz, Field Alternative
    {{       31,  0, NSM}, 29700,  {ProgrVPosHPos,     5000, {2640, 1125}}, {192, 41}, {1920,1080}, R_16,    {0,  44, 5, 36, 148, 528,   4,    0},    0,    {0,     0,     0,     0,    0},     0}, // 60 - 3D, 31, 1920 x 1080p/50Hz, Frame Packing
    {{       31,  0, NSM}, 29700,  {ProgrVPosHPos,     5000, {2640, 1125}}, {192, 41}, {1920,1080}, R_16,    {0,  44, 5, 36, 148, 528,   4,    0},    0,    {0,     0,     0,     0,    0},     2}, // 61 - 3D, 31, 1920 x 1080p/50Hz, Line Alternative
    {{       31,  0, NSM}, 29700,  {ProgrVPosHPos,     5000, {2650, 1125}}, {192, 41}, {1920,1080}, R_16,    {0,  44, 5, 36, 148, 528,   4,    0},    0,    {0,     0,     0,     0,    0},     3}, // 62 - 3D, 31, 1920 x 1080p/50Hz, Side-by-Side (Full)
    {{       32,  0, NSM}, 14850,  {ProgrVPosHPos,     2400, {2750, 1125}}, {192, 41}, {1920,1080}, R_16,    {0,  44, 5, 36, 148, 638,   4,    0},    0,    {0,     0,     0,     0,    0},     0}, // 63 - 3D, 32, 1920 x 1080p@24Hz, Frame Packing

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// NOTE: DO NOT ATTEMPT INPUT RESOLUTIONS THAT REQUIRE PIXEL CLOCK FREQUENCIES HIGHER THAN THOSE SUPPORTED BY THE TRANSMITTER CHIP

	//===================================================================================================================================================================================================================================
	//                                                                                                              Sync Pulse
    //  VIC                          Refresh type   fresh-Rate  Pixel-Totals    Position    Active     Aspect   Int  Width            Hbit  Vbit  Field  Pixel          Vact Space/Blank
    // 1   2  SubM         PixClk    V/H Position       VFreq   H      V        H    V       H    V     Ratio   {Adj  H   V  Top  Dly HSync VSync Offset} Repl  Space1 Space2 Blank1 Blank2 Blank3  3D
    //===================================================================================================================================================================================================================================
    {{PC_BASE  , 0,NSM},    3150,   {ProgrVNegHPos,     8508,   {832, 445}},    {160,63},   {640,350},   R_16,  {0,  64,  3,  60,  96,  32,  32,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 64 - 640x350@85.08
    {{PC_BASE+1, 0,NSM},    3150,   {ProgrVPosHNeg,     8508,   {832, 445}},    {160,44},   {640,400},   R_16,  {0,  64,  3,  41,  96,  32,   1,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 65 - 640x400@85.08
    {{PC_BASE+2, 0,NSM},    2700,   {ProgrVPosHNeg,     7008,   {900, 449}},    {0,0},      {720,400},   R_16,  {0,   0,  0,   0,   0,   0,   0,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 66 - 720x400@70.08
    {{PC_BASE+3, 0,NSM},    3500,   {ProgrVPosHNeg,     8504,   {936, 446}},    {20,45},    {720,400},   R_16,  {0,  72,  3,  42, 108,  36,   1,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 67 - 720x400@85.04
    {{PC_BASE+4, 0,NSM},    2517,   {ProgrVNegHNeg,     5994,   {800, 525}},    {144,35},   {640,480},   R_4,   {0,  96,  2,  33,  48,  16,  10,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 68 - 640x480@59.94
    {{PC_BASE+5, 0,NSM},    3150,   {ProgrVNegHNeg,     7281,   {832, 520}},    {144,31},   {640,480},   R_4,   {0,  40,  3,  28, 128, 128,   9,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 69 - 640x480@72.80
    {{PC_BASE+6, 0,NSM},    3150,   {ProgrVNegHNeg,     7500,   {840, 500}},    {21,19},    {640,480},   R_4,   {0,  64,  3,  28, 128,  24,   9,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 70 - 640x480@75.00
    {{PC_BASE+7,0,NSM},     3600,   {ProgrVNegHNeg,     8500,   {832, 509}},    {168,28},   {640,480},   R_4,   {0,  56,  3,  25, 128,  24,   9,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 71 - 640x480@85.00
    {{PC_BASE+8,0,NSM},     3600,   {ProgrVPosHPos,     5625,   {1024, 625}},   {200,24},   {800,600},   R_4,   {0,  72,  2,  22, 128,  24,   1,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 72 - 800x600@56.25
    {{PC_BASE+9,0,NSM},     4000,   {ProgrVPosHPos,     6032,   {1056, 628}},   {216,27},   {800,600},   R_4,   {0, 128,  4,  23,  88,  40,   1,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 73 - 800x600@60.317
    {{PC_BASE+10,0,NSM},    5000,   {ProgrVPosHPos,     7219,   {1040, 666}},   {184,29},   {800,600},   R_4,   {0, 120,  6,  23,  64,  56,  37,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 74 - 800x600@72.19
    {{PC_BASE+11,0,NSM},    4950,   {ProgrVPosHPos,     7500,   {1056, 625}},   {240,24},   {800,600},   R_4,   {0,  80,  3,  21, 160,  16,   1,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 75 - 800x600@75
    {{PC_BASE+12,0,NSM},    5625,   {ProgrVPosHPos,     8506,   {1048, 631}},   {216,30},   {800,600},   R_4,   {0,  64,  3,  27, 152,  32,   1,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 76 - 800x600@85.06
    {{PC_BASE+13,0,NSM},    6500,   {ProgrVNegHNeg,     6000,   {1344, 806}},   {296,35},   {1024,768},  R_4,   {0, 136,  6,  29, 160,  24,   3,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 77 - 1024x768@60
    {{PC_BASE+14,0,NSM},    7500,   {ProgrVNegHNeg,     7007,   {1328, 806}},   {280,35},   {1024,768},  R_4,   {0, 136,  6,  19, 144,  24,   3,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 78 - 1024x768@70.07
    {{PC_BASE+15,0,NSM},    7875,   {ProgrVPosHPos,     7503,   {1312, 800}},   {272,31},   {1024,768},  R_4,   {0,  96,  3,  28, 176,  16,   1,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 79 - 1024x768@75.03
    {{PC_BASE+16,0,NSM},    9450,   {ProgrVPosHPos,     8500,   {1376, 808}},   {304,39},   {1024,768},  R_4,   {0,  96,  3,  36, 208,  48,   1,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 80 - 1024x768@85
    {{PC_BASE+17,0,NSM},   10800,   {ProgrVPosHPos,     7500,   {1600, 900}},   {384,35},   {1152,864},  R_4,   {0, 128,  3,  32, 256,  64,   1,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 81 - 1152x864@75
    {{PC_BASE+18,0,NSM},   16200,   {ProgrVPosHPos,     6000,   {2160, 1250}},  {496,49},   {1600,1200}, R_4,   {0, 304,  3,  46, 304,  64,   1,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 82 - 1600x1200@60
    {{PC_BASE+19,0,NSM},    6825,   {ProgrVNegHPos,     6000,   {1440, 790}},   {112,19},   {1280,768},  R_16,  {0,  32,  7,  12,  80,  48,   3,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 83 - 1280x768@59.95
    {{PC_BASE+20,0,NSM},    7950,   {ProgrVPosHNeg,     5987,   {1664, 798}},   {320,27},   {1280,768},  R_16,  {0, 128,  7,  20, 192,  64,   3,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 84 - 1280x768@59.87
    {{PC_BASE+21,0,NSM},   10220,   {ProgrVPosHNeg,     6029,   {1696, 805}},   {320,27},   {1280,768},  R_16,  {0, 128,  7,  27, 208,  80,   3,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 85 - 1280x768@74.89
    {{PC_BASE+22,0,NSM},   11750,   {ProgrVPosHNeg,     8484,   {1712, 809}},   {352,38},   {1280,768},  R_16,  {0, 136,  7,  31, 216,  80,   3,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 86 - 1280x768@85
    {{PC_BASE+23,0,NSM},   10800,   {ProgrVPosHPos,     6000,   {1800, 1000}},  {424,39},   {1280,960},  R_4,   {0, 112,  3,  36, 312,  96,   1,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 87 - 1280x960@60
    {{PC_BASE+24,0,NSM},   14850,   {ProgrVPosHPos,     8500,   {1728, 1011}},  {384,50},   {1280,960},  R_4,   {0, 160,  3,  47, 224,  64,   1,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 88 - 1280x960@85
    {{PC_BASE+25,0,NSM},   10800,   {ProgrVPosHPos,     6002,   {1688, 1066}},  {360,41},   {1280,1024}, R_4,   {0, 112,  3,  38, 248,  48,   1,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 89 - 1280x1024@60
    {{PC_BASE+26,0,NSM},   13500,   {ProgrVPosHPos,     7502,   {1688, 1066}},  {392,41},   {1280,1024}, R_4,   {0, 144,  3,  38, 248,  16,   1,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 90 - 1280x1024@75
    {{PC_BASE+27,0,NSM},   15750,   {ProgrVPosHPos,     8502,   {1728, 1072}},  {384,47},   {1280,1024}, R_4,   {0, 160,  3,   4, 224,  64,   1,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 91 - 1280x1024@85
    {{PC_BASE+28,0,NSM},    8550,   {ProgrVPosHPos,     6002,   {1792, 795}},   {368,24},   {1360,768},  R_16,  {0, 112,  6,  18, 256,  64,   3,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 92 - 1360x768@60
    {{PC_BASE+29,0,NSM},   10100,   {ProgrVNegHPos,     5995,   {1560, 1080}},  {112,27},   {1400,1050}, R_4,   {0,  32,  4,  23,  80,  48,   3,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 93 - 1400x105@59.95
    {{PC_BASE+30,0,NSM},   12175,   {ProgrVPosHNeg,     5998,   {1864, 1089}},  {376,36},   {1400,1050}, R_4,   {0, 144,  4,  32, 232,  88,   3,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 94 - 1400x105@59.98
    {{PC_BASE+31,0,NSM},   15600,   {ProgrVPosHNeg,     7487,   {1896, 1099}},  {392,46},   {1400,1050}, R_4,   {0, 144,  4,  22, 248, 104,   3,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 95 - 1400x105@74.87
    {{PC_BASE+32,0,NSM},   17950,   {ProgrVPosHNeg,     8496,   {1912, 1105}},  {408,52},   {1400,1050}, R_4,   {0, 152,  4,  48, 256, 104,   3,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 96 - 1400x105@84.96
    {{PC_BASE+33,0,NSM},   17550,   {ProgrVPosHPos,     6500,   {2160, 1250}},  {496,49},   {1600,1200}, R_4,   {0, 192,  3,  46, 304,  64,   1,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 97 - 1600x1200@65
    {{PC_BASE+34,0,NSM},   18900,   {ProgrVPosHPos,     7000,   {2160, 1250}},  {496,49},   {1600,1200}, R_4,   {0, 192,  3,  46, 304,  64,   1,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 98 - 1600x1200@70
    {{PC_BASE+35,0,NSM},   20250,   {ProgrVPosHPos,     7500,   {2160, 1250}},  {496,49},   {1600,1200}, R_4,   {0, 192,  3,  46, 304,  64,   1,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 99 - 1600x1200@75
    {{PC_BASE+36,0,NSM},   22950,   {ProgrVPosHPos,     8500,   {2160, 1250}},  {496,49},   {1600,1200}, R_4,   {0, 192,  3,  46, 304,  64,   1,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 100 - 1600x1200@85
    {{PC_BASE+37,0,NSM},   20475,   {ProgrVPosHNeg,     6000,   {2448, 1394}},  {528,49},   {1792,1344}, R_4,   {0, 200,  3,  46, 328, 128,   1,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 101 - 1792x1344@60
    {{PC_BASE+38,0,NSM},   26100,   {ProgrVPosHNeg,     7500,   {2456, 1417}},  {568,72},   {1792,1344}, R_4,   {0, 216,  3,  69, 352,  96,   1,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 102 - 1792x1344@74.997
    {{PC_BASE+39,0,NSM},   21825,   {ProgrVPosHNeg,     6000,   {2528, 1439}},  {576,46},   {1856,1392}, R_4,   {0, 224,  3,  43, 352,  96,   1,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 103 - 1856x1392@60
    {{PC_BASE+40,0,NSM},   28800,   {ProgrVPosHNeg,     7500,   {2560, 1500}},  {576,107},  {1856,1392}, R_4,   {0, 224,  3, 104, 352, 128,   1,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 104 - 1856x1392@75
    {{PC_BASE+41,0,NSM},   15400,   {ProgrVNegHPos,     5995,   {2080, 1235}},  {112,32},   {1920,1200}, R_16,  {0,  32,  6,  26,  80,  48,   3,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 106 - 1920x1200@59.95
    {{PC_BASE+42,0,NSM},   19325,   {ProgrVPosHNeg,     5988,   {2592, 1245}},  {536,42},   {1920,1200}, R_16,  {0, 200,  6,  36, 336, 136,   3,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 107 - 1920x1200@59.88
    {{PC_BASE+43,0,NSM},   24525,   {ProgrVPosHNeg,     7493,   {2608, 1255}},  {552,52},   {1920,1200}, R_16,  {0, 208,  6,  46, 344, 136,   3,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 108 - 1920x1200@74.93
    {{PC_BASE+44,0,NSM},   28125,   {ProgrVPosHNeg,     8493,   {2624, 1262}},  {560,59},   {1920,1200}, R_16,  {0, 208,  6,  53, 352, 144,   3,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 109 - 1920x1200@84.93
    {{PC_BASE+45,0,NSM},   23400,   {ProgrVPosHNeg,     6000,   {2600, 1500}},  {552,59},   {1920,1440}, R_4,   {0, 208,  3,  56, 344, 128,   1,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 110 - 1920x1440@60
    {{PC_BASE+46,0,NSM},   29700,   {ProgrVPosHNeg,     7500,   {2640, 1500}},  {576,59},   {1920,1440}, R_4,   {0, 224,  3,  56, 352, 144,   1,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 111 - 1920x1440@75
    {{PC_BASE+47,0,NSM},   24150,   {ProgrVPosHNeg,     6000,   {2720, 1481}},  {48,  3},   {2560,1440}, R_16,  {0,  32,  5,  56, 352, 144,   1,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 112 - 2560x1440@60 // %%% need work
    {{PC_BASE+48,0,NSM},    2700,   {InterlaceVNegHNeg, 6000,   {1716,  264}},  {244,18},   {1440, 480},R_4or16,{3, 124,  3,  15, 114,  17,   5,     429},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 113 - 1440 x 480i 
};

//------------------------------------------------------------------------------
// Aspect Ratio table defines the aspect ratio as function of VIC. This table
// should be used in conjunction with the 861-D part of VModeInfoType VModesTable[]
// (formats 0 - 59) because some formats that differ only in their AR are grouped
// together (e.g., formats 2 and 3).
//------------------------------------------------------------------------------
const byte AspectRatioTable[] =
{
       R_4,  R_4, R_16, R_16, R_16,  R_4, R_16,  R_4, R_16,  R_4,
	R_16,  R_4, R_16,  R_4, R_16, R_16,  R_4, R_16, R_16, R_16,
	R_4, R_16,  R_4, R_16,  R_4, R_16,  R_4, R_16,  R_4, R_16,
	R_16, R_16, R_16, R_16,  R_4, R_16,  R_4, R_16, R_16, R_16,
	R_16,  R_4, R_16,  R_4, R_16, R_16, R_16,  R_4, R_16,  R_4,
	R_16,  R_4, R_16,  R_4, R_16,  R_4, R_16,  R_4, R_16
};

//------------------------------------------------------------------------------
// VIC to Indexc table defines which VideoModeTable entry is appropreate for this VIC code. 
// Note: This table is valid ONLY for VIC codes in 861-D formats, NOT for HDMI_VIC codes
// or 3D codes!
//------------------------------------------------------------------------------
const byte VIC2Index[] =
{
	0,  0,  1,  1,  2,  3,  4,  4,  5,  5,
	7,  7,  8,  8, 10, 10, 11, 12, 12, 13,
	14, 15, 15, 16, 16, 19, 19, 20, 20, 23,
	23, 24, 25, 26, 27, 28, 28, 29, 29, 30,
	31, 32, 33, 33, 34, 34, 35, 36, 37, 37,
	38, 38, 39, 39, 40, 40, 41, 41, 42, 42
};


void siHdmiTx_PowerStateD2 (void);
void siHdmiTx_PowerStateD0fromD2 (void);

byte siHdmiTx_VideoSet (void);
byte siHdmiTx_AudioSet (void);
byte siHdmiTx_TPI_Init (void);
void siHdmiTx_TPI_Poll (void);
void siHdmiTx_VideoSel (byte vmode);
void siHdmiTx_AudioSel (byte Afs);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif	/* _SIHDMITX_902X_TPI_H_ */

