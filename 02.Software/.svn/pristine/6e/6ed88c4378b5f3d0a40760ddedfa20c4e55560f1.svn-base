#include "io_ctrl.h"
#include "board.h"
#include "SignalManage.h"


/// @brief Ω˚”√À˘”–«π
/// @param  
void DisableAllGun(void)
{
    // Disable all guns
    DISABLE_A_GUN_DO_SUPER();
    DISABLE_B_GUN_DO_SUPER();
    DISABLE_A_GUN_DO_24V();
    DISABLE_B_GUN_DO_24V();
    DISABLE_A_GUN_DO_12V();
    DISABLE_B_GUN_DO_12V();
    DISABLE_A_GUN_DO_GND();
    DISABLE_B_GUN_DO_GND();
    DISABLE_A_GUN_DO_LOCK();
    DISABLE_B_GUN_DO_LOCK();

    SetSigVal(SIGNAL_STATUS_DO_A_GUN_SUPER,     FALSE);
    SetSigVal(SIGNAL_STATUS_DO_B_GUN_SUPER,     FALSE);
    SetSigVal(SIGNAL_STATUS_DO_A_GUN_POWER_24V, FALSE);
    SetSigVal(SIGNAL_STATUS_DO_B_GUN_POWER_24V, FALSE);
    SetSigVal(SIGNAL_STATUS_DO_A_GUN_POWER_12V, FALSE);
    SetSigVal(SIGNAL_STATUS_DO_B_GUN_POWER_12V, FALSE);
    SetSigVal(SIGNAL_STATUS_DO_A_GUN_POWER_GND, FALSE);
    SetSigVal(SIGNAL_STATUS_DO_B_GUN_POWER_GND, FALSE);
    SetSigVal(SIGNAL_STATUS_DO_A_GUN_LOCK,      FALSE);
    SetSigVal(SIGNAL_STATUS_DO_B_GUN_LOCK,      FALSE);
}




void IO_Ctrl(enumIO_CTRL_NO ioNo, BOOL outEnable)
{
    switch(ioNo)
    {
        case IO_A_GUN_DO_SUPER:
            SetSigVal(SIGNAL_STATUS_DO_A_GUN_SUPER, outEnable);
            outEnable == TRUE ? ENABLE_A_GUN_DO_SUPER() : DISABLE_A_GUN_DO_SUPER();
            break;
        case IO_B_GUN_DO_SUPER:
            SetSigVal(SIGNAL_STATUS_DO_B_GUN_SUPER, outEnable);
            outEnable == TRUE ? ENABLE_B_GUN_DO_SUPER() : DISABLE_B_GUN_DO_SUPER();
            break;
        case IO_A_GUN_DO_24V:
            SetSigVal(SIGNAL_STATUS_DO_A_GUN_POWER_24V, outEnable);
            outEnable == TRUE ? ENABLE_A_GUN_DO_24V() : DISABLE_A_GUN_DO_24V();
            break;
        case IO_B_GUN_DO_24V:
            SetSigVal(SIGNAL_STATUS_DO_B_GUN_POWER_24V, outEnable);
            outEnable == TRUE ? ENABLE_B_GUN_DO_24V() : DISABLE_B_GUN_DO_24V();
            break;
        case IO_A_GUN_DO_12V:
            SetSigVal(SIGNAL_STATUS_DO_A_GUN_POWER_12V, outEnable);
            outEnable == TRUE ? ENABLE_A_GUN_DO_12V() : DISABLE_A_GUN_DO_12V();
            break;
        case IO_B_GUN_DO_12V:
            SetSigVal(SIGNAL_STATUS_DO_B_GUN_POWER_12V, outEnable);
            outEnable == TRUE ? ENABLE_B_GUN_DO_12V() : DISABLE_B_GUN_DO_12V();
            break;
        case IO_A_GUN_DO_GND:
            SetSigVal(SIGNAL_STATUS_DO_A_GUN_POWER_GND, outEnable);
            outEnable == TRUE ? ENABLE_A_GUN_DO_GND() : DISABLE_A_GUN_DO_GND();
            break;
        case IO_B_GUN_DO_GND:
            SetSigVal(SIGNAL_STATUS_DO_B_GUN_POWER_GND, outEnable);
            outEnable == TRUE ? ENABLE_B_GUN_DO_GND() : DISABLE_B_GUN_DO_GND();
            break;
        case IO_A_GUN_DO_LOCK:
            SetSigVal(SIGNAL_STATUS_DO_A_GUN_LOCK, outEnable);
            outEnable == TRUE ? ENABLE_A_GUN_DO_LOCK() : DISABLE_A_GUN_DO_LOCK();
            break;
        case IO_B_GUN_DO_LOCK:
            SetSigVal(SIGNAL_STATUS_DO_B_GUN_LOCK, outEnable);
            outEnable == TRUE ? ENABLE_B_GUN_DO_LOCK() : DISABLE_B_GUN_DO_LOCK();
            break;
    }
}


