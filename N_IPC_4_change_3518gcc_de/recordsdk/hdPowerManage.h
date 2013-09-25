/******************************************************************************
* 版权信息：(C) 2006-2010, 深圳市泰达仕科技有限公司研发部版权所有
* 系统名称：录像SDK
* 文件名称：hdPowerManage.h
* 文件说明：该文件描述了硬盘电源管理的函数声明
*           包括：
*           1．宏的定义
*			2. 数据结构的定义
*           3．硬盘电源管理的函数声明
* 作    者：庄惠斌
* 版本信息：V1.0
* 设计日期: 2007-02-07
* 修改记录:
*   修改1      日    期:
*              姓    名:
*              修改内容:
*   修改:2     日    期:
*              姓    名:
*              修改内容:
* 其他说明: 无
******************************************************************************/

#ifndef __HARD_DISK_H_
#define __HARD_DISK_H_

//设备类型
#define NO_DEV                  0xffff
#define ATA_DEV                 0x0000
#define ATAPI_DEV               0x0001

/* word definitions */
/* ---------------- */
#define GEN_CONFIG			0   /* general configuration */
#define LCYLS				1   /* number of logical cylinders */
#define CONFIG				2   /* specific configuration */
#define LHEADS				3   /* number of logical heads */
#define TRACK_BYTES			4   /* number of bytes/track (ATA-1) */
#define SECT_BYTES			5   /* number of bytes/sector (ATA-1) */
#define LSECTS				6   /* number of logical sectors/track */
#define START_SERIAL		10  /* ASCII serial number */
#define LENGTH_SERIAL		10  /* 10 words (20 bytes or characters) */
#define BUF_TYPE			20  /* buffer type (ATA-1) */
#define BUFFER__SIZE		21  /* buffer size (ATA-1) */
#define RW_LONG				22  /* extra bytes in R/W LONG cmd ( < ATA-4)*/
#define START_FW_REV		23  /* ASCII firmware revision */
#define LENGTH_FW_REV		4  /*  4 words (8 bytes or characters) */
#define START_MODEL    		27  /* ASCII model number */
#define LENGTH_MODEL		20  /* 20 words (40 bytes or characters) */
#define SECTOR_XFER_MAX		47  /* r/w multiple: max sectors xfered */
#define DWORD_IO			48  /* can do double-word IO (ATA-1 only) */
#define CAPAB_0				49  /* capabilities */
#define CAPAB_1				50
#define PIO_MODE			51  /* max PIO mode supported (obsolete)*/
#define DMA_MODE			52  /* max Singleword DMA mode supported (obs)*/
#define WHATS_VALID			53  /* what fields are valid */
#define LCYLS_CUR			54  /* current logical cylinders */
#define LHEADS_CUR			55  /* current logical heads */
#define LSECTS_CUR	        56  /* current logical sectors/track */
#define CAPACITY_LSB		57  /* current capacity in sectors */
#define CAPACITY_MSB		58
#define SECTOR_XFER_CUR		59  /* r/w multiple: current sectors xfered */
#define LBA_SECTS_LSB		60  /* LBA: total number of user */
#define LBA_SECTS_MSB		61  /*      addressable sectors */
#define SINGLE_DMA			62  /* singleword DMA modes */
#define MULTI_DMA			63  /* multiword DMA modes */
#define ADV_PIO_MODES		64  /* advanced PIO modes supported */
								/* multiword DMA xfer cycle time: */
#define DMA_TIME_MIN		65  /*   - minimum */
#define DMA_TIME_NORM		66  /*   - manufacturer's recommended	*/
								/* minimum PIO xfer cycle time: */
#define PIO_NO_FLOW			67  /*   - without flow control */
#define PIO_FLOW			68  /*   - with IORDY flow control */
#define PKT_REL				71  /* typical #ns from PKT cmd to bus rel */
#define SVC_NBSY			72  /* typical #ns from SERVICE cmd to !BSY */
#define CDR_MAJOR			73  /* CD ROM: major version number */
#define CDR_MINOR			74  /* CD ROM: minor version number */
#define QUEUE_DEPTH			75  /* queue depth */
#define MAJOR				80  /* major version number */
#define MINOR				81  /* minor version number */
#define CMDS_SUPP_0			82  /* command/feature set(s) supported */
#define CMDS_SUPP_1			83
#define CMDS_SUPP_2			84
#define CMDS_EN_0			85  /* command/feature set(s) enabled */
#define CMDS_EN_1			86
#define CMDS_EN_2			87
#define ULTRA_DMA			88  /* ultra DMA modes */
								/* time to complete security erase */
#define ERASE_TIME			89  /*   - ordinary */
#define ENH_ERASE_TIME		90  /*   - enhanced */
#define ADV_PWR				91  /* current advanced power management level
									 in low byte, 0x40 in high byte. */
#define PSWD_CODE			92  /* master password revision code	*/
#define HWRST_RSLT			93  /* hardware reset result */
#define ACOUSTIC  			94  /* acoustic mgmt values ( >= ATA-6) */
#define LBA_LSB				100 /* LBA: maximum.  Currently only 48 */
#define LBA_MID				101 /*      bits are used, but addr 103 */
#define LBA_48_MSB			102 /*      has been reserved for LBA in */
#define LBA_64_MSB			103 /*      the future. */
#define RM_STAT 			127 /* removable media status notification feature set support */
#define SECU_STATUS			128 /* security status */
#define CFA_PWR_MODE		160 /* CFA power mode 1 */
#define START_MEDIA         176 /* media serial number */
#define LENGTH_MEDIA        20  /* 20 words (40 bytes or characters)*/
#define START_MANUF         196 /* media manufacturer I.D. */
#define LENGTH_MANUF        10  /* 10 words (20 bytes or characters) */
#define INTEGRITY			255 /* integrity word */

/* bit definitions within the words */
/* -------------------------------- */

/* many words are considered valid if bit 15 is 0 and bit 14 is 1 */
#define VALID			0xc000
#define VALID_VAL		0x4000
/* many words are considered invalid if they are either all-0 or all-1 */
#define NOVAL_0			0x0000
#define NOVAL_1			0xffff

/* word 0: gen_config */
#define NOT_ATA			0x8000
#define NOT_ATAPI		0x4000	/* (check only if bit 15 == 1) */
#define MEDIA_REMOVABLE		0x0080
#define DRIVE_NOT_REMOVABLE	0x0040  /* bit obsoleted in ATA 6 */
#define INCOMPLETE		0x0004
#define CFA_SUPPORT_VAL		0x848a	/* 848a=CFA feature set support */
#define DRQ_RESPONSE_TIME	0x0060
#define DRQ_3MS_VAL		0x0000
#define DRQ_INTR_VAL		0x0020
#define DRQ_50US_VAL		0x0040
#define PKT_SIZE_SUPPORTED	0x0003
#define PKT_SIZE_12_VAL		0x0000
#define PKT_SIZE_16_VAL		0x0001
#define EQPT_TYPE		0x1f00
#define SHIFT_EQPT		8

#define CDROM 0x0005

static const char *pkt_str[] = {
	"Direct-access device",			/* word 0, bits 12-8 = 00 */
	"Sequential-access device",		/* word 0, bits 12-8 = 01 */
	"Printer",						/* word 0, bits 12-8 = 02 */
	"Processor",					/* word 0, bits 12-8 = 03 */
	"Write-once device",			/* word 0, bits 12-8 = 04 */
	"CD-ROM",						/* word 0, bits 12-8 = 05 */
	"Scanner",						/* word 0, bits 12-8 = 06 */
	"Optical memory",				/* word 0, bits 12-8 = 07 */
	"Medium changer",				/* word 0, bits 12-8 = 08 */
	"Communications device",		/* word 0, bits 12-8 = 09 */
	"ACS-IT8 device",				/* word 0, bits 12-8 = 0a */
	"ACS-IT8 device",				/* word 0, bits 12-8 = 0b */
	"Array controller",				/* word 0, bits 12-8 = 0c */
	"Enclosure services",			/* word 0, bits 12-8 = 0d */
	"Reduced block command device",	/* word 0, bits 12-8 = 0e */
	"Optical card reader/writer",	/* word 0, bits 12-8 = 0f */
	"",					/* word 0, bits 12-8 = 10 */
	"",					/* word 0, bits 12-8 = 11 */
	"",					/* word 0, bits 12-8 = 12 */
	"",					/* word 0, bits 12-8 = 13 */
	"",					/* word 0, bits 12-8 = 14 */
	"",					/* word 0, bits 12-8 = 15 */
	"",					/* word 0, bits 12-8 = 16 */
	"",					/* word 0, bits 12-8 = 17 */
	"",					/* word 0, bits 12-8 = 18 */
	"",					/* word 0, bits 12-8 = 19 */
	"",					/* word 0, bits 12-8 = 1a */
	"",					/* word 0, bits 12-8 = 1b */
	"",					/* word 0, bits 12-8 = 1c */
	"",					/* word 0, bits 12-8 = 1d */
	"",					/* word 0, bits 12-8 = 1e */
	"Unknown",			/* word 0, bits 12-8 = 1f */
};

static const char *ata1_cfg_str[] = {			/* word 0 in ATA-1 mode */
	"reserved",							/* bit 0 */
	"hard sectored",					/* bit 1 */
	"soft sectored",					/* bit 2 */
	"not MFM encoded ",					/* bit 3 */
	"head switch time > 15us",			/* bit 4 */
	"spindle motor control option",		/* bit 5 */
	"fixed drive",						/* bit 6 */
	"removable drive",					/* bit 7 */
	"disk xfer rate <= 5Mbs",			/* bit 8 */
	"disk xfer rate > 5Mbs, <= 10Mbs",	/* bit 9 */
	"disk xfer rate > 5Mbs",			/* bit 10 */
	"rotational speed tol.",			/* bit 11 */
	"data strobe offset option",		/* bit 12 */
	"track offset option",				/* bit 13 */
	"format speed tolerance gap reqd",	/* bit 14 */
	"ATAPI"								/* bit 14 */
};

/* word 1: number of logical cylinders */
#define LCYLS_MAX		0x3fff /* maximum allowable value */

/* word 2: specific configuration
 * (a) require SET FEATURES to spin-up
 * (b) require spin-up to fully reply to IDENTIFY DEVICE
 */
#define STBY_NID_VAL		0x37c8  /*     (a) and     (b) */
#define STBY_ID_VAL			0x738c	/*     (a) and not (b) */
#define PWRD_NID_VAL 		0x8c73	/* not (a) and     (b) */
#define PWRD_ID_VAL			0xc837	/* not (a) and not (b) */

/* words 47 & 59: sector_xfer_max & sector_xfer_cur */
#define SECTOR_XFER				0x00ff  /* sectors xfered on r/w multiple cmds*/
#define MULTIPLE_SETTING_VALID  0x0100  /* 1=multiple sector setting is valid */

/* word 49: capabilities 0 */
#define STD_STBY  	  	0x2000  /* 1=standard values supported (ATA);
					   0=vendor specific values */
#define IORDY_SUP		0x0800  /* 1=support; 0=may be supported */
#define IORDY_OFF		0x0400  /* 1=may be disabled */
#define LBA_SUP			0x0200  /* 1=Logical Block Address support */
#define DMA_SUP			0x0100  /* 1=Direct Memory Access support */
#define DMA_IL_SUP		0x8000  /* 1=interleaved DMA support (ATAPI) */
#define CMD_Q_SUP		0x4000  /* 1=command queuing support (ATAPI) */
#define OVLP_SUP		0x2000  /* 1=overlap operation support (ATAPI) */
#define SWRST_REQ		0x1000  /* 1=ATA SW reset required (ATAPI, obsolete */

/* word 50: capabilities 1 */
#define MIN_STANDBY_TIMER	0x0001  /* 1=device specific standby timer value minimum */

/* words 51 & 52: PIO & DMA cycle times */
#define MODE			0xff00  /* the mode is in the MSBs */

/* word 53: whats_valid */
#define OK_W88     	   	0x0004	/* the ultra_dma info is valid */
#define OK_W64_70		0x0002  /* see above for word descriptions */
#define OK_W54_58		0x0001  /* current cyl, head, sector, cap. info valid */

/*word 63,88: dma_mode, ultra_dma_mode*/
#define MODE_MAX		7	/* bit definitions force udma <=7 (when
					 * udma >=8 comes out it'll have to be
					 * defined in a new dma_mode word!) */

/* word 64: PIO transfer modes */

#define PIO_SUP			0x00ff  /* only bits 0 & 1 are used so far,  */
#define PIO_MODE_MAX		8       /* but all 8 bits are defined        */

/* word 75: queue_depth */
#define DEPTH_BITS		0x001f  /* bits used for queue depth */

/* words 80-81: version numbers */
/* NOVAL_0 or  NOVAL_1 means device does not report version */

/* word 81: minor version number */
#define MINOR_MAX		0x1C
#ifdef CONFIG_FEATURE_HDPARM_GET_IDENTITY
static const char *minor_str[] = {				/* word 81 value: */
	"device does not report version",			/* 0x0000	*/
	"ATA-1 X3T9.2 781D prior to revision 4",	/* 0x0001	*/
	"ATA-1 published, ANSI X3.221-1994",		/* 0x0002	*/
	"ATA-1 X3T9.2 781D revision 4",				/* 0x0003	*/
	"ATA-2 published, ANSI X3.279-1996",		/* 0x0004	*/
	"ATA-2 X3T10 948D prior to revision 2k",	/* 0x0005	*/
	"ATA-3 X3T10 2008D revision 1",				/* 0x0006	*/
	"ATA-2 X3T10 948D revision 2k",				/* 0x0007	*/
	"ATA-3 X3T10 2008D revision 0",				/* 0x0008	*/
	"ATA-2 X3T10 948D revision 3",				/* 0x0009	*/
	"ATA-3 published, ANSI X3.298-199x",		/* 0x000a	*/
	"ATA-3 X3T10 2008D revision 6",				/* 0x000b	*/
	"ATA-3 X3T13 2008D revision 7 and 7a",		/* 0x000c	*/
	"ATA/ATAPI-4 X3T13 1153D revision 6",		/* 0x000d	*/
	"ATA/ATAPI-4 T13 1153D revision 13",		/* 0x000e	*/
	"ATA/ATAPI-4 X3T13 1153D revision 7",		/* 0x000f	*/
	"ATA/ATAPI-4 T13 1153D revision 18",		/* 0x0010	*/
	"ATA/ATAPI-4 T13 1153D revision 15",		/* 0x0011	*/
	"ATA/ATAPI-4 published, ANSI NCITS 317-1998",	/* 0x0012	*/
	"ATA/ATAPI-5 T13 1321D revision 3",
	"ATA/ATAPI-4 T13 1153D revision 14",			/* 0x0014	*/
	"ATA/ATAPI-5 T13 1321D revision 1",				/* 0x0015	*/
	"ATA/ATAPI-5 published, ANSI NCITS 340-2000",	/* 0x0016	*/
	"ATA/ATAPI-4 T13 1153D revision 17",			/* 0x0017	*/
	"ATA/ATAPI-6 T13 1410D revision 0",				/* 0x0018	*/
	"ATA/ATAPI-6 T13 1410D revision 3a",			/* 0x0019	*/
	"Reserved",										/* 0x001a	*/
	"ATA/ATAPI-6 T13 1410D revision 2",				/* 0x001b	*/
	"ATA/ATAPI-6 T13 1410D revision 1",				/* 0x001c	*/
	"reserved"										/* 0x001d	*/
	"reserved"										/* 0x001e	*/
	"reserved"										/* 0x001f-0xfffe*/
};
#endif
static const char actual_ver[] = {
			/* word 81 value: */
	0,		/* 0x0000	WARNING: 	*/
	1,		/* 0x0001	WARNING: 	*/
	1,		/* 0x0002	WARNING: 	*/
	1,		/* 0x0003	WARNING: 	*/
	2,		/* 0x0004	WARNING:   This array 		*/
	2,		/* 0x0005	WARNING:   corresponds 		*/
	3,		/* 0x0006	WARNING:   *exactly*		*/
	2,		/* 0x0007	WARNING:   to the ATA/		*/
	3,		/* 0x0008	WARNING:   ATAPI version	*/
	2,		/* 0x0009	WARNING:   listed in	 	*/
	3,		/* 0x000a	WARNING:   the 		 	*/
	3,		/* 0x000b	WARNING:   minor_str 		*/
	3,		/* 0x000c	WARNING:   array		*/
	4,		/* 0x000d	WARNING:   above.		*/
	4,		/* 0x000e	WARNING:  			*/
	4,		/* 0x000f	WARNING:   if you change 	*/
	4,		/* 0x0010	WARNING:   that one,      	*/
	4,		/* 0x0011	WARNING:   change this one	*/
	4,		/* 0x0012	WARNING:   too!!!        	*/
	5,		/* 0x0013	WARNING:	*/
	4,		/* 0x0014	WARNING:	*/
	5,		/* 0x0015	WARNING:	*/
	5,		/* 0x0016	WARNING:	*/
	4,		/* 0x0017	WARNING:	*/
	6,		/* 0x0018	WARNING:	*/
	6,		/* 0x0019	WARNING:	*/
	0,		/* 0x001a	WARNING:	*/
	6,		/* 0x001b	WARNING:	*/
	6,		/* 0x001c	WARNING:	*/
	0		/* 0x001d-0xfffe    		*/
};

/* words 82-84: cmds/feats supported */
#define CMDS_W82		0x77ff  /* word 82: defined command locations*/
#define CMDS_W83		0x3fff  /* word 83: defined command locations*/
#define CMDS_W84		0x002f  /* word 83: defined command locations*/
#define SUPPORT_48_BIT		0x0400
#define NUM_CMD_FEAT_STR	48

#ifdef CONFIG_FEATURE_HDPARM_GET_IDENTITY
static const char *cmd_feat_str[] = {
	"",							/* word 82 bit 15: obsolete  */
	"NOP cmd",					/* word 82 bit 14 */
	"READ BUFFER cmd",			/* word 82 bit 13 */
	"WRITE BUFFER cmd",			/* word 82 bit 12 */
	"",							/* word 82 bit 11: obsolete  */
	"Host Protected Area feature set",	/* word 82 bit 10 */
	"DEVICE RESET cmd",					/* word 82 bit  9 */
	"SERVICE interrupt",				/* word 82 bit  8 */
	"Release interrupt",				/* word 82 bit  7 */
	"Look-ahead",						/* word 82 bit  6 */
	"Write cache",						/* word 82 bit  5 */
	"PACKET command feature set",		/* word 82 bit  4 */
	"Power Management feature set",		/* word 82 bit  3 */
	"Removable Media feature set",		/* word 82 bit  2 */
	"Security Mode feature set",		/* word 82 bit  1 */
	"SMART feature set",				/* word 82 bit  0 */
										/* --------------*/
	"",									/* word 83 bit 15: !valid bit */
	"",									/* word 83 bit 14:  valid bit */
	"FLUSH CACHE EXT command",			/* word 83 bit 13 */
	"Mandatory FLUSH CACHE command ",	/* word 83 bit 12 */
	"Device Configuration Overlay feature set ",
	"48-bit Address feature set ",		/* word 83 bit 10 */
	"",
	"SET MAX security extension",		/* word 83 bit  8 */
	"Address Offset Reserved Area Boot",	/* word 83 bit  7 */
	"SET FEATURES subcommand required to spinup after power up",
	"Power-Up In Standby feature set",	/* word 83 bit  5 */
	"Removable Media Status Notification feature set",
	"Adv. Power Management feature set",/* word 83 bit  3 */
	"CFA feature set",					/* word 83 bit  2 */
	"READ/WRITE DMA QUEUED",			/* word 83 bit  1 */
	"DOWNLOAD MICROCODE cmd", 			/* word 83 bit  0 */
						/* --------------*/
	"",					/* word 84 bit 15: !valid bit */
	"",					/* word 84 bit 14:  valid bit */
	"",					/* word 84 bit 13:  reserved */
	"",					/* word 84 bit 12:  reserved */
	"",					/* word 84 bit 11:  reserved */
	"",					/* word 84 bit 10:  reserved */
	"",					/* word 84 bit  9:  reserved */
	"",					/* word 84 bit  8:  reserved */
	"",					/* word 84 bit  7:  reserved */
	"",					/* word 84 bit  6:  reserved */
	"General Purpose Logging feature set",	/* word 84 bit  5 */
	"",					/* word 84 bit  4:  reserved */
	"Media Card Pass Through Command feature set ",
	"Media serial number ",			/* word 84 bit  2 */
	"SMART self-test ",				/* word 84 bit  1 */
	"SMART error logging "			/* word 84 bit  0 */
};
#endif


/* words 85-87: cmds/feats enabled */
/* use cmd_feat_str[] to display what commands and features have
 * been enabled with words 85-87
 */

/* words 89, 90, SECU ERASE TIME */
#define ERASE_BITS		0x00ff

/* word 92: master password revision */
/* NOVAL_0 or  NOVAL_1 means no support for master password revision */

/* word 93: hw reset result */
#define CBLID			0x2000  /* CBLID status */
#define RST0			0x0001  /* 1=reset to device #0 */
#define DEV_DET			0x0006  /* how device num determined */
#define JUMPER_VAL		0x0002  /* device num determined by jumper */
#define CSEL_VAL		0x0004  /* device num determined by CSEL_VAL */


/* word 127: removable media status notification feature set support */
#define RM_STAT_BITS 		0x0003
#define RM_STAT_SUP		0x0001

/* word 128: security */
#define SECU_ENABLED	0x0002
#define SECU_LEVEL		0x0010
#define NUM_SECU_STR	6
#ifdef CONFIG_FEATURE_HDPARM_GET_IDENTITY
static const char *secu_str[] = {
	"supported",			/* word 128, bit 0 */
	"enabled",				/* word 128, bit 1 */
	"locked",				/* word 128, bit 2 */
	"frozen",				/* word 128, bit 3 */
	"expired: security count",	/* word 128, bit 4 */
	"supported: enhanced erase"	/* word 128, bit 5 */
};
#endif

/* word 160: CFA power mode */
#define VALID_W160			0x8000  /* 1=word valid */
#define PWR_MODE_REQ		0x2000  /* 1=CFA power mode req'd by some cmds*/
#define PWR_MODE_OFF		0x1000  /* 1=CFA power moded disabled */
#define MAX_AMPS			0x0fff  /* value = max current in ma */

/* word 255: integrity */
#define SIG				0x00ff  /* signature location */
#define SIG_VAL			0x00A5  /* signature value */

#define TIMING_MB		64
#define TIMING_BUF_MB		1
#define TIMING_BUF_BYTES	(TIMING_BUF_MB * 1024 * 1024)
#define TIMING_BUF_COUNT	(timing_MB / TIMING_BUF_MB)
#define BUFCACHE_FACTOR		2


#ifndef MAX_DISK_NUM
#define MAX_DISK_NUM		8
#endif

#define APM_LEVEL			250		//高级电源管理级别
#define STANDBY_TIMEOUT		10		//挂起超时时间

#define ACTIVE				1		//活动
#define IDLE				2		//空闲
#define STANDBY				3		//挂起
#define	SLEEP				4		//睡眠

#define HD_POWER_MANAGE_PRIORITY	-10

//硬盘电源信息
typedef struct __HD_POWER_INFO
{
	unsigned int hd_no;				//硬盘号, 0-7
	char hd_name[32];				//硬盘名字
	unsigned int APM_flag;			//高级电源管理标志, 1-允许, 0-不允许
	unsigned int APM_level;			//高级电源管理级别: 1-255
	unsigned int cur_power_status;	//当前硬盘的状态: 1-活动, 2-空闲, 3-挂起, 4-睡眠
	unsigned int standby_timeout;	//挂起超时时间, 单位: 分钟
	unsigned int reserved[8];		//备用
} HD_POWER_INFO;

//硬盘电源管理信息
typedef struct __HD_POWER_MANAGE_INFO
{
	int hd_number;
	int cur_active_disk_no;			//当前活动盘, 0-7 为盘号, -1 则说明无当前活动盘,
									//可能是系统无硬盘或系统的硬盘都处于非活动状态
	int prev_active_disk_no;		//预激活盘号, 0-7
	unsigned int active_disk_num;	//当前活动盘数, 0-7
	unsigned int idle_disk_num;		//当前空闲盘数, 0-7
	unsigned int standby_disk_num;	//当前挂起盘数, 0-7
	unsigned int sleep_disk_num;	//当前睡眠盘数, 0-7
	HD_POWER_INFO hd_power_info[MAX_DISK_NUM];	//硬盘电源信息
} HD_POWER_MANAGE_INFO;

#endif


