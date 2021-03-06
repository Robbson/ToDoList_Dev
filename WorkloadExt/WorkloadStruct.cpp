// WorkloadStruct.cpp: implementation of the CWorkloadStruct class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "WorkloadStruct.h"

#include "..\shared\graphicsMisc.h"
#include "..\shared\misc.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////

CMapDayAllocations::CMapDayAllocations() 
{
}

CMapDayAllocations::~CMapDayAllocations() 
{
}

void CMapDayAllocations::Copy(const CMapDayAllocations& other)
{
	Misc::CopyStrT<WORKLOADALLOCATION>(other, *this);
}

BOOL CMapDayAllocations::MatchAll(const CMapDayAllocations& other) const
{
	return Misc::MatchAllStrT<WORKLOADALLOCATION>(*this, other);
}

void CMapDayAllocations::RemoveAll()
{
	CMap<CString, LPCTSTR, WORKLOADALLOCATION, WORKLOADALLOCATION&>::RemoveAll();
}

BOOL CMapDayAllocations::Get(const CString& sAllocTo, WORKLOADALLOCATION& wa) const
{
	wa.Reset();

	if (sAllocTo == ALLOCTO_TOTALID)
	{
		wa.dDays = GetTotalDays();
		wa.bOverlapping = FALSE;

		return TRUE;
	}

	// else
	return Lookup(Misc::ToUpper(sAllocTo), wa);
}

double CMapDayAllocations::GetDays(const CString& sAllocTo) const
{
	WORKLOADALLOCATION wa;
	
	Lookup(Misc::ToUpper(sAllocTo), wa);

	return wa.dDays;
}

BOOL CMapDayAllocations::SetDays(const CString& sAllocTo, double dValue)
{
	if (dValue < 0)
	{
		ASSERT(0);
		return FALSE;
	}

	if (dValue == 0.0)
	{
		RemoveKey(Misc::ToUpper(sAllocTo));
	}
	else
	{
		WORKLOADALLOCATION wa;
		wa.dDays = dValue;

		SetAt(Misc::ToUpper(sAllocTo), wa);
	}

	return TRUE;
}

CString CMapDayAllocations::GetDays(const CString& sAllocTo, int nDecimals) const
{
	double dValue = GetDays(sAllocTo);

	return Format(dValue, nDecimals);
}

BOOL CMapDayAllocations::SetDays(const CString& sAllocTo, const CString& sDays)
{
	return SetDays(sAllocTo, Misc::Atof(sDays));
}

BOOL CMapDayAllocations::AppendOverlaps(const CMapDayAllocations& mapOther)
{
	BOOL bAppended = FALSE;

	if (GetCount())
	{
		CString sAllocTo;
		WORKLOADALLOCATION wa;

		POSITION pos = GetStartPosition();

		while (pos)
		{
			GetNextAssoc(pos, sAllocTo, wa);

			if (!wa.bOverlapping && (mapOther.GetDays(sAllocTo) > 0.0))
			{
				wa.bOverlapping = TRUE;
				SetAt(sAllocTo, wa);

				bAppended = TRUE;
			}
		}
	}

	return bAppended;
}

BOOL CMapDayAllocations::IsOverlapping(const CString& sAllocTo) const
{
	WORKLOADALLOCATION wa;

	return (Lookup(Misc::ToUpper(sAllocTo), wa) && wa.bOverlapping);
}

void CMapDayAllocations::ClearOverlaps()
{
	if (GetCount())
	{
		CString sAllocTo;
		WORKLOADALLOCATION wa;

		POSITION pos = GetStartPosition();

		while (pos)
		{
			GetNextAssoc(pos, sAllocTo, wa);

			if (wa.bOverlapping)
			{
				wa.bOverlapping = FALSE;
				SetAt(sAllocTo, wa);
			}
		}
	}
}

void CMapDayAllocations::Decode(const CString& sAllocations)
{
	RemoveAll();

	CStringArray aAllocations;
	int nAllocTo = Misc::Split(sAllocations, aAllocations, '\n');

	while (nAllocTo--)
	{
		CString sDays, sAllocTo = aAllocations[nAllocTo];
		Misc::Split(sAllocTo, sDays, ':');

		SetDays(sAllocTo, sDays);
	}
}

CString CMapDayAllocations::Encode() const
{
	CString sAllocations, sAllocTo;
	CStringArray aAllocations;
	WORKLOADALLOCATION wa;
	POSITION pos = GetStartPosition();

	while (pos)
	{
		GetNextAssoc(pos, sAllocTo, wa);
		ASSERT(!sAllocTo.IsEmpty());

		if (!sAllocTo.IsEmpty())
		{
			CString sDays;

			if (wa.dDays > 0)
				aAllocations.Add(sAllocTo + ':' + Misc::Format(wa.dDays));
		}
	}

	return Misc::FormatArray(aAllocations, '\n');
}

double CMapDayAllocations::GetTotalDays() const
{
	CString sAllocTo;
	CStringArray aAllocations;
	double dTotalDays = 0;
	WORKLOADALLOCATION wa;
	POSITION pos = GetStartPosition();

	while (pos)
	{
		GetNextAssoc(pos, sAllocTo, wa);
		ASSERT(!sAllocTo.IsEmpty());

		if (!sAllocTo.IsEmpty())
			dTotalDays += wa.dDays;
	}

	return dTotalDays;
}

CString CMapDayAllocations::GetTotalDays(int nDecimals) const
{
	return Format(GetTotalDays(), nDecimals);
}

CString CMapDayAllocations::Format(double dValue, int nDecimals)
{
	return ((dValue == 0.0) ? _T("") : Misc::Format(dValue, nDecimals));
}

//////////////////////////////////////////////////////////////////////

CMapAllocationTotals::CMapAllocationTotals(BOOL bReturnAverageForTotal) 
	:
m_bReturnAverageForTotal(bReturnAverageForTotal)
{
}

CMapAllocationTotals::~CMapAllocationTotals() 
{
}

void CMapAllocationTotals::RemoveAll()
{
	CMap<CString, LPCTSTR, double, double&>::RemoveAll();
}

double CMapAllocationTotals::Get(const CString& sAllocTo) const
{
	if (sAllocTo == ALLOCTO_TOTALID)
		return GetTotal();

	// else
	double dDays = 0.0;
	Lookup(Misc::ToUpper(sAllocTo), dDays);

	return dDays;
}

BOOL CMapAllocationTotals::Set(const CString& sAllocTo, double dValue)
{
	if (dValue < 0)
	{
		ASSERT(0);
		return FALSE;
	}

	if (dValue == 0.0)
		RemoveKey(Misc::ToUpper(sAllocTo));
	else
		SetAt(Misc::ToUpper(sAllocTo), dValue);

	return TRUE;
}

CString CMapAllocationTotals::Get(const CString& sAllocTo, int nDecimals) const
{
	double dValue = Get(sAllocTo);

	return Format(dValue, nDecimals);
}

BOOL CMapAllocationTotals::Add(const CString& sAllocTo, double dValue)
{
	if (dValue <= 0)
		return FALSE;

	return Set(sAllocTo, (Get(sAllocTo) + dValue));
}

void CMapAllocationTotals::Increment(const CString& sAllocTo)
{
	Add(sAllocTo, 1.0);
}

double CMapAllocationTotals::GetTotal() const
{
	CString sAllocTo;
	CStringArray aAllocations;
	double dTotalDays = 0, dValue;
	POSITION pos = GetStartPosition();

	while (pos)
	{
		GetNextAssoc(pos, sAllocTo, dValue);
		ASSERT(!sAllocTo.IsEmpty());

		if (!sAllocTo.IsEmpty())
			dTotalDays += dValue;
	}

	if (m_bReturnAverageForTotal && GetCount())
		dTotalDays /= GetCount();

	return dTotalDays;
}

CString CMapAllocationTotals::GetTotal(int nDecimals) const
{
	return Format(GetTotal(), nDecimals);
}

CString CMapAllocationTotals::Format(double dValue, int nDecimals)
{
	return ((dValue == 0.0) ? _T("") : Misc::Format(dValue, nDecimals));
}

//////////////////////////////////////////////////////////////////////

WORKLOADITEM::WORKLOADITEM(DWORD dwID, LPCTSTR szTitle) 
	: 
	sTitle(szTitle),
	color(CLR_NONE), 
	bParent(FALSE), 
	dwTaskID(dwID), 
	dwRefID(0), 
	dwOrgRefID(0), 
	bGoodAsDone(FALSE),
	bDone(FALSE),
	nPosition(-1),
	bLocked(FALSE),
	bSomeSubtaskDone(FALSE)
{
}

WORKLOADITEM::WORKLOADITEM(const WORKLOADITEM& wi)
{
	*this = wi;
}

WORKLOADITEM& WORKLOADITEM::operator=(const WORKLOADITEM& wi)
{
	sTitle = wi.sTitle;
	dtRange = wi.dtRange;
	bDone = wi.bDone;
	color = wi.color;
	bParent = wi.bParent;
	dwTaskID = wi.dwTaskID;
	dwRefID = wi.dwRefID;
	nPercent = wi.nPercent;
	bGoodAsDone = wi.bGoodAsDone;
	bDone = wi.bDone;
	nPosition = wi.nPosition;
	bLocked = wi.bLocked;
	bHasIcon = wi.bHasIcon;
	bSomeSubtaskDone = wi.bSomeSubtaskDone;
	
	aAllocTo.Copy(wi.aAllocTo);
	mapAllocatedDays.Copy(wi.mapAllocatedDays);
	
	return (*this);
}

BOOL WORKLOADITEM::operator==(const WORKLOADITEM& wi) const
{
	return ((sTitle == wi.sTitle) &&
			(dtRange == wi.dtRange) &&
			(bDone == wi.bDone) &&
			(color == wi.color) &&
			(bParent == wi.bParent) &&
			(dwTaskID == wi.dwTaskID) &&
			(dwRefID == wi.dwRefID) &&
			(nPercent == wi.nPercent) &&	
			(nPosition == wi.nPosition) &&
			(bGoodAsDone == wi.bGoodAsDone) &&
			(bDone == wi.bDone) &&
			(bLocked == wi.bLocked) &&
			(bHasIcon == wi.bHasIcon) &&
			(bSomeSubtaskDone == wi.bSomeSubtaskDone) &&
			Misc::MatchAll(aAllocTo, wi.aAllocTo) &&
			mapAllocatedDays.MatchAll(wi.mapAllocatedDays));
}

WORKLOADITEM::~WORKLOADITEM()
{
	
}

BOOL WORKLOADITEM::HasStartDate() const
{
	return dtRange.HasStart();
}

BOOL WORKLOADITEM::HasDueDate() const
{
	return dtRange.HasEnd();
}

COLORREF WORKLOADITEM::GetTextColor(BOOL bSelected, BOOL bColorIsBkgnd) const
{
	if (HasColor())
	{
		if (bColorIsBkgnd && !bSelected && !bDone && !bGoodAsDone)
			return GraphicsMisc::GetBestTextColor(color);
		else
			return color;
	}
	
	// else
	return GetSysColor(COLOR_WINDOWTEXT);
}

COLORREF WORKLOADITEM::GetTextBkColor(BOOL bSelected, BOOL bColorIsBkgnd) const
{
	if (!bSelected && HasColor())
	{
		if (bColorIsBkgnd && !bDone && !bGoodAsDone)
			return color;
	}
	
	// else
	return CLR_NONE;
}

BOOL WORKLOADITEM::HasColor() const
{
	return ((color != CLR_NONE) && (color != GetSysColor(COLOR_WINDOWTEXT)));
}

//////////////////////////////////////////////////////////////////////

CWorkloadItemMap::~CWorkloadItemMap()
{
	RemoveAll();
}

void CWorkloadItemMap::RemoveAll()
{
	DWORD dwTaskID = 0;
	WORKLOADITEM* pWI = NULL;
	
	POSITION pos = GetStartPosition();
	
	while (pos)
	{
		GetNextAssoc(pos, dwTaskID, pWI);
		ASSERT(pWI);
		
		delete pWI;
	}
	
	CMap<DWORD, DWORD, WORKLOADITEM*, WORKLOADITEM*&>::RemoveAll();
}

void CWorkloadItemMap::CalculateTotals(const COleDateTimeRange& dtPeriod,
										CMapAllocationTotals& mapTotalDays, 
										CMapAllocationTotals& mapTotalTasks) const
{
	mapTotalDays.RemoveAll();
	mapTotalTasks.RemoveAll();

	DWORD dwTaskID = 0;
	WORKLOADITEM* pWI = NULL;

	POSITION pos = GetStartPosition();

	while (pos)
	{
		GetNextAssoc(pos, dwTaskID, pWI);
		ASSERT(pWI);

		// Weed out unwanted tasks
		if (pWI->bParent)
			continue;

		if (!pWI->HasValidDates() || pWI->IsDone())
			continue;

		// Determine how many the days of the task fall within the specified period
		COleDateTimeRange dtIntersect;
		
		if (!dtIntersect.GetIntersection(dtPeriod, pWI->dtRange))
			continue;

		double dTaskDuration = pWI->dtRange.GetWeekdayCount();
		
		if (dTaskDuration == 0.0)
			continue;


		double dTaskDays = dtIntersect.GetWeekdayCount();
		double dProportion = (dTaskDays / dTaskDuration);

		for (int nAllocTo = 0; nAllocTo < pWI->aAllocTo.GetSize(); nAllocTo++)
		{
			CString sAllocTo(pWI->aAllocTo[nAllocTo]);
			double dDays = (pWI->mapAllocatedDays.GetDays(sAllocTo) * dProportion);

			if (mapTotalDays.Add(sAllocTo, dDays))
				mapTotalTasks.Increment(sAllocTo);
		}
	}
}

BOOL CWorkloadItemMap::ItemIsLocked(DWORD dwTaskID) const
{
	const WORKLOADITEM* pWI = GetItem(dwTaskID);
	
	return (pWI && pWI->bLocked);
}

BOOL CWorkloadItemMap::RemoveKey(DWORD dwKey)
{
	WORKLOADITEM* pWI = NULL;
	
	if (Lookup(dwKey, pWI))
	{
		delete pWI;
		return CMap<DWORD, DWORD, WORKLOADITEM*, WORKLOADITEM*&>::RemoveKey(dwKey);
	}
	
	// else
	return FALSE;
}

BOOL CWorkloadItemMap::HasItem(DWORD dwKey) const
{
	if (dwKey == 0)
		return FALSE;

	return (GetItem(dwKey) != NULL);
}

WORKLOADITEM* CWorkloadItemMap::GetItem(DWORD dwKey) const
{
	if (dwKey == 0)
		return NULL;

	WORKLOADITEM* pWI = NULL;
	
	if (Lookup(dwKey, pWI))
		ASSERT(pWI);
	
	return pWI;
}

void CWorkloadItemMap::RecalculateOverlaps()
{
	CArray<WORKLOADITEM*, WORKLOADITEM*&> aItems;
	int nNumItems = BuildDateSortedList(aItems);

	DWORD dwTaskID = 0;
	WORKLOADITEM* pWI = NULL;

	POSITION pos = GetStartPosition();

	while (pos)
	{
		GetNextAssoc(pos, dwTaskID, pWI);
		ASSERT(pWI);

		pWI->mapAllocatedDays.ClearOverlaps(); // reset

		if (pWI->HasValidDates())
		{
			for (int nItem = 0; nItem < nNumItems; nItem++)
			{
				const WORKLOADITEM* pWIOther = aItems[nItem];
				ASSERT(pWIOther->HasValidDates());

				if (pWIOther != pWI)
				{
					// Stop when we reach the first item which has a 
					// start date later than this task's due date
					if (pWIOther->dtRange.m_dtStart > pWI->dtRange.m_dtEnd)
						break;

					if (pWI->dtRange.IntersectsWith(pWIOther->dtRange))
						pWI->mapAllocatedDays.AppendOverlaps(pWIOther->mapAllocatedDays);
				}				
			}
		}
	}
}

int CWorkloadItemMap::BuildDateSortedList(CArray<WORKLOADITEM*, WORKLOADITEM*&>& aItems) const
{
	aItems.RemoveAll();

	DWORD dwTaskID = 0;
	WORKLOADITEM* pWI = NULL;

	POSITION pos = GetStartPosition();

	while (pos)
	{
		GetNextAssoc(pos, dwTaskID, pWI);
		ASSERT(pWI);

		if (pWI->HasValidDates())
			aItems.Add(pWI);
	}

	if (aItems.GetSize() > 1)
		qsort(aItems.GetData(), aItems.GetSize(), sizeof(WORKLOADITEM*), CompareItems);

	return aItems.GetSize();
}

int CWorkloadItemMap::CompareItems(const void* pV1, const void* pV2)
{
	typedef WORKLOADITEM* LPWORKLOADITEM;

	const WORKLOADITEM* pWI1 = *(static_cast<const LPWORKLOADITEM*>(pV1));
	const WORKLOADITEM* pWI2 = *(static_cast<const LPWORKLOADITEM*>(pV2));

	ASSERT(pWI1 && pWI2 && (pWI1 != pWI2));
	ASSERT(pWI1->HasValidDates() && pWI2->HasValidDates());

	// Sort by start date
	if (pWI1->dtRange.m_dtStart < pWI2->dtRange.m_dtStart)
		return -1;

	if (pWI1->dtRange.m_dtStart > pWI2->dtRange.m_dtStart)
		return 1;

	// Then by due date
	if (pWI1->dtRange.m_dtEnd < pWI2->dtRange.m_dtEnd)
		return -1;

	if (pWI1->dtRange.m_dtEnd > pWI2->dtRange.m_dtEnd)
		return 1;

	return 0;
}

//////////////////////////////////////////////////////////////////////

WORKLOADSORTCOLUMN::WORKLOADSORTCOLUMN() : nBy(WLCC_NONE), bAscending(-1)
{

}

BOOL WORKLOADSORTCOLUMN::Matches(WLC_COLUMNID nSortBy, BOOL bSortAscending) const
{
	return ((nBy == nSortBy) && (bAscending == bSortAscending));
}

BOOL WORKLOADSORTCOLUMN::operator==(const WORKLOADSORTCOLUMN& col) const
{
	return Matches(col.nBy, col.bAscending);
}

BOOL WORKLOADSORTCOLUMN::Sort(WLC_COLUMNID nSortBy, BOOL bAllowToggle, BOOL bSortAscending)
{
	if (!bAllowToggle && Matches(nSortBy, bSortAscending))
		return FALSE;

	WLC_COLUMNID nOldSort = nBy;
	nBy = nSortBy;

	if (nSortBy != WLCC_NONE)
	{
		// if it's the first time or we are changing columns 
		// we always reset the direction
		if ((bAscending == -1) || (nSortBy != nOldSort))
		{
			if (bSortAscending != -1)
			{
				bAscending = bSortAscending;
			}
			else
			{
				bAscending = 1;
			}
		}
		else if (bAllowToggle)
		{
			ASSERT(bAscending != -1);
			bAscending = !bAscending;
		}
		else
		{
			ASSERT(bSortAscending != -1);
			bAscending = bSortAscending;
		}
	}
	else
	{
		// Always ascending for 'unsorted' to match app
		bAscending = 1;
	}

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////

WORKLOADSORTCOLUMNS::WORKLOADSORTCOLUMNS()
{

}

BOOL WORKLOADSORTCOLUMNS::Sort(const WORKLOADSORTCOLUMNS& sort)
{
	if (*this == sort)
		return FALSE;

	for (int nCol = 0; nCol < 3; nCol++)
		cols[nCol] = sort.cols[nCol];

	return TRUE;
}

BOOL WORKLOADSORTCOLUMNS::operator==(const WORKLOADSORTCOLUMNS& sort) const
{
	for (int nCol = 0; nCol < 3; nCol++)
	{
		if (!(cols[nCol] == sort.cols[nCol]))
			return FALSE;
	}

	// else
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////

WORKLOADSORT::WORKLOADSORT() : bMultiSort(FALSE)
{

}

BOOL WORKLOADSORT::IsSorting() const
{
	if (!bMultiSort)
		return (single.nBy != WLCC_NONE);

	// else
	return (multi.cols[0].nBy != WLCC_NONE);
}

BOOL WORKLOADSORT::IsSortingBy(WLC_COLUMNID nColID) const
{
	if (!bMultiSort)
		return IsSingleSortingBy(nColID);

	return IsMultiSortingBy(nColID);
}

BOOL WORKLOADSORT::IsSingleSortingBy(WLC_COLUMNID nColID) const
{
	return (!bMultiSort && (single.nBy == nColID));
}

BOOL WORKLOADSORT::IsMultiSortingBy(WLC_COLUMNID nColID) const
{
	if (bMultiSort)
	{
		for (int nCol = 0; nCol < 3; nCol++)
		{
			if (multi.cols[nCol].nBy == nColID)
				return TRUE;
		}
	}

	return FALSE;
}

BOOL WORKLOADSORT::Sort(WLC_COLUMNID nBy, BOOL bAllowToggle, BOOL bAscending)
{
	if (bMultiSort)
	{
		bMultiSort = FALSE;
		return single.Sort(nBy, FALSE, bAscending);
	}

	return single.Sort(nBy, bAllowToggle, bAscending);
}

BOOL WORKLOADSORT::Sort(const WORKLOADSORTCOLUMNS& sort)
{
	bMultiSort = TRUE;
	return multi.Sort(sort);
}

/////////////////////////////////////////////////////////////////////////////

