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
    EOggPlayControllerCCGetFrequencies = 20503 // Random number
                                               // Hopefully only OggPlay compatible
                                               // plugins will support a custom
                                               // command with this ID.
    };


class TMMFGetFreqsParams
	{
public:
	TTime iTime ; // Time of the desired frequency bins
	const TInt32 *iFreqBins ; //Pointer to the frequency bins
	                          //to be updated.
	} ;

typedef TPckgBuf<TMMFGetFreqsParams>  TMMFGetFreqsConfig ;

#endif      // OGGPLAYCONTROLLERCUSTOMCOMMANDS_H
            
// End of File
