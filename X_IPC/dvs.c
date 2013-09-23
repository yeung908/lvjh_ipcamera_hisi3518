#include <stdlib.h>
#include <stdio.h>

#include "dvsInit.h"
#include "sysCheck/sysCheck.h"

int main()
{
    int nRet = -1;
    int nFlag = 0;

    DeleteTempFile();

    SYSCHECK_ReadFlash();
    nRet = dvsInit();
    if (!nRet)
    {
        SYSCHECK_AddAppRunCount();			// Add the code by lvjh, 2006-11-11
        SYSCHECK_SetAppFailedCount(0);
        SYSCHECK_SetAppStatus(APP_OK);
        SYSCHECK_WriteFlash();
    }
    while (1)
    {
        sleep(30);
    }

    return 0;
}

