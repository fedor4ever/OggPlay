/*
*  Copyright (c) 2004 OggPlay Team
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

#include <OggOs.h>
#include <badesca.h>
#include "OggRateConvert.h"  // For TGainType
#include "OggMsgEnv.h"
#include "OggAbsPlayback.h"

#include <MdaAudioSampleEditor.h>
#ifndef MMF_AVAILABLE
#include <e32uid.h>
#include <OggPlayPlugin.h>
#endif

#ifndef OGGPLUGINADAPTOR_H
#define OGGPLUGINADAPTOR_H

class COggPluginAdaptor :  public CAbsPlayback,  public MMdaObjectStateChangeObserver
{
  
 public:
  
  COggPluginAdaptor(COggMsgEnv* anEnv, MPlaybackObserver* anObserver);
  virtual ~COggPluginAdaptor();
  void ConstructL();

  virtual TInt   Info(const TDesC& aFileName, TBool silent= EFalse);
  virtual TInt   Open(const TDesC& aFileName);

  virtual void   Pause();
  virtual void   Resume();
  virtual void   Play();
  virtual void   Stop();
  virtual void   SetVolume(TInt aVol);
  virtual void   SetPosition(TInt64 aPos);

  virtual TInt64 Position();
  virtual TInt64 Time();
  virtual TInt   Volume();
#ifdef MDCT_FREQ_ANALYSER
  virtual const TInt32 * GetFrequencyBins(TTime aTime);
#else
  virtual const void* GetDataChunk();
#endif
  virtual void SetVolumeGain(TGainType aGain);
  
  virtual CDesCArrayFlat * SupportedExtensions();
  virtual CExtensionSupportedPluginList & GetPluginListL(const TDesC & anExtension);

  private:
      // From MMdaObjectStateChangeObserver
  virtual void MoscoStateChangeEvent(CBase* aObject, TInt aPreviousState, TInt aCurrentState, TInt aErrorCode);
 private:
      // New Functions
  void SearchPluginsL(const TDesC &aName);
  void OpenL(const TDesC& aFileName);
  void ConstructAPlayerL(const TDesC &aFileName);
  void ParseMetaDataValueL(CMMFMetaDataEntry &aMetaData, TDes & aDestinationBuffer);
  COggMsgEnv*               iEnv;
  
  TBuf<100> iFilename;
  TInt iError;

  CArrayPtrFlat <CExtensionSupportedPluginList>* iExtensionSupportedPluginList; 

#ifdef MMF_AVAILABLE
  // RecorderUtility is used instead of PlayerUtility to be able to choose which plugin
  // we will use, in case of conflits (2 codecs supporting same type)
  // The PlayerUtility doesn't allow this choice.
  CMdaAudioRecorderUtility *iPlayer; 
  CActiveSchedulerWait iWait;
#else
  CPseudoMMFController * iPlayer;
  
  CPseudoMMFController * iOggPlayer;
  CPseudoMMFController * iMp3Player;
  // Use RLibrary object to interface to the DLL
  RLibrary iOggLibrary;
  RLibrary iMp3Library;
#endif

  TTimeIntervalMicroSeconds iLastPosition;
  TBool iInterrupted;
  TInt iVolume;
};

#endif