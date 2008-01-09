
#include <OggOs.h>
#include <e32std.h>
#include <e32base.h>
#include <ivorbiscodec.h>
#include <ivorbisfile.h>

#include <MdaAudioSamplePlayer.h>
#include "OggPluginAdaptor.h"


class CIvorbisTest: public CBase, public MKeyPressObserver, public MPlaybackObserver
	{
public:   
	static CIvorbisTest * NewL(CConsoleBase *aConsole);
	~CIvorbisTest();

	virtual void NotifyPlayComplete();
	virtual void NotifyUpdate();
	virtual void NotifyPlayInterrupted();

private:
	void ConstructL(CConsoleBase *aConsole);
	TInt ProcessKeyPress(TChar aChar);

private:     
	CConsoleBase* iConsole;
	TInt iVolume;
	TFileName iFilename;
	TInt iFilenameIdx;
	COggPluginAdaptor* iPlayer;
    };
