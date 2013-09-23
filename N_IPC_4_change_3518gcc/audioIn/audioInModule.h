#ifndef __AIN_MODULE_H_
#define __AIN_MODULE_H_

#define MAX_AIN_CHANNEL 1

typedef struct
{
	unsigned int nSample;
	unsigned int nChannel;
	unsigned int nBitsPerChannel;
}AIN_PARAM;
#if 0//G726
/*-------------------------------------------*/
//mody by lv start add------------
#if 1
#define AUDIO_SAMPLE_SIZE      			40
/*! Bitstream handler state */
typedef struct bitstream_state_s
{
	/*! The bit stream. */
	unsigned int bitstream;
	/*! The residual bits in bitstream. */
	int residue;
}bitstream_state_t;

typedef struct g726_state_s g726_state_t;
typedef short (*g726_decoder_func_t)(g726_state_t *s, unsigned char code);
typedef unsigned char (*g726_encoder_func_t)(g726_state_t *s, short amp);


/*!
* The following is the definition of the state structure
* used by the G.726 encoder and decoder to preserve their internal
* state between successive calls.  The meanings of the majority
* of the state structure fields are explained in detail in the
* CCITT Recommendation G.726.  The field names are essentially indentical
* to variable names in the bit level description of the coding algorithm
* included in this recommendation.
*/
struct g726_state_s
{
	/*! The bit rate */
	int rate;
	/*! The external coding, for tandem operation */
	//int ext_coding;
	/*! The number of bits per sample */
	int bits_per_sample;
	/*! One of the G.726_PACKING_xxx options */
	//int packing;

	/*! Locked or steady state step size multiplier. */
	int yl;
	/*! Unlocked or non-steady state step size multiplier. */
	short yu;
	/*! short term energy estimate. */
	short dms;
	/*! Long term energy estimate. */
	short dml;
	/*! Linear weighting coefficient of 'yl' and 'yu'. */
	short ap;

	/*! Coefficients of pole portion of prediction filter. */
	short a[2];
	/*! Coefficients of zero portion of prediction filter. */
	short b[6];
	/*! Signs of previous two samples of a partially reconstructed signal. */
	short pk[2];
	/*! Previous 6 samples of the quantized difference signal represented in
	an internal floating point format. */
	short dq[6];
	/*! Previous 2 samples of the quantized difference signal represented in an
	internal floating point format. */
	short sr[2];
	/*! Delayed tone detect */
	int td;

	/*! \brief The bit stream processing context. */
	bitstream_state_t bs;

	/*! \brief The current encoder function. */
	g726_encoder_func_t enc_func;
	/*! \brief The current decoder function. */
	g726_decoder_func_t dec_func;
};

/*
* Maps G.726_16 code word to reconstructed scale factor normalized log
* magnitude values.
*/
static const int g726_16_dqlntab[4] =
{
	116, 365, 365, 116
};

/* Maps G.726_16 code word to log of scale factor multiplier. */
static const int g726_16_witab[4] =
{
	-704, 14048, 14048, -704
};

/*
* Maps G.726_16 code words to a set of values whose long and short
* term averages are computed and then compared to give an indication
* how stationary (steady state) the signal is.
*/
static const int g726_16_fitab[4] =
{
	0x000, 0xE00, 0xE00, 0x000
};

/*
* Maps G.726_24 code word to reconstructed scale factor normalized log
* magnitude values.
*/
static const int g726_24_dqlntab[8] =
{
	-2048, 135, 273, 373, 373, 273, 135, -2048
};

/* Maps G.726_24 code word to log of scale factor multiplier. */
static const int g726_24_witab[8] =
{
	-128, 960, 4384, 18624, 18624, 4384, 960, -128
};

/*
* Maps G.726_24 code words to a set of values whose long and short
* term averages are computed and then compared to give an indication
* how stationary (steady state) the signal is.
*/
static const int g726_24_fitab[8] =
{
	0x000, 0x200, 0x400, 0xE00, 0xE00, 0x400, 0x200, 0x000
};

/*
* Maps G.726_32 code word to reconstructed scale factor normalized log
* magnitude values.
*/
static const int g726_32_dqlntab[16] =
{
	-2048,   4, 135, 213, 273, 323, 373,   425,
	425, 373, 323, 273, 213, 135,   4, -2048
};

/* Maps G.726_32 code word to log of scale factor multiplier. */
static const int g726_32_witab[16] =
{
	-384,   576,  1312,  2048,  3584,  6336, 11360, 35904,
	35904, 11360,  6336,  3584,  2048,  1312,   576,  -384
};

/*
* Maps G.726_32 code words to a set of values whose long and short
* term averages are computed and then compared to give an indication
* how stationary (steady state) the signal is.
*/
static const int g726_32_fitab[16] =
{
	0x000, 0x000, 0x000, 0x200, 0x200, 0x200, 0x600, 0xE00,
	0xE00, 0x600, 0x200, 0x200, 0x200, 0x000, 0x000, 0x000
};

/*
* Maps G.726_40 code word to ructeconstructed scale factor normalized log
* magnitude values.
*/
static const int g726_40_dqlntab[32] =
{
	-2048, -66, 28, 104, 169, 224, 274, 318,
	358, 395, 429, 459, 488, 514, 539, 566,
	566, 539, 514, 488, 459, 429, 395, 358,
	318, 274, 224, 169, 104, 28, -66, -2048
};

/* Maps G.726_40 code word to log of scale factor multiplier. */
static const int g726_40_witab[32] =
{
	448,   448,   768,  1248,  1280,  1312,  1856,  3200,
	4512,  5728,  7008,  8960, 11456, 14080, 16928, 22272,
	22272, 16928, 14080, 11456,  8960,  7008,  5728,  4512,
	3200,  1856,  1312,  1280,  1248,   768,   448,   448
};

/*
* Maps G.726_40 code words to a set of values whose long and short
* term averages are computed and then compared to give an indication
* how stationary (steady state) the signal is.
*/
static const int g726_40_fitab[32] =
{
	0x000, 0x000, 0x000, 0x000, 0x000, 0x200, 0x200, 0x200,
	0x200, 0x200, 0x400, 0x600, 0x800, 0xA00, 0xC00, 0xC00,
	0xC00, 0xC00, 0xA00, 0x800, 0x600, 0x400, 0x200, 0x200,
	0x200, 0x200, 0x200, 0x000, 0x000, 0x000, 0x000, 0x000
};


g726_state_t *g726_init(g726_state_t *s, int bit_rate);

int g726_decode(g726_state_t *s, short amp[], const unsigned char g726_data[], int g726_bytes);

int g726_encode(g726_state_t *s, unsigned char g726_data[], const short amp[], int len);

#endif
//mody by lv end add------------
#endif
#if 1  //G711
#define SUPERCEDED
/*
 * linear2alaw() - Convert a 16-bit linear PCM value to 8-bit A-law
 *
 * linear2alaw() accepts an 16-bit integer and encodes it as A-law data.
 *
 *		Linear Input Code	Compressed Code
 *	------------------------	---------------
 *	0000000wxyza			000wxyz
 *	0000001wxyza			001wxyz
 *	000001wxyzab			010wxyz
 *	00001wxyzabc			011wxyz
 *	0001wxyzabcd			100wxyz
 *	001wxyzabcde			101wxyz
 *	01wxyzabcdef			110wxyz
 *	1wxyzabcdefg			111wxyz
 *
 * For further information see John C. Bellamy's Digital Telephony, 1982,
 * John Wiley & Sons, pps 98-111 and 472-476.
 */

/* pcm_val is 2's complement (16-bit range) */
unsigned char _af_linear2alaw (short pcm_val);

/*
 * alaw2linear() - Convert an A-law value to 16-bit linear PCM
 *
 */

short _af_alaw2linear (unsigned char a_val);

/*
 * linear2ulaw() - Convert a linear PCM value to u-law
 *
 * In order to simplify the encoding process, the original linear magnitude
 * is biased by adding 33 which shifts the encoding range from (0 - 8158) to
 * (33 - 8191). The result can be seen in the following encoding table:
 *
 *	Biased Linear Input Code	Compressed Code
 *	------------------------	---------------
 *	00000001wxyza			000wxyz
 *	0000001wxyzab			001wxyz
 *	000001wxyzabc			010wxyz
 *	00001wxyzabcd			011wxyz
 *	0001wxyzabcde			100wxyz
 *	001wxyzabcdef			101wxyz
 *	01wxyzabcdefg			110wxyz
 *	1wxyzabcdefgh			111wxyz
 *
 * Each biased linear code has a leading 1 which identifies the segment
 * number. The value of the segment number is equal to 7 minus the number
 * of leading 0's. The quantization interval is directly available as the
 * four bits wxyz.  * The trailing bits (a - h) are ignored.
 *
 * Ordinarily the complement of the resulting code word is used for
 * transmission, and so the code word is complemented before it is returned.
 *
 * For further information see John C. Bellamy's Digital Telephony, 1982,
 * John Wiley & Sons, pps 98-111 and 472-476.
 */

/* pcm_val is 2's complement (16-bit range) */
unsigned char _af_linear2ulaw (short pcm_val);

/*
 * ulaw2linear() - Convert a u-law value to 16-bit linear PCM
 *
 * First, a biased linear code is derived from the code word. An unbiased
 * output can then be obtained by subtracting 33 from the biased code.
 *
 * Note that this function expects to be passed the complement of the
 * original code word. This is in keeping with ISDN conventions.
 */

short _af_ulaw2linear (unsigned char u_val);

void G711Encoder(short *pcm,unsigned char *code,int size,int lawflag);
void G711Decoder(short *pcm,unsigned char *code,int size,int lawflag);
#endif
typedef struct ainModuleInfo  *ainModuleInfo_t;

typedef int (*ainOpenFun_t)(int nChannel);
typedef int (*ainCloseFun_t)(int nChannel);
typedef int (*ainSetupFun_t)(int nChannel, void *param);
typedef int (*ainGetSetupFun_t)(int nChannel, void *param);
typedef int (*ainStartFun_t)(int nChannel);
typedef int (*ainStopFun_t)(int nChannel);
typedef int (*ainGetStreamFun_t)(int nChannel, void *stream, int *size);
typedef int (*ainReleaseStreamFun_t)(int nChannel);

struct ainModuleInfo 
{
    ainOpenFun_t				open;
    ainCloseFun_t				close;
    ainSetupFun_t				setup;
    ainGetSetupFun_t			getSetup;
    ainStartFun_t				start;
    ainStopFun_t				stop;
    ainGetStreamFun_t			getStream;
    ainReleaseStreamFun_t		releaseStream;

};

#endif
