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
   frames as JPEGs (and mismatching settled frames as BMPs).

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

- Uses a deterministic 3-level tree fixture (1110 tasks, distinct titles and dark
  row colours) or a `-tdl` tasklist.
- Launches the app with an isolated temp `.ini` (`MultiInstance=1`, no update
  checks).
- Sizes the frame to 1400x900 and brings it to the foreground.
- Locates the task view and attribute pane, derives the split-bar point next to
  the task view, and drags it +/-120 px with real mouse input.
- At every step captures on-screen pixels, forces `RedrawWindow(...UPDATENOW |
  ALLCHILDREN)` and captures again. A non-zero difference beyond tolerance is an
  artefact. A second clean capture (`cleanDiff`) verifies the oracle is stable.
- Fails if the task view geometry did not change or any step failed the artefact
  check.

### `UIPerf::ListScrollSync`

- Uses a flat 3000-task fixture (or `-tdl`).
- Switches to List View via `ID_ACTIVATEVIEW_LISTVIEW`.
- Finds the paired `SysListView32` controls and scrolls with real input: mouse
  wheel by default, or Page Down (`-keys`) after clicking the list to give it
  focus. Page Down exposes the CUST-002 artefacts more reliably.
- `-size <WxH>` sets a taller/wider or smaller frame so more rows are visible and
  the stale region is more evident. It is clamped to the desktop work area.
- Two checks:
  - **Settled pixel check:** after the scroll has settled, on-screen pixels are
    compared with a forced repaint (`artifactSteps`, `maxDiff`). This catches stale
    task rows *and* stale attribute-pane fields next to the tree.
  - **Fast-scroll sync check (`-record`):** scroll input is sent rapidly and
    `LVM_GETTOPINDEX` of the title list and column list are compared immediately.
    A mismatch means the title column is lagging the attribute columns, which is
    CUST-002. A worker thread captures frames and encodes them to JPEG **in
    memory** (GDI+ worker thread, COM initialised) without disk I/O; the JPEGs are
    written as `frame_NNNNN_<tick>.jpg` after the run/thread stops.
- `-capture task` crops recordings to the union of the two list views, so filter
  panels, the attribute pane and comments are excluded and frames are small.
  `-capture taskattr` also includes the attribute pane; `full` is the container.
- `-maximize` sends `ID_VIEW_MAXTASKLIST` (32836) before recording. It hides the
  other panes, but in the current implementation synthetic Page Down did not scroll
  afterwards; prefer `-capture task` instead.
- `-recms 50` gives roughly 20 fps, `-recms 100` roughly 10 fps; `-maxframes` caps
  memory/disk use. JPEG quality is 80.
- The number of scroll steps is `max(1, -steps / 2)`, so `-steps 6` performs three
  Page Downs. `-delay` is applied after each step; `150` gives the recorder several
  frames per key press.
- Manual recording: `-uiscroll -uihold -record` leaves ToDoList open and records
  while you press Page Up/Page Down yourself. Frames are written when you close the
  app.

## How the artefact oracle works

- `CaptureClient()` does a screen `BitBlt` of the `CToDoCtrl` client. This is what
  the user currently sees, including stale pixels.
- `ForceRepaint()` calls `RedrawWindow(hContainer, NULL, NULL,
  RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN | RDW_FRAME)` and captures
  again. This is the "correct" reference at the same geometry.
- `CompareCaptures()` counts pixels whose RGB channels differ by more than 6.
  `-tol` is the allowed count. A failing step saves `_onscreen.bmp` and `_clean.bmp`.
- For scrolling, the comparison is only valid once the wheel input has settled.
  The harness therefore waits at least 100 ms before the on-screen capture (unless
  `-record` is used), otherwise it can compare two different scroll offsets.
- Use `-frames` to save every on-screen capture during the settled check, and
  `-record` for a fast-scroll frame dump + top-index mismatch check. This replaces
  a manual 30 fps screen recording for reproducing the transient title lag.

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
::   -tdl <path>    use an existing tasklist instead of the generated fixture
::   -steps <n>     drag/scroll steps (default 20)
::   -delay <ms>    wait between steps (default 25; scroll settles for >=100 ms)
::   -tol <pixels>  artefact pixel-count tolerance (default 2000)
::   -notches <n>   wheel notches / Page Down presses per scroll step (default 1)
::   -keys          use Page Down keys instead of the mouse wheel
::   -size <WxH>    tasklist frame size, clamped to the work area (default 1400x900)
::   -maximize      send ID_VIEW_MAXTASKLIST to hide comments/attributes/filter panes
::   -capture <m>   full | task | taskattr (default full; task = the two list views)
::   -record        record JPEG frames in memory + fast top-index sync check
::   -recms <ms>    delay between recorded frames (default 10; 100 ≈ 10 fps)
::   -maxframes <n> cap recorded frames (default 400)
::   -frames        save every on-screen frame during the settled pixel check
::   -regenerate    ignore the cached generated fixtures
::   -uifix         generate fixtures only (no GUI launch) and print manual launch commands
::   -uiscroll      run only UIPerf::ListScrollSync
::   -uisplitter    run only UIPerf::SplitterDrag
::   -uihold        leave ToDoList open after setup; wait for you to close it (manual mode)
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
- When `-ui` is passed, only `CUIPerfTest` runs. `-uiscroll`/`-uisplitter`
  restrict it to one scenario; default is both.
- `-uifix` only generates both fixtures, writes `Perf.ini` and prints ready-to-run
  manual launch commands (no GUI launch).
- `-uihold` launches the app, performs the scenario setup (e.g. activates List View)
  and then waits for the app to be closed, so you can test manually. No automated
  input is sent in this mode. If `-tdl` is omitted the generated fixture is used.
  Waiting stops (and the app is terminated) after 30 minutes.
- `-nodrag` launches the app, locates the splitter/lists and prints the geometry
  but sends no mouse input, then closes the app. Use it to diagnose
  layout/window-discovery problems.

Implementation notes:

- Generated fixtures are cached under `%TEMP%\TDLPerf\Cache\` as `PerfTree_v3.tdl`
  and `PerfList_v3.tdl`. They are generated once, then copied into each run folder;
  `-regenerate` forces regeneration. This removes fixture construction from normal
  startup (the tree was also reduced from 11k to 1,110 tasks).
- Prefer real samples from the sibling `ToDoList_Test` repository where available
  (for example `..\..\..\..\ToDoList_Test\Tasklists\Commercial Construction.tdl`
  when running from `Core\ToDoList`). `-tdl` uses them directly and avoids the
  generated-fixture cache entirely. Note that the sample path is relative to the
  working directory: from `Core\ToDoList\Unicode_Debug` it is four levels up.
- Fixture titles are distinct (`"Spring MVC #12"`) and rows use dark task colours so
  a title lag/stale row is visible and changes pixels. Light colours made the text
  unreadable because ToDoList uses the task colour as text colour when it does not
  fill the row.

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

First baseline (unmodified build):

- `SplitterDrag[0/1]` (`-ui -steps 3/4/8`): `moved=1`, but every step reported stale
  pixels (`maxDiff` about 59k–78k pixels, `artifactSteps` = `steps`). `cleanDiff=0`
  confirms two consecutive forced repaints are identical, so the diff is real
  stale content, not oracle noise. The saved frames show the task view's right
  edge/Tags column still painted at the previous splitter position, plus the old
  bar strip. This is the CUST-003 artefact reproduced automatically.
- `ListScroll` settled check: `top` advanced, `syncErrors=0`, `maxDiff≈529`,
  `artifactSteps=0`. The title list catches up when the scroll settles.
- `ListScroll` fast check (`-record -notches 2 -delay 0`): `ListScroll[0]:
  top1=0 top2=5 MISMATCH`, `syncErrors=1`. the first recorded JPEG shows titles
  `List task 5...` against IDs `11...`. This is CUST-002 reproduced automatically
  with the generated fixture.
- Real sample (`ToDoList_Test\Tasklists\Commercial Construction.tdl`, `-keys
  -size 1200x1200 -record`):
  `ListScroll[2]: top1=21 top2=42 MISMATCH`, `syncErrors=1`.
- 20 fps short run (`-capture task -recms 50 -steps 6 -delay 150`): 12 JPEG frames
  at 745x400. Frame 4 shows the title column stale (`Commercial Construction`,
  `Three-story Office Building`, ...) against IDs 19/20/21 and the scrolled
  attribute columns; frame 5 shows the titles caught up. The synchronous top-index
  query reported `syncErrors=0` because it ran after the catch-up, so frame
  recording is required to see this transient.
- Same sample, settled Page Down: `maxDiff=1,166,022`, `artifactSteps=5/10`.
  The on-screen frame shows the attribute pane fields (`Allocated To`,
  `Dependency`, `Due Date`, ...) still blank, while the forced repaint shows them
  populated. The pane next to the tree is affected too.
- `SplitterDrag` fails until the artefact is fixed; pass is `artifactSteps == 0`
  with `moved=1` and `cleanDiff == 0`. CUST-002 pass is `syncErrors == 0` in
  fast-scroll mode and `artifactSteps == 0` in the settled key-scroll run.
- Fixtures, isolated `.ini` and captured frames are written under
  `%TEMP%\TDLPerf\<Scenario>_<timestamp>\`.
- Print lines are machine-readable enough to compare two runs:
  - `SplitterDrag[n]: steps=... maxDiff=... cleanDiff=... artifactSteps=... elapsedMs=... gdi=a->b moved=...`
  - `ListScroll: steps=... top=a->b syncErrors=... maxDiff=... artifactSteps=...`
  - `ListScroll[i]: top1=... top2=... [MISMATCH]` (fast mode)

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

1. Run the harness on the unmodified tree and record the baseline output and frames.
2. Add the same run as the acceptance check for any CUST-003 retry.
3. Optionally add `TDL_PERF_LOG` counters in the app for paint counts/durations.

