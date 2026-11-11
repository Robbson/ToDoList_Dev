#if !defined(AFX_TDLTASKATTRIBUTEBARCTRL_H__9A6B5E51_4A2D_4E7E_9F3B_5C2D8A1B7C10__INCLUDED_)
#define AFX_TDLTASKATTRIBUTEBARCTRL_H__9A6B5E51_4A2D_4E7E_9F3B_5C2D8A1B7C10__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif

/////////////////////////////////////////////////////////////////////////////

class CUIThemeFile;

/////////////////////////////////////////////////////////////////////////////

class CTDLTaskAttributeBarCtrl : public CWnd
{
	DECLARE_DYNAMIC(CTDLTaskAttributeBarCtrl)

public:
	CTDLTaskAttributeBarCtrl();
	virtual ~CTDLTaskAttributeBarCtrl();

	BOOL Create(CWnd* pParent, UINT nID, const CRect& rect);
	void SetUITheme(const CUIThemeFile& theme);
	void SetBackgroundColor(COLORREF crBack);
	BOOL HasFocus() const { return (::GetFocus() == GetSafeHwnd()); }

protected:
	COLORREF m_crBack;

protected:
	afx_msg void OnPaint();
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

#endif // AFX_TDLTASKATTRIBUTEBARCTRL_H__9A6B5E51_4A2D_4E7E_9F3B_5C2D8A1B7C10__INCLUDED_
