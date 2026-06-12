#include "debug.h"
#include "eps.h"
#include "uart.h"
#include <stdlib.h>
DEBUG_Handler_t hdebug;


void DEBUG_Init(void)
{
    hdebug.request_debug_mode  = false;
    hdebug.debug_enable        = false;       
    hdebug.adcs_efuse_test     = false;         
    hdebug.payload_efuse_test  = false;             
    hdebug.payload_test        = false;     
    hdebug.camera_test         = false;     
}
void DEBUG_Task(void)
{
    if (hdebug.debug_enable)
    {
        if (hdebug.adcs_efuse_test)
        {
            EPS_enableEFuse(EPS_EFUSE_5V_CH1);

        }
        else
        {
            EPS_disableEFuse(EPS_EFUSE_5V_CH1);
        }
        if (hdebug.payload_efuse_test)
        {
            EPS_enableEFuse(EPS_EFUSE_6V_CH1);
        }
        else
        {
            EPS_disableEFuse(EPS_EFUSE_6V_CH1);
        }
    }
    DEBUG_updateADCS();
}


void DEBUG_updateADCS(void)
{
    UART_msg_t msg;
    msg.id = ADCS_DEBUG_ID;
    msg.length = sizeof(ADCS_debugCommand_t);
    memcpy(&msg.payload, &hdebug.adcs_cmd, sizeof(ADCS_debugCommand_t));
    UART_transmit(&Serial5, &msg);
}
