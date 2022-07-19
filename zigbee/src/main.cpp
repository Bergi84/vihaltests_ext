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
#include "hwintflash.h"

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
THwIntFlash gFlash;

extern "C" void IRQ_Handler_03() {gTs.irqHandler();}

void setPowerMode(TLowPowerManger::lpm_t aLpm);

volatile unsigned hbcounter = 0;

uint8_t setupZigbeeTaskId;
TzeBase endPoint1;
TzeBase endPoint2;
TzcOnOffClient onOffClientCluster1;
TzcOnOffServer onOffServerCluster1;

bool flagSendNext;

void sendNext(THwRtc::time_t time)
{
  flagSendNext = true;
}

void resetNwk(THwRtc::time_t time)
{
  TRACECPU1("search new network\r\n")
  gZigbee.joinNewNwk();
}

TzcOnOffServer::statusCode_t onOffServerCluster1_onCb(TzcOnOffServer::zAdr_t* adr)
{
  onOffServerCluster1.setAttrOnOff(true);
  TRACECPU1("recieved cmd on\r\n");
  return 0;     // ZCL: SUCCESS
}

TzcOnOffServer::statusCode_t onOffServerCluster1_offCb(TzcOnOffServer::zAdr_t* adr)
{
  onOffServerCluster1.setAttrOnOff(false);
  TRACECPU1("recieved cmd off\r\n");
  return 0;     // ZCL: SUCCESS
}

TzcOnOffServer::statusCode_t onOffServerCluster1_toggleCb(TzcOnOffServer::zAdr_t* adr)
{
  TRACECPU1("recieved cmd toggle\r\n");
  bool onOffState;
  onOffServerCluster1.getAttrOnOff(onOffState);
  onOffServerCluster1.setAttrOnOff(!onOffState);
  return 0;     // ZCL: SUCCESS
}

void setupZigbeeTask()
{
  TRACECPU1("init wireless stack\r\n")

  gZigbee.init(&gSeq, &gPwr, &gTs, &gFlash);
  gSeq.waitForEvent(&gZigbee.flagStackInitDone);

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

  // setup endpoints and clusters
  endPoint1.setEpId(1);
  endPoint1.addCluster(&onOffClientCluster1);

  onOffServerCluster1.setOffCmdInCb(onOffServerCluster1_offCb);
  onOffServerCluster1.setOnCmdInCb(onOffServerCluster1_onCb);
  onOffServerCluster1.setToggleCmdInCb(onOffServerCluster1_toggleCb);

  endPoint2.setEpId(2);
  endPoint2.addCluster(&onOffServerCluster1);

  gZigbee.addEndpoint(&endPoint1);
  gZigbee.addEndpoint(&endPoint2);

  // config stack with endpoints and clusters,
  // also calls from each endpoint and cluster the init function
  gZigbee.config();

  TRACECPU1("join network\r\n");

  // set device type and join a network
  gZigbee.setDeviceType(TzdBase::DT_Router);
  gZigbee.startup();
  gSeq.waitForEvent(&gZigbee.flagJoined);

  // send every 5sec a toggle cmd
  uint8_t sendTimer;
  gTs.create(sendTimer, sendNext, true);
  gTs.start(sendTimer, 5000);


/*
 * this code is only for reset testing
 * resets after 60sec the network connection
  uint8_t resetTimer;
  gTs.create(resetTimer, resetNwk, false);
  gTs.start(resetTimer, 60000);
*/
  while(1)
  {
    flagSendNext = false;
    gSeq.waitForEvent(&flagSendNext);

    if(onOffClientCluster1.sendCmdToggle())
      TRACECPU1("send toggle\r\n");
  }
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

	gFlash.Init();

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
