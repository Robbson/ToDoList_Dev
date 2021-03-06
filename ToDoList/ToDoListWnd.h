// ToDoListWnd.h : header file
//

#if !defined(AFX_TODOLISTWND_H__13051D32_D372_4205_BA71_05FAC2159F1C__INCLUDED_)
#define AFX_TODOLISTWND_H__13051D32_D372_4205_BA71_05FAC2159F1C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "filteredtodoctrl.h"
#include "preferencesdlg.h"
#include "tdlfindtasksDlg.h"
#include "todoctrlmgr.h"
#include "TDCImportExportMgr.h"
#include "TDLContentMgr.h"
#include "TDLfilterbar.h"
#include "TDLSendTasksDlg.h"
#include "taskselectiondlg.h"
#include "todoctrlreminders.h"
#include "tdlTasklistStorageMgr.h"
#include "tdcstartupoptions.h"
#include "TDLTimeTrackerDlg.h"
#include "tdlquickfindcombobox.h"
#include "TDLThreadedExporterWnd.h"
#include "TDLCustomToolbar.h"

#include "..\shared\trayicon.h"
#include "..\shared\toolbarhelper.h"
#include "..\shared\filemisc.h"
#include "..\shared\ShortcutManager.h"
#include "..\shared\driveinfo.h"
#include "..\shared\entoolbar.h"
#include "..\shared\tabctrlex.h"
#include "..\shared\enrecentfilelist.h"
#include "..\shared\enimagelist.h"
#include "..\shared\enmenu.h"
#include "..\shared\dialoghelper.h"
#include "..\shared\tabbedcombobox.h"
#include "..\shared\deferWndMove.h"
#include "..\shared\enBrowserctrl.h"
#include "..\shared\UIExtensionMgr.h"
#include "..\shared\menuiconmgr.h"
#include "..\shared\autocombobox.h"
#include "..\shared\uithemefile.h"
#include "..\shared\toolbarhelper.h"
#include "..\shared\StatusbarProgress.h"
#include "..\shared\stickieswnd.h"
#include "..\shared\windowicons.h"
#include "..\shared\sessionstatuswnd.h"
#include "..\shared\statusbarACTEx.h"

/////////////////////////////////////////////////////////////////////////////
// CToDoListWnd 

const UINT WM_TDL_SHOWWINDOW		= ::RegisterWindowMessage(_T("WM_TDL_SHOWWINDOW"));
const UINT WM_TDL_ISCLOSING			= ::RegisterWindowMessage(_T("WM_TDL_ISCLOSING"));
const UINT WM_TDL_REFRESHPREFS		= ::RegisterWindowMessage(_T("WM_TDL_REFRESHPREFS"));
const UINT WM_TDL_RESTORE			= ::RegisterWindowMessage(_T("WM_TDL_RESTORE"));

/////////////////////////////////////////////////////////////////////////////

class CTDLPrintDialog;

/////////////////////////////////////////////////////////////////////////////

class CToDoListWnd : public CFrameWnd, public CDialogHelper
{
public:
	// Construction
	CToDoListWnd(); 
	~CToDoListWnd();
	
	BOOL Create(const CTDCStartupOptions& startup);

	static BOOL EnableLogging(BOOL bEnable = TRUE);
	static CString GetVersion(BOOL bExtended = TRUE);
	static CString GetTitle(BOOL bExtended = TRUE);
	
protected:
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CToDoListWnd)
	protected:
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL
	virtual void OnOK() {}
	virtual void OnCancel();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual int OnCreate(LPCREATESTRUCT lpCreateStruct);

	// Implementation
protected:
	CTDLQuickFindComboBox m_cbQuickFind;
	CEnBrowserCtrl m_IE;
	CEnMenu m_menubar;
	CEnRecentFileList m_mruList;
	CEnToolBar m_toolbarMain;
	CTDLCustomToolbar m_toolbarCustom;
	CFont m_fontMain;
	CFont m_fontTree, m_fontComments; // shared by all tasklists
	CEnImageList m_ilTabCtrl;
	CPreferencesDlg* m_pPrefs;
	CStatusBarACTEx	m_statusBar;
	CStatusBarProgress m_sbProgress;
	CTDLFilterBar m_filterBar;
	CTDLFindTasksDlg m_findDlg;
	CTabCtrlEx m_tabCtrl;
	CTaskListDropTarget m_dropTarget;
	CToDoCtrlReminders m_reminders;
	CToolbarHelper m_tbHelperMain;
	CTrayIcon m_trayIcon;
	CUIThemeFile m_theme;
	CWindowIcons m_icons;
	HWND m_hwndLastFocus;
	CTDCStartupOptions m_startupOptions;
	TDC_COLUMN m_nContextColumnID;
	TDC_MAXSTATE m_nMaxState, m_nPrevMaxState;
	TODOITEM m_tdiDefault;
	CTDLTimeTrackerDlg m_dlgTimeTracker;
	CSessionStatusWnd m_wndSessionStatus;
	CTDLThreadedExporterWnd m_wndExport;

	int m_nLastSelItem; // just for flicker-free todoctrl switching

	CShortcutManager m_mgrShortcuts;
	CTDCImportExportMgr m_mgrImportExport;
	CTDLTasklistStorageMgr m_mgrStorage;
	CToDoCtrlMgr m_mgrToDoCtrls;
	CTDLContentMgr m_mgrContent;
	CWndPromptManager m_mgrPrompts;
	CMenuIconMgr m_mgrMenuIcons;
	CUIExtensionMgr m_mgrUIExtensions;

	CString m_sQuickFind;
	CString m_sThemeFile;
	CEnString m_sCurrentFocus;

	BOOL m_bVisible;
	BOOL m_bShowFilterBar;
	BOOL m_bShowProjectName;
	BOOL m_bShowStatusBar;
	BOOL m_bShowMainToolbar;
	BOOL m_bShowCustomToolbar;
	BOOL m_bShowTasklistBar;
	BOOL m_bShowTreeListBar;
	BOOL m_bInNewTask;
	BOOL m_bSaving;
	BOOL m_bInTimer;
	BOOL m_bClosing, m_bEndingSession;
	BOOL m_bFindShowing;
	BOOL m_bQueryOpenAllow;
	BOOL m_bPasswordPrompting;
	BOOL m_bReloading; // on startup
	BOOL m_bStartHidden;
	BOOL m_bLogging;
	BOOL m_bSaveUIVisInTaskList;
	BOOL m_bReshowTimeTrackerOnEnable;
	BOOL m_bSettingAttribDefs;
	BOOL m_bPromptLanguageChangeRestartOnActivate;
	BOOL m_bAllowForcedCheckOut;
	
	// Generated message map functions
	//{{AFX_MSG(CToDoListWnd)
	afx_msg void OnEditSetTasklistTabColor();
	afx_msg void OnEditClearTasklistTabColor();
	afx_msg void OnUpdateEditSetTasklistTabColor(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditClearTasklistTabColor(CCmdUI* pCmdUI);
	afx_msg void OnViewIncrementTaskViewFontSize();
	afx_msg void OnUpdateViewIncrementTaskViewFontSize(CCmdUI* pCmdUI);
	afx_msg void OnViewDecrementTaskViewFontSize();
	afx_msg void OnUpdateViewDecrementTaskViewFontSize(CCmdUI* pCmdUI);
	afx_msg void OnViewRestoreDefaultTaskViewFontSize();
	afx_msg void OnUpdateViewRestoreDefaultTaskViewFontSize(CCmdUI* pCmdUI);
	afx_msg void OnMoveGoToTask();
	afx_msg void OnUpdateMoveGoToTask(CCmdUI* pCmdUI);
	afx_msg void OnToolsCleanupIniPreferences();
	afx_msg void OnUpdateToolsCleanupIniPreferences(CCmdUI* pCmdUI);
	afx_msg void OnToolsToggleLogging();
	afx_msg void OnUpdateToolsToggleLogging(CCmdUI* pCmdUI);
	afx_msg void OnEditFindReplaceInTaskTitles();
	afx_msg void OnUpdateEditFindReplaceInTaskTitles(CCmdUI* pCmdUI);
	afx_msg void OnViewShowRemindersWindow();
	afx_msg void OnUpdateViewShowRemindersWindow(CCmdUI* pCmdUI);
	//}}AFX_MSG
	afx_msg void OnViewShowTimeTracker();
	afx_msg BOOL OnQueryOpen();
	afx_msg void OnAddtimetologfile();
	afx_msg void OnArchiveSelectedTasks();
	afx_msg void OnCloseallbutthis();
	afx_msg void OnCopyTaskasDependency();
	afx_msg void OnCopyTaskasDependencyFull();
	afx_msg void OnCopyTaskasPath();
	afx_msg void OnCopyTaskasLink();
	afx_msg void OnCopyTaskasLinkFull();
	afx_msg void OnEditClearReminder();
	afx_msg void OnEditCleartaskcolor();
	afx_msg void OnEditCleartaskicon();
	afx_msg void OnEditDectaskpercentdone();
	afx_msg void OnEditDectaskpriority();
	afx_msg void OnEditFlagtask();
	afx_msg void OnEditLocktask();
	afx_msg void OnEditInctaskpercentdone();
	afx_msg void OnEditInctaskpriority();
	afx_msg void OnEditInsertdate();
	afx_msg void OnEditInsertdatetime();
	afx_msg void OnEditInserttime();
	afx_msg void OnEditOffsetDates();
	afx_msg void OnEditOffsetDatesForwards(UINT nCmdID);
	afx_msg void OnEditOffsetDatesBackwards(UINT nCmdID);
	afx_msg void OnEditRedo();
	afx_msg void OnEditSelectall();
	afx_msg void OnEditSetReminder();
	afx_msg void OnEditSettaskicon();
	afx_msg void OnEditUndo();
	afx_msg void OnEnable(BOOL bEnable);
	afx_msg void OnFileChangePassword();
	afx_msg void OnFileOpenarchive();
	afx_msg void OnGotoNexttask();
	afx_msg void OnGotoPrevtask();
	afx_msg void OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct);
	afx_msg void OnMoveSelectTaskDependencies();
	afx_msg void OnMoveSelectTaskDependents();
	afx_msg void OnPrintpreview();
	afx_msg void OnQuickFind();
	afx_msg void OnQuickFindNext();
	afx_msg void OnQuickFindPrev();
	afx_msg void OnSendTasks();
	afx_msg void OnSendSelectedTasks();
	afx_msg void OnShowKeyboardshortcuts();
	afx_msg void OnShowTimelogfile();
	afx_msg void OnSortMulti();
	afx_msg void OnSysColorChange();
	afx_msg void OnTabctrlPreferences();
	afx_msg void OnTasklistSelectColumns();
	afx_msg void OnToolsCheckforupdates();
	afx_msg void OnToolsCopyTasklistPath();
	afx_msg void OnToolsTransformactivetasklist();
	afx_msg void OnUpdateAddtimetologfile(CCmdUI* pCmdUI);
	afx_msg void OnUpdateArchiveSelectedCompletedTasks(CCmdUI* pCmdUI);
	afx_msg void OnUpdateCloseallbutthis(CCmdUI* pCmdUI);
	afx_msg void OnUpdateCopyTaskasDependency(CCmdUI* pCmdUI);
	afx_msg void OnUpdateCopyTaskasDependencyFull(CCmdUI* pCmdUI);
	afx_msg void OnUpdateCopyTaskasPath(CCmdUI* pCmdUI);
	afx_msg void OnUpdateCopyTaskasLink(CCmdUI* pCmdUI);
	afx_msg void OnUpdateCopyTaskasLinkFull(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditClearReminder(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditCleartaskcolor(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditCleartaskicon(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditDectaskpercentdone(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditDectaskpriority(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditFlagtask(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditLocktask(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditInctaskpercentdone(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditInctaskpriority(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditInsertdate(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditInsertdatetime(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditInserttime(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditOffsetDates(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditOffsetDatesForwards(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditOffsetDatesBackwards(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditRedo(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditSelectall(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditSetReminder(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditSettaskicon(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditUndo(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFileChangePassword(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFileOpenarchive(CCmdUI* pCmdUI);
	afx_msg void OnUpdateGotoNexttask(CCmdUI* pCmdUI);
	afx_msg void OnUpdateGotoPrevtask(CCmdUI* pCmdUI);
	afx_msg void OnUpdateMoveSelectTaskDependencies(CCmdUI* pCmdUI);
	afx_msg void OnUpdateMoveSelectTaskDependents(CCmdUI* pCmdUI);
	afx_msg void OnUpdateQuickFind(CCmdUI* pCmdUI);
	afx_msg void OnUpdateQuickFindNext(CCmdUI* pCmdUI);
	afx_msg void OnUpdateQuickFindPrev(CCmdUI* pCmdUI);
	afx_msg void OnUpdateShowTimelogfile(CCmdUI* pCmdUI);
	afx_msg void OnUpdateSendTasks(CCmdUI* pCmdUI);
	afx_msg void OnUpdateSendSelectedTasks(CCmdUI* pCmdUI);
	afx_msg void OnUpdateToolsCopyTasklistPath(CCmdUI* pCmdUI);
	afx_msg void OnUpdateViewClearfilter(CCmdUI* pCmdUI);
	afx_msg void OnUpdateViewExpandTasks(CCmdUI* pCmdUI);
	afx_msg void OnUpdateViewFilter(CCmdUI* pCmdUI);
	afx_msg void OnUpdateViewProjectname(CCmdUI* pCmdUI);
	afx_msg void OnUpdateViewShowTasklistTabbar(CCmdUI* pCmdUI);
	afx_msg void OnUpdateViewShowTreeListTabbar(CCmdUI* pCmdUI);
	afx_msg void OnUpdateViewShowfilterbar(CCmdUI* pCmdUI);
	afx_msg void OnUpdateViewSorttasklisttabs(CCmdUI* pCmdUI);
	afx_msg void OnUpdateViewStatusBar(CCmdUI* pCmdUI);
	afx_msg void OnUpdateViewCycleTaskViews(CCmdUI* pCmdUI);
	afx_msg void OnUpdateViewToggleTreeandList(CCmdUI* pCmdUI);
	afx_msg void OnUpdateViewTogglefilter(CCmdUI* pCmdUI);
	afx_msg void OnUpdateViewToggletasksandcomments(CCmdUI* pCmdUI);
	afx_msg void OnUpdateWindow(CCmdUI* pCmdUI);
	afx_msg void OnViewClearfilter();
	afx_msg void OnViewExpandTasks(UINT nCmdID);
	afx_msg void OnViewFilter();
	afx_msg void OnViewProjectname();
	afx_msg void OnViewShowTasklistTabbar();
	afx_msg void OnViewShowTreeListTabbar();
	afx_msg void OnViewShowfilterbar();
	afx_msg void OnViewSorttasklisttabs();
	afx_msg void OnViewStatusBar();
	afx_msg void OnViewCycleTaskViews();
	afx_msg void OnViewToggleTreeandList();
	afx_msg void OnViewTogglefilter();
	afx_msg void OnViewToggletasksandcomments();
	afx_msg void OnTasklistCustomColumns();
	afx_msg void OnEditGotoDependency();
	afx_msg void OnUpdateEditGotoDependency(CCmdUI* pCmdUI);
	afx_msg void OnEditRecurrence();
	afx_msg void OnUpdateEditRecurrence(CCmdUI* pCmdUI);
	afx_msg BOOL OnQueryEndSession();
	afx_msg void OnEditClearAttribute();
	afx_msg void OnUpdateEditClearAttribute(CCmdUI* pCmdUI);
	afx_msg void OnEditClearFocusedAttribute();
	afx_msg void OnUpdateEditClearFocusedAttribute(CCmdUI* pCmdUI);
	afx_msg void OnUpdateTasklistCustomcolumns(CCmdUI* pCmdUI);
	afx_msg void OnEditPasteAsRef();
	afx_msg void OnUpdateEditPasteAsRef(CCmdUI* pCmdUI);
	afx_msg void OnEditSelectReferenceTarget();
	afx_msg void OnUpdateEditSelectReferenceTarget(CCmdUI* pCmdUI);
	afx_msg void OnEditSelectTaskReferences();
	afx_msg void OnUpdateEditSelectTaskReferences(CCmdUI* pCmdUI);
	afx_msg void OnToolsAnalyseLoggedTime();
	afx_msg void OnUpdateToolsAnalyseLoggedTime(CCmdUI* pCmdUI);
	afx_msg void OnToolsSelectinExplorer();
	afx_msg void OnUpdateToolsSelectinExplorer(CCmdUI* pCmdUI);
	afx_msg void OnToolsAddtoSourceControl();
	afx_msg void OnUpdateToolsAddtoSourceControl(CCmdUI* pCmdUI);
	afx_msg void OnViewResizeColsToFit();
	afx_msg void OnUpdateViewResizeColsToFit(CCmdUI* pCmdUI);
#if _MSC_VER >= 1400
	afx_msg void OnActivateApp(BOOL bActive, DWORD dwThreadID);
#else
	afx_msg void OnActivateApp(BOOL bActive, HTASK hTask);
#endif
	afx_msg BOOL OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg BOOL OnOpenRecentFile(UINT nID);
	afx_msg void OnEndSession(BOOL bEnding);
	afx_msg LRESULT OnUpdateUDTsInToolbar(WPARAM wp, LPARAM lp);
	afx_msg LRESULT OnAppRestoreFocus(WPARAM wp, LPARAM lp);
	afx_msg LRESULT OnDoInitialDueTaskNotify(WPARAM wp, LPARAM lp);
	afx_msg LRESULT OnExportThreadFinished(WPARAM wp, LPARAM lp);
	afx_msg LRESULT OnToDoCtrlNotifyClickReminderCol(WPARAM wp, LPARAM lp);
	afx_msg LRESULT OnDropFile(WPARAM wParam, LPARAM lParam);
	afx_msg void OnFileOpenFromUserStorage(UINT nCmdID);
	afx_msg void OnFileSaveToUserStorage(UINT nCmdID);
	afx_msg LRESULT OnFindApplyAsFilter(WPARAM wp, LPARAM lp);
	afx_msg LRESULT OnFindAddSearch(WPARAM wp, LPARAM lp);
	afx_msg LRESULT OnFindSaveSearch(WPARAM wp, LPARAM lp);
	afx_msg LRESULT OnFindDeleteSearch(WPARAM wp, LPARAM lp);
	afx_msg LRESULT OnFindDlgClose(WPARAM wp, LPARAM lp);
	afx_msg LRESULT OnFindDlgFind(WPARAM wp, LPARAM lp);
	afx_msg LRESULT OnFindSelectAll(WPARAM wp, LPARAM lp);
	afx_msg LRESULT OnFindSelectResult(WPARAM wp, LPARAM lp);
	afx_msg LRESULT OnFocusChange(WPARAM wp, LPARAM lp);
	afx_msg LRESULT OnGetFont(WPARAM wp, LPARAM lp);
	afx_msg LRESULT OnGetIcon(WPARAM bLargeIcon, LPARAM /*not used*/);
	afx_msg LRESULT OnHotkey(WPARAM wp, LPARAM lp);
	afx_msg LRESULT OnPostOnCreate(WPARAM wp, LPARAM lp);
	afx_msg LRESULT OnSessionStatusChange(WPARAM wp, LPARAM lp);
	afx_msg LRESULT OnPreferencesClearMRU(WPARAM wp, LPARAM lp);
	afx_msg LRESULT OnPreferencesEditLanguageFile(WPARAM wp, LPARAM lp);
	afx_msg LRESULT OnPreferencesTestTool(WPARAM wp, LPARAM lp);
	afx_msg LRESULT OnPostTranslateMenu(WPARAM wp, LPARAM lp);
	afx_msg LRESULT OnQuickFindItemAdded(WPARAM wp, LPARAM lp);
	afx_msg LRESULT OnSelchangeFilter(WPARAM wp, LPARAM lp);
	afx_msg LRESULT OnTimeTrackerStartTracking(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnTimeTrackerStopTracking(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnTimeTrackerResetElapsedTime(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnTimeTrackerLoadDelayedTasklist(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnToDoCtrlDoLengthyOperation(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnToDoCtrlSelectTask(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnToDoCtrlGetLinkTooltip(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnToDoCtrlFailedLink(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnToDoCtrlImportDropFiles(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnToDoCtrlCanImportDropFiles(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnToDoCtrlNotifyListChange(WPARAM wp, LPARAM lp);
	afx_msg LRESULT OnToDoCtrlNotifyMinWidthChange(WPARAM wp, LPARAM lp);
	afx_msg LRESULT OnToDoCtrlNotifyMod(WPARAM wp, LPARAM lp);
	afx_msg LRESULT OnToDoCtrlNotifyRecreateRecurringTask(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnToDoCtrlNotifyTimeTrack(WPARAM wp, LPARAM lp);
	afx_msg LRESULT OnToDoCtrlNotifyViewChange(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnToDoCtrlNotifyTimeTrackReminder(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnToDoCtrlNotifySourceControlSave(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnToDoCtrlGetTaskReminder(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnToDoCtrlIsTaskDone(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnToDoListIsClosing(WPARAM /*wp*/, LPARAM /*lp*/) { return m_bClosing; }
	afx_msg LRESULT OnToDoListRefreshPrefs(WPARAM wp, LPARAM lp);
	afx_msg LRESULT OnToDoListRestore(WPARAM wp, LPARAM lp);
	afx_msg LRESULT OnToDoListShowWindow(WPARAM wp, LPARAM lp);
	afx_msg LRESULT OnDismissReminder(WPARAM wp, LPARAM lp);
	afx_msg LRESULT OnClose(WPARAM /*wp*/, LPARAM bForUpdate);
	afx_msg void OnAbout();
	afx_msg void OnArchiveCompletedtasks();
	afx_msg void OnCloseTasklist();
	afx_msg void OnCloseall();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnDeleteAllTasks();
	afx_msg void OnDeleteTask();
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	afx_msg void OnEditChangeQuickFind();
	afx_msg void OnEditCopy();
	afx_msg void OnEditCopyashtml();
	afx_msg void OnEditCopyastext();
	afx_msg void OnEditCut();
	afx_msg void OnEditOpenfileref(UINT nCmdID);
	afx_msg void OnEditPasteAfter();
	afx_msg void OnEditPasteSub();
	afx_msg void OnEditAddFileLink();
	afx_msg void OnEditTaskcolor();
	afx_msg void OnEditTaskdone();
	afx_msg void OnEditTasktext();
	afx_msg void OnEditTimeTrackTask();
	afx_msg void OnExit();
	afx_msg void OnExport();
	afx_msg void OnFileEncrypt();
	afx_msg void OnFileResetversion();
	afx_msg void OnFindTasks();
	afx_msg void OnImportTasklist();
	afx_msg void OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu);
	afx_msg void OnLoad();
	afx_msg void OnMaximizeComments();
	afx_msg void OnMaximizeTasklist();
	afx_msg void OnMinimizeToTray();
	afx_msg void OnMove(int x, int y);
	afx_msg void OnMovetaskdown();
	afx_msg void OnMovetaskleft();
	afx_msg void OnMovetaskright();
	afx_msg void OnMovetaskup();
	afx_msg void OnNcLButtonDblClk(UINT nHitTest, CPoint point);
	afx_msg void OnNeedTooltipText(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNewTasklist();
	afx_msg void OnNewTask(UINT nCmdID);
	afx_msg void OnSplitTask(UINT nCmdID);
	afx_msg void OnNexttopleveltask();
	afx_msg void OnPreferences();
	afx_msg void OnPrevtopleveltask();
	afx_msg void OnPrint();
	afx_msg void OnReload();
	afx_msg void OnSave();
	afx_msg void OnSaveall();
	afx_msg void OnSaveas();
	afx_msg void OnSelChangeQuickFind();
	afx_msg void OnTabCtrlCloseTab(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnTabCtrlEndDrag(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnTabCtrlSelchange(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnTabCtrlSelchanging(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnTabCtrlGetBackColor(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnTabCtrlPostDrawTab(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSetPriority(UINT nCmdID);
	afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnShowTaskView(UINT nCmdID);
	afx_msg void OnSort();
	afx_msg void OnSortBy(UINT nCmdID);
	afx_msg void OnSpellcheckTasklist();
	afx_msg void OnSpellcheckcomments();
	afx_msg void OnSpellchecktitle();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnToolsCheckin();
	afx_msg void OnToolsCheckout();
	afx_msg void OnToolsShowtasksDue(UINT nCmdID);
 	afx_msg void OnToolsRemovefromsourcecontrol();
	afx_msg void OnToolsToggleCheckin();
	afx_msg void OnTrayIconClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnTrayIconDblClk(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnTrayIconRClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnTrayiconClose();
	afx_msg void OnTrayiconCreatetask();
	afx_msg void OnTrayiconShow();
	afx_msg void OnTrayiconShowDueTasks(UINT nCmdID);
	afx_msg void OnUpdateArchiveCompletedtasks(CCmdUI* pCmdUI);
	afx_msg void OnUpdateCloseall(CCmdUI* pCmdUI);
	afx_msg void OnUpdateDeletealltasks(CCmdUI* pCmdUI);
	afx_msg void OnUpdateDeletetask(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditCopy(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditCut(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditOpenfileref(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditPasteAfter(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditPasteSub(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditAddFileLink(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditTasktext(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditTimeTrackTask(CCmdUI* pCmdUI);
	afx_msg void OnUpdateExport(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFileEncrypt(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFileResetversion(CCmdUI* pCmdUI);
	afx_msg void OnUpdateImport(CCmdUI* pCmdUI);
	afx_msg void OnUpdateMaximizeComments(CCmdUI* pCmdUI);
	afx_msg void OnUpdateMaximizeTasklist(CCmdUI* pCmdUI);
	afx_msg void OnUpdateMovetaskdown(CCmdUI* pCmdUI);
	afx_msg void OnUpdateMovetaskleft(CCmdUI* pCmdUI);
	afx_msg void OnUpdateMovetaskright(CCmdUI* pCmdUI);
	afx_msg void OnUpdateMovetaskup(CCmdUI* pCmdUI);
	afx_msg void OnUpdateNewTasklist(CCmdUI* pCmdUI);
	afx_msg void OnUpdateNewTask(CCmdUI* pCmdUI);
	afx_msg void OnUpdateSplitTask(CCmdUI* pCmdUI);
	afx_msg void OnUpdateNexttopleveltask(CCmdUI* pCmdUI);
	afx_msg void OnUpdatePrevtopleveltask(CCmdUI* pCmdUI);
	afx_msg void OnUpdatePrint(CCmdUI* pCmdUI);
	afx_msg void OnUpdateRecentFileMenu(CCmdUI* pCmdUI);
	afx_msg void OnUpdateReload(CCmdUI* pCmdUI);
	afx_msg void OnUpdateSBSelectionCount(CCmdUI* pCmdUI);
	afx_msg void OnUpdateSBTaskCount(CCmdUI* pCmdUI);
	afx_msg void OnUpdateSave(CCmdUI* pCmdUI);
	afx_msg void OnUpdateSaveToWeb(CCmdUI* pCmdUI);
	afx_msg void OnUpdateSaveall(CCmdUI* pCmdUI);
	afx_msg void OnUpdateSaveas(CCmdUI* pCmdUI);
	afx_msg void OnUpdateSetPriority(CCmdUI* pCmdUI);
	afx_msg void OnUpdateShowTaskView(CCmdUI* pCmdUI);
	afx_msg void OnUpdateSort(CCmdUI* pCmdUI);
	afx_msg void OnUpdateSortBy(CCmdUI* pCmdUI);
	afx_msg void OnUpdateSpellcheckTasklist(CCmdUI* pCmdUI);
	afx_msg void OnUpdateSpellcheckcomments(CCmdUI* pCmdUI);
	afx_msg void OnUpdateSpellchecktitle(CCmdUI* pCmdUI);
	afx_msg void OnUpdateTaskcolor(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditTaskdone(CCmdUI* pCmdUI);
	afx_msg void OnUpdateToolsCheckin(CCmdUI* pCmdUI);
	afx_msg void OnUpdateToolsCheckout(CCmdUI* pCmdUI);
 	afx_msg void OnUpdateToolsRemovefromsourcecontrol(CCmdUI* pCmdUI);
	afx_msg void OnUpdateToolsToggleCheckin(CCmdUI* pCmdUI);
	afx_msg void OnUpdateUserTool(CCmdUI* pCmdUI);
	afx_msg void OnUpdateUserUIExtension(CCmdUI* pCmdUI);
	afx_msg void OnUpdateViewMovetasklistleft(CCmdUI* pCmdUI);
	afx_msg void OnUpdateViewMovetasklistright(CCmdUI* pCmdUI);
	afx_msg void OnUpdateViewNext(CCmdUI* pCmdUI);
	afx_msg void OnUpdateViewNextSel(CCmdUI* pCmdUI);
	afx_msg void OnUpdateViewPrev(CCmdUI* pCmdUI);
	afx_msg void OnUpdateViewPrevSel(CCmdUI* pCmdUI);
	afx_msg void OnUpdateViewRefreshfilter(CCmdUI* pCmdUI);
	afx_msg void OnUpdateViewSaveToImage(CCmdUI* pCmdUI);
	afx_msg void OnUpdateViewMainToolbar(CCmdUI* pCmdUI);
	afx_msg void OnUpdateViewCustomToolbar(CCmdUI* pCmdUI);
	afx_msg void OnUserTool(UINT nCmdID);
	afx_msg void OnUserUIExtension(UINT nCmdID);
	afx_msg void OnViewMovetasklistleft();
	afx_msg void OnViewMovetasklistright();
	afx_msg void OnViewNext();
	afx_msg void OnViewNextSel();
	afx_msg void OnViewPrev();
	afx_msg void OnViewPrevSel();
	afx_msg void OnViewRefreshfilter();
	afx_msg void OnViewSaveToImage();
	afx_msg void OnViewMainToolbar();
	afx_msg void OnViewCustomToolbar();
	afx_msg void OnWindow(UINT nCmdID);
#ifdef _DEBUG
	afx_msg void OnDebugEndSession();
	afx_msg void OnDebugShowSetupDlg();
	afx_msg void OnDebugShowReminderDlg();
	afx_msg void OnDebugUpdateTranslation();
	afx_msg void OnDebugCleanDictionaries();
#endif
	DECLARE_MESSAGE_MAP()
		
	// Pseudo-handlers
	void OnTimerReadOnlyStatus(int nCtrl = -1, BOOL bForceCheckRemote = FALSE);
	void OnTimerTimestampChange(int nCtrl = -1, BOOL bForceCheckRemote = FALSE);
	void OnTimerCheckoutStatus(int nCtrl = -1, BOOL bForceCheckRemote = FALSE);
	void OnTimerCheckReloadTasklists(int nCtrl = -1, BOOL bForceCheckRemote = FALSE);

	void OnTimerAutoSave();
	void OnTimerDueItems(int nCtrl = -1);
	void OnTimerTimeTracking();
	void OnTimerTimeTrackReminder();
	void OnTimerAutoMinimize();

	void OnChangeFilter(TDCFILTER& filter, const CString& sCustom, DWORD dwCustomFlags);
	void OnEditUndoRedo(BOOL bUndo);
	void OnUpdateEditUndoRedo(CCmdUI* pCmdUI, BOOL bUndo);

	void OnViewIncrementTaskViewFontSize(BOOL bLarger);
	void OnUpdateViewIncrementTaskViewFontSize(CCmdUI* pCmdUI, BOOL bLarger);

	void KillTimers();
	void RestoreTimers();

	virtual void LoadSettings();
	virtual void SaveSettings();
	void RestorePosition();
	void RestoreVisibility();
	
	TDC_FILE DelayOpenTaskList(LPCTSTR szFilePath); // 0 = failed, 1 = success, -1 = cancelled
	TDC_FILE OpenTaskList(LPCTSTR szFilePath, BOOL bNotifyDueTasks = TRUE); // 0 = failed, 1 = success, -1 = cancelled
	TDC_FILE OpenTaskList(CFilteredToDoCtrl* pCtrl, LPCTSTR szFilePath = NULL, TSM_TASKLISTINFO* pInfo = NULL);

	TDC_PREPAREPATH PrepareFilePath(CString& sFilePath, TSM_TASKLISTINFO* pInfo = NULL);

	BOOL ReloadTaskList(int nIndex, BOOL bNotifyDueTasks = TRUE, BOOL bNotifyError = TRUE);
	BOOL VerifyTaskListOpen(int nIndex, BOOL bWantNotifyDueTasks);
	BOOL ImportFile(LPCTSTR szFilePath, BOOL bSilent);

	void InitShortcutManager();
	void InitMenuIconManager();
	void InitUIFont();
	BOOL LoadMenubar();
	BOOL InitTrayIcon();
	BOOL InitMainToolbar();
	BOOL InitCustomToolbar();
	BOOL InitStatusbar();
	BOOL InitFilterbar();
	BOOL InitTimeTrackDlg();
	BOOL InitTabCtrl();
	BOOL InitFindDialog(BOOL bShow = FALSE);

	BOOL CreateNewTask(const CString& sTitle, TDC_INSERTWHERE nInsertWhere, BOOL bEdit = TRUE, DWORD dwDependency = 0);
	BOOL CanCreateNewTask(TDC_INSERTWHERE nInsertWhere, BOOL bDependent = FALSE) const;
	BOOL CreateNewDependentTask(const CString& sTitle, TDC_INSERTWHERE nInsertWhere, BOOL bEdit = TRUE);
	BOOL CanPasteTasks(TDC_PASTE nWhere, BOOL bAsRef) const;
	BOOL CanImportPasteFromClipboard() const;

	BOOL ProcessStartupOptions(const CTDCStartupOptions& startup, BOOL bStartup);
	void CheckMinWidth();
	void MinimizeToTray();
	void Show(BOOL bAllowToggle);
	void RefreshPauseTimeTracking();
	BOOL IsActivelyTimeTracking() const;
	void SetTimer(UINT nTimerID, BOOL bOn);
	void RefreshTabOrder();
	void UpdateGlobalHotkey();
	LPCTSTR GetFileFilter(BOOL bLoad) const;
	LPCTSTR GetDefaultFileExt(BOOL bLoad) const;
	BOOL WantTDLExtensionSupport(BOOL bLoad) const;
	void UpdateCwd();
	BOOL WantTasklistTabbarVisible() const;
	void UpdateAeroFeatures();
	void CopySelectedTasksToClipboard(TDC_TASKS2CLIPBOARD nAsFormat);
	void SetUITheme(const CString& sThemeFile);
	BOOL HasSysTrayOptions(int nOption1, int nOption2 = STO_NONE) const;
	void ProcessQuickFindTextChange(BOOL bComboSelChange);
	void RefreshFindTasksListData();
	void CheckCreateDefaultReminder(const CFilteredToDoCtrl& tdc, DWORD dwTaskID);
	BOOL GetFirstTaskReminder(const CFilteredToDoCtrl& tdc, const CDWordArray& aTaskIDs, TDCREMINDER& rem) const;
	BOOL GetAutoArchiveOptions(TDC_ARCHIVE& nRemove, BOOL& bRemoveFlagged) const;
	BOOL ValidateTaskLinkFilePath(CString& sPath) const;
	BOOL WantCheckRemoteFiles(int nCtrl, int nInterval, int& nElapsed) const;
	BOOL WantCheckReloadFiles(int nOption) const;

	BOOL HandleReservedShortcut(DWORD dwShortcut);
	BOOL SendShortcutCommand(UINT nCmdID);
	BOOL HandleEscapeTabReturn(MSG* pMsg);
	BOOL AppOverridesToDoCtrlProcessing(UINT nCmdID, DWORD dwShortcut) const;
	BOOL ProcessShortcut(MSG* pMsg);

	enum TIMETRACKSRC
	{
		FROM_TRACKER,
		FROM_TASKLIST,
		FROM_APP
	};
	void StartTimeTrackingTask(int nTDC, DWORD dwTaskID, TIMETRACKSRC nFrom);
	void StopTimeTrackingTask(int nTDC, TIMETRACKSRC nFrom);

	void RefreshFilterBarControls(BOOL bClearCheckboxHistory = FALSE);
	void RefreshFilterBarAdvancedFilters();

	void Resize(int cx = 0, int cy = 0, BOOL bMaximized = FALSE);
	BOOL CalcToDoCtrlRect(CRect& rect, int cx = 0, int cy = 0, BOOL bMaximized = FALSE);
	int ReposTabBar(CDeferWndMove& dwm, const CPoint& ptOrg, int nWidth, BOOL bCalcOnly = FALSE);
	BOOL GetFilterBarRect(CRect& rect) const;

	void PrepareEditMenu(CMenu* pMenu);
	void PrepareSortMenu(CMenu* pMenu);
	void AddUserStorageToMenu(CMenu* pMenu);

	void ShowFindDialog(BOOL bShow = TRUE);
	void UpdateFindDialogActiveTasklist(const CFilteredToDoCtrl* pCtrl = NULL);
	
	void PrepareToolbar(int nOption);
	void SetToolbarOption(int nOption);
	void UpdateUDTsInMainToolbar();
	void PopulateToolArgs(USERTOOLARGS& args) const;

	CFilteredToDoCtrl& GetToDoCtrl();
	CFilteredToDoCtrl& GetToDoCtrl(int nIndex);
	const CFilteredToDoCtrl& GetToDoCtrl() const;
	const CFilteredToDoCtrl& GetToDoCtrl(int nIndex) const;
	CFilteredToDoCtrl* NewToDoCtrl(BOOL bVisible = TRUE, BOOL bEnabled = TRUE);
	int AddToDoCtrl(CFilteredToDoCtrl* pCtrl, TSM_TASKLISTINFO* pInfo = NULL, BOOL bResizeDlg = TRUE);
	inline int GetTDCCount() const { return m_mgrToDoCtrls.GetCount(); }
	BOOL SelectToDoCtrl(LPCTSTR szFilePath, BOOL bCheckPassword, int nNotifyDueTasksBy = -1);
	BOOL SelectToDoCtrl(int nIndex, BOOL bCheckPassword, int nNotifyDueTasksBy = -1);
	int GetSelToDoCtrl() const;
	BOOL CreateNewTaskList(BOOL bAddDefTask);
	BOOL VerifyToDoCtrlPassword() const;
	BOOL VerifyToDoCtrlPassword(int nIndex) const;
	BOOL SelectTask(CFilteredToDoCtrl& tdc, DWORD dwTaskID);

	// caller must flush todoctrls if required before calling these
	BOOL CloseToDoCtrl(int nIndex);
	TDC_FILE ConfirmSaveTaskList(int nIndex, DWORD dwFlags = 0);
	TDC_FILE SaveTaskList(int nIndex, LPCTSTR szFilePath = NULL, BOOL bAuto = FALSE); // returns FALSE only if the user was prompted to save and cancelled
	TDC_FILE SaveAll(DWORD dwFlags); // returns FALSE only if the user was prompted to save and cancelled
	
	void UpdateTooltip();
	void UpdateCaption();
	void UpdateStatusbar();
	void UpdateSBPaneAndTooltip(UINT nIDPane, UINT nIDTextFormat, const CString& sValue, UINT nIDTooltip, TDC_COLUMN nTDCC);
	void UpdateStatusBarInfo(const CFilteredToDoCtrl& tdc, TDCSTATUSBARINFO& sbi) const;
	void UpdateMenuIconMgrSourceControlStatus();
	void UpdateMenuBackgroundColor();
	void UpdateTimeTrackerPreferences();
	void UpdateWindowIcons();

	void UpdateToDoCtrlPreferences(CFilteredToDoCtrl* pCtrl, BOOL bFirst);
	void UpdateActiveToDoCtrlPreferences();
	const CPreferencesDlg& Prefs() const;
	void ResetPrefs();
	
	// helpers
	int GetTasks(CFilteredToDoCtrl& tdc, BOOL bHtmlComments, BOOL bTransform, 
					const CTaskSelectionDlg& taskSel, CTaskFile& tasks, LPCTSTR szHtmlImageDir) const;
	int GetTasks(CFilteredToDoCtrl& tdc, BOOL bHtmlComments, BOOL bTransform, 
					TSD_TASKS nWhatTasks, TDCGETTASKS& filter, DWORD dwSelFlags, CTaskFile& tasks, LPCTSTR szHtmlImageDir) const;
	
	void DoSendTasks(BOOL bSelected);
	void DoPreferences(int nInitPage = -1);
	void DoPrint(BOOL bPreview = FALSE);
	BOOL DoDueTaskNotification(int nTDC, int nDueBy);
	BOOL DoTaskLink(const CString& sPath, DWORD dwTaskID, BOOL bStartup);
	void DoInsertDateAndTime(BOOL bDate, BOOL bTime);
	BOOL DoImportPasteFromClipboard(TDLID_IMPORTTO nWhere);
	TDC_FILE DoSaveWithBackupAndProgress(CFilteredToDoCtrl& tdc, int nIndex, CTaskFile& tasks, LPCTSTR szFilePath = NULL);
	BOOL DoExit(BOOL bRestart = FALSE, BOOL bClosingWindows = FALSE);

	TDCEXPORTTASKLIST* PrepareNewDueTaskNotification(int nTDC, int nDueBy);
	TDCEXPORTTASKLIST* PrepareNewExportAfterSave(int nTDC, const CTaskFile& tasks);

	void UpdateTimeTrackerTasks(const CFilteredToDoCtrl& tdc, BOOL bAllTasks);
	BOOL ImportTasks(BOOL bFromClipboard, const CString& sImportFrom,
					int nImporter, TDLID_IMPORTTO nImportTo);
	BOOL Export2Html(const CTaskFile& tasks, const CString& sFilePath, 
					const CString& sStylesheet) const;
	BOOL CreateTempPrintFile(const CTDLPrintDialog& dlg, const CString& sFilePath);
	UINT GetNewTaskCmdID() const;
	UINT GetNewSubtaskCmdID() const;

	static UINT MapNewTaskPos(PUIP_NEWTASKPOS nPos, BOOL bSubtask);
	static void HandleImportTasklistError(IIMPORTEXPORT_RESULT nErr, const CString& sImportPath, BOOL bFromClipboard, BOOL bAnyTasksSucceeded);
	static void HandleExportTasklistError(IIMPORTEXPORT_RESULT nErr);
	static void HandleLoadTasklistError(TDC_FILE& nErr, LPCTSTR szTasklist);
	static BOOL HandleSaveTasklistError(TDC_FILE& nErr, LPCTSTR szTasklist); // Note the 'reference'

	static void EnableTDLExtension(BOOL bEnable, BOOL bStartup);
	static void EnableTDLProtocol(BOOL bEnable, BOOL bStartup);
	static void SetupUIStrings();
	static void EnableDynamicMenuTranslation(BOOL bEnable);
	static CString GetEndSessionFilePath();
	static BOOL IsEndSessionFilePath(const CString& sFilePath);
	static BOOL LogIntermediateTaskList(CTaskFile& tasks, LPCTSTR szRefPath);
	static CString GetIntermediateTaskListPath(LPCTSTR szRefPath);
	static void ProcessProtocolRegistrationFailure(BOOL bStartup, BOOL bExistingReg, UINT nMsgID, LPCTSTR szCheckPrefKey);
	static BOOL GetStylesheetPath(const CFilteredToDoCtrl& tdc, CString& sDlgStylesheet);

	BOOL UpdateLanguageTranslationAndCheckForRestart(const CPreferencesDlg& oldPrefs);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TODOLISTWND_H__13051D32_D372_4205_BA71_05FAC2159F1C__INCLUDED_)
