#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>

#include "audioInModule.h"

#include "hi_type.h"
#include "hi_debug.h"
#include "hi_comm_sys.h"
#include "hi_comm_aio.h"
#include "hi_comm_aenc.h"
#include "hi_comm_adec.h"
#include "mpi_sys.h"
#include "mpi_ai.h"
//#include <faac.h>

static int g_Hi3511_ain_pub_attr_flag = 0;
static int g_Hi3511_ain_open_flag = 0;
static int g_Hi3511_ain_standard = 0;
#if 1   //G711
/*
 * g711.c
 *
 * u-law, A-law and linear PCM conversions.
 */
 typedef struct
{
	unsigned long long u64TimeStamp;
	unsigned int u32FrameNum;
	unsigned int u32Len;

}AUDIO_HEADER;

#define	SIGN_BIT	(0x80)		/* Sign bit for a A-law byte. */
#define	QUANT_MASK	(0xf)		/* Quantization field mask. */
#define	NSEGS		(8)		/* Number of A-law segments. */
#define	SEG_SHIFT	(4)		/* Left shift for segment number. */
#define	SEG_MASK	(0x70)		/* Segment field mask. */

/* copy from CCITT G.711 specifications */
unsigned char _u2a[128] = {			/* u- to A-law conversions */
	1,	1,	2,	2,	3,	3,	4,	4,
	5,	5,	6,	6,	7,	7,	8,	8,
	9,	10,	11,	12,	13,	14,	15,	16,
	17,	18,	19,	20,	21,	22,	23,	24,
	25,	27,	29,	31,	33,	34,	35,	36,
	37,	38,	39,	40,	41,	42,	43,	44,
	46,	48,	49,	50,	51,	52,	53,	54,
	55,	56,	57,	58,	59,	60,	61,	62,
	64,	65,	66,	67,	68,	69,	70,	71,
	72,	73,	74,	75,	76,	77,	78,	79,
	81,	82,	83,	84,	85,	86,	87,	88,
	89,	90,	91,	92,	93,	94,	95,	96,
	97,	98,	99,	100,	101,	102,	103,	104,
	105,	106,	107,	108,	109,	110,	111,	112,
	113,	114,	115,	116,	117,	118,	119,	120,
	121,	122,	123,	124,	125,	126,	127,	128};

unsigned char _a2u[128] = {			/* A- to u-law conversions */
	1,	3,	5,	7,	9,	11,	13,	15,
	16,	17,	18,	19,	20,	21,	22,	23,
	24,	25,	26,	27,	28,	29,	30,	31,
	32,	32,	33,	33,	34,	34,	35,	35,
	36,	37,	38,	39,	40,	41,	42,	43,
	44,	45,	46,	47,	48,	48,	49,	49,
	50,	51,	52,	53,	54,	55,	56,	57,
	58,	59,	60,	61,	62,	63,	64,	64,
	65,	66,	67,	68,	69,	70,	71,	72,
	73,	74,	75,	76,	77,	78,	79,	79,
	80,	81,	82,	83,	84,	85,	86,	87,
	88,	89,	90,	91,	92,	93,	94,	95,
	96,	97,	98,	99,	100,	101,	102,	103,
	104,	105,	106,	107,	108,	109,	110,	111,
	112,	113,	114,	115,	116,	117,	118,	119,
	120,	121,	122,	123,	124,	125,	126,	127};

/* see libst.h */
#ifdef	SUPERCEDED

static short seg_end[8] = {0xFF, 0x1FF, 0x3FF, 0x7FF,
			    0xFFF, 0x1FFF, 0x3FFF, 0x7FFF};

static short
search(short val, short *table, short size)
{
	short		i;

	for (i = 0; i < size; i++) {
		if (val <= *table++)
			return (i);
	}
	return (size);
}

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
unsigned char
_af_linear2alaw(short pcm_val)
	/* 2's complement (16-bit range) */
{
	short		mask;
	short		seg;
	unsigned char	aval;

	if (pcm_val >= 0) {
		mask = 0xD5;		/* sign (7th) bit = 1 */
	} else {
		mask = 0x55;		/* sign bit = 0 */
		pcm_val = -pcm_val-1;/* - 8;*/  
		if(pcm_val<0)pcm_val = 32767;
	}

	/* Convert the scaled magnitude to segment number. */
	seg = search(pcm_val, seg_end, 8);

	/* Combine the sign, segment, and quantization bits. */

	if (seg >= 8)		/* out of range, return maximum value. */
		return (0x7F ^ mask);
	else {
		aval = seg << SEG_SHIFT;
		if (seg < 2)
			aval |= (pcm_val >> 4) & QUANT_MASK;
		else
			aval |= (pcm_val >> (seg + 3)) & QUANT_MASK;
		return (aval ^ mask);
	}
}

/*
 * alaw2linear() - Convert an A-law value to 16-bit linear PCM
 *
 */
short
_af_alaw2linear(	unsigned char a_val)
	
{
	short		t;
	short		seg;

	a_val ^= 0x55;

	t = (a_val & QUANT_MASK) << 4;
	seg = ((unsigned)a_val & SEG_MASK) >> SEG_SHIFT;
	switch (seg) {
	case 0:
		t += 8;
		break;
	case 1:
		t += 0x108;
		break;
	default:
		t += 0x108;
		t <<= seg - 1;
	}
	return ((a_val & SIGN_BIT) ? t : -t);
}

#define	BIAS		(0x84)		/* Bias for linear code. */

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

/* 2's complement (16-bit range) */
unsigned char _af_linear2ulaw (short pcm_val)
{
	short		mask;
	short		seg;
	unsigned char	uval;

	/* Get the sign and the magnitude of the value. */
	if (pcm_val < 0) {
		pcm_val = BIAS - pcm_val;
		mask = 0x7F;
	} else {
		pcm_val += BIAS;
		mask = 0xFF;
	}

	/* Convert the scaled magnitude to segment number. */
	seg = search(pcm_val, seg_end, 8);

	/*
	 * Combine the sign, segment, quantization bits;
	 * and complement the code word.
	 */
	if (seg >= 8)		/* out of range, return maximum value. */
		return (0x7F ^ mask);
	else {
		uval = (seg << 4) | ((pcm_val >> (seg + 3)) & 0xF);
		return (uval ^ mask);
	}

}

/*
 * ulaw2linear() - Convert a u-law value to 16-bit linear PCM
 *
 * First, a biased linear code is derived from the code word. An unbiased
 * output can then be obtained by subtracting 33 from the biased code.
 *
 * Note that this function expects to be passed the complement of the
 * original code word. This is in keeping with ISDN conventions.
 */
short _af_ulaw2linear (unsigned char u_val)
{
	short		t;

	/* Complement to obtain normal u-law value. */
	u_val = ~u_val;

	/*
	 * Extract and bias the quantization bits. Then
	 * shift up by the segment number and subtract out the bias.
	 */
	t = ((u_val & QUANT_MASK) << 3) + BIAS;
	t <<= ((unsigned)u_val & SEG_MASK) >> SEG_SHIFT;

	return ((u_val & SIGN_BIT) ? (BIAS - t) : (t - BIAS));
}

#endif

/* A-law to u-law conversion */
unsigned char zj_alaw2ulaw(unsigned char aval)
{
	aval &= 0xff;
	return ((aval & 0x80) ? (0xFF ^ _a2u[aval ^ 0xD5]) :
	    (0x7F ^ _a2u[aval ^ 0x55]));
}

/* u-law to A-law conversion */
unsigned char zj_ulaw2alaw(unsigned char uval)
{
	uval &= 0xff;
	return ((uval & 0x80) ? (0xD5 ^ (_u2a[0xFF ^ uval] - 1)) :
	    (0x55 ^ (_u2a[0x7F ^ uval] - 1)));
}


//lawflag :0-A,1-U
void G711Encoder(short *pcm,unsigned char *code,int size,int lawflag)
{
	int i;

	if(lawflag==0){
		for(i=0;i<size;i++){
			code[i]=_af_linear2alaw(pcm[i]);
		///if(abs(pcm[i])>1000)	printf("%d\n",pcm[i]);
		}
	}
	else{
		for(i=0;i<size;i++){
			code[i]=_af_linear2ulaw(pcm[i]);
		}
	}
}


void G711Decoder(short *pcm,unsigned char *code,int size,int lawflag)
{
	int i;

	if(lawflag==0){
		for(i=0;i<size;i++){
			pcm[i]=_af_alaw2linear(code[i]);
		}
	}
	else{
		for(i=0;i<size;i++){
			pcm[i]=_af_ulaw2linear(code[i]);
		}
	}
}
#endif


#if 0  //g726
/*-------------see old program-----------*/
//mody by lv start add-----------------------------------------------------
FILE *g_pFile;
FILE *g_pOutFile;
g726_state_t *g_state726_32 = NULL; //for g726_16 



typedef unsigned long   ULONG;
typedef unsigned int    UINT;
typedef unsigned char   BYTE;
typedef char            _TCHAR;


#if 0
static FILE* g_fpOut;
faacEncHandle g_hEncoder;
static int g_nPCMBufferSize = 0;
int   g_nBytesRead = 0;
static int  g_nInputSamples = 0;
ULONG 	g_nMaxOutputBytes = 134931904;
BYTE* g_pbPCMBuffer;
BYTE* g_pbAACBuffer;
#endif


#if 1
typedef struct
{
	unsigned long long u64TimeStamp;
	unsigned int u32FrameNum;
	unsigned int u32Len;

}AUDIO_HEADER;


static const int qtab_726_16[1] =
{
    261
};

static const int qtab_726_24[3] =
{
    8, 218, 331
};

static const int qtab_726_32[7] =
{
    -124, 80, 178, 246, 300, 349, 400
};

static const int qtab_726_40[15] =
{
    -122, -16,  68, 139, 198, 250, 298, 339,
     378, 413, 445, 475, 502, 528, 553
};


static __inline int top_bit(unsigned int bits)
{
#if defined(__i386__)  ||  defined(__x86_64__)
	int res;

	__asm__ (" xorl %[res],%[res];\n"
		" decl %[res];\n"
		" bsrl %[bits],%[res]\n"
		: [res] "=&r" (res)
		: [bits] "rm" (bits));
	return res;
#elif defined(__ppc__)  ||   defined(__powerpc__)
	int res;

	__asm__ ("cntlzw %[res],%[bits];\n"
		: [res] "=&r" (res)
		: [bits] "r" (bits));
	return 31 - res;
#elif defined(_M_IX86) // Visual Studio x86
	__asm
	{
		xor eax, eax
			dec eax
			bsr eax, bits
	}
#else
	int res;

	if (bits == 0)
		return -1;
	res = 0;
	if (bits & 0xFFFF0000)
	{
		bits &= 0xFFFF0000;
		res += 16;
	}
	if (bits & 0xFF00FF00)
	{
		bits &= 0xFF00FF00;
		res += 8;
	}
	if (bits & 0xF0F0F0F0)
	{
		bits &= 0xF0F0F0F0;
		res += 4;
	}
	if (bits & 0xCCCCCCCC)
	{
		bits &= 0xCCCCCCCC;
		res += 2;
	}
	if (bits & 0xAAAAAAAA)
	{
		bits &= 0xAAAAAAAA;
		res += 1;
	}
	return res;
#endif
}


static bitstream_state_t *bitstream_init(bitstream_state_t *s)
{
	if (s == NULL)
		return NULL;
	s->bitstream = 0;
	s->residue = 0;
	return s;
}

/*
 * Given a raw sample, 'd', of the difference signal and a
 * quantization step size scale factor, 'y', this routine returns the
 * ADPCM codeword to which that sample gets quantized.  The step
 * size scale factor division operation is done in the log base 2 domain
 * as a subtraction.
 */
static short quantize(int d,                  /* Raw difference signal sample */
                        int y,                  /* Step size multiplier */
                        const int table[],     /* quantization table */
                        int quantizer_states)   /* table size of short integers */
{
    short dqm;    /* Magnitude of 'd' */
    short exp;    /* Integer part of base 2 log of 'd' */
    short mant;   /* Fractional part of base 2 log */
    short dl;     /* Log of magnitude of 'd' */
    short dln;    /* Step size scale factor normalized log */
    int i;
    int size;

    /*
     * LOG
     *
     * Compute base 2 log of 'd', and store in 'dl'.
     */
    dqm = (short) abs(d);
    exp = (short) (top_bit(dqm >> 1) + 1);
    /* Fractional portion. */
    mant = ((dqm << 7) >> exp) & 0x7F;
    dl = (exp << 7) + mant;

    /*
     * SUBTB
     *
     * "Divide" by step size multiplier.
     */
    dln = dl - (short) (y >> 2);

    /*
     * QUAN
     *
     * Search for codword i for 'dln'.
     */
    size = (quantizer_states - 1) >> 1;
    for (i = 0;  i < size;  i++)
    {
        if (dln < table[i])
            break;
    }
    if (d < 0)
    {
        /* Take 1's complement of i */
        return (short) ((size << 1) + 1 - i);
    }
    if (i == 0  &&  (quantizer_states & 1))
    {
        /* Zero is only valid if there are an even number of states, so
           take the 1's complement if the code is zero. */
        return (short) quantizer_states;
    }
    return (short) i;
}
/*- End of function --------------------------------------------------------*/


/*
* returns the integer product of the 14-bit integer "an" and
* "floating point" representation (4-bit exponent, 6-bit mantessa) "srn".
*/
static short fmult(short an, short srn)
{
	short anmag;
	short anexp;
	short anmant;
	short wanexp;
	short wanmant;
	short retval;

	anmag = (an > 0)  ?  an  :  ((-an) & 0x1FFF);
	anexp = (short) (top_bit(anmag) - 5);
	anmant = (anmag == 0)  ?  32  :  (anexp >= 0)  ?  (anmag >> anexp)  :  (anmag << -anexp);
	wanexp = anexp + ((srn >> 6) & 0xF) - 13;

	wanmant = (anmant*(srn & 0x3F) + 0x30) >> 4;
	retval = (wanexp >= 0)  ?  ((wanmant << wanexp) & 0x7FFF)  :  (wanmant >> -wanexp);

	return (((an ^ srn) < 0)  ?  -retval  :  retval);
}

/*
* Compute the estimated signal from the 6-zero predictor.
*/
static __inline short predictor_zero(g726_state_t *s)
{
	int i;
	int sezi;

	sezi = fmult(s->b[0] >> 2, s->dq[0]);
	/* ACCUM */
	for (i = 1;  i < 6;  i++)
		sezi += fmult(s->b[i] >> 2, s->dq[i]);
	return (short) sezi;
}
/*- End of function --------------------------------------------------------*/

/*
* Computes the estimated signal from the 2-pole predictor.
*/
static __inline short predictor_pole(g726_state_t *s)
{
	return (fmult(s->a[1] >> 2, s->sr[1]) + fmult(s->a[0] >> 2, s->sr[0]));
}

/*
* Computes the quantization step size of the adaptive quantizer.
*/
static int step_size(g726_state_t *s)
{
	int y;
	int dif;
	int al;

	if (s->ap >= 256)
		return s->yu;
	y = s->yl >> 6;
	dif = s->yu - y;
	al = s->ap >> 2;
	if (dif > 0)
		y += (dif*al) >> 6;
	else if (dif < 0)
		y += (dif*al + 0x3F) >> 6;
	return y;
}
/*- End of function --------------------------------------------------------*/

/*
* Returns reconstructed difference signal 'dq' obtained from
* codeword 'i' and quantization step size scale factor 'y'.
* Multiplication is performed in log base 2 domain as addition.
*/
static short reconstruct(int sign,    /* 0 for non-negative value */
						 int dqln,    /* G.72x codeword */
						 int y)       /* Step size multiplier */
{
	short dql;    /* Log of 'dq' magnitude */
	short dex;    /* Integer part of log */
	short dqt;
	short dq;     /* Reconstructed difference signal sample */

	dql = (short) (dqln + (y >> 2));  /* ADDA */

	if (dql < 0)
		return ((sign)  ?  -0x8000  :  0);
	/* ANTILOG */
	dex = (dql >> 7) & 15;
	dqt = 128 + (dql & 127);
	dq = (dqt << 7) >> (14 - dex);
	return ((sign)  ?  (dq - 0x8000)  :  dq);
}
/*- End of function --------------------------------------------------------*/

/*
* updates the state variables for each output code
*/
static void update(g726_state_t *s,
				   int y,       /* quantizer step size */
				   int wi,      /* scale factor multiplier */
				   int fi,      /* for long/short term energies */
				   int dq,      /* quantized prediction difference */
				   int sr,      /* reconstructed signal */
				   int dqsez)   /* difference from 2-pole predictor */
{
	short mag;
	short exp;
	short a2p;        /* LIMC */
	short a1ul;       /* UPA1 */
	short pks1;       /* UPA2 */
	short fa1;
	short ylint;
	short dqthr;
	short ylfrac;
	short thr;
	short pk0;
	int i;
	int tr;

	a2p = 0;
	/* Needed in updating predictor poles */
	pk0 = (dqsez < 0)  ?  1  :  0;

	/* prediction difference magnitude */
	mag = (short) (dq & 0x7FFF);
	/* TRANS */
	ylint = (short) (s->yl >> 15);            /* exponent part of yl */
	ylfrac = (short) ((s->yl >> 10) & 0x1F);  /* fractional part of yl */
	/* Limit threshold to 31 << 10 */
	thr = (ylint > 9)  ?  (31 << 10)  :  ((32 + ylfrac) << ylint);
	dqthr = (thr + (thr >> 1)) >> 1;            /* dqthr = 0.75 * thr */
	if (!s->td)                                 /* signal supposed voice */
		tr = 0;
	else if (mag <= dqthr)                      /* supposed data, but small mag */
		tr = 0;                             /* treated as voice */
	else                                        /* signal is data (modem) */
		tr = 1;

	/*
	* Quantizer scale factor adaptation.
	*/

	/* FUNCTW & FILTD & DELAY */
	/* update non-steady state step size multiplier */
	s->yu = (short) (y + ((wi - y) >> 5));

	/* LIMB */
	if (s->yu < 544)
		s->yu = 544;
	else if (s->yu > 5120)
		s->yu = 5120;

	/* FILTE & DELAY */
	/* update steady state step size multiplier */
	s->yl += s->yu + ((-s->yl) >> 6);

	/*
	* Adaptive predictor coefficients.
	*/
	if (tr)
	{
		/* Reset the a's and b's for a modem signal */
		s->a[0] = 0;
		s->a[1] = 0;
		s->b[0] = 0;
		s->b[1] = 0;
		s->b[2] = 0;
		s->b[3] = 0;
		s->b[4] = 0;
		s->b[5] = 0;
	}
	else
	{
		/* Update the a's and b's */
		/* UPA2 */
		pks1 = pk0 ^ s->pk[0];

		/* Update predictor pole a[1] */
		a2p = s->a[1] - (s->a[1] >> 7);
		if (dqsez != 0)
		{
			fa1 = (pks1)  ?  s->a[0]  :  -s->a[0];
			/* a2p = function of fa1 */
			if (fa1 < -8191)
				a2p -= 0x100;
			else if (fa1 > 8191)
				a2p += 0xFF;
			else
				a2p += fa1 >> 5;

			if (pk0 ^ s->pk[1])
			{
				/* LIMC */
				if (a2p <= -12160)
					a2p = -12288;
				else if (a2p >= 12416)
					a2p = 12288;
				else
					a2p -= 0x80;
			}
			else if (a2p <= -12416)
				a2p = -12288;
			else if (a2p >= 12160)
				a2p = 12288;
			else
				a2p += 0x80;
		}

		/* TRIGB & DELAY */
		s->a[1] = a2p;

		/* UPA1 */
		/* Update predictor pole a[0] */
		s->a[0] -= s->a[0] >> 8;
		if (dqsez != 0)
		{
			if (pks1 == 0)
				s->a[0] += 192;
			else
				s->a[0] -= 192;
		}
		/* LIMD */
		a1ul = 15360 - a2p;
		if (s->a[0] < -a1ul)
			s->a[0] = -a1ul;
		else if (s->a[0] > a1ul)
			s->a[0] = a1ul;

		/* UPB : update predictor zeros b[6] */
		for (i = 0;  i < 6;  i++)
		{
			/* Distinguish 40Kbps mode from the others */
			s->b[i] -= s->b[i] >> ((s->bits_per_sample == 5)  ?  9  :  8);
			if (dq & 0x7FFF)
			{
				/* XOR */
				if ((dq ^ s->dq[i]) >= 0)
					s->b[i] += 128;
				else
					s->b[i] -= 128;
			}
		}
	}

	for (i = 5;  i > 0;  i--)
		s->dq[i] = s->dq[i - 1];
	/* FLOAT A : convert dq[0] to 4-bit exp, 6-bit mantissa f.p. */
	if (mag == 0)
	{
		s->dq[0] = (dq >= 0)  ?  0x20  :  0xFC20;
	}
	else
	{
		exp = (short) (top_bit(mag) + 1);
		s->dq[0] = (dq >= 0)
			?  ((exp << 6) + ((mag << 6) >> exp))
			:  ((exp << 6) + ((mag << 6) >> exp) - 0x400);
	}

	s->sr[1] = s->sr[0];
	/* FLOAT B : convert sr to 4-bit exp., 6-bit mantissa f.p. */
	if (sr == 0)
	{
		s->sr[0] = 0x20;
	}
	else if (sr > 0)
	{
		exp = (short) (top_bit(sr) + 1);
		s->sr[0] = (short) ((exp << 6) + ((sr << 6) >> exp));
	}
	else if (sr > -32768)
	{
		mag = (short) -sr;
		exp = (short) (top_bit(mag) + 1);
		s->sr[0] =  (exp << 6) + ((mag << 6) >> exp) - 0x400;
	}
	else
	{
		s->sr[0] = (short) 0xFC20;
	}

	/* DELAY A */
	s->pk[1] = s->pk[0];
	s->pk[0] = pk0;

	/* TONE */
	if (tr)                 /* this sample has been treated as data */
		s->td = 0;      /* next one will be treated as voice */
	else if (a2p < -11776)  /* small sample-to-sample correlation */
		s->td = 1;       /* signal may be data */
	else                    /* signal is voice */
		s->td = 0;

	/* Adaptation speed control. */
	/* FILTA */
	s->dms += ((short) fi - s->dms) >> 5;
	/* FILTB */
	s->dml += (((short) (fi << 2) - s->dml) >> 7);

	if (tr)
		s->ap = 256;
	else if (y < 1536)                      /* SUBTC */
		s->ap += (0x200 - s->ap) >> 4;
	else if (s->td)
		s->ap += (0x200 - s->ap) >> 4;
	else if (abs((s->dms << 2) - s->dml) >= (s->dml >> 3))
		s->ap += (0x200 - s->ap) >> 4;
	else
		s->ap += (-s->ap) >> 4;
}

/*
* Decodes a 2-bit CCITT G.726_16 ADPCM code and returns
* the resulting 16-bit linear PCM, A-law or u-law sample value.
*/
static short g726_16_decoder(g726_state_t *s, unsigned char code)
{
	short sezi;
	short sei;
	short se;
	short sr;
	short dq;
	short dqsez;
	int y;

	/* Mask to get proper bits */
	code &= 0x03;
	sezi = predictor_zero(s);
	sei = sezi + predictor_pole(s);

	y = step_size(s);
	dq = reconstruct(code & 2, g726_16_dqlntab[code], y);

	/* Reconstruct the signal */
	se = sei >> 1;
	sr = (dq < 0)  ?  (se - (dq & 0x3FFF))  :  (se + dq);

	/* Pole prediction difference */
	dqsez = sr + (sezi >> 1) - se;

	update(s, y, g726_16_witab[code], g726_16_fitab[code], dq, sr, dqsez);

	return (sr << 2);
}
/*- End of function --------------------------------------------------------*/


/*
 * Encodes a linear PCM, A-law or u-law input sample and returns its 3-bit code.
 */
static unsigned char g726_16_encoder(g726_state_t *s, short amp)
{
    int y;
    short sei;
    short sezi;
    short se;
    short d;
    short sr;
    short dqsez;
    short dq;
    short i;
    
    sezi = predictor_zero(s);
    sei = sezi + predictor_pole(s);
    se = sei >> 1;
    d = amp - se;

    /* Quantize prediction difference */
    y = step_size(s);
    i = quantize(d, y, qtab_726_16, 4);
    dq = reconstruct(i & 2, g726_16_dqlntab[i], y);

    /* Reconstruct the signal */
    sr = (dq < 0)  ?  (se - (dq & 0x3FFF))  :  (se + dq);

    /* Pole prediction difference */
    dqsez = sr + (sezi >> 1) - se;
    
    update(s, y, g726_16_witab[i], g726_16_fitab[i], dq, sr, dqsez);
    return (unsigned char) i;
}

/*
* Decodes a 3-bit CCITT G.726_24 ADPCM code and returns
* the resulting 16-bit linear PCM, A-law or u-law sample value.
*/
static short g726_24_decoder(g726_state_t *s, unsigned char code)
{
	short sezi;
	short sei;
	short se;
	short sr;
	short dq;
	short dqsez;
	int y;

	/* Mask to get proper bits */
	code &= 0x07;
	sezi = predictor_zero(s);
	sei = sezi + predictor_pole(s);

	y = step_size(s);
	dq = reconstruct(code & 4, g726_24_dqlntab[code], y);

	/* Reconstruct the signal */
	se = sei >> 1;
	sr = (dq < 0)  ?  (se - (dq & 0x3FFF))  :  (se + dq);

	/* Pole prediction difference */
	dqsez = sr + (sezi >> 1) - se;

	update(s, y, g726_24_witab[code], g726_24_fitab[code], dq, sr, dqsez);

	return (sr << 2);
}
/*- End of function --------------------------------------------------------*/


/*
 * Encodes a linear PCM, A-law or u-law input sample and returns its 3-bit code.
 */
static unsigned char g726_24_encoder(g726_state_t *s, short amp)
{
    short sei;
    short sezi;
    short se;
    short d;
    short sr;
    short dqsez;
    short dq;
    short i;
    int y;
    
    sezi = predictor_zero(s);
    sei = sezi + predictor_pole(s);
    se = sei >> 1;
    d = amp - se;

    /* Quantize prediction difference */
    y = step_size(s);
    i = quantize(d, y, qtab_726_24, 7);
    dq = reconstruct(i & 4, g726_24_dqlntab[i], y);

    /* Reconstruct the signal */
    sr = (dq < 0)  ?  (se - (dq & 0x3FFF))  :  (se + dq);

    /* Pole prediction difference */
    dqsez = sr + (sezi >> 1) - se;
    
    update(s, y, g726_24_witab[i], g726_24_fitab[i], dq, sr, dqsez);
    return (unsigned char) i;
}


/*
* Decodes a 4-bit CCITT G.726_32 ADPCM code and returns
* the resulting 16-bit linear PCM, A-law or u-law sample value.
*/
static short g726_32_decoder(g726_state_t *s, unsigned char code)
{
	short sezi;
	short sei;
	short se;
	short sr;
	short dq;
	short dqsez;
	int y;

	/* Mask to get proper bits */
	code &= 0x0F;
	sezi = predictor_zero(s);
	sei = sezi + predictor_pole(s);

	y = step_size(s);
	dq = reconstruct(code & 8, g726_32_dqlntab[code], y);

	/* Reconstruct the signal */
	se = sei >> 1;
	sr = (dq < 0)  ?  (se - (dq & 0x3FFF))  :  (se + dq);

	/* Pole prediction difference */
	dqsez = sr + (sezi >> 1) - se;

	update(s, y, g726_32_witab[code], g726_32_fitab[code], dq, sr, dqsez);

	return (sr << 2);
}
/*- End of function --------------------------------------------------------*/

/*
 * Encodes a linear input sample and returns its 4-bit code.
 */
static unsigned char g726_32_encoder(g726_state_t *s, short amp)
{
    short sei;
    short sezi;
    short se;
    short d;
    short sr;
    short dqsez;
    short dq;
    short i;
    int y;
    
    sezi = predictor_zero(s);
    sei = sezi + predictor_pole(s);
    se = sei >> 1;
    d = amp - se;

    /* Quantize the prediction difference */
    y = step_size(s);
    i = quantize(d, y, qtab_726_32, 15);
    dq = reconstruct(i & 8, g726_32_dqlntab[i], y);

    /* Reconstruct the signal */
    sr = (dq < 0)  ?  (se - (dq & 0x3FFF))  :  (se + dq);

    /* Pole prediction difference */
    dqsez = sr + (sezi >> 1) - se;

    update(s, y, g726_32_witab[i], g726_32_fitab[i], dq, sr, dqsez);
    return (unsigned char) i;
}

/*
* Decodes a 5-bit CCITT G.726 40Kbps code and returns
* the resulting 16-bit linear PCM, A-law or u-law sample value.
*/
static short g726_40_decoder(g726_state_t *s, unsigned char code)
{
	short sezi;
	short sei;
	short se;
	short sr;
	short dq;
	short dqsez;
	int y;

	/* Mask to get proper bits */
	code &= 0x1F;
	sezi = predictor_zero(s);
	sei = sezi + predictor_pole(s);

	y = step_size(s);
	dq = reconstruct(code & 0x10, g726_40_dqlntab[code], y);

	/* Reconstruct the signal */
	se = sei >> 1;
	sr = (dq < 0)  ?  (se - (dq & 0x7FFF))  :  (se + dq);

	/* Pole prediction difference */
	dqsez = sr + (sezi >> 1) - se;

	update(s, y, g726_40_witab[code], g726_40_fitab[code], dq, sr, dqsez);

	return (sr << 2);
}
/*- End of function --------------------------------------------------------*/


/*
 * Encodes a 16-bit linear PCM, A-law or u-law input sample and retuens
 * the resulting 5-bit CCITT G.726 40Kbps code.
 */
static unsigned char g726_40_encoder(g726_state_t *s, short amp)
{
    short sei;
    short sezi;
    short se;
    short d;
    short sr;
    short dqsez;
    short dq;
    short i;
    int y;
    
    sezi = predictor_zero(s);
    sei = sezi + predictor_pole(s);
    se = sei >> 1;
    d = amp - se;

    /* Quantize prediction difference */
    y = step_size(s);
    i = quantize(d, y, qtab_726_40, 31);
    dq = reconstruct(i & 0x10, g726_40_dqlntab[i], y);

    /* Reconstruct the signal */
    sr = (dq < 0)  ?  (se - (dq & 0x7FFF))  :  (se + dq);

    /* Pole prediction difference */
    dqsez = sr + (sezi >> 1) - se;

    update(s, y, g726_40_witab[i], g726_40_fitab[i], dq, sr, dqsez);
    return (unsigned char) i;
}

g726_state_t *g726_init(g726_state_t *s, int bit_rate)
{
	int i;

	if (bit_rate != 16000  &&  bit_rate != 24000  &&  bit_rate != 32000  &&  bit_rate != 40000)
		return NULL;

	s->yl = 34816;
	s->yu = 544;
	s->dms = 0;
	s->dml = 0;
	s->ap = 0;
	s->rate = bit_rate;

	for (i = 0; i < 2; i++)
	{
		s->a[i] = 0;
		s->pk[i] = 0;
		s->sr[i] = 32;
	}
	for (i = 0; i < 6; i++)
	{
		s->b[i] = 0;
		s->dq[i] = 32;
	}
	s->td = 0;
	switch (bit_rate)
	{
	case 16000:
		s->enc_func = g726_16_encoder;
		s->dec_func = g726_16_decoder;
		s->bits_per_sample = 2;
		break;
	case 24000:
		s->enc_func = g726_24_encoder;
		s->dec_func = g726_24_decoder;
		s->bits_per_sample = 3;
		break;
	case 32000:
	default:
		s->enc_func = g726_32_encoder;
		s->dec_func = g726_32_decoder;
		s->bits_per_sample = 4;
		break;
	case 40000:
		s->enc_func = g726_40_encoder;
		s->dec_func = g726_40_decoder;
		s->bits_per_sample = 5;
		break;
	}
	bitstream_init(&s->bs);
	return s;
}

int g726_decode(g726_state_t *s,
				short amp[],
				const unsigned char g726_data[],
				int g726_bytes)
{
	int i;
	int samples;
	unsigned char code;
	int sl;

	for (samples = i = 0;  ;  )
	{
		if (s->bs.residue < s->bits_per_sample)
		{
			if (i >= g726_bytes)
				break;
			s->bs.bitstream = (s->bs.bitstream << 8) | g726_data[i++];
			s->bs.residue += 8;
		}
		code = (unsigned char) ((s->bs.bitstream >> (s->bs.residue - s->bits_per_sample)) & ((1 << s->bits_per_sample) - 1));

		s->bs.residue -= s->bits_per_sample;

		sl = s->dec_func(s, code);

		amp[samples++] = (short) sl;
	}
	return samples;
}


int g726_encode(g726_state_t *s,
                unsigned char g726_data[],
                const short amp[],
                int len)
{
    int i;
    int g726_bytes;
    short sl;
    unsigned char code;

    for (g726_bytes = i = 0;  i < len;  i++)
    {
		sl = amp[i] >> 2;

        code = s->enc_func(s, sl);

		s->bs.bitstream = (s->bs.bitstream << s->bits_per_sample) | code;
		s->bs.residue += s->bits_per_sample;
		if (s->bs.residue >= 8)
		{
			g726_data[g726_bytes++] = (unsigned char) ((s->bs.bitstream >> (s->bs.residue - 8)) & 0xFF);
			s->bs.residue -= 8;
		}
		
    }
	
    return g726_bytes;
}

#endif
//mody by lv end add--------------------------------------------------------
#endif

int Hi3511AinOpen(int nChannel)
{
#if 0//G726
	
		g_pFile = fopen("/mnt/mtd/dvs/mobile/tmpfs/g726in.pcm", "w+");
			if(!g_pFile){
				printf("fopen error\n");
			}
		g_state726_32 = (g726_state_t *)malloc(sizeof(g726_state_t));
		g_state726_32 = g726_init(g_state726_32, 8000*4);
	
		g_pOutFile = fopen("/mnt/mtd/dvs/mobile/tmpfs/g726out.726", "w+");
		if(!g_pOutFile){
			printf("fopen error\n");
		}
#endif


	if (nChannel<0 || nChannel>=MAX_AIN_CHANNEL)
	{
		return -1;	
	}		
	
	return 0; 
} 

int Hi3511AinClose(int nChannel)
{
	if (nChannel<0 || nChannel>=MAX_AIN_CHANNEL)
	{
		return -1;	
	}
	
#if 0
		fclose(g_fpOut);
		free(g_pbPCMBuffer);
		free(g_pbAACBuffer);
		faacEncClose(g_hEncoder);
#endif
	
	return 0;
}

HI_S32 SAMPLE_Audio_InitMpp()
{
    HI_S32 s32ret;
    MPP_SYS_CONF_S struSysConf;

    HI_MPI_SYS_Exit();
    
    struSysConf.u32AlignWidth = 64;
    s32ret = HI_MPI_SYS_SetConf(&struSysConf);
    if (HI_SUCCESS != s32ret)
    {
        printf("Set mpi sys config failed!\n");
        return s32ret;
    }
    s32ret = HI_MPI_SYS_Init();
    if (HI_SUCCESS != s32ret)
    {
        printf("Mpi init failed!\n");
        return s32ret;
    }
    
    return HI_SUCCESS;
}


int Hi3511AinSetup(int nChannel, void *param)
{
	HI_S32 s32ret;
	AIO_ATTR_S stAttr;
	
	if (nChannel<0 || nChannel>=MAX_AIN_CHANNEL)
	{
		return -1;	
	}

	stAttr.enBitwidth = AUDIO_BIT_WIDTH_16;
	stAttr.enSamplerate = AUDIO_SAMPLE_RATE_8000;
	stAttr.enSoundmode = AUDIO_SOUND_MODE_MOMO;
	stAttr.enWorkmode = AIO_MODE_I2S_MASTER;// AIO_MODE_I2S_SLAVE
	stAttr.u32EXFlag = 0;	// 1
	stAttr.u32FrmNum = 10;
	stAttr.u32PtNumPerFrm = 160;

	s32ret = HI_MPI_AI_SetPubAttr(nChannel, &stAttr);
	if(HI_SUCCESS != s32ret)
	{
		printf("set ai %d attr err:0x%x\n", nChannel, s32ret);
		return -1;
 	}
    
	s32ret = HI_MPI_AI_Enable(nChannel);
	if(HI_SUCCESS != s32ret)
	{
		printf("enable ai dev %d err:0x%x\n", nChannel, s32ret);
		return -1;
	}

	s32ret = HI_MPI_AI_EnableChn(nChannel, nChannel);
	if (HI_SUCCESS != s32ret)
	{
		printf("enable ai chn %d err:0x%x\n", nChannel, s32ret);
		return -1;
	}
   
	g_Hi3511_ain_open_flag = 1;	
	
	return 0;
}

int Hi3511AinGetSetup(int nChannel, void *param)
{	
	return 0;
}

int Hi3511AinStart(int nChannel)
{	
	return 0;
}

int Hi3511AinStop(int nChannel)
{
	return 0;
}

FILE *pfd_ai = NULL;
int SaveAiFrame(int chn, AUDIO_FRAME_S *pstFrame)
{
    if (!pfd_ai)
    {
        char file[32];
        sprintf(file, "ai_chn%d.data", chn);
        pfd_ai = fopen(file, "w+");
        if (!pfd_ai)
        {
            printf("open file %s for save ai data err\n", file);
            return -1;
        }
    }
    fwrite(pstFrame->aData, 1, pstFrame->u32Len, pfd_ai);
    return 0;
}


int Hi3511AinGetStream(int nChannel, void *stream, int *size)
{
	HI_S32 s32ret;
	AEC_FRAME_S stRefFrame;
	AUDIO_FRAME_S stFrame;

	int aac_size = 0;
	int nRet = 0;
	int nPCMBitSize = 16;
	int i = 0;

	int iRet;
	char ucOutBuff[4096];
	unsigned long currentPosition = 0;
	unsigned long audioSize = 0;
	AUDIO_HEADER *audioHeader;
		
	if (stream==NULL || size==NULL)
	{
		return -1;
	}
	if (nChannel<0 || nChannel>=MAX_AIN_CHANNEL)
	{
		return -1;
	}	
	if (g_Hi3511_ain_open_flag == 0)
	{
		return -1;
	}
	
	// Add the code by lvjh, 2009-04-17
	memset(&stFrame, 0, sizeof(AUDIO_FRAME_S));
	
	s32ret = HI_MPI_AI_GetFrame(0, 0, &stFrame, NULL, HI_IO_BLOCK);
	if (HI_SUCCESS != s32ret)
	{	
		printf("get ai frame err:0x%x ai(%d,%d)\n", s32ret, 0, nChannel);		
		return -1;
	}

	 memcpy(stream, stFrame.aData, stFrame.u32Len);
	 *size = stFrame.u32Len;
	 
	// printf("stFrame.u32Len = %d %d\n", stFrame.u32Len, sizeof(AUDIO_HEADER));

#ifdef  G726
	  while(currentPosition < *size){
		 iRet = g726_encode(g_state726_32, ucOutBuff+audioSize,  (short*)(stream+currentPosition), 80);
		 currentPosition += 160;
		 audioSize += iRet;
	 }
	  audioHeader = (AUDIO_HEADER *)stream;
	  audioHeader->u64TimeStamp = 16000;
	  audioHeader->u32FrameNum = 16000;
	  audioHeader->u32Len = audioSize;
	  memcpy(stream+sizeof(AUDIO_HEADER), ucOutBuff, audioSize);
#else   //G711--zhangjing
	G711Encoder((short *)stFrame.aData,ucOutBuff,stFrame.u32Len/2,0);

	  audioHeader = (AUDIO_HEADER *)stream;
	  audioHeader->u64TimeStamp = 16000;
	  audioHeader->u32FrameNum = 16000;
	  audioHeader->u32Len = stFrame.u32Len/2;
	  memcpy(stream+sizeof(AUDIO_HEADER), ucOutBuff, stFrame.u32Len/2);
#endif
	//fwrite(ucOutBuff, 1, audioSize, g_pOutFile);
 	*size = sizeof(AUDIO_HEADER) + audioHeader->u32Len;
 return 0;
}


int Hi3511AinReleaseStream(int nChannel)
{
	if (nChannel<0 || nChannel>=MAX_AIN_CHANNEL)
	{
		return -1;
	}	
	if (g_Hi3511_ain_open_flag == 0)
	{
		return -1;
	}
	
	return 0;
}

static struct ainModuleInfo Hi3511AinInfoStruct = 
{
	open:			Hi3511AinOpen,
	close:			Hi3511AinClose,
	setup:			Hi3511AinSetup,
	getSetup:			Hi3511AinGetSetup,
	start:			Hi3511AinStart,
	stop:				Hi3511AinStop,
	getStream:		Hi3511AinGetStream,
	releaseStream:		Hi3511AinReleaseStream,
	
};

ainModuleInfo_t Hi3511AinInfo = &Hi3511AinInfoStruct;

