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


#ifndef OGGABSPLAYBACK_H
#define OGGABSPLAYBACK_H

#include <OggOs.h>
#include "OggFileInfo.h"
#include "OggRateConvert.h"

const TInt KMaxVolume = 100;
const TInt KStepVolume = 10;


#if defined(PLUGIN_SYSTEM)
#include <MdaAudioSampleEditor.h>

class CPluginInfo : public CBase
	{
public:
    static CPluginInfo* NewL( const TDesC& anExtension, 
	const CMMFFormatImplementationInformation &aFormatInfo, const TUid aControllerUid);
	~CPluginInfo();
    
private :
	CPluginInfo();
	void ConstructL(const TDesC& anExtension, 
	const CMMFFormatImplementationInformation &aFormatInfo, const TUid aControllerUid);
  
public:
	HBufC* iExtension;
	HBufC* iName;
	HBufC* iSupplier;
	TInt iVersion;
	TUid iFormatUid;
	TUid iControllerUid;
	};

class CPluginSupportedList: public CBase
	{
public:
	CPluginSupportedList();
	~CPluginSupportedList();
        
	void ConstructL();

	void AddPluginL(const TDesC &extension,
	const CMMFFormatImplementationInformation &aFormatInfo, const TUid aControllerUid);
            
	void SelectPluginL(const TDesC &extension, TUid aUid);
	CPluginInfo* GetSelectedPluginInfo(const TDesC &extension);
	CArrayPtrFlat <CPluginInfo>* GetPluginInfoList(const TDesC &extension);

	void AddExtension(const TDesC &extension);
	CDesCArrayFlat* SupportedExtensions();

private:
	TInt FindListL(const TDesC &anExtension);

	class TExtensionList
		{
	public:
		TFileName extension;
		CPluginInfo *selectedPlugin;
		CArrayPtrFlat <CPluginInfo>* listPluginInfos; 
		};

	// Plugins that have been selected
	CArrayPtrFlat <TExtensionList>* iPlugins;
	};
#endif


class MPlaybackObserver
	{
public:
	virtual void NotifyUpdate() = 0;
	virtual void NotifyPlayComplete() = 0;
	virtual void NotifyPlayInterrupted() = 0;
	virtual void ResumeUpdates() = 0;

#if !defined(PLUGIN_SYSTEM)
	virtual void NotifyStreamOpen(TInt aErr) = 0;
#endif

#if defined(DELAY_AUDIO_STREAMING_OPEN)
	virtual void NotifyFileOpen(TInt aErr) = 0;
#endif

#if defined (DELAY_AUDIO_STREAMING_START)
	virtual void NotifyPlayStarted() = 0;
#endif

	virtual void NotifyFatalPlayError() = 0;
	};

class MFileInfoObserver
	{
public:
	virtual void FileInfoCallback(TInt aErr, const TOggFileInfo& aFileInfo) = 0;
	};

class CAbsPlayback : public CBase
	{
public:
	// Playback states
	enum TState
		{
		EClosed,		// The playback engine is closed (playback is not possible)
		EStreamOpen,	// The playback engine is open, but not ready (playback is not possible)
		EStopped,		// The playback engine is ready to play
		EPaused,		// The playback engine is paused
		EPlaying		// The playback engine is playing 
		};

	CAbsPlayback(MPlaybackObserver* aObserver);

	// Here is a bunch of abstract methods which need to implemented for each audio format
	virtual void ConstructL() = 0;

	// Open a file and retrieve information (meta data, file size etc.) without playing it
	virtual TInt Info(const TDesC& aFileName, MFileInfoObserver& aFileInfoObserver) = 0;
	virtual TInt FullInfo(const TDesC& aFileName, MFileInfoObserver& aFileInfoObserver) = 0;
	virtual void InfoCancel() = 0;

	// Open a file and prepare playback
	virtual TInt Open(const TDesC& aFileName) = 0;
	virtual const TOggFileInfo& DecoderFileInfo();

	virtual void Pause() = 0;
	virtual void Resume() = 0;
	virtual void Play() = 0;
	virtual void Stop() = 0;

	virtual void SetVolume(TInt aVol) = 0;
	virtual void SetPosition(TInt64 aPos) = 0;

	virtual TInt64 Position() = 0;
	virtual TInt Volume() = 0;
  
	virtual const TInt32* GetFrequencyBins() = 0;

#if defined(PLUGIN_SYSTEM)
	virtual CPluginSupportedList& GetPluginListL() = 0;
#endif

	// Implemented Helpers 
	virtual TState State();
   
	// Information about the playback file 
	virtual TInt BitRate();
	virtual TInt Channels();
	virtual TInt FileSize();
	virtual TInt Rate();
	virtual TInt64 Time();

	virtual const TDesC& Album();
	virtual const TDesC& Artist();
	virtual const TFileName& FileName();
	virtual const TDesC& Genre();
	virtual const TDesC& Title();
	virtual const TDesC& TrackNumber();

	virtual void ClearComments();
	virtual void SetVolumeGain(TGainType aGain);

protected:
	TState iState;
	MPlaybackObserver* iObserver;

	// File properties and tag information
	TOggFileInfo iFileInfo;

#if defined(PLUGIN_SYSTEM)
	CPluginSupportedList iPluginSupportedList;
#endif
};

#endif
