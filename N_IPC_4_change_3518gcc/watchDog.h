#ifndef __WATCHDOG_H_
#define __WATCHDOG_H_

int watchDogModuleInit();
int watchDogModuleDeInit();

int watchDogStart();
int watchDogStop();
int watchDogPause();
int watchDogResume();

#endif
