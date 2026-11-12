# CUST-002 — Stale task title in List View while scrolling

| Field | Value |
|---|---|
| Status | Deferred (investigation only; attempted fixes reverted) |
| Upstream baseline | ToDoList 9.3.DP7 |
| Scope | List View title column custom drawing |
| Default | No behaviour change; no code retained |
| Related | [index.md](index.md), [../PERFORMANCE.md](../PERFORMANCE.md) |

## Problem

In "List View" the task title column can show the same title for many consecutive
rows after scrolling, while the attribute columns (Last Mod., Completed, Status, ...)
continue to show the correct values. The screenshot `task-list-scrolling-issue.png`
shows one task titled `Interessantes` repeated down the title column while the
attribute columns correctly show the following tasks (`Spring MVC`, `Spring Data`,
`Spring & Angular`, ...).

List View is not one control. It consists of two list views:

- `m_lcTasks` — the title/client column (`CTDLTaskListCtrl`).
- `m_lcColumns` — the attribute columns (`CTDLTaskCtrlBase`).

They are synchronised by `CTreeListSyncer` using
`TLSL_RIGHTDATA_IS_LEFTITEM`; the column list stores an index into the title list.
Both lists have `LVS_EX_DOUBLEBUFFER`, but double buffering is per window, so it
cannot make the two independently painted surfaces update atomically.

## Root cause

The title column draws its text in `CDDS_ITEMPOSTPAINT` and takes the task ID from
`nmcd.lItemlParam` (`CTDLTaskCtrlBase::OnPostPaintTaskTitle`). During
double-buffered/synchronised scrolling that notification can carry a stale item
`lParam`, while `nmcd.dwItemSpec` still identifies the item being painted. The
title therefore resolves to the last task that was drawn successfully.

The attribute columns do not use `lItemlParam`; they resolve the task from
`nmcd.dwItemSpec` and the list item-data mapping
(`CTDLTaskCtrlBase::OnListCustomDraw` -> `GetColumnItemTaskID`), which is why they
remain correct.

Evidence: `biglist.tdl` contains exactly one task titled `Interessantes`
(ID 372). The screenshot repeats that title while the attribute columns show the
correct dates for tasks 353, 352, 358, 46, 300, 302, 301 and 341.

## Attempted fixes (reverted)

Two local fixes were tried and manually tested:

1. Resolve the title from `nmcd.dwItemSpec`/`GetTaskID()` instead of
   `nmcd.lItemlParam` in `OnPrePaintTaskTitle`/`OnPostPaintTaskTitle`.
2. Resolve `LVN_GETDISPINFO` text/image from `item.iItem`/`GetTaskID()` and always
   derive the title rect with `GetItemRect()` instead of trusting `nmcd.rc`.

Neither changed the artefact. The screenshots after the first fix still showed the
same title repeated, and after the second fix the problem remained. The code was
therefore reverted to keep the working tree clean. The investigation above remains
useful, but the actual paint/skip mechanism is not yet proven.

## Verification harness (CUST-004)

The [CUST-004 harness](CUST-004-ui-perf-harness.md) has a `UIPerf::ListScrollSync`
scenario that drives List View with the real wheel and checks the title list and
column list `LVM_GETTOPINDEX` values against each other. This top-index mismatch is
the reliable automated signal for this bug: the title column lags behind the
attribute columns by several rows.

```bat
:: automated fast-scroll check using the generated tasklist (no -tdl needed)
Core\TDLTest\Unicode_Debug\TDLTest.exe -ui -uiscroll -notches 2 -delay 0 -record

:: real sample, 3 Page Downs, 20 fps JPEG recording, cropped to the list windows
Core\TDLTest\Unicode_Debug\TDLTest.exe -ui -uiscroll ^
  -tdl ..\..\..\..\ToDoList_Test\Tasklists\Commercial Construction.tdl ^
  -size 800x600 -capture task -keys -record -recms 50 -steps 6 -delay 150

:: manual: leave List View open and record while you press Page Up/Down
Core\TDLTest\Unicode_Debug\TDLTest.exe -ui -uiscroll -uihold -record ^
  -tdl ..\..\..\..\ToDoList_Test\Tasklists\Commercial Construction.tdl ^
  -size 800x600 -capture task -recms 50

:: same, settled pixel check (also catches stale attribute-pane fields)
Core\TDLTest\Unicode_Debug\TDLTest.exe -ui -uiscroll ^
  -tdl ..\..\..\..\ToDoList_Test\Tasklists\Commercial Construction.tdl ^
  -size 1200x1200 -keys -steps 20

:: manual run: opens the tasklist in List View and leaves it open
Core\TDLTest\Unicode_Debug\TDLTest.exe -ui -uiscroll -uihold -tdl ^
  ..\..\..\..\ToDoList_Test\Tasklists\Commercial Construction.tdl
```

- Fast-scroll pass criterion: `syncErrors=0` and the top index has advanced.
- 20 fps, 3 Page Downs (`-recms 50 -steps 6 -delay 150`) recorded 12 JPEG frames at
  745x400. Frame 4 shows the title column stale: `Commercial Construction`,
  `Three-story Office Building`, ... against IDs 19, 20, 21 and the scrolled
  attribute columns. Frame 5 shows the titles caught up. This is CUST-002 captured
  visually; the synchronous top-index query missed it because it ran after the
  catch-up.
- `-maximize` is currently not recommended for key scrolling: in the test run the
  Page Down keys did not scroll after it (`top=0`). `-capture task` already excludes
  the filter/group-by panels and attribute pane, so maximize is unnecessary.
- Baseline with `Commercial Construction.tdl` + Page Down:
  `ListScroll[2]: top1=21 top2=42 MISMATCH`, `syncErrors=1`. The title list lags
  the attribute columns by a page.
- Settled Page Down run on the same file: `maxDiff=1,166,022`,
  `artifactSteps=5/10`. The on-screen frame shows the attribute pane fields blank
  (`Allocated To`, `Dependency`, `Due Date`, ...) while the forced-repaint frame
  shows them populated. So the pane next to the tree is affected too, not only the
  title column.
- Generated fixture baseline (no `-tdl`): `ListScroll[0]: top1=0 top2=5 MISMATCH`,
  `syncErrors=1`; the first recorded JPEG shows titles `List task 5...` against IDs `11...`.
- Settled wheel-only run (`maxDiff≈529`, `artifactSteps=0`) does not reproduce it;
  use Page Down (`-keys`) and a tall window (`-size`), as above.
- If `-tdl` is omitted the generated fixture is used. It is cached under
  `%TEMP%\TDLPerf\Cache\`; add `-regenerate` after changing fixture code. Real
  samples from the sibling `ToDoList_Test` repository are preferred.
- Recorded frames are JPEGs `frame_NNNNN_<tick>.jpg` (fast mode) or
  `scroll_NN_onscreen.bmp`/`_clean.bmp` (settled mode) under
  `%TEMP%\TDLPerf\ListScroll_<timestamp>\`.
- To generate a fixture and launch ToDoList yourself, run `-ui -uifix` and use the
  printed `Manual list:` command line.

## What the screenshots establish

- The repeated title is always the title of the **first visible row** at the top of
  the viewport. On page-down it repeats down the entire client area.
- The attribute columns keep showing the correct per-row values.
- The affected row range depends on scroll speed/step size, which points to a
  listview scroll/blit optimisation interacting with custom draw, not to task-ID
  lookup.
- The title list is a separate window from the column list, so double buffering
  cannot make their updates atomic.

## Next hypotheses (not yet tested)

| Hypothesis | Suggested experiment |
|---|---|
| `LVS_EX_DOUBLEBUFFER` + title post-paint is incompatible during bulk scroll | Temporarily remove `LVS_EX_DOUBLEBUFFER` from `m_lcTasks` only and repeat the page-down test. |
| The title list retains copied pixels and does not repaint the exposed range | After the synced scroll, force `RedrawWindow(m_lcTasks, NULL, NULL, RDW_INVALIDATE \| RDW_ERASE \| RDW_UPDATENOW)` and measure. |
| Custom draw post-paint is skipped while the control asks only for prepaint | Return `CDRF_SKIPDEFAULT` from the title item prepaint and draw the full title item in one stage, or move to a single list control. |
| Two-window scroll sync is inherently racy | Wrap the paired scroll in `WM_SETREDRAW(FALSE)`/`TRUE` or replace the title/column split with one list view. |

## Findings from the CUST-004 harness (2026-11-12)

The harness captured the artefact at 20 fps (`-recms 50`) and made the paint
mechanism observable:

- The repeated row is a **copy of one painted row**, not a stale task lookup.
  A temporary 3x5 magenta marker drawn in `OnPostPaintTaskTitle` appeared on the
  repeated rows as well. A per-row repaint with a stale `lItemlParam` would paint
  the marker on every row; a copied pixel block reproduces the marker too.
- The same failure can appear in either pane: frame 4 of the reference run
  (`ListScroll_20261112_034949`) had the title pane correct and the attribute
  pane repeating ID `20`/its values; later runs showed the title pane repeating
  one title.
- `CHoldScrollRepaint` (hold both windows with `WM_SETREDRAW(FALSE)`, scroll,
  resync, then `RedrawWindow(parent, RDW_INVALIDATE | RDW_UPDATENOW |
  RDW_ALLCHILDREN)`) removed the settled top-index mismatch
  (`syncErrors=0` for `-notches 2 -delay 0`), but 20 fps recording still caught
  one-frame partial paints. `LockWindowUpdate(parent)` delayed the resync and
  did not remove them.
- Setting `WS_EX_COMPOSITED` on the syncer parent made the attribute columns
  render blank intermittently, so it is not usable as-is.
- Removing `LVS_EX_DOUBLEBUFFER` replaced the repeated rows with a blank title
  pane during the transition (visible flicker), not a fix.

Conclusion: the defect is the non-atomic paint of two independent child windows,
not the task-ID/lParam lookup. Per-window double buffering, redraw suppression
and `RedrawWindow` can only shorten the window in which a partial frame is
visible; they cannot make a screen capture (or the user's eye) see one coherent
state.

## Recommended fix path

1. Render the title and attribute panes into one surface and present that surface
   atomically. The least invasive form is a single list control that owns both
   the title/client column and the attribute columns; the splitter then becomes
   a column boundary rather than a window boundary.
2. If the two-window design must stay, add a parent-level compositor: paint both
   child windows into one memory DC (`WM_PRINTCLIENT` or the existing
   `SaveToImage` path) and blit the result to the parent once per scroll, with
   the child windows hidden from direct screen painting during the scroll.
3. Do not pursue task-ID/lParam or `LVN_GETDISPINFO` changes; the marker
   experiment and the symmetric title/attribute failures rule that out.

The CUST-004 `UIPerf::ListScrollSync` scenario should gate the work:
`syncErrors=0`, settled `artifactSteps=0`, and a manual scan of the 20 fps JPEG
sequence for repeated rows.

## Files touched in the investigation (now reverted)

- `Core/ToDoList/TDLTaskCtrlBase.h`
- `Core/ToDoList/TDLTaskCtrlBase.cpp`
- `Core/ToDoList/TDLTaskListCtrl.h`
- `Core/ToDoList/TDLTaskListCtrl.cpp`
- `Core/Shared/TreeListSyncer.h` (hold-redraw experiment)
- `Core/Shared/TreeListSyncer.cpp` (hold-redraw experiment)

## Verification

- Both attempted builds completed (`Unicode Debug|Win32` and
  `Unicode Release|Win32`).
- Manual testing confirmed the artefact remains, so the changes were reverted.
- Full `TDLTest` run: 310240/310264 pass; the 24 failures are the pre-existing
  `CTDCTaskMatcherTest::IndividualRules` failures already recorded in CUST-001.

