// ResourceManagement.cpp
//
// Copyright (c) 2000 Symbian Ltd.  All rights reserved.

#include <e32cons.h>
#include "activeconsole.h"
#include "oggtremortest.h"

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

_LIT(KTxtEEMemMan,"Berthier Player");
_LIT(KTxtPressAnyKey," [press any key to quit]");

LOCAL_C void mainL()
    {
    console=Console::NewL(KTxtEEMemMan,TSize(KConsFullScreen,KConsFullScreen));
	CleanupStack::PushL(console);                // Push onto the cleanup stack
    // Active Object Scheduler
    CExampleScheduler*  exampleScheduler = new (ELeave) CExampleScheduler;
	CleanupStack::PushL(exampleScheduler);	     // Push onto the cleanup stack
	CActiveScheduler::Install(exampleScheduler); // Install as the active scheduler
  

    CIvorbisTest * vorbisDecoder;
    
    vorbisDecoder =  CIvorbisTest::NewL(console);
    CleanupStack::PushL( vorbisDecoder );

   
    // KeyPress handler
    CEventHandler * keypressHandler = new(ELeave) CEventHandler( console, vorbisDecoder );
    CleanupStack::PushL( keypressHandler );
    keypressHandler->ConstructL();
    keypressHandler->RequestCharacter();

    CActiveScheduler::Start();

    CleanupStack::PopAndDestroy(2); // CIvorbisTest, keypressHandler

	console->Printf(KTxtPressAnyKey);
	console->Getch(); // get and ignore character

     CleanupStack::PopAndDestroy(2); // Console, Active Scheduler
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

void CActiveConsole::RequestCharacter()
    {
    // A request is issued to the CConsoleBase to accept a
    // character from the keyboard. 
    iConsole->Read(iStatus); 
    SetActive();
    iAction = EWaitForKey;
    }

///////////////
// CEventHandler
///////////////

CEventHandler::CEventHandler(CConsoleBase* aConsole,MKeyPressObserver* anObserver)
:CActiveConsole(aConsole)
    {
    iObserver = anObserver;
    };



void CEventHandler::ProcessKeyPress(TChar aChar)
    {
    if ( iObserver->ProcessKeyPress(aChar) )
        RequestCharacter(); // Wait for an other keypress
    else
        CActiveScheduler::Stop();  // Leave the test.
    }

