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
#include "OggRateConvert.h"  // For TGainType
#include "OggMsgEnv.h"
#include "OggAbsPlayback.h"

#include <MdaAudioSamplePlayer.h>
#ifndef MMF_AVAILABLE
#include <e32uid.h>
#include <OggPlayPlugin.h>
#endif

#ifndef OGGPLUGINADAPTOR_H
#define OGGPLUGINADAPTOR_H

class CPluginInfo : public CBase
{
private :
  CPluginInfo();
  
  HBufC* iExtension;
  HBufC* iName;
  HBufC* iSupplier;
  TInt iVersion;

public:
   ~CPluginInfo();
   static CPluginInfo* NewL( const TDesC& anExtension, 
	   const TDesC& aName,
	   const TDesC& aSupplier,
	   const TInt aVersion);
   void ConstructL ( const TDesC& anExtension, 
	   const TDesC& aName,
	   const TDesC& aSupplier,
	   const TInt aVersion );

};

class COggPluginAdaptor :  public CAbsPlayback,  public MMdaAudioPlayerCallback
{
  
 public:
  
  COggPluginAdaptor(COggMsgEnv* anEnv, MPlaybackObserver* anObserver);
  virtual ~COggPluginAdaptor();
  void ConstructL();

  virtual TInt   Info(const TDesC& aFileName, TBool silent= EFalse);
  virtual TInt   Open(const TDesC& aFileName);

  virtual void   Pause();
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
  
  virtual void MapcInitComplete(TInt aError, const TTimeIntervalMicroSeconds& aDuration);
  virtual void MapcPlayComplete(TInt aError);
 private:
  void SupportedFormatL();
  void SearchPluginsL(const TDesC &aName);
  void OpenL(const TDesC& aFileName);
  void ConstructAPlayerL();
  COggMsgEnv*               iEnv;
  
  TBuf<100> iFilename;
  TInt iError;

  CArrayPtrFlat <CPluginInfo>* iPluginInfos; 

#ifdef MMF_AVAILABLE
  CMdaAudioPlayerUtility *iPlayer;
#else
  CPseudoMMFController * iPlayer;
  // Use RLibrary object to interface to the DLL
  RLibrary iLibrary;
#endif

};

#endif