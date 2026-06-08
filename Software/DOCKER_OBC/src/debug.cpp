#include "debug.h"
DEBUG_Handler_t hdebug;


void DEBUG_Init(void)
{
    hdebug.request_debug_mode  = false;
    hdebug.debug_enable        = false;     
    hdebug.xrw_test            = false; 
    hdebug.yrw_test            = false; 
    hdebug.zrw_test            = false; 
    hdebug.xmag_test           = false;     
    hdebug.ymag_test           = false;     
    hdebug.zmag_test           = false;     
    hdebug.adcs_efuse_test     = false;         
    hdebug.payload_efuse_test  = false;             
    hdebug.payload_test        = false;     
    hdebug.camera_test         = false;     
}
void DEBUG_Task(void)
{
    if (hdebug.debug_enable)
    {
        hdebug.adcs_cmd.xrw  = hdebug.xrw_test ? 1000 : 0;
        hdebug.adcs_cmd.yrw  = hdebug.yrw_test ? 1000 : 0;
        hdebug.adcs_cmd.zrw  = hdebug.zrw_test ? 1000 : 0;
        hdebug.adcs_cmd.xmag = hdebug.xmag_test? 50: 0;
        hdebug.adcs_cmd.ymag = hdebug.xmag_test? 50: 0; 
        hdebug.adcs_cmd.zmag = hdebug.xmag_test? 50: 0;  
    }
}



