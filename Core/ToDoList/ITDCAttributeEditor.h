#if !defined(AFX_ITDLATTRIBUTEEDITOR_H__C4E1B6F2_6D3A_4A0B_9C7E_51D2A8B3F604__INCLUDED_)
#define AFX_ITDLATTRIBUTEEDITOR_H__C4E1B6F2_6D3A_4A0B_9C7E_51D2A8B3F604__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif

#include "TDCStruct.h"

/////////////////////////////////////////////////////////////////////////////

class ITDCAttributeEditor
{
public:
	virtual ~ITDCAttributeEditor() {}

	virtual void SetDefaultAutoListData(const TDCAUTOLISTDATA& tldDefault) = 0;
	virtual void SetAutoListData(TDC_ATTRIBUTE nAttribID, const TDCAUTOLISTDATA& tld) = 0;
	virtual void GetAutoListData(TDC_ATTRIBUTE nAttribID, TDCAUTOLISTDATA& tld) const = 0;
	virtual void SetAutoListDataReadOnly(TDC_ATTRIBUTE nAttribID, BOOL bReadOnly) = 0;
	virtual BOOL IsAutoListDataReadOnly(TDC_ATTRIBUTE nAttribID) const = 0;

	virtual BOOL SetSelectedTaskIDs(const CDWordArray& aTaskIDs) = 0;
	virtual BOOL IsAnyTaskSelected(const CDWordArray& aTaskIDs) const = 0;
	virtual TDC_ATTRIBUTE GetSelectedAttributeID() const = 0;
	virtual CString GetSelectedAttributeLabel() const = 0;

	virtual void RefreshSelectedTasksValues(const CTDCAttributeMap& mapAttribIDs) = 0;
	virtual void RefreshSelectedTasksValue(TDC_ATTRIBUTE nAttribID) = 0;
	virtual void RefreshDateTimeFormatting() = 0;

	virtual void SetCurrentFolder(const CString& sFolder) = 0;
	virtual void SetCompletionStatus(const CString& sStatus) = 0;
	virtual void SetPriorityColors(const CDWordArray& aColors) = 0;
	virtual void SetPercentDoneIncrement(int nAmount) = 0;
	virtual void SetTimeTrackTaskID(DWORD dwTaskID) = 0;
	virtual void SetNumPriorityRiskLevels(int nNumLevels) = 0;

	virtual void RedrawValue(TDC_ATTRIBUTE nAttribID) = 0;
	virtual void SelectValue(TDC_ATTRIBUTE nAttribID) = 0;

	virtual CString GetTaskTitle() const = 0;
	virtual CString GetAllocBy() const = 0;
	virtual CString GetStatus() const = 0;
	virtual CString GetExternalID() const = 0;
	virtual CString GetVersion() const = 0;

	virtual int GetAllocTo(CStringArray& aMatched, CStringArray& aMixed) const = 0;
	virtual int GetCategories(CStringArray& aMatched, CStringArray& aMixed) const = 0;
	virtual int GetTags(CStringArray& aMatched, CStringArray& aMixed) const = 0;

	virtual int GetFileLinks(CStringArray& aFiles) const = 0;
	virtual int GetDependencies(CTDCDependencyArray& aDepends) const = 0;

	virtual int GetPercentDone() const = 0;
	virtual int GetPriority() const = 0;
	virtual int GetRisk() const = 0;

	virtual BOOL GetTimeEstimate(TDCTIMEPERIOD& timeEst) const = 0;
	virtual BOOL GetTimeSpent(TDCTIMEPERIOD& timeSpent) const = 0;
	virtual BOOL GetCost(TDCCOST& cost) const = 0;
	virtual BOOL GetFlag() const = 0;
	virtual BOOL GetLock() const = 0;

	virtual COleDateTime GetStartDate() const = 0;
	virtual COleDateTime GetDueDate() const = 0;
	virtual COleDateTime GetDoneDate() const = 0;
	virtual COleDateTime GetStartTime() const = 0;
	virtual COleDateTime GetDueTime() const = 0;
	virtual COleDateTime GetDoneTime() const = 0;

	virtual BOOL GetCustomAttributeData(const CString& sAttribID, TDCCADATA& data, BOOL bFormatted = FALSE) const = 0;
	virtual int GetCustomAttributeAutoListData(const CString& sAttribID, CStringArray& aItems) const = 0;

	virtual void OnCustomAttributesChange() = 0;
	virtual void OnAttributeVisibilityChange() = 0;

	virtual void Flush() = 0;
};

/////////////////////////////////////////////////////////////////////////////

#endif // AFX_ITDLATTRIBUTEEDITOR_H__C4E1B6F2_6D3A_4A0B_9C7E_51D2A8B3F604__INCLUDED_
