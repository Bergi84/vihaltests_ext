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

#include "timeServer.h"
#include "board_pins.h"

THwRtc gRtc;
THwClkTree gClkTree;

TTimerServer gTs;
extern "C" void IRQ_Handler_03() {gTs.irqHandler();}

void timerHandler0(THwRtc::time_t aTime);
void timerHandler1(THwRtc::time_t aTime);
void timerHandler2(THwRtc::time_t aTime);
void timerHandler3(THwRtc::time_t aTime);

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

	gClkTree.init();
	gClkTree.setRtcClkSource(THwClkTree::RTC_LSE);
	gClkTree.confFlashForSpeed(THwClkTree::hseSpeed*2);
	gClkTree.setPllClkSource(THwClkTree::PLL_HSE, 8);
	gClkTree.confPllMain(64, 4, 4, 4);
	gClkTree.setSysClkSource(THwClkTree::SYS_PLLRCLK);

	clockcnt_init();

	// go on with the hardware initializations
	board_pins_init();
	gRtc.init();

	uint32_t speed;
	gClkTree.getPeriClkSpeed(gRtc.regs, speed);
	gRtc.setPeriphClock(speed);
	gClkTree.getPeriClkSpeed(conuart.regs, speed);
	conuart.SetPeriphClock(speed);

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

	gTrace.init();
	TRACE("\r\n--------------------------------------\r\n");
	TRACE("%sHello From VIHAL !\r\n", CC_BLU);
	TRACE("Board: %s\r\n", BOARD_NAME);
	TRACE("SystemCoreClock: %u\r\n", SystemCoreClock);

	printTime();
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

	// Infinite loop
	while (1)
	{
	  gTrace.service();
	}
}

void timerHandler0(THwRtc::time_t aTime)
{
  TRACE("%sTimer0: ", CC_RED);
  TRACE("%02hhu:%02hhu:%02hhu.%03hu %02hhu.%02hhu.%02hhu\r\n", aTime.hour, aTime.min, aTime.sec, aTime.msec, aTime.day, aTime.month, aTime.year);
}

void timerHandler1(THwRtc::time_t aTime)
{
  TRACE("%sTimer1: ", CC_MAG);
  TRACE("%02hhu:%02hhu:%02hhu.%03hu %02hhu.%02hhu.%02hhu\r\n", aTime.hour, aTime.min, aTime.sec, aTime.msec, aTime.day, aTime.month, aTime.year);
}

void timerHandler2(THwRtc::time_t aTime)
{
  TRACE("%sTimer2: ", CC_YEL);
  TRACE("%02hhu:%02hhu:%02hhu.%03hu %02hhu.%02hhu.%02hhu\r\n", aTime.hour, aTime.min, aTime.sec, aTime.msec, aTime.day, aTime.month, aTime.year);
}

void timerHandler3(THwRtc::time_t aTime)
{
  TRACE("%sTimer3: ", CC_BLU);
  TRACE("%02hhu:%02hhu:%02hhu.%03hu %02hhu.%02hhu.%02hhu\r\n", aTime.hour, aTime.min, aTime.sec, aTime.msec, aTime.day, aTime.month, aTime.year);
}

// ----------------------------------------------------------------------------
