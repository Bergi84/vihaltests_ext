/*
 * traces.cpp
 *
 *  Created on: 08.04.2022
 *      Author: Bergi
 */

#include "traces.h"

TTrace gTrace;

constexpr const char TTrace::strCpu1[];
constexpr const char TTrace::strCpu2[];

bool TTrace::init(THwUart* aUart, TLowPowerManger* aLpm)
{
  if(aUart == 0)
    uart = &conuart;
  else
    uart = aUart;

  lpm = aLpm;
  if(lpm != 0)
  {
    lpm->registerApp(lpmId);
  }

  bufWInd = 0;
  bufRInd = 0;

  lastAktiv = LA_NONE;

  return true;
}

void TTrace::traceCpu1(const char* format, ...)
{
  va_list argptr;
  va_start(argptr, format);

  char locBuf[128];
  uint32_t pos = mp_vsnprintf(&locBuf[0], 128, format, argptr);
  va_end(argptr);

  TCriticalSection cSec(true);

  if(lastAktiv != LA_CPU1)
  {
    lastAktiv = LA_CPU1;
    for(int i = 0; i < strCpu1Len; i++)
    {
      buf[bufWInd] = strCpu1[i];
      bufWInd = (bufWInd < bufLength-1) ? (bufWInd + 1) : 0;
    }
    intend = false;
  }

  for(int i = 0; i < pos; i++)
  {
    if(intend && (locBuf[i] != '\r'))
    {
      intend = false;

      for(int j = 0; j < strCpuIndent; j++)
      {
        buf[bufWInd] = ' ';
        bufWInd = (bufWInd < bufLength-1) ? (bufWInd + 1) : 0;
      }
    }

    buf[bufWInd] = locBuf[i];
    bufWInd = (bufWInd < bufLength-1) ? (bufWInd + 1) : 0;

    if(locBuf[i] == '\n')
    {
      intend = true;
    }
  }

  cSec.leave();
}

void TTrace::traceCpu2(const char* format, ...)
{
  va_list argptr;
  va_start(argptr, format);

  char locBuf[128];
  uint32_t pos = mp_vsnprintf(&locBuf[0], 128, format, argptr);
  va_end(argptr);

  TCriticalSection cSec(true);

  if(lastAktiv != LA_CPU2)
  {
    lastAktiv = LA_CPU2;
    for(int i = 0; i < strCpu2Len; i++)
    {
      buf[bufWInd] = strCpu2[i];
      bufWInd = (bufWInd < bufLength-1) ? (bufWInd + 1) : 0;
    }
    intend = false;
  }

  for(int i = 0; i < pos; i++)
  {
    if(intend && (locBuf[i] != '\r'))
    {
      intend = false;

      for(int j = 0; j < strCpuIndent; j++)
      {
        buf[bufWInd] = ' ';
        bufWInd = (bufWInd < bufLength-1) ? (bufWInd + 1) : 0;
      }
    }

    buf[bufWInd] = locBuf[i];
    bufWInd = (bufWInd < bufLength-1) ? (bufWInd + 1) : 0;

    if(locBuf[i] == '\n')
    {
      intend = true;
    }
  }

  cSec.leave();
}
