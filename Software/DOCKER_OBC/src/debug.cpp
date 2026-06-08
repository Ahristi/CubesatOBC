#include "debug.h"
DEBUG_Handler_t hdebug;


void DEBUG_Init(void)
{
    hdebug.request_debug_mode  = false;
    hdebug.debug_enable        = false;     
    hdebug.xrw                 = 0.0f; 
    hdebug.yrw                 = 0.0f; 
    hdebug.zrw                 = 0.0f; 
    hdebug.xmag                = 0.0f;     
    hdebug.ymag                = 0.0f;     
    hdebug.zmag                = 0.0f;     
    hdebug.adcs_efuse_test     = false;         
    hdebug.payload_efuse_test  = false;             
    hdebug.payload_test        = false;     
    hdebug.camera_test         = false;     
}
void DEBUG_Task(void)
{
    if (hdebug.debug_enable)
    {
        //TODO: Implement debug
    }
}



