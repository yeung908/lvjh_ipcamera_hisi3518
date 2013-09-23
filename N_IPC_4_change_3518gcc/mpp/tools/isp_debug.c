#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "hi_common.h"
#include "hi_comm_video.h"
#include "hi_comm_sys.h"
#include "hi_comm_vi.h"
#include "hi_comm_vo.h"
#include "mpi_vb.h"
#include "mpi_sys.h"
#include "mpi_vi.h"
#include "mpi_vo.h"
#include "hi_comm_isp.h"
#include "mpi_isp.h"
#include "hi_sns_ctrl.h"



HI_S32 main(int argc, char *argv[])
{
    HI_S32 s32Ret = 0;
    HI_U32 u32Nbit = 0;
    HI_U32 enable = 0;
    HI_U32 u32depth = 10;
    HI_U32 u32SizeHi;
    HI_U32 u32SizeLo;
    HI_U32 u32MemSize;
    
    HI_U32 u32PhyAddr;
    HI_VOID *pVitAddr;
    FILE *fp;    
    ISP_DEBUG_INFO_S  stIspDebug;    

    printf("\nNOTICE: This tool only can be used for DEBUG_ING !!!\n");
    
    if(argc < 3)
    {
        printf("usage: ./Isp debug [0/1/2] [enable] [depth]. sample: ./isp_debug 0 1 10\n");
        printf("[0/1/2]----------AE/AWB/SYS:0;1;2\r\n");
        printf("[enable/disable]---------1/0");
        printf("[depth]----------the frame number \r\n\n\n");

        return s32Ret;
    }

    s32Ret = HI_MPI_ISP_GetDebug(&stIspDebug);

    if (argc > 1)
    {
        u32Nbit = atoi(argv[1]);    /* AE/AWB/SYS */       
    }
    if(argc > 2)
    {
       enable = atoi(argv[2]);   /*enable/disable*/
       if((0 == u32Nbit)&&(1 == enable))
       {
         stIspDebug.bAEDebugEnable = 1;
       }
       else if((0 == u32Nbit)&&(0 == enable))
       {
         stIspDebug.bAEDebugEnable = 0;
       }
       else if((1 == u32Nbit)&&(1 == enable))
       {
         stIspDebug.bAWBDebugEnable = 1;
       }
       else if((1 == u32Nbit)&&(0 == enable))
       {
         stIspDebug.bAWBDebugEnable = 0;
       }
       else if((2 == u32Nbit)&&(1 == enable))
       {
        stIspDebug.bSysDebugEnable = 1;
       }
       else if((2 == u32Nbit)&&(0 == enable))
       {
        stIspDebug.bSysDebugEnable = 0;
       }
    }
    if (argc > 3)
    {
        u32depth = atoi(argv[3]);/* the frame depth */
        stIspDebug.u32DebugDepth = u32depth;
    }
    


   if(stIspDebug.bAEDebugEnable == 1)
   {
     u32SizeHi = (stIspDebug.u32AESize & 0xFFFF0000) >> 16;  /*status*/
     u32SizeLo = stIspDebug.u32AESize & 0xFFFF;              /*cfg*/
     u32MemSize = u32SizeLo + u32SizeHi * stIspDebug.u32DebugDepth;
   }
   else if(stIspDebug.bAWBDebugEnable == 1)
   {
     u32SizeHi = (stIspDebug.u32AWBSize & 0xFFFF0000) >> 16;  /*status*/
     u32SizeLo = stIspDebug.u32AWBSize & 0xFFFF;              /*cfg*/
     u32MemSize = u32SizeLo + u32SizeHi * stIspDebug.u32DebugDepth;
   }
   else if(stIspDebug.bSysDebugEnable == 1)
   {
     u32SizeHi = (stIspDebug.u32SysSize & 0xFFFF0000) >> 16;  /*status*/
     u32SizeLo = stIspDebug.u32SysSize & 0xFFFF;              /*cfg*/
     u32MemSize = u32SizeLo + u32SizeHi * stIspDebug.u32DebugDepth;
   }
    
    s32Ret = HI_MPI_SYS_MmzAlloc_Cached(&u32PhyAddr, &pVitAddr, NULL, NULL, u32MemSize);

    if(stIspDebug.bAEDebugEnable == 1)
    {
      stIspDebug.u32AEAddr = u32PhyAddr;
    }
    else if(stIspDebug.bAWBDebugEnable == 1)
    {
      stIspDebug.u32AWBAddr = u32PhyAddr;
    }
    else if(stIspDebug.bSysDebugEnable == 1)
    {
        stIspDebug.u32SysAddr = u32PhyAddr;
    }
    
    HI_MPI_ISP_SetDebug(&stIspDebug);
    
    printf("Waiting  ,press any key to stop adjust!\n");
    getchar();
    
    HI_U32 *pu32VirAddr = (HI_U32 *)HI_MPI_SYS_Mmap(stIspDebug.u32AEAddr, u32MemSize);

    char *filename;
    if(stIspDebug.bAEDebugEnable == 1)
    {
      filename = "isp_debug_ae.dat";
    }
    else if(stIspDebug.bAWBDebugEnable == 1)
    {
      filename = "isp_debug_awb.dat";;
    }
    else if(stIspDebug.bSysDebugEnable == 1)
    {
      filename = "isp_debug_sys.dat";;
    }
    
    fp=fopen(filename,"wb");
    if(fp==NULL)
    {
      printf("open file %s error \n",filename);
      return -1;
    }   

      fwrite(pu32VirAddr,1,u32MemSize,fp);

      sleep(1);

     fclose(fp);
    
     stIspDebug.bAEDebugEnable = 0;
     stIspDebug.bAWBDebugEnable = 0;
     stIspDebug.bSysDebugEnable = 0;
     HI_MPI_ISP_SetDebug(&stIspDebug);
     
     HI_MPI_SYS_Munmap(pu32VirAddr, u32MemSize);
     HI_MPI_SYS_MmzFree(u32PhyAddr, pVitAddr);   

    return HI_SUCCESS;
}



#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */


