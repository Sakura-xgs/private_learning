#include "hal_rtc.h"
#include "gd32f4xx.h"


/*!
    \brief      RTC INIT
    \param[in]  none
    \param[out] none
    \retval     none
*/
void RTC_INIT(void)
{
    /* PMU lock enable */
    rcu_periph_clock_enable(RCU_PMU);
    rcu_periph_clock_enable(RCU_BKPSRAM);
    
    pmu_backup_write_enable();
}









