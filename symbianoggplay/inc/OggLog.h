
#ifndef _OGGLOG_H
#define _OGGLOG_H

#include <flogger.h>

#define OGGLOG COggLog::InstanceL()->iLog

// OggLog is a class giving access to the RFileLogger facility for all OggPlay classes.
// It's implemented closely to the singleton pattern.

class COggLog : CBase
{

public:
	static COggLog* InstanceL();
    //static RFileLogger LogL();

	RFileLogger iLog;

protected:
	COggLog();
	ConstructL();

private:
	static COggLog* iInstance;


};


#endif
