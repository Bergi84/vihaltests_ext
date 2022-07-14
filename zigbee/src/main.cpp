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

#include "zDevice.h"
#include "zEndpoint.h"
#include "zLibrary.h"

THwRtc gRtc;
THwClkTree gClkTree;
TTimerServer gTs;
TLowPowerManger gLpm;
THwPwr gPwr;
TSequencer gSeq;
TzdBase gZigbee;

extern "C" void IRQ_Handler_03() {gTs.irqHandler();}

void setPowerMode(TLowPowerManger::lpm_t aLpm);

volatile unsigned hbcounter = 0;

uint8_t setupZigbeeTaskId;
TzeBase endPoint1;
TzcOnOffClient onOffCluster1;

void setupZigbeeTask()
{
  TRACECPU1("init wireless stack\r\n")

  gZigbee.init(&gSeq, &gPwr);
  gSeq.waitForEvent(&gZigbee.stackInitDone);

  TRACECPU1("configure stack\r\n");

  // setup basic cluster attributes
  // must be done before call of config
  uint8_t maName[] = "Bergi84";
  gZigbee.setAttrManufacturaName(maName);
  uint8_t moName[] = "DIY";
  gZigbee.setAttrModelIdentifier(moName);
  gZigbee.setAttrHWVersion(2);
  gZigbee.setAttrAppVersion(1);
  gZigbee.setAttrStackVersion(10);
  uint8_t date[] = "14.07.2022";
  gZigbee.setAttrDateCode(date);
  uint8_t buildId[] = "test";
  gZigbee.setAttrSWBuildId(buildId);
  gZigbee.setAttrPowerSource(4);    // DC Source

  endPoint1.setEpId(17);
  endPoint1.addCluster(&onOffCluster1);
  gZigbee.addEndpoint(&endPoint1);

  gZigbee.config();
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

	// setup clock configuration for maximal speed
	gClkTree.init();
	gClkTree.loadHseTune();
	gClkTree.setRtcClkSource(THwClkTree::RTC_LSE);
	gClkTree.confFlashForSpeed(THwClkTree::hseSpeed);
	gClkTree.setPllClkSource(THwClkTree::PLL_HSE, 8);
	gClkTree.confPllMain(64, 4, 4, 4);
	gClkTree.setCpu2ClkDiv(2);
	gClkTree.setFlashClkDiv(2);
	gClkTree.setSysClkSource(THwClkTree::SYS_PLLRCLK);
	gClkTree.setSysStopClkSource(THwClkTree::SYS_HSI16);
	gClkTree.setRfWakeupClkSource(THwClkTree::RFWU_LSE);

	// clock output for debugging
	gClkTree.setOutClkSource(THwClkTree::OUT_SYSCLK, 1);

	// enable and configure SMPS
	gClkTree.setSmpsClkSource(THwClkTree::SMPS_HSI16, true);
	//gPwr.enableSMPS(1750, 220);

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

	gTrace.init(0, &gLpm, &gSeq);

	TRACE("\r\n--------------------------------------\r\n");
	TRACE("Hello From VIHAL !\r\n");
	TRACE("Board: %s\r\n", BOARD_NAME);
  uint32_t sysSpeed;
  gClkTree.getSysClkSpeed(sysSpeed);
	TRACE("SystemCoreClock: %u\r\n", sysSpeed);

	// init time server
	uint8_t TimerID[4];

	gTs.init(&gRtc);

	mcu_enable_interrupts();

	gLpm.disableLpMode(locLpmId, TLowPowerManger::LPM_Run);

	gSeq.addTask(setupZigbeeTaskId, setupZigbeeTask);
	gSeq.queueTask(setupZigbeeTaskId);

  gSeq.startIdle();

  while(1);
}


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
