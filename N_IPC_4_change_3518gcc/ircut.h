#ifndef __IRCUT_H_
#define __IRCUT_H_

int ircutModuleInit();
int ircutModuleDeInit();

int ircutStart();
int ircutStop();
int ircutPause();
int ircutResume();

typedef enum 
{

 SET_IR_CUT_P,
 CLR_IR_CUT_P,
 SET_IR_CUT_N,
 CLR_IR_CUT_N,
 IR_CUT_OPT_ERRO,
	
}CMD_IR_CUT_OPT;

#endif
