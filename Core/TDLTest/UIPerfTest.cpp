// UIPerfTest.cpp: UI performance and artefact harness (opt-in via -ui)
//
// Launches ToDoList.exe against a temporary fixture, drives the real UI with
// SendInput and compares on-screen pixels with a forced-repaint reference to
// detect stale/intermediate painting.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TDLTest.h"
#include "UIPerfTest.h"

#include "..\todolist\TaskFile.h"
#include "..\shared\FileMisc.h"
#include "..\shared\Misc.h"

#include <commctrl.h>
#include <algorithm>
#include <vector>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

using namespace std;

//////////////////////////////////////////////////////////////////////

namespace
{
	// Control IDs. See Core/ToDoList/TDCEnum.h and Core/ToDoList/resource.h
	const int TDL_IDC_CONTAINER			= 101;		// IDC_TODOLIST
	const int TDL_IDC_TASKTREECTRL		= 101;		// IDC_TASKTREECTRL (child scope)
	const int TDL_IDC_TASKLISTCTRL		= 365;		// IDC_TASKLISTCTRL
	const int TDL_IDC_ATTRIBUTELIST		= 1345;		// IDC_ATTRIBUTELIST

	const UINT TDL_CMD_TOGGLETREEANDLIST = 34238;	// ID_VIEW_TOGGLETREEANDLIST
	const UINT TDL_CMD_ACTIVATELISTVIEW = 34645;	// ID_ACTIVATEVIEW_LISTVIEW

	const int DEFAULT_STEPS				= 20;
	const int DEFAULT_STEP_DELAY_MS		= 25;
	const int DEFAULT_PIXEL_TOLERANCE	= 2000;

	// ---------------------------------------------------------------

	struct CAPTURE
	{
		CAPTURE() : cx(0), cy(0) {}

		int cx, cy;
		CRect rcScreen;
		vector<DWORD> aPixels;
	};

	struct FRAMEDIFF
	{
		FRAMEDIFF() : nPixels(0) {}

		int nPixels;
		CRect rcBounds;
	};

	struct DRAGSTATS
	{
		DRAGSTATS() : nSteps(0), nArtifactSteps(0), nMaxDiffPixels(0),
					  nMaxCleanDiff(0), dwElapsedMs(0), dwGdiStart(0), dwGdiEnd(0) {}

		int nSteps;
		int nArtifactSteps;
		int nMaxDiffPixels;
		int nMaxCleanDiff;
		DWORD dwElapsedMs;
		DWORD dwGdiStart;
		DWORD dwGdiEnd;
	};

	struct SCROLLSTATS
	{
		SCROLLSTATS() : nSteps(0), nSyncErrors(0), nArtifactSteps(0),
						nMaxDiffPixels(0), nFirstTopIndex(0), nLastTopIndex(0) {}

		int nSteps;
		int nSyncErrors;
		int nArtifactSteps;
		int nMaxDiffPixels;
		int nFirstTopIndex;
		int nLastTopIndex;
	};

	// ---------------------------------------------------------------

	int PixelDiff(DWORD p1, DWORD p2)
	{
		int nR = abs((int)GetRValue(p1) - (int)GetRValue(p2));
		int nG = abs((int)GetGValue(p1) - (int)GetGValue(p2));
		int nB = abs((int)GetBValue(p1) - (int)GetBValue(p2));

		return (std::max)(nR, (std::max)(nG, nB));
	}

	BOOL CaptureClient(HWND hWnd, CAPTURE& capture)
	{
		CRect rcClient;
		::GetClientRect(hWnd, &rcClient);

		if (rcClient.IsRectEmpty())
			return FALSE;

		CPoint ptOrigin(0, 0);
		::ClientToScreen(hWnd, &ptOrigin);

		capture.cx = rcClient.Width();
		capture.cy = rcClient.Height();
		capture.rcScreen = CRect(ptOrigin, rcClient.Size());
		capture.aPixels.assign(capture.cx * capture.cy, 0);

		HDC hScreenDC = ::GetDC(NULL);
		HDC hMemDC = ::CreateCompatibleDC(hScreenDC);

		BITMAPINFO bmi = { 0 };
		bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmi.bmiHeader.biWidth = capture.cx;
		bmi.bmiHeader.biHeight = -capture.cy; // top-down
		bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biBitCount = 32;
		bmi.bmiHeader.biCompression = BI_RGB;

		void* pBits = NULL;
		HBITMAP hbm = ::CreateDIBSection(hScreenDC, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
		BOOL bRes = FALSE;

		if (hbm && pBits)
		{
			HGDIOBJ hOld = ::SelectObject(hMemDC, hbm);

			::GdiFlush();
			bRes = ::BitBlt(hMemDC, 0, 0, capture.cx, capture.cy,
							hScreenDC, ptOrigin.x, ptOrigin.y, SRCCOPY);
			::GdiFlush();

			if (bRes)
			{
				const DWORD* pSrc = (const DWORD*)pBits;
				std::copy(pSrc, pSrc + capture.aPixels.size(), capture.aPixels.begin());
			}

			::SelectObject(hMemDC, hOld);
		}

		if (hbm)
			::DeleteObject(hbm);

		::DeleteDC(hMemDC);
		::ReleaseDC(NULL, hScreenDC);

		return bRes;
	}

	FRAMEDIFF CompareCaptures(const CAPTURE& c1, const CAPTURE& c2, int nChannelTol)
	{
		FRAMEDIFF diff;

		if (c1.cx != c2.cx || c1.cy != c2.cy)
		{
			diff.nPixels = (c1.cx * c1.cy);
			diff.rcBounds = CRect(0, 0, (std::max)(c1.cx, c2.cx), (std::max)(c1.cy, c2.cy));
			return diff;
		}

		for (int y = 0; y < c1.cy; y++)
		{
			const DWORD* p1 = &c1.aPixels[y * c1.cx];
			const DWORD* p2 = &c2.aPixels[y * c2.cx];

			for (int x = 0; x < c1.cx; x++)
			{
				if (PixelDiff(p1[x], p2[x]) > nChannelTol)
				{
					diff.nPixels++;

					CRect rcPixel(x, y, x + 1, y + 1);

					if (diff.rcBounds.IsRectEmpty())
						diff.rcBounds = rcPixel;
					else
						diff.rcBounds.UnionRect(diff.rcBounds, rcPixel);
				}
			}
		}

		return diff;
	}

	BOOL SaveCaptureBmp(const CAPTURE& capture, LPCTSTR szPath)
	{
		if (capture.aPixels.empty())
			return FALSE;

		BITMAPINFOHEADER bih = { 0 };
		bih.biSize = sizeof(BITMAPINFOHEADER);
		bih.biWidth = capture.cx;
		bih.biHeight = capture.cy;
		bih.biPlanes = 1;
		bih.biBitCount = 32;
		bih.biCompression = BI_RGB;
		bih.biSizeImage = (capture.cx * capture.cy * 4);

		BITMAPFILEHEADER bfh = { 0 };
		bfh.bfType = 0x4D42; // 'BM'
		bfh.bfOffBits = (sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER));
		bfh.bfSize = (bfh.bfOffBits + bih.biSizeImage);

		CFile file;

		if (!file.Open(szPath, CFile::modeCreate | CFile::modeWrite))
			return FALSE;

		file.Write(&bfh, sizeof(bfh));
		file.Write(&bih, sizeof(bih));

		for (int y = (capture.cy - 1); y >= 0; y--)
			file.Write(&capture.aPixels[y * capture.cx], (capture.cx * 4));

		return TRUE;
	}

	void ForceRepaint(HWND hWnd)
	{
		::RedrawWindow(hWnd, NULL, NULL,
					   RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN | RDW_FRAME);
		::GdiFlush();
		::Sleep(20);
	}

	// ---------------------------------------------------------------

	void SendMouseAbsolute(DWORD dwFlags, CPoint ptScreen)
	{
		INPUT input = { 0 };
		input.type = INPUT_MOUSE;
		input.mi.dwFlags = (dwFlags | MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK);

		int nVX = ::GetSystemMetrics(SM_XVIRTUALSCREEN);
		int nVY = ::GetSystemMetrics(SM_YVIRTUALSCREEN);
		int nVW = (std::max)(1, ::GetSystemMetrics(SM_CXVIRTUALSCREEN) - 1);
		int nVH = (std::max)(1, ::GetSystemMetrics(SM_CYVIRTUALSCREEN) - 1);

		input.mi.dx = (LONG)MulDiv(ptScreen.x - nVX, 65535, nVW);
		input.mi.dy = (LONG)MulDiv(ptScreen.y - nVY, 65535, nVH);

		::SendInput(1, &input, sizeof(INPUT));
	}

	void SendMouseWheel(CPoint ptScreen, int nDelta)
	{
		SendMouseAbsolute(MOUSEEVENTF_MOVE, ptScreen);
		::Sleep(20);

		INPUT input = { 0 };
		input.type = INPUT_MOUSE;
		input.mi.dwFlags = MOUSEEVENTF_WHEEL;
		input.mi.mouseData = (DWORD)nDelta;

		::SendInput(1, &input, sizeof(INPUT));
	}

	// ---------------------------------------------------------------

	struct FINDBYIDCTX
	{
		int nID;
		HWND hFound;
	};

	BOOL CALLBACK EnumFindByIdProc(HWND hWnd, LPARAM lParam)
	{
		FINDBYIDCTX* pCtx = (FINDBYIDCTX*)lParam;

		if (::GetDlgCtrlID(hWnd) == pCtx->nID)
		{
			pCtx->hFound = hWnd;
			return FALSE;
		}

		return TRUE;
	}

	HWND FindDescendantById(HWND hParent, int nID)
	{
		FINDBYIDCTX ctx = { nID, NULL };
		::EnumChildWindows(hParent, EnumFindByIdProc, (LPARAM)&ctx);
		return ctx.hFound;
	}

	struct FINDBYCLASSCTX
	{
		CString sClass;
		HWND hFound;
	};

	BOOL CALLBACK EnumFindByClassProc(HWND hWnd, LPARAM lParam)
	{
		FINDBYCLASSCTX* pCtx = (FINDBYCLASSCTX*)lParam;

		TCHAR szClass[64] = { 0 };
		::GetClassName(hWnd, szClass, 63);

		if (pCtx->sClass.CompareNoCase(szClass) == 0)
		{
			pCtx->hFound = hWnd;
			return FALSE;
		}

		return TRUE;
	}

	HWND FindDescendantByClass(HWND hParent, LPCTSTR szClass)
	{
		FINDBYCLASSCTX ctx = { szClass, NULL };
		::EnumChildWindows(hParent, EnumFindByClassProc, (LPARAM)&ctx);
		return ctx.hFound;
	}

	struct COLLECTBYCLASSCTX
	{
		CString sClass;
		int nMax;
		vector<HWND>* pResults;
	};

	BOOL CALLBACK EnumCollectByClassProc(HWND hWnd, LPARAM lParam)
	{
		COLLECTBYCLASSCTX* pCtx = (COLLECTBYCLASSCTX*)lParam;

		if ((int)pCtx->pResults->size() >= pCtx->nMax)
			return FALSE;

		TCHAR szClass[64] = { 0 };
		::GetClassName(hWnd, szClass, 63);

		if (pCtx->sClass.CompareNoCase(szClass) == 0)
			pCtx->pResults->push_back(hWnd);

		return TRUE;
	}

	void CollectDescendantsByClass(HWND hParent, LPCTSTR szClass, vector<HWND>& results, int nMax = 32)
	{
		COLLECTBYCLASSCTX ctx = { szClass, nMax, &results };
		::EnumChildWindows(hParent, EnumCollectByClassProc, (LPARAM)&ctx);
	}

	BOOL IsDialogClass(HWND hWnd)
	{
		TCHAR szClass[64] = { 0 };
		::GetClassName(hWnd, szClass, 63);
		return (lstrcmpi(szClass, _T("#32770")) == 0);
	}

	struct TOPLEVELCTX
	{
		DWORD dwPID;
		HWND hWnd;
	};

	HWND FindTaskContainer(HWND hMain);

	BOOL CALLBACK EnumTopLevelProc(HWND hWnd, LPARAM lParam)
	{
		TOPLEVELCTX* pCtx = (TOPLEVELCTX*)lParam;

		DWORD dwPID = 0;
		::GetWindowThreadProcessId(hWnd, &dwPID);

		if ((dwPID == pCtx->dwPID) && ::IsWindowVisible(hWnd) && (::GetWindow(hWnd, GW_OWNER) == NULL))
		{
			// Skip splash screens: only accept a frame that hosts a tasklist
			if (FindTaskContainer(hWnd))
			{
				pCtx->hWnd = hWnd;
				return FALSE;
			}
		}

		return TRUE;
	}

	HWND FindTopLevelWnd(DWORD dwPID)
	{
		TOPLEVELCTX ctx = { dwPID, NULL };
		::EnumWindows(EnumTopLevelProc, (LPARAM)&ctx);
		return ctx.hWnd;
	}

	HWND FindTaskContainer(HWND hMain)
	{
		HWND hChild = ::GetWindow(hMain, GW_CHILD);

		while (hChild)
		{
			if ((::GetDlgCtrlID(hChild) == TDL_IDC_CONTAINER) && IsDialogClass(hChild))
			{
				if (FindDescendantByClass(hChild, _T("SysTreeView32")) ||
					FindDescendantByClass(hChild, _T("SysListView32")))
				{
					return hChild;
				}
			}

			hChild = ::GetWindow(hChild, GW_HWNDNEXT);
		}

		return NULL;
	}

	// ---------------------------------------------------------------

	class CAppRunner
	{
	public:
		CAppRunner() : m_hProcess(NULL), m_dwPID(0), m_hMainWnd(NULL)
		{
			ZeroMemory(&m_pi, sizeof(m_pi));
		}

		~CAppRunner()
		{
			Close();
		}

		BOOL Launch(const CString& sExePath, const CString& sArgs)
		{
			STARTUPINFO si = { sizeof(si) };
			si.dwFlags = STARTF_USESHOWWINDOW;
			si.wShowWindow = SW_SHOWNORMAL;

			CString sCmdLine;
			sCmdLine.Format(_T("\"%s\" %s"), (LPCTSTR)sExePath, (LPCTSTR)sArgs);

			CString sWorkingDir = FileMisc::GetFolderFromFilePath(sExePath);
			LPCTSTR szWorkingDir = (sWorkingDir.IsEmpty() ? NULL : (LPCTSTR)sWorkingDir);

			LPTSTR szCmdLine = sCmdLine.GetBuffer();

			BOOL bLaunched = ::CreateProcess(NULL, szCmdLine, NULL, NULL, FALSE, 0, NULL,
											 szWorkingDir, &si, &m_pi);
			sCmdLine.ReleaseBuffer();

			if (!bLaunched)
				return FALSE;

			m_hProcess = m_pi.hProcess;
			m_dwPID = m_pi.dwProcessId;

			return TRUE;
		}

		BOOL WaitForMainWnd(HWND& hWnd, int nTimeoutMs)
		{
			DWORD dwStart = ::GetTickCount();

			while ((int)(::GetTickCount() - dwStart) < nTimeoutMs)
			{
				if (!IsRunning())
					return FALSE;

				hWnd = FindTopLevelWnd(m_dwPID);

				if (hWnd)
				{
					m_hMainWnd = hWnd;
					return TRUE;
				}

				::Sleep(100);
			}

			return FALSE;
		}

		BOOL IsRunning() const
		{
			return (m_hProcess && (::WaitForSingleObject(m_hProcess, 0) == WAIT_TIMEOUT));
		}

		DWORD GetGDICount() const
		{
			return (m_hProcess ? ::GetGuiResources(m_hProcess, GR_GDIOBJECTS) : 0);
		}

		HWND GetMainWnd() const { return m_hMainWnd; }
		HANDLE GetProcessHandle() const { return m_hProcess; }

		void Close()
		{
			if (m_pi.hProcess)
			{
				if (IsRunning())
					::TerminateProcess(m_pi.hProcess, 0);

				::WaitForSingleObject(m_pi.hProcess, 2000);
				::CloseHandle(m_pi.hThread);
				::CloseHandle(m_pi.hProcess);

				ZeroMemory(&m_pi, sizeof(m_pi));
			}

			m_hProcess = NULL;
			m_hMainWnd = NULL;
		}

	protected:
		PROCESS_INFORMATION m_pi;
		HANDLE m_hProcess;
		DWORD m_dwPID;
		HWND m_hMainWnd;
	};

	// ---------------------------------------------------------------

	BOOL GetSplitBarPoint(HWND hContainer, HWND& hTaskCtrl, CPoint& ptBar, BOOL& bVertical)
	{
		hTaskCtrl = FindDescendantById(hContainer, TDL_IDC_TASKTREECTRL);
		HWND hAttribs = FindDescendantById(hContainer, TDL_IDC_ATTRIBUTELIST);

		if (!hTaskCtrl || !hAttribs)
			return FALSE;

		if (!::IsWindowVisible(hTaskCtrl) || !::IsWindowVisible(hAttribs))
			return FALSE;

		CRect rcTask, rcAttribs;
		::GetWindowRect(hTaskCtrl, &rcTask);
		::GetWindowRect(hAttribs, &rcAttribs);

		if (rcTask.IsRectEmpty() || rcAttribs.IsRectEmpty())
			return FALSE;

		if ((rcAttribs.left >= rcTask.right) || (rcAttribs.right <= rcTask.left))
		{
			bVertical = TRUE;

			// The bar starts immediately outside the task view
			int x = ((rcAttribs.left >= rcTask.right) ? (rcTask.right + 1) : (rcTask.left - 1));
			int yTop = (std::max)(rcTask.top, rcAttribs.top);
			int yBottom = (std::min)(rcTask.bottom, rcAttribs.bottom);

			if (yBottom <= yTop)
			{
				yTop = (std::min)(rcTask.top, rcAttribs.top);
				yBottom = (std::max)(rcTask.bottom, rcAttribs.bottom);
			}

			ptBar = CPoint(x, ((yTop + yBottom) / 2));
			return TRUE;
		}

		if ((rcAttribs.top >= rcTask.bottom) || (rcAttribs.bottom <= rcTask.top))
		{
			bVertical = FALSE;

			int y = ((rcAttribs.top >= rcTask.bottom) ? (rcTask.bottom + 1) : (rcTask.top - 1));
			int xLeft = (std::max)(rcTask.left, rcAttribs.left);
			int xRight = (std::min)(rcTask.right, rcAttribs.right);

			if (xRight <= xLeft)
			{
				xLeft = (std::min)(rcTask.left, rcAttribs.left);
				xRight = (std::max)(rcTask.right, rcAttribs.right);
			}

			ptBar = CPoint(((xLeft + xRight) / 2), y);
			return TRUE;
		}

		return FALSE;
	}

	// ---------------------------------------------------------------

	BOOL RunDrag(HWND hContainer, HWND hTaskCtrl, CPoint ptBar, BOOL bVertical, int nDelta,
				 const int nSteps, const int nStepDelayMs, const int nPixelTolerance,
				 const CString& sArtifactFolder, HANDLE hProcess, DRAGSTATS& stats)
	{
		CPoint ptEnd = ptBar;

		if (bVertical)
			ptEnd.x += nDelta;
		else
			ptEnd.y += nDelta;

		stats.dwGdiStart = (hProcess ? ::GetGuiResources(hProcess, GR_GDIOBJECTS) : 0);
		stats.nSteps = nSteps;

		DWORD dwStart = ::GetTickCount();

		SendMouseAbsolute(MOUSEEVENTF_MOVE, ptBar);
		::Sleep(120);
		SendMouseAbsolute(MOUSEEVENTF_MOVE | MOUSEEVENTF_LEFTDOWN, ptBar);
		::Sleep(60);

		for (int i = 1; i <= nSteps; i++)
		{
			CPoint pt = ptBar;

			if (bVertical)
				pt.x = (ptBar.x + MulDiv(nDelta * i, 1, nSteps));
			else
				pt.y = (ptBar.y + MulDiv(nDelta * i, 1, nSteps));

			SendMouseAbsolute(MOUSEEVENTF_MOVE, pt);
			::Sleep(nStepDelayMs);

			CAPTURE cOnScreen, cClean;

			if (!CaptureClient(hContainer, cOnScreen))
				break;

			ForceRepaint(hContainer);
			::Sleep(20);

			if (!CaptureClient(hContainer, cClean))
				break;

			FRAMEDIFF diff = CompareCaptures(cOnScreen, cClean, 6);

			// Verify the clean reference is stable: a second forced repaint must
			// produce the same pixels, otherwise the oracle itself is unreliable.
			CAPTURE cClean2;

			ForceRepaint(hContainer);
			::Sleep(20);

			if (CaptureClient(hContainer, cClean2))
			{
				FRAMEDIFF cleanDiff = CompareCaptures(cClean, cClean2, 6);
				stats.nMaxCleanDiff = (std::max)(stats.nMaxCleanDiff, cleanDiff.nPixels);
			}

			if (diff.nPixels > nPixelTolerance)
			{
				stats.nArtifactSteps++;

				CString sBase;
				sBase.Format(_T("%s\\drag_%02d"), (LPCTSTR)sArtifactFolder, i);

				SaveCaptureBmp(cOnScreen, sBase + _T("_onscreen.bmp"));
				SaveCaptureBmp(cClean, sBase + _T("_clean.bmp"));
			}

			stats.nMaxDiffPixels = (std::max)(stats.nMaxDiffPixels, diff.nPixels);
		}

		SendMouseAbsolute(MOUSEEVENTF_MOVE | MOUSEEVENTF_LEFTUP, ptEnd);
		::Sleep(200);

		// Final artefact check after the drop
		{
			CAPTURE cOnScreen, cClean;

			if (CaptureClient(hContainer, cOnScreen))
			{
				ForceRepaint(hContainer);
				::Sleep(20);

				if (CaptureClient(hContainer, cClean))
				{
					FRAMEDIFF diff = CompareCaptures(cOnScreen, cClean, 6);

					if (diff.nPixels > nPixelTolerance)
					{
						stats.nArtifactSteps++;

						CString sBase;
						sBase.Format(_T("%s\\drop"), (LPCTSTR)sArtifactFolder);

						SaveCaptureBmp(cOnScreen, sBase + _T("_onscreen.bmp"));
						SaveCaptureBmp(cClean, sBase + _T("_clean.bmp"));
					}

					stats.nMaxDiffPixels = (std::max)(stats.nMaxDiffPixels, diff.nPixels);
				}
			}
		}

		stats.dwElapsedMs = (::GetTickCount() - dwStart);
		stats.dwGdiEnd = (hProcess ? ::GetGuiResources(hProcess, GR_GDIOBJECTS) : 0);

		return TRUE;
	}

	// ---------------------------------------------------------------

	CString MakeRunFolder(LPCTSTR szScenario)
	{
		CString sFolder = FileMisc::PathConcat(FileMisc::GetTempFolder(), _T("TDLPerf"));

		SYSTEMTIME st;
		::GetLocalTime(&st);

		CString sSub;
		sSub.Format(_T("%s_%04d%02d%02d_%02d%02d%02d"),
					szScenario, st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

		sFolder = FileMisc::PathConcat((LPCTSTR)sFolder, sSub);
		FileMisc::CreateFolder(sFolder);

		return sFolder;
	}

	CString FindDefaultExePath()
	{
			const CString sModuleFolder = FileMisc::GetModuleFolder();

		LPCTSTR szCandidates[] =
		{
			_T("..\\..\\ToDoList\\Unicode_Debug\\ToDoList.exe"),
			_T("..\\..\\ToDoList\\Unicode_Release\\ToDoList.exe"),
			_T("..\\..\\Plugins\\debug\\ToDoList.exe"),
			_T("..\\..\\Plugins\\release\\ToDoList.exe"),
			_T("..\\ToDoList.exe"),
			_T("ToDoList.exe"),
		};

		for (int i = 0; i < _countof(szCandidates); i++)
		{
			CString sPath = FileMisc::GetFullPath(FileMisc::PathConcat(sModuleFolder, szCandidates[i]));

			if (FileMisc::FileExists(sPath))
				return sPath;
		}

		return _T("");
	}

	BOOL LaunchApp(CAppRunner& app, const CString& sExePath, LPCTSTR szTasklistPath, LPCTSTR szIniPath)
	{
		::SetEnvironmentVariable(_T("TDL_ASSERT_LOG"), _T("1"));

		CString sArgs;
		sArgs.Format(_T("-f \"%s\" -i \"%s\" -q"), szTasklistPath, szIniPath);

		return app.Launch(sExePath, sArgs);
	}

	void ForceForegroundWindow(HWND hWnd)
	{
		if (!hWnd)
			return;

		HWND hFore = ::GetForegroundWindow();
		DWORD dwForeThread = (hFore ? ::GetWindowThreadProcessId(hFore, NULL) : 0);
		DWORD dwThisThread = ::GetCurrentThreadId();

		BOOL bAttached = FALSE;

		if (dwForeThread && (dwForeThread != dwThisThread))
			bAttached = ::AttachThreadInput(dwThisThread, dwForeThread, TRUE);

		::BringWindowToTop(hWnd);
		::SetForegroundWindow(hWnd);
		::SetActiveWindow(hWnd);

		if (bAttached)
			::AttachThreadInput(dwThisThread, dwForeThread, FALSE);
	}

	BOOL SetDefaultWindowSize(HWND hMain)
	{
		if (!hMain)
			return FALSE;

		::ShowWindow(hMain, SW_RESTORE);
		::SetWindowPos(hMain, HWND_TOP, 80, 80, 1400, 900, SWP_SHOWWINDOW);
		ForceForegroundWindow(hMain);
		::Sleep(500);

		return (::GetForegroundWindow() == hMain);
	}
}

//////////////////////////////////////////////////////////////////////
// PERFOPTIONS
//////////////////////////////////////////////////////////////////////

CUIPerfTest::PERFOPTIONS::PERFOPTIONS()
	:
	nSteps(DEFAULT_STEPS),
	nStepDelayMs(DEFAULT_STEP_DELAY_MS),
	nPixelTolerance(DEFAULT_PIXEL_TOLERANCE)
{
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CUIPerfTest::CUIPerfTest(const CTestUtils& utils)
	:
	CTDLTestBase(_T("CUIPerfTest"), utils)
{
}

CUIPerfTest::~CUIPerfTest()
{
}

TESTRESULT CUIPerfTest::Run()
{
	ClearTotals();

	// Off by default: this harness launches the GUI app, moves the real mouse
	// and needs an interactive desktop.
	if (!m_utils.HasCommandlineFlag(_T("ui")))
		return GetTotals();

	PERFOPTIONS opt;

	if (!GetOptions(opt))
	{
		_tprintf(_T("\n  CUIPerfTest: ToDoList.exe not found. Use -exe <path>.\n"));
		return GetTotals();
	}

	_tprintf(_T("\n  CUIPerfTest: exe=%s\n"), (LPCTSTR)opt.sExePath);

	// Fixture-only mode: no GUI launch, useful for diagnosing setup problems
	if (m_utils.HasCommandlineFlag(_T("uifix")))
	{
		TestFixtures();
		return GetTotals();
	}

	TestSplitterDrag(opt);
	TestListScrollSync(opt);

	return GetTotals();
}

BOOL CUIPerfTest::GetOptions(PERFOPTIONS& opt) const
{
	opt.sExePath = m_utils.GetCommandlineOption(_T("exe"));

	if (opt.sExePath.IsEmpty())
		opt.sExePath = FindDefaultExePath();
	else
		opt.sExePath = FileMisc::GetFullPath(opt.sExePath);

	opt.sTasklistPath = m_utils.GetCommandlineOption(_T("tdl"));

	if (!opt.sTasklistPath.IsEmpty())
		opt.sTasklistPath = FileMisc::GetFullPath(opt.sTasklistPath);

	int nValue = _ttoi(m_utils.GetCommandlineOption(_T("steps")));

	if (nValue > 0)
		opt.nSteps = nValue;

	CString sValue = m_utils.GetCommandlineOption(_T("delay"));

	if (!sValue.IsEmpty())
		opt.nStepDelayMs = (std::max)(0, _ttoi(sValue));

	sValue = m_utils.GetCommandlineOption(_T("tol"));

	if (!sValue.IsEmpty())
		opt.nPixelTolerance = (std::max)(0, _ttoi(sValue));

	return (!opt.sExePath.IsEmpty() && FileMisc::FileExists(opt.sExePath));
}

//////////////////////////////////////////////////////////////////////
// Fixtures
//////////////////////////////////////////////////////////////////////

namespace
{
	void AddGeneratedTask(CTaskFile& tasks, HTASKITEM hTask, int nIndex)
	{
		tasks.AddTaskAllocatedTo(hTask, _T("Perf Person"));
		tasks.AddTaskCategory(hTask, _T("Perf Category"));
		tasks.AddTaskTag(hTask, _T("PerfTag"));
		tasks.SetTaskStatus(hTask, ((nIndex % 3) ? _T("In Progress") : _T("Not Started")));
		tasks.SetTaskPriority(hTask, ((nIndex % 10) + 1));
	}

	void GenerateTreeTasks(CTaskFile& tasks, HTASKITEM hParent, DWORD dwParentID,
						   DWORD& dwNextID, int nDepth, int nChildren)
	{
		for (int i = 0; i < nChildren; i++)
		{
			DWORD dwID = dwNextID++;
			CString sTitle;
			sTitle.Format(_T("Task %u (depth %d)"), dwID, nDepth);

			HTASKITEM hTask = tasks.NewTask(sTitle, hParent, dwID, dwParentID, FALSE);
			AddGeneratedTask(tasks, hTask, (int)dwID);

			if (nDepth > 1)
				GenerateTreeTasks(tasks, hTask, dwID, dwNextID, (nDepth - 1), nChildren);
		}
	}
}

BOOL CUIPerfTest::GenerateTreeFixture(LPCTSTR szFolderPath, CString& sTasklistPath) const
{
	sTasklistPath = FileMisc::PathConcat(szFolderPath, _T("PerfTree.tdl"));

	CTaskFile tasks;
	tasks.SetProjectName(_T("TDL Perf Tree"));

	DWORD dwNextID = 1;
	GenerateTreeTasks(tasks, NULL, 0, dwNextID, 4, 10);

	tasks.SetNextUniqueID(dwNextID);

	BOOL bSaved = tasks.Save(sTasklistPath, SFEF_UTF16);

	if (!bSaved)
		_tprintf(_T("  tree fixture save failed: %s (err=%lu)\n"), (LPCTSTR)sTasklistPath, ::GetLastError());

	return bSaved;
}

BOOL CUIPerfTest::GenerateListFixture(LPCTSTR szFolderPath, CString& sTasklistPath) const
{
	sTasklistPath = FileMisc::PathConcat(szFolderPath, _T("PerfList.tdl"));

	CTaskFile tasks;
	tasks.SetProjectName(_T("TDL Perf List"));

	const int nNumTasks = 3000;

	for (int i = 0; i < nNumTasks; i++)
	{
		CString sTitle;
		sTitle.Format(_T("List task %d"), i);

		HTASKITEM hTask = tasks.NewTask(sTitle, NULL, (DWORD)(i + 1), 0, FALSE);
		AddGeneratedTask(tasks, hTask, i);
	}

	tasks.SetNextUniqueID((DWORD)(nNumTasks + 1));

	BOOL bSaved = tasks.Save(sTasklistPath, SFEF_UTF16);

	if (!bSaved)
		_tprintf(_T("  list fixture save failed: %s (err=%lu)\n"), (LPCTSTR)sTasklistPath, ::GetLastError());

	return bSaved;
}

BOOL CUIPerfTest::CopyTasklistFixture(LPCTSTR szSrcPath, LPCTSTR szFolderPath, CString& sTasklistPath) const
{
	sTasklistPath = FileMisc::PathConcat(szFolderPath, _T("PerfUser.tdl"));

	return ::CopyFile(szSrcPath, sTasklistPath, FALSE);
}

BOOL CUIPerfTest::WriteIniFile(LPCTSTR szIniPath) const
{
	CStdioFile file;

	if (!file.Open(szIniPath, CFile::modeCreate | CFile::modeWrite | CFile::typeText))
		return FALSE;

	file.WriteString(_T("[Preferences]\n"));
	file.WriteString(_T("MultiInstance=1\n"));
	file.WriteString(_T("AutoCheckForUpdates=0\n"));
	file.WriteString(_T("EnableSourceControl=0\n"));
	file.WriteString(_T("CheckoutOnCheckin=0\n"));
	file.WriteString(_T("CheckinOnClose=0\n"));
	file.Close();

	return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Splitter drag
//////////////////////////////////////////////////////////////////////

void CUIPerfTest::TestSplitterDrag(const PERFOPTIONS& opt)
{
	CTDCScopedTest test(*this, _T("UIPerf::SplitterDrag"));

	const CString sRunFolder = MakeRunFolder(_T("SplitterDrag"));
	CString sTdlPath, sIniPath = FileMisc::PathConcat(sRunFolder, _T("Perf.ini"));

	BOOL bHasFixture = opt.sTasklistPath.IsEmpty()
						? GenerateTreeFixture(sRunFolder, sTdlPath)
						: CopyTasklistFixture(opt.sTasklistPath, sRunFolder, sTdlPath);

	ExpectTrue(bHasFixture);

	if (!bHasFixture)
		return;

	ExpectTrue(WriteIniFile(sIniPath));

	CAppRunner app;

	if (!ExpectTrue(LaunchApp(app, opt.sExePath, sTdlPath, sIniPath)))
		return;

	HWND hMain = NULL;

	if (!ExpectTrue(app.WaitForMainWnd(hMain, 20000)))
		return;

	if (!ExpectTrue(SetDefaultWindowSize(hMain)))
	{
		_tprintf(_T("  SplitterDrag: could not bring ToDoList to the foreground\n"));
		return;
	}

	HWND hContainer = NULL, hTaskCtrl = NULL;
	CPoint ptBar(0, 0);
	BOOL bVertical = FALSE;

	for (int i = 0; i < 100; i++)
	{
		hContainer = FindTaskContainer(hMain);

		if (hContainer && GetSplitBarPoint(hContainer, hTaskCtrl, ptBar, bVertical))
			break;

		::Sleep(100);
	}

	BOOL bFoundBar = (hContainer && hTaskCtrl && (ptBar.x != 0 || ptBar.y != 0));

	ExpectTrue(bFoundBar);

	if (!bFoundBar)
	{
		_tprintf(_T("  SplitterDrag: could not locate task view/attribute split bar\n"));
		return;
	}

	CRect rcTaskBefore;
	::GetWindowRect(hTaskCtrl, &rcTaskBefore);

	if (m_utils.HasCommandlineFlag(_T("nodrag")))
	{
		_tprintf(_T("  SplitterDrag: bar=(%d,%d) vertical=%d task=(%d,%d,%d,%d)\n"),
				 ptBar.x, ptBar.y, bVertical,
				 rcTaskBefore.left, rcTaskBefore.top, rcTaskBefore.right, rcTaskBefore.bottom);
		return;
	}

	// Drag right/down, then back
	for (int nDir = 0; nDir < 2; nDir++)
	{
		CRect rcTaskBefore;
		::GetWindowRect(hTaskCtrl, &rcTaskBefore);

		GetSplitBarPoint(hContainer, hTaskCtrl, ptBar, bVertical);

		int nDelta = ((nDir == 0) ? 120 : -120);

		DRAGSTATS stats;
		BOOL bDragged = RunDrag(hContainer, hTaskCtrl, ptBar, bVertical, nDelta,
								opt.nSteps, opt.nStepDelayMs, opt.nPixelTolerance,
								sRunFolder, app.GetProcessHandle(), stats);

		ExpectTrue(bDragged);

		CRect rcTaskAfter;
		::GetWindowRect(hTaskCtrl, &rcTaskAfter);

		BOOL bGeometryChanged = (bVertical ? (rcTaskAfter.Width() != rcTaskBefore.Width())
										   : (rcTaskAfter.Height() != rcTaskBefore.Height()));

		_tprintf(_T("  SplitterDrag[%d]: steps=%d maxDiff=%d cleanDiff=%d artifactSteps=%d elapsedMs=%d gdi=%u->%u moved=%d\n"),
				 nDir, stats.nSteps, stats.nMaxDiffPixels, stats.nMaxCleanDiff, stats.nArtifactSteps,
				 stats.dwElapsedMs, stats.dwGdiStart, stats.dwGdiEnd, bGeometryChanged);

		ExpectTrue(bGeometryChanged);
		ExpectTrue(stats.nArtifactSteps == 0);
	}

	_tprintf(_T("  SplitterDrag: artefacts saved under %s\n"), (LPCTSTR)sRunFolder);
}

//////////////////////////////////////////////////////////////////////
// Fixture-only diagnostics
//////////////////////////////////////////////////////////////////////

void CUIPerfTest::TestFixtures()
{
	CTDCScopedTest test(*this, _T("UIPerf::Fixtures"));

	const CString sRunFolder = MakeRunFolder(_T("Fixtures"));
	CString sTreePath, sListPath;

	BOOL bTree = GenerateTreeFixture(sRunFolder, sTreePath);
	BOOL bList = GenerateListFixture(sRunFolder, sListPath);

	_tprintf(_T("  Fixtures folder: %s\n"), (LPCTSTR)sRunFolder);
	_tprintf(_T("  Tree: %s (%s)\n"), (LPCTSTR)sTreePath, (bTree ? _T("OK") : _T("FAILED")));
	_tprintf(_T("  List: %s (%s)\n"), (LPCTSTR)sListPath, (bList ? _T("OK") : _T("FAILED")));

	ExpectTrue(bTree);
	ExpectTrue(bList);
	ExpectTrue(FileMisc::FileExists(sTreePath));
	ExpectTrue(FileMisc::FileExists(sListPath));
}

//////////////////////////////////////////////////////////////////////
// List scroll sync
//////////////////////////////////////////////////////////////////////

void CUIPerfTest::TestListScrollSync(const PERFOPTIONS& opt)
{
	CTDCScopedTest test(*this, _T("UIPerf::ListScrollSync"));

	const CString sRunFolder = MakeRunFolder(_T("ListScroll"));
	CString sTdlPath, sIniPath = FileMisc::PathConcat(sRunFolder, _T("Perf.ini"));

	BOOL bHasFixture = opt.sTasklistPath.IsEmpty()
						? GenerateListFixture(sRunFolder, sTdlPath)
						: CopyTasklistFixture(opt.sTasklistPath, sRunFolder, sTdlPath);

	ExpectTrue(bHasFixture);

	if (!bHasFixture)
		return;

	ExpectTrue(WriteIniFile(sIniPath));

	CAppRunner app;

	if (!ExpectTrue(LaunchApp(app, opt.sExePath, sTdlPath, sIniPath)))
		return;

	HWND hMain = NULL;

	if (!ExpectTrue(app.WaitForMainWnd(hMain, 20000)))
		return;

	if (!ExpectTrue(SetDefaultWindowSize(hMain)))
	{
		_tprintf(_T("  ListScroll: could not bring ToDoList to the foreground\n"));
		return;
	}

	HWND hContainer = NULL;

	for (int i = 0; i < 100; i++)
	{
		hContainer = FindTaskContainer(hMain);

		if (hContainer)
			break;

		::Sleep(100);
	}

	ExpectTrue(hContainer != NULL);

	if (!hContainer)
		return;

	// Switch to List View if necessary
	for (int nTry = 0; nTry < 3; nTry++)
	{
		HWND hListHost = FindDescendantById(hContainer, TDL_IDC_TASKLISTCTRL);
		HWND hTreeHost = FindDescendantById(hContainer, TDL_IDC_TASKTREECTRL);

		_tprintf(_T("  ListScroll: try=%d listHost=%p visible=%d treeHost=%p visible=%d\n"),
				 nTry, hListHost, (hListHost && ::IsWindowVisible(hListHost)),
				 hTreeHost, (hTreeHost && ::IsWindowVisible(hTreeHost)));

		if (hListHost && ::IsWindowVisible(hListHost))
			break;

		// Try the direct activate command first, then the toggle
		UINT nCmd = ((nTry == 0) ? TDL_CMD_ACTIVATELISTVIEW : TDL_CMD_TOGGLETREEANDLIST);
		::SendMessage(hMain, WM_COMMAND, nCmd, 0);
		::Sleep(750);
	}

	HWND hListHost = FindDescendantById(hContainer, TDL_IDC_TASKLISTCTRL);

	ExpectTrue(hListHost && ::IsWindowVisible(hListHost));

	if (!hListHost || !::IsWindowVisible(hListHost))
		return;

	vector<HWND> aLists;
	CollectDescendantsByClass(hListHost, _T("SysListView32"), aLists, 4);

	ExpectTrue(aLists.size() == 2);

	if (aLists.size() != 2)
	{
		_tprintf(_T("  ListScroll: found %u list windows\n"), (unsigned)aLists.size());
		return;
	}

	if (m_utils.HasCommandlineFlag(_T("nodrag")))
	{
		_tprintf(_T("  ListScroll: found %u list windows\n"), (unsigned)aLists.size());
		return;
	}

	// Use the wider list (the columns list) as the scroll target
	HWND hList1 = aLists[0];
	HWND hList2 = aLists[1];

	CRect rc1, rc2;
	::GetWindowRect(hList1, &rc1);
	::GetWindowRect(hList2, &rc2);

	HWND hScrollList = ((rc1.Width() >= rc2.Width()) ? hList1 : hList2);
	CRect rcScroll;
	::GetWindowRect(hScrollList, &rcScroll);
	CPoint ptCenter = rcScroll.CenterPoint();

	SCROLLSTATS stats;

	const int nScrollSteps = (std::max)(5, opt.nSteps / 2);

	for (int i = 0; i < nScrollSteps; i++)
	{
		SendMouseWheel(ptCenter, -WHEEL_DELTA);
		::Sleep(120);

		stats.nSteps++;

		int nTop1 = (int)::SendMessage(hList1, LVM_GETTOPINDEX, 0, 0);
		int nTop2 = (int)::SendMessage(hList2, LVM_GETTOPINDEX, 0, 0);

		if (i == 0)
			stats.nFirstTopIndex = nTop1;

		stats.nLastTopIndex = nTop1;

		if (abs(nTop1 - nTop2) > 1)
			stats.nSyncErrors++;

		CAPTURE cOnScreen, cClean;

		if (!CaptureClient(hContainer, cOnScreen))
			break;

		ForceRepaint(hContainer);
		::Sleep(20);

		if (!CaptureClient(hContainer, cClean))
			break;

		FRAMEDIFF diff = CompareCaptures(cOnScreen, cClean, 6);

		if (diff.nPixels > opt.nPixelTolerance)
		{
			stats.nArtifactSteps++;

			CString sBase;
			sBase.Format(_T("%s\\scroll_%02d"), (LPCTSTR)sRunFolder, i);

			SaveCaptureBmp(cOnScreen, sBase + _T("_onscreen.bmp"));
			SaveCaptureBmp(cClean, sBase + _T("_clean.bmp"));
		}

		stats.nMaxDiffPixels = (std::max)(stats.nMaxDiffPixels, diff.nPixels);
	}

	_tprintf(_T("  ListScroll: steps=%d top=%d->%d syncErrors=%d maxDiff=%d artifactSteps=%d\n"),
			 stats.nSteps, stats.nFirstTopIndex, stats.nLastTopIndex,
			 stats.nSyncErrors, stats.nMaxDiffPixels, stats.nArtifactSteps);

	ExpectTrue(stats.nLastTopIndex > stats.nFirstTopIndex); // actually scrolled
	ExpectTrue(stats.nSyncErrors == 0);
	ExpectTrue(stats.nArtifactSteps == 0);

	_tprintf(_T("  ListScroll: artefacts saved under %s\n"), (LPCTSTR)sRunFolder);
}
