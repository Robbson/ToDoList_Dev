// TDLTaskAttributeBarCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "TDLTaskAttributeBarCtrl.h"
#include "tdcmsg.h"

#include "..\shared\Localizer.h"
#include "..\shared\DateHelper.h"
#include "..\shared\EnString.h"
#include "..\shared\DlgUnits.h"
#include "..\interfaces\UIThemeFile.h"
#include "..\shared\wclassdefines.h"

#include "TDCDialogHelper.h"
#include "TDCCustomEditAttributeUIHelper.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC(CTDLTaskAttributeBarCtrl, CRuntimeDlg)

/////////////////////////////////////////////////////////////////////////////

static const TDCCONTROL LEGACYCONTROLS[] =
{
	{ WC_STATIC,		IDS_TDC_FIELD_PRIORITY,		SS_CENTERIMAGE, 0, 0, 0, 22, 8, IDC_TDLAB_PRIORITYLABEL },
	{ WC_COMBOBOX,		0, CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS, 0, 0, 0, 65, 300, IDC_TDLAB_PRIORITY },
	{ WC_STATIC,		IDS_TDC_FIELD_RISK,			SS_CENTERIMAGE, 0, 0, 0, 22, 8, IDC_TDLAB_RISKLABEL },
	{ WC_COMBOBOX,		0, CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS, 0, 0, 0, 65, 300, IDC_TDLAB_RISK },
	{ WC_STATIC,		IDS_TDC_FIELD_PERCENT,		SS_CENTERIMAGE, 0, 0, 0, 37, 8, IDC_TDLAB_PERCENTLABEL },
	{ WC_EDIT,			0, WS_TABSTOP, 0, 0, 0, 65, 13, IDC_TDLAB_PERCENT },
	{ WC_SPIN,			0, UDS_SETBUDDYINT | UDS_ARROWKEYS, 0, 0, 0, 13, 14, IDC_TDLAB_PERCENTSPIN },
	{ WC_STATIC,		IDS_TDC_FIELD_TIMEEST,		SS_CENTERIMAGE, 0, 0, 0, 40, 8, IDC_TDLAB_TIMEESTLABEL },
	{ WC_EDIT,			0, ES_AUTOHSCROLL | WS_TABSTOP, 0, 0, 0, 65, 13, IDC_TDLAB_TIMEEST },
	{ WC_STATIC,		IDS_TDC_FIELD_TIMESPENT,	SS_CENTERIMAGE, 0, 0, 0, 40, 8, IDC_TDLAB_TIMESPENTLABEL },
	{ WC_EDIT,			0, ES_AUTOHSCROLL | WS_TABSTOP, 0, 0, 0, 65, 13, IDC_TDLAB_TIMESPENT },
	{ WC_STATIC,		IDS_TDC_FIELD_STARTDATE,	SS_CENTERIMAGE, 0, 0, 0, 33, 8, IDC_TDLAB_STARTLABEL },
	{ WC_DATETIMEPICK,	0, DTS_RIGHTALIGN | DTS_SHOWNONE | WS_TABSTOP, 0, 0, 0, 65, 13, IDC_TDLAB_STARTDATE },
	{ WC_STATIC,		IDS_TDC_FIELD_STARTTIME,	SS_CENTERIMAGE, 0, 0, 0, 32, 8, IDC_TDLAB_STARTTIMELABEL },
	{ WC_COMBOBOX,		0, CBS_DROPDOWN | WS_VSCROLL | CBS_AUTOHSCROLL | WS_TABSTOP | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS, 0, 0, 0, 65, 13, IDC_TDLAB_STARTTIME },
	{ WC_STATIC,		IDS_TDC_FIELD_DUEDATE,		SS_CENTERIMAGE, 0, 0, 0, 32, 8, IDC_TDLAB_DUELABEL },
	{ WC_DATETIMEPICK,	0, DTS_RIGHTALIGN | DTS_SHOWNONE | WS_TABSTOP, 0, 0, 0, 65, 13, IDC_TDLAB_DUEDATE },
	{ WC_STATIC,		IDS_TDC_FIELD_DUETIME,		SS_CENTERIMAGE, 0, 0, 0, 32, 8, IDC_TDLAB_DUETIMELABEL },
	{ WC_COMBOBOX,		0, CBS_DROPDOWN | WS_VSCROLL | CBS_AUTOHSCROLL | WS_TABSTOP | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS, 0, 0, 0, 65, 13, IDC_TDLAB_DUETIME },
	{ WC_STATIC,		IDS_TDC_FIELD_DONEDATE,		SS_CENTERIMAGE, 0, 0, 0, 34, 8, IDC_TDLAB_DONELABEL },
	{ WC_DATETIMEPICK,	0, DTS_RIGHTALIGN | DTS_SHOWNONE | WS_TABSTOP, 0, 0, 0, 65, 13, IDC_TDLAB_DONEDATE },
	{ WC_STATIC,		IDS_TDC_FIELD_DONETIME,		SS_CENTERIMAGE, 0, 0, 0, 32, 8, IDC_TDLAB_DONETIMELABEL },
	{ WC_COMBOBOX,		0, CBS_DROPDOWN | WS_VSCROLL | CBS_AUTOHSCROLL | WS_TABSTOP | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS, 0, 0, 0, 65, 13, IDC_TDLAB_DONETIME },
	{ WC_STATIC,		IDS_TDC_FIELD_RECURRENCE,	SS_CENTERIMAGE, 0, 0, 0, 37, 8, IDC_TDLAB_RECURRENCELABEL },
	{ WC_EDIT,			0, ES_AUTOHSCROLL | ES_LEFT | WS_TABSTOP, 0, 0, 0, 65, 13, IDC_TDLAB_RECURRENCE },
	{ WC_STATIC,		IDS_TDC_FIELD_COLOUR,		SS_CENTERIMAGE, 0, 0, 0, 38, 8, IDC_TDLAB_COLOURLABEL },
	{ WC_BUTTON,		0, WS_TABSTOP, 0, 0, 0, 38, 13, IDC_TDLAB_COLOUR },
	{ WC_STATIC,		IDS_TDC_FIELD_ALLOCTO,		SS_CENTERIMAGE, 0, 0, 0, 38, 8, IDC_TDLAB_ALLOCTOLABEL },
	{ WC_COMBOBOX,		0, CBS_SORT | CBS_DROPDOWN | WS_VSCROLL | CBS_AUTOHSCROLL | WS_TABSTOP | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS, 0, 0, 0, 65, 200, IDC_TDLAB_ALLOCTO },
	{ WC_STATIC,		IDS_TDC_FIELD_ALLOCBY,		SS_CENTERIMAGE, 0, 0, 0, 38, 8, IDC_TDLAB_ALLOCBYLABEL },
	{ WC_COMBOBOX,		0, CBS_SORT | CBS_DROPDOWN | WS_VSCROLL | CBS_AUTOHSCROLL | WS_TABSTOP | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS, 0, 0, 0, 65, 200, IDC_TDLAB_ALLOCBY },
	{ WC_STATIC,		IDS_TDC_FIELD_STATUS,		SS_CENTERIMAGE, 0, 0, 0, 38, 8, IDC_TDLAB_STATUSLABEL },
	{ WC_COMBOBOX,		0, CBS_SORT | CBS_DROPDOWN | WS_VSCROLL | CBS_AUTOHSCROLL | WS_TABSTOP | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS, 0, 0, 0, 65, 200, IDC_TDLAB_STATUS },
	{ WC_STATIC,		IDS_TDC_FIELD_CATEGORY,		SS_CENTERIMAGE, 0, 0, 0, 38, 8, IDC_TDLAB_CATEGORYLABEL },
	{ WC_COMBOBOX,		0, CBS_SORT | CBS_DROPDOWN | WS_VSCROLL | CBS_AUTOHSCROLL | WS_TABSTOP | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS, 0, 0, 0, 65, 200, IDC_TDLAB_CATEGORY },
	{ WC_STATIC,		IDS_TDC_FIELD_TAGS,			SS_CENTERIMAGE, 0, 0, 0, 38, 8, IDC_TDLAB_TAGSLABEL },
	{ WC_COMBOBOX,		0, CBS_SORT | CBS_DROPDOWN | WS_VSCROLL | CBS_AUTOHSCROLL | WS_TABSTOP | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS, 0, 0, 0, 65, 200, IDC_TDLAB_TAGS },
	{ WC_STATIC,		IDS_TDC_FIELD_EXTID,		SS_CENTERIMAGE, 0, 0, 0, 37, 8, IDC_TDLAB_EXTERNALIDLABEL },
	{ WC_EDIT,			0, ES_AUTOHSCROLL | ES_LEFT | WS_TABSTOP, 0, 0, 0, 65, 13, IDC_TDLAB_EXTERNALID },
	{ WC_STATIC,		IDS_TDC_FIELD_COST,			SS_CENTERIMAGE, 0, 0, 0, 37, 8, IDC_TDLAB_COSTLABEL },
	{ WC_EDIT,			0, ES_AUTOHSCROLL | ES_LEFT | WS_TABSTOP, 0, 0, 0, 65, 13, IDC_TDLAB_COST },
	{ WC_STATIC,		IDS_TDC_FIELD_DEPENDENCY,	SS_CENTERIMAGE, 0, 0, 0, 37, 8, IDC_TDLAB_DEPENDSLABEL },
	{ WC_EDIT,			0, ES_AUTOHSCROLL | ES_LEFT | WS_TABSTOP, 0, 0, 0, 65, 13, IDC_TDLAB_DEPENDS },
	{ WC_STATIC,		IDS_TDC_FIELD_VERSION,		SS_CENTERIMAGE, 0, 0, 0, 37, 8, IDC_TDLAB_VERSIONLABEL },
	{ WC_COMBOBOX,		0, CBS_SORT | CBS_DROPDOWN | WS_VSCROLL | CBS_AUTOHSCROLL | WS_TABSTOP | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS, 0, 0, 0, 65, 200, IDC_TDLAB_VERSION },
	{ WC_STATIC,		IDS_TDC_FIELD_FILELINK,		SS_CENTERIMAGE, 0, 0, 0, 37, 8, IDC_TDLAB_FILEPATHLABEL },
	{ WC_COMBOBOX,		0, CBS_DROPDOWN | WS_VSCROLL | CBS_AUTOHSCROLL | WS_TABSTOP | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS, 0, 0, 0, 65, 13, IDC_TDLAB_FILEPATH },
};

static const CTRLITEM LEGACYCTRLITEMS[] =
{
	CTRLITEM(IDC_TDLAB_PRIORITY,		IDC_TDLAB_PRIORITYLABEL,	TDCA_PRIORITY),
	CTRLITEM(IDC_TDLAB_RISK,			IDC_TDLAB_RISKLABEL,		TDCA_RISK),
	CTRLITEM(IDC_TDLAB_PERCENT,			IDC_TDLAB_PERCENTLABEL,		TDCA_PERCENT),
	CTRLITEM(IDC_TDLAB_TIMEEST,			IDC_TDLAB_TIMEESTLABEL,		TDCA_TIMEESTIMATE),
	CTRLITEM(IDC_TDLAB_TIMESPENT,		IDC_TDLAB_TIMESPENTLABEL,	TDCA_TIMESPENT),
	CTRLITEM(IDC_TDLAB_STARTDATE,		IDC_TDLAB_STARTLABEL,		TDCA_STARTDATE),
	CTRLITEM(IDC_TDLAB_STARTTIME,		IDC_TDLAB_STARTTIMELABEL,	TDCA_STARTTIME),
	CTRLITEM(IDC_TDLAB_DUEDATE,			IDC_TDLAB_DUELABEL,			TDCA_DUEDATE),
	CTRLITEM(IDC_TDLAB_DUETIME,			IDC_TDLAB_DUETIMELABEL,		TDCA_DUETIME),
	CTRLITEM(IDC_TDLAB_DONEDATE,		IDC_TDLAB_DONELABEL,		TDCA_DONEDATE),
	CTRLITEM(IDC_TDLAB_DONETIME,		IDC_TDLAB_DONETIMELABEL,	TDCA_DONETIME),
	CTRLITEM(IDC_TDLAB_RECURRENCE,		IDC_TDLAB_RECURRENCELABEL,	TDCA_RECURRENCE),
	CTRLITEM(IDC_TDLAB_COLOUR,			IDC_TDLAB_COLOURLABEL,		TDCA_COLOR),
	CTRLITEM(IDC_TDLAB_ALLOCTO,			IDC_TDLAB_ALLOCTOLABEL,		TDCA_ALLOCTO),
	CTRLITEM(IDC_TDLAB_ALLOCBY,			IDC_TDLAB_ALLOCBYLABEL,		TDCA_ALLOCBY),
	CTRLITEM(IDC_TDLAB_STATUS,			IDC_TDLAB_STATUSLABEL,		TDCA_STATUS),
	CTRLITEM(IDC_TDLAB_CATEGORY,		IDC_TDLAB_CATEGORYLABEL,	TDCA_CATEGORY),
	CTRLITEM(IDC_TDLAB_TAGS,			IDC_TDLAB_TAGSLABEL,		TDCA_TAGS),
	CTRLITEM(IDC_TDLAB_EXTERNALID,		IDC_TDLAB_EXTERNALIDLABEL,	TDCA_EXTERNALID),
	CTRLITEM(IDC_TDLAB_COST,			IDC_TDLAB_COSTLABEL,		TDCA_COST),
	CTRLITEM(IDC_TDLAB_DEPENDS,			IDC_TDLAB_DEPENDSLABEL,		TDCA_DEPENDENCY),
	CTRLITEM(IDC_TDLAB_VERSION,			IDC_TDLAB_VERSIONLABEL,		TDCA_VERSION),
	CTRLITEM(IDC_TDLAB_FILEPATH,		IDC_TDLAB_FILEPATHLABEL,	TDCA_FILELINK),
};

static const int NUM_LEGACYCONTROLS = _countof(LEGACYCONTROLS);
static const int NUM_LEGACYCTRLITEMS = _countof(LEGACYCTRLITEMS);

/////////////////////////////////////////////////////////////////////////////

CTDLTaskAttributeBarCtrl::CTDLTaskAttributeBarCtrl(const CToDoCtrlData& data,
												   const CContentMgr& mgrContent,
												   const CTDCImageList& ilIcons,
												   const TDCCOLEDITVISIBILITY& vis,
												   const CTDCCustomAttribDefinitionArray& aCustAttribDefs)
	:
	m_data(data),
	m_mgrContent(mgrContent),
	m_ilIcons(ilIcons),
	m_vis(vis),
	m_aCustAttribDefs(aCustAttribDefs),
	m_multitasker(data, mgrContent),
	m_nPercentDoneIncrement(10),
	m_nNumPriorityRiskLevels(10),
	m_dwTimeTrackTaskID(0),
	m_bRefreshing(FALSE),
	m_crBack(CLR_NONE),
	m_nPriority(FM_NOPRIORITYORRISK),
	m_nRisk(FM_NOPRIORITYORRISK),
	m_nPercentDone(0),
	m_crColour(CLR_DEFAULT),
	m_cbAllocBy(ACBS_ALLOWDELETE | ACBS_AUTOCOMPLETE),
	m_cbAllocTo(ACBS_ALLOWDELETE | ACBS_AUTOCOMPLETE),
	m_cbCategory(ACBS_ALLOWDELETE | ACBS_AUTOCOMPLETE),
	m_cbFileLink(FES_GOBUTTON | FES_ALLOWURL | FES_RELATIVEPATHS),
	m_cbStatus(ACBS_ALLOWDELETE | ACBS_AUTOCOMPLETE),
	m_cbTags(ACBS_ALLOWDELETE | ACBS_AUTOCOMPLETE),
	m_cbTimeDone(TCB_HALFHOURS | TCB_NOTIME | TCB_HOURSINDAY),
	m_cbTimeDue(TCB_HALFHOURS | TCB_NOTIME | TCB_HOURSINDAY),
	m_cbTimeStart(TCB_HALFHOURS | TCB_NOTIME | TCB_HOURSINDAY),
	m_cbVersion(ACBS_ALLOWDELETE | ACBS_AUTOCOMPLETE),
	m_cbPriority(FALSE),
	m_cbRisk(FALSE)
{
	SetBorders(0);
	BuildControls();
}

CTDLTaskAttributeBarCtrl::~CTDLTaskAttributeBarCtrl()
{
}

BEGIN_MESSAGE_MAP(CTDLTaskAttributeBarCtrl, CRuntimeDlg)
	ON_WM_SIZE()
	ON_WM_DESTROY()
	ON_WM_ERASEBKGND()
	ON_WM_CTLCOLOR()
	ON_MESSAGE(CPN_SELENDOK, OnColourChanged)
	ON_REGISTERED_MESSAGE(WM_EE_BTNCLICK, OnEEBtnClick)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////

BOOL CTDLTaskAttributeBarCtrl::Create(CWnd* pParent, UINT nID, const CRect& rect)
{
	CLocalizer::IgnoreString(_T("TDLTaskAttributeBar"));

	DWORD dwStyle = (WS_CHILD | WS_VISIBLE | WS_TABSTOP | DS_SETFONT);

	return CRuntimeDlg::Create(_T("TDLTaskAttributeBar"), dwStyle, WS_EX_CONTROLPARENT, rect, pParent, nID);
}

void CTDLTaskAttributeBarCtrl::BuildControls()
{
	for (int nCtrl = 0; nCtrl < NUM_LEGACYCONTROLS; nCtrl++)
	{
		const TDCCONTROL& ctrl = LEGACYCONTROLS[nCtrl];

		CString sCaption;

		if (ctrl.nIDCaption)
			sCaption = CEnString(ctrl.nIDCaption);

		AddRCControl(_T("CONTROL"), ctrl.szClass, sCaption,
					 (ctrl.dwStyle | WS_NOTVISIBLE), ctrl.dwExStyle,
					 ctrl.nX, ctrl.nY, ctrl.nCx, ctrl.nCy, ctrl.nID);
	}
}

void CTDLTaskAttributeBarCtrl::DoDataExchange(CDataExchange* pDX)
{
	CRuntimeDlg::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_TDLAB_ALLOCBY, m_cbAllocBy);
	DDX_Control(pDX, IDC_TDLAB_ALLOCTO, m_cbAllocTo);
	DDX_Control(pDX, IDC_TDLAB_CATEGORY, m_cbCategory);
	DDX_Control(pDX, IDC_TDLAB_COLOUR, m_cpColour);
	DDX_Control(pDX, IDC_TDLAB_COST, m_eCost);
	DDX_Control(pDX, IDC_TDLAB_DEPENDS, m_eDependency);
	DDX_Control(pDX, IDC_TDLAB_DONEDATE, m_dtcDone);
	DDX_Control(pDX, IDC_TDLAB_DONETIME, m_cbTimeDone);
	DDX_Control(pDX, IDC_TDLAB_DUEDATE, m_dtcDue);
	DDX_Control(pDX, IDC_TDLAB_DUETIME, m_cbTimeDue);
	DDX_Control(pDX, IDC_TDLAB_EXTERNALID, m_eExternalID);
	DDX_Control(pDX, IDC_TDLAB_FILEPATH, m_cbFileLink);
	DDX_Control(pDX, IDC_TDLAB_PERCENT, m_ePercentDone);
	DDX_Control(pDX, IDC_TDLAB_PERCENTSPIN, m_spinPercent);
	DDX_Control(pDX, IDC_TDLAB_PRIORITY, m_cbPriority);
	DDX_Control(pDX, IDC_TDLAB_RECURRENCE, m_eRecurrence);
	DDX_Control(pDX, IDC_TDLAB_RISK, m_cbRisk);
	DDX_Control(pDX, IDC_TDLAB_STARTDATE, m_dtcStart);
	DDX_Control(pDX, IDC_TDLAB_STARTTIME, m_cbTimeStart);
	DDX_Control(pDX, IDC_TDLAB_STATUS, m_cbStatus);
	DDX_Control(pDX, IDC_TDLAB_TAGS, m_cbTags);
	DDX_Control(pDX, IDC_TDLAB_TIMEEST, m_eTimeEstimate);
	DDX_Control(pDX, IDC_TDLAB_TIMESPENT, m_eTimeSpent);
	DDX_Control(pDX, IDC_TDLAB_VERSION, m_cbVersion);

	DDX_Text(pDX, IDC_TDLAB_EXTERNALID, m_sExternalID);
	DDX_ColourPicker(pDX, IDC_TDLAB_COLOUR, m_crColour);

	CTDCDialogHelper::DDX_Text(pDX, IDC_TDLAB_COST, m_cost);
	CTDCDialogHelper::DDX_Text(pDX, IDC_TDLAB_PERCENT, m_nPercentDone, m_spinPercent);

	CTDCDialogHelper::DDX_Text(pDX, m_eTimeEstimate, m_timeEstimate);
	CTDCDialogHelper::DDX_Text(pDX, m_eTimeSpent, m_timeSpent);

	m_cbAllocBy.DDX(pDX, m_sAllocBy);
	m_cbStatus.DDX(pDX, m_sStatus);
	m_cbVersion.DDX(pDX, m_sVersion);
	m_cbPriority.DDX(pDX, m_nPriority);
	m_cbRisk.DDX(pDX, m_nRisk);
	m_eRecurrence.DDX(pDX, m_tRecurrence);
	m_cbFileLink.DDX(pDX, m_aFileLinks);
	m_eDependency.DDX(pDX, m_aDepends);
}

BOOL CTDLTaskAttributeBarCtrl::OnInitDialog()
{
	CRuntimeDlg::OnInitDialog();

	m_ePercentDone.SetMask(_T("0123456789"));
	m_eCost.SetMask(_T("@-.0123456789"), ME_LOCALIZEDECIMAL);
	m_cpColour.SetSelectionMode(CP_MODE_TEXT);
	m_cpColour.SetWindowText(CEnString(IDS_COLOR_SAMPLETEXT));
	m_cpColour.SetDefaultText(CEnString(IDS_COLOR_AUTOMATIC));
	m_cpColour.SetCustomText(CEnString(IDS_COLOR_MORECOLORS));
	m_cpColour.SetBkColour(CLR_DEFAULT);
	m_cpColour.SetTextColour(CLR_DEFAULT);

	m_spinPercent.SetRange(0, 100);
	m_spinPercent.SetBuddy(GetDlgItem(IDC_TDLAB_PERCENT));

	m_iconTrackTime.Load(IDI_TIMETRACK, 16, FALSE);
	m_iconAddTime.Load(IDI_ADD_LOGGED_TIME, 16, FALSE);
	m_iconLink.Load(IDI_DEPENDS_LINK, 16, FALSE);

	m_eTimeSpent.InsertButton(1, IDC_TDLAB_TIME_TRACK, m_iconTrackTime, CEnString(IDS_TDC_STARTSTOPCLOCK), 15);
	m_eTimeSpent.InsertButton(2, IDC_TDLAB_ADD_TIME, m_iconAddTime, CEnString(IDS_TDC_ADDLOGGEDTIME), 15);

	m_eDependency.AddButton(IDC_TDLAB_DEPENDS_EDIT, _T("..."), CEnString(IDS_OPTIONS));
	m_eDependency.AddButton(IDC_TDLAB_DEPENDS_LINK, m_iconLink, _T("Select dependency"));

	SetNumPriorityRiskLevels(m_nNumPriorityRiskLevels);
	SetPercentDoneIncrement(m_nPercentDoneIncrement);
	SetPriorityColors(m_aPriorityColors);
	PopulateAutoLists();
	RebuildCustomControls();
	ShowHideControls();

	return TRUE;
}

BOOL CTDLTaskAttributeBarCtrl::OnEraseBkgnd(CDC* pDC)
{
	if (m_crBack == CLR_NONE)
		return CRuntimeDlg::OnEraseBkgnd(pDC);

	CRect rClient;
	GetClientRect(rClient);
	pDC->FillSolidRect(rClient, m_crBack);

	return TRUE;
}

void CTDLTaskAttributeBarCtrl::OnSize(UINT nType, int cx, int cy)
{
	CRuntimeDlg::OnSize(nType, cx, cy);
	ReposControls();
}

void CTDLTaskAttributeBarCtrl::SetUITheme(const CUIThemeFile& theme)
{
	SetBackgroundColor(theme.crAppBackLight);
}

void CTDLTaskAttributeBarCtrl::SetBackgroundColor(COLORREF crBack)
{
	m_crBack = crBack;

	m_brBack.DeleteObject();

	if (m_crBack != CLR_NONE)
		m_brBack.CreateSolidBrush(m_crBack);

	if (GetSafeHwnd())
		Invalidate();
}

HBRUSH CTDLTaskAttributeBarCtrl::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CRuntimeDlg::OnCtlColor(pDC, pWnd, nCtlColor);

	if ((nCtlColor == CTLCOLOR_STATIC) && m_brBack.GetSafeHandle())
	{
		pDC->SetBkMode(TRANSPARENT);
		return (HBRUSH)m_brBack.GetSafeHandle();
	}

	return hbr;
}

BOOL CTDLTaskAttributeBarCtrl::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if (!m_bRefreshing)
	{
		UINT nID = LOWORD(wParam);
		UINT nCode = HIWORD(wParam);
		TDC_ATTRIBUTE nAttrib = GetAttributeID(nID);

		if ((nAttrib == TDCA_NONE) && !GetCustomAttributeID(nID, nAttrib))
			nAttrib = TDCA_NONE;

		if (nAttrib != TDCA_NONE)
		{
			switch (nCode)
			{
			case EN_CHANGE:
			case EN_KILLFOCUS:
			case CBN_SELCHANGE:
			case CBN_KILLFOCUS:
			case BN_CLICKED:
				NotifyParent(nAttrib);
				return TRUE;
			}
		}
	}

	return CRuntimeDlg::OnCommand(wParam, lParam);
}

BOOL CTDLTaskAttributeBarCtrl::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	NMHDR* pNMHDR = (NMHDR*)lParam;

	if (!m_bRefreshing && pNMHDR)
	{
		if (pNMHDR->code == DTN_DATETIMECHANGE)
		{
			TDC_ATTRIBUTE nAttrib = GetAttributeID(pNMHDR->idFrom);

			if ((nAttrib == TDCA_NONE) && !GetCustomAttributeID(pNMHDR->idFrom, nAttrib))
				nAttrib = TDCA_NONE;

			if (nAttrib != TDCA_NONE)
			{
				NotifyParent(nAttrib);
				return TRUE;
			}
		}
		else if ((pNMHDR->code == UDN_DELTAPOS) && (pNMHDR->idFrom == IDC_TDLAB_PERCENTSPIN))
		{
			if (GetParent())
				GetParent()->PostMessage(WM_TDCN_ATTRIBUTEEDITED, TDCA_PERCENT, 0);

			return FALSE;
		}
	}

	return CRuntimeDlg::OnNotify(wParam, lParam, pResult);
}

void CTDLTaskAttributeBarCtrl::NotifyParent(TDC_ATTRIBUTE nAttribID)
{
	if (GetParent())
		GetParent()->SendMessage(WM_TDCN_ATTRIBUTEEDITED, nAttribID, 0);
}

LRESULT CTDLTaskAttributeBarCtrl::OnColourChanged(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	if (!m_bRefreshing)
		NotifyParent(TDCA_COLOR);

	return 0;
}

LRESULT CTDLTaskAttributeBarCtrl::OnEEBtnClick(WPARAM wParam, LPARAM lParam)
{
	CWnd* pTDC = (GetParent() ? GetParent()->GetParent() : NULL);

	if (!pTDC)
		return 0;

	UINT nCtrlID = (UINT)wParam;
	UINT nBtnID = (UINT)lParam;

	if (nCtrlID == IDC_TDLAB_TIMESPENT)
	{
		switch (nBtnID)
		{
		case IDC_TDLAB_TIME_TRACK:
			pTDC->SendMessage(WM_TDCM_TOGGLETIMETRACKING);
			break;

		case IDC_TDLAB_ADD_TIME:
			pTDC->SendMessage(WM_TDCM_ADDTIMETOLOGFILE);
			break;
		}
	}
	else if (nCtrlID == IDC_TDLAB_DEPENDS)
	{
		switch (nBtnID)
		{
		case IDC_TDLAB_DEPENDS_LINK:
			pTDC->SendMessage(WM_TDCM_SELECTDEPENDENCIES);
			break;

		case IDC_TDLAB_DEPENDS_EDIT:
			pTDC->SendMessage(WM_TDCM_EDITTASKATTRIBUTE, TDCA_DEPENDENCY);
			break;
		}
	}

	return 0;
}

void CTDLTaskAttributeBarCtrl::OnCustomAttributesChange()
{
	RebuildCustomControls();
	ShowHideCustomControls();
	RefreshAllValues();
	ReposControls();
}

void CTDLTaskAttributeBarCtrl::OnDestroy()
{
	CTDCCustomEditAttributeUIHelper::CleanupControls(m_aCustomControls, this);

	CRuntimeDlg::OnDestroy();
}

void CTDLTaskAttributeBarCtrl::OnAttributeVisibilityChange()
{
	ShowHideControls();
}

CDateTimeCtrlEx* CTDLTaskAttributeBarCtrl::GetDateCtrl(UINT nCtrlID)
{
	switch (nCtrlID)
	{
	case IDC_TDLAB_STARTDATE:	return &m_dtcStart;
	case IDC_TDLAB_DUEDATE:		return &m_dtcDue;
	case IDC_TDLAB_DONEDATE:		return &m_dtcDone;
	}

	return NULL;
}

TDC_ATTRIBUTE CTDLTaskAttributeBarCtrl::GetAttributeID(UINT nCtrlID)
{
	switch (nCtrlID)
	{
	case IDC_TDLAB_PRIORITY:		return TDCA_PRIORITY;
	case IDC_TDLAB_RISK:			return TDCA_RISK;
	case IDC_TDLAB_PERCENT:			return TDCA_PERCENT;
	case IDC_TDLAB_TIMEEST:			return TDCA_TIMEESTIMATE;
	case IDC_TDLAB_TIMESPENT:		return TDCA_TIMESPENT;
	case IDC_TDLAB_STARTDATE:		return TDCA_STARTDATE;
	case IDC_TDLAB_STARTTIME:		return TDCA_STARTTIME;
	case IDC_TDLAB_DUEDATE:			return TDCA_DUEDATE;
	case IDC_TDLAB_DUETIME:			return TDCA_DUETIME;
	case IDC_TDLAB_DONEDATE:		return TDCA_DONEDATE;
	case IDC_TDLAB_DONETIME:		return TDCA_DONETIME;
	case IDC_TDLAB_RECURRENCE:		return TDCA_RECURRENCE;
	case IDC_TDLAB_COLOUR:			return TDCA_COLOR;
	case IDC_TDLAB_ALLOCTO:			return TDCA_ALLOCTO;
	case IDC_TDLAB_ALLOCBY:			return TDCA_ALLOCBY;
	case IDC_TDLAB_STATUS:			return TDCA_STATUS;
	case IDC_TDLAB_CATEGORY:		return TDCA_CATEGORY;
	case IDC_TDLAB_TAGS:			return TDCA_TAGS;
	case IDC_TDLAB_EXTERNALID:		return TDCA_EXTERNALID;
	case IDC_TDLAB_COST:			return TDCA_COST;
	case IDC_TDLAB_DEPENDS:			return TDCA_DEPENDENCY;
	case IDC_TDLAB_VERSION:			return TDCA_VERSION;
	case IDC_TDLAB_FILEPATH:		return TDCA_FILELINK;
	}

	return TDCA_NONE;
}

void CTDLTaskAttributeBarCtrl::ShowHideControls()
{
	for (int nCtrl = 0; nCtrl < NUM_LEGACYCTRLITEMS; nCtrl++)
	{
		const CTRLITEM& ctrl = LEGACYCTRLITEMS[nCtrl];
		BOOL bShow = m_vis.IsEditFieldVisible(ctrl.nAttributeID);

		if (GetDlgItem(ctrl.nCtrlID))
			GetDlgItem(ctrl.nCtrlID)->ShowWindow(bShow ? SW_SHOW : SW_HIDE);

		if (GetDlgItem(ctrl.nLabelID))
			GetDlgItem(ctrl.nLabelID)->ShowWindow(bShow ? SW_SHOW : SW_HIDE);

		if (ctrl.nCtrlID == IDC_TDLAB_PERCENT)
			m_spinPercent.ShowWindow(bShow ? SW_SHOW : SW_HIDE);
	}

	ShowHideCustomControls();
	ReposControls();
}

void CTDLTaskAttributeBarCtrl::ShowHideCustomControls()
{
	for (int nCtrl = 0; nCtrl < m_aCustomControls.GetSize(); nCtrl++)
	{
		const CUSTOMATTRIBCTRLITEM& ctrl = m_aCustomControls[nCtrl];

		if (GetDlgItem(ctrl.nCtrlID))
			GetDlgItem(ctrl.nCtrlID)->ShowWindow(SW_SHOW);

		if (GetDlgItem(ctrl.nLabelID))
			GetDlgItem(ctrl.nLabelID)->ShowWindow(SW_SHOW);

		if (ctrl.HasBuddy())
		{
			if (GetDlgItem(ctrl.nBuddyCtrlID))
				GetDlgItem(ctrl.nBuddyCtrlID)->ShowWindow(SW_SHOW);

			if (GetDlgItem(ctrl.nBuddyLabelID))
				GetDlgItem(ctrl.nBuddyLabelID)->ShowWindow(SW_SHOW);
		}
	}
}

int CTDLTaskAttributeBarCtrl::GetVisibleCustomControlCount() const
{
	int nCount = 0;

	for (int nCtrl = 0; nCtrl < m_aCustomControls.GetSize(); nCtrl++)
	{
		nCount++;

		if (m_aCustomControls[nCtrl].HasBuddy())
			nCount++;
	}

	return nCount;
}

void CTDLTaskAttributeBarCtrl::ReposCustomControls(int nStartCell, int nCols, int nCellH, int nCtrlW, int nLabelH, int nCtrlH, int nHSpacing, const CRect& /*rClient*/)
{
	int nCell = nStartCell;

	for (int nCtrl = 0; nCtrl < m_aCustomControls.GetSize(); nCtrl++)
	{
		const CUSTOMATTRIBCTRLITEM& ctrl = m_aCustomControls[nCtrl];

		int nX = ((nCell % nCols) * (nCtrlW + nHSpacing));
		int nY = ((nCell / nCols) * nCellH);

		if (GetDlgItem(ctrl.nLabelID))
			GetDlgItem(ctrl.nLabelID)->MoveWindow(nX, nY, nCtrlW, nLabelH);

		if (GetDlgItem(ctrl.nCtrlID))
			GetDlgItem(ctrl.nCtrlID)->MoveWindow(nX, nY + nLabelH, nCtrlW, nCtrlH);

		nCell++;

		if (ctrl.HasBuddy())
		{
			nX = ((nCell % nCols) * (nCtrlW + nHSpacing));
			nY = ((nCell / nCols) * nCellH);

			if (GetDlgItem(ctrl.nBuddyLabelID))
				GetDlgItem(ctrl.nBuddyLabelID)->MoveWindow(nX, nY, nCtrlW, nLabelH);

			if (GetDlgItem(ctrl.nBuddyCtrlID))
				GetDlgItem(ctrl.nBuddyCtrlID)->MoveWindow(nX, nY + nLabelH, nCtrlW, nCtrlH);

			nCell++;
		}
	}
}

void CTDLTaskAttributeBarCtrl::RebuildCustomControls()
{
	CTDCCustomEditAttributeUIHelper::CleanupControls(m_aCustomControls, this);
	CTDCCustomEditAttributeUIHelper::RebuildEditControls(this,
														 m_aCustAttribDefs,
														 m_ilIcons,
														 IDC_TDLAB_FILEPATH,
														 FALSE,
														 m_aCustomControls);
}

BOOL CTDLTaskAttributeBarCtrl::GetCustomAttributeID(UINT nCtrlID, TDC_ATTRIBUTE& nAttribID) const
{
	CUSTOMATTRIBCTRLITEM ctrl;

	if (CTDCCustomEditAttributeUIHelper::GetControl(nCtrlID, m_aCustomControls, ctrl))
	{
		nAttribID = ctrl.nAttributeID;
		return TRUE;
	}

	return FALSE;
}
void CTDLTaskAttributeBarCtrl::ReposControls()
{
	if (!GetSafeHwnd())
		return;

	CRect rClient;
	GetClientRect(rClient);

	if (rClient.IsRectEmpty())
		return;

	CDlgUnits dlu(this);
	const int nCtrlH = dlu.ToPixelsY(13);
	const int nLabelH = dlu.ToPixelsY(8);
	const int nVSpacing = dlu.ToPixelsY(4);
	const int nCtrlW = dlu.ToPixelsX(80);
	const int nHSpacing = dlu.ToPixelsX(6);
	const int nCellH = (nLabelH + nCtrlH + nVSpacing);

	int nVisible = 0;

	for (int nCtrl = 0; nCtrl < NUM_LEGACYCTRLITEMS; nCtrl++)
	{
		if (m_vis.IsEditFieldVisible(LEGACYCTRLITEMS[nCtrl].nAttributeID))
			nVisible++;
	}

	nVisible += GetVisibleCustomControlCount();

	if (!nVisible)
		return;

	int nCols = max(1, rClient.Width() / (nCtrlW + nHSpacing));
	int nRow = 0, nCol = 0, nCell = 0;

	for (int nCtrl = 0; nCtrl < NUM_LEGACYCTRLITEMS; nCtrl++)
	{
		const CTRLITEM& ctrl = LEGACYCTRLITEMS[nCtrl];

		if (!m_vis.IsEditFieldVisible(ctrl.nAttributeID))
			continue;

		int nX = (nCol * (nCtrlW + nHSpacing));
		int nY = (nRow * nCellH);

		CWnd* pLabel = GetDlgItem(ctrl.nLabelID);
		CWnd* pCtrl = GetDlgItem(ctrl.nCtrlID);

		if (pLabel)
			pLabel->MoveWindow(nX, nY, nCtrlW, nLabelH);

		if (pCtrl)
		{
			if (ctrl.nCtrlID == IDC_TDLAB_PERCENT)
			{
				const int nSpinW = dlu.ToPixelsX(13);

				pCtrl->MoveWindow(nX, nY + nLabelH, nCtrlW - nSpinW, nCtrlH);

				if (m_spinPercent.GetSafeHwnd())
					m_spinPercent.MoveWindow(nX + nCtrlW - nSpinW, nY + nLabelH, nSpinW, nCtrlH);
			}
			else if ((ctrl.nCtrlID == IDC_TDLAB_FILEPATH) && (GetVisibleCustomControlCount() == 0))
			{
				pCtrl->MoveWindow(nX, nY + nLabelH, (rClient.right - nX), nCtrlH);
			}
			else
			{
				pCtrl->MoveWindow(nX, nY + nLabelH, nCtrlW, nCtrlH);
			}
		}

		if (++nCol == nCols)
		{
			nCol = 0;
			nRow++;
		}

		nCell++;
	}

	ReposCustomControls(nCell, nCols, nCellH, nCtrlW, nLabelH, nCtrlH, nHSpacing, rClient);
}

int CTDLTaskAttributeBarCtrl::GetRequiredHeight(int cx) const
{
	int nVisible = 0;

	for (int nCtrl = 0; nCtrl < NUM_LEGACYCTRLITEMS; nCtrl++)
	{
		if (m_vis.IsEditFieldVisible(LEGACYCTRLITEMS[nCtrl].nAttributeID))
			nVisible++;
	}

	nVisible += GetVisibleCustomControlCount();

	if (!nVisible)
		return 0;

	CDlgUnits dlu(GetSafeHwnd() ? (const CWnd*)this : AfxGetMainWnd());
	const int nCtrlH = dlu.ToPixelsY(13);
	const int nLabelH = dlu.ToPixelsY(8);
	const int nVSpacing = dlu.ToPixelsY(4);
	const int nCtrlW = dlu.ToPixelsX(80);
	const int nHSpacing = dlu.ToPixelsX(6);
	const int nCellH = (nLabelH + nCtrlH + nVSpacing);

	int nCols = max(1, cx / (nCtrlW + nHSpacing));
	int nRows = ((nVisible + nCols - 1) / nCols);

	return (nRows * nCellH);
}

int CTDLTaskAttributeBarCtrl::GetRequiredWidth(int cy) const
{
	int nVisible = 0;

	for (int nCtrl = 0; nCtrl < NUM_LEGACYCTRLITEMS; nCtrl++)
	{
		if (m_vis.IsEditFieldVisible(LEGACYCTRLITEMS[nCtrl].nAttributeID))
			nVisible++;
	}

	nVisible += GetVisibleCustomControlCount();

	if (!nVisible)
		return 0;

	CDlgUnits dlu(GetSafeHwnd() ? (const CWnd*)this : AfxGetMainWnd());
	const int nCtrlH = dlu.ToPixelsY(13);
	const int nLabelH = dlu.ToPixelsY(8);
	const int nVSpacing = dlu.ToPixelsY(4);
	const int nCtrlW = dlu.ToPixelsX(80);
	const int nHSpacing = dlu.ToPixelsX(6);
	const int nCellH = (nLabelH + nCtrlH + nVSpacing);

	int nRows = max(1, cy / nCellH);
	int nCols = ((nVisible + nRows - 1) / nRows);

	return ((nCols * nCtrlW) + ((nCols - 1) * nHSpacing));
}

void CTDLTaskAttributeBarCtrl::SetCtrlDate(CDateTimeCtrl& ctrl, const COleDateTime& date)
{
	if (CDateHelper::IsDateSet(date))
	{
		ctrl.SetTime(CDateHelper::GetDateOnly(date));
	}
	else
	{
		ctrl.SetTime(CDateHelper::GetDateOnly(COleDateTime::GetCurrentTime()));
		ctrl.SendMessage(DTM_SETSYSTEMTIME, GDT_NONE, 0);
	}

	ctrl.Invalidate(FALSE);
}

void CTDLTaskAttributeBarCtrl::EnableTimeCtrl(CTimeComboBox& ctrl, const COleDateTime& date)
{
	BOOL bHasDate = CDateHelper::IsDateSet(date);

	ctrl.EnableWindow(bHasDate);

	if (!bHasDate)
		ctrl.SetOleTime(-1);
}

void CTDLTaskAttributeBarCtrl::PopulateAutoLists()
{
	if (!GetSafeHwnd())
		return;

	m_cbAllocBy.ResetContent();
	m_cbStatus.ResetContent();
	m_cbVersion.ResetContent();
	m_cbAllocTo.ResetContent();
	m_cbCategory.ResetContent();
	m_cbTags.ResetContent();

	for (int nItem = 0; nItem < m_tldAll.aAllocBy.GetSize(); nItem++)
		m_cbAllocBy.AddUniqueItem(m_tldAll.aAllocBy[nItem]);

	for (int nItem = 0; nItem < m_tldAll.aStatus.GetSize(); nItem++)
		m_cbStatus.AddUniqueItem(m_tldAll.aStatus[nItem]);

	for (int nItem = 0; nItem < m_tldAll.aVersion.GetSize(); nItem++)
		m_cbVersion.AddUniqueItem(m_tldAll.aVersion[nItem]);

	for (int nItem = 0; nItem < m_tldAll.aAllocTo.GetSize(); nItem++)
		m_cbAllocTo.AddUniqueItem(m_tldAll.aAllocTo[nItem]);

	for (int nItem = 0; nItem < m_tldAll.aCategory.GetSize(); nItem++)
		m_cbCategory.AddUniqueItem(m_tldAll.aCategory[nItem]);

	for (int nItem = 0; nItem < m_tldAll.aTags.GetSize(); nItem++)
		m_cbTags.AddUniqueItem(m_tldAll.aTags[nItem]);
}

void CTDLTaskAttributeBarCtrl::PopulateAutoList(TDC_ATTRIBUTE nAttribID)
{
	PopulateAutoLists();
}

void CTDLTaskAttributeBarCtrl::SetDefaultAutoListData(const TDCAUTOLISTDATA& tldDefault)
{
	m_tldDefault.Copy(tldDefault, TDCA_ALL);
}

void CTDLTaskAttributeBarCtrl::SetAutoListData(TDC_ATTRIBUTE nAttribID, const TDCAUTOLISTDATA& tld)
{
	m_tldAll.Copy(tld, nAttribID);
	PopulateAutoLists();
}

void CTDLTaskAttributeBarCtrl::GetAutoListData(TDC_ATTRIBUTE nAttribID, TDCAUTOLISTDATA& tld) const
{
	tld.Copy(m_tldAll, nAttribID);
}

void CTDLTaskAttributeBarCtrl::SetAutoListDataReadOnly(TDC_ATTRIBUTE nAttribID, BOOL bReadOnly)
{
	if (bReadOnly)
	{
		BOOL bValue = TRUE;
		m_mapReadOnlyAutoList.SetAt(nAttribID, bValue);
	}
	else
		m_mapReadOnlyAutoList.RemoveKey(nAttribID);
}

BOOL CTDLTaskAttributeBarCtrl::IsAutoListDataReadOnly(TDC_ATTRIBUTE nAttribID) const
{
	BOOL bReadOnly = FALSE;
	m_mapReadOnlyAutoList.Lookup(nAttribID, bReadOnly);
	return bReadOnly;
}

void CTDLTaskAttributeBarCtrl::SetCurrentFolder(const CString& sFolder)
{
	m_sCurrentFolder = sFolder;

	if (m_cbFileLink.GetSafeHwnd())
		m_cbFileLink.SetCurrentFolder(sFolder);
}

void CTDLTaskAttributeBarCtrl::SetCompletionStatus(const CString& sStatus)
{
	m_sCompletionStatus = sStatus;
}

void CTDLTaskAttributeBarCtrl::SetPriorityColors(const CDWordArray& aColors)
{
	if (&aColors != &m_aPriorityColors)
		m_aPriorityColors.Copy(aColors);

	if (m_cbPriority.GetSafeHwnd() && m_aPriorityColors.GetSize())
		m_cbPriority.SetColors(m_aPriorityColors);
}

void CTDLTaskAttributeBarCtrl::SetPercentDoneIncrement(int nAmount)
{
	m_nPercentDoneIncrement = max(1, nAmount);

	if (m_spinPercent.GetSafeHwnd())
	{
		UDACCEL uda = { 0, (UINT)m_nPercentDoneIncrement };
		m_spinPercent.SetAccel(1, &uda);
	}
}

void CTDLTaskAttributeBarCtrl::SetTimeTrackTaskID(DWORD dwTaskID)
{
	m_dwTimeTrackTaskID = dwTaskID;

	if (m_eTimeSpent.GetSafeHwnd())
		m_eTimeSpent.CheckButton(IDC_TDLAB_TIME_TRACK, (dwTaskID != 0));
}

void CTDLTaskAttributeBarCtrl::SetNumPriorityRiskLevels(int nNumLevels)
{
	m_nNumPriorityRiskLevels = max(1, nNumLevels);

	if (m_cbPriority.GetSafeHwnd())
		m_cbPriority.SetNumLevels(m_nNumPriorityRiskLevels);

	if (m_cbRisk.GetSafeHwnd())
		m_cbRisk.SetNumLevels(m_nNumPriorityRiskLevels);
}

BOOL CTDLTaskAttributeBarCtrl::SetSelectedTaskIDs(const CDWordArray& aTaskIDs)
{
	if (&aTaskIDs != &m_aSelectedTaskIDs)
		m_aSelectedTaskIDs.Copy(aTaskIDs);

	RefreshAllValues();
	EnableDisableControls();

	return m_aSelectedTaskIDs.GetSize();
}

BOOL CTDLTaskAttributeBarCtrl::IsAnyTaskSelected(const CDWordArray& aTaskIDs) const
{
	for (int nID = 0; nID < aTaskIDs.GetSize(); nID++)
	{
		for (int nSel = 0; nSel < m_aSelectedTaskIDs.GetSize(); nSel++)
		{
			if (m_aSelectedTaskIDs[nSel] == aTaskIDs[nID])
				return TRUE;
		}
	}

	return FALSE;
}

TDC_ATTRIBUTE CTDLTaskAttributeBarCtrl::GetSelectedAttributeID() const
{
	return TDCA_NONE;
}

CString CTDLTaskAttributeBarCtrl::GetSelectedAttributeLabel() const
{
	return _T("");
}

void CTDLTaskAttributeBarCtrl::RefreshSelectedTasksValues(const CTDCAttributeMap& /*mapAttribIDs*/)
{
	RefreshAllValues();
}

void CTDLTaskAttributeBarCtrl::RefreshSelectedTasksValue(TDC_ATTRIBUTE /*nAttribID*/)
{
	RefreshAllValues();
}

void CTDLTaskAttributeBarCtrl::RefreshDateTimeFormatting()
{
	RefreshAllValues();
}

void CTDLTaskAttributeBarCtrl::RefreshAllValues()
{
	if (!GetSafeHwnd())
		return;

	m_bRefreshing = TRUE;

	if (!m_aSelectedTaskIDs.GetSize())
	{
		ClearValues();
	}
	else
	{
		CStringArray aMatched, aMixed;
		int nValue = 0;
		COleDateTime date;

		m_nPriority = m_multitasker.GetTasksPriority(m_aSelectedTaskIDs, nValue) ? nValue : FM_NOPRIORITYORRISK;
		m_nRisk = m_multitasker.GetTasksRisk(m_aSelectedTaskIDs, nValue) ? nValue : FM_NOPRIORITYORRISK;
		m_nPercentDone = m_multitasker.GetTasksPercentDone(m_aSelectedTaskIDs, nValue) ? nValue : 0;

		m_sAllocBy = m_multitasker.GetTasksAllocatedBy(m_aSelectedTaskIDs, m_sAllocBy) ? m_sAllocBy : _T("");
		m_sStatus = m_multitasker.GetTasksStatus(m_aSelectedTaskIDs, m_sStatus) ? m_sStatus : _T("");
		m_sVersion = m_multitasker.GetTasksVersion(m_aSelectedTaskIDs, m_sVersion) ? m_sVersion : _T("");
		m_sExternalID = m_multitasker.GetTasksExternalID(m_aSelectedTaskIDs, m_sExternalID) ? m_sExternalID : _T("");

		m_multitasker.GetTasksAllocatedTo(m_aSelectedTaskIDs, aMatched, aMixed);
		m_cbAllocTo.SetChecked(aMatched, aMixed);

		m_multitasker.GetTasksCategories(m_aSelectedTaskIDs, aMatched, aMixed);
		m_cbCategory.SetChecked(aMatched, aMixed);

		m_multitasker.GetTasksTags(m_aSelectedTaskIDs, aMatched, aMixed);
		m_cbTags.SetChecked(aMatched, aMixed);

		m_aFileLinks.RemoveAll();

		if (!m_multitasker.GetTasksFileLinks(m_aSelectedTaskIDs, m_aFileLinks))
			m_aFileLinks.RemoveAll();

		m_cbFileLink.SetFileList(m_aFileLinks);

		m_aDepends.RemoveAll();

		if (!m_multitasker.GetTasksDependencies(m_aSelectedTaskIDs, m_aDepends))
			m_aDepends.RemoveAll();

		m_eDependency.SetDependencies(m_aDepends);

		TDCTIMEPERIOD time;

		m_timeEstimate = (m_multitasker.GetTasksTimeEstimate(m_aSelectedTaskIDs, time) ? time : TDCTIMEPERIOD());
		m_timeSpent = (m_multitasker.GetTasksTimeSpent(m_aSelectedTaskIDs, time) ? time : TDCTIMEPERIOD());

		TDCCOST cost;

		m_cost = (m_multitasker.GetTasksCost(m_aSelectedTaskIDs, cost) ? cost : TDCCOST());

		TDCRECURRENCE recurrence;

		if (m_multitasker.GetTasksRecurrence(m_aSelectedTaskIDs, recurrence))
		{
			m_tRecurrence = recurrence;
		}
		else
		{
			m_tRecurrence = TDCRECURRENCE();
		}

		COLORREF crColor = CLR_DEFAULT;

		if (m_multitasker.GetTasksColor(m_aSelectedTaskIDs, crColor))
			m_crColour = crColor;
		else
			m_crColour = CLR_DEFAULT;

		if (m_multitasker.GetTasksDate(m_aSelectedTaskIDs, TDCD_START, date))
		{
			SetCtrlDate(m_dtcStart, date);
			m_cbTimeStart.SetOleTime(date.m_dt);
		}
		else
		{
			SetCtrlDate(m_dtcStart, COleDateTime());
			m_cbTimeStart.SetOleTime(-1);
		}

		if (m_multitasker.GetTasksDate(m_aSelectedTaskIDs, TDCD_DUE, date))
		{
			SetCtrlDate(m_dtcDue, date);
			m_cbTimeDue.SetOleTime(date.m_dt);
		}
		else
		{
			SetCtrlDate(m_dtcDue, COleDateTime());
			m_cbTimeDue.SetOleTime(-1);
		}

		if (m_multitasker.GetTasksDate(m_aSelectedTaskIDs, TDCD_DONE, date))
		{
			SetCtrlDate(m_dtcDone, date);
			m_cbTimeDone.SetOleTime(date.m_dt);
		}
		else
		{
			SetCtrlDate(m_dtcDone, COleDateTime());
			m_cbTimeDone.SetOleTime(-1);
		}

		EnableTimeCtrl(m_cbTimeStart, m_multitasker.GetTasksDate(m_aSelectedTaskIDs, TDCD_START, date) ? date : COleDateTime());
		EnableTimeCtrl(m_cbTimeDue, m_multitasker.GetTasksDate(m_aSelectedTaskIDs, TDCD_DUE, date) ? date : COleDateTime());
		EnableTimeCtrl(m_cbTimeDone, m_multitasker.GetTasksDate(m_aSelectedTaskIDs, TDCD_DONE, date) ? date : COleDateTime());

		m_cbPriority.SetSelectedPriority(m_nPriority);
		m_cbRisk.SetSelectedRisk(m_nRisk);
		m_eRecurrence.SetRecurrenceOptions(m_tRecurrence);

		CTDCCustomAttributeDataMap mapCustomData;

		for (int nDef = 0; nDef < m_aCustAttribDefs.GetSize(); nDef++)
		{
			const TDCCUSTOMATTRIBUTEDEFINITION& def = m_aCustAttribDefs[nDef];
			TDCCADATA data;

			if (m_multitasker.GetTasksCustomAttributeData(m_aSelectedTaskIDs, def, data))
				mapCustomData.SetAt(def.sUniqueID, data);
		}

		CTDCCustomEditAttributeUIHelper::UpdateControls(this, m_aCustomControls, m_aCustAttribDefs, mapCustomData);

		UpdateData(FALSE);
	}

	m_bRefreshing = FALSE;
}

void CTDLTaskAttributeBarCtrl::ClearValues()
{
	m_nPriority = FM_NOPRIORITYORRISK;
	m_nRisk = FM_NOPRIORITYORRISK;
	m_nPercentDone = 0;
	m_sAllocBy.Empty();
	m_sStatus.Empty();
	m_sVersion.Empty();
	m_sExternalID.Empty();
	m_timeEstimate = TDCTIMEPERIOD();
	m_timeSpent = TDCTIMEPERIOD();
	m_cost = TDCCOST();
	m_tRecurrence = TDCRECURRENCE();
	m_crColour = CLR_DEFAULT;
	m_aFileLinks.RemoveAll();
	m_aDepends.RemoveAll();

	m_cbAllocTo.CheckAll(CCBC_UNCHECKED);
	m_cbCategory.CheckAll(CCBC_UNCHECKED);
	m_cbTags.CheckAll(CCBC_UNCHECKED);
	m_cbFileLink.SetFileList(m_aFileLinks);
	m_eDependency.SetDependencies(m_aDepends);
	m_cbPriority.SetSelectedPriority(FM_NOPRIORITYORRISK);
	m_cbRisk.SetSelectedRisk(FM_NOPRIORITYORRISK);
	m_eRecurrence.SetRecurrenceOptions(m_tRecurrence);

	CTDCCustomEditAttributeUIHelper::ClearControls(this, m_aCustomControls, m_aCustAttribDefs);

	SetCtrlDate(m_dtcStart, COleDateTime());
	SetCtrlDate(m_dtcDue, COleDateTime());
	SetCtrlDate(m_dtcDone, COleDateTime());

	m_cbTimeStart.SetOleTime(-1);
	m_cbTimeDue.SetOleTime(-1);
	m_cbTimeDone.SetOleTime(-1);

	UpdateData(FALSE);
}

void CTDLTaskAttributeBarCtrl::EnableDisableControls()
{
	BOOL bEnable = (m_aSelectedTaskIDs.GetSize() > 0);

	for (int nCtrl = 0; nCtrl < NUM_LEGACYCTRLITEMS; nCtrl++)
	{
		const CTRLITEM& ctrl = LEGACYCTRLITEMS[nCtrl];
		CWnd* pCtrl = GetDlgItem(ctrl.nCtrlID);

		if (!pCtrl)
			continue;

		BOOL bEnableCtrl = bEnable;

		switch (ctrl.nAttributeID)
		{
		case TDCA_STARTTIME:
			bEnableCtrl = (bEnableCtrl && m_dtcStart.IsDateSet());
			break;

		case TDCA_DUETIME:
			bEnableCtrl = (bEnableCtrl && m_dtcDue.IsDateSet());
			break;

		case TDCA_DONETIME:
			bEnableCtrl = (bEnableCtrl && m_dtcDone.IsDateSet());
			break;

		default:
			break;
		}

		pCtrl->EnableWindow(bEnableCtrl);

		if (ctrl.nCtrlID == IDC_TDLAB_PERCENT)
			m_spinPercent.EnableWindow(bEnableCtrl);
	}

	for (int nCust = 0; nCust < m_aCustomControls.GetSize(); nCust++)
	{
		const CUSTOMATTRIBCTRLITEM& ctrl = m_aCustomControls[nCust];

		if (CWnd* pCtrl = GetDlgItem(ctrl.nCtrlID))
			pCtrl->EnableWindow(bEnable);

		if (ctrl.HasBuddy())
		{
			if (CWnd* pBuddy = GetDlgItem(ctrl.nBuddyCtrlID))
				pBuddy->EnableWindow(bEnable);
		}
	}
}

void CTDLTaskAttributeBarCtrl::ReadControlValues() const
{
	const_cast<CTDLTaskAttributeBarCtrl*>(this)->UpdateData(TRUE);
}

void CTDLTaskAttributeBarCtrl::RedrawValue(TDC_ATTRIBUTE /*nAttribID*/)
{
	if (GetSafeHwnd())
		Invalidate();
}

void CTDLTaskAttributeBarCtrl::SelectValue(TDC_ATTRIBUTE /*nAttribID*/)
{
}

void CTDLTaskAttributeBarCtrl::Flush()
{
	UpdateData(TRUE);
}

CString CTDLTaskAttributeBarCtrl::GetTaskTitle() const
{
	return _T("");
}

CString CTDLTaskAttributeBarCtrl::GetAllocBy() const
{
	ReadControlValues();
	return m_sAllocBy;
}

CString CTDLTaskAttributeBarCtrl::GetStatus() const
{
	ReadControlValues();
	return m_sStatus;
}

CString CTDLTaskAttributeBarCtrl::GetExternalID() const
{
	ReadControlValues();
	return m_sExternalID;
}

CString CTDLTaskAttributeBarCtrl::GetVersion() const
{
	ReadControlValues();
	return m_sVersion;
}

int CTDLTaskAttributeBarCtrl::GetAllocTo(CStringArray& aMatched, CStringArray& aMixed) const
{
	m_cbAllocTo.GetChecked(aMatched, CCBC_CHECKED);
	m_cbAllocTo.GetChecked(aMixed, CCBC_MIXED);

	return (aMatched.GetSize() + aMixed.GetSize());
}

int CTDLTaskAttributeBarCtrl::GetCategories(CStringArray& aMatched, CStringArray& aMixed) const
{
	m_cbCategory.GetChecked(aMatched, CCBC_CHECKED);
	m_cbCategory.GetChecked(aMixed, CCBC_MIXED);

	return (aMatched.GetSize() + aMixed.GetSize());
}

int CTDLTaskAttributeBarCtrl::GetTags(CStringArray& aMatched, CStringArray& aMixed) const
{
	m_cbTags.GetChecked(aMatched, CCBC_CHECKED);
	m_cbTags.GetChecked(aMixed, CCBC_MIXED);

	return (aMatched.GetSize() + aMixed.GetSize());
}

int CTDLTaskAttributeBarCtrl::GetFileLinks(CStringArray& aFiles) const
{
	ReadControlValues();
	aFiles.Copy(m_aFileLinks);
	return aFiles.GetSize();
}

int CTDLTaskAttributeBarCtrl::GetDependencies(CTDCDependencyArray& aDepends) const
{
	ReadControlValues();
	aDepends.Copy(m_aDepends);
	return aDepends.GetSize();
}

int CTDLTaskAttributeBarCtrl::GetPercentDone() const
{
	CString sValue;
	m_ePercentDone.GetWindowText(sValue);

	return max(0, min(100, _ttoi(sValue)));
}

int CTDLTaskAttributeBarCtrl::GetPriority() const
{
	return GetSafeHwnd() ? m_cbPriority.GetSelectedPriority() : -1;
}

int CTDLTaskAttributeBarCtrl::GetRisk() const
{
	return GetSafeHwnd() ? m_cbRisk.GetSelectedRisk() : -1;
}

BOOL CTDLTaskAttributeBarCtrl::GetTimeEstimate(TDCTIMEPERIOD& timeEst) const
{
	ReadControlValues();
	timeEst = m_timeEstimate;
	return TRUE;
}

BOOL CTDLTaskAttributeBarCtrl::GetTimeSpent(TDCTIMEPERIOD& timeSpent) const
{
	ReadControlValues();
	timeSpent = m_timeSpent;
	return TRUE;
}

BOOL CTDLTaskAttributeBarCtrl::GetCost(TDCCOST& cost) const
{
	ReadControlValues();
	cost = m_cost;
	return TRUE;
}

BOOL CTDLTaskAttributeBarCtrl::GetFlag() const
{
	return FALSE;
}

BOOL CTDLTaskAttributeBarCtrl::GetLock() const
{
	return FALSE;
}

COleDateTime CTDLTaskAttributeBarCtrl::GetStartDate() const
{
	COleDateTime date;
	m_dtcStart.GetTime(date);
	return m_dtcStart.IsDateSet() ? date : COleDateTime();
}

COleDateTime CTDLTaskAttributeBarCtrl::GetDueDate() const
{
	COleDateTime date;
	m_dtcDue.GetTime(date);
	return m_dtcDue.IsDateSet() ? date : COleDateTime();
}

COleDateTime CTDLTaskAttributeBarCtrl::GetDoneDate() const
{
	COleDateTime date;
	m_dtcDone.GetTime(date);
	return m_dtcDone.IsDateSet() ? date : COleDateTime();
}

COleDateTime CTDLTaskAttributeBarCtrl::GetStartTime() const
{
	double dTime = m_cbTimeStart.GetOleTime();
	return ((dTime < 0) ? COleDateTime() : COleDateTime(dTime));
}

COleDateTime CTDLTaskAttributeBarCtrl::GetDueTime() const
{
	double dTime = m_cbTimeDue.GetOleTime();
	return ((dTime < 0) ? COleDateTime() : COleDateTime(dTime));
}

COleDateTime CTDLTaskAttributeBarCtrl::GetDoneTime() const
{
	double dTime = m_cbTimeDone.GetOleTime();
	return ((dTime < 0) ? COleDateTime() : COleDateTime(dTime));
}

COLORREF CTDLTaskAttributeBarCtrl::GetColor() const
{
	ReadControlValues();
	return (m_crColour == CLR_DEFAULT) ? CLR_NONE : m_crColour;
}

BOOL CTDLTaskAttributeBarCtrl::GetRecurrence(TDCRECURRENCE& tr) const
{
	ReadControlValues();
	tr = m_tRecurrence;
	return TRUE;
}

BOOL CTDLTaskAttributeBarCtrl::GetCustomAttributeData(const CString& sAttribID, TDCCADATA& data, BOOL /*bFormatted*/) const
{
	CUSTOMATTRIBCTRLITEM ctrl;

	if (!CTDCCustomEditAttributeUIHelper::GetControl(sAttribID, m_aCustomControls, ctrl))
		return FALSE;

	TDCCADATA dataPrev, dataValue;
	TDCCAUI_UPDATERESULT nResult = CTDCCustomEditAttributeUIHelper::GetControlData(this, ctrl, m_aCustAttribDefs, dataPrev, dataValue);

	if (nResult == TDCCAUIRES_FAIL)
		return FALSE;

	data = dataValue;
	return TRUE;
}

int CTDLTaskAttributeBarCtrl::GetCustomAttributeAutoListData(const CString& /*sAttribID*/, CStringArray& /*aItems*/) const
{
	return 0;
}




















