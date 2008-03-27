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

#ifndef OGGPLUGINADAPTOR_H
#define OGGPLUGINADAPTOR_H

#include <OggOs.h>
#include <badesca.h>
#include <MdaAudioSampleEditor.h>

#include "OggRateConvert.h"
#include "OggMsgEnv.h"
#include "OggAbsPlayback.h"

class COggPluginAdaptor;
class TUtilFileOpenObserver : public MMdaObjectStateChangeObserver
	{
public:
	void SetFileInfo(const TDesC& aFileName);

private:
	// From MMdaObjectStateChangeObserver
	void MoscoStateChangeEvent(CBase* aObject, TInt aPreviousState, TInt aCurrentState, TInt aErrorCode);

public:
	TBool iFullInfo;
	TUid iControllerUid;
	TOggFileInfo iFileInfo;

	MFileInfoObserver* iFileInfoObserver;
	};

class COggPluginAdaptor :  public CAbsPlayback,  public MMdaObjectStateChangeObserver
	{
public: 
	COggPluginAdaptor(COggMsgEnv* aEnv, CPluginSupportedList& aPluginSupportedList, MPlaybackObserver* aObserver);
	~COggPluginAdaptor();
 
	void ConstructL();

	// From CAbsPlayback
	TInt Info(const TDesC& aFileName, TUid aControllerUid, MFileInfoObserver& aFileInfoObserver);
	TInt FullInfo(const TDesC& aFileName, TUid aControllerUid, MFileInfoObserver& aFileInfoObserver);
	void InfoCancel();

	TInt Open(const TOggSource& aSource, TUid aControllerUid);

	void Pause();
	void Resume();
	void Play();
	void Stop();
	void SetVolume(TInt aVol);
	void SetPosition(TInt64 aPos);
	TBool Seekable();

	TInt64 Position();
	TInt64 Time();
	TInt Volume();
	const TInt32* GetFrequencyBins();
	void SetVolumeGain(TGainType aGain);

private:
	// From MMdaObjectStateChangeObserver
	void MoscoStateChangeEvent(CBase* aObject, TInt aPreviousState, TInt aCurrentState, TInt aErrorCode);

	// New Functions
	void SearchPluginsL(const TDesC &aName, TBool isEnabled);

	void OpenL(const TDesC& aFileName, TUid aControllerUid);
	void OpenInfoL(const TDesC& aFileName, TUid aControllerUid, MFileInfoObserver& aFileInfoObserver, TBool aFullInfo = EFalse);

	void ConstructAPlayerL();
	void ConstructAFileInfoUtilL();

	static void GetAudioProperties(TOggFileInfo& aFileInfo, CMdaAudioRecorderUtility* aUtil, TUid aControllerUid);
	static void ParseMetaDataValueL(CMMFMetaDataEntry &aMetaData, TDes & aDestinationBuffer);
	static void RetrieveFileInfo(CMdaAudioRecorderUtility* aUtil, TOggFileInfo& aFileInfo, TBool aFullInfo, TUid aPluginControllerUid);

private:
	COggMsgEnv* iEnv;
  
	TFileName iFilename;
	TInt iError;

	// RecorderUtility is used instead of PlayerUtility to be able to choose which plugin
	// we will use, in case of conflits (2 codecs supporting same type)
	// The PlayerUtility doesn't allow this choice.
	CMdaAudioRecorderUtility *iPlayer; 
	TUid iPluginControllerUID;
	TInt32 iFreqBins[16];

	TTimeIntervalMicroSeconds iLastPosition;
	TBool iInterrupted;

	TInt iVolume;
	TGainType iGain;

	CMdaAudioRecorderUtility* iFileInfoUtil; 
	TUtilFileOpenObserver iFileOpenObserver;
	
	friend class TUtilFileOpenObserver;
	};

#endif
