/*
 *  Copyright (c) 2006 Tomas Vanek
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

#ifndef _HttpSource_h
#define _HttpSource_h

#include <e32std.h>
#include <es_sock.h>
#include <in_sock.h>
#include "OggSource.h"

#if defined(HTTP_OVER_BT)
#include "bt_sock.h"
#endif

class MHttpSourceObserver
	{
public:
	virtual void HttpStateUpdate(TInt aState) = 0;
	virtual void HttpOpenComplete() = 0;

	virtual void HttpDataReceived() = 0;
	virtual void HttpError(TInt aErr) = 0;
	};

class COggHttpSource : public CActive, public MOggSource
	{
public: 
	COggHttpSource();
	~COggHttpSource();

	static TBool IsNameValid(const TDesC& aUrl);

	// From MOggSource
	TInt Open(const TDesC& aUrl, MHttpSourceObserver& aObserver);
	// TInt Read(TDes8& aDes);
	void Close();
  
	TSourceType Type() const
	{ return EHttpStream; }

	TInt BufferedBytes(TInt aType= 0);

	TBool CheckRead();
	TInt CheckAndStartRead();
	TInt StartRead();

	void SetAutoBuffering(TInt autoFeedLen)
	{ iAutoFeedLen = autoFeedLen; }

	TBool AsyncFeeding(const TAsyncCB& aAsyncCB)
	{ iAsyncCB = aAsyncCB; return ETrue; }

	void Lock();
	void Unlock();
	
private:
	enum THttpState { EClosed, EResolving, EConnecting, ESendingRequest, EReadingHeaders, EFillingData /*, EReadingData */ };
	enum THdrState { ELine, ECr, ELf };

private:
	void ProcessHeaderLine();

	// from CActive
	void RunL();
	void DoCancel();

private:
	THttpState iState;
	THdrState iHdrState;

	// Network related
	RSocketServ iSocketServ;
	RSocket iSock;
	RHostResolver iResolver;

	TFileName iHost;
	TNameEntry iNameEntry;
	TInetAddr iAddress;

#if defined(HTTP_OVER_BT)
	TBTSockAddr iBTAddr;
	TBool iBT;
#endif

	TSockXfrLength iLen;

	HBufC8* iHdrBf;
	TPtr8  iHdrPtr;

	TBuf8<256> iHdrLine;
	TInt iHeadersScanned;
	TInt iLeftFromHeaders;

	// Decoder interface
	TAsyncCB iAsyncCB;
	TPtr8 iDataPtr;
	TInt iAutoFeedLen;

#ifdef DEBUG_TRAP
	TInt RunError(TInt aError);
#endif

	MHttpSourceObserver* iObserver;
	TBool iOpenComplete;
	TBool iOpenCompleted;

	TUint32 iLock;
	TBool iFillingData;
	};

#endif//_HttpSource_h
