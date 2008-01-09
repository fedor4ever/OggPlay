 /*
 *  Copyright (c) 2004 OggPlay Team.
 * 
 *  Part of this code is from OggPlay Controller example,
 *    Copyright © Symbian Software Ltd 2004.  All rights reserved.
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
   
#ifndef OGGPLAYMMFSTREAMING_H
#define OGGPLAYMMFSTREAMING_H

#include "OggRateConvert.h"

class CMMFOggPlayStreaming : public CBase, public MDataSource, public MOggSampleRateFillBuffer
{

public:
    CMMFOggPlayStreaming();
    ~CMMFOggPlayStreaming();
    
    void SetVolumeGain(TGainType aGain);
    void SetVolume(TInt aVol);
    TInt Volume();
    TInt MaxVolume();

    // from MDataSource:
    virtual TFourCC SourceDataTypeCode(TMediaId aMediaId);
    virtual void FillBufferL(CMMFBuffer* aBuffer, MDataSink* aConsumer,TMediaId aMediaId);
    virtual void BufferEmptiedL(CMMFBuffer* aBuffer);
    virtual TBool CanCreateSourceBuffer();
    virtual CMMFBuffer* CreateSourceBufferL(TMediaId aMediaId, TBool &aReference);
    virtual void ConstructSourceL(  const TDesC8& aInitData );
private:

};

#endif
