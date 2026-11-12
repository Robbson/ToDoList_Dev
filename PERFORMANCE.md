# ToDoList Performance Analysis

Status: **Phase 1 (static analysis) complete; runtime measurement pending**
Scope: `Core/ToDoList`, `Core/Shared`, `Core/3rdParty/SimpleSplitter.*`
Date: 2026-11-12

This document records where the UI spends its time today, why splitter dragging
produces visible artefacts, and what should be measured and fixed next. No source
changes were made as part of this analysis.

---

## 1. Executive summary

The splitter artefacts are not a rendering-polish problem. They come from a
combination of four design choices:

1. **Live resize on every mouse move** with forced synchronous paints.
2. **A full-client splitter window used as a transparent overlay** that paints only
   its bars and relies on the task view's pixels surviving underneath.
3. **Erasing invalidations** (`Invalidate(TRUE)`) of the whole splitter on every
   resize, plus missing `WS_CLIPCHILDREN` on the splitter.
4. **No double buffering at the container level** (the tree and list have
   `LVS_EX_DOUBLEBUFFER`/`TVS_EX_DOUBLEBUFFER`, the splitter and host dialog do not).

The "slow UI" feeling is amplified by the same mechanism: one splitter mouse move
can trigger several full moves and several synchronous `UpdateWindow()` passes over
the whole task view. The cost scales with the visible task count and with the number
of visible columns because the task views do expensive custom drawing per row and per
cell.

The highest-value structural fixes are therefore (a) stop resizing live and use a
drag tracker, and (b) stop erasing/repainting the world on every layout change. Fixing
the custom-draw cost without fixing the invalidation strategy will not remove the
artefacts.

The screenshot supplied with the follow-up shows a separate list-view scrolling
artefact. That one is **not** caused by missing double buffering in the task views: it
is caused by "List View" actually being two independently painted list controls (title
and attribute columns) that are scrolled one after the other, and by the title text
being drawn in `CDDS_ITEMPOSTPAINT` from `nmcd.lItemlParam`. See P0-4.

---

## 2. Method and limitations

- Read-only static inspection of the paint, layout, splitter and task-view code paths.
- Traced call chains from `CSimpleSplitter::OnMouseMove` through
  `CToDoCtrlLayout`, `CToDoCtrl`, `CTabbedToDoCtrl` and `CTreeListSyncer`.
- Checked window styles, double-buffer flags, invalidation flags and forced updates.
- No profiler or ETW capture was run for this phase, so the individual timings below
  are reasoned costs, not measured numbers. Section 7 defines the measurement plan.
- Plugin and extension projects were excluded, per `AGENTS.md`.

Confidence labels used below:

- **Confirmed** – directly visible in code.
- **Strongly indicated** – code implies the behaviour; confirm on a live window with
  Spy++/WinSpy (z-order and update regions).
- **Measure** – likely cost; needs runtime data.

---

## 3. Relevant rendering architecture

The important window hierarchy for performance is:

```text
CToDoListWnd (frame, WS_CLIPCHILDREN)
  └─ CFilteredToDoCtrl : CTabbedToDoCtrl : CToDoCtrl (no WS_CLIPCHILDREN)
       ├─ CTDLTaskTreeCtrl / CTDLTaskListCtrl   (the active task view)
       │    ├─ CTreeCtrl or CListCtrl           (double-buffered system control)
       │    └─ CListCtrl (attribute columns)    (double-buffered)
       ├─ CTDLTaskAttributeCtrl
       ├─ CTDLCommentsCtrl
       ├─ CSimpleSplitter (full-client child, created last => above the task view)
       │    └─ attribute/comments panes when they are laid out inside it
       └─ CTDLViewTabControl
```

Two different splitters exist and both are live-resizing:

| Splitter | Owner | Purpose |
|---|---|---|
| Outer `CSimpleSplitter` pair | `CToDoCtrlLayout` | Tasks vs attributes vs comments |
| Inner syncer split | `CTreeListSyncer` | Task title tree vs attribute-column list |

The task view draws itself in `NM_CUSTOMDRAW` handlers. The tree and list controls
themselves request double buffering in `CTreeListSyncer::InitialiseStyles`
(`Core/Shared/TreeListSyncer.cpp:446-472`), but the container windows above them do
not.

---

## 4. Findings

### P0-1: Live splitter resize forces multiple synchronous paints per mouse move

**Confirmed.** The outer splitter resizes panes on every `WM_MOUSEMOVE`, then calls
`UpdateWindow()`:

- `Core/3rdParty/SimpleSplitter.cpp:570-598` – `OnMouseMove` recomputes sizes, calls
  `ResizePaneWindows()`, then `UpdateWindow()` (line 597).
- `Core/3rdParty/SimpleSplitter.cpp:430-439` – `ResizePaneWindows()` calls
  `MoveWindow()` for every pane and ends with `Invalidate(TRUE)` (line 438).
- `Core/3rdParty/SimpleSplitter.cpp:441-457` – `ResizePaneWindow()` moves each child
  with the default `bRepaint = TRUE`. A `NULL` pane notifies the parent.

For the normal "tasks + attributes" layout the task view is the `NULL` pane, so the
move is delegated:

```text
CSimpleSplitter::OnMouseMove                  SimpleSplitter.cpp:570
  -> ResizePaneWindows                        SimpleSplitter.cpp:430
     -> MoveWindow(attributes/comments, TRUE) SimpleSplitter.cpp:448
     -> WM_SS_NOTIFYSPLITCHANGE (task pane)   SimpleSplitter.cpp:455
        -> CToDoCtrl::OnSplitChange           ToDoCtrl.cpp:834
           -> ReposTaskCtrl                   ToDoCtrl.cpp:852
              -> CTabbedToDoCtrl::ReposTaskCtrl   TabbedToDoCtrl.cpp:2407
                 -> m_tabViews.Resize             TabbedToDoCtrl.cpp:2410
                    -> pView->MoveWindow(TRUE)    TDLViewTabControl.cpp:300
                 -> UpdateWindow()                TabbedToDoCtrl.cpp:2423
  -> Invalidate(TRUE) (whole splitter)        SimpleSplitter.cpp:438
  -> UpdateWindow() (whole splitter)          SimpleSplitter.cpp:597
```

So a single mouse move can cause:

- one move-and-repaint of the attributes/comments pane,
- one move-and-repaint of the whole active task view,
- one synchronous full repaint of the host `CToDoCtrl` (`UpdateWindow` at
  `TabbedToDoCtrl.cpp:2423`),
- one erase-invalidate and synchronous repaint of the splitter itself.

This is repeated at the mouse-move rate. There is no coalescing, no
`WM_SETREDRAW(FALSE)`/`DeferWindowPos`, and no drag tracker.

The internal tree/list split is worse:

- `Core/Shared/TreeListSyncer.cpp:1607-1632` – `WM_MOUSEMOVE` calls `Resize()` and
  then `InvalidateAll(TRUE, TRUE)`.
- `Core/Shared/TreeListSyncer.cpp:3115-3130` – `InvalidateAll(..., bUpdate=TRUE)`
  immediately calls `UpdateAll()`, which calls `UpdateWindow()` on the tree, the list
  and the header (three synchronous paints).
- `CTreeListSyncer::Resize` (`Core/Shared/TreeListSyncer.cpp:3251-3423`) moves both
  controls and resyncs scroll positions on every mouse move.
- `CTDLTaskCtrlBase::OnNotifySplitterChange`
  (`Core/ToDoList/TDLTaskCtrlBase.cpp:1904-1911`) then calls `InvalidateAll(TRUE)`
  again, re-erasing both controls.

**Impact:** splitter drag latency is directly proportional to task-view paint cost.
On a large tasklist with many columns this can feel like the UI is ignoring the mouse.

---

### P0-2: The splitter is a transparent overlay that is periodically erased

**Confirmed/strongly indicated.** `CSimpleSplitter` is a normal child window covering
the whole client of `CToDoCtrl`:

- `Core/ToDoList/ToDoCtrlLayout.cpp:128-156` – `Resize` moves the root splitter to the
  full client rect.
- `Core/ToDoList/ToDoCtrlLayout.cpp:318-332` – `ResizeIfRoot` calls
  `splitter.MoveWindow(rect)`.
- `Core/3rdParty/SimpleSplitter.cpp:96-106` – the splitter is created with
  `WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS` only.

It was added in `RebuildLayout` after the task tree was created, so in z-order it sits
above the task view (`Core/ToDoList/ToDoCtrl.cpp:571` creates the task view;
`Core/ToDoList/ToDoCtrlLayout.cpp:419+` creates the splitters).

It only paints its bars, not a background:

- `Core/3rdParty/SimpleSplitter.cpp:472-502` – `OnPaint` draws only the split bars
  and has no background fill.
- `Core/3rdParty/SimpleSplitter.h:48-62` – there is no `ON_WM_ERASEBKGND()` handler.
- `Core/ToDoList/ToDoCtrl.cpp:788-814` – the host dialog explicitly clips children and
  split-bar rectangles out of its own erase, but **not the splitter window**. The
  intent is clearly that the splitter's non-bar pixels show the task view through.

That design only works while the splitter's client is not erased. But
`ResizePaneWindows()` calls `Invalidate(TRUE)` (`SimpleSplitter.cpp:438`). Because the
splitter has no `WM_ERASEBKGND` override and no `WS_CLIPCHILDREN`, the default erase
fills the invalid region with the window background. The region includes:

- the task-view pixels that the splitter is supposed to be transparent over, and
- the attribute/comments pane pixels, because the splitter does not clip children.

`OnPaint` then redraws only the bars, leaving the erased area blank until the task
view happens to repaint. The interleaving of the two independent paint streams (task
view via `ReposTaskCtrl`/`UpdateWindow`, splitter via its own `UpdateWindow`) produces
the bands, flashes and stale strips seen while dragging.

`CSimpleSplitter::OnWindowPosChanging` additionally sets `SWP_NOCOPYBITS`
(`Core/3rdParty/SimpleSplitter.cpp:612-616`), which throws away the OS copy-bits
optimisation and forces full repaint on every size change. That removes one of the few
cheap mitigations the system would otherwise provide.

**Why the artefact is visible only while dragging:** at rest, the task view is normally
repainted last or is not invalidated, so the pixels under the splitter are complete.
During a drag the splitter erase is issued after the task-view paint, so the final
visible state is the erased splitter (bars only) until the next task-view paint.

**Confidence:** the erase-over-sibling mechanism is strongly indicated by the code;
z-order and update-region order should be confirmed with Spy++ before implementing a
fix, because the intended "transparent overlay" also depends on that order.

Related list-view-specific amplifier:

- `Core/ToDoList/TabbedToDoCtrl.cpp:6805-6811` – in list view every split change calls
  `Invalidate()` (default `bErase = TRUE`) on the whole tabbed control before forwarding
  to the base. That is another full-host erase per mouse move.

---

### P0-3: No container-level double buffering and no clipping style

**Confirmed.**

- `CTreeListSyncer::InitialiseStyles`
  (`Core/Shared/TreeListSyncer.cpp:446-472`) enables `LVS_EX_DOUBLEBUFFER` and
  `TVS_EX_DOUBLEBUFFER` on the system controls. Good.
- `CSimpleSplitter::Create` (`Core/3rdParty/SimpleSplitter.cpp:105`) does **not** set
  `WS_CLIPCHILDREN` and does not set `WS_EX_COMPOSITED`.
- `CToDoCtrl::Create` (`Core/ToDoList/ToDoCtrl.cpp:543-549`) does not set
  `WS_CLIPCHILDREN` either; it compensates only with manual exclusions in
  `OnEraseBkgnd` (`ToDoCtrl.cpp:788-814`). Those exclusions do not cover the splitter
  windows.
- There is no `WS_EX_COMPOSITED` anywhere in the code base (searched `Core/`).

The result is that the splitter and host erase/paint work is not isolated from the
system controls, and the only double buffering is inside the leaf controls. That is
why the leaf controls themselves look acceptable while the join between them flickers.

---

### P0-4: Why `LVS_EX_DOUBLEBUFFER` does not prevent the list-view scroll artefact

The attached screenshot (`task-list-scrolling-issue.png`) shows a second, distinct
class of artefact from the splitter problem. In "List View" the application is not one
list control: it is **two independent list views** kept in step by `CTreeListSyncer`:

- `m_lcTasks` – the single title/client column on the left (`TDLTaskListCtrl.h:124`).
- `m_lcColumns` – the attribute columns on the right (`TDLTaskCtrlBase.h:240`).

They are synced with `TLSL_RIGHTDATA_IS_LEFTITEM` (`TDLTaskCtrlBase.cpp:317-328`), so
the column list stores an *index* into the title list, not a task ID.

Double buffering is set per control (`TreeListSyncer.cpp:446-472`). That means:

- each list is protected against flicker **inside its own window**, but
- the two windows still paint independently, and
- a scroll step is applied to one list first and to the other afterwards
  (`TreeListSyncer.cpp:2640-2649`: `ScDefault(hRealWnd)` then
  `ResyncScrollPos(OtherWnd(hRealWnd), hRealWnd)`).

There is no `WM_SETREDRAW(FALSE)`, `LockWindowUpdate` or deferred paint around the two
scroll calls, so double buffering cannot make the two surfaces change atomically. One
list can already show a new row while the other still shows the old row.

The screenshot proves that this is more than a one-frame tear. The tasklist contains
exactly one task titled "Interessantes" (ID 372, `biglist.tdl`). The attribute columns
in the screenshot continue correctly with the following tasks:

| Row | Title list shows | Attribute columns show | Actual task |
|---|---|---|---|
| 1 | Interessantes | 13.02.2018 16:32 | 372 Interessantes |
| 2 | Interessantes | 31.05.2018 15:05 | 353 Spring MVC |
| 3 | Interessantes | 06.11.2017 14:19 | 352 Spring Data |
| 4 | Interessantes | 17.09.2017 15:30 | 358 Spring & Angular |
| ... | ... | ... | ... |

The right-hand dates resolve through `GetColumnItemTaskID`
(`TDLTaskListCtrl.cpp:301-308`) and are correct. The left-hand title is stuck on task
372, so the title list is not showing the same frame as the column list.

The difference is in how the two lists resolve the task during custom draw:

- **Attribute columns** draw in `CDDS_ITEMPREPAINT` and derive the task from
  `dwItemSpec` plus the list item-data mapping:
  `TDLTaskCtrlBase.cpp:2460-2461`.
- **Title column** draws in `CDDS_ITEMPOSTPAINT` and uses
  `nmcd.lItemlParam` directly:
  `TDLTaskListCtrl.cpp:371-377` -> `TDLTaskCtrlBase.cpp:2574` ->
  `m_data.GetTrueTask(dwTrueID, ...)`.

`OnPostPaintTaskTitle` returns `CDRF_SKIPDEFAULT` but, because the pre-paint stage did
not return `CDRF_SKIPDEFAULT`, the control has already performed its default paint in
between. During a double-buffered scroll the post-paint notification can refer to a
stale or partially updated item state; using its `lItemlParam` therefore repeats the
last successfully resolved title. The columns path avoids this because it uses the
index from `dwItemSpec` and looks the task up through the title list.

This is also why the title text is wrong but the icons, checkboxes, selection, and
attribute columns look plausible: those are drawn by different code paths. The title
is the only part whose identity comes from the post-paint notification.

**Confidence:** the stale title is confirmed from the screenshot and the task file;
the exact notification state needs a debug trace to prove which of the two mechanisms
(stale `lItemlParam`, or the title list simply not repainting those rows in that frame)
dominates. Either way, double buffering cannot fix it because the two lists are
separate windows and the title path draws after the control's default paint.

**Recommended correction for this specific bug:**

1. In the list-view title path, resolve the task from `nmcd.dwItemSpec` and
   `GetTaskID((int)nmcd.dwItemSpec)` instead of `nmcd.lItemlParam`. Keep
   `lItemlParam` only for the tree view, where the item data is the tree item's task
   ID and is stable.
2. Alternatively, return `CDRF_SKIPDEFAULT` from the item pre-paint and draw the
   entire title item in one stage, so no default text is painted between pre and post.
3. Wrap the paired scroll in `WM_SETREDRAW(FALSE)`/`SendMessage(WM_SETREDRAW, TRUE)`
   (or `LockWindowUpdate` for a short, non-nested scope) and invalidate both lists once
   at the end. This makes the sync visually atomic even if the underlying lists remain
   separate windows.
4. Longer term, use a single list control for the title and attribute columns in list
   view. That removes the two-window sync problem entirely and makes double buffering
   cover the whole row.

**Implementation status:** deferred. Two attempts were made and manually tested:
resolving the title from `dwItemSpec`/`GetTaskID`, and resolving the
`LVN_GETDISPINFO` callback plus the item rect from the item index. Neither removed
the repeated-title artefact, so both changes were reverted. The evidence now points
to a listview scroll/blit + custom-draw interaction rather than task-ID lookup: the
repeated title is always the first visible row, and page-down repeats it down the
whole viewport. See
[customized/CUST-002-list-view-stale-title.md](customized/CUST-002-list-view-stale-title.md)
for the attempted fixes and the next experiments (disable double buffering on the
title list only; force a full redraw after the paired scroll; or use one list control
for the title and columns).

---

### P1-1: The task view is fully invalidated for many ordinary operations

**Confirmed.**

- `CTDLTaskCtrlBase::OnNotifySplitterChange` calls `InvalidateAll(TRUE)`
  (`Core/ToDoList/TDLTaskCtrlBase.cpp:1909`). The `TRUE` means erase background for
  both the title and column views.
- `CTreeListSyncer::WM_MOUSEMOVE` already calls `InvalidateAll(TRUE, TRUE)` before the
  derived callback (`Core/Shared/TreeListSyncer.cpp:1627`), so the inner splitter
  erases twice per mouse move.
- Horizontal scrolling invalidates the whole task view because comments can trail the
  title text: `TDLTaskCtrlBase.cpp:4223-4239`.
- `CToDoListWnd::Resize` ends with `Invalidate(TRUE)` on the whole frame
  (`Core/ToDoList/ToDoListWnd.cpp:6547`), and `CToDoCtrl::OnSize` calls
  `UpdateWindow()` after every resize (`Core/ToDoList/ToDoCtrl.cpp:714-721`).

**Impact:** even when only a narrow strip changes, the task views are asked to repaint
all visible rows. With custom draw that is the expensive path.

---

### P1-2: Custom-draw cost scales with rows × columns and repeats formatting work

**Confirmed.** The task views return `CDRF_SKIPDEFAULT` and draw every item themselves:

- List/title columns: `CTDLTaskCtrlBase::OnListCustomDraw`
  (`Core/ToDoList/TDLTaskCtrlBase.cpp:2446-2531`).
- Tree title: `OnPrePaintTaskTitle`/`OnPostPaintTaskTitle`
  (`TDLTaskCtrlBase.cpp:2534-2645`).
- Column text: `DrawColumnsRowText` (`TDLTaskCtrlBase.cpp:2647-2690`) calls
  `GetTaskColumnText` per cell (`TDLTaskCtrlBase.cpp:3890-3977`).

Per row this means:

- `GetTaskTextColors` (`TDLTaskCtrlBase.cpp:1935-2090+`) runs calculator queries such
  as `IsTaskDone`, `IsTaskDueToday`, `IsTaskOverDue`, and optionally
  `GetTaskPriority`.
- `DrawExplorerItemSelection` (`Core/Shared/GraphicsMisc.cpp:1584-1788`) constructs a
  `CThemed` object and issues themed background draws per selected row. The themed
  path draws the selection twice for `GMIS_SELECTED` (`GraphicsMisc.cpp:1714-1718`).
- Every cell formats a fresh `CString` through `CTDCTaskFormatter`; some formatters do
  non-trivial work:
  - `GetTaskPosition` walks the ancestor chain (`ToDoCtrlDataUtils.cpp:3953-3959`,
    `ToDoitem.cpp:2007-2025`).
  - `GetTaskPath` builds a path with repeated `CString` concatenation
    (`ToDoCtrlDataUtils.cpp:3961-3989`).
  - `GetTaskSubtaskCompletion` aggregates descendants
    (`ToDoCtrlDataUtils.cpp:3940-3951`).
  - `GetTaskTimeRemaining` runs the time calculator
    (`ToDoCtrlDataUtils.cpp:4447+`).
- `GetTaskPriority` can recurse through descendants when
  `TDCS_USEHIGHESTPRIORITY`/`TDCS_DUEHAVEHIGHESTPRIORITY` are enabled
  (`ToDoCtrlDataUtils.cpp:3215-3263`), and `CTDCTaskCalculator::IsTaskDone` has
  child-checking variants (`ToDoCtrlDataUtils.cpp:3329-3345`) used by completion,
  sorting and filter paths.

There is no per-paint cache of formatted cell text or row colours. Every repaint of a
row recomputes all of it. This is the main reason that "the whole view repaints"
translates into a visible stall on large tasklists.

**Measure:** instrument `WM_PAINT` for tree and list and report rows × columns × paint
time. Compare a 100-task list with a 5,000-task list, and a 5-column vs 20-column
layout.

---

### P1-3: Column-width recalculation is O(visible tasks × columns)

**Confirmed.** `CTDLTaskCtrlBase::RecalcUntrackedColumnWidths`
(`Core/ToDoList/TDLTaskCtrlBase.cpp:1316-1418`):

- Builds a `CDWordArray` of every visible task ID
  (`GetColumnTaskIDs`, `TDLTaskCtrlBase.cpp:1268-1298`).
- Runs `m_sizer.GetLongestValues(...)` across all of them.
- Falls back to `CalcColumnWidth` per column (`TDLTaskCtrlBase.cpp:5388+`), which in
  turn calls `GetAverageMaxStringWidth`, i.e. a GDI `GetTextExtent` per candidate
  string (`Core/Shared/GraphicsMisc.cpp:860-877`).

It is scheduled through idle processing (`IDLETASKS::Process`,
`TDLTaskCtrlBase.cpp:157-188`) and called from several ordinary operations:

- after a tree rebuild (`OnEndRebuild`, `TDLTaskTreeCtrl.cpp:307-316`),
- after expanding items (`TDLTaskTreeCtrl.cpp:285-289`),
- after item expansion notifications (`TDLTaskTreeCtrl.cpp:954-956`),
- after column/style/attribute changes (`TDLTaskCtrlBase.cpp:1241-1258` and callers),
- after font changes (`CToDoCtrl.cpp:668-671` -> `RecalcAllColumnWidths`).

For large tasklists this is the classic "one edit causes a full scan and a freeze"
pattern. It is also a common trigger for repaint storms if a width change causes the
splitter or scrollbars to re-layout.

**Measure:** time `RecalcUntrackedColumnWidths` with logging enabled; the code already
has a permanent `CScopedLogTimer` at `TDLTaskCtrlBase.cpp:1319`. Note that the timer
still formats its scope string on every call even when logging is off
(`Core/Shared/ScopedTimer.cpp:15-21`), so the "permanent logging" itself has a small
constant cost.

---

### P1-4: Filter and tab-view operations rebuild or drain work synchronously

**Confirmed.**

- `CFilteredToDoCtrl::RefreshTreeFilter` calls `RebuildTree()` from scratch
  (`Core/ToDoList/FilteredToDoCtrl.cpp:567-596`), and `RebuildTree` builds a search
  query from the filter (`FilteredToDoCtrl.cpp:673-688`). Rebuilding means deleting and
  re-inserting every matching item, re-syncing the column list and re-running idle
  column-width recalculation.
- `RefreshFilter` rebuilds the active list or schedules a full task update
  (`FilteredToDoCtrl.cpp:514-565`).
- `CTabbedToDoCtrl::OnPostTabViewChange` drains idle processing synchronously when
  switching to the list view:
  `while (m_taskList.DoIdleProcessing());` (`TabbedToDoCtrl.cpp:1114-1121`).
  That loop can include full column-width recalculation and a pending resort, so the tab
  switch blocks until all of it is done.

**Impact:** toggling a filter or switching to the list view can freeze the UI for as
long as a full rebuild/scan takes. This is separate from splitter dragging but
contributes to the overall "slow app" impression.

---

### P2-1: Smaller costs worth cleaning up

- **Header tracking** does erase-invalidate plus `UpdateWindow` per mouse move:
  `CTreeListSyncer::OnListHeaderTrackItem` (`TreeListSyncer.cpp:1923-2011`, especially
  lines 2000-2010). Dragging a column divider is in the same class of problem as the
  splitters.
- **Full frame erase on resize:** `CToDoListWnd::Resize` invalidates the whole frame
  with erase (`ToDoListWnd.cpp:6547`) after deferred moves. The frame itself is
  `WS_CLIPCHILDREN`, which helps, but a single final invalidation of affected strips
  would be cheaper.
- **Per-call timer formatting:** `CScopedLogTimer` format strings are built even when
  logging is disabled (`ScopedTimer.cpp:15-21`, `86-90`). This affects
  `RecalcUntrackedColumnWidths`, `DoSort`, `SelectItem`, `ToggleFilter`, `ExpandTasks`,
  etc. The cost is small relative to painting but easy to remove.
- **`UpdateWindow()` in `CToDoCtrl::OnSize`** (`ToDoCtrl.cpp:720`) makes every host
  resize synchronous, including resizes caused by the frame and by splitter changes
  that reach the host. Deferring/coalescing this is usually a net win.

---

## 5. Why the blitting artefacts appear: end-to-end view

For a single outer-splitter mouse move in the normal tree layout:

1. The task view has just painted its visible rows. The splitter (above it in
   z-order) has painted its bars and nothing else; the task view's pixels are visible
   through the splitter's untouched client area.
2. `CSimpleSplitter::OnMouseMove` updates sizes and calls `ResizePaneWindows()`.
3. `ResizePaneWindow()` moves the attribute/comments pane with `bRepaint = TRUE`, which
   invalidates that child and schedules its paint.
4. The task pane is `NULL`, so `WM_SS_NOTIFYSPLITCHANGE` is sent to `CToDoCtrl`.
   `ReposTaskCtrl()` moves the task view (`MoveWindow(..., TRUE)`) and
   `CTabbedToDoCtrl::ReposTaskCtrl` calls `UpdateWindow()` on the host. That forces the
   host to paint synchronously while child paints are still queued.
5. `ResizePaneWindows()` then calls `Invalidate(TRUE)` on the splitter. The splitter's
   invalid region covers the full client, including the task-view area it overlaps.
6. Because the splitter has no `WM_ERASEBKGND` override and no `WS_CLIPCHILDREN`, the
   default erase fills that region with the splitter background. The task-view pixels
   and the child pane pixels are overwritten.
7. `OnPaint` redraws only the bars. `OnMouseMove` then calls `UpdateWindow()` on the
   splitter, so the erased state is presented immediately.
8. The queued task-view and pane paints run later (or are forced by the next mouse
   move), restoring some regions. Because the two paint streams are not synchronized,
   the user sees bands, flashes and stale strips while the mouse moves.

The result is exactly the "no double buffering anywhere" appearance the user
described. The leaf tree/list controls do have double buffering, but the join between
them and the splitter is painted by the non-buffered overlay.

**Quick confirmation test (no code change):** open the app with a large tasklist,
start a splitter drag, and watch the task-view region with the UI Spy tool. The
splitter window's update region should cover the task view. Alternatively, temporarily
comment out the `Invalidate(TRUE)` at `SimpleSplitter.cpp:438` and the `UpdateWindow()`
at line 597; if the artefacts mostly disappear, the diagnosis is confirmed.

---

## 6. What is *not* the primary problem

To avoid wasted work:

- The system tree/list controls already request double buffering. Adding another
  `WS_EX_COMPOSITED` blindly to the main frame is not the first fix; it can increase
  CPU and change scroll/child behaviour.
- The filter/search engine itself is not implicated in the drag artefact. It can make
  filter toggles slow, but it does not run per mouse move during a splitter drag.
- Release-vs-debug does not solve this. The artefact is an invalidation/paint-order
  issue and the drag cost is real work, not assertions or trace output.

---

## 7. Measurement plan (Phase 2)

Do this before and after any fix, with the same tasklists and window sizes.

### 7.1 Scenarios

| Scenario | What to capture |
|---|---|
| Drag outer splitter (tree view) | `WM_MOUSEMOVE` count, `WM_PAINT` count, total drag time, GDI objects |
| Drag outer splitter (list view) | same, plus time in `CTabbedToDoCtrl::OnSplitChange` |
| Drag inner title/columns splitter | `UpdateAll()` calls, tree/list paint times |
| Resize main window | `CToDoListWnd::Resize`, `CToDoCtrl::OnSize`, paint times |
| Scroll vertically/horizontally | rows repainted per scroll step; title vs column top index/offset; `LVN_GETDISPINFO` and `NM_CUSTOMDRAW` item IDs |
| Toggle filter | `RefreshTreeFilter`, `RebuildTree`, `RecalcUntrackedColumnWidths` |
| Switch tree -> list tab | time in the synchronous `DoIdleProcessing()` drain |
| 100 / 1,000 / 10,000 tasks | scaling curve for paint and width recalc |

### 7.2 Instrumentation

The code already has a logging facility and permanent timers in useful places:

- Enable logging at startup with the `-g` switch (`Core/ToDoList/TDCSwitch.h:15`,
  parsed in `TDCStartupOptions.cpp:425-438`), or toggle it from the UI
  (`ToDoListWnd.cpp:14186`).
- Existing timers to read first:
  - `CTDLTaskCtrlBase::RecalcUntrackedColumnWidths`
    (`TDLTaskCtrlBase.cpp:1319`),
  - `CTDLTaskCtrlBase::DoSort` (`TDLTaskCtrlBase.cpp:1833`),
  - `CToDoCtrl::SelectItem` (`ToDoCtrl.cpp:7420`),
  - `CFilteredToDoCtrl::ToggleFilter` (`FilteredToDoCtrl.cpp:428`),
  - `CToDoCtrl::ExpandTasks` (`ToDoCtrl.cpp:9616`).
- Add temporary timers around:
  - `CSimpleSplitter::OnMouseMove` and `ResizePaneWindows`,
  - `CToDoCtrl::OnSplitChange`,
  - `CTabbedToDoCtrl::ReposTaskCtrl`,
  - `CTreeListSyncer::Resize`/`InvalidateAll`,
  - the tree/list `WM_PAINT` handlers (the commented-out timing code in
    `TDLTaskListCtrl.cpp:1075-1094` can be re-enabled in Debug).
- Use `GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS)` before/after a drag to
  detect GDI object churn.
- Use the Visual Studio profiler or an ETW capture (`Microsoft-Windows-Win32k`,
  GDI/USER providers) to attribute CPU time to `WM_PAINT`, `WM_ERASEBKGND`,
  `MoveWindow`, and the splitter handlers.

### 7.3 Acceptance targets

- One splitter drag should not force more than one final repaint of the task view.
- `WM_PAINT` for the task view should be proportional to the visible rows, not to the
  total task count.
- A drag should not allocate a growing number of GDI objects.
- Resizing the window should not call `UpdateWindow()` once per intermediate
  `WM_SIZE`.

---

### 7.4 Automated verification harness

**Implemented:** `customized/CUST-004-ui-perf-harness.md` adds `CUIPerfTest` to
`Core/TDLTest`, opt-in via `TDLTest.exe -ui`. It launches the app with generated
tree/list fixtures (or `-tdl`), drives the splitter drag and list-wheel scroll with
real `SendInput`, and compares on-screen `BitBlt` captures against a forced
repaint to count stale pixels. It also records elapsed time, GDI-object counts and
tree/attribute scroll sync.

First baseline on the unmodified build: the splitter drag reports stale pixels on
every step (`artifactSteps == steps`, `maxDiff` about 59k–78k pixels, `cleanDiff=0`)
— the CUST-003 artefact is reproduced automatically and the clean reference is
stable. List scrolling with the generated flat list is clean (`maxDiff=0..2`,
scroll sync OK); CUST-002 may need the real `biglist.tdl`. `-uifix` (fixtures only)
and `-nodrag` (window discovery only) are available for diagnosing the harness
itself.

Manual drag-and-eyeball testing is not sufficient for this class of change. Three
CUST-003 iterations were built and tested by hand; two regressed repaint behaviour
and one (`WS_EX_COMPOSITED`) broke the attribute pane and scroll synchronisation.
See [customized/CUST-003-splitter-paint-performance.md](customized/CUST-003-splitter-paint-performance.md).
Build a repeatable harness before attempting more P0 changes.

Goals:

1. Reproduce the live drag deterministically.
2. Record timing, paint counts, GDI counts and a pass/fail artefact signal.
3. Run the same scenario before and after a change and compare.

Proposed design:

- **Harness process:** a small Win32/MFC test executable (`TDLPerf`) or an extension
  of `TDLTest` that launches `ToDoList.exe` with a fixed fixture tasklist and a
  fixed window size. Record DPI and theme in the result header.
- **Fixtures:** a large tree fixture and a large list-view fixture (for example
  `biglist.tdl` if present, otherwise generated once and checked in under
  `Core/TDLTest/Data`). Include enough rows/columns that paint cost is measurable.
- **Input:** use `SendInput` (real mouse events), not synthetic `SendMessage`.
  Message starvation and paint coalescing only reproduce with real input. Capture
  the splitter window handle (enumerate children, or expose a debug-only
  `WM_TDL_GETSPLITTER` message) and drive `SetCursorPos` + `LButtonDown/Move/Up`.
  Insert short waits so the target's message loop runs between moves.
- **Frame capture:** during and after the drag, call `PrintWindow`/`BitBlt` on the
  `CToDoCtrl` client into a bitmap. Since `PrintWindow` renders a fresh image, use
  `BitBlt` from the screen DC for the "what the user currently sees" signal.
- **Artefact signal:** after each drag step, capture the on-screen client, then
  force a full repaint (for example `RedrawWindow(..., RDW_INVALIDATE |
  RDW_UPDATENOW | RDW_ALLCHILDREN)`) and capture again. A non-zero pixel diff
  beyond a small tolerance means stale/intermediate pixels were visible. Also
  capture after `LButtonUp` and compare with the at-rest rendering. A second check
  compares tree-list and attribute-list scroll positions.
- **Metrics:** wrap the existing hot paths with counters gated by an environment
  variable such as `TDL_PERF_LOG`:
  - `CSimpleSplitter::OnMouseMove`, `ResizePaneWindows`
  - `CToDoCtrl::OnSplitChange`, `CTabbedToDoCtrl::ReposTaskCtrl`
  - `CTreeListSyncer::Resize`, `InvalidateAll`, `UpdateAll`
  - `WM_PAINT`/`WM_ERASEBKGND` count and duration for the tree/list/header
  - `GetGuiResources(GR_GDIOBJECTS)` before/after
  Emit one machine-readable line per scenario (`key=value`), not free text.
- **Acceptance (from 7.3):** no pixel diff above tolerance at any captured step;
  tree and attribute scroll synchronisation intact; drag wall time and paint count
  recorded and not worse than the recorded baseline; GDI count stable.
- **Baseline protocol:** run the harness on the unmodified tree, store the result
  as the baseline, then run each candidate change and compare both the artefact
  signal and the metrics.

The harness itself is a `customized/` artefact (new `CUST-nnn`) because it adds
non-upstream test code.

---

## 8. Recommended fix sequence

### P0 – artefact and drag responsiveness

0. **Status:** the whole P0 implementation attempt is **deferred and reverted**;
   details in
   [customized/CUST-003-splitter-paint-performance.md](customized/CUST-003-splitter-paint-performance.md).
   Findings:
   - Removing the per-move `UpdateWindow()` starves `WM_PAINT` while the mouse is
     captured, leaving stale scrollbar copies.
   - `WS_EX_COMPOSITED` on `CToDoCtrl` makes the app much slower, hides the
     attribute table and desynchronises tree/attribute scrolling.
   - Erase/clip cleanup alone was never evaluated in isolation because the first
     build also removed the forced repaints.
   Do not retry items 2–6 until the harness in section 7.4 exists. Item 1 (ghost
   tracker) remains intentionally deferred: live pane movement is the user's
   manual check for optimisation progress.

1. **Stop live `MoveWindow` during splitter tracking.**
   Implement classic drag tracking: draw only a ghost/tracker line while the mouse is
   down, then resize panes once on `LButtonUp`. This removes all repeated child repaints
   and is the single biggest win for both artefacts and latency. The change is local to
   `CSimpleSplitter`, but it is a third-party file, so record it under `customized/` as
   a new `CUST-nnn-*` entry per `AGENTS.md`.
2. If live resizing must be kept, wrap the whole move in
   `WM_SETREDRAW(FALSE)`/`BeginDeferWindowPos` and restore with a single invalidation
   of changed strips at the end. Do not call `UpdateWindow()` per mouse move.
3. **Remove the erase from the splitter path.** Add `ON_WM_ERASEBKGND` returning `TRUE`
   (or paint the bar/gap background in `OnPaint`), replace `Invalidate(TRUE)` with
   `Invalidate(FALSE)` and invalidate only the exposed strips and bar rectangles.
4. **Add `WS_CLIPCHILDREN` to the splitter** so its paint/erase cannot touch the
   attribute/comments panes. Consider also making the splitter a small window covering
   only the bars rather than a full-client overlay.
5. **Remove or defer `UpdateWindow()` calls** in `CToDoCtrl::OnSize`,
   `CTabbedToDoCtrl::ReposTaskCtrl`, and `CSimpleSplitter::OnMouseMove`. Let the message
   loop coalesce paints.
6. **Avoid `Invalidate()` with erase in `CTabbedToDoCtrl::OnSplitChange`**
   (`TabbedToDoCtrl.cpp:6808`); invalidate only the affected area or nothing.

### P1 – general UI responsiveness

7. **Cache per-paint/per-revision cell text and row colours.** At minimum, compute
   `GetTaskColumnText` and `GetTaskTextColors` once per visible row per paint instead of
   once per cell/repaint. Longer term, cache by `(task ID, column, data revision)`.
8. **Debounce column-width recalculation.** Keep it in idle processing, coalesce
   repeated requests, and do not drain it synchronously on tab switch
   (`TabbedToDoCtrl.cpp:1118-1120`). Consider recalculation only for visible rows or
   only for the changed column.
9. **Reduce full invalidation on horizontal scroll** (`TDLTaskCtrlBase.cpp:4223-4239`)
   if trailing comments can be handled by invalidating only the affected rows/strips.
10. **Trim `CScopedLogTimer` construction** when logging is disabled (make scope
    formatting lazy), especially in hot paths.

### P2 – longer term

11. Consider an incremental filter/rebuild path (`CFilteredToDoCtrl::RefreshTreeFilter`)
    instead of rebuilding the whole tree, or virtualize the list.
12. If container-level composition is still needed after the above, evaluate
    `WS_EX_COMPOSITED` only on the small splitter/container window, and measure the
    trade-off (it can increase CPU and affect child z-order).

---

## 9. Progress log

- [x] Identified outer and inner splitter implementations and their ownership.
- [x] Traced splitter drag -> layout -> task-view move -> forced repaint chains.
- [x] Confirmed missing `WS_CLIPCHILDREN`/double buffering on the splitter and host.
- [x] Confirmed erase invalidations and `UpdateWindow` calls in the drag path.
- [x] Identified the task-view custom-draw cost and the uncached formatting path.
- [x] Identified column-width recalculation and filter/tab-switch blocking paths.
- [x] Identified the list-view artefact as a two-list sync/stale-title problem rather
      than a missing double buffer (see P0-4).
- [x] Attempted two fixes (item-index task lookup in custom draw and
      `LVN_GETDISPINFO`, plus forced item-rect resolution); both failed manual
      verification and were reverted.
- [ ] Capture ETW/profiler data for the scenarios in section 7.
- [ ] Test the listview scroll/blit hypotheses in CUST-002 (disable double buffering
      on the title list only; force full redraw after the paired scroll; or use a
      single list control).
- [ ] Confirm splitter z-order and update-region overlap with Spy++.
- [x] Attempted the P0-2 to P0-6 changes three times; all iterations were built and
      manually tested and all were reverted. Results in
      [customized/CUST-003-splitter-paint-performance.md](customized/CUST-003-splitter-paint-performance.md).
      Key results: deferring `UpdateWindow()` starves `WM_PAINT` during live drag;
      `WS_EX_COMPOSITED` hides the attribute table, desynchronises scrolling and is
      much slower.
- [x] Build the automated harness described in section 7.4
      ([customized/CUST-004-ui-perf-harness.md](customized/CUST-004-ui-perf-harness.md)).
- [x] Run the harness on the unmodified tree and record the baseline (splitter
      artefacts reproduced; list scroll clean with the synthetic fixture).
- [ ] Re-run the splitter experiments only after the harness passes the baseline.
- [ ] Implement and measure the remaining P0 fixes (ghost tracker + inner splitter).
- [ ] Re-run the measurement matrix and update this document with numbers.

---

## 10. Key file reference

| Area | File |
|---|---|
| Splitter behaviour | `Core/3rdParty/SimpleSplitter.cpp`, `Core/3rdParty/SimpleSplitter.h` |
| Outer layout | `Core/ToDoList/ToDoCtrlLayout.cpp` |
| Host dialog / split change | `Core/ToDoList/ToDoCtrl.cpp` |
| Tabbed host movement | `Core/ToDoList/TabbedToDoCtrl.cpp`, `Core/ToDoList/TDLViewTabControl.cpp` |
| Inner tree/list split and scrolling | `Core/Shared/TreeListSyncer.cpp` |
| Task custom drawing | `Core/ToDoList/TDLTaskCtrlBase.cpp` |
| Tree/list paint entry points | `Core/ToDoList/TDLTaskTreeCtrl.cpp`, `Core/ToDoList/TDLTaskListCtrl.cpp` |
| Column width calculation | `Core/ToDoList/TDLTaskCtrlBase.cpp`, `Core/ToDoList/ToDoCtrlDataUtils.cpp` |
| Filter rebuild | `Core/ToDoList/FilteredToDoCtrl.cpp` |
| Themed row drawing | `Core/Shared/GraphicsMisc.cpp`, `Core/Shared/Themed.cpp` |
| Logging/timers | `Core/Shared/ScopedTimer.cpp`, `Core/Shared/FileMisc.cpp` |
