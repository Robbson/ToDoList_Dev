// TDLTaskAttributeBarCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "TDLTaskAttributeBarCtrl.h"

#include "..\interfaces\UIThemeFile.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC(CTDLTaskAttributeBarCtrl, CWnd)

CTDLTaskAttributeBarCtrl::CTDLTaskAttributeBarCtrl()
	:
	m_crBack(CLR_NONE)
{
}

CTDLTaskAttributeBarCtrl::~CTDLTaskAttributeBarCtrl()
{
}

BEGIN_MESSAGE_MAP(CTDLTaskAttributeBarCtrl, CWnd)
	ON_WM_PAINT()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////

BOOL CTDLTaskAttributeBarCtrl::Create(CWnd* pParent, UINT nID, const CRect& rect)
{
	LPCTSTR szClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW,
										  ::LoadCursor(NULL, IDC_ARROW),
										  (HBRUSH)::GetStockObject(NULL_BRUSH));

	return CWnd::CreateEx(0, szClass, _T(""), (WS_CHILD | WS_VISIBLE), rect, pParent, nID);
}

void CTDLTaskAttributeBarCtrl::SetUITheme(const CUIThemeFile& theme)
{
	SetBackgroundColor(theme.crAppBackLight);
}

void CTDLTaskAttributeBarCtrl::SetBackgroundColor(COLORREF crBack)
{
	m_crBack = crBack;

	if (GetSafeHwnd())
		Invalidate();
}

void CTDLTaskAttributeBarCtrl::OnPaint()
{
	CPaintDC dc(this);

	CRect rClient;
	GetClientRect(rClient);

	COLORREF crBack = ((m_crBack == CLR_NONE) ? ::GetSysColor(COLOR_BTNFACE) : m_crBack);
	dc.FillSolidRect(rClient, crBack);

	dc.SetBkMode(TRANSPARENT);
	dc.SetTextColor(::GetSysColor(COLOR_GRAYTEXT));

	dc.DrawText(_T("Legacy attribute editor will appear here (Phase 1 placeholder)"),
				rClient,
				(DT_CENTER | DT_VCENTER | DT_SINGLELINE));
}
