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

#include "timeServer.h"
#include "board_pins.h"

THwRtc gRtc;

TTimerServer gTs;
extern "C" void IRQ_Handler_03() {gTs.serviceQueue();}

void timerHandler0(void* aObjP, THwRtc::time_t aTime);
void timerHandler1(void* aObjP, THwRtc::time_t aTime);
void timerHandler2(void* aObjP, THwRtc::time_t aTime);
void timerHandler3(void* aObjP, THwRtc::time_t aTime);

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


  if (!hwclk_init(EXTERNAL_XTAL_HZ, MCU_CLOCK_SPEED))  // if the EXTERNAL_XTAL_HZ == 0, then the internal RC oscillator will be used
  {
    while (1)
    {
    // error
    }
  }

  hwlsclk_init(true);

	mcu_enable_fpu();    // enable coprocessor if present
	mcu_enable_icache(); // enable instruction cache if present

	clockcnt_init();

	// go on with the hardware initializations
	board_pins_init();
	gRtc.init();
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
	TRACE("SystemCoreClock: %u\r\n", SystemCoreClock);

	printTime();
	uint8_t TimerID[4];

	gTs.init(&gRtc);

	gTs.create(TimerID[0], timerHandler0, 0, true);
	gTs.create(TimerID[1], timerHandler1, 0, true);
	gTs.create(TimerID[2], timerHandler2, 0, true);
	gTs.create(TimerID[3], timerHandler3, 0, true);
	gTs.start(TimerID[0], 10000);
	gTs.start(TimerID[1], 1000);
	gTs.start(TimerID[2], 500);
	gTs.start(TimerID[3], 5000);
	printTime();

	mcu_enable_interrupts();

	// Infinite loop
	while (1)
	{

	}
}

void timerHandler0(void* aObjP, THwRtc::time_t aTime)
{
  TRACE("%sTimer0: ", CC_RED);
  TRACE("%02hhu:%02hhu:%02hhu.%03hu %02hhu.%02hhu.%02hhu\r\n", aTime.hour, aTime.min, aTime.sec, aTime.msec, aTime.day, aTime.month, aTime.year);
}

void timerHandler1(void* aObjP, THwRtc::time_t aTime)
{
  TRACE("%sTimer1: ", CC_MAG);
  TRACE("%02hhu:%02hhu:%02hhu.%03hu %02hhu.%02hhu.%02hhu\r\n", aTime.hour, aTime.min, aTime.sec, aTime.msec, aTime.day, aTime.month, aTime.year);
}

void timerHandler2(void* aObjP, THwRtc::time_t aTime)
{
  TRACE("%sTimer2: ", CC_YEL);
  TRACE("%02hhu:%02hhu:%02hhu.%03hu %02hhu.%02hhu.%02hhu\r\n", aTime.hour, aTime.min, aTime.sec, aTime.msec, aTime.day, aTime.month, aTime.year);
}

void timerHandler3(void* aObjP, THwRtc::time_t aTime)
{
  TRACE("%sTimer3: ", CC_BLU);
  TRACE("%02hhu:%02hhu:%02hhu.%03hu %02hhu.%02hhu.%02hhu\r\n", aTime.hour, aTime.min, aTime.sec, aTime.msec, aTime.day, aTime.month, aTime.year);
}

// ----------------------------------------------------------------------------
