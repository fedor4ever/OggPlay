/*
 *  Copyright (c) 2004 OggPlay Team
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


#ifndef OGGPLAYCONTROLLERCUSTOMCOMMANDS_H
#define OGGPLAYCONTROLLERCUSTOMCOMMANDS_H

enum TOggPlayControllerCustomCommands
    {   
    EOggPlayControllerStreamWait = 20503,       // Random number
                                                // Hopefully only OggPlay compatible
                                                // plugins will support a custom
                                                // command with this ID.


	EOggPlayControllerCCGetAudioProperties = 20504, // Random number + 1
	EOggPlayControllerCCGetFrequencies = 20505,     // Random number + 2
	EOggPlayControllerCCSetVolumeGain = 20506       // Random number + 3
    };


class TMMFGetFreqsParams
	{
public:
	const TInt32 *iFreqBins ; //Pointer to the frequency bins to be updated
	} ;

typedef TPckgBuf<TMMFGetFreqsParams>  TMMFGetFreqsConfig ;

class TMMFSetVolumeGainParams
	{
public:
	TInt iGain; // Volume gain to set
	} ;

typedef TPckgBuf<TMMFSetVolumeGainParams>  TMMFSetVolumeGainConfig ;

#endif      // OGGPLAYCONTROLLERCUSTOMCOMMANDS_H
            
// End of File
