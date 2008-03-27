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

#include <OggOs.h>
#include <e32std.h>
#include "OggAbsPlayback.h"


TPluginImplementationInformation::TPluginImplementationInformation(const TDesC& aDisplayName, const TDesC& aSupplier, TUid aUid, TInt aVersion)
: iDisplayName(aDisplayName), iSupplier(aSupplier), iUid(aUid), iVersion(aVersion)
	{
	}


CPluginInfo::CPluginInfo()
	{
	}

CPluginInfo* CPluginInfo::NewL(const TDesC& aExtension, const TPluginImplementationInformation &aFormatInfo, TUid aControllerUid)
	{
	CPluginInfo* self = new(ELeave) CPluginInfo;
	CleanupStack::PushL(self);
	self->ConstructL(aExtension, aFormatInfo, aControllerUid);

	CleanupStack::Pop(self);
	return self;
	}

void CPluginInfo::ConstructL(const TDesC& aExtension, const TPluginImplementationInformation &aFormatInfo, TUid aControllerUid) 
	{
    iExtension = aExtension.AllocL();

    iName = aFormatInfo.iDisplayName.AllocL();
    iSupplier = aFormatInfo.iSupplier.AllocL();
    iVersion = aFormatInfo.iVersion;

	iFormatUid = aFormatInfo.iUid;
    iControllerUid = aControllerUid;
	}

CPluginInfo::~CPluginInfo()
	{
    delete iExtension;

	delete iName;
    delete iSupplier;
	}


CPluginSupportedList::CPluginSupportedList()
	{
	}

void CPluginSupportedList::ConstructL()
	{
	iPlugins = new(ELeave) CArrayPtrFlat<TExtensionList>(3);
	}

CPluginSupportedList::~CPluginSupportedList()
	{
	for (TInt i = 0 ; i<iPlugins->Count() ; i++)
		{
		iPlugins->At(i)->listPluginInfos->ResetAndDestroy();
		delete(iPlugins->At(i)->listPluginInfos);
		}

	iPlugins->ResetAndDestroy();
	delete iPlugins;
	}

void CPluginSupportedList::AddPluginL(const TDesC &aExtension, const TPluginImplementationInformation &aFormatInfo, TUid aControllerUid)
	{
	CPluginInfo* info = CPluginInfo::NewL(aExtension, aFormatInfo, aControllerUid);
	CleanupStack::PushL(info);
    
	AddExtension(aExtension);
	TInt index = FindListL(aExtension);

	// Add the plugin to the existing list
	TExtensionList* list = iPlugins->At(index);
	list->listPluginInfos->AppendL(info);
    
	CleanupStack::Pop(info);
	}

void CPluginSupportedList::SelectPluginL(const TDesC& aExtension, TUid aSelectedUid)
	{ 
	TInt index = FindListL(aExtension);
	if (index<0) 
		User::Leave(KErrNotFound);
       
	TExtensionList* list = iPlugins->At(index);   
    if (aSelectedUid == TUid::Null())
		{
		list->selectedPlugin = NULL;
		return;
		}
    
	TBool found = EFalse;
	for (TInt i = 0 ; i<list->listPluginInfos->Count() ; i++)
		{
		if (list->listPluginInfos->At(i)->iControllerUid == aSelectedUid)
			{
			list->selectedPlugin = list->listPluginInfos->At(i);
			found = ETrue;
			}
		}

	if (!found)
		User::Leave(KErrNotFound);
	}

CDesCArrayFlat* CPluginSupportedList::SupportedExtensions() const
	{
	CDesCArrayFlat * extensions = NULL;
	TRAPD(err,
		{
		extensions = new(ELeave) CDesCArrayFlat(3);
		CleanupStack::PushL(extensions);
		for (TInt i=0; i<iPlugins->Count(); i++)
			extensions->AppendL(iPlugins->At(i)->extension);

		CleanupStack::Pop();
		});

	return extensions;
	}

TInt CPluginSupportedList::FindListL(const TDesC &aExtension) const
	{
    for (TInt i = 0 ; i<iPlugins->Count() ; i++)
		{
        if (aExtension.CompareF(iPlugins->At(i)->extension) == 0)
            return i;
		}

    return KErrNotFound;
	}

const CPluginInfo* CPluginSupportedList::GetSelectedPluginInfo(const TDesC &aExtension) const
	{
	TInt index = FindListL(aExtension);
	if(index<0) 
		return NULL;

	TExtensionList* list = iPlugins->At(index);
	if (!list)
		return NULL;

	return list->selectedPlugin;
	}

const CArrayPtrFlat<CPluginInfo>* CPluginSupportedList::GetPluginInfoList(const TDesC &anExtension) const
	{
	TInt index = FindListL(anExtension);
	if(index<0) 
		return NULL;

	TExtensionList* list = iPlugins->At(index);
	if (!list)
		return NULL;

	return list->listPluginInfos;
	}

void CPluginSupportedList::AddExtension(const TDesC &anExtension)
	{
	TInt index = FindListL(anExtension);
    if(index>=0) 
      return; // The extension already exists
    
    // List not found, create a new list
    TExtensionList* list = new(ELeave) TExtensionList;	
    CleanupStack::PushL(list);
    list->listPluginInfos = new(ELeave) CArrayPtrFlat <CPluginInfo>(1);
    list->extension = anExtension;
    list->selectedPlugin = NULL;

    iPlugins->AppendL(list);
    CleanupStack::Pop(list);
	}

CAbsPlayback::CAbsPlayback(CPluginSupportedList& aPluginSupportedList, MPlaybackObserver* aObserver)
: iState(CAbsPlayback::EClosed), iPluginSupportedList(aPluginSupportedList), iObserver(aObserver)
	{
	}

CAbsPlayback::TState CAbsPlayback::State()
	{
	return iState;
	}
   
const TOggFileInfo& CAbsPlayback::DecoderFileInfo()
	{
	return iFileInfo;
	}

TInt CAbsPlayback::BitRate()
	{
	return iFileInfo.iBitRate;
	}

TInt CAbsPlayback::Channels()
	{
	return iFileInfo.iChannels;
	}

TInt CAbsPlayback::FileSize()
	{
	return iFileInfo.iFileSize;
	}

TInt CAbsPlayback::Rate()
	{
	return iFileInfo.iRate;
	}

TInt64 CAbsPlayback::Time()
	{
	return iFileInfo.iTime;
	}

const TDesC& CAbsPlayback::Album()
	{
	return iFileInfo.iAlbum;
	}

const TDesC& CAbsPlayback::Artist()
	{
	return iFileInfo.iArtist;
	}

const TFileName& CAbsPlayback::FileName()
	{
	return iFileInfo.iFileName;
	}

const TDesC& CAbsPlayback::Genre()
	{
	return iFileInfo.iGenre;
	}

const TDesC& CAbsPlayback::Title()
	{
	return iFileInfo.iTitle;
	}

const TDesC& CAbsPlayback::TrackNumber()
	{
	return iFileInfo.iTrackNumber;
	}

void CAbsPlayback::ClearComments()
	{
	iFileInfo.iArtist.SetLength(0);
	iFileInfo.iTitle.SetLength(0);
	iFileInfo.iAlbum.SetLength(0);
	iFileInfo.iGenre.SetLength(0);
	iFileInfo.iTrackNumber.SetLength(0);
	iFileInfo.iFileName.SetLength(0);
	}

void CAbsPlayback::SetVolumeGain(TGainType /*aGain*/)
	{
	/* No gain by default */
	}
