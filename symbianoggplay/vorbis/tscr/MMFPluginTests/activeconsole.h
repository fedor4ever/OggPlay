#ifndef CACTIVECONSOLE
#define CACTIVECONSOLE 1

#include <e32cons.h>
//////////////////////////////////////////////////////////////////////////////
//
// -----> CActiveConsole (definition)
//
// An abstract class which provides the facility to issue key requests. 
//
//////////////////////////////////////////////////////////////////////////////
class MKeyPressObserver 
{
public:
    virtual TInt ProcessKeyPress(TChar aChar) = 0;
};



class CActiveConsole : public CActive
    {
    public:
        // Construction
        CActiveConsole(CConsoleBase* aConsole );
        void ConstructL();
        
        // Destruction
        ~CActiveConsole();
        
        // Issue request
        void RequestCharacter();
        
        // Cancel request.
        // Defined as pure virtual by CActive;
        // implementation provided by this class.
        void DoCancel();
        
        // Service completed request.
        // Defined as pure virtual by CActive;
        // implementation provided by this class,
        void RunL();
        
        // Called from RunL() - an implementation must be provided
        // by derived classes to handle the completed request
        virtual void ProcessKeyPress(TChar aChar) = 0; 
        
    protected:
        // Data members defined by this class
        CConsoleBase* iConsole; // A console for reading from
        TInt iAction;
        enum {	EWaitForKey=100,
            EWaitForStop};
        
    };

class CEventHandler : public CActiveConsole
    {
    public:
        CEventHandler(CConsoleBase* aConsole, MKeyPressObserver* anObserver);
    private:
        void ProcessKeyPress(TChar aChar);
        MKeyPressObserver* iObserver;
    };


#endif

