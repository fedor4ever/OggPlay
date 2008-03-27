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

#include <OggOs.h>
#include <barsread.h>
#include <eikbtpan.h>
#include <eikcmbut.h>
#include <eiklabel.h>
#include <eikmover.h>
#include <eikbtgpc.h>
#include <eikon.hrh>
#include <eikon.rsg>
#include <charconv.h>
#include <hal.h>

#include <utf.h>
#include <uri16.h>
#include <uri8.h>
#include <UriUtils.h>

#include <OggPlay.rsg>

#include "OggLog.h"
#include "OggPlay.h"
#include "OggTremor.h"
#include "OggHttpSource.h"
#include "TremorDecoder.h"

#if defined(MP3_SUPPORT)
#include "MadDecoder.h"
#endif


COggHttpSource::COggHttpSource()
: CActive(EPriorityLow), iHdrPtr(NULL, 0), iDataPtr(NULL, 0)
	{
	}

COggHttpSource::~COggHttpSource()
	{
	Close();
	}

#if 0
#if 0
// not used - decoder source fifo is filled by RunL instead
TInt COggHttpSource::Read(TDes8&)
{
  User::Leave(KErrNotSupported);
  return KErrNotSupported;
}
#else
// old style blocking read
TInt COggHttpSource::Read(TDes8& aDes)
{
  if (iState == EFillingData) iState= EReadingData;
  if (iState != EReadingData) return KErrNotReady;
  TSockXfrLength len= 0;
  TRequestStatus status;
  iSock.RecvOneOrMore(aDes, 0, status, len);
  User::WaitForRequest(status);
  return status.Int();
}
#endif
#endif

void COggHttpSource::Close()
	{
	if (IsAdded())
		Deque();

	if (iObserver)
		{
		TInt err = iStatus.Int();
		if ((err != KErrNone))
			{
			// Report the error if we are opening and the open
			// has been cancelled or if the error is not KErrCancel
			TBool reportError = (err == KErrCancel) && ((iState == EResolving) || (iState == EConnecting));
			reportError |= (err != KErrCancel);

			if (reportError)
				iObserver->HttpError(err);
			}

		iObserver = NULL;
		}

	if (iHdrBf)
		{
		delete iHdrBf;
		iHdrBf = NULL;
		}

	iResolver.Close();
	iSock.Close();
	iSocketServ.Close();

	iState = EClosed;
	iAutoFeedLen = 0;
	}

// BufferedBytes() is sum of prebuffered source data in
//    socket buffer
//    decoder fifo
TInt COggHttpSource::BufferedBytes(TInt type)
{
  TInt i= 0;
  if (! type) type= EBufTypeSocket | EBufTypeDecoderInternal;
  if (type & EBufTypeSocket && iState >= EFillingData){
    iSock.GetOpt(KSOReadBytesPending, KSOLSocket, i);
  }
  if ((type & EBufTypeDecoderInternal) && iAsyncCB.iFifoLength) {
    i+= iAsyncCB.iFifoLength(iAsyncCB.iDecoder, ECurrentlyFilled);
  }
  if ((type & EMaxBufLen)) {
//    if (! iBT) {
//      iSock.GetOpt(KSoTcpRecvWinSize, KSolInetTcp, i);
//    }
    i= iAsyncCB.iFifoLength(iAsyncCB.iDecoder, EMaxFifoLen);
  }
  return i;
}

TBool COggHttpSource::CheckRead()
	{
	TInt err = iStatus.Int();
	if (err != KErrNone)
		return EFalse;

	if ((iAutoFeedLen>0) && (iAutoFeedLen<iAsyncCB.iFifoLength(iAsyncCB.iDecoder, ECurrentlyFilled)))
		{
		// Fifo filled enough, stop reading for now
		return EFalse;
		}

	return ETrue;
	}

// Check the length of decoder input fifo and start reading from socket
const TInt KSourceBufferSize = 4096;
TInt COggHttpSource::CheckAndStartRead()
	{
	TInt err = iStatus.Int();
	if (err != KErrNone)
		return err;

	if ((iAutoFeedLen>0) && (iAutoFeedLen<iAsyncCB.iFifoLength(iAsyncCB.iDecoder, ECurrentlyFilled)))
		{
		// Fifo filled enough, stop reading for now
		return KErrNone;
		}

	return StartRead();
	}

TInt COggHttpSource::StartRead()
	{
	if (iDataPtr.Ptr() && !iLen())
		{
		// we have an allocated unused buffer
		// so use it
		}
	else
		{
		if (!iFillingData)
			Lock();

		iDataPtr.Set(iAsyncCB.iGetBuffer(iAsyncCB.iDecoder, KSourceBufferSize));

		Unlock();
		iFillingData = EFalse;

		if (!iDataPtr.Ptr())
			{
			TRACEF(_L("COggHttpSource::StartRead, KErrNoMemory"));
			return KErrNoMemory;
			}
		}

	// Add rest of data from iHdrBf
	if (iHdrBf && iLeftFromHeaders)
		{
		TRACEF(_L("COggHttpSource::CheckAndStartRead copying rest after header scan"));
		if (iLeftFromHeaders>iDataPtr.MaxSize())
			{
			TRACEF(_L("COggHttpSource::CheckAndStartRead ==PROBLEM rest after header scan is bigger than buffer"));
			iLeftFromHeaders = iDataPtr.MaxSize();
			}

		iDataPtr = iHdrPtr.Right(iLeftFromHeaders);
		iLen = iLeftFromHeaders;
		iLeftFromHeaders = 0;
		
		TRequestStatus* s = &iStatus;
		User::RequestComplete(s, KErrNone);
		}
	else
		{
		iDataPtr.SetLength(0);
		iLen = 0;
		
		// Send request for more data
		iSock.RecvOneOrMore(iDataPtr, 0, iStatus, iLen);
		}

	if (iHdrBf)
		{
		delete iHdrBf;
		iHdrBf = NULL;
		}

	SetActive();
	return KErrNone;
	}


_LIT(KHttp, "http");
TBool COggHttpSource::IsNameValid(const TDesC& aUrl)
	{
	TUriParser uri; 
	if ((uri.Parse(aUrl) == KErrNone) && uri.IsSchemeValid())
		{
		if (uri.Extract(EUriScheme) == KHttp)
			return ETrue;
		}

	return EFalse;
	}

TInt COggHttpSource::Open(const TDesC& aUrl, MHttpSourceObserver& aObserver)
	{
	__ASSERT_ALWAYS(!iHdrBf, User::Panic(_L("COHS::Open"), 0));

	CActiveScheduler::Add(this);
	iHdrBf = HBufC8::New(KSourceBufferSize);
	if (!iHdrBf)
		{
		Close();
		return KErrNoMemory;
		}

	iHeadersScanned = 0;
	iAutoFeedLen = 0;

	TUriParser uri;
	TInt err = uri.Parse(aUrl);
	if (err != KErrNone)
		{
		Close();
		return err;
		}

	iHost = uri.Extract(EUriHost);

	// Open channel to Socket Server
	err = iSocketServ.Connect();
	if (err != KErrNone)
		{
		Close();
		return err;
		}

	iAddress.SetPort(80);

#if defined(HTTP_OVER_BT)
	iBTAddr.SetPort(18);
	iBT = EFalse;
#endif

	if (uri.IsPresent(EUriPort))
		{
		TLex ps = uri.Extract(EUriPort);
		TInt i;
		if (ps.Val(i) == KErrNone)
			{
			iAddress.SetPort(i);

#if defined(HTTP_OVER_BT)
// dont change port number to bt proxy iBTAddr.SetPort(i);
#endif
			}
		}
  
#if defined(HTTP_OVER_BT)
	if (iEnv->Settings().iBTProxy != TBTDevAddr(0))
		{
		// does not work on p900    CBTOnOff::SetBluetoothRadio(CBTOnOff::EBTRadioOn);
		User::LeaveIfError(iSock.Open(iSocketServ, _L("RFCOMM")));
		VERBOSETRACE(_L("COggHttpSource::OpenL btSock.Open"));
		iBTAddr.SetBTAddr(iEnv->Settings().iBTProxy);
		iState = EConnecting;
		iSock.Connect(iBTAddr, iStatus);
		iBT = ETrue;
		}
	else 
#elif defined DIRECT_BT_ADDR
	TBuf<10> sch(uri.Extract(EUriScheme));
	if (sch == _L("rfcomm") || sch == _L("l2cap"))
		{
		sch.UpperCase();
		User::LeaveIfError(iSock.Open(iSocketServ, sch));
		VERBOSETRACE(_L("COggHttpSource::OpenL btSock.Open"));

		TLex16 l(iHost);
		TInt64 a= 0;
		l.Val(a, EHex);

		iBTAddr.SetBTAddr(TBTDevAddr(a));
		iState = EConnecting;
		iSock.Connect(iBTAddr, iStatus);
		iHost.SetLength(0); // don't send numeric bt addr as the host name
		iBT = ETrue;
		}
	else 
#endif
	{

	TBuf8<KMaxFileName> p, h;
#if 0 // may be used for SOS < 7.0
	CCnvCharacterSetConverter* characterSetConverter= CCnvCharacterSetConverter::NewL();
	// check to see if the character set is supported - if not then leave
	if (characterSetConverter->PrepareToConvertToOrFromL(KCharacterSetIdentifierAscii, aFs) != CCnvCharacterSetConverter::EAvailable)
		User::Leave(KErrNotSupported);

	TInt r = characterSetConverter->ConvertFromUnicode(p, uri.Extract(EUriPath));
	r = characterSetConverter->ConvertFromUnicode(h, iHost);
	delete characterSetConverter;
#else
	CUri8 *uri8 = NULL;
	TRAP(err, uri8 = UriUtils::ConvertToInternetFormL(uri));
	if (err != KErrNone)
		{
		Close();
		return err;
		}

	p = uri8->Uri().Extract(EUriPath);
	if (iHost.Length()>0)
		h = uri8->Uri().Extract(EUriHost);

	delete uri8;
#endif

	iHdrPtr.Set(iHdrBf->Des());
	iHdrPtr.Format(_L8(
	"GET %S HTTP/1.0\r\n"
	"User-Agent: symbianoggplay/0.1\r\n"
	//Connection: Keep-Alive
	"Accept: */*\r\n"), &p);

	if (iHost.Length())
		iHdrPtr.AppendFormat(_L8("Host: %S:%d\r\n"), &h, iAddress.Port());
	iHdrPtr.Append(_L8("\r\n"));

	// Open a TCP socket
	err = iSock.Open(iSocketServ, KAfInet, KSockStream, KProtocolInetTcp);
	if (err != KErrNone)
		{
		Close();
		return err;
		}

	// iSock.SetOpt(KSoTcpRecvWinSize, KSolInetTcp, 0xfe00);
	if (TChar(iHost[0]).IsDigit())
		{
		err = iAddress.Input(iHost);
		if (err != KErrNone)
			{
			Close();
			return err;
			}

		iState = EConnecting;
		iSock.Connect(iAddress, iStatus);
		iHost.SetLength(0); // don't send numeric ip addr as the host name
		}
	else
		{
		// DNS request for name resolution
		err = iResolver.Open(iSocketServ, KAfInet, KProtocolInetUdp);
		if (err != KErrNone)
			{
			Close();
			return err;
			}

		iState = EResolving;
		iResolver.GetByName(iHost, iNameEntry, iStatus);
		}
	}

	iObserver = &aObserver;
	SetActive();

	return KErrNone;
	}

#ifdef DEBUG_TRAP
TInt COggHttpSource::RunError(TInt aError)
{
  TRACETRAP("COggHttpSource::RunL ==******== Leaved");
  return aError;
}
#endif

void COggHttpSource::RunL()
	{
	TInt i, j, l;
	TInt err = iStatus.Int();
	switch (iState)
		{
		case EResolving:
			if (err != KErrNone)
				{
				Close();
				break;
				}

			// DNS look up successful
			iResolver.Close();
			iAddress.SetAddress(TInetAddr::Cast(iNameEntry().iAddr).Address());

			iState = EConnecting;
			iObserver->HttpStateUpdate(iState);

			iSock.Connect(iAddress, iStatus);
			SetActive();
			break;

		case EConnecting:
			if (err != KErrNone)
				{
				Close();
				break;
				}

			iState = ESendingRequest;
			iObserver->HttpStateUpdate(iState);

			iSock.Write(*iHdrBf, iStatus);
			SetActive();
			break;

		case ESendingRequest:
			if (err != KErrNone)
				{
				Close();
				break;
				}

			iState = EReadingHeaders;
			iObserver->HttpStateUpdate(iState);

			iSock.RecvOneOrMore(iHdrPtr, 0, iStatus, iLen);
			SetActive();
			break;

		case EReadingHeaders:
			if (err != KErrNone)
				{
				Close();
				break;
				}

			j = 0;
			while (j<iHdrPtr.Length())
				{
				if ((iHdrState == ECr) && (iHdrPtr[j] == '\n'))
					{
					iHdrState = ELf;
					j++;

					ProcessHeaderLine();
					iHdrLine.SetLength(0);
					if (iState != EReadingHeaders)
						break;
					}

				if (j>=iHdrPtr.Length())
					break;

				i = iHdrPtr.Mid(j).Locate('\r');
				if (i == KErrNotFound)
					{
					l = Min(iHdrPtr.Length() - j, iHdrLine.MaxLength() - iHdrLine.Length());
					iHdrState = ELine;
					iHdrLine.Append(iHdrPtr.Mid(j, l));
					break;  // line split over packet boundary
					}

				iHdrState = ECr;
				l = Min(i, iHdrLine.MaxLength() - iHdrLine.Length());
				iHdrLine.Append(iHdrPtr.Mid(j, l));
				j += l + 1;
				}

			if (iState == EReadingHeaders)
				{
				iSock.RecvOneOrMore(iHdrPtr, 0, iStatus, iLen);
				SetActive();
				break;
				}

			if (iState == EFillingData)
				{
				if (j<iHdrPtr.Length())
					{
					// Header and data in one packet
					iLeftFromHeaders = iHdrPtr.Length() - j;
					}

				// Start auto feeding
				if (iAutoFeedLen>0)
					CheckAndStartRead();
				}
			break;

		case EFillingData:
			// inform decoder about source data readed in buffer
			// start a new read if more data needed
			if (err == KErrNone)
				{
				if (iLen())
					{
					TBool openComplete = EFalse;
					iFillingData = ETrue;

					Lock();
					TInt fifoLen = iAsyncCB.iWrote(iAsyncCB.iDecoder, iLen(), openComplete);

					// TO DO: Handle fifoLen<0 (codec error)
					// iStatus = ?; Close();

					if ((iAutoFeedLen>0) && (fifoLen>=0) && (iAutoFeedLen>fifoLen))
						CheckAndStartRead();

					if (iFillingData)
						{
						Unlock();
						iFillingData = EFalse;
						}

					if (openComplete)
						iOpenComplete = ETrue;

					if (iOpenComplete && !iOpenCompleted && fifoLen>=8192 /*iStartPlaybackLen*/)
						{
						iObserver->HttpOpenComplete();
						iOpenCompleted = ETrue;
						}
					else if (iOpenCompleted)
						iObserver->HttpDataReceived();
					}
				}
			else if ((err == KErrEof) || (err == KErrDisconnected))
				Close();  // Stop reading at eof
			else
				{
				TRACEF(_L("======http feeder STOPped on net err"));
				Close();  // TODO: allow retries
				// iEnv->OggErrorMsgL(R_OGG_ERROR_NETERR,R_OGG_ERROR_SEND);
				}
			break;

		default:
			break;
		}
	}

void COggHttpSource::DoCancel()
		{
		// Cancel the currect asynchrnous request
		if (iState == EResolving)
			iResolver.Cancel();
		else
			iSock.CancelAll();
		}

void COggHttpSource::ProcessHeaderLine()
	{
	if (iHeadersScanned == 0)
		{
		if (iHdrLine.Left(9) != _L8("HTTP/1.0 "))
			{
			if (iHdrLine.Left(9) != _L8("HTTP/1.1 "))
				{
				TRACEF(_L("CHttpSource::Open Unexpected http header"));
				iStatus = KErrNotSupported;

				Close();
				// iEnv->OggErrorMsgL(R_OGG_ERROR_NETERR,R_OGG_ERROR_HTTP);
				return;
				}
			}

		TLex8 ps = iHdrPtr.Mid(9);
		TInt i;
		if ((ps.Val(i) != KErrNone) || ((i/100) != 2))
			{
			TRACEF(_L("COggHttpSource::RunL Unexpected http response"));
			iStatus = KErrNotSupported;

			Close();
			// iEnv->OggErrorMsgL(R_OGG_ERROR_NETERR,R_OGG_ERROR_NOTFND);
			return;
			}
		}
	else
		{
		if (iHdrLine.Length() == 0)
			{
			// Header scan complete
			iState = EFillingData;
			iObserver->HttpStateUpdate(iState);
			return;
			}
		}

	iHeadersScanned++;
	}

void COggHttpSource::Lock()
	{
#if !defined(__WINS__)
	volatile TUint32 locked;
	TUint32 *lockAddr = &iLock;
	for ( ; ; )
		{
		// Attempt to get the lock
		asm __volatile__ ("swp %0, %1, [%2]"
		: "=&r"(locked)
		: "r"(1), "r"(lockAddr)
		: "memory");

		// If we get the lock, exit the loop
		if (!locked)
			break;

		// No luck, so try again a bit later
		User::After(1000);
		}
#endif
	}

void COggHttpSource::Unlock()
	{
#if !defined(__WINS__)  
	// Clear the lock
	iLock = 0;
#endif
	}
