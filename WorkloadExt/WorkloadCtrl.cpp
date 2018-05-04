// WorkloadTreeList.cpp: implementation of the CWorkloadTreeList class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "WorkloadCtrl.h"
#include "WorkloadMsg.h"
#include "WorkloadStatic.h"

#include "..\shared\DialogHelper.h"
#include "..\shared\DateHelper.h"
#include "..\shared\timeHelper.h"
#include "..\shared\holdredraw.h"
#include "..\shared\graphicsMisc.h"
#include "..\shared\TreeCtrlHelper.h"
#include "..\shared\autoflag.h"
#include "..\shared\misc.h"
#include "..\shared\enstring.h"
#include "..\shared\localizer.h"
#include "..\shared\themed.h"
#include "..\shared\osversion.h"
#include "..\shared\enbitmap.h"
#include "..\shared\copywndcontents.h"

#include "..\Interfaces\iuiextension.h"

#include "..\3rdparty\shellicons.h"

#include <float.h> // for DBL_MAX
#include <math.h>  // for fabs()

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////

#ifndef GET_WHEEL_DELTA_WPARAM
#	define GET_WHEEL_DELTA_WPARAM(wParam)  ((short)HIWORD(wParam))
#endif 

#ifndef CDRF_SKIPPOSTPAINT
#	define CDRF_SKIPPOSTPAINT	(0x00000100)
#endif

//////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
const int MAX_YEAR = 2100;
#else
const int MAX_YEAR = 2200;
#endif

const int MIN_COL_WIDTH			= GraphicsMisc::ScaleByDPIFactor(6);
const int MIN_LABEL_EDIT_WIDTH	= GraphicsMisc::ScaleByDPIFactor(200);
const int DEF_MONTH_WIDTH		= GraphicsMisc::ScaleByDPIFactor(72);
const int TREE_TITLE_MIN_WIDTH	= GraphicsMisc::ScaleByDPIFactor(75); 
const int COLUMN_PADDING		= GraphicsMisc::ScaleByDPIFactor(15);
const int MIN_MONTH_WIDTH		= GraphicsMisc::ScaleByDPIFactor(2);
const int MINS_IN_HOUR			= 60;
const int MINS_IN_DAY			= (MINS_IN_HOUR * 24);
const int LV_COLPADDING			= GraphicsMisc::ScaleByDPIFactor(3);
const int TV_TIPPADDING			= GraphicsMisc::ScaleByDPIFactor(3);
const int HD_COLPADDING			= GraphicsMisc::ScaleByDPIFactor(6);
const int MAX_HEADER_WIDTH		= 32000; // (SHRT_MAX - tolerance)
const int DRAG_BUFFER			= GraphicsMisc::ScaleByDPIFactor(50);
const int DONE_BOX				= GraphicsMisc::ScaleByDPIFactor(6);
const int IMAGE_SIZE			= GraphicsMisc::ScaleByDPIFactor(16);

const LONG DEPENDPICKPOS_NONE = 0xFFFFFFFF;
const double DAY_WEEK_MULTIPLIER = 1.5;
const double HOUR_DAY_MULTIPLIER = 6;
const double MULTIYEAR_MULTIPLIER = 2.0;
const double DAYS_IN_YEAR = 365.25;
const double DAYS_IN_MONTH = (DAYS_IN_YEAR / 12);

//////////////////////////////////////////////////////////////////////

#define GET_GI_RET(id, gi, ret)	\
{								\
	if (id == 0) return ret;	\
	gi = GetWorkloadItem(id);		\
	ASSERT(gi);					\
	if (gi == NULL) return ret;	\
}

#define GET_GI(id, gi)		\
{							\
	if (id == 0) return;	\
	gi = GetWorkloadItem(id);	\
	ASSERT(gi);				\
	if (gi == NULL)	return;	\
}

//////////////////////////////////////////////////////////////////////

#define FROMISABOVE(t) ((t == GCDDT_FROMISABOVELEFT) || (t == GCDDT_FROMISABOVERIGHT))

//////////////////////////////////////////////////////////////////////

class CWorkloadLockUpdates : public CLockUpdates
{
public:
	CWorkloadLockUpdates(CWorkloadCtrl* pCtrl, BOOL bTree, BOOL bAndSync) 
		: 
	CLockUpdates(bTree ? pCtrl->m_tcTasks.GetSafeHwnd() : pCtrl->m_lcColumns.GetSafeHwnd()),
		m_bAndSync(bAndSync), 
		m_pCtrl(pCtrl)
	{
		ASSERT(m_pCtrl);
		
		if (m_bAndSync)
			m_pCtrl->EnableResync(FALSE);
	}
	
	~CWorkloadLockUpdates()
	{
		ASSERT(m_pCtrl);
		
		if (m_bAndSync)
			m_pCtrl->EnableResync(TRUE, m_hWnd);
	}
	
private:
	BOOL m_bAndSync;
	CWorkloadCtrl* m_pCtrl;
};

enum
{
	IDC_TASKTREE = 100,		
		IDC_TASKTREECOLUMNS,		
		IDC_TASKTREEHEADER,		
};

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CWorkloadCtrl::CWorkloadCtrl() 
	:
	CTreeListSyncer(TLSF_SYNCSELECTION | TLSF_SYNCFOCUS | TLSF_BORDER | TLSF_SYNCDATA | TLSF_SPLITTER),
	m_pTCH(NULL),
	m_nMonthWidth(DEF_MONTH_WIDTH),
	m_nMonthDisplay(WLC_DISPLAY_MONTHS_LONG),
	m_dwOptions(WLCF_SHOWSPLITTERBAR),
	m_crAltLine(CLR_NONE),
	m_crGridLine(CLR_NONE),
	m_crDefault(CLR_NONE),
	m_crParent(CLR_NONE),
	m_crToday(CLR_NONE),
	m_crWeekend(CLR_NONE),
	m_dwMaxTaskID(0),
	m_bReadOnly(FALSE),
	m_bMovingTask(FALSE),
	m_nPrevDropHilitedItem(-1),
	m_tshDragDrop(m_tcTasks),
	m_treeDragDrop(m_tshDragDrop, m_tcTasks)
{

}

CWorkloadCtrl::~CWorkloadCtrl()
{
	//CTreeListSyncer::Release();
}

BEGIN_MESSAGE_MAP(CWorkloadCtrl, CWnd)
	ON_NOTIFY(TVN_BEGINLABELEDIT, IDC_WORKLOADTREE, OnBeginEditTreeLabel)
	ON_NOTIFY(HDN_ENDDRAG, IDC_TASKTREEHEADER, OnEndDragTreeHeader)
	ON_NOTIFY(HDN_ITEMCLICK, IDC_TASKTREEHEADER, OnClickTreeHeader)
	ON_NOTIFY(HDN_ITEMCHANGING, IDC_TASKTREEHEADER, OnItemChangingTreeHeader)
	ON_NOTIFY(HDN_ITEMCHANGED, IDC_TASKTREEHEADER, OnItemChangedTreeHeader)
	ON_NOTIFY(HDN_DIVIDERDBLCLICK, IDC_TASKTREEHEADER, OnDblClickTreeHeaderDivider)
	ON_NOTIFY(NM_RCLICK, IDC_TASKTREEHEADER, OnRightClickTreeHeader)
	ON_NOTIFY(TVN_GETDISPINFO, IDC_TASKTREE, OnTreeGetDispInfo)
	ON_NOTIFY(TVN_ITEMEXPANDED, IDC_TASKTREE, OnTreeItemExpanded)
	ON_NOTIFY(TVN_KEYUP, IDC_WORKLOADTREE, OnTreeKeyUp)
	ON_NOTIFY(NM_CLICK, IDC_WORKLOADLIST, OnColumnsClick)

	ON_REGISTERED_MESSAGE(WM_DD_DRAGENTER, OnTreeDragEnter)
	ON_REGISTERED_MESSAGE(WM_DD_PREDRAGMOVE, OnTreePreDragMove)
	ON_REGISTERED_MESSAGE(WM_DD_DRAGOVER, OnTreeDragOver)
	ON_REGISTERED_MESSAGE(WM_DD_DRAGDROP, OnTreeDragDrop)
	ON_REGISTERED_MESSAGE(WM_DD_DRAGABORT, OnTreeDragAbort)

	ON_WM_CREATE()
	ON_WM_SIZE()
END_MESSAGE_MAP()


BOOL CWorkloadCtrl::Create(CWnd* pParentWnd, const CRect& rect, UINT nID, BOOL bVisible)
{
	DWORD dwStyle = (WS_CHILD | (bVisible ? WS_VISIBLE : 0) | WS_TABSTOP);
	
	// create ourselves
	return CWnd::CreateEx(WS_EX_CONTROLPARENT, NULL, NULL, dwStyle, rect, pParentWnd, nID);
}

int CWorkloadCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	BOOL bVisible = (lpCreateStruct->style & WS_VISIBLE);
	CRect rect(0, 0, lpCreateStruct->cx, lpCreateStruct->cy);
	
	DWORD dwStyle = (WS_CHILD | (bVisible ? WS_VISIBLE : 0));
	
	if (!m_tcTasks.Create((dwStyle | WS_TABSTOP | TVS_SHOWSELALWAYS | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_NONEVENHEIGHT),
							rect, 
							this, 
							IDC_TASKTREE))
	{
		return -1;
	}
	
	// Tasks Header ---------------------------------------------------------------------
	if (!m_hdrTasks.Create((dwStyle | HDS_BUTTONS | HDS_DRAGDROP), rect, this, IDC_TASKTREEHEADER))
	{
		return -1;
	}
	
	// Column List ---------------------------------------------------------------------
	if (!m_lcColumns.Create((dwStyle | WS_TABSTOP),	rect, this, IDC_TASKTREECOLUMNS))
	{
		return -1;
	}
	
	// extended styles
	ListView_SetExtendedListViewStyleEx(m_lcColumns, LVS_EX_HEADERDRAGDROP, LVS_EX_HEADERDRAGDROP);
	
	// subclass the tree and list
	if (!Sync(m_tcTasks, m_lcColumns, TLSL_RIGHTDATA_IS_LEFTITEM, m_hdrTasks))
	{
		return -1;
	}
	
	// Column Header ---------------------------------------------------------------------
	if (!m_hdrColumns.SubclassWindow(ListView_GetHeader(m_lcColumns)))
	{
		return FALSE;
	}
	m_hdrColumns.EnableToolTips();
	
	BuildTreeColumns();
	BuildListColumns();
	
	CalcMinMonthWidths();

	// prevent column reordering on columns
	m_hdrColumns.ModifyStyle(HDS_DRAGDROP, 0);

	// prevent translation of the list header
	CLocalizer::EnableTranslation(m_hdrColumns, FALSE);

	// Initialise tree drag and drop
	m_treeDragDrop.Initialize(m_tcTasks.GetParent(), TRUE, FALSE);

	// misc
	m_tcTasks.ModifyStyle(TVS_SHOWSELALWAYS, 0, 0);
	m_lcColumns.ModifyStyle(LVS_SHOWSELALWAYS, 0, 0);

	ListView_SetExtendedListViewStyleEx(m_lcColumns, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

 	PostResize();
	
	return 0;
}

/*
BOOL CWorkloadCtrl::Initialize(UINT nIDTreeHeader)
{
	ASSERT(m_tcTasks.GetSafeHwnd());
	ASSERT(m_lcColumns.GetSafeHwnd());

	// misc
	m_nMonthWidth = DEF_MONTH_WIDTH;

	// initialize tree header
	if (!m_hdrTasks.SubclassDlgItem(nIDTreeHeader, m_tcTasks.GetParent()))
		return FALSE;

	m_hdrTasks.ModifyStyle(0, (HDS_FULLDRAG | HDS_HOTTRACK | HDS_BUTTONS | HDS_DRAGDROP));

	// subclass the tree and list
	if (!CTreeListSyncer::Sync(m_tcTasks, m_lcColumns, TLSL_RIGHTDATA_IS_LEFTITEM, m_hdrTasks))
		return FALSE;

	// subclass the list header
	VERIFY(m_hdrColumns.SubclassWindow(ListView_GetHeader(m_lcColumns)));
	

	BuildTreeColumns();
	BuildListColumns();

	CalcMinMonthWidths();

	if (m_nMonthWidth != DEF_MONTH_WIDTH)
		RecalcListColumnWidths(DEF_MONTH_WIDTH, m_nMonthWidth);

	return TRUE;
}
*/

void CWorkloadCtrl::InitItemHeights()
{
	CTreeListSyncer::InitItemHeights();

	//WorkloadDEPENDENCY::STUB = (m_tcTasks.GetItemHeight() / 2);
}

/*
void CWorkloadCtrl::Release() 
{ 
	if (::IsWindow(m_hdrTasks))
		m_hdrTasks.UnsubclassWindow();

	if (::IsWindow(m_hdrColumns))
		m_hdrColumns.UnsubclassWindow();

	Unsync(); 

	delete m_pTCH;
	m_pTCH = NULL;
}
*/

HTREEITEM CWorkloadCtrl::GetSelectedItem() const
{
	return GetTreeSelItem();
}

DWORD CWorkloadCtrl::GetSelectedTaskID() const
{
	HTREEITEM hti = GetTreeSelItem();

	return (hti ? GetTaskID(hti) : 0);
}

BOOL CWorkloadCtrl::GetSelectedTaskDates(COleDateTime& dtStart, COleDateTime& dtDue) const
{
	DWORD dwTaskID = GetSelectedTaskID();
	const WORKLOADITEM* pGI = NULL;

	GET_GI_RET(dwTaskID, pGI, FALSE);
	
	if (GetTaskStartDueDates(*pGI, dtStart, dtDue))
	{
		// handle durations of whole days
		COleDateTime dtDuration(dtDue - dtStart);

		if (CDateHelper::IsDateSet(dtDuration) && (dtDuration > CDateHelper::GetEndOfDay(dtDuration)))
		{
			double dWholeDays = (CDateHelper::GetDateOnly(dtDuration).m_dt + 1.0);

			if (!CDateHelper::DateHasTime(dtStart))
				dWholeDays--;

			dtDue.m_dt = (dtStart.m_dt + dWholeDays);
		}

		return TRUE;
	}

	// all else
	return FALSE;
}

BOOL CWorkloadCtrl::SelectTask(DWORD dwTaskID)
{
	HTREEITEM hti = FindTreeItem(m_tcTasks, dwTaskID);

	return SelectItem(hti);
}

BOOL CWorkloadCtrl::SelectItem(HTREEITEM hti)
{
	if (hti == NULL)
		return FALSE;

	BOOL bWasVisible = IsTreeItemVisible(m_tcTasks, hti);

	SelectTreeItem(hti, FALSE);
	ResyncSelection(m_lcColumns, m_tcTasks, FALSE);

	if (!bWasVisible)
		ExpandList();

	return TRUE;
}

BOOL CWorkloadCtrl::SelectTask(IUI_APPCOMMAND nCmd, const IUISELECTTASK& select)
{
	HTREEITEM htiStart = NULL;
	BOOL bForwards = TRUE;

	switch (nCmd)
	{
	case IUI_SELECTFIRSTTASK:
		htiStart = m_tcTasks.TCH().GetFirstItem();
		break;

	case IUI_SELECTNEXTTASK:
		htiStart = m_tcTasks.TCH().GetNextItem(GetSelectedItem());
		break;
		
	case IUI_SELECTNEXTTASKINCLCURRENT:
		htiStart = GetSelectedItem();
		break;

	case IUI_SELECTPREVTASK:
		htiStart = m_tcTasks.TCH().GetPrevItem(GetSelectedItem());

		if (htiStart == NULL) // we were on the first task
			htiStart = m_tcTasks.TCH().GetLastItem();
		
		bForwards = FALSE;
		break;

	case IUI_SELECTLASTTASK:
		htiStart = m_tcTasks.TCH().GetLastItem();
		bForwards = FALSE;
		break;

	default:
		return FALSE;
	}

	return SelectTask(htiStart, select, bForwards);
}

BOOL CWorkloadCtrl::SelectTask(HTREEITEM hti, const IUISELECTTASK& select, BOOL bForwards)
{
	if (!hti)
		return FALSE;

	CString sTitle = m_tcTasks.GetItemText(hti);

	if (Misc::Find(select.szWords, sTitle, select.bCaseSensitive, select.bWholeWord) != -1)
	{
		if (SelectItem(hti))
			return TRUE;

		ASSERT(0);
	}

	if (bForwards)
		return SelectTask(m_tcTasks.TCH().GetNextItem(hti), select, TRUE);

	// else
	return SelectTask(m_tcTasks.TCH().GetPrevItem(hti), select, FALSE);
}

void CWorkloadCtrl::RecalcParentDates()
{
	WORKLOADITEM dummy;
	WORKLOADITEM* pGI = &dummy;

	RecalcParentDates(NULL, pGI);
}

void CWorkloadCtrl::RecalcParentDates(HTREEITEM htiParent, WORKLOADITEM*& pGI)
{
	// ignore root
	DWORD dwTaskID = 0;

	// get Workload item for this tree item
	if (htiParent)
	{
		dwTaskID = m_tcTasks.GetItemData(htiParent);
		GET_GI(dwTaskID, pGI);
	}
	
	// bail if this is a reference
	if (pGI->dwRefID)
		return;
	
	// reset dates
	pGI->dtMinStart = pGI->dtStart;
	pGI->dtMaxDue = pGI->dtDue;

	// iterate children 
	HTREEITEM htiChild = m_tcTasks.GetChildItem(htiParent);
	
	while (htiChild)
	{
		WORKLOADITEM* pGIChild;

		// recalc child if it has children itself
		RecalcParentDates(htiChild, pGIChild);
		ASSERT(pGIChild);

		// keep track of earliest start date and latest due date
		if (pGIChild)
			pGI->MinMaxDates(*pGIChild);

		// next child
		htiChild = m_tcTasks.GetNextItem(htiChild, TVGN_NEXT);
	}
}

int CWorkloadCtrl::GetExpandedState(CDWordArray& aExpanded, HTREEITEM hti) const
{
	int nStart = 0;

	if (hti == NULL)
	{
		// guestimate initial size
		aExpanded.SetSize(0, m_tcTasks.GetCount() / 4);
	}
	else if (TCH().IsItemExpanded(hti) <= 0)
	{
		return 0; // nothing added
	}
	else // expanded
	{
		nStart = aExpanded.GetSize();
		aExpanded.Add(GetTaskID(hti));
	}

	// process children
	HTREEITEM htiChild = m_tcTasks.GetChildItem(hti);

	while (htiChild)
	{
		GetExpandedState(aExpanded, htiChild);
		htiChild = m_tcTasks.GetNextItem(htiChild, TVGN_NEXT);
	}

	return (aExpanded.GetSize() - nStart);
}

void CWorkloadCtrl::SetExpandedState(const CDWordArray& aExpanded)
{
	int nNumExpanded = aExpanded.GetSize();

	if (nNumExpanded)
	{
		for (int nItem = 0; nItem < nNumExpanded; nItem++)
		{
			HTREEITEM hti = GetTreeItem(aExpanded[nItem]);

			if (hti)
				m_tcTasks.Expand(hti, TVE_EXPAND);
		}

		ExpandList();
	}
}

BOOL CWorkloadCtrl::EditWantsResort(IUI_UPDATETYPE nUpdate, const CSet<IUI_ATTRIBUTE>& attrib) const
{
	switch (nUpdate)
	{
	case IUI_ALL:
		// Note: Tasks should arrive 'unsorted' so we only need to
		// resort if an attribute is set
		return m_sort.IsSorting();

	case IUI_NEW:
		// Don't sort new tasks because it's confusing
		return FALSE;

	case IUI_EDIT:
		if (m_sort.IsSorting())
		{
			if (!m_sort.bMultiSort)
				return attrib.Has(MapColumnToAttribute(m_sort.single.nBy));

			// else
			for (int nCol = 0; nCol < 3; nCol++)
			{
				if (attrib.Has(MapColumnToAttribute(m_sort.multi.cols[nCol].nBy)))
					return TRUE;
			}
		}
		break;

	case IUI_DELETE:
		break;

	default:
		ASSERT(0);
	}

	return FALSE;
}

void CWorkloadCtrl::UpdateTasks(const ITaskList* pTaskList, IUI_UPDATETYPE nUpdate, const CSet<IUI_ATTRIBUTE>& attrib)
{
	// we must have been initialized already
	ASSERT(m_lcColumns.GetSafeHwnd() && m_tcTasks.GetSafeHwnd());

	// always cancel any ongoing operation
	CancelOperation();

	const ITASKLISTBASE* pTasks = GetITLInterface<ITASKLISTBASE>(pTaskList, IID_TASKLISTBASE);

	if (pTasks == NULL)
	{
		ASSERT(0);
		return;
	}

	switch (nUpdate)
	{
	case IUI_ALL:
		{
			CWorkloadLockUpdates glu(this, TRUE, TRUE);
			
			CDWordArray aExpanded;
			GetExpandedState(aExpanded);
			
			DWORD dwSelID = GetSelectedTaskID();
			
			RebuildTree(pTasks);

			ValidateMonthDisplay();
			UpdateListColumns();

			// Odd bug: The very last tree item will sometimes not scroll into view. 
			// Expanding and collapsing an item is enough to resolve the issue. 
			// First time only though.
			if (aExpanded.GetSize() == 0)
				PreFixVScrollSyncBug();
			
			SetExpandedState(aExpanded);
			SelectTask(dwSelID);

			if (dwSelID)
				ScrollToSelectedTask();
			else
				ScrollToToday();
		}
		break;
		
	case IUI_NEW:
	case IUI_EDIT:
		{
			CHoldRedraw hr(m_tcTasks);
			CHoldRedraw hr2(m_lcColumns);
			
			// cache current year range to test for changes
			int nNumMonths = GetNumMonths(m_nMonthDisplay);
			
			// update the task(s)
			if (UpdateTask(pTasks, pTasks->GetFirstTask(), nUpdate, attrib, TRUE))
			{
				// recalc parent dates as required
				if (attrib.Has(IUI_STARTDATE) || attrib.Has(IUI_DUEDATE) || attrib.Has(IUI_DONEDATE))
				{
					RecalcDateRange();
					RecalcParentDates();
				}
				
				// Refresh list columns as required
				if (GetNumMonths(m_nMonthDisplay) != nNumMonths)
				{
					ValidateMonthDisplay();
					UpdateListColumns();
				}
			}
		}
		break;
		
	case IUI_DELETE:
		{
			CHoldRedraw hr(m_tcTasks);
			CHoldRedraw hr2(m_lcColumns);

			CSet<DWORD> mapIDs;
			BuildTaskMap(pTasks, pTasks->GetFirstTask(), mapIDs, TRUE);
			
			RemoveDeletedTasks(NULL, pTasks, mapIDs);

			// cache current year range to test for changes
			int nNumMonths = GetNumMonths(m_nMonthDisplay);
			
			RefreshTreeItemMap();
			RecalcDateRange();
			RecalcParentDates();

			// fixup list columns as required
			if (GetNumMonths(m_nMonthDisplay) != nNumMonths)
			{
				ValidateMonthDisplay();
				UpdateListColumns();
			}
		}
		break;
		
	default:
		ASSERT(0);
	}
	
	InitItemHeights();
	RecalcTreeColumns(TRUE);

	if (EditWantsResort(nUpdate, attrib))
	{
		ASSERT(m_sort.IsSorting());

		CHoldRedraw hr(m_tcTasks);

		if (m_sort.bMultiSort)
			CTreeListSyncer::Sort(MultiSortProc, (DWORD)this);
		else
			CTreeListSyncer::Sort(SortProc, (DWORD)this);
	}
}

void CWorkloadCtrl::PreFixVScrollSyncBug()
{
	// Odd bug: The very last tree item will not scroll into view. 
	// Expanding and collapsing an item is enough to resolve the issue.
	HTREEITEM hti = TCH().FindFirstParent();
		
	if (hti)
	{
		TCH().ExpandItem(hti, TRUE);
		TCH().ExpandItem(hti, FALSE);
	}
}

CString CWorkloadCtrl::GetTaskAllocTo(const ITASKLISTBASE* pTasks, HTASKITEM hTask)
{
	int nAllocTo = pTasks->GetTaskAllocatedToCount(hTask);
	
	if (nAllocTo == 0)
	{
		return _T("");
	}
	else if (nAllocTo == 1)
	{
		return pTasks->GetTaskAllocatedTo(hTask, 0);
	}
	
	// nAllocTo > 1 
	CStringArray aAllocTo;
	
	while (nAllocTo--)
		aAllocTo.InsertAt(0, pTasks->GetTaskAllocatedTo(hTask, nAllocTo));
	
	return Misc::FormatArray(aAllocTo);
}

BOOL CWorkloadCtrl::WantEditUpdate(IUI_ATTRIBUTE nAttrib)
{
	switch (nAttrib)
	{
	case IUI_ALLOCTO:
	case IUI_COLOR:
	case IUI_DEPENDENCY:
	case IUI_DONEDATE:
	case IUI_DUEDATE:
	case IUI_ICON:
	case IUI_ID:
	case IUI_NONE:
	case IUI_PERCENT:
	case IUI_STARTDATE:
	case IUI_SUBTASKDONE:
	case IUI_TAGS:
	case IUI_TASKNAME:
		return TRUE;
	}
	
	// all else 
	return (nAttrib == IUI_ALL);
}

BOOL CWorkloadCtrl::WantSortUpdate(IUI_ATTRIBUTE nAttrib)
{
	switch (nAttrib)
	{
	case IUI_ALLOCTO:
	case IUI_DUEDATE:
	case IUI_ID:
	case IUI_PERCENT:
	case IUI_STARTDATE:
	case IUI_TASKNAME:
	case IUI_TAGS:
	case IUI_DONEDATE:
	case IUI_DEPENDENCY:
		return (MapAttributeToColumn(nAttrib) != WLCC_NONE);

	case IUI_NONE:
		return TRUE;
	}
	
	// all else 
	return FALSE;
}

IUI_ATTRIBUTE CWorkloadCtrl::MapColumnToAttribute(WLC_COLUMN nCol)
{
	switch (nCol)
	{
	case WLCC_TITLE:		return IUI_TASKNAME;
	case WLCC_DUEDATE:		return IUI_DUEDATE;
	case WLCC_STARTDATE:	return IUI_STARTDATE;
	case WLCC_ALLOCTO:		return IUI_ALLOCTO;
	case WLCC_PERCENT:		return IUI_PERCENT;
	case WLCC_TASKID:		return IUI_ID;
	case WLCC_DONEDATE:	return IUI_DONEDATE;
	case WLCC_TAGS:		return IUI_TAGS;
	case WLCC_DEPENDENCY:	return IUI_DEPENDENCY;
	}
	
	// all else 
	return IUI_NONE;
}

WLC_COLUMN CWorkloadCtrl::MapAttributeToColumn(IUI_ATTRIBUTE nAttrib)
{
	switch (nAttrib)
	{
	case IUI_TASKNAME:		return WLCC_TITLE;		
	case IUI_DUEDATE:		return WLCC_DUEDATE;		
	case IUI_STARTDATE:		return WLCC_STARTDATE;	
	case IUI_ALLOCTO:		return WLCC_ALLOCTO;		
	case IUI_PERCENT:		return WLCC_PERCENT;		
	case IUI_ID:			return WLCC_TASKID;		
	case IUI_DONEDATE:		return WLCC_DONEDATE;
	case IUI_TAGS:			return WLCC_TAGS;
	case IUI_DEPENDENCY:	return WLCC_DEPENDENCY;
	}
	
	// all else 
	return WLCC_NONE;
}

BOOL CWorkloadCtrl::UpdateTask(const ITASKLISTBASE* pTasks, HTASKITEM hTask, 
									IUI_UPDATETYPE nUpdate, const CSet<IUI_ATTRIBUTE>& attrib, 
									BOOL bAndSiblings)
{
	if (hTask == NULL)
		return FALSE;

	ASSERT((nUpdate == IUI_EDIT) || (nUpdate == IUI_NEW));

	// handle task if not NULL (== root)
	DWORD dwTaskID = pTasks->GetTaskID(hTask);
	m_dwMaxTaskID = max(m_dwMaxTaskID, dwTaskID);

	// This can be a new task
	if (!m_data.HasItem(dwTaskID))
	{
		ASSERT(nUpdate == IUI_NEW);

		// Parent must exist or be NULL
		DWORD dwParentID = pTasks->GetTaskParentID(hTask);
		HTREEITEM htiParent = NULL;

		if (dwParentID)
		{
			if (!m_data.HasItem(dwParentID))
			{
				ASSERT(0);
				return FALSE;
			}

			htiParent = GetTreeItem(dwParentID);

			if (!htiParent)
			{
				ASSERT(0);
				return FALSE;
			}
		}

		// Before anything else we increment the position of 
		// any tasks having the same position of this new task 
		// or greater within the parent
		int nPos = pTasks->GetTaskPosition(hTask);
		IncrementItemPositions(htiParent, nPos);

		BuildTreeItem(pTasks, hTask, htiParent, FALSE, FALSE);
		RefreshTreeItemMap();

		return TRUE;
	}
	
	WORKLOADITEM* pGI = NULL;
	GET_GI_RET(dwTaskID, pGI, FALSE);

	// update taskID to refID 
	if (pGI->dwOrgRefID)
	{
		dwTaskID = pGI->dwOrgRefID;
		pGI->dwOrgRefID = 0;
	}

	// take a snapshot we can check changes against
	WORKLOADITEM giOrg = *pGI;

	// can't use a switch here because we also need to check for IUI_ALL
	time64_t tDate = 0;
	
	if (attrib.Has(IUI_TASKNAME))
		pGI->sTitle = pTasks->GetTaskTitle(hTask);
	
	if (attrib.Has(IUI_ALLOCTO))
		pGI->sAllocTo = GetTaskAllocTo(pTasks, hTask);
	
	if (attrib.Has(IUI_ICON))
		pGI->bHasIcon = !Misc::IsEmpty(pTasks->GetTaskIcon(hTask));

	if (attrib.Has(IUI_PERCENT))
		pGI->nPercent = pTasks->GetTaskPercentDone(hTask, TRUE);
		
	if (attrib.Has(IUI_STARTDATE))
	{
		if (pTasks->GetTaskStartDate64(hTask, pGI->bParent, tDate))
		{
			pGI->dtStart = pGI->dtMinStart = GetDate(tDate, FALSE); // start of day
		}
		else
		{
			CDateHelper::ClearDate(pGI->dtStart);
			CDateHelper::ClearDate(pGI->dtMinStart);
		}
	}
	
	if (attrib.Has(IUI_DUEDATE))
	{
		if (pTasks->GetTaskDueDate64(hTask, pGI->bParent, tDate))
		{
			pGI->dtDue = pGI->dtMaxDue = GetDate(tDate, TRUE); // end of day
		}
		else
		{
			CDateHelper::ClearDate(pGI->dtDue);
			CDateHelper::ClearDate(pGI->dtMaxDue);
		}
	}
	
	if (attrib.Has(IUI_DONEDATE))
	{
		if (pTasks->GetTaskDoneDate64(hTask, tDate))
			pGI->dtDone = GetDate(tDate, TRUE);
		else
			CDateHelper::ClearDate(pGI->dtDone);
	}
	
	if (attrib.Has(IUI_SUBTASKDONE))
	{
		LPCWSTR szSubTaskDone = pTasks->GetTaskSubtaskCompletion(hTask);
		pGI->bSomeSubtaskDone = (!Misc::IsEmpty(szSubTaskDone) && (szSubTaskDone[0] != '0'));
	}

	if (attrib.Has(IUI_TAGS))
	{
		int nTag = pTasks->GetTaskTagCount(hTask);
		pGI->aTags.RemoveAll();
		
		while (nTag--)
			pGI->aTags.Add(pTasks->GetTaskTag(hTask, nTag));
	}
	
	if (attrib.Has(IUI_DEPENDENCY))
	{
		int nDepend = pTasks->GetTaskDependencyCount(hTask);
		pGI->aDependIDs.RemoveAll();
		
		while (nDepend--)
		{
			// Local dependencies only
			DWORD dwTaskID = _ttoi(pTasks->GetTaskDependency(hTask, nDepend));

			if (dwTaskID)
				pGI->aDependIDs.Add(dwTaskID);
		}
	}

	// update date range
	m_dateRange.MinMax(*pGI);
	
	// always update lock states
	pGI->bLocked = pTasks->IsTaskLocked(hTask, true);

	// always update colour because it can change for so many reasons
	pGI->color = pTasks->GetTaskTextColor(hTask);

	// likewise 'Good as Done'
	pGI->bGoodAsDone = pTasks->IsTaskGoodAsDone(hTask);

	// detect update
	BOOL bChange = !(*pGI == giOrg);
		
	// children
	if (UpdateTask(pTasks, pTasks->GetFirstTask(hTask), nUpdate, attrib, TRUE))
		bChange = TRUE;

	// handle siblings WITHOUT RECURSION
	if (bAndSiblings)
	{
		HTASKITEM hSibling = pTasks->GetNextTask(hTask);
		
		while (hSibling)
		{
			// FALSE == not siblings
			if (UpdateTask(pTasks, hSibling, nUpdate, attrib, FALSE))
				bChange = TRUE;
			
			hSibling = pTasks->GetNextTask(hSibling);
		}
	}
	
	return bChange;
}

void CWorkloadCtrl::BuildTaskMap(const ITASKLISTBASE* pTasks, HTASKITEM hTask, 
									  CSet<DWORD>& mapIDs, BOOL bAndSiblings)
{
	if (hTask == NULL)
		return;

	mapIDs.Add(pTasks->GetTaskID(hTask));

	// children
	BuildTaskMap(pTasks, pTasks->GetFirstTask(hTask), mapIDs, TRUE);

	// handle siblings WITHOUT RECURSION
	if (bAndSiblings)
	{
		HTASKITEM hSibling = pTasks->GetNextTask(hTask);
		
		while (hSibling)
		{
			// FALSE == not siblings
			BuildTaskMap(pTasks, hSibling, mapIDs, FALSE);
			hSibling = pTasks->GetNextTask(hSibling);
		}
	}
}

void CWorkloadCtrl::RemoveDeletedTasks(HTREEITEM hti, const ITASKLISTBASE* pTasks, const CSet<DWORD>& mapIDs)
{
	// traverse the tree looking for items that do not 
	// exist in pTasks and delete them
	if (hti && !mapIDs.Has(GetTaskID(hti)))
	{
		DeleteTreeItem(hti);
		return;
	}

	// check its children
	HTREEITEM htiChild = m_tcTasks.GetChildItem(hti);
	
	while (htiChild)
	{
		// get next sibling before we (might) delete this one
		HTREEITEM htiNext = m_tcTasks.GetNextItem(htiChild, TVGN_NEXT);
		
		RemoveDeletedTasks(htiChild, pTasks, mapIDs);
		htiChild = htiNext;
	}
}

WORKLOADITEM* CWorkloadCtrl::GetWorkloadItem(DWORD dwTaskID, BOOL bCopyRefID) const
{
	WORKLOADITEM* pGI = m_data.GetItem(dwTaskID);
	ASSERT(pGI);
	
	if (pGI)
	{
		// For references we use the 'real' task but with the 
		// original reference reference ID copied over
		DWORD dwRefID = pGI->dwRefID;
		
		if (dwRefID && (dwRefID != dwTaskID) && m_data.Lookup(dwRefID, pGI))
		{
			// copy over the reference id so that the caller can still detect it
			if (bCopyRefID)
			{
				ASSERT((pGI->dwRefID == 0) || (pGI->dwRefID == dwRefID));
				pGI->dwOrgRefID = dwRefID;
			}
		}
		else
		{
			pGI->dwOrgRefID = 0;
		}
	}
	
	return pGI;
}

BOOL CWorkloadCtrl::RestoreWorkloadItem(const WORKLOADITEM& giPrev)
{
	if (m_data.RestoreItem(giPrev))
	{
		RecalcParentDates();
		RedrawList();
	
		return TRUE;
	}

	// else
	return FALSE;
}

void CWorkloadCtrl::RebuildTree(const ITASKLISTBASE* pTasks)
{
	m_tcTasks.DeleteAllItems();
	m_lcColumns.DeleteAllItems();

	m_data.RemoveAll();

	m_dwMaxTaskID = 0;

	// cache and reset year range which will get 
	// recalculated as we build the tree
	WORKLOADDATERANGE prevRange = m_dateRange;
	m_dateRange.Clear();

	BuildTreeItem(pTasks, pTasks->GetFirstTask(), NULL, TRUE);

	// restore previous date range if no data
	if (m_data.GetCount() == 0)
		m_dateRange = prevRange;

	RefreshTreeItemMap();
	RecalcParentDates();
	ExpandList();
	RefreshItemBoldState();
}

void CWorkloadCtrl::RefreshTreeItemMap()
{
	TCH().BuildHTIMap(m_mapHTItems);
}

void CWorkloadCtrl::RecalcDateRange()
{
	if (m_data.GetCount())
	{
		m_dateRange.Clear();

		POSITION pos = m_data.GetStartPosition();
		WORKLOADITEM* pGI = NULL;
		DWORD dwTaskID = 0;

		while (pos)
		{
			m_data.GetNextAssoc(pos, dwTaskID, pGI);
			ASSERT(pGI);

			if (pGI)
				m_dateRange.MinMax(*pGI);
		}
	}
}

COleDateTime CWorkloadCtrl::GetDate(time64_t tDate, BOOL bEndOfDay)
{
	COleDateTime date = CDateHelper::GetDate(tDate);

	// only implement 'end of day' if the date has no time
	if (CDateHelper::IsDateSet(date) && bEndOfDay && !CDateHelper::DateHasTime(date))
		date = CDateHelper::GetEndOfDay(date);

	return date;
}

void CWorkloadCtrl::BuildTreeItem(const ITASKLISTBASE* pTasks, HTASKITEM hTask, 
										HTREEITEM htiParent, BOOL bAndSiblings, BOOL bInsertAtEnd)
{
	if (hTask == NULL)
		return;

	DWORD dwTaskID = pTasks->GetTaskID(hTask);
	ASSERT(!m_data.HasItem(dwTaskID));

	m_dwMaxTaskID = max(m_dwMaxTaskID, dwTaskID);

	// map the data
	WORKLOADITEM* pGI = new WORKLOADITEM;
	m_data[dwTaskID] = pGI;
	
	pGI->dwTaskID = dwTaskID;
	pGI->dwRefID = pTasks->GetTaskReferenceID(hTask);

	// Except for position
	pGI->nPosition = pTasks->GetTaskPosition(hTask);

	// Only save data for non-references
	if (pGI->dwRefID == 0)
	{
		pGI->sTitle = pTasks->GetTaskTitle(hTask);
		pGI->color = pTasks->GetTaskTextColor(hTask);
		pGI->bGoodAsDone = pTasks->IsTaskGoodAsDone(hTask);
		pGI->sAllocTo = GetTaskAllocTo(pTasks, hTask);
		pGI->bParent = pTasks->IsTaskParent(hTask);
		pGI->nPercent = pTasks->GetTaskPercentDone(hTask, TRUE);
		pGI->bLocked = pTasks->IsTaskLocked(hTask, true);
		pGI->bHasIcon = !Misc::IsEmpty(pTasks->GetTaskIcon(hTask));

		LPCWSTR szSubTaskDone = pTasks->GetTaskSubtaskCompletion(hTask);
		pGI->bSomeSubtaskDone = (!Misc::IsEmpty(szSubTaskDone) && (szSubTaskDone[0] != '0'));

		time64_t tDate = 0;

		if (pTasks->GetTaskStartDate64(hTask, pGI->bParent, tDate))
			pGI->dtStart = GetDate(tDate, FALSE);

		if (pTasks->GetTaskDueDate64(hTask, pGI->bParent, tDate))
			pGI->dtDue = GetDate(tDate, TRUE);

		if (pTasks->GetTaskDoneDate64(hTask, tDate))
			pGI->dtDone = GetDate(tDate, TRUE);

		int nTag = pTasks->GetTaskTagCount(hTask);

		while (nTag--)
			pGI->aTags.Add(pTasks->GetTaskTag(hTask, nTag));

		// Local dependencies only
		int nDepend = pTasks->GetTaskDependencyCount(hTask);
		
		while (nDepend--)
		{	
			DWORD dwTaskID = _ttoi(pTasks->GetTaskDependency(hTask, nDepend));

			if (dwTaskID)
				pGI->aDependIDs.Add(dwTaskID);
		}
		
		// track earliest and latest dates
		m_dateRange.MinMax(*pGI);
	}
	
	// add item to tree
	HTREEITEM htiAfter = TVI_LAST; // default

	if (!bInsertAtEnd)
	{
		// Find the sibling task whose position is one less
		HTREEITEM htiSibling = m_tcTasks.GetChildItem(htiParent);

		while (htiSibling)
		{
			DWORD dwSiblingID = m_tcTasks.GetItemData(htiSibling);
			const WORKLOADITEM* pGISibling = GetWorkloadItem(dwSiblingID);
			ASSERT(pGISibling);

			if (pGISibling && (pGISibling->nPosition == (pGI->nPosition - 1)))
			{
				htiAfter = htiSibling;
				break;
			}

			htiSibling = m_tcTasks.GetNextItem(htiSibling, TVGN_NEXT);
		}
	}

	HTREEITEM hti = m_tcTasks.TCH().InsertItem(LPSTR_TEXTCALLBACK, 
											I_IMAGECALLBACK, 
											I_IMAGECALLBACK, 
											dwTaskID, // lParam
											htiParent, 
											htiAfter,
											FALSE,
											FALSE);
	
	// add first child which will add all the rest
	BuildTreeItem(pTasks, pTasks->GetFirstTask(hTask), hti, TRUE);
	
	// handle siblings WITHOUT RECURSION
	if (bAndSiblings)
	{
		HTASKITEM hSibling = pTasks->GetNextTask(hTask);
		
		while (hSibling)
		{
			// FALSE == not siblings
			BuildTreeItem(pTasks, hSibling, htiParent, FALSE);
			
			hSibling = pTasks->GetNextTask(hSibling);
		}
	}
}

void CWorkloadCtrl::IncrementItemPositions(HTREEITEM htiParent, int nFromPos)
{
	HTREEITEM htiChild = m_tcTasks.GetChildItem(htiParent);

	while (htiChild)
	{
		DWORD dwTaskID = GetTaskID(htiChild);
		WORKLOADITEM* pGI = NULL;

		GET_GI(dwTaskID, pGI);

		if (pGI->nPosition >= nFromPos)
			pGI->nPosition++;
		
		htiChild = m_tcTasks.GetNextItem(htiChild, TVGN_NEXT);
	}
}

COleDateTime CWorkloadCtrl::GetStartDate(WLC_MONTH_DISPLAY nDisplay) const
{
	return m_dateRange.GetStart(nDisplay, !HasOption(WLCF_DECADESAREONEBASED));
}

COleDateTime CWorkloadCtrl::GetEndDate(WLC_MONTH_DISPLAY nDisplay) const
{
	return m_dateRange.GetEnd(nDisplay, !HasOption(WLCF_DECADESAREONEBASED));
}

int CWorkloadCtrl::GetStartYear(WLC_MONTH_DISPLAY nDisplay) const
{
	return GetStartDate(nDisplay).GetYear();
}

int CWorkloadCtrl::GetEndYear(WLC_MONTH_DISPLAY nDisplay) const
{
	int nYear = GetEndDate(nDisplay).GetYear();

	// for now, do not let end year exceed MAX_YEAR
	return min(nYear, MAX_YEAR);
}

int CWorkloadCtrl::GetNumMonths(WLC_MONTH_DISPLAY nDisplay) const
{
	COleDateTime dtStart(GetStartDate(nDisplay)), dtEnd(GetEndDate(nDisplay));
	
	int nStartMonth = dtStart.GetMonth(), nStartYear = dtStart.GetYear();
	int nEndMonth = dtEnd.GetMonth(), nEndYear = dtEnd.GetYear();

	int nNumMonths = ((((nEndYear * 12) + nEndMonth) - ((nStartYear * 12) + nStartMonth)) + 1);
	ASSERT(nNumMonths > 0);

	return nNumMonths;
}

void CWorkloadCtrl::SetOption(DWORD dwOption, BOOL bSet)
{
	if (dwOption)
	{
		DWORD dwPrev = m_dwOptions;

		if (bSet)
			m_dwOptions |= dwOption;
		else
			m_dwOptions &= ~dwOption;

		// specific handling
		if (m_dwOptions != dwPrev)
		{
			switch (dwOption)
			{
			case WLCF_STRIKETHRUDONETASKS:
				m_tcTasks.Fonts().Clear();
				CWnd::Invalidate(FALSE);
				break;

			case WLCF_DECADESAREONEBASED:
				if ((m_nMonthDisplay == WLC_DISPLAY_QUARTERCENTURIES) || 
					(m_nMonthDisplay == WLC_DISPLAY_DECADES))
				{
					UpdateListColumnsWidthAndText();
				}
				break;

			case WLCF_SHOWSPLITTERBAR:
				CTreeListSyncer::SetSplitBarWidth(bSet ? 10 : 0);
				break;

			case WLCF_DISPLAYISODATES:
				UpdateListColumnsWidthAndText();
				CWnd::Invalidate(FALSE);
				break;

			case WLCF_SHOWTREECHECKBOXES:
				m_tcTasks.ShowCheckboxes(bSet);
				break;
			}

			if (IsSyncing())
				RedrawList();
		}
	}
}

CString CWorkloadCtrl::FormatListColumnHeaderText(WLC_MONTH_DISPLAY nDisplay, int nMonth, int nYear) const
{
	if (nMonth == 0)
		return _T("");
	
	//else
	CString sHeader;
	
	switch (nDisplay)
	{
	case WLC_DISPLAY_QUARTERCENTURIES:
	case WLC_DISPLAY_DECADES:
		{
			int nStartYear = GetStartYear(nDisplay);
			int nEndYear = GetEndYear(nDisplay);

			sHeader.Format(_T("%d-%d"), nStartYear, nEndYear);
		}
		break;

	case WLC_DISPLAY_YEARS:
		sHeader.Format(_T("%d"), nYear);
		break;
		
	case WLC_DISPLAY_QUARTERS_SHORT:
		sHeader.Format(_T("Q%d %d"), (1 + ((nMonth-1) / 3)), nYear);
		break;
		
	case WLC_DISPLAY_QUARTERS_MID:
		sHeader.Format(_T("%s-%s %d"), 
			CDateHelper::GetMonthName(nMonth, TRUE),
			CDateHelper::GetMonthName(nMonth+2, TRUE), 
			nYear);
		break;
		
	case WLC_DISPLAY_QUARTERS_LONG:
		sHeader.Format(_T("%s-%s %d"), 
			CDateHelper::GetMonthName(nMonth, FALSE),
			CDateHelper::GetMonthName(nMonth+2, FALSE), 
			nYear);
		break;
		
	case WLC_DISPLAY_MONTHS_SHORT:
		sHeader = FormatDate(COleDateTime(nYear, nMonth, 1, 0, 0, 0), (DHFD_NODAY | DHFD_NOCENTURY));
		break;
		
	case WLC_DISPLAY_MONTHS_MID:
		sHeader.Format(_T("%s %d"), CDateHelper::GetMonthName(nMonth, TRUE), nYear);
		break;
		
	case WLC_DISPLAY_MONTHS_LONG:
		sHeader.Format(_T("%s %d"), CDateHelper::GetMonthName(nMonth, FALSE), nYear);
		break;

	case WLC_DISPLAY_WEEKS_SHORT:
	case WLC_DISPLAY_WEEKS_MID:
	case WLC_DISPLAY_WEEKS_LONG:
		sHeader.Format(_T("%s %d (%s)"), CDateHelper::GetMonthName(nMonth, FALSE), nYear, CEnString(IDS_WORKLOAD_WEEKS));
		break;

	case WLC_DISPLAY_DAYS_SHORT:
	case WLC_DISPLAY_DAYS_MID:
	case WLC_DISPLAY_DAYS_LONG:
	case WLC_DISPLAY_HOURS:
		sHeader.Format(_T("%s %d (%s)"), CDateHelper::GetMonthName(nMonth, FALSE), nYear, CEnString(IDS_WORKLOAD_DAYS));
		break;
		
	default:
		ASSERT(0);
		break;
	}

	return sHeader;
}

double CWorkloadCtrl::GetMonthWidth(int nColWidth) const
{
	return GetMonthWidth(m_nMonthDisplay, nColWidth);
}

double CWorkloadCtrl::GetMonthWidth(WLC_MONTH_DISPLAY nDisplay, int nColWidth)
{
	switch (nDisplay)
	{
	case WLC_DISPLAY_QUARTERCENTURIES:
		return (nColWidth / (25 * 12.0));
		
	case WLC_DISPLAY_DECADES:
		return (nColWidth / (10 * 12.0));
		
	case WLC_DISPLAY_YEARS:
		return (nColWidth / 12.0);
		
	case WLC_DISPLAY_QUARTERS_SHORT:
	case WLC_DISPLAY_QUARTERS_MID:
	case WLC_DISPLAY_QUARTERS_LONG:
		return (nColWidth / 3.0);
		
	case WLC_DISPLAY_MONTHS_SHORT:
	case WLC_DISPLAY_MONTHS_MID:
	case WLC_DISPLAY_MONTHS_LONG:
	case WLC_DISPLAY_WEEKS_SHORT:
	case WLC_DISPLAY_WEEKS_MID:
	case WLC_DISPLAY_WEEKS_LONG:
	case WLC_DISPLAY_DAYS_SHORT:
	case WLC_DISPLAY_DAYS_MID:
	case WLC_DISPLAY_DAYS_LONG:
	case WLC_DISPLAY_HOURS:
		return (double)nColWidth;
	}

	ASSERT(0);
	return 0.0;
}

int CWorkloadCtrl::GetRequiredListColumnCount() const
{
	return GetRequiredListColumnCount(m_nMonthDisplay);
}

int CWorkloadCtrl::GetRequiredListColumnCount(WLC_MONTH_DISPLAY nDisplay) const
{
	int nNumMonths = GetNumMonths(nDisplay);
	int nNumCols = 0;

	switch (nDisplay)
	{
	case WLC_DISPLAY_QUARTERCENTURIES:
		nNumCols = (nNumMonths / (25 * 12));
		break;
		
	case WLC_DISPLAY_DECADES:
		nNumCols = (nNumMonths / (10 * 12));
		break;
		
	case WLC_DISPLAY_YEARS:
		nNumCols = (nNumMonths / 12);
		break;
		
	case WLC_DISPLAY_QUARTERS_SHORT:
	case WLC_DISPLAY_QUARTERS_MID:
	case WLC_DISPLAY_QUARTERS_LONG:
		nNumCols = (nNumMonths / 3);
		break;
		
	case WLC_DISPLAY_MONTHS_SHORT:
	case WLC_DISPLAY_MONTHS_MID:
	case WLC_DISPLAY_MONTHS_LONG:
	case WLC_DISPLAY_WEEKS_SHORT:
	case WLC_DISPLAY_WEEKS_MID:
	case WLC_DISPLAY_WEEKS_LONG:
	case WLC_DISPLAY_DAYS_SHORT:
	case WLC_DISPLAY_DAYS_MID:
	case WLC_DISPLAY_DAYS_LONG:
	case WLC_DISPLAY_HOURS:
		nNumCols = nNumMonths;
		break;

	default:
		ASSERT(0);
		break;
	}

	return (nNumCols + 1);
}

int CWorkloadCtrl::GetColumnWidth() const
{
	return GetColumnWidth(m_nMonthDisplay, m_nMonthWidth);
}

int CWorkloadCtrl::GetColumnWidth(WLC_MONTH_DISPLAY nDisplay) const
{
	return GetColumnWidth(nDisplay, m_nMonthWidth);
}

int CWorkloadCtrl::GetNumMonthsPerColumn(WLC_MONTH_DISPLAY nDisplay)
{
	switch (nDisplay)
	{
	case WLC_DISPLAY_QUARTERCENTURIES:
		return (25 * 12);
		
	case WLC_DISPLAY_DECADES:
		return (10 * 12);
		
	case WLC_DISPLAY_YEARS:
		return 12;
		
	case WLC_DISPLAY_QUARTERS_SHORT:
	case WLC_DISPLAY_QUARTERS_MID:
	case WLC_DISPLAY_QUARTERS_LONG:
		return 3;
		
	case WLC_DISPLAY_MONTHS_SHORT:
	case WLC_DISPLAY_MONTHS_MID:
	case WLC_DISPLAY_MONTHS_LONG:
	case WLC_DISPLAY_WEEKS_SHORT:
	case WLC_DISPLAY_WEEKS_MID:
	case WLC_DISPLAY_WEEKS_LONG:
	case WLC_DISPLAY_DAYS_SHORT:
	case WLC_DISPLAY_DAYS_MID:
	case WLC_DISPLAY_DAYS_LONG:
	case WLC_DISPLAY_HOURS:
		return 1;
	}

	// else
	ASSERT(0);
	return 1;
}

int CWorkloadCtrl::GetColumnWidth(WLC_MONTH_DISPLAY nDisplay, int nMonthWidth)
{
	return (GetNumMonthsPerColumn(nDisplay) * nMonthWidth);
}

BOOL CWorkloadCtrl::GetListColumnDate(int nCol, int& nMonth, int& nYear) const
{
	ASSERT (nCol > 0);
	nMonth = nYear = 0;
	
	if (nCol > 0)
	{
		DWORD dwData = m_hdrColumns.GetItemData(nCol);

		nMonth = LOWORD(dwData);
		nYear = HIWORD(dwData);
	}

	return (nMonth >= 1 && nMonth <= 12);
}

void CWorkloadCtrl::BuildTreeColumns()
{
	// delete existing columns
	while (m_hdrTasks.DeleteItem(0));

	// add columns
	m_hdrTasks.InsertItem(0, 0, _T("Task"), (HDF_LEFT | HDF_STRING), 0, WLCC_TITLE);
	m_hdrTasks.EnableItemDragging(0, FALSE);

	for (int nCol = 0; nCol < NUM_TREECOLUMNS; nCol++)
	{
		m_hdrTasks.InsertItem(nCol + 1, 
								0, 
								CEnString(WORKLOADTREECOLUMNS[nCol].nIDColName), 
								(WORKLOADTREECOLUMNS[nCol].nColAlign | HDF_STRING),
								0,
								WORKLOADTREECOLUMNS[nCol].nColID);
	}
}

BOOL CWorkloadCtrl::IsTreeItemLineOdd(HTREEITEM hti) const
{
	int nItem = GetListItem(hti);

	return IsListItemLineOdd(nItem);
}

BOOL CWorkloadCtrl::IsListItemLineOdd(int nItem) const
{
	return ((nItem % 2) == 1);
}

void CWorkloadCtrl::SetFocus()
{
	if (!HasFocus())
		m_tcTasks.SetFocus();
}

void CWorkloadCtrl::Resize()
{
	CRect rect;
	GetBoundingRect(rect);
	
	Resize(rect);
}

void CWorkloadCtrl::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);
	
	if (cx && cy)
	{
		CRect rect(0, 0, cx, cy);
		CTreeListSyncer::Resize(rect);
	}
}

void CWorkloadCtrl::Resize(const CRect& rect)
{
	if (m_hdrTasks.GetItemCount())
	{
		CTreeListSyncer::Resize(rect, GetSplitPos());

		m_tcTasks.SendMessage(WM_GTCN_TITLECOLUMNWIDTHCHANGE, m_hdrTasks.GetItemWidth(0), (LPARAM)m_tcTasks.GetSafeHwnd());
	}
}

void CWorkloadCtrl::ExpandAll(BOOL bExpand)
{
	ExpandItem(NULL, bExpand, TRUE);

	RecalcTreeColumns(TRUE);
}

void CWorkloadCtrl::ExpandItem(HTREEITEM hti, BOOL bExpand, BOOL bAndChildren)
{
	// avoid unnecessary processing
	if (hti && !CanExpandItem(hti, bExpand))
		return;

	CAutoFlag af(m_bTreeExpanding, TRUE);
	EnableResync(FALSE);

	CHoldRedraw hr(m_lcColumns);
	CHoldRedraw hr2(m_tcTasks);

	TCH().ExpandItem(hti, bExpand, bAndChildren);

	if (bExpand)
	{
		if (hti)
		{
			int nNextIndex = (GetListItem(hti) + 1);
			ExpandList(hti, nNextIndex);
		}
		else
			ExpandList(); // all
	}
	else
	{
		CollapseList(hti);
	}
	
	m_tcTasks.EnsureVisible(hti);

	EnableResync(TRUE, m_tcTasks);
	RecalcTreeColumns(TRUE);
}

BOOL CWorkloadCtrl::CanExpandItem(HTREEITEM hti, BOOL bExpand) const
{
	int nFullyExpanded = TCH().IsItemExpanded(hti, TRUE);
			
	if (nFullyExpanded == -1)	// item has no children
	{
		return FALSE; // can neither expand nor collapse
	}
	else if (bExpand)
	{
		return !nFullyExpanded;
	}
			
	// else
	return TCH().IsItemExpanded(hti, FALSE);
}

LRESULT CWorkloadCtrl::OnTreeCustomDraw(NMTVCUSTOMDRAW* pTVCD)
{
	HTREEITEM hti = (HTREEITEM)pTVCD->nmcd.dwItemSpec;
	
	switch (pTVCD->nmcd.dwDrawStage)
	{
	case CDDS_PREPAINT:
		return CDRF_NOTIFYITEMDRAW;
								
	case CDDS_ITEMPREPAINT:
		{
			DWORD dwTaskID = pTVCD->nmcd.lItemlParam;
			WORKLOADITEM* pGI = NULL;

			GET_GI_RET(dwTaskID, pGI, 0L);
				
 			CDC* pDC = CDC::FromHandle(pTVCD->nmcd.hdc);
			CRect rItem(pTVCD->nmcd.rc);

			COLORREF crBack = DrawTreeItemBackground(pDC, hti, *pGI, rItem, rItem, FALSE);
				
			// hide text because we will draw it later
			pTVCD->clrTextBk = pTVCD->clrText = crBack;
		
			return (CDRF_NOTIFYPOSTPAINT | CDRF_NEWFONT); // always
		}
		break;
								
	case CDDS_ITEMPOSTPAINT:
		{
			// check row is visible
			CRect rItem;
			GetTreeItemRect(hti, WLCC_TITLE, rItem);

			CRect rClient;
			m_tcTasks.GetClientRect(rClient);
			
			if ((rItem.bottom > 0) && (rItem.top < rClient.bottom))
			{
				DWORD dwTaskID = pTVCD->nmcd.lItemlParam;
				WORKLOADITEM* pGI = NULL;

				GET_GI_RET(dwTaskID, pGI, 0L);
				
				CDC* pDC = CDC::FromHandle(pTVCD->nmcd.hdc);

				GM_ITEMSTATE nState = GetItemState(hti);
				BOOL bSelected = (nState != GMIS_NONE);

				// draw horz gridline before selection
				DrawItemDivider(pDC, pTVCD->nmcd.rc, DIV_HORZ, bSelected);

				// Draw icon
				if (pGI->bHasIcon || pGI->bParent)
				{
					int iImageIndex = -1;
					HIMAGELIST hilTask = m_tcTasks.GetTaskIcon(pGI->dwTaskID, iImageIndex);

					if (hilTask && (iImageIndex != -1))
					{
						CRect rItem;
						m_tcTasks.GetItemRect(hti, rItem, TRUE);

						CRect rIcon(rItem);
						rIcon.left -= (IMAGE_SIZE + 2);
						rIcon.bottom = (rIcon.top + IMAGE_SIZE);
						GraphicsMisc::CentreRect(rIcon, rItem, FALSE, TRUE);

						ImageList_Draw(hilTask, iImageIndex, *pDC, rIcon.left, rIcon.top, ILD_TRANSPARENT);
					}
				}
				
				// draw background
				COLORREF crBack = DrawTreeItemBackground(pDC, hti, *pGI, rItem, rClient, bSelected);
				
				// draw Workload item attribute columns
				DrawTreeItem(pDC, hti, *pGI, bSelected, crBack);
			}			
	
			return CDRF_SKIPDEFAULT;
		}
		break;
	}

	return CDRF_DODEFAULT;
}

COLORREF CWorkloadCtrl::DrawTreeItemBackground(CDC* pDC, HTREEITEM hti, const WORKLOADITEM& gi, const CRect& rItem, const CRect& rClient, BOOL bSelected)
{
	BOOL bAlternate = (HasAltLineColor() && !IsTreeItemLineOdd(hti));
	COLORREF crBack = GetTreeTextBkColor(gi, bSelected, bAlternate);

	if (!bSelected)
	{
		// redraw item background else tooltips cause overwriting
		CRect rBack(rItem);
		rBack.bottom--;
		rBack.right = rClient.right;

		pDC->FillSolidRect(rBack, crBack);
	}
	else
	{
		DWORD dwFlags = (GMIB_THEMECLASSIC | GMIB_EXTENDRIGHT | GMIB_CLIPRIGHT);
		GraphicsMisc::DrawExplorerItemBkgnd(pDC, m_tcTasks, GetItemState(hti), rItem, dwFlags);
	}

	return crBack;
}

GM_ITEMSTATE CWorkloadCtrl::GetItemState(int nItem) const
{
	if (!m_bSavingToImage)
	{
		if (IsListItemSelected(m_lcColumns, nItem))
		{
			if (HasFocus())
				return GMIS_SELECTED;
			else
				return GMIS_SELECTEDNOTFOCUSED;
		}
		else if (ListItemHasState(m_lcColumns, nItem, LVIS_DROPHILITED))
		{
			return GMIS_DROPHILITED;
		}
	}

	// else
	return GMIS_NONE;
}

GM_ITEMSTATE CWorkloadCtrl::GetItemState(HTREEITEM hti) const
{
	if (!m_bSavingToImage)
	{
		if (IsTreeItemSelected(m_tcTasks, hti))
		{
			if (HasFocus())
				return GMIS_SELECTED;
			else
				return GMIS_SELECTEDNOTFOCUSED;
		}
		else if (TreeItemHasState(m_tcTasks, hti, TVIS_DROPHILITED))
		{
			return GMIS_DROPHILITED;
		}
	}

	// else
	return GMIS_NONE;
}

LRESULT CWorkloadCtrl::OnListCustomDraw(NMLVCUSTOMDRAW* pLVCD)
{
	HWND hwndList = pLVCD->nmcd.hdr.hwndFrom;
	int nItem = (int)pLVCD->nmcd.dwItemSpec;
	
	switch (pLVCD->nmcd.dwDrawStage)
	{
	case CDDS_PREPAINT:
#ifdef _DEBUG
		{
			static int nCount = 1;
			TRACE(_T("\nCWorkloadTreeListCtrl::OnListCustomDraw(begin_%d)\n"), nCount++);
		}
#endif

		return CDRF_NOTIFYITEMDRAW;
								
	case CDDS_ITEMPREPAINT:
		{
			DWORD dwTaskID = GetTaskID(nItem);
			TRACE(_T("CWorkloadTreeListCtrl::OnListCustomDraw(ID = %ld)\n"), dwTaskID);

			WORKLOADITEM* pGI = NULL;
			GET_GI_RET(dwTaskID, pGI, 0L);

			CDC* pDC = CDC::FromHandle(pLVCD->nmcd.hdc);
			
			// draw item bkgnd and gridlines full width of list
			COLORREF crBack = GetRowColor(nItem);
			pLVCD->clrTextBk = pLVCD->clrText = crBack;
			
			CRect rItem;
			VERIFY(GetListItemRect(nItem, rItem));

			CRect rFullWidth(rItem);
			GraphicsMisc::FillItemRect(pDC, rFullWidth, crBack, m_lcColumns);
			
			// draw horz gridline before selection
			DrawItemDivider(pDC, rFullWidth, DIV_HORZ, FALSE);

			// draw background
			GM_ITEMSTATE nState = GetItemState(nItem);
			DWORD dwFlags = (GMIB_THEMECLASSIC | GMIB_CLIPLEFT);

			GraphicsMisc::DrawExplorerItemBkgnd(pDC, m_lcColumns, nState, rItem, dwFlags);

			// draw row
			DrawListItem(pDC, nItem, *pGI, (nState != GMIS_NONE));
		}
		return CDRF_SKIPDEFAULT;
	}

	return CDRF_DODEFAULT;
}

COLORREF CWorkloadCtrl::GetRowColor(int nItem) const
{
	BOOL bAlternate = (!IsListItemLineOdd(nItem) && (m_crAltLine != CLR_NONE));
	COLORREF crBack = (bAlternate ? m_crAltLine : GetSysColor(COLOR_WINDOW));

	return crBack;
}

LRESULT CWorkloadCtrl::OnHeaderCustomDraw(NMCUSTOMDRAW* pNMCD)
{
	if (pNMCD->hdr.hwndFrom == m_hdrColumns)
	{
		switch (pNMCD->dwDrawStage)
		{
		case CDDS_PREPAINT:
			// only need handle drawing for double row height
			if (m_hdrColumns.GetRowCount() > 1)
				return CDRF_NOTIFYITEMDRAW;
							
		case CDDS_ITEMPREPAINT:
			// only need handle drawing for double row height
			if (m_hdrColumns.GetRowCount() > 1)
			{
				CDC* pDC = CDC::FromHandle(pNMCD->hdc);
				int nItem = (int)pNMCD->dwItemSpec;

				DrawListHeaderItem(pDC, nItem);

				return CDRF_SKIPDEFAULT;
			}
			break;
		}
	}
	else if (pNMCD->hdr.hwndFrom == m_hdrTasks)
	{
		switch (pNMCD->dwDrawStage)
		{
		case CDDS_PREPAINT:
			return CDRF_NOTIFYITEMDRAW;
			
		case CDDS_ITEMPREPAINT:
			{
				// don't draw columns having min width
				CRect rItem(pNMCD->rc);
				
				if (rItem.Width() <= MIN_COL_WIDTH)
					return CDRF_DODEFAULT;
			}
			return CDRF_NOTIFYPOSTPAINT;
			
		case CDDS_ITEMPOSTPAINT:
			{
				// draw sort direction
				int nCol = (int)pNMCD->dwItemSpec;
				WLC_COLUMN nColID = GetColumnID(nCol);
				CDC* pDC = CDC::FromHandle(pNMCD->hdc);
				
				if (m_sort.IsSingleSortingBy(nColID))
					m_hdrTasks.DrawItemSortArrow(pDC, nCol, m_sort.single.bAscending);
			}
			break;
		}
	}
	
	return CDRF_DODEFAULT;
}

void CWorkloadCtrl::DrawSplitBar(CDC* pDC, const CRect& rSplitter, COLORREF crSplitBar)
{
	GraphicsMisc::DrawSplitBar(pDC, rSplitter, crSplitBar);
}

void CWorkloadCtrl::OnHeaderDividerDblClk(NMHEADER* pHDN)
{
	int nCol = pHDN->iItem;
	ASSERT(nCol != -1);
	
	HWND hwnd = pHDN->hdr.hwndFrom;

	if (hwnd == m_hdrTasks)
	{
		CClientDC dc(&m_tcTasks);
		RecalcTreeColumnWidth(GetColumnID(nCol), &dc);

		SetSplitPos(m_hdrTasks.CalcTotalItemsWidth());
		
		Resize();
	}
	else if (hwnd == m_hdrColumns)
	{
		if (nCol > 0) // first column always zero width
			m_hdrColumns.SetItemWidth(nCol, GetColumnWidth());
	}
}

// Called by parent
void CWorkloadCtrl::Sort(WLC_COLUMN nBy, BOOL bAllowToggle, BOOL bAscending)
{
	Sort(nBy, bAllowToggle, bAscending, FALSE);
}

void CWorkloadCtrl::Sort(WLC_COLUMN nBy, BOOL bAllowToggle, BOOL bAscending, BOOL bNotifyParent)
{
	m_sort.Sort(nBy, bAllowToggle, bAscending);

	// do the sort
	CHoldRedraw hr(m_tcTasks);
	CTreeListSyncer::Sort(SortProc, (DWORD)this);

	// update sort arrow
	m_hdrTasks.Invalidate(FALSE);

	if (bNotifyParent)
		GetCWnd()->PostMessage(WM_WLCN_SORTCHANGE, 0, m_sort.single.nBy);
}

void CWorkloadCtrl::Sort(const WORKLOADSORTCOLUMNS multi)
{
	m_sort.Sort(multi);

	// do the sort
	CHoldRedraw hr(m_tcTasks);
	CTreeListSyncer::Sort(MultiSortProc, (DWORD)this);

	// hide sort arrow
	m_hdrTasks.Invalidate(FALSE);
}

void CWorkloadCtrl::OnBeginEditTreeLabel(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
	*pResult = TRUE; // cancel our edit
	
	// notify app to edit
	CWnd::GetParent()->SendMessage(WM_WLC_EDITTASKTITLE);
}

void CWorkloadCtrl::OnEndDragTreeHeader(NMHDR* /*pNMHDR*/, LRESULT* /*pResult*/)
{
	m_tcTasks.InvalidateRect(NULL, TRUE);
}

void CWorkloadCtrl::OnClickTreeHeader(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	HD_NOTIFY *pHDN = (HD_NOTIFY *) pNMHDR;
	
	if (pHDN->iButton == 0) // left button
	{
		WLC_COLUMN nColID = GetColumnID(pHDN->iItem);
		Sort(nColID, TRUE, -1, TRUE);
	}
}

void CWorkloadCtrl::OnItemChangingTreeHeader(NMHDR* pNMHDR, LRESULT* pResult)
{
	NMHEADER* pHDN = (NMHEADER*) pNMHDR;
	*pResult = 0;
	
	if (pHDN->iButton == 0) // left button
	{
		if (pHDN->pitem->mask & HDI_WIDTH)
		{
			// don't allow columns get too small
			WLC_COLUMN nColID = GetColumnID(pHDN->iItem);
			
			switch (nColID)
			{
			case WLCC_TITLE:
				if (pHDN->pitem->cxy < TREE_TITLE_MIN_WIDTH)
					*pResult = TRUE; // prevent change
				break;
				
			case WLCC_STARTDATE:
			case WLCC_DUEDATE:
			case WLCC_DONEDATE:
			case WLCC_ALLOCTO:
			case WLCC_PERCENT:
			case WLCC_TASKID:
				if (m_hdrTasks.IsItemVisible(pHDN->iItem))
				{
					if (pHDN->pitem->cxy < MIN_COL_WIDTH)
						pHDN->pitem->cxy = MIN_COL_WIDTH;
				}
				break;
			}
		}
	}
}

void CWorkloadCtrl::OnItemChangedTreeHeader(NMHDR* /*pNMHDR*/, LRESULT* /*pResult*/)
{
	SetSplitPos(m_hdrTasks.CalcTotalItemsWidth());
	Resize();
	
	m_tcTasks.UpdateWindow();
	m_lcColumns.UpdateWindow();
}

void CWorkloadCtrl::OnDblClickTreeHeaderDivider(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	OnHeaderDividerDblClk((NMHEADER*)pNMHDR);
}

void CWorkloadCtrl::OnRightClickTreeHeader(NMHDR* /*pNMHDR*/, LRESULT* /*pResult*/)
{
	// pass on to parent
	CWnd::GetParent()->SendMessage(WM_CONTEXTMENU, (WPARAM)GetSafeHwnd(), (LPARAM)::GetMessagePos());
}

void CWorkloadCtrl::OnTreeKeyUp(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NMTVKEYDOWN* pTVKD = (NMTVKEYDOWN*)pNMHDR;
	
	switch (pTVKD->wVKey)
	{
	case VK_UP:
	case VK_DOWN:
	case VK_PRIOR:
	case VK_NEXT:
		//UpdateSelectedTaskDates();
		//SendParentSelectionUpdate();
		break;
	}
	
	*pResult = 0;
}

void CWorkloadCtrl::OnColumnsClick(NMHDR* /*pNMHDR*/, LRESULT* pResult) 
{
	//UpdateSelectedTaskDates();
	//SendParentSelectionUpdate();
	
	*pResult = 0;
}

void CWorkloadCtrl::OnTreeGetDispInfo(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	TV_DISPINFO* pDispInfo = (TV_DISPINFO*)pNMHDR;
	DWORD dwTaskID = pDispInfo->item.lParam;
	
	const WORKLOADITEM* pGI = NULL;
	GET_GI(dwTaskID, pGI);
	
	if (pDispInfo->item.mask & TVIF_TEXT)
	{
		pDispInfo->item.pszText = (LPTSTR)(LPCTSTR)pGI->sTitle;
	}
	
	if (pDispInfo->item.mask & (TVIF_SELECTEDIMAGE | TVIF_IMAGE))
	{
		// checkbox image
		pDispInfo->item.mask |= TVIF_STATE;
		pDispInfo->item.stateMask = TVIS_STATEIMAGEMASK;
		
		if (pGI->IsDone(FALSE))
		{
			pDispInfo->item.state = TCHC_CHECKED;
		}
		else if (pGI->bSomeSubtaskDone)
		{
			pDispInfo->item.state = TCHC_MIXED;
		}
		else 
		{
			pDispInfo->item.state = TCHC_UNCHECKED;
		}
	}
}

void CWorkloadCtrl::OnTreeItemExpanded(NMHDR* /*pNMHDR*/, LRESULT* /*pResult*/)
{
	RecalcTreeColumns(TRUE);
}

LRESULT CWorkloadCtrl::OnTreeDragEnter(WPARAM /*wp*/, LPARAM /*lp*/)
{
	// Make sure the selection helper is synchronised
	// with the tree's current selection
	m_tshDragDrop.ClearHistory();
	m_tshDragDrop.RemoveAll(TRUE, FALSE);
	m_tshDragDrop.AddItem(m_tcTasks.GetSelectedItem(), FALSE);
	
	return m_treeDragDrop.ProcessMessage(CWnd::GetCurrentMessage());
}

LRESULT CWorkloadCtrl::OnTreePreDragMove(WPARAM /*wp*/, LPARAM /*lp*/)
{
	return m_treeDragDrop.ProcessMessage(CWnd::GetCurrentMessage());
}

LRESULT CWorkloadCtrl::OnTreeDragOver(WPARAM /*wp*/, LPARAM /*lp*/)
{
	// We currently DON'T support 'linking'
	UINT nCursor = m_treeDragDrop.ProcessMessage(CWnd::GetCurrentMessage());
	
	if (nCursor == DD_DROPEFFECT_LINK)
		nCursor = DD_DROPEFFECT_NONE;
	
	return nCursor;
}

LRESULT CWorkloadCtrl::OnTreeDragDrop(WPARAM /*wp*/, LPARAM /*lp*/)
{
	if (m_treeDragDrop.ProcessMessage(CWnd::GetCurrentMessage()))
	{
		HTREEITEM htiDropTarget = NULL, htiAfterSibling = NULL;
		
		if (m_treeDragDrop.GetDropTarget(htiDropTarget, htiAfterSibling))
		{
			// Notify parent of move
			HTREEITEM htiSel = GetSelectedItem();
			ASSERT(htiSel);
			
			IUITASKMOVE move = { 0 };
			
			move.dwSelectedTaskID = GetTaskID(htiSel);
			move.dwParentID = GetTaskID(htiDropTarget);
			move.dwAfterSiblingID = GetTaskID(htiAfterSibling);
			move.bCopy = (Misc::ModKeysArePressed(MKS_CTRL) != FALSE);
			
			// If copying a task, app will send us a full update 
			// so we do not need to perform the move ourselves
			if (CWnd::SendMessage(WM_WLC_MOVETASK, 0, (LPARAM)&move) && !move.bCopy)
			{
				htiSel = TCH().MoveTree(htiSel, htiDropTarget, htiAfterSibling, TRUE, TRUE);
				
				RefreshTreeItemMap();
				SelectItem(htiSel);
			}
		}
	}

	return 0L;
}

LRESULT CWorkloadCtrl::OnTreeDragAbort(WPARAM /*wp*/, LPARAM /*lp*/)
{
	return m_treeDragDrop.ProcessMessage(CWnd::GetCurrentMessage());
}

void CWorkloadCtrl::OnTreeSelectionChange(NMTREEVIEW* pNMTV)
{
	if (m_bMovingTask)
		return;
	
	// Ignore setting selection to 'NULL' unless there are no tasks at all
	// because we know it's temporary only
	if ((pNMTV->itemNew.hItem == NULL) && (m_tcTasks.GetCount() != 0))
		return;
	
	// ignore notifications arising out of SelectTask()
// 	if (/*m_bInSelectTask && */(pNMTV->action == TVC_UNKNOWN))
// 		return;
	
	// we're only interested in non-keyboard changes
	// because keyboard gets handled in OnKeyUpWorkload
	if (pNMTV->action != TVC_BYKEYBOARD)
		CWnd::GetParent()->SendMessage(WM_WLCN_SELCHANGE, 0, GetTaskID(pNMTV->itemNew.hItem));
}

LRESULT CWorkloadCtrl::ScWindowProc(HWND hRealWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	if (!IsResyncEnabled())
		return CTreeListSyncer::ScWindowProc(hRealWnd, msg, wp, lp);

	if (hRealWnd == m_lcColumns)
	{
		switch (msg)
		{
		case WM_TIMER:
			switch (wp)
			{
			case 0x2A:
			case 0x2B:
				// These are timers internal to the list view associated
				// with editing labels and which cause unwanted selection
				// changes. Given that we have disabled label editing for 
				// the attribute columns we can safely kill these timers
				::KillTimer(hRealWnd, wp);
				return TRUE;
			}
			break;
			
		case WM_NOTIFY:
			{
				LPNMHDR pNMHDR = (LPNMHDR)lp;
				HWND hwnd = pNMHDR->hwndFrom;
				
				// let base class have its turn first
				LRESULT lr = CTreeListSyncer::ScWindowProc(hRealWnd, msg, wp, lp);

				switch (pNMHDR->code)
				{
				case NM_RCLICK:
					if (hwnd == m_hdrColumns)
					{
						// pass on to parent
						::SendMessage(GetHwnd(), WM_CONTEXTMENU, (WPARAM)hwnd, (LPARAM)::GetMessagePos());
					}
					break;

				case HDN_DIVIDERDBLCLICK:
					if (hwnd == m_hdrColumns)
					{
						OnHeaderDividerDblClk((NMHEADER*)pNMHDR);
					}
					break;

				case HDN_ITEMCHANGING:
					if (hwnd == m_hdrColumns)
					{
						NMHEADER* pHDN = (NMHEADER*)pNMHDR;
						
						// don't let user drag column too narrow
						if ((pHDN->iButton == 0) && (pHDN->pitem->mask & HDI_WIDTH))
						{
							if (m_hdrColumns.IsItemTrackable(pHDN->iItem) && (pHDN->pitem->cxy < MIN_COL_WIDTH))
								pHDN->pitem->cxy = MIN_COL_WIDTH;

							m_lcColumns.Invalidate(FALSE);
						}
					}
					break;

				}
				return lr;
			}
			break;
			
		case WM_ERASEBKGND:
			if (COSVersion() == OSV_LINUX)
			{
				CRect rClient;
				m_lcColumns.GetClientRect(rClient);
				
				CDC::FromHandle((HDC)wp)->FillSolidRect(rClient, GetSysColor(COLOR_WINDOW));
			}
			return TRUE;

		case WM_SETCURSOR:
			break;

		case WM_LBUTTONDBLCLK:
			if (OnListLButtonDblClk(wp, lp))
			{
				return FALSE; // eat
			}
			break;

		case WM_LBUTTONDOWN:
			if (OnListLButtonDown(wp, lp))
			{
				return FALSE; // eat
			}
			break;

		case WM_LBUTTONUP:
			if (OnListLButtonUp(wp, lp))
			{
				return FALSE; // eat
			}
			break;

		case WM_MOUSEMOVE:
			if (OnListMouseMove(wp, lp))
			{
				return FALSE; // eat
			}
			break;

		case WM_CAPTURECHANGED:
			break;

		case WM_KEYDOWN:
			break;

		case WM_RBUTTONDOWN:
			{
				DWORD dwTaskID = ListHitTestTask(lp, FALSE);

				if (dwTaskID != 0)
					SelectTask(dwTaskID);
			}
			break;

		case WM_SETFOCUS:
			m_tcTasks.SetFocus();
			break;
		}
	}
	else if (hRealWnd == m_tcTasks)
	{
		switch (msg)
		{
		case WM_RBUTTONDOWN:
			{
				DWORD dwTaskID = TreeHitTestTask(lp, FALSE);

				if (dwTaskID)
					SelectTask(dwTaskID);
			}
			break;

		case WM_LBUTTONDOWN:
			if (OnTreeLButtonDown(wp, lp))
			{
				return FALSE; // eat
			}
			break;

		case WM_LBUTTONUP:
			if (OnTreeLButtonUp(wp, lp))
			{
				return FALSE; // eat
			}
			break;

		case WM_LBUTTONDBLCLK:
			if (OnTreeLButtonDblClk(wp, lp))
			{
				return FALSE; // eat
			}
			break;

		case WM_MOUSEMOVE:
			if (OnTreeMouseMove(wp, lp))
			{
				return FALSE; // eat
			}
			break;

		case WM_MOUSELEAVE:
			// Remove any drophilighting from the list
			if (m_nPrevDropHilitedItem != -1)
			{
				m_lcColumns.SetItemState(m_nPrevDropHilitedItem, 0, LVIS_DROPHILITED);
				m_nPrevDropHilitedItem = -1;
			}
			break;

		case WM_MOUSEWHEEL:
			// if we have a horizontal scrollbar but NOT a vertical scrollbar
			// then we need to redraw the whole tree to prevent artifacts
			if (HasHScrollBar(hRealWnd) && !HasVScrollBar(hRealWnd))
			{
				CHoldRedraw hr(hRealWnd, NCR_PAINT | NCR_UPDATE);

				return CTreeListSyncer::ScWindowProc(hRealWnd, msg, wp, lp);
			}
			break;

		case WM_HSCROLL:
			{
				CHoldRedraw hr(hRealWnd, NCR_PAINT | NCR_UPDATE);

				return CTreeListSyncer::ScWindowProc(hRealWnd, msg, wp, lp);
			}
			break;

		case WM_SETCURSOR:
			if (!m_bReadOnly)
			{
				CPoint ptCursor(GetMessagePos());
				DWORD dwTaskID = TreeHitTestTask(ptCursor, TRUE);

				if (dwTaskID && m_data.ItemIsLocked(dwTaskID))
					return GraphicsMisc::SetAppCursor(_T("Locked"), _T("Resources\\Cursors"));
			}
			break;
		}
	}

	// else tree or list
	switch (msg)
	{
	case WM_MOUSEWHEEL:
		{
			int zDelta = GET_WHEEL_DELTA_WPARAM(wp);

			if (zDelta != 0)
			{
				WORD wKeys = LOWORD(wp);
				
				if (wKeys == MK_CONTROL)
				{
					// cache prev value
					WLC_MONTH_DISPLAY nPrevDisplay = m_nMonthDisplay;

					// work out where we are going to scroll to after the zoom
					DWORD dwScrollID = 0;
					COleDateTime dtScroll;

					// centre on the mouse if over the list
					if (hRealWnd == m_lcColumns)
					{
						CPoint pt(::GetMessagePos());
						m_lcColumns.ScreenToClient(&pt);

						GetDateFromScrollPos((pt.x + m_lcColumns.GetScrollPos(SB_HORZ)), dtScroll);
					}
					else // centre on the task beneath the mouse
					{
						dwScrollID = HitTestTask(::GetMessagePos());
					}

					// For reasons I don't understand, the resource context is
					// wrong when handling the mousewheel
					AFX_MANAGE_STATE(AfxGetStaticModuleState());

					// do the zoom
					ZoomIn(zDelta > 0);

					// scroll to area of interest
					if (dwScrollID)
					{
						ScrollToTask(dwScrollID);
					}
					else if (CDateHelper::IsDateSet(dtScroll))
					{
						ScrollTo(dtScroll);
					}
					
					// notify parent
					GetCWnd()->SendMessage(WM_WLCN_ZOOMCHANGE, nPrevDisplay, m_nMonthDisplay);
				}
				else
				{
					CHoldHScroll hhs(m_tcTasks);
					
					return CTreeListSyncer::ScWindowProc(hRealWnd, msg, wp, lp);
				}
			}
		}
		break;

	case WM_KEYUP:
		{
			LRESULT lr = CTreeListSyncer::ScWindowProc(hRealWnd, msg, wp, lp);

			NMTVKEYDOWN tvkd = { 0 };

			tvkd.hdr.hwndFrom = hRealWnd;
			tvkd.hdr.idFrom = ::GetDlgCtrlID(hRealWnd);
			tvkd.hdr.code = TVN_KEYUP;
			tvkd.wVKey = LOWORD(wp);
			tvkd.flags = lp;

			CWnd::SendMessage(WM_NOTIFY, ::GetDlgCtrlID(hRealWnd), (LPARAM)&tvkd);
			return lr;
		}
		
	case WM_VSCROLL:
		{
			CHoldHScroll hhs(m_tcTasks);
			
			return CTreeListSyncer::ScWindowProc(hRealWnd, msg, wp, lp);
		}
		break;
	}
	
	return CTreeListSyncer::ScWindowProc(hRealWnd, msg, wp, lp);
}

void CWorkloadCtrl::SetDropHilite(HTREEITEM hti, int nItem)
{
	if (m_nPrevDropHilitedItem != -1)
		m_lcColumns.SetItemState(m_nPrevDropHilitedItem, 0, LVIS_DROPHILITED);
	
	m_tcTasks.SelectDropTarget(hti);
	
	if (nItem != -1)
		m_lcColumns.SetItemState(nItem, LVIS_DROPHILITED, LVIS_DROPHILITED);
	
	m_nPrevDropHilitedItem = nItem;
}

BOOL CWorkloadCtrl::OnTreeMouseMove(UINT /*nFlags*/, CPoint /*point*/)
{
	if (!m_bReadOnly)
	{
	}

	// not handled
	return FALSE;
}

BOOL CWorkloadCtrl::OnTreeLButtonDown(UINT nFlags, CPoint point)
{
	HTREEITEM hti = m_tcTasks.HitTest(point, &nFlags);
	
	// Don't process if expanding an item
	if (nFlags & TVHT_ONITEMBUTTON)
		return FALSE;

	if (!m_bReadOnly)
	{
	}

	if (!(nFlags & TVHT_ONITEMBUTTON))
	{
		if (hti && (hti != GetTreeSelItem(m_tcTasks)))
		{
			SelectTreeItem(m_tcTasks, hti);
			return TRUE;
		}
	}

	// not handled
	return FALSE;
}

BOOL CWorkloadCtrl::OnTreeLButtonUp(UINT nFlags, CPoint point)
{
	HTREEITEM hti = m_tcTasks.HitTest(point, &nFlags);

	if (!m_bReadOnly && (nFlags & TVHT_ONITEMSTATEICON))
	{
		DWORD dwTaskID = GetTaskID(hti);
		const WORKLOADITEM* pGI = m_data.GetItem(dwTaskID);
		ASSERT(pGI);
		
		if (pGI)
			GetCWnd()->SendMessage(WM_WLCN_COMPLETIONCHANGE, (WPARAM)m_tcTasks.GetSafeHwnd(), !pGI->IsDone(FALSE));
		
		return TRUE; // eat
	}

	// not handled
	return FALSE;
}

BOOL CWorkloadCtrl::OnTreeLButtonDblClk(UINT nFlags, CPoint point)
{
	HTREEITEM hti = m_tcTasks.HitTest(point, &nFlags);
				
	if (!(nFlags & (TVHT_ONITEM | TVHT_ONITEMRIGHT)))
		return FALSE;
	
	if (!TCH().TreeCtrl().ItemHasChildren(hti))
	{
		m_tcTasks.EditLabel(hti);
		return TRUE;
	}
	else
	{
		ExpandItem(hti, !TCH().IsItemExpanded(hti), TRUE);
		return TRUE;
	}

	// not handled
	return FALSE;
}

BOOL CWorkloadCtrl::OnListMouseMove(UINT /*nFlags*/, CPoint /*point*/)
{
	if (!m_bReadOnly)
	{
	}

	// not handled
	return FALSE;
}

BOOL CWorkloadCtrl::OnListLButtonDown(UINT /*nFlags*/, CPoint point)
{
	if (!m_bReadOnly)
	{
	}
	
	// don't let the selection to be set to -1
	{
		CPoint ptScreen(point);
		m_lcColumns.ClientToScreen(&ptScreen);
		
		if (HitTestTask(ptScreen) == 0)
		{
			SetFocus();
			return TRUE; // eat
		}
	}

	// not handled
	return FALSE;
}

BOOL CWorkloadCtrl::OnListLButtonUp(UINT /*nFlags*/, CPoint /*point*/)
{
	// not handled
	return FALSE;
}

BOOL CWorkloadCtrl::OnListLButtonDblClk(UINT /*nFlags*/, CPoint point)
{
	int nHit = m_lcColumns.HitTest(point);
	
	if (nHit == -1)
		return FALSE;

	HTREEITEM hti = CTreeListSyncer::GetTreeItem(m_tcTasks, m_lcColumns, nHit);
	ASSERT(hti == GetTreeSelItem(m_tcTasks));

	if (TCH().TreeCtrl().ItemHasChildren(hti))
	{
		ExpandItem(hti, !TCH().IsItemExpanded(hti));
		return TRUE;
	}

	// not handled
	return FALSE;
}

BOOL CWorkloadCtrl::GetLabelEditRect(LPRECT pEdit) const
{
	HTREEITEM htiSel = GetSelectedItem();
	
	// scroll into view first
	//m_tcTasks.EnsureVisible(htiSel);
	
	if (m_tcTasks.GetItemRect(htiSel, pEdit, TRUE)) // label only
	{
		// make width of tree column or 200 whichever is larger
		int nWidth = (m_hdrTasks.GetItemWidth(0) - pEdit->left);
		nWidth = max(nWidth, MIN_LABEL_EDIT_WIDTH);

		pEdit->right = (pEdit->left + nWidth);

		// convert from tree to 'our' coords
		m_tcTasks.ClientToScreen(pEdit);
		CWnd::ScreenToClient(pEdit);

		return true;
	}
	
	return false;
}


void CWorkloadCtrl::SetAlternateLineColor(COLORREF crAltLine)
{
	SetColor(m_crAltLine, crAltLine);
}

void CWorkloadCtrl::SetGridLineColor(COLORREF crGridLine)
{
	SetColor(m_crGridLine, crGridLine);
}

void CWorkloadCtrl::SetTodayColor(COLORREF crToday)
{
	SetColor(m_crToday, crToday);
}

void CWorkloadCtrl::SetWeekendColor(COLORREF crWeekend)
{
	SetColor(m_crWeekend, crWeekend);
}

void CWorkloadCtrl::SetNonWorkingHoursColor(COLORREF crNonWorkingHoursColor)
{
	SetColor(m_crNonWorkingHoursColor, crNonWorkingHoursColor);
}

void CWorkloadCtrl::SetDefaultColor(COLORREF crDefault)
{
	SetColor(m_crDefault, crDefault);
}

void CWorkloadCtrl::SetSplitBarColor(COLORREF crSplitBar) 
{ 
	CTreeListSyncer::SetSplitBarColor(crSplitBar); 
}

void CWorkloadCtrl::SetMilestoneTag(const CString& sTag)
{
	if (sTag != m_sMilestoneTag)
	{
		m_sMilestoneTag = sTag;

		if (IsHooked())
			InvalidateAll();
	}
}

void CWorkloadCtrl::SetColor(COLORREF& color, COLORREF crNew)
{
	if (IsHooked() && (crNew != color))
		InvalidateAll();

	color = crNew;
}

CString CWorkloadCtrl::FormatDate(const COleDateTime& date, DWORD dwFlags) const
{
	dwFlags &= ~DHFD_ISO;
	dwFlags |= (HasOption(WLCF_DISPLAYISODATES) ? DHFD_ISO : 0);

	return CDateHelper::FormatDate(date, dwFlags);
}

CString CWorkloadCtrl::GetTreeItemColumnText(const WORKLOADITEM& gi, WLC_COLUMN nCol) const
{
	CString sItem;

	switch (nCol)
	{
		case WLCC_TITLE:
			sItem = gi.sTitle;
			break;

		case WLCC_TASKID:
			sItem.Format(_T("%ld"), gi.dwTaskID);
			break;
			
		case WLCC_STARTDATE:
		case WLCC_DUEDATE:
			{
				COleDateTime dtStart, dtDue;
				GetTaskStartDueDates(gi, dtStart, dtDue);

				sItem = FormatDate((nCol == WLCC_STARTDATE) ? dtStart : dtDue);
			}
			break;

		case WLCC_DONEDATE:
			if (CDateHelper::IsDateSet(gi.dtDone))
				sItem = FormatDate(gi.dtDone);
			break;

		case WLCC_ALLOCTO:
			sItem = gi.sAllocTo;
			break;
			
		case WLCC_PERCENT:
			sItem.Format(_T("%d%%"), gi.nPercent);
			break;
	}

	return sItem;
}

void CWorkloadCtrl::DrawTreeItem(CDC* pDC, HTREEITEM hti, const WORKLOADITEM& gi, BOOL bSelected, COLORREF crBack)
{
	int nNumCol = m_hdrTasks.GetItemCount();

	for (int nCol = 0; nCol < nNumCol; nCol++)
		DrawTreeItemText(pDC, hti, nCol, gi, bSelected, crBack);
}

void CWorkloadCtrl::DrawTreeItemText(CDC* pDC, HTREEITEM hti, int nCol, const WORKLOADITEM& gi, BOOL bSelected, COLORREF crBack)
{
	CRect rItem;
	GetTreeItemRect(hti, nCol, rItem);

	if (rItem.Width() == 0)
		return;

	DrawItemDivider(pDC, rItem, DIV_VERT_LIGHT, bSelected);

	WLC_COLUMN nColID = GetColumnID(nCol);
	BOOL bTitleCol = (nColID == WLCC_TITLE);

	// draw item background colour
	if (!bSelected && (crBack != CLR_NONE))
	{
		CRect rBack(rItem);

		if (bTitleCol)
			GetTreeItemRect(hti, WLCC_TITLE, rBack, TRUE); // label only
		
		// don't overwrite gridlines
		if (m_crGridLine != CLR_NONE)
			rBack.DeflateRect(0, 0, 1, 1);
		
		pDC->FillSolidRect(rBack, crBack);
	}
	
	if (rItem.Width() <= MIN_COL_WIDTH)
		return;

	// draw text
	CString sItem = GetTreeItemColumnText(gi, nColID);

	if (!sItem.IsEmpty())
	{
		if (bTitleCol)
			rItem.DeflateRect(2, 2, 1, 0);
		else
			rItem.DeflateRect(LV_COLPADDING, 2, LV_COLPADDING, 0);

		// text color and alignment
		BOOL bLighter = FALSE; 
		UINT nFlags = (DT_LEFT | DT_VCENTER | DT_NOPREFIX | GraphicsMisc::GetRTLDrawTextFlags(m_tcTasks));

		switch (nColID)
		{
		case WLCC_TITLE:
			nFlags |= DT_END_ELLIPSIS;
			break;

		case  WLCC_TASKID:
			nFlags |= DT_RIGHT;
			break;
			
		case WLCC_STARTDATE:
		case WLCC_DUEDATE:
		case WLCC_DONEDATE:
			{
				// draw non-selected calculated dates lighter
				if (!bSelected && !gi.IsDone(TRUE))
				{
					if (!bLighter)
					{
						if (nColID == WLCC_STARTDATE)
							bLighter = (!gi.HasStart() && HasOption(WLCF_CALCMISSINGSTARTDATES));
						else
							bLighter = (!gi.HasDue() && HasOption(WLCF_CALCMISSINGDUEDATES));
					}
				}
				
				// Right-align if the column width can show the entire date
				// else keep left align to ensure day and month remain visible
				if (rItem.Width() >= pDC->GetTextExtent(sItem).cx)
					nFlags |= DT_RIGHT;
			}
			break;
			
		case WLCC_PERCENT:
			nFlags |= DT_CENTER;
			break;
		}

		COLORREF crText = GetTreeTextColor(gi, bSelected, bLighter);
		COLORREF crOldColor = pDC->SetTextColor(crText);
		HGDIOBJ hFontOld = pDC->SelectObject(GetTreeItemFont(hti, gi, nColID));
		
		pDC->SetBkMode(TRANSPARENT);
		pDC->DrawText(sItem, rItem, nFlags);
		pDC->SetTextColor(crOldColor);
		pDC->SelectObject(hFontOld);
	}

	// special case: drawing shortcut icon for reference tasks
	if (bTitleCol && gi.dwOrgRefID)
	{
		GetTreeItemRect(hti, nCol, rItem, TRUE);
		CPoint ptIcon(rItem.left, rItem.bottom - 32);

		ShellIcons::DrawIcon(pDC, ShellIcons::SI_SHORTCUT, ptIcon, true);
	}
}

CWorkloadCtrl::DIV_TYPE CWorkloadCtrl::GetVerticalDivider(int nMonth, int nYear) const
{
	switch (m_nMonthDisplay)
	{
	case WLC_DISPLAY_QUARTERCENTURIES:
		{
			if (nMonth == 12)
			{
				if (nYear == (GetEndYear(m_nMonthDisplay)))
					return DIV_VERT_DARK;

				return DIV_VERT_MID;
			}
			else if ((nMonth % 3) == 0)
			{
				return DIV_VERT_LIGHT;
			}

			return DIV_NONE;
		}
		break;

	case WLC_DISPLAY_DECADES:
	case WLC_DISPLAY_YEARS:
		{
			if (nMonth == 12)
			{
				if (nYear == (GetEndYear(m_nMonthDisplay)))
					return DIV_VERT_DARK;

				return DIV_VERT_MID;
			}

			// else
			return DIV_VERT_LIGHT;
		}
		break;

	case WLC_DISPLAY_QUARTERS_SHORT:
	case WLC_DISPLAY_QUARTERS_MID:
	case WLC_DISPLAY_QUARTERS_LONG:
		{
			if (nMonth == 12)
				return DIV_VERT_DARK;
			
			if ((nMonth % 3) == 0)
				return DIV_VERT_MID;

			// else
			return DIV_VERT_LIGHT;
		}
		break;

	case WLC_DISPLAY_MONTHS_SHORT:
	case WLC_DISPLAY_MONTHS_MID:
	case WLC_DISPLAY_MONTHS_LONG:
		{
			if (nMonth == 12)
				return DIV_VERT_DARK;
	
			if (nMonth == 6)
				return DIV_VERT_MID;
		
			// else
			return DIV_VERT_LIGHT;
		}
		break;

	case WLC_DISPLAY_WEEKS_SHORT:
	case WLC_DISPLAY_WEEKS_MID:
	case WLC_DISPLAY_WEEKS_LONG:
	case WLC_DISPLAY_DAYS_SHORT:
	case WLC_DISPLAY_DAYS_MID:
	case WLC_DISPLAY_DAYS_LONG:
		{
			if (nMonth == 12)
				return DIV_VERT_DARK;
			
			// else
			return DIV_VERT_MID;
		}
		break;
		
	case WLC_DISPLAY_HOURS:
		return DIV_VERT_DARK;
	}

	ASSERT(0);
	return DIV_NONE;
}

BOOL CWorkloadCtrl::IsVerticalDivider(DIV_TYPE nType)
{
	switch (nType)
	{
	case DIV_VERT_LIGHT:
	case DIV_VERT_MID:
	case DIV_VERT_DARK:
		return TRUE;
	}

	// else
	return FALSE;
}

void CWorkloadCtrl::DrawItemDivider(CDC* pDC, const CRect& rItem, DIV_TYPE nType, BOOL bSelected)
{
	if (!HasGridlines() || (nType == DIV_NONE) || (IsVerticalDivider(nType) && (rItem.right < 0)))
		return;

	COLORREF color = m_crGridLine;

	switch (nType)
	{
	case DIV_VERT_LIGHT:
	case DIV_HORZ:
		break;
		
	case DIV_VERT_MID:
		color = GraphicsMisc::Darker(m_crGridLine, 0.25);
		break;
		
	case DIV_VERT_DARK:
		color = GraphicsMisc::Darker(m_crGridLine, 0.5);
		break;
	}

	CRect rDiv(rItem);

	if (nType == DIV_HORZ)
	{
		rDiv.top = (rDiv.bottom - 1);
	}
	else
	{
		rDiv.left = (rDiv.right - 1);
		
		if (bSelected)
			rDiv.DeflateRect(0, 1);
	}

	COLORREF crOld = pDC->GetBkColor();

	pDC->FillSolidRect(rDiv, color);
	pDC->SetBkColor(crOld);
}

CString CWorkloadCtrl::GetLongestVisibleAllocTo(HTREEITEM hti) const
{
	CString sLongest;

	if (hti)
	{
		DWORD dwTaskID = GetTaskID(hti);

		const WORKLOADITEM* pGI = NULL;
		GET_GI_RET(dwTaskID, pGI, _T(""));

		sLongest = GetTreeItemColumnText(*pGI, WLCC_ALLOCTO);
	}

	// children
	if (!hti || TCH().IsItemExpanded(hti))
	{
		HTREEITEM htiChild = m_tcTasks.GetChildItem(hti);
		
		while (htiChild)
		{
			CString sLongestChild = GetLongestVisibleAllocTo(htiChild);
			
			if (sLongestChild.GetLength() > sLongest.GetLength())
				sLongest = sLongestChild;
			
			htiChild = m_tcTasks.GetNextItem(htiChild, TVGN_NEXT);
		}
	}
	
	return sLongest;
}

HFONT CWorkloadCtrl::GetTreeItemFont(HTREEITEM hti, const WORKLOADITEM& gi, WLC_COLUMN nCol)
{
	BOOL bStrikThru = (HasOption(WLCF_STRIKETHRUDONETASKS) && gi.IsDone(FALSE));
	BOOL bBold = ((nCol == WLCC_TITLE) && (m_tcTasks.GetParentItem(hti) == NULL));

	return m_tcTasks.Fonts().GetHFont(bBold, FALSE, FALSE, bStrikThru);
}

void CWorkloadCtrl::GetTreeItemRect(HTREEITEM hti, int nCol, CRect& rItem, BOOL bText) const
{
	rItem.SetRectEmpty();

	if (m_tcTasks.GetItemRect(hti, rItem, TRUE)) // text rect only
	{
		WLC_COLUMN nColID = GetColumnID(nCol);

		switch (nColID)
		{
		case WLCC_TITLE:
			{
				int nColWidth = m_hdrTasks.GetItemWidth(0); // always
	
				if (!bText)
					rItem.right = nColWidth;
				else
					rItem.right = min(rItem.right, nColWidth);
			}
			break;

		case WLCC_TASKID:
		case WLCC_STARTDATE:
		case WLCC_DUEDATE:
		case WLCC_DONEDATE:
		case WLCC_ALLOCTO:
		case WLCC_PERCENT:
			{
				CRect rHdrItem;
				m_hdrTasks.GetItemRect(nCol, rHdrItem);
				
				rItem.left = rHdrItem.left;
				rItem.right = rHdrItem.right;
			}
			break;

		case WLCC_NONE:
		default:
			ASSERT(0);
			break;
		}
	}

	if (m_bSavingToImage)
		rItem.OffsetRect(-1, 0);
}

void CWorkloadCtrl::DrawListItemYears(CDC* pDC, const CRect& rItem, 
											int nYear, int nNumYears, const WORKLOADITEM& gi,
											BOOL bSelected)
{
	double dYearWidth = (rItem.Width() / (double)nNumYears);
	CRect rYear(rItem);

	for (int j = 0; j < nNumYears; j++)
	{
		if (j == (nNumYears - 1))
			rYear.right = rItem.right;
		else
			rYear.right = (rItem.left + (int)(dYearWidth * (j + 1)));

		DrawListItemYear(pDC, rYear, (nYear + j), gi, bSelected);

		// next year
		rYear.left = rYear.right; 
	}
}

void CWorkloadCtrl::DrawListItemYear(CDC* pDC, const CRect& rYear, int nYear, 
											const WORKLOADITEM& gi, BOOL bSelected)
{
	DrawListItemMonths(pDC, rYear, 1, 12, nYear, gi, bSelected);
}

void CWorkloadCtrl::DrawListItemMonths(CDC* pDC, const CRect& rItem, 
											int nMonth, int nNumMonths, int nYear, 
											const WORKLOADITEM& gi, BOOL bSelected)
{
	double dMonthWidth = (rItem.Width() / (double)nNumMonths);
	CRect rMonth(rItem);

	for (int i = 0; i < nNumMonths; i++)
	{
		if ((nMonth + i) == 12)
			rMonth.right = rItem.right;
		else
			rMonth.right = (rItem.left + (int)(dMonthWidth * (i + 1)));

		DrawListItemMonth(pDC, rMonth, (nMonth + i), nYear, gi, bSelected);

		// next item
		rMonth.left = rMonth.right; 
	}
}

void CWorkloadCtrl::DrawListItemMonth(CDC* pDC, const CRect& rMonth, 
											int nMonth, int nYear, 
											const WORKLOADITEM& /*gi*/, BOOL bSelected)
{
	DIV_TYPE nDiv = GetVerticalDivider(nMonth, nYear);
	DrawItemDivider(pDC, rMonth, nDiv, bSelected);
}

void CWorkloadCtrl::DrawListItemWeeks(CDC* pDC, const CRect& rMonth, 
											int nMonth, int nYear, 
											const WORKLOADITEM& gi, BOOL bSelected)
{
	// draw vertical week dividers
	int nNumDays = CDateHelper::GetDaysInMonth(nMonth, nYear);
	double dMonthWidth = rMonth.Width();

	int nFirstDOW = CDateHelper::GetFirstDayOfWeek();
	CRect rDay(rMonth);

	COleDateTime dtDay = COleDateTime(nYear, nMonth, 1, 0, 0, 0);

	for (int nDay = 1; nDay <= nNumDays; nDay++)
	{
		rDay.left = rMonth.left + (int)(((nDay - 1) * dMonthWidth) / nNumDays);
		rDay.right = rMonth.left + (int)((nDay * dMonthWidth) / nNumDays);

		// draw divider
		if ((dtDay.GetDayOfWeek() == nFirstDOW) && (nDay > 1))
		{
			rDay.right = rDay.left; // draw at start of day
			DrawItemDivider(pDC, rDay, DIV_VERT_LIGHT, bSelected);
		}

		// next day
		dtDay += 1;
	}

	DrawListItemMonth(pDC, rMonth, nMonth, nYear, gi, bSelected);
}

void CWorkloadCtrl::DrawListItemDays(CDC* pDC, const CRect& rMonth, 
											int nMonth, int nYear, const WORKLOADITEM& gi, 
											BOOL bSelected, BOOL bDrawHours)
{
	// draw vertical day dividers
	CRect rDay(rMonth);
	COleDateTime dtDay = COleDateTime(nYear, nMonth, 1, 0, 0, 0);
	
	int nNumDays = CDateHelper::GetDaysInMonth(nMonth, nYear);
	double dDayWidth = (rMonth.Width() / (double)nNumDays);
	
	for (int nDay = 1; nDay <= nNumDays; nDay++)
	{
		rDay.right = (rMonth.left + (int)(nDay * dDayWidth));
		
		// only draw visible days
		if (rDay.right > 0)
		{
			BOOL bDrawNonWorkingHours = (!bSelected && (m_crNonWorkingHoursColor != CLR_NONE));
			
			if (bDrawHours)
			{
				CRect rHour(rDay);
				double dHourWidth = (rMonth.Width() / (nNumDays * 24.0));
				
				int nStartHour = -1, nEndHour = -1;
				
				if (bDrawNonWorkingHours)
				{
					CTimeHelper th;
					
					nStartHour = (int)th.GetStartOfWorkday(FALSE);
					nEndHour = (int)(th.GetEndOfWorkday(FALSE) + 0.5);
					
					if (m_crGridLine != CLR_NONE)
						rHour.bottom--;
				}
				
				// draw all but the first and last hours dividers
				for (int nHour = 1; nHour <= 24; nHour++)
				{
					rHour.right = (rMonth.left + (int)((dDayWidth * (nDay - 1)) + (dHourWidth * nHour)));
					
					if (bDrawNonWorkingHours)
					{
						if ((nHour < (nStartHour + 1)) || (nHour > nEndHour))
							pDC->FillSolidRect(rHour, m_crNonWorkingHoursColor);
					}
					
					if (nHour != 24)
						DrawItemDivider(pDC, rHour, DIV_VERT_LIGHT, bSelected);
					
					rHour.left = rHour.right;
				}
			}
			
			// draw all but the last day divider
			if (nDay < nNumDays)
				DrawItemDivider(pDC, rDay, (bDrawHours ? DIV_VERT_MID : DIV_VERT_LIGHT), bSelected);
		}
		
		// next day
		dtDay.m_dt += 1;
		rDay.left = rDay.right;
	}

	DrawListItemMonth(pDC, rMonth, nMonth, nYear, gi, bSelected);
}

void CWorkloadCtrl::DrawListItem(CDC* pDC, int nItem, const WORKLOADITEM& gi, BOOL bSelected)
{
	ASSERT(nItem != -1);
	int nNumCol = GetRequiredListColumnCount();

	BOOL bContinue = TRUE;

	for (int nCol = 1; ((nCol <= nNumCol) && bContinue); nCol++)
	{
		bContinue = DrawListItemColumn(pDC, nItem, nCol, gi, bSelected);
	}
}

void CWorkloadCtrl::DrawListItemText(CDC* /*pDC*/, const WORKLOADITEM& /*gi*/, const CRect& /*rItem*/, const CRect& /*rClip*/, COLORREF /*crRow*/)
{
}

BOOL CWorkloadCtrl::DrawListItemColumn(CDC* pDC, int nItem, int nCol, const WORKLOADITEM& gi, BOOL bSelected)
{
	if (nCol == 0)
		return TRUE;

	if (m_hdrColumns.GetItemWidth(nCol) == 0)
		return TRUE;

	// see if we can avoid drawing this sub-item at all
	CRect rColumn;
	m_lcColumns.GetSubItemRect(nItem, nCol, LVIR_BOUNDS, rColumn);

	CRect rClip;
	pDC->GetClipBox(rClip);

	if (rColumn.right < rClip.left)
		return TRUE;
	
	if (rColumn.left > rClip.right)
		return FALSE; // we can stop

	return DrawListItemColumnRect(pDC, nCol, rColumn, gi, bSelected);
}

BOOL CWorkloadCtrl::DrawListItemColumnRect(CDC* pDC, int nCol, const CRect& rColumn, const WORKLOADITEM& gi, BOOL bSelected)
{
	// get the date range for this column
	int nMonth = 0, nYear = 0;
	
	if (!GetListColumnDate(nCol, nMonth, nYear))
		return FALSE;

	int nSaveDC = pDC->SaveDC();

	double dMonthWidth = GetMonthWidth(rColumn.Width());
	BOOL bToday = FALSE;

	// Use higher resolution where possible
	WLC_MONTH_DISPLAY nCalcDisplay = GetColumnDisplay((int)dMonthWidth);
	BOOL bUseHigherRes = (CompareDisplays(nCalcDisplay, m_nMonthDisplay) > 0);

	switch (m_nMonthDisplay)
	{
	case WLC_DISPLAY_QUARTERCENTURIES:
		DrawListItemYears(pDC, rColumn, nYear, 25, gi, bSelected);
		break;
		
	case WLC_DISPLAY_DECADES:
		DrawListItemYears(pDC, rColumn, nYear, 10, gi, bSelected);
		break;
		
	case WLC_DISPLAY_YEARS:
		DrawListItemYear(pDC, rColumn, nYear, gi, bSelected);
		break;
		
	case WLC_DISPLAY_QUARTERS_SHORT:
	case WLC_DISPLAY_QUARTERS_MID:
	case WLC_DISPLAY_QUARTERS_LONG:
		DrawListItemMonths(pDC, rColumn, nMonth, 3, nYear, gi, bSelected);
		break;
		
	case WLC_DISPLAY_MONTHS_SHORT:
	case WLC_DISPLAY_MONTHS_MID:
	case WLC_DISPLAY_MONTHS_LONG:
		if (bUseHigherRes)
			DrawListItemWeeks(pDC, rColumn, nMonth, nYear, gi, bSelected);
		else
			DrawListItemMonth(pDC, rColumn, nMonth, nYear, gi, bSelected);
		break;
		
	case WLC_DISPLAY_WEEKS_SHORT:
	case WLC_DISPLAY_WEEKS_MID:
	case WLC_DISPLAY_WEEKS_LONG:
		if (bUseHigherRes)
			DrawListItemDays(pDC, rColumn, nMonth, nYear, gi, bSelected, FALSE);
		else
			DrawListItemWeeks(pDC, rColumn, nMonth, nYear, gi, bSelected);
		break;

	case WLC_DISPLAY_DAYS_SHORT:
	case WLC_DISPLAY_DAYS_MID:
	case WLC_DISPLAY_DAYS_LONG:
		DrawListItemDays(pDC, rColumn, nMonth, nYear, gi, bSelected, bUseHigherRes);
		break;

	case WLC_DISPLAY_HOURS:
		DrawListItemDays(pDC, rColumn, nMonth, nYear, gi, bSelected, TRUE);
		break;
	
	default:
		ASSERT(0);
		break;
	}

	pDC->RestoreDC(nSaveDC);

	return TRUE;
}

void CWorkloadCtrl::DrawListHeaderItem(CDC* pDC, int nCol)
{
	CRect rItem;
	m_hdrColumns.GetItemRect(nCol, rItem);

	if (nCol == 0)
		return;

	// get the date range for this column
	int nMonth = 0, nYear = 0;
	
	if (!GetListColumnDate(nCol, nMonth, nYear))
		return;

	int nSaveDC = pDC->SaveDC();
	double dMonthWidth = GetMonthWidth(rItem.Width());
	CFont* pOldFont = GraphicsMisc::PrepareDCFont(pDC, m_hdrColumns);
	
	CThemed th;
	BOOL bThemed = (th.AreControlsThemed() && th.Open(GetCWnd(), _T("HEADER")));
	CThemed* pThemed = (bThemed ? &th :NULL);

	CRect rClip;
	pDC->GetClipBox(rClip);

	switch (m_nMonthDisplay)
	{
	case WLC_DISPLAY_YEARS:
	case WLC_DISPLAY_QUARTERS_SHORT:
	case WLC_DISPLAY_QUARTERS_MID:
	case WLC_DISPLAY_QUARTERS_LONG:
	case WLC_DISPLAY_MONTHS_SHORT:
	case WLC_DISPLAY_MONTHS_MID:
	case WLC_DISPLAY_MONTHS_LONG:
		// should never get here
		ASSERT(0);
		break;

	case WLC_DISPLAY_QUARTERCENTURIES:
	case WLC_DISPLAY_DECADES:
		{
			CRect rRange(rItem), rYear(rItem);
			rYear.top += (rYear.Height() / 2);

			// draw range header
			rRange.bottom = rYear.top;
			DrawListHeaderRect(pDC, rRange, m_hdrColumns.GetItemText(nCol), pThemed);

			// draw year elements
			int nNumYears = ((m_nMonthDisplay == WLC_DISPLAY_DECADES) ? 10 : 25);
			double dYearWidth = (rRange.Width() / (double)nNumYears);

			rYear.right = rYear.left;

			for (int i = 0; i < nNumYears; i++)
			{
				rYear.left = rYear.right;
				rYear.right = rRange.left + (int)((i + 1) * dYearWidth);

				// check if we can stop
				if (rYear.left > rClip.right)
					break; 

				// check if we need to draw
				if (rYear.right >= rClip.left)
					DrawListHeaderRect(pDC, rYear, Misc::Format(nYear + i), pThemed);
			}
		}
		break;
		
	case WLC_DISPLAY_WEEKS_SHORT:
	case WLC_DISPLAY_WEEKS_MID:
	case WLC_DISPLAY_WEEKS_LONG:
		{
			CRect rMonth(rItem), rWeek(rItem);
			rWeek.top += (rWeek.Height() / 2);

			// draw month header
			rMonth.bottom = rWeek.top;
			DrawListHeaderRect(pDC, rMonth, m_hdrColumns.GetItemText(nCol), pThemed);

			// draw week elements
			int nNumDays = CDateHelper::GetDaysInMonth(nMonth, nYear);
			double dDayWidth = (rMonth.Width() / (double)nNumDays);

			// first week starts at 'First DOW of month'
			int nFirstDOW = CDateHelper::GetFirstDayOfWeek();
			int nDay = CDateHelper::CalcDayOfMonth(nFirstDOW, 1, nMonth, nYear);

			// If this is column 1 (column 0 is hidden) then we might need
			// to draw part of the preceding week
			if ((nCol == 1) && (nDay != -1))
			{
				rWeek.right = (rWeek.left + (int)((nDay - 1) * dDayWidth));
				DrawListHeaderRect(pDC, rWeek, _T(""), pThemed);
			}

			// calc number of first week
			COleDateTime dtWeek(nYear, nMonth, nDay, 0, 0, 0);
			int nWeek = CDateHelper::GetWeekofYear(dtWeek);
			BOOL bDone = FALSE;

			while (!bDone)
			{
				rWeek.left = rMonth.left + (int)((nDay - 1) * dDayWidth);

				// if this week bridges into next month this needs special handling
				if ((nDay + 6) > nNumDays)
				{
					// rest of this month
					rWeek.right = rMonth.right;
					
					// plus some of next month
					nDay += (6 - nNumDays);
					nMonth++;
					
					if (nMonth > 12)
					{
						nMonth = 1;
						nYear++;
					}
					
					if (m_hdrColumns.GetItemRect(nCol+1, rMonth))
					{
						nNumDays = CDateHelper::GetDaysInMonth(nMonth, nYear);
						dDayWidth = (rMonth.Width() / (double)nNumDays);

						rWeek.right += (int)(nDay * dDayWidth);
					}

					// if this is week 53, check that its not really week 1 of the next year
					if (nWeek == 53)
					{
						ASSERT(nMonth == 1);

						COleDateTime dtWeek(nYear, nMonth, nDay, 0, 0, 0);
						nWeek = CDateHelper::GetWeekofYear(dtWeek);
					}

					bDone = TRUE;
				}
				else 
				{
					rWeek.right = rMonth.left + (int)((nDay + 6) * dDayWidth);
				}

				// check if we can stop
				if (rWeek.left > rClip.right)
					break; 

				// check if we need to draw
				if (rWeek.right >= rClip.left)
					DrawListHeaderRect(pDC, rWeek, Misc::Format(nWeek), pThemed);

				// next week
				nDay += 7;
				nWeek++;

				// are we done?
				bDone = (bDone || nDay > nNumDays);
			}
		}
		break;

	case WLC_DISPLAY_DAYS_SHORT:
	case WLC_DISPLAY_DAYS_MID:
	case WLC_DISPLAY_DAYS_LONG:
	case WLC_DISPLAY_HOURS:
		{
			CRect rMonth(rItem), rDay(rItem);
			rDay.top += (rDay.Height() / 2);

			// draw month header
			rMonth.bottom = rDay.top;
			DrawListHeaderRect(pDC, rMonth, m_hdrColumns.GetItemText(nCol), pThemed);

			// draw day elements
			int nNumDays = CDateHelper::GetDaysInMonth(nMonth, nYear);
			double dDayWidth = (rMonth.Width() / (double)nNumDays);

			rDay.right = rDay.left;
			
			for (int nDay = 1; nDay <= nNumDays; nDay++)
			{
				rDay.left = rDay.right;
				rDay.right = rMonth.left + (int)(nDay * dDayWidth);

				// check if we can stop
				if (rDay.left > rClip.right)
					break; 

				// check if we need to draw
				if (rDay.right >= rClip.left)
				{
					CString sHeader;

					if (m_nMonthDisplay == WLC_DISPLAY_HOURS)
					{
						COleDateTime dtDay(nYear, nMonth, nDay, 0, 0, 0);
						sHeader = FormatDate(dtDay, DHFD_DOW);
					}
					else if (m_nMonthDisplay == WLC_DISPLAY_DAYS_LONG)
					{
						COleDateTime dtDay(nYear, nMonth, nDay, 0, 0, 0);
						sHeader = FormatDate(dtDay, DHFD_NOYEAR);
					}
					else
					{
						sHeader.Format(_T("%d"), nDay);
					}

					DrawListHeaderRect(pDC, rDay, sHeader, pThemed);
				}
			}
		}
		break;
		
	default:
		ASSERT(0);
		break;
	}

	pDC->SelectObject(pOldFont); // not sure if this is necessary but play safe
	pDC->RestoreDC(nSaveDC);
}

void CWorkloadCtrl::DrawListHeaderRect(CDC* pDC, const CRect& rItem, const CString& sItem, CThemed* pTheme)
{
	if (!pTheme)
	{
		pDC->FillSolidRect(rItem, ::GetSysColor(COLOR_3DFACE));
		pDC->Draw3dRect(rItem, ::GetSysColor(COLOR_3DHIGHLIGHT), ::GetSysColor(COLOR_3DSHADOW));
	}
	else
	{
		pTheme->DrawBackground(pDC, HP_HEADERITEM, HIS_NORMAL, rItem);
	}
	
	// text
	if (!sItem.IsEmpty())
	{
		pDC->SetBkMode(TRANSPARENT);
		pDC->SetTextColor(GetSysColor(COLOR_WINDOWTEXT));

		const UINT nFlags = (DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_CENTER | GraphicsMisc::GetRTLDrawTextFlags(m_hdrColumns));
		pDC->DrawText(sItem, (LPRECT)(LPCRECT)rItem, nFlags);
	}
}

BOOL CWorkloadCtrl::HasDisplayDates(const WORKLOADITEM& gi) const
{
	if (HasDoneDate(gi))
		return TRUE;

	// else
	COleDateTime dummy1, dummy2;
	return GetTaskStartDueDates(gi, dummy1, dummy2);
}

BOOL CWorkloadCtrl::HasDoneDate(const WORKLOADITEM& gi) const
{
	return CDateHelper::IsDateSet(gi.dtDone);
}

BOOL CWorkloadCtrl::GetTaskStartDueDates(const WORKLOADITEM& gi, COleDateTime& dtStart, COleDateTime& dtDue) const
{
	dtStart = gi.dtStart;
	dtDue = gi.dtDue;

	BOOL bDoneSet = CDateHelper::IsDateSet(gi.dtDone);
		
	// do we need to calculate due date?
	if (!CDateHelper::IsDateSet(dtDue) && HasOption(WLCF_CALCMISSINGDUEDATES))
	{
		// always take completed date if that is set
		if (bDoneSet)
		{
			dtDue = gi.dtDone;
		}
		else // take later of start date and today
		{
			dtDue = CDateHelper::GetDateOnly(dtStart);
			CDateHelper::Max(dtDue, CDateHelper::GetDate(DHD_TODAY));
			
			// and move to end of the day
			dtDue = CDateHelper::GetEndOfDay(dtDue);
		}
		
		ASSERT(CDateHelper::IsDateSet(dtDue));
	}
	
	// do we need to calculate start date?
	if (!CDateHelper::IsDateSet(dtStart) && HasOption(WLCF_CALCMISSINGSTARTDATES))
	{
		// take earlier of due or completed date
		dtStart = CDateHelper::GetDateOnly(dtDue);
		CDateHelper::Min(dtStart, CDateHelper::GetDateOnly(gi.dtDone));

		// take the earlier of that and 'today'
		CDateHelper::Min(dtStart, CDateHelper::GetDate(DHD_TODAY));
		
		ASSERT(CDateHelper::IsDateSet(dtStart));
	}

	return (CDateHelper::IsDateSet(dtStart) && CDateHelper::IsDateSet(dtDue));
}

COLORREF CWorkloadCtrl::GetTreeTextBkColor(const WORKLOADITEM& gi, BOOL bSelected, BOOL bAlternate) const
{
	COLORREF crTextBk = gi.GetTextBkColor(bSelected, HasOption(WLCF_TASKTEXTCOLORISBKGND));

	if (crTextBk == CLR_NONE)
	{
		if (bAlternate && HasAltLineColor())
		{
			crTextBk = m_crAltLine;
		}
		else if ((m_crDefault != CLR_NONE) && HasOption(WLCF_TASKTEXTCOLORISBKGND))
		{
			crTextBk = m_crDefault;
		}
		else
		{
			crTextBk = GetSysColor(COLOR_WINDOW);
		}
	}
	
	return crTextBk;
}

COLORREF CWorkloadCtrl::GetTreeTextColor(const WORKLOADITEM& gi, BOOL bSelected, BOOL bLighter) const
{
	COLORREF crText = gi.GetTextColor(bSelected, HasOption(WLCF_TASKTEXTCOLORISBKGND));
	ASSERT(crText != CLR_NONE);

	if (bSelected)
	{
		crText = GraphicsMisc::GetExplorerItemTextColor(crText, GMIS_SELECTED, GMIB_THEMECLASSIC);
	}
	else if (bLighter)
	{
		crText = GraphicsMisc::Lighter(crText, 0.5);
	}

	return crText;
}

BOOL CWorkloadCtrl::CalcDateRect(const CRect& rMonth, int nMonth, int nYear, 
							const COleDateTime& dtFrom, const COleDateTime& dtTo, CRect& rDate)
{
	int nDaysInMonth = CDateHelper::GetDaysInMonth(nMonth, nYear);

	if (nDaysInMonth == 0)
		return FALSE;
	
	COleDateTime dtMonthStart(nYear, nMonth, 1, 0, 0, 0);
	COleDateTime dtMonthEnd(nYear, nMonth, nDaysInMonth, 23, 59, 59); // end of last day

	return CalcDateRect(rMonth, nDaysInMonth, dtMonthStart, dtMonthEnd, dtFrom, dtTo, rDate);
}

BOOL CWorkloadCtrl::CalcDateRect(const CRect& rMonth, int nDaysInMonth, 
							const COleDateTime& dtMonthStart, const COleDateTime& dtMonthEnd, 
							const COleDateTime& dtFrom, const COleDateTime& dtTo, CRect& rDate)
{
	if (dtFrom > dtTo || dtTo < dtMonthStart || dtFrom > dtMonthEnd)
		return FALSE;

	double dDayWidth = (rMonth.Width() / (double)nDaysInMonth);

	if (dtFrom > dtMonthStart)
		rDate.left = (rMonth.left + (int)(((dtFrom.m_dt - dtMonthStart.m_dt) * dDayWidth)));
	else
		rDate.left = rMonth.left;

	if (dtTo < dtMonthEnd)
		rDate.right = (rMonth.left + (int)(((dtTo.m_dt - dtMonthStart.m_dt) * dDayWidth)));
	else
		rDate.right = rMonth.right;

	rDate.top = rMonth.top;
	rDate.bottom = rMonth.bottom;

	return (rDate.right > 0);
}

HTREEITEM CWorkloadCtrl::GetTreeItem(DWORD dwTaskID) const
{
	HTREEITEM hti = NULL;
	m_mapHTItems.Lookup(dwTaskID, hti);
	
	return hti;
}

int CWorkloadCtrl::GetListItem(DWORD dwTaskID) const
{
	HTREEITEM hti = GetTreeItem(dwTaskID);

	return (hti ? GetListItem(hti) : -1);
}

BOOL CWorkloadCtrl::GetMonthDates(int nMonth, int nYear, COleDateTime& dtStart, COleDateTime& dtEnd)
{
	int nDaysInMonth = CDateHelper::GetDaysInMonth(nMonth, nYear);
	ASSERT(nDaysInMonth);

	if (nDaysInMonth == 0)
		return FALSE;
	
	dtStart.SetDate(nYear, nMonth, 1);
	dtEnd.m_dt = dtStart.m_dt + nDaysInMonth;
	
	return TRUE;
}

COLORREF CWorkloadCtrl::GetColor(COLORREF crBase, double dLighter, BOOL bSelected)
{
	COLORREF crResult(crBase);

	if (dLighter > 0.0)
		crResult = GraphicsMisc::Lighter(crResult, dLighter);

	if (bSelected)
		crResult = GraphicsMisc::Darker(crResult, 0.15);

	return crResult;
}

int CWorkloadCtrl::GetListItem(HTREEITEM htiFrom) const
{
	if (!htiFrom)
		return -1;

	return CTreeListSyncer::FindListItem(m_lcColumns, (DWORD)htiFrom);
}

void CWorkloadCtrl::ExpandList()
{
	int nNextIndex = 0;
	ExpandList(NULL, nNextIndex);
}

void CWorkloadCtrl::ExpandList(HTREEITEM htiFrom, int& nNextIndex)
{
	CTreeListSyncer::ExpandList(m_lcColumns, m_tcTasks, htiFrom, nNextIndex);
}

void CWorkloadCtrl::CollapseList(HTREEITEM htiFrom)
{
	CTreeListSyncer::CollapseList(m_lcColumns, m_tcTasks, htiFrom);
}

void CWorkloadCtrl::DeleteTreeItem(HTREEITEM htiFrom)
{
	ASSERT(htiFrom);

	DWORD dwTaskID = GetTaskID(htiFrom);

	m_tcTasks.DeleteItem(htiFrom);
	VERIFY(m_data.RemoveKey(dwTaskID));
}

BOOL CWorkloadCtrl::ZoomIn(BOOL bIn)
{
	WLC_MONTH_DISPLAY nNewDisplay = (bIn ? GetNextDisplay(m_nMonthDisplay) : GetPreviousDisplay(m_nMonthDisplay));
	ASSERT(nNewDisplay != WLC_DISPLAY_NONE);

	return SetMonthDisplay(nNewDisplay);
}

BOOL CWorkloadCtrl::SetMonthDisplay(WLC_MONTH_DISPLAY nNewDisplay)
{
	if (!IsValidDisplay(nNewDisplay))
	{
		ASSERT(0);
		return FALSE;
	}

	// calculate the min month width for this display
	int nMinMonthWidth = GetMinMonthWidth(nNewDisplay);
	
	// Check that the new mode does not exceed the max allowed
	if (!CanSetMonthDisplay(nNewDisplay, nMinMonthWidth))
		return FALSE;

	// adjust the header height where we need to draw days/weeks
	switch (nNewDisplay)
	{
	case WLC_DISPLAY_YEARS:
	case WLC_DISPLAY_QUARTERS_SHORT:
	case WLC_DISPLAY_QUARTERS_MID:
	case WLC_DISPLAY_QUARTERS_LONG:
	case WLC_DISPLAY_MONTHS_SHORT:
	case WLC_DISPLAY_MONTHS_MID:
	case WLC_DISPLAY_MONTHS_LONG:
		m_hdrColumns.SetRowCount(1);
		m_hdrTasks.SetRowCount(1);
		break;

	case WLC_DISPLAY_QUARTERCENTURIES:
	case WLC_DISPLAY_DECADES:
	case WLC_DISPLAY_WEEKS_SHORT:
	case WLC_DISPLAY_WEEKS_MID:
	case WLC_DISPLAY_WEEKS_LONG:
	case WLC_DISPLAY_DAYS_SHORT:
	case WLC_DISPLAY_DAYS_MID:
	case WLC_DISPLAY_DAYS_LONG:
	case WLC_DISPLAY_HOURS:
		m_hdrColumns.SetRowCount(2);
		m_hdrTasks.SetRowCount(2);
		break;
		
	default:
		ASSERT(0);
		break;
	}

	return ZoomTo(nNewDisplay, max(MIN_MONTH_WIDTH, nMinMonthWidth));
}

BOOL CWorkloadCtrl::CanSetMonthDisplay(WLC_MONTH_DISPLAY nDisplay) const
{
	return CanSetMonthDisplay(nDisplay, GetMinMonthWidth(nDisplay));
}

BOOL CWorkloadCtrl::CanSetMonthDisplay(WLC_MONTH_DISPLAY nDisplay, int nMonthWidth) const
{
	ASSERT(nMonthWidth > 0);

	if (!IsValidDisplay(nDisplay) || (nMonthWidth < 1))
	{
		ASSERT(0);
		return FALSE;
	}

	int nNumReqColumns = (GetRequiredListColumnCount(nDisplay) + 1);
	int nColWidth = GetColumnWidth(nDisplay, nMonthWidth);
	int nTotalReqdWidth = (nNumReqColumns * nColWidth);
	
	return (nTotalReqdWidth <= MAX_HEADER_WIDTH);
}

BOOL CWorkloadCtrl::ValidateMonthDisplay(WLC_MONTH_DISPLAY& nDisplay) const
{
	if (!IsValidDisplay(nDisplay))
	{
		ASSERT(0);
		return FALSE;
	}

	int nWidth = GetMinMonthWidth(nDisplay);
	
	return ValidateMonthDisplay(nDisplay, nWidth);
}

BOOL CWorkloadCtrl::ValidateMonthDisplay(WLC_MONTH_DISPLAY& nDisplay, int& nMonthWidth) const
{
	if (!IsValidDisplay(nDisplay))
	{
		ASSERT(0);
		return FALSE;
	}

	if (!CanSetMonthDisplay(nDisplay, nMonthWidth))
	{
		// Look backwards until we find a valid display
		WLC_MONTH_DISPLAY nPrev = nDisplay;
		nDisplay = GetPreviousDisplay(nDisplay);

		while (nDisplay != nPrev)
		{
			nMonthWidth = GetMinMonthWidth(nDisplay);

			if (CanSetMonthDisplay(nDisplay, nMonthWidth))
				return TRUE;

			nPrev = nDisplay;
			nDisplay = GetPreviousDisplay(nDisplay);
		} 

		// We never get here 
		ASSERT(0);
		return FALSE;
	}

	return TRUE;
}

void CWorkloadCtrl::ValidateMonthDisplay()
{
	WLC_MONTH_DISPLAY nPrevDisplay = m_nMonthDisplay, nDisplay = nPrevDisplay;
	int nMonthWidth = m_nMonthWidth;

	VERIFY(ValidateMonthDisplay(nDisplay, nMonthWidth));

	if (m_nMonthDisplay != nDisplay)
	{
		VERIFY(SetMonthDisplay(nDisplay));
		GetCWnd()->SendMessage(WM_WLCN_ZOOMCHANGE, nPrevDisplay, m_nMonthDisplay);
	}
}

BOOL CWorkloadCtrl::ZoomTo(WLC_MONTH_DISPLAY nNewDisplay, int nNewMonthWidth)
{
	// validate new display
	if (!ValidateMonthDisplay(nNewDisplay, nNewMonthWidth))
		return FALSE;

	if ((nNewDisplay == m_nMonthDisplay) && (nNewMonthWidth == m_nMonthWidth))
		return TRUE;

	// cache the scroll-pos at the centre of the view so we can restore it
	CRect rClient;
	CWnd::GetClientRect(rClient);

	COleDateTime dtPos;
	BOOL bRestorePos = GetDateFromScrollPos((m_lcColumns.GetScrollPos(SB_HORZ) + (rClient.Width() / 2)), dtPos);

	// always cancel any ongoing operation
	CancelOperation();
	
	// do the update
	CWorkloadLockUpdates glu(this, FALSE, FALSE);

	if (nNewDisplay != m_nMonthDisplay)
	{
		int nNewColWidth = GetColumnWidth(nNewDisplay, nNewMonthWidth);

		m_nMonthWidth = nNewMonthWidth;
		m_nMonthDisplay = nNewDisplay;

		UpdateListColumns(nNewColWidth);
	}
	else
	{
		int nCurColWidth = GetColumnWidth();
		int nNewColWidth = GetColumnWidth(m_nMonthDisplay, nNewMonthWidth);

		RecalcListColumnWidths(nCurColWidth, nNewColWidth);

		m_nMonthWidth = (int)GetMonthWidth(nNewColWidth);
	}

	RefreshSize();

	// restore scroll-pos
	if (bRestorePos)
	{
		int nScrollPos = GetScrollPosFromDate(dtPos);

		// Date was at the centre of the view
		nScrollPos -= (rClient.Width() / 2);
		nScrollPos = max(0, nScrollPos);

		if (nScrollPos)
			ListView_Scroll(m_lcColumns, (nScrollPos - m_lcColumns.GetScrollPos(SB_HORZ)), 0);
	}

	return TRUE;
}

BOOL CWorkloadCtrl::ZoomBy(int nAmount)
{
	int nNewMonthWidth = (m_nMonthWidth + nAmount);

	// will this trigger a min-width change?
	WLC_MONTH_DISPLAY nNewMonthDisplay = GetColumnDisplay(nNewMonthWidth);

	return ZoomTo(nNewMonthDisplay, nNewMonthWidth);
}

void CWorkloadCtrl::RecalcListColumnWidths(int nFromWidth, int nToWidth)
{
	// resize the required number of columns
	int nNumReqColumns = GetRequiredListColumnCount(), i;

	for (i = 1; i <= nNumReqColumns; i++)
	{
		int nWidth = m_hdrColumns.GetItemWidth(i);

		if (nFromWidth < nToWidth && nWidth < nToWidth)
		{
			m_hdrColumns.SetItemWidth(i, nToWidth);
		}
		else if (nFromWidth > nToWidth && nWidth > nToWidth)
		{
			m_hdrColumns.SetItemWidth(i, nToWidth);
		}
	}

	// and zero out the rest
	int nNumCols = m_hdrColumns.GetItemCount();

	for (; i < nNumCols; i++)
	{
		m_hdrColumns.EnableItemTracking(i, FALSE);
		m_hdrColumns.SetItemWidth(i, 0);
	}
}

WLC_COLUMN CWorkloadCtrl::GetColumnID(int nCol) const
{
	return (WLC_COLUMN)m_hdrTasks.GetItemData(nCol);
}

void CWorkloadCtrl::ResizeColumnsToFit()
{
	// tree columns
	CClientDC dc(&m_tcTasks);
	int nCol = m_hdrTasks.GetItemCount(), nTotalColWidth = 0;

	while (nCol--)
		nTotalColWidth += RecalcTreeColumnWidth(GetColumnID(nCol), &dc);

	SetSplitPos(nTotalColWidth);
	
	// list columns (except first dummy column)
	nCol = GetRequiredListColumnCount();
	
	while (--nCol > 0)
		m_hdrColumns.SetItemWidth(nCol, GetColumnWidth());

	Resize();
}

void CWorkloadCtrl::OnNotifySplitterChange(int nSplitPos)
{
	CTreeListSyncer::OnNotifySplitterChange(nSplitPos);

	// Adjust 'Title' column to suit
	int nRestOfColsWidth = m_hdrTasks.CalcTotalItemsWidth(0);

	CClientDC dc(&m_tcTasks);
	int nMinColWidth = CalcWidestItemTitle(NULL, &dc, FALSE);

	int nTitleColWidth = max(nMinColWidth, (nSplitPos - nRestOfColsWidth));

	m_hdrTasks.SetItemWidth(WLCC_TITLE, nTitleColWidth);
	m_hdrTasks.SetItemTracked(WLCC_TITLE, TRUE);
	m_hdrTasks.UpdateWindow();

	m_tcTasks.SendMessage(WM_GTCN_TITLECOLUMNWIDTHCHANGE, nTitleColWidth, (LPARAM)m_tcTasks.GetSafeHwnd());
}

BOOL CWorkloadCtrl::RecalcTreeColumns(BOOL bResize)
{
	// Only need recalc non-fixed column widths
	BOOL bTitle = !m_hdrTasks.IsItemTracked(WLCC_TITLE);
	BOOL bAllocTo = !m_hdrTasks.IsItemTracked(WLCC_ALLOCTO);
	BOOL bTaskID = !m_hdrTasks.IsItemTracked(WLCC_TASKID);

	if (bTitle || bAllocTo || bTaskID)
	{
		CClientDC dc(&m_tcTasks);

		if (bTitle)
			RecalcTreeColumnWidth(WLCC_TITLE, &dc);
			
		if (bAllocTo)
			RecalcTreeColumnWidth(WLCC_ALLOCTO, &dc);
		
		if (bTaskID)
			RecalcTreeColumnWidth(WLCC_TASKID, &dc);
		
		if (bResize)
			Resize();

		return TRUE;
	}

	return FALSE;
}

int CWorkloadCtrl::RecalcTreeColumnWidth(int nCol, CDC* pDC)
{
	int nColWidth = CalcTreeColumnWidth(nCol, pDC);

	m_hdrTasks.SetItemWidth(nCol, nColWidth);

	return nColWidth;
}

int CWorkloadCtrl::CalcTreeColumnWidth(int nCol, CDC* pDC) const
{
	ASSERT(pDC);
	CFont* pOldFont = GraphicsMisc::PrepareDCFont(pDC, m_tcTasks);

	int nColWidth = 0;
	WLC_COLUMN nColID = GetColumnID(nCol);

	switch (nColID)
	{
	case WLCC_TITLE:
		nColWidth = CalcWidestItemTitle(NULL, pDC, TRUE);
		nColWidth = max(nColWidth, TREE_TITLE_MIN_WIDTH);
		break;

	case WLCC_TASKID:
		nColWidth = pDC->GetTextExtent(Misc::Format(m_dwMaxTaskID)).cx;
		break;
		
	case WLCC_ALLOCTO:
		nColWidth = GraphicsMisc::GetAverageMaxStringWidth(GetLongestVisibleAllocTo(NULL), pDC);
		break;
		
	// rest of attributes are fixed width
	case WLCC_STARTDATE:
	case WLCC_DUEDATE: 
	case WLCC_DONEDATE: 
		{
			COleDateTime date(2015, 12, 31, 23, 59, 59);
			nColWidth = GraphicsMisc::GetAverageMaxStringWidth(FormatDate(date), pDC);
		}
		break;
		
	case WLCC_PERCENT: 
		nColWidth = GraphicsMisc::GetAverageMaxStringWidth(_T("100%"), pDC);
		break;

	default:
		ASSERT(0);
	}

	if (nColWidth < MIN_COL_WIDTH)
		nColWidth = MIN_COL_WIDTH;
	else
		nColWidth += (2 * LV_COLPADDING);
	
	// take max of this and column title
	int nTitleWidth = (m_hdrTasks.GetItemTextWidth(nCol, pDC) + (2 * HD_COLPADDING));
	ASSERT(nTitleWidth);

	pDC->SelectObject(pOldFont);

	return max(nTitleWidth, nColWidth);
}

int CWorkloadCtrl::CalcWidestItemTitle(HTREEITEM htiParent, CDC* pDC, BOOL bEnd) const
{
	// we only want parents
	HTREEITEM htiChild = m_tcTasks.GetChildItem(htiParent);
	
	if (htiChild == NULL)
		return 0;
	
	// or we only want expanded items
	if (htiParent && !TCH().IsItemExpanded(htiParent, FALSE))
		return 0;
	
	// Prepare font
	HFONT hFont = NULL, hOldFont = NULL;
	
// 	if (bEnd)
// 	{
// 		hFont = m_tcTasks.Fonts().GetHFont((htiParent == NULL) ? GMFS_BOLD : 0);
// 		hOldFont = (HFONT)pDC->SelectObject(hFont);
// 	}
	
	// else try children
	int nWidest = 0;
	
	while (htiChild)
	{
		CRect rChild;
		
		if (m_tcTasks.GetItemRect(htiChild, rChild, TRUE)) // text rect only
		{
			int nWidth = 0;

			if (bEnd)
			{
				DWORD dwTaskID = GetTaskID(htiChild);
				const WORKLOADITEM* pGI = NULL;
			
				GET_GI_RET(dwTaskID, pGI, 0);
			
				int nTextWidth = pDC->GetTextExtent(pGI->sTitle).cx;
				nWidth = max(nTextWidth, (rChild.left + nTextWidth));
			}
			else
			{
				nWidth = (rChild.left + TREE_TITLE_MIN_WIDTH);
			}
			
			int nWidestChild = CalcWidestItemTitle(htiChild, pDC, bEnd); // RECURSIVE CALL
			nWidest = max(max(nWidest, nWidth), nWidestChild);
		}
		
		htiChild = m_tcTasks.GetNextItem(htiChild, TVGN_NEXT);
	}
	
	pDC->SelectObject(hOldFont);
	
	return nWidest;
}

void CWorkloadCtrl::UpdateListColumnsWidthAndText(int nWidth)
{
	// first column is always zero width and not trackable
	m_hdrColumns.SetItemWidth(0, 0);
	m_hdrColumns.EnableItemTracking(0, FALSE);

	if (nWidth == -1)
		nWidth = GetColumnWidth();

	int nNumReqColumns = (GetRequiredListColumnCount() + 1);
	BOOL bUsePrevWidth = (m_aPrevColWidths.GetSize() == nNumReqColumns);
	int nTotalReqdWidth = 0;

	COleDateTime dtStart = GetStartDate(m_nMonthDisplay);
	int nYear = dtStart.GetYear(), nMonth = dtStart.GetMonth();
	
	int nCol = 1;

	do
	{
		if (nMonth && nYear)
		{
			CString sTitle = FormatListColumnHeaderText(m_nMonthDisplay, nMonth, nYear);
			DWORD dwData = MAKELONG(nMonth, nYear);

			if (bUsePrevWidth)
				nWidth = m_aPrevColWidths[nCol];

			m_hdrColumns.SetItem(nCol, nWidth, sTitle, dwData);
			m_hdrColumns.EnableItemTracking(nCol, TRUE);

			nTotalReqdWidth += nWidth;
		}

		// Next column
		switch (m_nMonthDisplay)
		{
		case WLC_DISPLAY_QUARTERCENTURIES:
			nYear += 25;
			break;

		case WLC_DISPLAY_DECADES:
			nYear += 10;
			break;
			
		case WLC_DISPLAY_YEARS:
			nYear += 1;
			break;
			
		case WLC_DISPLAY_QUARTERS_SHORT:
		case WLC_DISPLAY_QUARTERS_MID:
		case WLC_DISPLAY_QUARTERS_LONG:
			nMonth += 3;

			if (nMonth > 12)
			{
				nMonth = 1;
				nYear += 1;
			}
			break;
			
		case WLC_DISPLAY_MONTHS_SHORT:
		case WLC_DISPLAY_MONTHS_MID:
		case WLC_DISPLAY_MONTHS_LONG:
		case WLC_DISPLAY_WEEKS_SHORT:
		case WLC_DISPLAY_WEEKS_MID:
		case WLC_DISPLAY_WEEKS_LONG:
		case WLC_DISPLAY_DAYS_SHORT:
		case WLC_DISPLAY_DAYS_MID:
		case WLC_DISPLAY_DAYS_LONG:
		case WLC_DISPLAY_HOURS:
			nMonth += 1;

			if (nMonth > 12)
			{
				nMonth = 1;
				nYear += 1;
			}
			break;
			
		default:
			ASSERT(0);
			break;
		}
		ASSERT(CDateHelper::IsValidDayInMonth(1, nMonth, nYear));
	}
	while (++nCol < nNumReqColumns);

	TRACE(_T("CWorkloadTreeListCtrl(Total Column Widths = %d)\n"), nTotalReqdWidth);

	// for the rest, clear the text and item data and prevent tracking
	int nNumCols = m_hdrColumns.GetItemCount();

	for (; nCol < nNumCols; nCol++)
	{
		m_hdrColumns.EnableItemTracking(nCol, FALSE);
		m_hdrColumns.SetItem(nCol, 0, _T(""), 0);
	}

	// always clear previous width/tracked arrays
	m_aPrevColWidths.RemoveAll();
	m_aPrevTrackedCols.RemoveAll();
}

void CWorkloadCtrl::BuildListColumns()
{
	// once only
	if (m_hdrColumns.GetItemCount())
		return;

	// add empty column as placeholder so we can
	// easily replace the other columns without
	// losing all our items too
	LVCOLUMN lvc = { LVCF_FMT | LVCF_WIDTH | LVCF_TEXT, 0 };
	m_lcColumns.InsertColumn(0, &lvc);
	
	// add other columns
	int nNumCols = GetRequiredListColumnCount(m_nMonthDisplay);

	for (int i = 0; i < nNumCols; i++)
	{
		lvc.cx = 0;
		lvc.fmt = LVCFMT_CENTER | HDF_STRING;
		lvc.pszText = _T("");
		lvc.cchTextMax = 50;

		int nCol = m_hdrColumns.GetItemCount();
		m_lcColumns.InsertColumn(nCol, &lvc);
	}

	UpdateListColumnsWidthAndText();
}

void CWorkloadCtrl::UpdateListColumns(int nWidth)
{
	// cache the scrolled position
	int nScrollPos = m_lcColumns.GetScrollPos(SB_HORZ);

	COleDateTime dtPos;
	BOOL bRestorePos = GetDateFromScrollPos(nScrollPos, dtPos);

	if (nWidth == -1)
		nWidth = GetColumnWidth();

	int nNumCols = m_hdrColumns.GetItemCount();
	int nReqCols = (GetRequiredListColumnCount(m_nMonthDisplay) + 1);
	int nDiffCols = (nReqCols - nNumCols);

	if (nDiffCols > 0)
	{
		// add other columns
		LVCOLUMN lvc = { LVCF_FMT | LVCF_WIDTH | LVCF_TEXT, 0 };

		for (int i = 0, nCol = nNumCols; i < nDiffCols; i++, nCol++)
		{
			lvc.cx = 0;
			lvc.fmt = LVCFMT_CENTER | HDF_STRING;
			lvc.pszText = _T("");
			lvc.cchTextMax = 50;

			m_lcColumns.InsertColumn(nCol, &lvc);
		}
	}
	else if (nDiffCols < 0)
	{
		int i = nNumCols;
		
		while (i-- > nReqCols)
		{
			m_lcColumns.DeleteColumn(i);
		}
	}
	ASSERT(m_hdrColumns.GetItemCount() == nReqCols);

	if (nDiffCols != 0)
		PostResize();

	UpdateListColumnsWidthAndText(nWidth);

	// restore scroll-pos
	if (bRestorePos)
	{
		nScrollPos = GetScrollPosFromDate(dtPos);
		m_lcColumns.Scroll(CSize(nScrollPos - m_lcColumns.GetScrollPos(SB_HORZ), 0));
	}
	else
	{
		m_lcColumns.SetScrollPos(SB_HORZ, 0, TRUE);
	}
}

int CWorkloadCtrl::GetMinMonthWidth(WLC_MONTH_DISPLAY nDisplay) const
{
	int nWidth = 0;
	VERIFY(m_mapMinMonthWidths.Lookup(nDisplay, nWidth) && (nWidth >= MIN_MONTH_WIDTH));

	return max(nWidth, MIN_MONTH_WIDTH);
}

void CWorkloadCtrl::CalcMinMonthWidths()
{
	CClientDC dcClient(&m_hdrTasks);
	CFont* pOldFont = GraphicsMisc::PrepareDCFont(&dcClient, m_lcColumns);

	for (int nMode = 0; nMode < NUM_DISPLAYMODES; nMode++)
	{
		WLC_MONTH_DISPLAY nDisplay = DISPLAYMODES[nMode].nDisplay;
		int nMinMonthWidth = 0;

		switch (nDisplay)
		{
		case WLC_DISPLAY_QUARTERCENTURIES:
			{
				CString sText = FormatListColumnHeaderText(WLC_DISPLAY_YEARS, 1, 2013);
				
				int nMinTextWidth = dcClient.GetTextExtent(sText).cx;
				nMinMonthWidth = (nMinTextWidth + COLUMN_PADDING) / 12;
			}
			break;

		case WLC_DISPLAY_DECADES:
		case WLC_DISPLAY_YEARS:
			{
				// just increase the width of the preceding display
				WLC_MONTH_DISPLAY nPrev = DISPLAYMODES[nMode - 1].nDisplay;

				nMinMonthWidth = GetMinMonthWidth(nPrev);
				nMinMonthWidth = (int)(nMinMonthWidth * MULTIYEAR_MULTIPLIER);
			}
			break;
			
		case WLC_DISPLAY_QUARTERS_SHORT:
			{
				CString sText = FormatListColumnHeaderText(nDisplay, 1, 2013);
				
				int nMinTextWidth = dcClient.GetTextExtent(sText).cx;
				nMinMonthWidth = (nMinTextWidth + COLUMN_PADDING) / 3;
			}
			break;
			
		case WLC_DISPLAY_QUARTERS_MID:
		case WLC_DISPLAY_QUARTERS_LONG:
			{
				int nMinTextWidth = 0;
				
				for (int nMonth = 1; nMonth <= 12; nMonth += 3)
				{
					CString sText = FormatListColumnHeaderText(nDisplay, nMonth, 2013);
					
					int nWidth = dcClient.GetTextExtent(sText).cx;
					nMinTextWidth = max(nWidth, nMinTextWidth);
				}
				
				nMinMonthWidth = (nMinTextWidth + COLUMN_PADDING) / 3;
			}
			break;
			
		case WLC_DISPLAY_MONTHS_SHORT:
		case WLC_DISPLAY_MONTHS_MID:
		case WLC_DISPLAY_MONTHS_LONG:
			{
				int nMinTextWidth = 0;
				
				for (int nMonth = 1; nMonth <= 12; nMonth++)
				{
					CString sText = FormatListColumnHeaderText(nDisplay, nMonth, 2013);
					
					int nWidth = dcClient.GetTextExtent(sText).cx;
					nMinTextWidth = max(nWidth, nMinTextWidth);
				}
				
				nMinMonthWidth = (nMinTextWidth + COLUMN_PADDING);
			}
			break;
			
		case WLC_DISPLAY_WEEKS_SHORT:
		case WLC_DISPLAY_WEEKS_MID:
		case WLC_DISPLAY_WEEKS_LONG:
		case WLC_DISPLAY_DAYS_SHORT:
		case WLC_DISPLAY_DAYS_MID:
			{
				// just increase the width of the preceding display
				WLC_MONTH_DISPLAY nPrev = DISPLAYMODES[nMode - 1].nDisplay;

				nMinMonthWidth = GetMinMonthWidth(nPrev);
				nMinMonthWidth = (int)(nMinMonthWidth * DAY_WEEK_MULTIPLIER);
			}
			break;

		case WLC_DISPLAY_DAYS_LONG:
			{
				// increase the width of the preceding display
				WLC_MONTH_DISPLAY nPrev = DISPLAYMODES[nMode - 1].nDisplay;

				nMinMonthWidth = GetMinMonthWidth(nPrev);
				nMinMonthWidth = (int)(nMinMonthWidth * DAY_WEEK_MULTIPLIER);

				int nWidth = dcClient.GetTextExtent(_T(" 31/12 ")).cx;
				nMinMonthWidth = max((nWidth * 31), nMinMonthWidth);
			}
			break;
			
		case WLC_DISPLAY_HOURS:
			{
				// just increase the width of the preceding display
				WLC_MONTH_DISPLAY nPrev = DISPLAYMODES[nMode - 1].nDisplay;

				nMinMonthWidth = GetMinMonthWidth(nPrev);
				nMinMonthWidth = (int)(nMinMonthWidth * HOUR_DAY_MULTIPLIER);
			}
			break;
			
		default:
			ASSERT(0);
			break;
		}

		if (nMinMonthWidth > 0)
		{
			nMinMonthWidth++; // for rounding
			m_mapMinMonthWidths[nDisplay] = nMinMonthWidth; 
		}
	}

	dcClient.SelectObject(pOldFont);
}

WLC_MONTH_DISPLAY CWorkloadCtrl::GetColumnDisplay(int nMonthWidth)
{
	int nMinWidth = 0;

	for (int nMode = 0; nMode < (NUM_DISPLAYMODES - 1); nMode++)
	{
		WLC_MONTH_DISPLAY nDisplay = DISPLAYMODES[nMode].nDisplay;
		WLC_MONTH_DISPLAY nNext = DISPLAYMODES[nMode + 1].nDisplay;

		int nFromWidth = GetMinMonthWidth(nDisplay);
		int nToWidth = GetMinMonthWidth(nNext);

		if ((nMonthWidth >= nFromWidth) && (nMonthWidth < nToWidth))
			return nDisplay;
	}

	return GetLastDisplay();
}

int CALLBACK CWorkloadCtrl::MultiSortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	const CWorkloadCtrl* pThis = (CWorkloadCtrl*)lParamSort;
	const WORKLOADSORTCOLUMNS& sort = pThis->m_sort.multi;

	int nCompare = 0;

	for (int nCol = 0; ((nCol < 3) && (nCompare == 0)); nCol++)
	{
		if (sort.cols[nCol].nBy == IUI_NONE)
			break;

		nCompare = pThis->CompareTasks(lParam1, lParam2, sort.cols[nCol]);
	}

	return nCompare;
}

int CALLBACK CWorkloadCtrl::SortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	const CWorkloadCtrl* pThis = (CWorkloadCtrl*)lParamSort;
	
	return pThis->CompareTasks(lParam1, lParam2, pThis->m_sort.single);
}

int CWorkloadCtrl::CompareTasks(DWORD dwTaskID1, DWORD dwTaskID2, const WORKLOADSORTCOLUMN& col) const
{
	int nCompare = 0;

	// Optimise for task ID
	if (col.nBy == WLCC_TASKID)
	{
		nCompare = (dwTaskID1 - dwTaskID2);
	}
	else
	{
		const WORKLOADITEM* pGI1 = GetWorkloadItem(dwTaskID1);
		const WORKLOADITEM* pGI2 = GetWorkloadItem(dwTaskID2);

		if (!pGI1 || !pGI2)
		{
			ASSERT(0);
			return 0;
		}

		switch (col.nBy)
		{
		case WLCC_TITLE:
			nCompare = Compare(pGI1->sTitle, pGI2->sTitle);
			break;

		case WLCC_STARTDATE:
			nCompare = CDateHelper::Compare(pGI1->dtStart, pGI2->dtStart, TRUE, FALSE);
			break;

		case WLCC_DUEDATE:
			nCompare = CDateHelper::Compare(pGI1->dtDue, pGI2->dtDue, TRUE, TRUE);
			break;

		case WLCC_DONEDATE:
			nCompare = CDateHelper::Compare(pGI1->dtDue, pGI2->dtDue, TRUE, TRUE);
			break;

		case WLCC_ALLOCTO:
			nCompare = Compare(pGI1->sAllocTo, pGI2->sAllocTo);
			break;

		case WLCC_PERCENT:
			nCompare = (pGI1->nPercent - pGI2->nPercent);
			break;

		case WLCC_NONE:
			nCompare = (pGI1->nPosition - pGI2->nPosition);
			break;

		case WLCC_TAGS:
			{
				CString sTags1 = Misc::FormatArray(pGI1->aTags);
				CString sTags2 = Misc::FormatArray(pGI2->aTags);
		
				nCompare = Misc::NaturalCompare(sTags1, sTags2, TRUE);
			}
			break;

		case WLCC_DEPENDENCY:
			{
				// If Task2 is dependent on Task1 then Task1 comes first
				if (m_data.IsItemDependentOn(pGI2, dwTaskID1))
				{
					TRACE(_T("Sort(Task %d depends on Task %d. Task %d sorts higher\n"), dwTaskID2, dwTaskID1, dwTaskID1);
					nCompare = -1;
				}
				// else if Task1 is dependent on Task2 then Task2 comes first
				else if (m_data.IsItemDependentOn(pGI1, dwTaskID2))
				{
					TRACE(_T("Sort(Task %d depends on Task %d. Task %d sorts higher\n"), dwTaskID1, dwTaskID2, dwTaskID2);
					nCompare = 1;
				}
			}
			break;

		default:
			ASSERT(0);
			break;
		}
	}

	return (col.bAscending ? nCompare : -nCompare);
}

int CWorkloadCtrl::Compare(const CString& sText1, const CString& sText2)
{
	BOOL bEmpty1 = sText1.IsEmpty();
	BOOL bEmpty2 = sText2.IsEmpty();
		
	if (bEmpty1 != bEmpty2)
		return (bEmpty1 ? 1 : -1);
	
	return Misc::NaturalCompare(sText1, sText2);
}

void CWorkloadCtrl::ScrollToToday()
{
	ScrollTo(COleDateTime::GetCurrentTime());
}

void CWorkloadCtrl::ScrollToSelectedTask()
{
	ScrollToTask(GetSelectedTaskID());
}

void CWorkloadCtrl::ScrollToTask(DWORD dwTaskID)
{
	WORKLOADITEM* pGI = NULL;
	GET_GI(dwTaskID, pGI);
	
	COleDateTime dtStart, dtDue;
	
	if (GetTaskStartDueDates(*pGI, dtStart, dtDue))
	{
		ScrollTo(dtStart);
	}
	else if (HasDoneDate(*pGI))
	{
		ScrollTo(pGI->dtDone);
	}
}

void CWorkloadCtrl::ScrollTo(const COleDateTime& date)
{
	COleDateTime dateOnly = CDateHelper::GetDateOnly(date);

	int nStartPos = GetScrollPosFromDate(dateOnly);
	int nEndPos = GetScrollPosFromDate(dateOnly.m_dt + 1.0);

	// if it is already visible no need to scroll
	int nListStart = m_lcColumns.GetScrollPos(SB_HORZ);

	CRect rList;
	m_lcColumns.GetClientRect(rList);

	if ((nStartPos >= nListStart) && (nEndPos <= (nListStart + rList.Width())))
		return;

	// deduct current scroll pos for relative move
	nStartPos -= nListStart;

	// and arbitrary offset
	nStartPos -= 50;

	if (m_lcColumns.Scroll(CSize(nStartPos, 0)))
		CWnd::Invalidate(FALSE);
}

BOOL CWorkloadCtrl::GetListColumnRect(int nCol, CRect& rColumn, BOOL bScrolled) const
{
	if (ListView_GetSubItemRect(m_lcColumns, 0, nCol, LVIR_BOUNDS, &rColumn))
	{
		if (!bScrolled)
		{
			int nScroll = m_lcColumns.GetScrollPos(SB_HORZ);
			rColumn.OffsetRect(nScroll, 0);
		}

		return TRUE;
	}

	return FALSE;
}

BOOL CWorkloadCtrl::GetDateFromScrollPos(int nScrollPos, WLC_MONTH_DISPLAY nDisplay, int nMonth, int nYear, const CRect& rColumn, COleDateTime& date)
{
	CRect rMonth(rColumn);

	// Clip 'rColumn' if displaying more than months
	switch (nDisplay)
	{
	case WLC_DISPLAY_QUARTERCENTURIES:
	case WLC_DISPLAY_DECADES:
	case WLC_DISPLAY_YEARS:
	case WLC_DISPLAY_QUARTERS_SHORT:
	case WLC_DISPLAY_QUARTERS_MID:
	case WLC_DISPLAY_QUARTERS_LONG:
		{
			double dMonthWidth = GetMonthWidth(nDisplay, rMonth.Width());

			// calc month as offset to start of column
			int nPxOffset = (nScrollPos - rMonth.left);
			int nMonthOffset = (int)(nPxOffset / dMonthWidth);

			// clip rect to this month
			rMonth.left += nPxOffset;
			rMonth.right = (rMonth.left + (int)dMonthWidth);

			nMonth += nMonthOffset;

			// Months here are one-based
			nYear += ((nMonth - 1) / 12);
			nMonth = (((nMonth - 1) % 12) + 1);
		}
		break;
	}

	int nDaysInMonth = CDateHelper::GetDaysInMonth(nMonth, nYear);
	int nNumMins = MulDiv((nScrollPos - rMonth.left), (60 * 24 * nDaysInMonth), rMonth.Width());

	int nDay = (1 + (nNumMins / MINS_IN_DAY));
	int nHour = ((nNumMins % MINS_IN_DAY) / MINS_IN_HOUR);
	int nMin = (nNumMins % MINS_IN_HOUR);

	ASSERT(nDay >= 1 && nDay <= nDaysInMonth);
	ASSERT(nHour >= 0 && nHour < 24);

	date.SetDateTime(nYear, nMonth, nDay, nHour, nMin, 0);

	return CDateHelper::IsDateSet(date);
}

BOOL CWorkloadCtrl::GetDateFromScrollPos(int nScrollPos, COleDateTime& date) const
{
	// find the column containing this scroll pos
	int nCol = FindColumn(nScrollPos);

	if (nCol == -1)
		return FALSE;

	// else
	CRect rColumn;
	VERIFY(GetListColumnRect(nCol, rColumn, FALSE));
	ASSERT(nScrollPos >= rColumn.left && nScrollPos < rColumn.right);

	int nYear, nMonth;
	VERIFY(GetListColumnDate(nCol, nMonth, nYear));

	return GetDateFromScrollPos(nScrollPos, m_nMonthDisplay, nMonth, nYear, rColumn, date);
}

int CWorkloadCtrl::GetDrawPosFromDate(const COleDateTime& date) const
{
	return (GetScrollPosFromDate(date) - m_lcColumns.GetScrollPos(SB_HORZ));
}

int CWorkloadCtrl::GetScrollPosFromDate(const COleDateTime& date) const
{
	// figure out which column contains 'date'
	int nCol = FindColumn(date);

	if (nCol != -1)
	{
		CRect rColumn;

		if (GetListColumnRect(nCol, rColumn, FALSE))
		{
			int nDay = date.GetDay();
			int nMonth = date.GetMonth();
			int nYear = date.GetYear();

			double dDayInCol = 0;
			int nDaysInCol = 0;

			switch (m_nMonthDisplay)
			{
			case WLC_DISPLAY_QUARTERCENTURIES:
				// Column == 25 years
				nDaysInCol = (int)(DAYS_IN_YEAR * 25);
				dDayInCol = (int)(((nYear - GetStartYear(m_nMonthDisplay)) * DAYS_IN_YEAR) + ((nMonth - 1) * DAYS_IN_MONTH) + nDay);
				break;

			case WLC_DISPLAY_DECADES:
				// Column == 10 years
				nDaysInCol = (int)(DAYS_IN_YEAR * 10);
				dDayInCol = (int)(((nYear - GetStartYear(m_nMonthDisplay)) * DAYS_IN_YEAR) + ((nMonth - 1) * DAYS_IN_MONTH) + nDay);
				break;

			case WLC_DISPLAY_YEARS:
				// Column == 12 months
				nDaysInCol = (int)DAYS_IN_YEAR;
				dDayInCol = ((int)((nMonth - 1) * DAYS_IN_MONTH) + nDay);
				break;
				
			case WLC_DISPLAY_QUARTERS_SHORT:
			case WLC_DISPLAY_QUARTERS_MID:
			case WLC_DISPLAY_QUARTERS_LONG:
				// Column == 3 months
				nDaysInCol = (int)(DAYS_IN_MONTH * 3);
				dDayInCol = ((int)(((nMonth - 1) % 3) * DAYS_IN_MONTH) + nDay);
				break;

			default: 
				// Column == Month
				nDaysInCol = CDateHelper::GetDaysInMonth(nMonth, nYear);
				dDayInCol = ((nDay - 1) + CDateHelper::GetTimeOnly(date));
				break;
			}

			ASSERT(nDaysInCol > 0);

			if (nDaysInCol > 0)
			{
				double dOffset = ((rColumn.Width() * dDayInCol) / nDaysInCol);
				return (rColumn.left + (int)dOffset);
			}
		}
	}

	// else
	return 0;
}

int CWorkloadCtrl::GetDateInMonths(int nMonth, int nYear) const
{
	return ((nYear * 12) + (nMonth - 1));
}

int CWorkloadCtrl::FindColumn(int nMonth, int nYear) const
{	
	int nMonths = GetDateInMonths(nMonth, nYear);
	int nNumReqColumns = GetRequiredListColumnCount();

	for (int i = 1; i <= nNumReqColumns; i++)
	{
		// get date for current column
		VERIFY (GetListColumnDate(i, nMonth, nYear));

		if (nMonths >= GetDateInMonths(nMonth, nYear))
		{
			if (i == nNumReqColumns)
			{
				return i;
			}
			else // get date for next column
			{
				VERIFY (GetListColumnDate(i+1, nMonth, nYear));
				
				if (nMonths < GetDateInMonths(nMonth, nYear))
				{
					return i;
				}
			}
		}
	}

	// not found
	return -1;
}

int CWorkloadCtrl::FindColumn(const COleDateTime& date) const
{
	return FindColumn(date.GetMonth(), date.GetYear());
}

int CWorkloadCtrl::FindColumn(int nScrollPos) const
{
	ASSERT(m_hdrColumns.GetItemCount() > 0);

	int nNumReqColumns = GetRequiredListColumnCount();

	for (int i = 1; i <= nNumReqColumns; i++)
	{
		CRect rColumn;
		VERIFY(GetListColumnRect(i, rColumn, FALSE));

		if ((nScrollPos >= rColumn.left) && (nScrollPos < rColumn.right))
			return i;
	}

	// not found
	return -1;
}

bool CWorkloadCtrl::PrepareNewTask(ITaskList* pTaskList) const
{
	ITASKLISTBASE* pTasks = GetITLInterface<ITASKLISTBASE>(pTaskList, IID_TASKLISTBASE);

	if (pTasks == NULL)
	{
		ASSERT(0);
		return false;
	}

	// Set the start date to today and of duration 1 day
	HTASKITEM hNewTask = pTasks->GetFirstTask();
	ASSERT(hNewTask);

	COleDateTime dt = CDateHelper::GetDate(DHD_TODAY);
	time64_t tDate = 0;

	if (CDateHelper::GetTimeT64(dt, tDate))
	{
		pTasks->SetTaskStartDate64(hNewTask, tDate);
		pTasks->SetTaskDueDate64(hNewTask, tDate);
	}

	return true;
}

DWORD CWorkloadCtrl::HitTestTask(const CPoint& ptScreen) const
{
	// try list first
	DWORD dwTaskID = ListHitTestTask(ptScreen, TRUE);

	// then tree
	if (!dwTaskID)
		dwTaskID = TreeHitTestTask(ptScreen, TRUE);

	return dwTaskID;
}

DWORD CWorkloadCtrl::ListHitTestTask(const CPoint& ptScreen, BOOL bScreen) const
{
	int nUnused = -1;
	int nItem = ListHitTestItem(ptScreen, bScreen, nUnused);
	
	if (nItem != -1)
		return GetTaskID(nItem);

	return 0;
}

DWORD CWorkloadCtrl::TreeHitTestTask(const CPoint& ptScreen, BOOL bScreen) const
{
	HTREEITEM htiHit = TreeHitTestItem(ptScreen, bScreen);
	
	if (htiHit)
		return GetTaskID(htiHit);
	
	return 0;
}

HTREEITEM CWorkloadCtrl::TreeHitTestItem(const CPoint& point, BOOL bScreen) const
{
	CPoint ptTree(point);

	if (bScreen)
		m_tcTasks.ScreenToClient(&ptTree);
	
	return m_tcTasks.HitTest(ptTree);
}

HTREEITEM CWorkloadCtrl::GetItem(CPoint ptScreen) const
{
	return TreeHitTestItem(ptScreen, TRUE);
}

CString CWorkloadCtrl::GetItemTip(CPoint ptScreen) const
{
	HTREEITEM htiHit = GetItem(ptScreen);

	if (htiHit)
	{
		CRect rItem;

		if (m_tcTasks.GetItemRect(htiHit, rItem, TRUE))
		{
			int nColWidth = m_hdrTasks.GetItemWidth(0);

			rItem.left = max(rItem.left, 0);
			rItem.right = nColWidth;

			CPoint ptClient(ptScreen);
			m_tcTasks.ScreenToClient(&ptClient);

			if (rItem.PtInRect(ptClient))
			{
				DWORD dwTaskID = GetTaskID(htiHit);
				WORKLOADITEM* pGI = NULL;

				GET_GI_RET(dwTaskID, pGI, _T(""));

				int nTextLen = GraphicsMisc::GetTextWidth(pGI->sTitle, m_tcTasks);
				rItem.DeflateRect(TV_TIPPADDING, 0);

				if (nTextLen > rItem.Width())
					return pGI->sTitle;
			}
		}
	}

	// else
	return _T("");
}

BOOL CWorkloadCtrl::PointInHeader(const CPoint& ptScreen) const
{
	CRect rHeader;

	// try tree
	m_hdrTasks.GetWindowRect(rHeader);

	if (rHeader.PtInRect(ptScreen))
		return TRUE;

	// then list
	m_hdrColumns.GetWindowRect(rHeader);

	return rHeader.PtInRect(ptScreen);
}

void CWorkloadCtrl::GetWindowRect(CRect& rWindow, BOOL bWithHeader) const
{
	CRect rTree, rList;

	m_tcTasks.GetWindowRect(rTree);
	m_lcColumns.GetWindowRect(rList);

	if (bWithHeader)
		rWindow = rList; // height will include header
	else
		rWindow = rTree; // height will not include header

	rWindow.left  = min(rTree.left, rList.left);
	rWindow.right = max(rTree.right, rList.right);
}

int CWorkloadCtrl::ListHitTestItem(const CPoint& point, BOOL bScreen, int& nCol) const
{
	if (m_data.GetCount() == 0)
		return -1;

	// convert to list coords
	LVHITTESTINFO lvht = { 0 };
	lvht.pt = point;

	if (bScreen)
		m_lcColumns.ScreenToClient(&(lvht.pt));

	if ((ListView_SubItemHitTest(m_lcColumns, &lvht) != -1) &&	(lvht.iSubItem > 0))
	{
		ASSERT(lvht.iItem != -1);

		// convert pos to date
		COleDateTime dtHit;

		if (GetDateFromPoint(lvht.pt, dtHit))
		{
			nCol = lvht.iSubItem;
			return lvht.iItem;
		}
	}

	// all else
	return -1;
}

BOOL CWorkloadCtrl::GetListItemRect(int nItem, CRect& rItem) const
{
	if (m_lcColumns.GetItemRect(nItem, rItem, LVIR_BOUNDS))
	{
		// Extend to end of client rect
		CRect rClient;
		CWnd::GetClientRect(rClient);

		rItem.right = max(rItem.right, rClient.right);
		return TRUE;
	}

	return FALSE;
}

DWORD CWorkloadCtrl::GetTaskID(HTREEITEM htiFrom) const
{
	if ((htiFrom == NULL) || (htiFrom == TVI_FIRST) || (htiFrom == TVI_ROOT))
		return 0;

	return GetTreeItemData(m_tcTasks, htiFrom);
}

DWORD CWorkloadCtrl::GetTaskID(int nItem) const
{
	return GetListTaskID(GetListItemData(m_lcColumns, nItem));
}

DWORD CWorkloadCtrl::GetListTaskID(DWORD dwItemData) const
{
	return GetTaskID((HTREEITEM)dwItemData);
}

void CWorkloadCtrl::RedrawList(BOOL bErase)
{
	m_lcColumns.InvalidateRect(NULL, bErase);
	m_lcColumns.UpdateWindow();
}

void CWorkloadCtrl::RedrawTree(BOOL bErase)
{
	m_tcTasks.InvalidateRect(NULL, bErase);
	m_tcTasks.UpdateWindow();
}

BOOL CWorkloadCtrl::GetDateFromPoint(const CPoint& ptCursor, COleDateTime& date) const
{
	// convert pos to date
	int nScrollPos = (m_lcColumns.GetScrollPos(SB_HORZ) + ptCursor.x);

	return GetDateFromScrollPos(nScrollPos, date);
}

void CWorkloadCtrl::SetReadOnly(bool bReadOnly) 
{ 
	m_bReadOnly = bReadOnly;

	m_treeDragDrop.EnableDragDrop(!bReadOnly);
}

// external version
BOOL CWorkloadCtrl::CancelOperation()
{
	if (m_treeDragDrop.IsDragging())
	{
		m_treeDragDrop.CancelDrag();
		return TRUE;
	}
	
	// else 
	return FALSE;
}

int CWorkloadCtrl::GetTreeColumnOrder(CIntArray& aTreeOrder) const
{
	return m_hdrTasks.GetItemOrder(aTreeOrder);
}

void CWorkloadCtrl::SetTreeColumnVisibility(const CDWordArray& aColumnVis)
{
	int nNumCols = aColumnVis.GetSize();

	for (int nColID = 1; nColID < nNumCols; nColID++)
	{
		int nCol = m_hdrTasks.FindItem(nColID);
		m_hdrTasks.ShowItem(nCol, aColumnVis[nColID]);
	}

	Resize();
}

BOOL CWorkloadCtrl::SetTreeColumnOrder(const CIntArray& aTreeOrder)
{
	if (!(aTreeOrder.GetSize() && (aTreeOrder[0] == 0)))
	{
		ASSERT(0);
		return FALSE;
	}

	return m_hdrTasks.SetItemOrder(aTreeOrder);
}

void CWorkloadCtrl::GetColumnWidths(CIntArray& aTreeWidths, CIntArray& aListWidths) const
{
	m_hdrTasks.GetItemWidths(aTreeWidths);
	m_hdrColumns.GetItemWidths(aListWidths);

	// trim the list columns to what's currently visible
	// remember to include hidden dummy first column
	int nNumMonths = (GetRequiredListColumnCount() + 1);
	int nItem = aListWidths.GetSize();

	while (nItem-- > nNumMonths)
		aListWidths.RemoveAt(nItem);
}

BOOL CWorkloadCtrl::SetColumnWidths(const CIntArray& aTreeWidths, const CIntArray& aListWidths)
{
	if (aTreeWidths.GetSize() != (NUM_TREECOLUMNS + 1))
		return FALSE;

	m_hdrTasks.SetItemWidths(aTreeWidths);

	// save list column widths for when we've initialised our columns
	// remember to include hidden dummy first column
	if (aListWidths.GetSize() == (GetRequiredListColumnCount() + 1))
		m_hdrColumns.SetItemWidths(aListWidths);
	else
		m_aPrevColWidths.Copy(aListWidths);

	return TRUE;
}

void CWorkloadCtrl::GetTrackedColumns(CIntArray& aTreeTracked, CIntArray& aListTracked) const
{
	m_hdrTasks.GetTrackedItems(aTreeTracked);
	m_hdrColumns.GetTrackedItems(aListTracked);

	// trim the list columns to what's currently visible
	// remember to include hidden dummy first column
	int nNumMonths = (GetRequiredListColumnCount() + 1);
	int nItem = aListTracked.GetSize();

	while (nItem-- > nNumMonths)
		aListTracked.RemoveAt(nItem);
}

BOOL CWorkloadCtrl::SetTrackedColumns(const CIntArray& aTreeTracked, const CIntArray& aListTracked)
{
	if (aTreeTracked.GetSize() != (NUM_TREECOLUMNS + 1))
		return FALSE;
	
	m_hdrTasks.SetTrackedItems(aTreeTracked); 

	// save list column tracking for when we've initialised our columns
	// remember to include hidden dummy first column
	if (aListTracked.GetSize() == (GetRequiredListColumnCount() + 1))
		m_hdrColumns.SetTrackedItems(aListTracked);
	else
		m_aPrevTrackedCols.Copy(aListTracked);
	
	return TRUE;
}

DWORD CWorkloadCtrl::GetNextTask(DWORD dwTaskID, IUI_APPCOMMAND nCmd) const
{
	HTREEITEM hti = FindTreeItem(m_tcTasks, dwTaskID);
	
	if (!hti)
	{
		ASSERT(0);
		return 0L;
	}

	DWORD dwNextID(dwTaskID);

	switch (nCmd)
	{
	case IUI_GETNEXTTASK:
		{
			HTREEITEM htiNext = TCH().GetNextVisibleItem(hti);
			
			if (htiNext)
				dwNextID = GetTreeItemData(m_tcTasks, htiNext);
		}
		break;
		
	case IUI_GETPREVTASK:
		{
			HTREEITEM htiPrev = TCH().GetPrevVisibleItem(hti);
			
			if (htiPrev)
				dwNextID = GetTreeItemData(m_tcTasks, htiPrev);
		}
		break;
		
	case IUI_GETNEXTTOPLEVELTASK:
	case IUI_GETPREVTOPLEVELTASK:
		{
			HTREEITEM htiNext = TCH().GetNextTopLevelItem(hti, (nCmd == IUI_GETNEXTTOPLEVELTASK));
			
			if (htiNext)
				dwNextID = GetTreeItemData(m_tcTasks, htiNext);
		}
		break;
		
	default:
		ASSERT(0);
	}
	
	return dwNextID;
}

BOOL CWorkloadCtrl::SaveToImage(CBitmap& bmImage)
{
	if (m_tcTasks.GetCount() == 0)
		return FALSE;

	CLockUpdates lock(m_tcTasks);

	// Resize tree header width to suit title text width
	int nColWidth = m_hdrTasks.GetItemWidth(0);
	BOOL bTracked = m_hdrTasks.IsItemTracked(0);

	ResizeColumnsToFit();	

	// Calculate the date range in scroll units
	// allow a month's buffer at each end
	COleDateTime dtFrom = CDateHelper::GetStartOfMonth(m_dateRange.GetStart());
	COleDateTime dtTo = CDateHelper::GetEndOfMonth(m_dateRange.GetEnd());

	CDateHelper::IncrementMonth(dtFrom, -1);
	CDateHelper::IncrementMonth(dtTo, 1);

	int nFrom = GetScrollPosFromDate(dtFrom);
	int nTo = GetScrollPosFromDate(dtTo);

	if (nTo == nFrom)
		nTo = -1;
	
	BOOL bRes = CTreeListSyncer::SaveToImage(bmImage, nFrom, nTo);
	
	// Restore title column width
	m_hdrTasks.SetItemWidth(0, nColWidth);
	m_hdrTasks.SetItemTracked(0, bTracked);

	Resize();
	
	return bRes;
}

void CWorkloadCtrl::RefreshItemBoldState(HTREEITEM htiFrom, BOOL bAndChildren)
{
	if (htiFrom && (htiFrom != TVI_ROOT))
	{
		TCH().SetItemBold(htiFrom, (m_tcTasks.GetParentItem(htiFrom) == NULL));
	}
	
	// children
	if (bAndChildren)
	{
		HTREEITEM htiChild = m_tcTasks.GetChildItem(htiFrom);
		
		while (htiChild)
		{
			RefreshItemBoldState(htiChild);
			htiChild = m_tcTasks.GetNextItem(htiChild, TVGN_NEXT);
		}
	}
}

BOOL CWorkloadCtrl::SetFont(HFONT hFont, BOOL bRedraw)
{
	if (!hFont || !m_tcTasks.GetSafeHwnd() || !m_lcColumns.GetSafeHwnd())
	{
		ASSERT(0);
		return FALSE;
	}
	
	m_tcTasks.SetFont(CFont::FromHandle(hFont), bRedraw);

	CalcMinMonthWidths();
	SetMonthDisplay(m_nMonthDisplay);
	ResizeColumnsToFit();

	return TRUE;
}

void CWorkloadCtrl::FilterToolTipMessage(MSG* pMsg)
{
	m_tcTasks.FilterToolTipMessage(pMsg);
	m_hdrTasks.FilterToolTipMessage(pMsg);
	m_hdrColumns.FilterToolTipMessage(pMsg);
}

bool CWorkloadCtrl::ProcessMessage(MSG* pMsg) 
{
	return (m_treeDragDrop.ProcessMessage(pMsg) != FALSE);
}

BOOL CWorkloadCtrl::CanMoveSelectedItem(const IUITASKMOVE& move) const
{
	return (GetSelectedTaskID() && 
			((move.dwParentID == 0) || m_data.HasItem(move.dwParentID)) &&
			((move.dwAfterSiblingID == 0) || m_data.HasItem(move.dwAfterSiblingID)));
}

BOOL CWorkloadCtrl::MoveSelectedItem(const IUITASKMOVE& move)
{
	if (!CanMoveSelectedItem(move))
		return FALSE;

	CAutoFlag af(m_bMovingTask, TRUE);
	
	HTREEITEM htiSel = GetSelectedItem(), htiNew = NULL;
	HTREEITEM htiDestParent = GetTreeItem(move.dwParentID);
	HTREEITEM htiDestAfterSibling = GetTreeItem(move.dwAfterSiblingID);
	
	{
		CHoldRedraw hr2(m_tcTasks, NCR_UPDATE);
		CHoldRedraw hr3(m_lcColumns);

		htiNew = TCH().MoveTree(htiSel, htiDestParent, htiDestAfterSibling, TRUE, TRUE);
		ASSERT(htiNew);
	}

	if (htiNew)
	{
		SelectItem(htiNew);
		return TRUE;
	}

	return FALSE;
}