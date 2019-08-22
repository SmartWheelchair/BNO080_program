 /// @file Watchdog.h provides the interface to the Watchdog module
///
/// This provides basic Watchdog service for the mbed. You can configure
/// various timeout intervals that meet your system needs. Additionally,
/// it is possible to identify if the Watchdog was the cause of any
/// system restart, permitting the application code to take appropriate
/// behavior.
///
/// Adapted from Simon's Watchdog code from http://mbed.org/forum/mbed/topic/508/
///
/// @note Copyright &copy; 2011 by Smartware Computing, all rights reserved.
///     This software may be used to derive new software, as long as
///     this copyright statement remains in the source file.
/// @author David Smart
///
/// History
/// \li v1.00 - 20110616: initial release with some documentation improvements
///aa
#ifndef WATCHDOG_H
#define WATCHDOG_H
#include "mbed.h"

/// The Watchdog class provides the interface to the Watchdog feature
///
/// Embedded programs, by their nature, are usually unattended. If things
/// go wrong, it is usually important that the system attempts to recover.
/// Aside from robust software, a hardware watchdog can monitor the
/// system and initiate a system reset when appropriate.
///
/// This Watchdog is patterned after one found elsewhere on the mbed site,
/// however this one also provides a method for the application software
/// to determine the cause of the reset - watchdog or otherwise.
///
/// example:
/// @code
/// Watchdog wd;
///
/// ...
/// main() {
///    if (wd.WatchdogCausedReset())
///        pc.printf("Watchdog caused reset.\r\n");
///
///    wd.Configure(3.0);       // sets the timeout interval
///    for (;;) {
///         wd.Service();       // kick the dog before the timeout
///         // do other work
///    }
/// }
/// @endcode
///
class Watchdog {
public:
    /// Create a Watchdog object
    ///
    /// example:
    /// @code
    /// Watchdog wd;    // placed before main
    /// @endcode
    Watchdog();

    /// Configure the timeout for the Watchdog
    ///
    /// This configures the Watchdog service and starts it. It must
    /// be serviced before the timeout, or the system will be restarted.
    ///
    /// example:
    /// @code
    ///     ...
    ///     wd.Configure(1.4);  // configure for a 1.4 second timeout
    ///     ...
    /// @endcode
    ///
    /// @param[in] timeout in seconds, as a floating point number
    /// @returns none
    ///
    void Configure(float timeout);

    /// Service the Watchdog so it does not cause a system reset
    ///
    /// example:
    /// @code
    ///    wd.Service();
    /// @endcode
    /// @returns none
    void Service();

    /// WatchdogCausedReset identifies if the cause of the system
    /// reset was the Watchdog
    ///
    /// example:
    /// @code
    ///    if (wd.WatchdogCausedReset())) {
    /// @endcode
    ///
    /// @returns true if the Watchdog was the cause of the reset
    bool WatchdogCausedReset();
private:
    bool wdreset;
};

#endif // WATCHDOG_H

/// @file Watchdog.cpp provides the interface to the Watchdog module
///
/// This provides basic Watchdog service for the mbed. You can configure
/// various timeout intervals that meet your system needs. Additionally,
/// it is possible to identify if the Watchdog was the cause of any
/// system restart.
///
/// Adapted from Simon's Watchdog code from http://mbed.org/forum/mbed/topic/508/
///
/// @note Copyright &copy; 2011 by Smartware Computing, all rights reserved.
///     This software may be used to derive new software, as long as
///     this copyright statement remains in the source file.
/// @author David Smart
///
/// @note Copyright &copy; 2015 by NBRemond, all rights reserved.
///     This software may be used to derive new software, as long as
///     this copyright statement remains in the source file.
///
///     Added support for STM32 Nucleo platforms
///
/// @author Bernaérd Remond
///

//#define LPC
#define ST_NUCLEO


#include "mbed.h"
#include "Watchdog.h"


/// Watchdog gets instantiated at the module level
Watchdog::Watchdog() {
#ifdef LPC
    wdreset = (LPC_WDT->WDMOD >> 2) & 1;    // capture the cause of the previous reset
#endif
#ifdef ST_NUCLEO
    // capture the cause of the previous reset
    /* Check if the system has resumed from IWDG reset */
/*
    if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST)) {
        wdreset = true;
    }
    else {
        wdreset = false;
    }
*/
        //printf("in constructor\r\n");
        wdreset = false;
#endif

}

/// Load timeout value in watchdog timer and enable
// Timeout is in units of seconds
void Watchdog::Configure(float timeout) {
#ifdef LPC
    LPC_WDT->WDCLKSEL = 0x1;                // Set CLK src to PCLK
    uint32_t clk = SystemCoreClock / 16;    // WD has a fixed /4 prescaler, PCLK default is /4
    LPC_WDT->WDTC = (uint32_t)(timeout * (float)clk);
    LPC_WDT->WDMOD = 0x3;                   // Enabled and Reset
#endif

#ifdef ST_NUCLEO
    // see http://embedded-lab.com/blog/?p=9662
    #define LsiFreq (45000)
    uint16_t PrescalerCode;
    uint16_t Prescaler;
    uint16_t ReloadValue;
    float Calculated_timeout;

    if ((timeout * (LsiFreq/4)) < 0x7FF) {
        PrescalerCode = IWDG_PRESCALER_4;
        Prescaler = 4;
    }
    else if ((timeout * (LsiFreq/8)) < 0xFF0) {
        PrescalerCode = IWDG_PRESCALER_8;
        Prescaler = 8;
    }
    else if ((timeout * (LsiFreq/16)) < 0xFF0) {
        PrescalerCode = IWDG_PRESCALER_16;
        Prescaler = 16;
    }
    else if ((timeout * (LsiFreq/32)) < 0xFF0) {
        PrescalerCode = IWDG_PRESCALER_32;
        Prescaler = 32;
    }
    else if ((timeout * (LsiFreq/64)) < 0xFF0) {
        PrescalerCode = IWDG_PRESCALER_64;
        Prescaler = 64;
    }
    else if ((timeout * (LsiFreq/128)) < 0xFF0) {
        PrescalerCode = IWDG_PRESCALER_128;
        Prescaler = 128;
    }
    else {
        PrescalerCode = IWDG_PRESCALER_256;
        Prescaler = 256;
    }

    // specifies the IWDG Reload value. This parameter must be a number between 0 and 0x0FFF.
    ReloadValue = (uint32_t)(timeout * (LsiFreq/Prescaler));

    Calculated_timeout = ((float)(Prescaler * ReloadValue)) / LsiFreq;
    printf("WATCHDOG set with prescaler:%d reload value: 0x%X - timeout:%f\n",Prescaler, ReloadValue, Calculated_timeout);

    IWDG->KR = 0x5555; //Disable write protection of IWDG registers
    IWDG->PR = PrescalerCode;      //Set PR value
    IWDG->RLR = ReloadValue;      //Set RLR value
    IWDG->KR = 0xAAAA;    //Reload IWDG
    IWDG->KR = 0xCCCC;    //Start IWDG - See more at: http://embedded-lab.com/blog/?p=9662#sthash.6VNxVSn0.dpuf
#endif

    Service();
}

/// "Service", "kick" or "feed" the dog - reset the watchdog timer
/// by writing this required bit pattern
void Watchdog::Service() {
#ifdef LPC
    LPC_WDT->WDFEED = 0xAA;
    LPC_WDT->WDFEED = 0x55;
#endif
#ifdef ST_NUCLEO
    IWDG->KR = 0xAAAA;         //Reload IWDG - See more at: http://embedded-lab.com/blog/?p=9662#sthash.6VNxVSn0.dpuf
#endif
}

/// get the flag to indicate if the watchdog causes the reset
bool Watchdog::WatchdogCausedReset() {
    return wdreset;
}
