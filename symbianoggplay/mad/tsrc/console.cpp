// ResourceManagement.cpp
//
// Copyright (c) 2000 Symbian Ltd.  All rights reserved.

#include <e32cons.h>
#include "activeconsole.h"
#include "madtest.h"
// All messages written to this
LOCAL_D CConsoleBase* console;

class CExampleScheduler : public CActiveScheduler
	{
public:
	void Error (TInt aError) const;
	};

void CExampleScheduler::Error(TInt aError) const
	{
	_LIT(KMsgSchedErr,"CExampleScheduler-error");
	User::Panic(KMsgSchedErr,aError);
	}

_LIT(KTxtEEMemMan,"Berthier Mad Player");
_LIT(KTxtPressAnyKey," [press any key to quit]");

LOCAL_C void mainL()
    {
    console=Console::NewL(KTxtEEMemMan,TSize(KConsFullScreen,KConsFullScreen));
	CleanupStack::PushL(console);                // Push onto the cleanup stack
    // Active Object Scheduler
    CExampleScheduler*  exampleScheduler = new (ELeave) CExampleScheduler;
	CleanupStack::PushL(exampleScheduler);	     // Push onto the cleanup stack
	CActiveScheduler::Install(exampleScheduler); // Install as the active scheduler
  

    CMadtest * decoder;
    ////////////////////////////////////////////
    // Test #1 :
    // Decode mp3 file and save it to a file.
    //
    ////////////////////////////////////////////
    
    decoder =  CMadtest::NewL();
//    CleanupStack::PushL( decoder );

    decoder->SetOutputFileL( _L8("C:\\pcm.out") );
    decoder->SetInputFileL( _L8( "C:\\aFile2.mp3" ) );
//    decoder->SetInputFileL( _L8( "C:\\aFile.mp3" ) );
//    decoder->SetInputFileL( _L8( "C:\\numb_tiny.mp3" ) );

    //while ( decoder->FillSampleBufferL() > 0);
    decoder->MpegAudioDecoder();

//    CleanupStack::PopAndDestroy();
    
	console->Printf(KTxtPressAnyKey);
	console->Getch(); // get and ignore character

    }

//////////////////////////////////////////////////////////////////////////////
//
// Main function called by E32
//
//////////////////////////////////////////////////////////////////////////////
GLDEF_C TInt E32Main() // main function called by E32
    {
	__UHEAP_MARK;

    // Cleanup Stack
	CTrapCleanup* cleanup=CTrapCleanup::New();
   
    // To use the CleanUp Stack, we have to start with a TRAPD before pushing anything.
    // Start the main function
    TRAPD(error,mainL());

    _LIT(KTxtMemMan,"MemMan");
	__ASSERT_ALWAYS(!error,User::Panic(KTxtMemMan,error));

	delete cleanup; // destroy clean-up stack
	__UHEAP_MARKEND;

	return 0; // and return
    }




///////////////////
// CActiveConsole
///////////////////

CActiveConsole::CActiveConsole( CConsoleBase* aConsole) 
: CActive(CActive::EPriorityUserInput)
// Construct high-priority active object
    {
    iConsole = aConsole;
    }

void CActiveConsole::ConstructL()
    {
    // Add to active scheduler
    CActiveScheduler::Add(this);
    }

CActiveConsole::~CActiveConsole()
    {
    // Make sure we're cancelled
    Cancel();
    }

void  CActiveConsole::DoCancel()
    {
    iConsole->ReadCancel();
    }

void  CActiveConsole::RunL()
    {
    // Handle completed request
    if (iAction == EWaitForKey)
        {
        ProcessKeyPress(TChar(iConsole->KeyCode()));
        }
    else
        {
        CActiveScheduler::Stop();
        }
    }
#if 0

void CActiveConsole::RequestCharacter()
    {
    // A request is issued to the CConsoleBase to accept a
    // character from the keyboard. 
    iConsole->Read(iStatus); 
    iAction = EWaitForKey;
    SetActive();
    }

///////////////
// CEventHandler
///////////////

CEventHandler::CEventHandler(CConsoleBase* aConsole)
:CActiveConsole(aConsole)
    {
    ConstructL();
    };



void CEventHandler::ProcessKeyPress(TChar /*aChar*/)
    {
   // RequestCharacter();
    CActiveScheduler::Stop();

    }
#endif
