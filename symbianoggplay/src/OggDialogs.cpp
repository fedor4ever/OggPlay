/*
 *  Copyright (c) 2003 L. H. Wilden. All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "OggOs.h"
#include "OggDialogs.h"

#include "OggPlay.hrh"
#include <OggPlay.rsg>

#include <eiklabel.h>
#if defined(SERIES60)
// include 
#else
#include <eikchlst.h>
#include <eikchkbx.h>
#include <qiktimeeditor.h>
#endif

CHotkeyDialog::CHotkeyDialog(int *aHotKeyIndex, int* anAlarmActive, TTime* anAlarmTime)
{
  iHotKeyIndex = aHotKeyIndex;
  iAlarmActive= anAlarmActive;
  iAlarmTime= anAlarmTime;
}


TBool
CHotkeyDialog::OkToExitL(int /* aButtonId */)
{
#if !defined(SERIES60)
  CEikChoiceList *cl = (CEikChoiceList*)Control(EOggOptionsHotkey);
  CEikCheckBox* cb= (CEikCheckBox*)Control(EOggOptionsAlarmActive);
  CQikTimeEditor* ct= (CQikTimeEditor*)Control(EOggOptionsAlarmTime);
  *iHotKeyIndex = cl->CurrentItem();
  *iAlarmActive= cb->State();
  *iAlarmTime= ct->Time();
#endif
  return ETrue;
}


void
CHotkeyDialog::PreLayoutDynInitL()
{
#if !defined(SERIES60)
  CEikChoiceList *cl = (CEikChoiceList*)Control(EOggOptionsHotkey);
  CEikCheckBox* cb= (CEikCheckBox*)Control(EOggOptionsAlarmActive);
  CQikTimeEditor* ct= (CQikTimeEditor*)Control(EOggOptionsAlarmTime);
  cl->SetArrayL(R_HOTKEY_ARRAY);
  cl->SetCurrentItem(*iHotKeyIndex);
  cb->SetState((CEikButtonBase::TState)*iAlarmActive);
  ct->SetTimeL(*iAlarmTime);
#endif
}


void
COggInfoDialog::PreLayoutDynInitL()
{
  CEikLabel* cFileName= (CEikLabel*)Control(EOggLabelFileName);
  cFileName->SetTextL(iFileName);

  CEikLabel* cFileSize= (CEikLabel*)Control(EOggLabelFileSize);
  TBuf<256> buf;
  buf.Num(iFileSize);
  cFileSize->SetTextL(buf);

  CEikLabel* cRate= (CEikLabel*)Control(EOggLabelRate);
  buf.Num(iRate);
  cRate->SetTextL(buf);

  CEikLabel* cChannels= (CEikLabel*)Control(EOggLabelChannels);
  buf.Num(iChannels);
  cChannels->SetTextL(buf);

  CEikLabel* cBitRate= (CEikLabel*)Control(EOggLabelBitRate);
  buf.Num(iBitRate);
  cBitRate->SetTextL(buf);
}

void
COggInfoDialog::SetFileName(const TDesC& aFileName)
{
  iFileName.Copy(aFileName);
}

void
COggInfoDialog::SetFileSize(TInt aFileSize)
{
  iFileSize= aFileSize;
}

void
COggInfoDialog::SetRate(TInt aRate)
{
  iRate= aRate;
}

void
COggInfoDialog::SetChannels(TInt aChannels)
{
  iChannels= aChannels;
}

void
COggInfoDialog::SetTime(TInt aTime)
{
  iTime= aTime;
}

void
COggInfoDialog::SetBitRate(TInt aBitRate)
{
  iBitRate= aBitRate;
}


void
COggAboutDialog::PreLayoutDynInitL()
{
  CEikLabel* cVersion= (CEikLabel*)Control(EOggLabelAboutVersion);
  cVersion->SetTextL(iVersion);
}

void
COggAboutDialog::SetVersion(const TDesC& aVersion)
{
  iVersion.Copy(aVersion);
}
