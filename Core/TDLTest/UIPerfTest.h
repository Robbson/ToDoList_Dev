// UIPerfTest.h: UI performance and artefact harness (opt-in via -ui)
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_UIPERFTEST_H__2E9B8D2A_2C43_4A4E_9A6C_3B6B6E1F6C31__INCLUDED_)
#define AFX_UIPERFTEST_H__2E9B8D2A_2C43_4A4E_9A6C_3B6B6E1F6C31__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "TDLTestBase.h"

//////////////////////////////////////////////////////////////////////

class CUIPerfTest : public CTDLTestBase
{
public:
	CUIPerfTest(const CTestUtils& utils);
	virtual ~CUIPerfTest();

	virtual TESTRESULT Run();

protected:
	struct PERFOPTIONS
	{
		PERFOPTIONS();

		CString sExePath;
		CString sTasklistPath;	// optional user fixture
		int nSteps;
		int nStepDelayMs;
		int nPixelTolerance;
		int nNotches;			// wheel notches per scroll step
		int nMaxFrames;			// cap on recorded frames
		int nRecordIntervalMs;	// delay between recorded frames
		int nWidth, nHeight;	// tasklist frame client-ish size
		CString sCapture;		// full | task | taskattr
		BOOL bMaximize;			// maximize the tasklist before recording
		BOOL bScrollKeys;		// use Page Down instead of the mouse wheel
		BOOL bSaveFrames;		// save every on-screen frame for manual inspection
		BOOL bRecord;			// fast scroll + save frames, no clean-reference comparison
		BOOL bRegenerate;		// ignore the fixture cache
	};

protected:
	BOOL GetOptions(PERFOPTIONS& opt) const;

	BOOL GenerateTreeFixture(LPCTSTR szFolderPath, CString& sTasklistPath) const;
	BOOL GenerateListFixture(LPCTSTR szFolderPath, CString& sTasklistPath) const;
	BOOL CopyTasklistFixture(LPCTSTR szSrcPath, LPCTSTR szFolderPath, CString& sTasklistPath) const;
	BOOL EnsureCachedFixture(BOOL bTree, BOOL bRegenerate, CString& sCachePath) const;
	BOOL PrepareFixture(BOOL bTree, const PERFOPTIONS& opt, LPCTSTR szRunFolder, CString& sTasklistPath) const;
	BOOL WriteIniFile(LPCTSTR szIniPath) const;

	void TestSplitterDrag(const PERFOPTIONS& opt);
	void TestListScrollSync(const PERFOPTIONS& opt);
	void TestFixtures(const PERFOPTIONS& opt);
};

#endif // !defined(AFX_UIPERFTEST_H__2E9B8D2A_2C43_4A4E_9A6C_3B6B6E1F6C31__INCLUDED_)
