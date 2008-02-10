/*
 *  Copyright (c) 2003 P.Wolff
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
#ifndef OGGCONTAINER_H
#define OGGCONTAINER_H

#if defined(SERIES60)
#include <aknsettingitemlist.h> 
#include <aknpopupsettingpage.h> 
#include "OggPlay.h"

class CGainSettingItem;
class CBufferingModeSettingItem;
class COggSettingItemList : public CAknSettingItemList
	{
public:
	COggSettingItemList(COggPlayAppUi& aAppUi);
	~COggSettingItemList();

	void BufferingModeChangedL();
	void VolumeGainChangedL();

protected:
	// From CAknSettingItemList
	CAknSettingItem* CreateSettingItemL(TInt aSettingId);
  
private:
	TOggplaySettings& iData;
	COggPlayAppUi& iAppUi;

	CBufferingModeSettingItem* iBufferingModeItem;
	CGainSettingItem* iGainSettingItem;
	};

class CRepeatSettingItem : public CAknEnumeratedTextPopupSettingItem
	{
public:
	CRepeatSettingItem(TInt aIdentifier,  COggPlayAppUi& aAppUi);

private:
	// Functions from base classes
	void EditItemL(TBool aCalledFromMenu);

private:
	COggPlayAppUi& iAppUi;
	};

class CRandomSettingItem : public CAknBinaryPopupSettingItem
	{
public:
	CRandomSettingItem(TInt aIdentifier,  COggPlayAppUi& aAppUi);

private:
	// Functions from base classes
	void EditItemL(TBool aCalledFromMenu);

private:
	COggPlayAppUi& iAppUi;
	};

class CBufferingModeSettingItem : public CAknEnumeratedTextPopupSettingItem
	{
public:
	CBufferingModeSettingItem(TInt aIdentifier,  COggPlayAppUi& aAppUi);

private:
	// Functions from base classes
	void EditItemL(TBool aCalledFromMenu);

private:
	COggPlayAppUi& iAppUi;
	};

class CThreadPrioritySettingItem : public CAknBinaryPopupSettingItem
	{
public:
	CThreadPrioritySettingItem(TInt aIdentifier,  COggPlayAppUi& aAppUi);

private:
	// Functions from base classes
	void EditItemL(TBool aCalledFromMenu);

private:
	COggPlayAppUi& iAppUi;
	};

class CGainSettingItem : public CAknEnumeratedTextPopupSettingItem 
	{
public:
	CGainSettingItem(TInt aIdentifier,  COggPlayAppUi& aAppUi);

private:
	// Functions from base classes
	void EditItemL(TBool aCalledFromMenu);

private:
	COggPlayAppUi& iAppUi;
	};

class CMp3DitheringSettingItem : public CAknBinaryPopupSettingItem
	{
public:
	CMp3DitheringSettingItem(TInt aIdentifier,  COggPlayAppUi& aAppUi);

private:
	// Functions from base classes
	void EditItemL(TBool aCalledFromMenu);

private:
	COggPlayAppUi& iAppUi;
	};

class CAlarmSettingItem : public CAknBinaryPopupSettingItem
	{
public:
	CAlarmSettingItem(TInt aIdentifier,  COggPlayAppUi& aAppUi);

private:
	// Functions from base classes
	void EditItemL(TBool aCalledFromMenu);

private:
	COggPlayAppUi& iAppUi;
	};

class CAlarmTimeSettingItem : public CAknTimeOrDateSettingItem
	{
public:
	CAlarmTimeSettingItem(TInt aIdentifier, COggPlayAppUi& aAppUi);

private:
	// Functions from base classes
	void EditItemL(TBool aCalledFromMenu);

private:
	COggPlayAppUi& iAppUi;
	};
#endif // SERIES60
#endif
