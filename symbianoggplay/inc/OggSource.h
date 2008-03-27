/*
 *  Copyright (c) 2006
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

#ifndef _OggSource_h
#define _OggSource_h

#include <f32file.h>

class TAsyncCB
	{
public:
	TAsyncCB()
	{ }

	TAsyncCB(TInt (*aFifoLength)(TAny* p, TBool aMax), TPtr8 (*aGetBuffer)(TAny* p, TInt bytes), TInt (*aWrote)(TAny* p, TInt bytes, TBool& aOpenComplete), TAny* p)
	: iFifoLength(aFifoLength), iGetBuffer(aGetBuffer), iWrote(aWrote), iDecoder(p)
	{ }

	TInt (*iFifoLength)(TAny* p, TBool aMax);
	TPtr8 (*iGetBuffer)(TAny* p, TInt bytes);
	TInt (*iWrote)(TAny* p, TInt bytes, TBool& aOpenComplete);

	TAny* iDecoder;
	};

class MOggSource
	{
public:
	enum TSourceType { EFile, EHttpStream };
	enum TBufferType { EBufTypeSocket = 1, EBufTypeDecoderInternal = 2, EMaxBufLen = 4 };
	enum TFifoLength { ECurrentlyFilled, EMaxFifoLen };

public:
	static TBool IsNameValid(const TDesC&)
	{ return ETrue; }

	// virtual TInt Open(const TDesC& aFileName) = 0;
	// virtual TInt Read(TDes8& aDes) = 0;
	virtual TInt Seek(TSeek /*aMode*/, TInt& /*aPos*/)
	{ return KErrNotSupported; }

	virtual TInt Size(TInt& /*aPos*/)
	{ return KErrNotSupported; }

	virtual void Close() = 0;
  
	virtual TSourceType Type() const = 0;
	virtual TInt BufferedBytes(TInt /*aType*/ = 0)
	{ return KMaxTInt; }

	virtual TBool AsyncFeeding(const TAsyncCB& /*aAsyncCB*/)
	{ return EFalse; }

	virtual TInt CheckAndStartRead()
	{ return KErrNotSupported; }

	virtual void SetAutoBuffering(TInt /*autoFeedLen*/)
	{ }

	virtual TInt Error()
	{ return KErrEof; }
	};

// The file source is not used. TO DO: Maybe get rid of this and the MOggSource base class
/*
class COggFileSource : public CBase, public MOggSource
	{
public: 
	COggFileSource(RFs& aFs)
	: iFs(aFs)
	{ }

	~COggFileSource()
	{ Close(); }

	// From MOggSource
	TInt Open(const TDesC& aFileName)
	{ return iFile.Open(iFs, aFileName, EFileShareReadersOnly); }

	TInt Read(TDes8& aDes)
	{ return iFile.Read(aDes); }

	TInt Seek(TSeek aMode, TInt& aPos)
	{ return iFile.Seek(aMode, aPos); }

	TInt Size(TInt& aPos)
	{ return iFile.Size(aPos); }

	void Close()
	{ iFile.Close(); }
  
	TSourceType Type() const
	{ return EFile; }

private:
	RFile iFile;
	RFs& iFs;
}; */

#endif
