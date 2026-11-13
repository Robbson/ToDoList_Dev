// TDLTasklistSwitcher.h : header file
//

#if !defined(AFX_TDLTASKLISTSWITCHER_H__9E1B5A3C_0A0D_4E5F_9B6A_2E0B3D8A4C71__INCLUDED_)
#define AFX_TDLTASKLISTSWITCHER_H__9E1B5A3C_0A0D_4E5F_9B6A_2E0B3D8A4C71__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CTDLTasklistSwitcher window

class CTDLTasklistSwitcher : public CWnd
{
public:
	enum ITEMSTATE
	{
		ITEM_NORMAL = 0,
		ITEM_MODIFIED,
		ITEM_NOTLOADED,
	};

	CTDLTasklistSwitcher();
	virtual ~CTDLTasklistSwitcher();

	// Returns the selected item or -1 if cancelled. The popup remains
	// visible until Close() is called.
	int Show(CWnd* pParent, const CStringArray& aItems, const CArray<int, int>& aStates, int nInitialSel);
	void SetBusy();
	void Close(BOOL bRestoreFocus = TRUE);

protected:
	CWnd* m_pParent;
	HWND m_hwndPrevFocus;
	BOOL m_bCancelled;
	BOOL m_bDone;
	BOOL m_bBusy;
	CListBox m_lb;
	CStringArray m_aItems;
	CArray<int, int> m_aStates;

protected:
	void MoveToScreenCentre(CWnd* pParent);
	void DrawItem(LPDRAWITEMSTRUCT lpDIS);

	afx_msg void OnListKillFocus();
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDIS);
	afx_msg void OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMIS);
	afx_msg LRESULT OnFloatStatus(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

#endif // !defined(AFX_TDLTASKLISTSWITCHER_H__9E1B5A3C_0A0D_4E5F_9B6A_2E0B3D8A4C71__INCLUDED_)
