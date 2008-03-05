/*
 *  Copyright (c) 2008 S. Fisher
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

#ifndef OGGMTFILE_H
#define OGGMTFILE_H

#include <OggOs.h>
#include <e32std.h>
#include <e32base.h>
#include <f32file.h>

class COggMTSource;
class MMTSourceHandler;
class TMTSourceData
	{
public:
	TInt DataSize()
	{ return iDataWritten-iDataRead; }

public:
	TUint8* iBuf;
	TInt iBufSize;
	TInt iHalfBufSize;

	COggMTSource* iSource;
	TBool iLastBuffer;

	TInt iDataRead;
	TInt iDataWritten;

	TInt iReadIdx;
	};

class MMTSourceHandler
	{
public:
	virtual TInt OpenFile(const TDesC& aFileName, TMTSourceData& aSourceData) = 0;
	virtual TInt OpenStream(const TDesC& aStreamName, TMTSourceData& aSourceData) = 0;
	virtual void SourceClose(TMTSourceData& aSourceData) = 0;

	virtual TInt Read(TMTSourceData& aSourceData, TDes8& aBuf) = 0;

	virtual TInt Read(TMTSourceData& aSourceData) = 0;
	virtual void ReadRequest(TMTSourceData& aSourceData) = 0;
	virtual void ReadCancel(TMTSourceData& aSourceData) = 0;

	virtual TInt FileSeek(TSeek aMode, TInt &aPos, TMTSourceData& aSourceData) = 0;
	virtual TInt FileSize(TInt& aSize, TMTSourceData& aSourceData) = 0;
	};

// Determine the buffer size to use approximately based on speed of hardware
// For example, playback start is noticibly slower on the Sendo with larger buffers (so use smaller ones!)
#if defined(UIQ) || (defined(SERIES60) && !defined(PLUGIN_SYSTEM)) // aka S60V1
const TInt KDefaultRFileMTBufSize = 65536; // 2 x 32K
#elif !defined(SERIES60V3)
const TInt KDefaultRFileMTBufSize = 131072; // 2 x 64K
#else
const TInt KDefaultRFileMTBufSize = 262144; // 2 x 128K
#endif

class RMTFile
	{
public:
	enum TMTBufferingMode { EBufferNone, EDoubleBuffer };

public:
	RMTFile(MMTSourceHandler& aSourceHandler1, MMTSourceHandler& aSourceHandler2);

	TInt Open(const TDesC& aFileName, TInt aBufSize = KDefaultRFileMTBufSize);
	void Close();

	TInt Read(TDes8& aBuf);

	TInt Seek(TSeek aMode, TInt &aPos);
	TInt Size(TInt& aSize);

	void EnableDoubleBuffering();
	void DisableDoubleBuffering();
	void ThreadRelease();

private:
	void CopyData(TUint8* aBuf, TInt aSize);

private:
	TMTSourceData iSourceData;
	TMTBufferingMode iBufferingMode;

	MMTSourceHandler* iSourceHandler;
	MMTSourceHandler& iSourceHandler1;
	MMTSourceHandler& iSourceHandler2;
	TThreadId iThreadId;
	};
#endif
