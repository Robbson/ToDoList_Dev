// TDLTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "TDLTest.h"

#include <crtdbg.h>
#include <objbase.h>

#include "DateHelperTest.h"
#include "FileMiscTest.h"
#include "ImportExportMgrTest.h"
#include "MiscTest.h"
#include "PreferencesTest.h"
#include "RemapMenuIDsTest.h"
#include "SEARCHPARAMSTest.h"
#include "TaskFileTest.h"
#include "TDCCustomAttributeCalculationTest.h"
#include "TDCDependencyTest.h"
#include "TDCFileBackupTest.h"
#include "TDCRecurrenceTest.h"
#include "TDCTaskCalculatorTest.h"
#include "TDCTaskTimeLogTest.h"
#include "TDLTaskCtrlBaseTest.h"
#include "TDCTaskMatcherTest.h"
#include "TimeHelperTest.h"
#include "ToDoCtrlDataTest.h"
#include "ToDoCtrlTaskLinkTest.h"
#include "UIPerfTest.h"
#include "WorkingWeekTest.h"
#include "XmlFileTest.h"

#include "..\shared\EnCommandLineInfo.h"

using namespace std; // for console output

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// The one and only application object

CWinApp theApp;

/////////////////////////////////////////////////////////////////////////////

// local functions
void DoTests(const CTestUtils& utils);

/////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
static CString s_sAssertLogPath;

static void GetAssertLogPath(CString& sPath)
{
	TCHAR szEnv[MAX_PATH * 2] = { 0 };
	DWORD dwLen = ::GetEnvironmentVariable(_T("TDL_ASSERT_LOG"), szEnv, _countof(szEnv));

	if ((dwLen > 0) && (dwLen < _countof(szEnv)) && (lstrcmpi(szEnv, _T("1")) != 0))
	{
		sPath = szEnv;
		return;
	}

	TCHAR szTemp[MAX_PATH] = { 0 };
	::GetTempPath(_countof(szTemp), szTemp);
	sPath.Format(_T("%sTDLTest_asserts.log"), szTemp);
}

static int __cdecl TDLTestAssertReportHook(int nReportType, char* szMessage, int* pnReturnValue)
{
	if ((nReportType != _CRT_ASSERT) && (nReportType != _CRT_ERROR))
		return FALSE;

	if (s_sAssertLogPath.IsEmpty())
		GetAssertLogPath(s_sAssertLogPath);

	FILE* pFile = NULL;

	if ((_tfopen_s(&pFile, s_sAssertLogPath, _T("a")) != 0) || !pFile)
		return FALSE; // fall back to default handling (dialog/break)

	SYSTEMTIME st = { 0 };
	::GetLocalTime(&st);

	fprintf(pFile,
			"[%04d-%02d-%02d %02d:%02d:%02d] [pid %lu] %s\n",
			st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond,
			::GetCurrentProcessId(), (szMessage ? szMessage : "(null)"));

	fflush(pFile);
	fclose(pFile);

	if (szMessage)
		::OutputDebugStringA(szMessage);

	if (pnReturnValue)
		*pnReturnValue = 0;

	return TRUE; // handled: suppress dialog and debug break
}

static void InitAssertLogging()
{
	TCHAR szEnv[MAX_PATH * 2] = { 0 };
	DWORD dwLen = ::GetEnvironmentVariable(_T("TDL_ASSERT_LOG"), szEnv, _countof(szEnv));

	// Allow interactive debugging to restore the normal dialog
	if ((dwLen > 0) && (dwLen < _countof(szEnv)) && (lstrcmpi(szEnv, _T("0")) == 0))
		return;

	_CrtSetReportHook2(_CRT_RPTHOOK_INSTALL, TDLTestAssertReportHook);
}
#endif

/////////////////////////////////////////////////////////////////////////////

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
#ifdef _DEBUG
	InitAssertLogging();
#endif

	// initialize MFC and print and error on failure
	if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
	{
		// TODO: change error code to suit your needs
		cerr << _T("Fatal Error: MFC initialization failed") << endl;
		return 1;
	}

	// CTaskFile needs MSXML, which requires COM on this thread
	HRESULT hrCoInit = ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

//	CTDLTestBase::SelfTest();

	CEnCommandLineInfo cmdInfo;
	theApp.ParseCommandLine(cmdInfo);

	CString sOutputDir;// = cmdInfo...;
	CString sControlDir;// = cmdInfo...;

	CTestUtils utils(cmdInfo);

// 	if (!utils.Initialise(sOutputDir, sControlDir))
// 	{
// 		cerr << _T("Fatal Error: Bad paths passed on command line") << endl;
// 		return 1;
// 	}

	DoTests(utils);

	if (SUCCEEDED(hrCoInit))
		::CoUninitialize();

	return 0;
}

#define DOTEST(test) test(utils).Run()

void DoTests(const CTestUtils& utils)
{
	TESTRESULT res;
	DWORD dwStartTick = GetTickCount();

	// UI harness is opt-in and runs on its own so a normal test run is not delayed
	// by launching and driving the GUI.
	if (utils.HasCommandlineFlag(_T("ui")))
	{
		res += DOTEST(CUIPerfTest);
		res.ReportResults(_T("UIPerf"), FALSE);

		_tprintf(_T("\n  %d tests took %d seconds\n"), res.GetTotal(), (((GetTickCount() - dwStartTick) / 1000) + 1));

		if (res.nNumError)
			_tprintf(_T("  %d UI test(s) FAILED\n"), res.nNumError);

		return;
	}

	// Keep sorted for easy scanning
	res += DOTEST(CDateHelperTest);
	res += DOTEST(CFileMiscTest);
	res += DOTEST(CImportExportMgrTest);
	res += DOTEST(CMiscTest);
	res += DOTEST(CPreferencesTest);
	res += DOTEST(CRemapMenuIDsTest);
	res += DOTEST(CSEARCHPARAMSTest);
	res += DOTEST(CTaskFileTest);
	res += DOTEST(CTDCCUSTOMATTRIBUTECALCULATIONTest);
	res += DOTEST(CTDCDEPENDENCYTest);
	res += DOTEST(CTDCRECURRENCETest);
	res += DOTEST(CTDCTaskCalculatorTest);
	res += DOTEST(CTDCTaskTimeLogTest);
	res += DOTEST(CTDCTaskMatcherTest);
	res += DOTEST(CTDLTaskCtrlBaseTest);
	res += DOTEST(CTimeHelperTest);
	res += DOTEST(CToDoCtrlDataTest);
	res += DOTEST(CTDCFileBackupTest);
	res += DOTEST(CToDoCtrlTaskLinkTest);
	res += DOTEST(CUIPerfTest);
	res += DOTEST(CWorkingDayTest);
	res += DOTEST(CWorkingWeekTest);
	res += DOTEST(CXmlFileTest);

	// More...

	res.ReportResults(_T("All"), FALSE);

	_tprintf(_T("\n  %d tests took %d seconds\n"), res.GetTotal(), (((GetTickCount() - dwStartTick) / 1000) + 1));

	// When we are NOT DEBUGGING we just assert once at the end
	// else we assert in the actual test that failed
	if (!::IsDebuggerPresent())
		ASSERT(res.nNumError == 0);
}
