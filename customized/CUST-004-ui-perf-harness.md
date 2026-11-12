# CUST-004 — Automated UI performance/artefact harness

| Field | Value |
|---|---|
| Status | In progress (implemented and built; interactive baseline run pending) |
| Upstream baseline | ToDoList 9.3.DP7 |
| Scope | `Core/TDLTest` only; production code unchanged |
| Default | Off unless `-ui` is passed; normal builds/tests are unaffected |
| Related | [index.md](index.md), [../PERFORMANCE.md](../PERFORMANCE.md) §7.4 |

## Goal

Replace blind build-and-eyeball testing of splitter/scroll performance with a
repeatable harness that:

1. Launches `ToDoList.exe` against a deterministic fixture.
2. Drives the real UI with `SendInput`.
3. Captures the visible client with `BitBlt`, forces a clean repaint and compares
   the two frames to detect stale/intermediate pixels.
4. Prints timing, GDI-object and paint-artefact metrics and saves mismatching
   frames as BMPs.

It is intended for CUST-002 (list-view stale titles) as well as the splitter work
in CUST-003.

## What was added

- `Core/TDLTest/UIPerfTest.h/.cpp` — `CUIPerfTest`.
- `Core/TDLTest/TDLTest.cpp` — registers the test.
- `Core/TDLTest/TDLTest.vcxproj(.filters)` — project entries.
- `Core/TDLTest/TDLTestBase.h` — `CTestUtils::GetCommandlineOption()` accessor.

No `Core/ToDoList` production file is changed.

## Scenarios

### `UIPerf::SplitterDrag`

- Generates a deterministic 4-level tree fixture (or uses `-tdl`).
- Launches the app with an isolated temp `.ini` (`MultiInstance=1`, no update
  checks).
- Sizes the frame to 1400x900 and brings it to the foreground.
- Locates the task view and attribute pane, derives the split-bar point next to
  the task view, and drags it +/-120 px with real mouse input.
- At every step captures on-screen pixels, forces `RedrawWindow(...UPDATENOW |
  ALLCHILDREN)` and captures again. A non-zero difference beyond tolerance is an
  artefact.
- Fails if the task view geometry did not change or any step failed the artefact
  check.

### `UIPerf::ListScrollSync`

- Generates a flat 3000-task fixture (or uses `-tdl`).
- Switches to List View via `ID_VIEW_TOGGLETREEANDLIST`.
- Finds the paired `SysListView32` controls, scrolls with the real mouse wheel and
  checks `LVM_GETTOPINDEX` stays synchronised.
- Runs the same on-screen vs forced-repaint frame comparison to catch stale titles
  (CUST-002).

## Running

```bat
:: from Core/TDLTest (normal default: skipped)
Unicode_Debug\TDLTest.exe

:: run the UI harness (runs only CUIPerfTest)
Unicode_Debug\TDLTest.exe -ui

:: to keep a copy of the metrics
Unicode_Debug\TDLTest.exe -ui > ui-perf-baseline.txt 2>&1

:: options
::   -exe <path>    ToDoList.exe (defaults to sibling build folders)
::   -tdl <path>    use an existing tasklist instead of generating one
::   -steps <n>     drag/scroll steps (default 20)
::   -delay <ms>    wait between drag steps (default 25)
::   -tol <pixels>  artefact pixel-count tolerance (default 2000)
::   -uifix         generate fixtures only (no GUI launch)
::   -nodrag        launch and locate windows but do not send mouse input
```

Requirements and behaviour:

- Needs an interactive desktop. The harness moves the real cursor and takes over
  the mouse while it runs.
- The harness brings the app to the foreground at 80,80 / 1400x900; other windows
  at that location or always-on-top windows will corrupt captures.
- Sets `TDL_ASSERT_LOG=1` and passes `-q` so debug assertions do not block it.
- Debug `TDLTest.exe` installs a CRT assert report hook at startup: assertions are
  appended to `%TEMP%\TDLTest_asserts.log` (or the path in `TDL_ASSERT_LOG`) and the
  dialog/debug break is suppressed. Set `TDL_ASSERT_LOG=0` to restore the normal
  interactive assertion dialog. This also stops unrelated pre-existing test
  assertions from blocking a harness run.
- When `-ui` is passed, only `CUIPerfTest` runs.
- `-uifix` only generates both fixtures and prints their paths (no GUI launch).
- `-nodrag` launches the app, locates the splitter/lists and prints the geometry
  but sends no mouse input. Use it to diagnose layout/window-discovery problems.

Implementation notes:

- `TDLTest` initialises COM (`CoInitializeEx`, apartment threaded) because
  `CTaskFile::Save` uses MSXML. Without it fixture generation fails with
  `CXmlFile::SaveEx` returning FALSE.
- `FileMisc::PathConcat` has an in-place overload that matches a mutable `CString`
  lvalue. Harness path variables are `const CString` so the non-mutating overload
  is selected; otherwise the run folder was mutated into the ini path.
- Missing options must not be treated as zero: `-tol` is only applied when the
  option string is non-empty (otherwise the default 2000 became 0 and every
  1-pixel difference was counted as an artefact).
- List View is activated with `ID_ACTIVATEVIEW_LISTVIEW` (34645) via
  `SendMessage(WM_COMMAND)`, which works when the toggle command does not. The
  command is retried with `ID_VIEW_TOGGLETREEANDLIST` (34238) as a fallback.

First baseline (unmodified build, `-ui -steps 3/4/8`):

- `SplitterDrag[0/1]`: `moved=1`, but every step reported stale pixels
  (`maxDiff` about 59k–78k pixels, `artifactSteps` = `steps`). `cleanDiff=0`
  confirms two consecutive forced repaints are identical, so the diff is real
  stale content, not oracle noise. The saved frames show the task view's right
  edge/Tags column still painted at the previous splitter position, plus the old
  bar strip. This is the CUST-003 artefact reproduced automatically.
- `ListScroll`: `top` advanced, `syncErrors=0`, `maxDiff=0..2`,
  `artifactSteps=0` with the generated flat list. CUST-002 may need a specific
  fixture/column mix (`-tdl`) to reproduce.
- `SplitterDrag` therefore fails until the artefact is fixed; the pass criterion is
  `artifactSteps == 0` with `moved=1` and `cleanDiff == 0`.
- Fixtures, isolated `.ini` and any mismatching BMP frames are written under
  `%TEMP%\TDLPerf\<Scenario>_<timestamp>\`.
- Print lines are machine-readable enough to compare two runs:
  - `SplitterDrag[n]: steps=... maxDiff=... artifactSteps=... elapsedMs=... gdi=a->b moved=...`
  - `ListScroll: steps=... top=a->b syncErrors=... maxDiff=... artifactSteps=...`

## Risks / limits

- Screen `BitBlt` captures what is visible; occlusion (console, RDP, notifications)
  invalidates the result.
- The forced-repaint reference can itself alter timing. It is used only as an
  oracle, not as a performance measurement.
- Wall-clock time and GDI counts are coarse; paint-count instrumentation in the
  app is still a future step if needed.
- There is no CI mode yet: the harness intentionally requires a real desktop.
- The list scenario relies on the 34238 (`ID_VIEW_TOGGLETREEANDLIST`) command ID
  and the default stacked layout; both are documented in
  `Core/ToDoList/TDCEnum.h` / `resource.h`.

## Next steps

1. Run the harness on the unmodified tree and record the baseline output and BMPs.
2. Add the same run as the acceptance check for any CUST-003 retry.
3. Optionally add `TDL_PERF_LOG` counters in the app for paint counts/durations.
