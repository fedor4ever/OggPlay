
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

  void OpenL(const TDesC& aFileName);

  COggMsgEnv*               iEnv;
  
  TBuf<100> iFilename;
  TInt iError;

#ifdef MMF_AVAILABLE
  CMdaAudioPlayerUtility *iPlayer;
#else
  CPseudoMMFController * iPlayer;
  // Use RLibrary object to interface to the DLL
  RLibrary iLibrary;
#endif

};

#endif