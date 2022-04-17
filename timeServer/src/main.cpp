// file:     main.cpp (uart)
// brief:    VIHAL UART Test
// created:  2021-10-03
// authors:  nvitya

#include "platform.h"
#include "cppinit.h"
#include "clockcnt.h"
#include "traces.h"

#include "hwclk.h"
#include "hwpins.h"
#include "hwuart.h"
#include "hwrtc.h"
#include "hwclktree.h"
#include "hwpwr.h"

#include "timeServer.h"
#include "board_pins.h"
#include "sequencer_armm.h"

THwRtc gRtc;
THwClkTree gClkTree;
TTimerServer gTs;
TLowPowerManger gLpm;
THwPwr gPwr;
TSequencer gSeq;

extern "C" void IRQ_Handler_03() {gTs.irqHandler();}

void timerHandler0(THwRtc::time_t aTime);
void timerHandler1(THwRtc::time_t aTime);
void timerHandler2(THwRtc::time_t aTime);
void timerHandler3(THwRtc::time_t aTime);

uint8_t timerSeqId[4];

void timerIdleTask0();
void timerIdleTask1();
void timerIdleTask2();
void timerIdleTask3();

uint8_t timerFreeSeqId;
void timerFreeTask();

bool waiEvt[4];
THwRtc::time_t evtTime[4];

void setPowerMode(TLowPowerManger::lpm_t aLpm);

volatile unsigned hbcounter = 0;

void printTime()
{
  THwRtc::time_t aktTime;
  gRtc.getTime(aktTime);
  TRACE("%02hhu:%02hhu:%02hhu.%03hu %02hhu.%02hhu.%02hhu\r\n", aktTime.hour, aktTime.min, aktTime.sec, aktTime.msec, aktTime.day, aktTime.month, aktTime.year);

}

extern "C" __attribute__((noreturn)) void _start(unsigned self_flashing)  // self_flashing = 1: self-flashing required for RAM-loaded applications
{
  // after ram setup and region copy the cpu jumps here, with probably RC oscillator
  mcu_disable_interrupts();

  // Set the interrupt vector table offset, so that the interrupts and exceptions work
  mcu_init_vector_table();

  // run the C/C++ initialization (variable initializations, constructors)
  cppinit();

	mcu_enable_fpu();    // enable coprocessor if present
	mcu_enable_icache(); // enable instruction cache if present

	gPwr.init();

	gClkTree.init();
	gClkTree.loadHseTune();
	gClkTree.setRtcClkSource(THwClkTree::RTC_LSE);
	gClkTree.confFlashForSpeed(THwClkTree::hseSpeed*2);
	gClkTree.setPllClkSource(THwClkTree::PLL_HSE, 8);
	gClkTree.confPllMain(64, 4, 4, 4);
	gClkTree.setSysClkSource(THwClkTree::SYS_PLLRCLK);
	gClkTree.setMsiSpeed(2000000);
	gClkTree.setSysStopClkSource(THwClkTree::SYS_HSI16);

	gClkTree.setOutClkSource(THwClkTree::OUT_HSE, 1);

	gClkTree.setSmpsClkSource(THwClkTree::SMPS_HSI16, true);
	gPwr.enableSMPS(1750, 220);

	clockcnt_init();

	// go on with the hardware initializations
	board_pins_init();
	gRtc.init();

	uint32_t speed;
	gClkTree.getPeriClkSpeed(gRtc.regs, speed);
	gRtc.setPeriphClock(speed);
	gClkTree.getPeriClkSpeed(conuart.regs, speed);
	conuart.SetPeriphClock(speed);

	gLpm.init(setPowerMode);
	uint32_t locLpmId;
	gLpm.registerApp(locLpmId);
	gLpm.disableLpMode(locLpmId, TLowPowerManger::LPM_off);

  gSeq.init();

  gSeq.addTask(timerSeqId[0], timerIdleTask0);
  gSeq.addTask(timerSeqId[1], timerIdleTask1);
  gSeq.addTask(timerSeqId[2], timerIdleTask2);
  gSeq.addTask(timerSeqId[3], timerIdleTask3);
  gSeq.addTask(timerFreeSeqId, timerFreeTask);
  gSeq.setIdleFunc(&gLpm, (void (TCbClass::*)(void)) &TLowPowerManger::enterLowPowerMode);

	gTrace.init(0, &gLpm, &gSeq);

	THwRtc::time_t startTime, lastTime;
	startTime.msec = 768;
	startTime.sec = 13;
	startTime.min = 43;
	startTime.hour = 13;
	startTime.day = 13;
	startTime.month = 03;
	startTime.year = 22;
	gRtc.setTime(startTime, lastTime);

	printTime();

	TRACE("\r\n--------------------------------------\r\n");
	TRACE("%sHello From VIHAL !\r\n", CC_BLU);
	TRACE("Board: %s\r\n", BOARD_NAME);
	uint32_t sysSpeed;
	gClkTree.getSysClkSpeed(sysSpeed);
	TRACE("SystemCoreClock: %u\r\n", sysSpeed);

	printTime();

	// init time server
	uint8_t TimerID[4];

  gTs.init(&gRtc);
	gTs.create(TimerID[0], timerHandler0, true);
	gTs.create(TimerID[1], timerHandler1, true);
	gTs.create(TimerID[2], timerHandler2, true);
	gTs.create(TimerID[3], timerHandler3, true);
	gTs.start(TimerID[0], 10000);
	gTs.start(TimerID[1], 1000);
	gTs.start(TimerID[2], 500);
	gTs.start(TimerID[3], 5000);
	printTime();

	mcu_enable_interrupts();

	gLpm.disableLpMode(locLpmId, TLowPowerManger::LPM_Run);
	gSeq.startIdle();
	while(1);
}


// functions called from time server after timer elapsed
void timerHandler0(THwRtc::time_t aTime)
{
  evtTime[0] = aTime;
  gSeq.queueTask(timerSeqId[0]);
}

void timerHandler1(THwRtc::time_t aTime)
{
  evtTime[1] = aTime;
  gSeq.queueTask(timerSeqId[1]);
}

void timerHandler2(THwRtc::time_t aTime)
{
  evtTime[2] = aTime;
  gSeq.queueTask(timerSeqId[2]);
}

void timerHandler3(THwRtc::time_t aTime)
{
  evtTime[3] = aTime;
  gSeq.queueTask(timerSeqId[3]);
}


// functions called from idle sequencer after function was queued
void timerIdleTask0()
{
  TRACE("%sTimer0: ", CC_RED);
  TRACE("%02hhu:%02hhu:%02hhu.%03hu %02hhu.%02hhu.%02hhu\r\n", evtTime[0].hour, evtTime[0].min, evtTime[0].sec, evtTime[0].msec, evtTime[0].day, evtTime[0].month, evtTime[0].year);
}

void timerIdleTask1()
{
  waiEvt[0] = false;
  gSeq.queueTask(timerFreeSeqId);
  gSeq.waitForEvent(&waiEvt[0]);

  TRACE("%sTimer1: ", CC_MAG);
  TRACE("%02hhu:%02hhu:%02hhu.%03hu %02hhu.%02hhu.%02hhu\r\n", evtTime[1].hour, evtTime[1].min, evtTime[1].sec, evtTime[1].msec, evtTime[1].day, evtTime[1].month, evtTime[1].year);
}

void timerIdleTask2()
{
  TRACE("%sTimer2: ", CC_YEL);
  TRACE("%02hhu:%02hhu:%02hhu.%03hu %02hhu.%02hhu.%02hhu\r\n", evtTime[2].hour, evtTime[2].min, evtTime[2].sec, evtTime[2].msec, evtTime[2].day, evtTime[2].month, evtTime[2].year);
}

void timerIdleTask3()
{
  TRACE("%sTimer3: ", CC_BLU);
  TRACE("%02hhu:%02hhu:%02hhu.%03hu %02hhu.%02hhu.%02hhu\r\n", evtTime[3].hour, evtTime[3].min, evtTime[3].sec, evtTime[3].msec, evtTime[3].day, evtTime[3].month, evtTime[3].year);
}

void timerFreeTask()
{
  waiEvt[0] = true;
  waiEvt[1] = true;
  waiEvt[2] = true;
  waiEvt[3] = true;
}

// power config function for low power manager
void setPowerMode(TLowPowerManger::lpm_t aLpm)
{
  TCriticalSection cSec(true);
  switch(aLpm)
  {
  case TLowPowerManger::LPM_Run:
    gPwr.leaveLowPowerRunMode();
    gClkTree.confFlashForSpeed(THwClkTree::hseSpeed*2);
    gClkTree.setPllClkSource(THwClkTree::PLL_HSE, 8);
    gClkTree.confPllMain(64, 4, 4, 4);
    gClkTree.setSysClkSource(THwClkTree::SYS_PLLRCLK);
    break;

  case TLowPowerManger::LPM_lpRun:
    gClkTree.setSysClkSource(THwClkTree::SYS_MSI);
    gClkTree.confFlashForSpeed(2000000);
    gPwr.enterLowPowerRunMode();
    break;

  case TLowPowerManger::LPM_sleep:
    gPwr.enterLowPowerMode(THwPwr::LPM_SLEEP);
    break;

  case TLowPowerManger::LPM_deepSleep:
    gClkTree.confFlashForSpeed(THwClkTree::hseSpeed*2);
    gClkTree.setSysClkSource(THwClkTree::SYS_HSI16);
    gClkTree.confFlashForSpeed(THwClkTree::hsi16Speed);

    gPwr.enterLowPowerMode(THwPwr::LPM_STOP2);

    gClkTree.confFlashForSpeed(THwClkTree::hseSpeed*2);
    gClkTree.setPllClkSource(THwClkTree::PLL_HSE, 8);
    gClkTree.confPllMain(64, 4, 4, 4);
    gClkTree.setSysClkSource(THwClkTree::SYS_PLLRCLK);
    break;

  case TLowPowerManger:: LPM_off:
    // not supported
    break;
  }
  cSec.leave();
}

// ----------------------------------------------------------------------------
