// TDLTasklistSwitcher.cpp : implementation file
//

#include "stdafx.h"
#include "TDLTasklistSwitcher.h"

#include "..\shared\graphicsmisc.h"
#include "..\shared\misc.h"

#include <afxpriv.h>

/////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////

const int SWITCHER_MAX_VISIBLE_ROWS = 15;
const UINT SWITCHER_TIMER_ID = 1;
const UINT SWITCHER_TIMER_ELAPSE = 50;
const UINT SWITCHER_LIST_ID = 1;

/////////////////////////////////////////////////////////////////////////////
// CTDLTasklistSwitcher

CTDLTasklistSwitcher::CTDLTasklistSwitcher()
	:
	m_pParent(NULL),
	m_hwndPrevFocus(NULL),
	m_bCancelled(FALSE),
	m_bDone(FALSE),
	m_bBusy(FALSE)
{
}

CTDLTasklistSwitcher::~CTDLTasklistSwitcher()
{
}

BEGIN_MESSAGE_MAP(CTDLTasklistSwitcher, CWnd)
	ON_LBN_KILLFOCUS(SWITCHER_LIST_ID, OnListKillFocus)
	ON_WM_DRAWITEM()
	ON_WM_MEASUREITEM()
	ON_MESSAGE(WM_FLOATSTATUS, OnFloatStatus)
END_MESSAGE_MAP()

int CTDLTasklistSwitcher::Show(CWnd* pParent, const CStringArray& aItems, const CArray<int, int>& aStates, int nInitialSel)
{
	if ((aItems.GetSize() < 2) || !pParent)
		return -1;

	m_pParent = pParent;
	m_hwndPrevFocus = ::GetFocus();
	m_bCancelled = FALSE;
	m_bDone = FALSE;
	m_bBusy = FALSE;

	m_aItems.Copy(aItems);
	m_aStates.Copy(aStates);

	LPCTSTR szClass = AfxRegisterWndClass(CS_DROPSHADOW,
										  ::LoadCursor(NULL, IDC_ARROW),
										  (HBRUSH)(COLOR_WINDOW + 1));

	if (!CreateEx(WS_EX_TOOLWINDOW, szClass, _T(""), (WS_POPUP | WS_BORDER),
				  CRect(0, 0, 0, 0), pParent, 0))
	{
		return -1;
	}

	DWORD dwListStyle = (WS_CHILD | WS_VISIBLE | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT |
						 LBS_OWNERDRAWFIXED | LBS_HASSTRINGS);

	if (aItems.GetSize() > SWITCHER_MAX_VISIBLE_ROWS)
		dwListStyle |= WS_VSCROLL;

	if (!m_lb.Create(dwListStyle, CRect(0, 0, 0, 0), this, SWITCHER_LIST_ID))
	{
		DestroyWindow();
		return -1;
	}

	m_lb.SetFont(CFont::FromHandle(GraphicsMisc::GetFont(pParent->GetSafeHwnd())));

	CClientDC dc(&m_lb);
	CFont* pOldFont = dc.SelectObject(m_lb.GetFont());

	TEXTMETRIC tm = { 0 };
	dc.GetTextMetrics(&tm);

	dc.SelectObject(pOldFont);

	m_lb.SetItemHeight(0, tm.tmHeight + GraphicsMisc::ScaleByDPIFactor(6));

	for (int nItem = 0; nItem < m_aItems.GetSize(); nItem++)
		m_lb.AddString(m_aItems[nItem]);

	if ((nInitialSel < 0) || (nInitialSel >= m_lb.GetCount()))
		nInitialSel = 0;

	m_lb.SetCurSel(nInitialSel);
	MoveToScreenCentre(pParent);

	ShowWindow(SW_SHOW);
	SetForegroundWindow();
	m_lb.SetFocus();

	SetTimer(SWITCHER_TIMER_ID, SWITCHER_TIMER_ELAPSE, NULL);

	int nCount = m_lb.GetCount();
	int nSel = nInitialSel;
	MSG msg;

	// A very quick Ctrl release may happen before the popup has focus
	if (!Misc::IsKeyPressed(VK_CONTROL, TRUE))
		m_bDone = TRUE;

	while (!m_bDone)
	{
		BOOL bGotMessage = GetMessage(&msg, NULL, 0, 0);

		if (bGotMessage <= 0)
		{
			if (bGotMessage == 0)
				PostQuitMessage((int)msg.wParam);

			m_bCancelled = TRUE;
			break;
		}

		if (m_bDone)
			break;

		if (msg.message == WM_KEYDOWN)
		{
			switch (msg.wParam)
			{
			case VK_TAB:
				nSel = (nSel + (Misc::IsKeyPressed(VK_SHIFT) ? -1 : 1) + nCount) % nCount;
				m_lb.SetCurSel(nSel);
				continue;

			case VK_UP:
				nSel = (nSel - 1 + nCount) % nCount;
				m_lb.SetCurSel(nSel);
				continue;

			case VK_DOWN:
				nSel = (nSel + 1) % nCount;
				m_lb.SetCurSel(nSel);
				continue;

			case VK_HOME:
				nSel = 0;
				m_lb.SetCurSel(nSel);
				continue;

			case VK_END:
				nSel = (nCount - 1);
				m_lb.SetCurSel(nSel);
				continue;

			case VK_RETURN:
				m_bDone = TRUE;
				continue;

			case VK_ESCAPE:
				m_bCancelled = TRUE;
				m_bDone = TRUE;
				continue;
			}
		}
		else if ((msg.message == WM_KEYUP) && (msg.wParam == VK_CONTROL))
		{
			m_bDone = TRUE;
			continue;
		}
		else if ((msg.message == WM_TIMER) && (msg.hwnd == GetSafeHwnd()))
		{
			// Guard against a missed WM_KEYUP if focus changed unexpectedly
			if (!Misc::IsKeyPressed(VK_CONTROL, TRUE))
				m_bDone = TRUE;

			continue;
		}
		else if (msg.message == WM_LBUTTONUP)
		{
			DispatchMessage(&msg);
			m_bDone = TRUE;
			continue;
		}

		TranslateMessage(&msg);
		DispatchMessage(&msg);

		nSel = m_lb.GetCurSel();

		if (nSel == LB_ERR)
			nSel = 0;
	}

	KillTimer(SWITCHER_TIMER_ID);

	return (m_bCancelled ? -1 : m_lb.GetCurSel());
}

void CTDLTasklistSwitcher::SetBusy()
{
	m_bBusy = TRUE;
	m_lb.EnableWindow(FALSE);
	m_lb.Invalidate();
	m_lb.UpdateWindow();
}

void CTDLTasklistSwitcher::Close(BOOL bRestoreFocus)
{
	if (GetSafeHwnd())
	{
		KillTimer(SWITCHER_TIMER_ID);
		ShowWindow(SW_HIDE);
		DestroyWindow();
	}

	if (bRestoreFocus && ::IsWindow(m_hwndPrevFocus))
		::SetFocus(m_hwndPrevFocus);
}

void CTDLTasklistSwitcher::OnListKillFocus()
{
	if (!m_bDone)
	{
		m_bCancelled = TRUE;
		m_bDone = TRUE;
	}
}

void CTDLTasklistSwitcher::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDIS)
{
	if ((nIDCtl != SWITCHER_LIST_ID) || !lpDIS || (lpDIS->itemID == (UINT)-1))
		return;

	int nItem = lpDIS->itemID;
	CString sText = ((nItem < m_aItems.GetSize()) ? m_aItems[nItem] : _T(""));
	int nState = ((nItem < m_aStates.GetSize()) ? m_aStates[nItem] : ITEM_NORMAL);

	BOOL bSelected = ((lpDIS->itemState & ODS_SELECTED) != 0);
	BOOL bDisabled = (m_bBusy || ((lpDIS->itemState & ODS_DISABLED) != 0));

	COLORREF crBack = GetSysColor(COLOR_WINDOW);

	if (bSelected)
		crBack = GetSysColor(m_bBusy ? COLOR_BTNFACE : COLOR_HIGHLIGHT);

	COLORREF crText;

	if (bDisabled)
		crText = GetSysColor(COLOR_GRAYTEXT);
	else if (bSelected)
		crText = GetSysColor(COLOR_HIGHLIGHTTEXT);
	else if (nState == ITEM_MODIFIED)
		crText = GetSysColor(COLOR_HOTLIGHT);
	else if (nState == ITEM_NOTLOADED)
		crText = GetSysColor(COLOR_GRAYTEXT);
	else
		crText = GetSysColor(COLOR_WINDOWTEXT);

	CDC* pDC = CDC::FromHandle(lpDIS->hDC);
	CRect rItem(lpDIS->rcItem);

	pDC->FillSolidRect(rItem, crBack);
	pDC->SetBkColor(crBack);
	pDC->SetTextColor(crText);

	CFont* pOldFont = pDC->SelectObject(m_lb.GetFont());
	rItem.left += GraphicsMisc::ScaleByDPIFactor(10);

	pDC->DrawText(sText, rItem, (DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_NOPREFIX));

	pDC->SelectObject(pOldFont);
}

void CTDLTasklistSwitcher::OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMIS)
{
	if ((nIDCtl == SWITCHER_LIST_ID) && lpMIS && (lpMIS->CtlType == ODT_LISTBOX))
	{
		HFONT hFont = GraphicsMisc::GetFont(m_pParent ? m_pParent->GetSafeHwnd() : NULL);
		lpMIS->itemHeight = (GraphicsMisc::GetFontPixelSize(hFont) + GraphicsMisc::ScaleByDPIFactor(6));
		return;
	}

	CWnd::OnMeasureItem(nIDCtl, lpMIS);
}

LRESULT CTDLTasklistSwitcher::OnFloatStatus(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	// Prevent the application window losing activation when we gain focus
	return 1L;
}

void CTDLTasklistSwitcher::MoveToScreenCentre(CWnd* pParent)
{
	int nCount = m_lb.GetCount();
	int nVisible = min(nCount, SWITCHER_MAX_VISIBLE_ROWS);
	int nItemHeight = m_lb.GetItemHeight(0);
	int nHeight = (nVisible * nItemHeight);

	CClientDC dc(&m_lb);
	CFont* pOldFont = dc.SelectObject(m_lb.GetFont());

	int nWidth = 0;
	CString sItem;

	for (int nItem = 0; nItem < nCount; nItem++)
	{
		m_lb.GetText(nItem, sItem);
		nWidth = max(nWidth, dc.GetTextExtent(sItem).cx);
	}

	dc.SelectObject(pOldFont);

	nWidth += (2 * GraphicsMisc::ScaleByDPIFactor(10));

	if (nCount > nVisible)
		nWidth += GetSystemMetrics(SM_CXVSCROLL);

	CRect rWnd(0, 0, nWidth, nHeight);
	AdjustWindowRectEx(&rWnd, GetStyle(), FALSE, GetExStyle());

	CRect rWork(0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
	HMONITOR hMon = MonitorFromWindow(pParent->GetSafeHwnd(), MONITOR_DEFAULTTONEAREST);

	if (hMon)
		GraphicsMisc::GetMonitorAvailableScreenSpace(hMon, rWork);

	int nWndWidth = min(rWnd.Width(), rWork.Width());
	int nWndHeight = min(rWnd.Height(), rWork.Height());
	int nX = (rWork.left + ((rWork.Width() - nWndWidth) / 2));
	int nY = (rWork.top + ((rWork.Height() - nWndHeight) / 2));

	MoveWindow(nX, nY, nWndWidth, nWndHeight, FALSE);

	CRect rClient;
	GetClientRect(&rClient);
	m_lb.MoveWindow(&rClient);
}
