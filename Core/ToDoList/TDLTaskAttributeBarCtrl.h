#if !defined(AFX_TDLTASKATTRIBUTEBARCTRL_H__9A6B5E51_4A2D_4E7E_9F3B_5C2D8A1B7C10__INCLUDED_)
#define AFX_TDLTASKATTRIBUTEBARCTRL_H__9A6B5E51_4A2D_4E7E_9F3B_5C2D8A1B7C10__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif

#include "..\shared\RuntimeDlg.h"
#include "..\shared\AutoComboBox.h"
#include "..\shared\CheckComboBox.h"
#include "..\shared\ColourPickerEx.h"
#include "..\shared\DateTimeCtrlEx.h"
#include "..\shared\EnEdit.h"
#include "..\shared\FileComboBox.h"
#include "..\shared\MaskEdit.h"
#include "..\shared\TimeComboBox.h"
#include "..\shared\TimeEdit.h"

#include "ITDCAttributeEditor.h"
#include "TDLPriorityComboBox.h"
#include "TDLRiskComboBox.h"
#include "TDLRecurringTaskEdit.h"
#include "TDLTaskDependencyEdit.h"
#include "ToDoCtrlDataUtils.h"

/////////////////////////////////////////////////////////////////////////////

class CContentMgr;
class CUIThemeFile;

/////////////////////////////////////////////////////////////////////////////

enum
{
	IDC_TDLAB_PRIORITY = 1700,
	IDC_TDLAB_PRIORITYLABEL,
	IDC_TDLAB_RISK,
	IDC_TDLAB_RISKLABEL,
	IDC_TDLAB_PERCENT,
	IDC_TDLAB_PERCENTLABEL,
	IDC_TDLAB_PERCENTSPIN,
	IDC_TDLAB_TIMEEST,
	IDC_TDLAB_TIMEESTLABEL,
	IDC_TDLAB_TIMESPENT,
	IDC_TDLAB_TIMESPENTLABEL,
	IDC_TDLAB_STARTDATE,
	IDC_TDLAB_STARTLABEL,
	IDC_TDLAB_STARTTIME,
	IDC_TDLAB_STARTTIMELABEL,
	IDC_TDLAB_DUEDATE,
	IDC_TDLAB_DUELABEL,
	IDC_TDLAB_DUETIME,
	IDC_TDLAB_DUETIMELABEL,
	IDC_TDLAB_DONEDATE,
	IDC_TDLAB_DONELABEL,
	IDC_TDLAB_DONETIME,
	IDC_TDLAB_DONETIMELABEL,
	IDC_TDLAB_RECURRENCE,
	IDC_TDLAB_RECURRENCELABEL,
	IDC_TDLAB_COLOUR,
	IDC_TDLAB_COLOURLABEL,
	IDC_TDLAB_ALLOCTO,
	IDC_TDLAB_ALLOCTOLABEL,
	IDC_TDLAB_ALLOCBY,
	IDC_TDLAB_ALLOCBYLABEL,
	IDC_TDLAB_STATUS,
	IDC_TDLAB_STATUSLABEL,
	IDC_TDLAB_CATEGORY,
	IDC_TDLAB_CATEGORYLABEL,
	IDC_TDLAB_TAGS,
	IDC_TDLAB_TAGSLABEL,
	IDC_TDLAB_EXTERNALID,
	IDC_TDLAB_EXTERNALIDLABEL,
	IDC_TDLAB_COST,
	IDC_TDLAB_COSTLABEL,
	IDC_TDLAB_DEPENDS,
	IDC_TDLAB_DEPENDSLABEL,
	IDC_TDLAB_VERSION,
	IDC_TDLAB_VERSIONLABEL,
	IDC_TDLAB_FILEPATH,
	IDC_TDLAB_FILEPATHLABEL,
	IDC_TDLAB_TIME_TRACK = 1760,
	IDC_TDLAB_ADD_TIME,
	IDC_TDLAB_DEPENDS_EDIT,
	IDC_TDLAB_DEPENDS_LINK,
};

/////////////////////////////////////////////////////////////////////////////

class CTDLTaskAttributeBarCtrl : public CRuntimeDlg, public ITDCAttributeEditor
{
	DECLARE_DYNAMIC(CTDLTaskAttributeBarCtrl)

public:
	CTDLTaskAttributeBarCtrl(const CToDoCtrlData& data,
							 const CContentMgr& mgrContent,
							 const TDCCOLEDITVISIBILITY& vis,
							 const CTDCCustomAttribDefinitionArray& aCustAttribDefs);
	virtual ~CTDLTaskAttributeBarCtrl();

	BOOL Create(CWnd* pParent, UINT nID, const CRect& rect);
	void SetUITheme(const CUIThemeFile& theme);
	void SetBackgroundColor(COLORREF crBack);
	BOOL HasFocus() const { return (::GetFocus() == GetSafeHwnd()); }

	COLORREF GetColor() const;
	BOOL GetRecurrence(TDCRECURRENCE& tr) const;

	// ITDCAttributeEditor
	void SetDefaultAutoListData(const TDCAUTOLISTDATA& tldDefault) override;
	void SetAutoListData(TDC_ATTRIBUTE nAttribID, const TDCAUTOLISTDATA& tld) override;
	void GetAutoListData(TDC_ATTRIBUTE nAttribID, TDCAUTOLISTDATA& tld) const override;
	void SetAutoListDataReadOnly(TDC_ATTRIBUTE nAttribID, BOOL bReadOnly) override;
	BOOL IsAutoListDataReadOnly(TDC_ATTRIBUTE nAttribID) const override;

	BOOL SetSelectedTaskIDs(const CDWordArray& aTaskIDs) override;
	BOOL IsAnyTaskSelected(const CDWordArray& aTaskIDs) const override;
	TDC_ATTRIBUTE GetSelectedAttributeID() const override;
	CString GetSelectedAttributeLabel() const override;

	void RefreshSelectedTasksValues(const CTDCAttributeMap& mapAttribIDs = TDCA_ALL) override;
	void RefreshSelectedTasksValue(TDC_ATTRIBUTE nAttribID) override;
	void RefreshDateTimeFormatting() override;

	void SetCurrentFolder(const CString& sFolder) override;
	void SetCompletionStatus(const CString& sStatus) override;
	void SetPriorityColors(const CDWordArray& aColors) override;
	void SetPercentDoneIncrement(int nAmount) override;
	void SetTimeTrackTaskID(DWORD dwTaskID) override;
	void SetNumPriorityRiskLevels(int nNumLevels) override;

	void RedrawValue(TDC_ATTRIBUTE nAttribID) override;
	void SelectValue(TDC_ATTRIBUTE nAttribID) override;

	CString GetTaskTitle() const override;
	CString GetAllocBy() const override;
	CString GetStatus() const override;
	CString GetExternalID() const override;
	CString GetVersion() const override;

	int GetAllocTo(CStringArray& aMatched, CStringArray& aMixed) const override;
	int GetCategories(CStringArray& aMatched, CStringArray& aMixed) const override;
	int GetTags(CStringArray& aMatched, CStringArray& aMixed) const override;

	int GetFileLinks(CStringArray& aFiles) const override;
	int GetDependencies(CTDCDependencyArray& aDepends) const override;

	int GetPercentDone() const override;
	int GetPriority() const override;
	int GetRisk() const override;

	BOOL GetTimeEstimate(TDCTIMEPERIOD& timeEst) const override;
	BOOL GetTimeSpent(TDCTIMEPERIOD& timeSpent) const override;
	BOOL GetCost(TDCCOST& cost) const override;
	BOOL GetFlag() const override;
	BOOL GetLock() const override;

	COleDateTime GetStartDate() const override;
	COleDateTime GetDueDate() const override;
	COleDateTime GetDoneDate() const override;
	COleDateTime GetStartTime() const override;
	COleDateTime GetDueTime() const override;
	COleDateTime GetDoneTime() const override;

	BOOL GetCustomAttributeData(const CString& sAttribID, TDCCADATA& data, BOOL bFormatted = FALSE) const override;
	int GetCustomAttributeAutoListData(const CString& sAttribID, CStringArray& aItems) const override;

	void OnCustomAttributesChange() override;
	void OnAttributeVisibilityChange() override;

	void Flush() override;

protected:
	const CToDoCtrlData& m_data;
	const CContentMgr& m_mgrContent;
	const TDCCOLEDITVISIBILITY& m_vis;
	const CTDCCustomAttribDefinitionArray& m_aCustAttribDefs;
	CTDCMultiTasker m_multitasker;

	CDWordArray m_aSelectedTaskIDs;
	TDCAUTOLISTDATA m_tldAll, m_tldDefault;
	CMap<TDC_ATTRIBUTE, TDC_ATTRIBUTE, BOOL, BOOL&> m_mapReadOnlyAutoList;

	CString m_sCurrentFolder, m_sCompletionStatus;
	CDWordArray m_aPriorityColors;
	int m_nPercentDoneIncrement, m_nNumPriorityRiskLevels;
	DWORD m_dwTimeTrackTaskID;

	BOOL m_bRefreshing;
	COLORREF m_crBack;

	CString m_sAllocBy, m_sStatus, m_sExternalID, m_sVersion;
	int m_nPriority, m_nRisk, m_nPercentDone;
	TDCTIMEPERIOD m_timeEstimate, m_timeSpent;
	TDCCOST m_cost;
	COLORREF m_crColour;
	TDCRECURRENCE m_tRecurrence;
	CStringArray m_aFileLinks;
	CTDCDependencyArray m_aDepends;

	CEnEdit m_eExternalID;
	CMaskEdit m_eCost;
	CMaskEdit m_ePercentDone;
	CSpinButtonCtrl m_spinPercent;
	CTimeEdit m_eTimeEstimate, m_eTimeSpent;
	CAutoComboBox m_cbAllocBy, m_cbStatus, m_cbVersion;
	CCheckComboBox m_cbAllocTo, m_cbCategory, m_cbTags;
	CColourPickerEx m_cpColour;
	CDateTimeCtrlEx m_dtcStart, m_dtcDue, m_dtcDone;
	CTimeComboBox m_cbTimeStart, m_cbTimeDue, m_cbTimeDone;
	CFileComboBox m_cbFileLink;
	CTDLPriorityComboBox m_cbPriority;
	CTDLRiskComboBox m_cbRisk;
	CTDLRecurringTaskEdit m_eRecurrence;
	CTDLTaskDependencyEdit m_eDependency;

protected:
	void BuildControls();
	void ShowHideControls();
	void ReposControls();
	void RefreshAllValues();
	void ClearValues();
	void EnableDisableControls();
	void NotifyParent(TDC_ATTRIBUTE nAttribID);
	void ReadControlValues() const;
	void PopulateAutoLists();
	void PopulateAutoList(TDC_ATTRIBUTE nAttribID);
	void SetCtrlDate(CDateTimeCtrl& ctrl, const COleDateTime& date);
	void EnableTimeCtrl(CTimeComboBox& ctrl, const COleDateTime& date);
	static TDC_ATTRIBUTE GetAttributeID(UINT nCtrlID);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);

	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg LRESULT OnColourChanged(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

#endif // AFX_TDLTASKATTRIBUTEBARCTRL_H__9A6B5E51_4A2D_4E7E_9F3B_5C2D8A1B7C10__INCLUDED_

