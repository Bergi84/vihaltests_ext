/*
 *  file:     traces.h (uart)
 *  brief:    Trace message redirection to UART
 *  version:  1.00
 *  date:     2021-10-02
 *  authors:  nvitya
*/

#ifndef __Traces__h
#define __Traces__h

#include "hwuart.h"
#include "lowPowerManager.h"

#include "mp_printf.h"
#include <string.h>

extern THwUart conuart;  // console uart

#define TRACE(...) { gTrace.traceCpu1(__VA_ARGS__); }

#ifdef LTRACES
 #define LTRACE(...)  TRACE( __VA_ARGS__ )
#else
 #define LTRACE(...)
#endif

#undef LTRACES

// "\xhh" inserts a byte into string with 0xhh value, "0x1B" means ESC, "[" means CSI (Control Sequence Intoducer),
// m means Select Graphic Rendition, 0 means reset to normal, 30-37 means set foreground color
#define CC_NRM  "\x1B[0m"
#define CC_RED  "\x1B[31m"
#define CC_GRN  "\x1B[32m"
#define CC_YEL  "\x1B[33m"
#define CC_BLU  "\x1B[34m"
#define CC_MAG  "\x1B[35m"
#define CC_CYN  "\x1B[36m"
#define CC_WHT  "\x1B[37m"

class TTrace
{
public:
  static constexpr uint32_t bufLength = 1024;

  THwUart* uart;
  TLowPowerManger* lpm;
  uint32_t lpmId;

  char buf[bufLength];
  uint32_t bufWInd;
  uint32_t bufRInd;

  static constexpr const char strCpu1[] = {"\x1B[0mCPU1: "};
  static constexpr uint32_t strCpu1Len = sizeof(strCpu1);
  static constexpr const char strCpu2[] = {"\x1B[35mCPU2: "};
  static constexpr uint32_t strCpu2Len = sizeof(strCpu2);
  static constexpr uint32_t strCpuIndent = 6;

  typedef enum {
   LA_NONE,
   LA_CPU1,
   LA_CPU2
  } lastAktiv_t;
  lastAktiv_t lastAktiv;
  bool intend;

  bool init(THwUart* aUart = 0, TLowPowerManger* aLpm = 0);

  void traceCpu1(const char* format, ...);
  void traceCpu2(const char* format, ...);

  inline void service()
  {
    while(bufWInd != bufRInd)
    {
      if(lpm != 0)
      {
        lpm->enableLpMode(lpmId, TLowPowerManger::LPM_Run);
      }

      if(uart->TrySendChar(buf[bufRInd]))
      {
        if(bufRInd == bufLength-1)
        {
          bufRInd = 0;
        }
        else
        {
          bufRInd++;
        }
      }
      else
      {
        return;
      }
    }

    if(lpm != 0)
    {
      lpm->disableLpMode(lpmId, TLowPowerManger::LPM_Run);
    }
  }
};

extern TTrace gTrace;

#endif //!defined(Traces__h)

//--- End Of file --------------------------------------------------------------
