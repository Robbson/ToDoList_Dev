# CUST-002 — Duplicated rows in List View while scrolling

| Field | Value |
|---|---|
| Status | Deferred (all targeted tree experiments reverted) |
| Upstream baseline | ToDoList 9.3.DP7 |
| Scope | Task List View scrolling/repainting; related Task Tree history |
| Default | No behaviour change; no code retained |
| Related | [index.md](index.md), [../PERFORMANCE.md](../PERFORMANCE.md), [CUST-004](CUST-004-ui-perf-harness.md) |

## Why the Task Tree has its own path

See [ARCHITECTURE.md §4.2.1](../ARCHITECTURE.md) for the comparison of the 6.9.6
single-tree/gutter design and the current split tree + attribute-List design.

The Task Tree is not special because it is a tree. It is special because
ToDoList needs a grid-like layout with a tree in the first column:

- the task title/tree occupies the left column;
- arbitrary attribute columns (dates, allocation, status, percentages, custom
  attributes) follow to the right;
- a movable splitter separates the two, and columns can be shown, hidden,
  reordered, resized, sorted and auto-fitted;
- the same task list can be shown flat (List View) or hierarchical (Task Tree).

A plain `SysTreeView32` can only host a label tree, not arbitrary attribute
columns. 6.9.6 solved this with `COrderedTreeCtrl` + `CNcGutter`: the columns
were painted in the tree's non-client area. 7.x replaced that with a normal
`CTreeCtrl` (`m_tcTasks`) plus a separate attribute `CListCtrl`
(`m_lcColumns`), a header control and a splitter, kept in step by
`CTreeListSyncer`.

That architecture is the source of the scrolling complexity:

- two native windows must scroll together, and each performs its own native
  scroll copy;
- the tree scrolls with `TVS_EX_DOUBLEBUFFER`/`ScrollWindowEx`, the columns list
  with `LVS_EX_DOUBLEBUFFER`/`ListView_Scroll`;
- ToDoList custom-draws the rows, so the control's default invalidation no
  longer repaints the copied band correctly.

So the "own path" is the price of the tree-plus-columns grid, not of the
hierarchy itself. A single ListView with indentation or a full grid/tree control
would avoid the two-window sync, but still has to solve the custom-draw/scroll
copy described above.

## Summary

- **List View:** copied rows occur for both keyboard Page keys and mouse
  wheel. The ListView API has no "change the top row without scrolling"
  operation, so there is no partial fix equivalent to the tree's
  `TreeView_SelectSetFirstVisible`.
- **Task Tree:** using `TreeView_SelectSetFirstVisible` for Page keys reduces
  the artefacts, but breaks the caret/selection behaviour in the current
  two-window design. The correct caret handling could not be restored without
  reintroducing the native scroll (and therefore the copies).
- All code experiments are reverted. CUST-002 is deferred; a real fix requires
  replacing the controls' scroll-copy path (custom scroll/repaint or a
  non-native control), not post-scroll repainting.

## Problem

While scrolling the Task List View, a row can be copied repeatedly down the
viewport until the newly exposed rows finish painting. The title column is
affected most often, but the attribute columns can show the same artefact; it
depends on which pane is scrolled and repainted last.

This is not a task lookup or `lItemlParam` defect. It is the result of how the
list view is scrolled and painted:

- Windows scrolls a ListView by copying existing pixels (`ScrollWindowEx`).
  The band of newly exposed rows is painted afterwards.
- If painting that band takes long enough, the copied rows are visible at their
  new positions before the correct rows overwrite them. The user sees duplicates
  of the row that was at the edge of the old viewport. It is similar to a web
  page that shows placeholder content while more rows load, except that here the
  placeholder is a copy of an already painted row.
- Whether the artefact is noticeable depends on how long the repaint takes and
  on how much else the machine is doing. A fast machine may repaint before the
  user notices; a slow repaint makes the duplicate band clearly visible.
- The List View has always worked this way. The old Task Tree did not: it
  repainted the visible rows in place, so old rows remained until new rows were
  drawn over them. There were no copied duplicates during the transition.

The Task Tree adopted the List View's scroll-by-copy behaviour when
`TVS_EX_DOUBLEBUFFER` was added in the 7.0 cycle. That is why 6.9.6's tree
looked different and why the List View never had the older, gentler behaviour.

### Version boundary

| Version | Observed behaviour |
|---|---|
| 6.9.6 | Task Tree repaints rows in place; no duplicated rows while scrolling |
| 7.0.0 | Task Tree behaves like the List View (scroll blit + later repaint) |
| 7.2.RC9 | List View artefact present |
| 8.x / 9.x | Same behaviour; no regression introduced by the 2019 title refactor |

Consequences for earlier analysis:

- The artefact is not caused by `OnPostPaintTaskTitle` reading
  `nmcd.lItemlParam`. Changing the title lookup (the two reverted attempts) did
  not change it, and 7.2.RC9 already used `GetTaskID(nItem)`.
- The artefact is not a two-list synchronisation race in the task-ID sense.
  Both panes can show it independently. `CTreeListSyncer` only decides which
  pane is scrolled and when the other catches up.
- 7.3.A1–A8 and 8.0 are the same lineage (7.3.A8 was renamed 8.0.A8 on
  2020-01-30), so comparing those builds cannot isolate the change.

### Source-level change in the Task Tree

The 6.9.6 source confirms the exact change. Both versions set list views up in
`CTreeListSyncer`, but only the newer version enables a **tree** double buffer:

| Source | List views | Tree views |
|---|---|---|
| 6.9.6 `Shared/TreeListSyncer.cpp` | `LVS_EX_DOUBLEBUFFER` | no `TVS_EX_DOUBLEBUFFER` |
| 7.0.998.9 / current `Core/Shared/TreeListSyncer.cpp` | `LVS_EX_DOUBLEBUFFER` | `TVS_EX_DOUBLEBUFFER` |

`TVS_EX_DOUBLEBUFFER` makes the tree scroll by copying its backing bitmap. The
newly exposed rows are painted later, which produces the same copied-row
artefact as the List View. Without it, 6.9.6 lets the tree repaint the exposed
rows in place, so the old rows stay visible until they are overwritten. That is
the behaviour the user remembers.

The earlier candidate, `618e0bd56` (2016-09-28, "Fix horizontal scroll
artifacts"), only removed horizontal-scroll workarounds and is not the cause of
the vertical scroll change.

## Who performs the scroll copy

It is not a TodoList smooth-scroll animation. The visible movement is done by
the Win32 common control itself (`SysListView32` / `SysTreeView32`):

1. The control handles `WM_VSCROLL` or the Page key and calls `ScrollWindowEx`
   (or the double-buffered equivalent) on its client area. That call copies the
   existing pixels to the new position. This is why the row that was at the
   bottom ("Spring") leaves a track as it moves up/down, while the row at the
   top ("Maven") appears to stay put.
2. The control invalidates only the newly exposed band.
3. `WM_PAINT` then reaches the application. Our `NM_CUSTOMDRAW` handlers paint
   the rows in that band, but the copied pixels are already on screen. The
   control may also erase the band with `WM_ERASEBKGND`, which is the white
   patch where a grid/background used to be.

This is not a sub-pixel smooth-scroll animation. For Page keys the copy is
normally row-aligned (a multiple of the item height). Multiple copies of the
same row appear when several scroll messages are processed before the exposed
band is repainted: each message shifts a buffer that already contains copied
rows, so the stale text is repeated.

Normal Windows applications do not show this. The copy is harmless when the
control's **default paint path** repaints the invalidated band, which is what
Explorer-like lists and ordinary ListView/TreeView apps use. ToDoList is
different: the attribute columns are fully custom-drawn with `CDRF_SKIPDEFAULT`,
and the title/tree rows are custom-drawn through prepaint/postpaint. Combining
that custom drawing with `LVS_EX_DOUBLEBUFFER`/`TVS_EX_DOUBLEBUFFER` is what
leaves parts of the copied band unpainted. It is a custom-draw/scroll
invalidation defect, not a limitation that every Win32 application suffers
from. Total Commander and similar tools either use their own list control or
the default paint path; modern Explorer uses DirectUI rather than
`SysListView32`.

`WM_SETREDRAW`/`LockWindowUpdate` do not reliably prevent the internal
`ScrollWindowEx` copy. There is also no `LVM_SCROLL`/`TVM_SCROLL` variant that
moves the top row without copying pixels.

So there are two valid layers for a fix:

- Ensure the scrolled client is **fully repainted from the model** after the
  scroll, or stop combining the custom-draw row painting with the control's
  double buffering. This is the layer where normal applications differ. The
  naive attempts at full repaint traded the copies for erase flicker, so the
  exact invalidate/paint order still needs work.
- Avoid the copy entirely by owning the top-row index and painting the visible
  range yourself (a custom row-drawing control). A single *surface* only makes
  the tree and columns paint together; it does not stop `SysTreeView32` from
  copying rows inside that surface.

## Reproducing

The [CUST-004 harness](CUST-004-ui-perf-harness.md) drives the real UI and can
record the transition at 20 or 60 fps:

```bat
:: real sample, 20 fps JPEG recording, cropped to the list windows
Core\TDLTest\Unicode_Debug\TDLTest.exe -ui -uiscroll ^
  -tdl ..\..\..\..\ToDoList_Test\Tasklists\Commercial Construction.tdl ^
  -size 800x600 -capture task -keys -record -recms 50 -steps 20 -delay 150

:: fast-scroll check using the generated tasklist
Core\TDLTest\Unicode_Debug\TDLTest.exe -ui -uiscroll -notches 2 -delay 0 -record

:: drive an older ToDoList build directly
Core\TDLTest\Unicode_Debug\TDLTest.exe -ui -uiscroll ^
  -tdl ...\Commercial Construction.tdl -size 800x600 -capture task ^
  -keys -record -recms 50 -steps 20 -delay 150 -exe "C:\path\ToDoList.exe"
```

Recorded frames are written to `%TEMP%\TDLPerf\ListScroll_<timestamp>\`.
At 60 fps the duplicated rows typically persist for one frame (about 16 ms);
the next frame is correct. On a slower repaint the same copy is visible for
longer, which is when it becomes noticeable.

## Harness findings (2026-11-12)

- The repeated row is a **copy of painted pixels**. A temporary magenta marker
  drawn in `OnPostPaintTaskTitle` appeared on the repeated rows as well. That is
  consistent with a copied row and does not prove a stale task ID.
- The same artefact can appear in either pane. Reference frame
  `ListScroll_20261112_034949/frame_00004_...jpg` showed coherent titles and a
  repeated ID/value block in the attribute pane; later runs showed the title
  pane repeating one title.
- Disabling `LVS_EX_DOUBLEBUFFER` replaced the duplicates with a blank pane
  during the transition (visible flicker), not with the old in-place overdraw.
- `WM_SETREDRAW(FALSE)` around the paired scroll plus a parent
  `RedrawWindow(RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN)` removed the
  settled `LVM_GETTOPINDEX` mismatch (`syncErrors=0`), but a 20/60 fps recording
  still caught partial frames.
- `LockWindowUpdate(parent)` delayed the resync and did not remove the copied
  rows. `WS_EX_COMPOSITED` on the parent made the attribute columns render
  blank intermittently and is not usable as-is.
- `RDW_ERASE` after the scroll was also tested. It did not remove the copied
  band: the scroll blit happens inside the ListView's scroll handling, before
  the invalidation/erase. One run also failed to advance the top index at all
  (`top=0`), so redrawing inside the scroll message can interfere with the
  ListView; a rerun advanced and still showed duplicated rows.

## Attempted Task Tree fixes (reverted)

All three attempts were built and tested manually; none produced an acceptable
result, so no code from them remains in the working tree.

1. **Match 6.9.6 exactly: remove `TVS_EX_DOUBLEBUFFER` from tree views**
   (`CTreeListSyncer::InitialiseStyles`). Result: the copied-row behaviour
   changed, but every Page key or wheel turn produced a short white flash, even
   when already at one end. The tree erased its background before painting the
   scrolled band.
2. **Keep no double buffer and suppress the flash**: return `TRUE` for the
   tree's `WM_ERASEBKGND` and fill the empty area below the last item in
   `CDDS_POSTPAINT`. Result: the flash was gone, but the copied-row artefact
   returned for Page Up/Down.
3. **Overwrite the copied rows after scrolling**: add
   `CTreeListSyncer::RepaintScrolledPair` to invalidate and update both the tree
   and its columns list after a scroll. Result: both the artefact and the
   flicker were present together.

A dynamic `TVS_EX_DOUBLEBUFFER` toggle (off for Page keys, on for wheel) was
considered but not implemented. It would only choose which symptom appears:
`TVS_EX_DOUBLEBUFFER` is a window style and changing it forces a repaint, the
scroll may paint after the style is restored, and without the buffer the Page
key scroll still copies rows unless it uses `TreeView_SelectSetFirstVisible`.

### Failed experiment: native key handling + full tree repaint (reverted)

Tried and reverted:

- Page Up/Down and line keys used the native tree handling exactly, preserving
  the caret/selection behaviour.
- After a tree Page key the whole tree client was invalidated and updated, with
  `TVS_EX_DOUBLEBUFFER` still enabled.

Result: the caret worked correctly, but the artefact frequency was unchanged
from the original. The repaint is issued after `ScDefault` has already scrolled
and blitted the copied rows, so it can only correct the final state. It cannot
hide the copies that are on screen during the scroll; a full custom-draw repaint
of the visible rows also takes long enough for that interval to remain visible.
The attribute columns were not repainted in this version.

Earlier variants:

- Inhibiting the native scroll (`SelectSetFirstVisible` only) made trees less
  prone to artefacts but changed the caret behaviour.
- Overriding the selection with `CTreeSelectionHelper` made the caret snap to
  first/last and stay there.
- Fully repainting the attribute columns list made the attribute-side artefacts
  worse (full-row artifacts).

No code from these experiments is retained. The conclusion is that the current
architecture (separate TreeView and columns ListView, both app-custom-drawn, with
the syncer scrolling them separately)
cannot reproduce the 6.9.6 behaviour with these local fixes. 6.9.6 used a single
`COrderedTreeCtrl` whose columns were drawn in a non-client gutter, so the tree
and its columns were rendered as one surface. Removing the double buffer in the
modern split architecture exposes the trade-off directly: with double buffering
the copied rows are visible, without it the erase flash is visible, and forcing
a full repaint lets the capture/eye see both transitions.

## Why the old Task Tree was different

6.9.6 did not enable `TVS_EX_DOUBLEBUFFER`. Without a backing bitmap, the tree
paints each exposed row in place, so the old rows stay on screen until they are
overwritten. With `TVS_EX_DOUBLEBUFFER`, the tree blits the old bitmap first and
paints the new rows later, producing the copied-row artefact.

The ListView has no equivalent "repaint without double buffer" switch that fixes
this while also giving flicker-free scrolling; disabling `LVS_EX_DOUBLEBUFFER`
there produces blank/flicker instead. This is why the list needs a different fix
from the tree.

6.9.6 also used a single tree control whose attribute columns were drawn in a
non-client gutter, so it did not have the modern separate columns ListView. The
double-buffer difference and the single-surface architecture therefore both
contribute; removing `TVS_EX_DOUBLEBUFFER` alone cannot reproduce 6.9.6.

## Recommended fix direction

The local tree fixes above were tried and reverted. The next attempt should
start at the layer that normal applications get right:

1. **Fix the custom-draw/scroll invalidation.** Make the custom-drawn rows
   repaint the entire scrolled client from the model, or stop combining
   `CDRF_SKIPDEFAULT`/postpaint row drawing with `LVS_EX_DOUBLEBUFFER`/
   `TVS_EX_DOUBLEBUFFER`. Compare against a default-painted test list to
   confirm the control itself behaves correctly. The earlier full-repaint
   attempts must be revisited with the correct erase/invalidate order; they
   traded copies for flicker.
2. **Custom list/tree scrolling and rendering.** Keep the controls for input and
   selection, but drive the visible row range yourself and repaint the client in
   place instead of letting the control scroll. This is effectively an
   owner-drawn control with its own top-row index and removes the internal
   copied rows entirely.
3. **Return to a single-surface task view.** 6.9.6 used one `COrderedTreeCtrl`
   with the columns painted in its non-client gutter. Reworking the modern Tree
   and List views along those lines makes the rows and columns part of one
   paint operation and removes the cross-pane mismatch. On its own it does not
   stop `SysTreeView32` from copying rows inside that surface; it would need to
   be combined with option 1 or 2.
4. **Accept and contain.** If none of the above is acceptable, keep the current
   double-buffered behaviour and document that a short copied-row frame can be
   seen at high frame rates or on slow repaints. This does not match 6.9.6.

Do not pursue task-ID/`lItemlParam` or `LVN_GETDISPINFO` changes. The marker
experiment, the symmetric title/attribute failures, the 7.2.RC9 result and the
`RDW_ERASE` result all rule that out. Do not retry any of the local tree fixes
listed in "Attempted Task Tree fixes" without a new tree-scroll test; each was
tried, built and rejected.

### Implementation complexity estimate (custom scroll)

Can we improve `SysListView32`/`SysTreeView32` itself? Not by subclassing or
patching. The copy-on-scroll logic is internal to ComCtl32; there is no public
hook between "compute the new scroll position" and `ScrollWindowEx`. Custom
draw/owner draw only controls item painting, not the scroll copy. The workable
interpretation of "our own improved control" is writing a **replacement**
control that keeps the needed ToDoList behaviour but paints rows from the model
and owns the top-row index. That is the custom-control option estimated below,
not a small patch. It does not have to implement the whole Win32 API, only the
subset ToDoList uses, but that subset is large because of selection, editing,
drag/drop and the Task Tree.

These are rough estimates for one experienced C++/MFC developer familiar with
the codebase. They assume the existing task model, selection helpers, data
layer and preferences stay in place.

| Approach | Scope | Estimate | Risk |
|---|---|---|---|
| Coalesce scroll input + one repaint | Batch wheel/Page input, suppress intermediate scrolls, repaint once | 1–3 days | Low; reduces multiple copies but does not remove the copy |
| Custom row-painting List View | Own top-row index, paint rows from the model, replace viewport scroll; keep header, selection, inline edit, drag/drop, tooltips, DPI | 6–10 weeks | High |
| Custom List View + Task Tree | Add hierarchy, expand/collapse, gutter/columns, multi-select, drag/drop, tree keyboard semantics | 3–6 months total | Very high |
| Third-party grid/tree control | Integrate a commercial/OSS grid or tree with retained-mode rendering | 3–8 weeks plus licensing and feature gaps | Medium–high |

Why the estimate is large:

- The controls are not self-contained. They interact with
  `CToDoCtrlData`, `CTreeSelectionHelper`, `CTreeCtrlHelper`,
  `CTreeListSyncer`, filters, sorting, grouping, inline label editing,
  drag/drop, tooltips, theming, high DPI, RTL, image export and the
  attribute columns.
- A custom scroll cannot be added as a thin layer over `SysListView32`/
  `SysTreeView32` because the controls always own their scroll position. The
  visible range, hit-testing and selection have to be reimplemented together.
- Both the List View (flat) and Task Tree (hierarchical) need parity; the Task
  Tree adds the hierarchy, collapse/expand and tree-specific keyboard
  navigation.
- Regression risk is high: the views are central to the application and are
  covered only partly by automated tests. A tree/List scroll scenario in
  CUST-004 is a prerequisite for measuring progress.

A reasonable sequence would be:

1. Add a tree-scroll and ListView-wheel scenario to CUST-004 (1–2 days).
2. Prototype a custom-scrolled flat List View with selection and keyboard only
   (1–2 weeks), measure performance and visual result.
3. Decide whether to continue to full parity or stop after the prototype.

### Acceptance criteria

- The 60 fps sequence shows no frame in which a row is duplicated down the
  viewport. A partially painted frame is acceptable if it shows the old rows
  being overdrawn, as in 6.9.6, rather than copies.
- `LVM_GETTOPINDEX` for the title and attribute lists match after each scroll
  (`syncErrors=0`).
- Settled pixel check reports `artifactSteps=0`.
- Existing List View/Task Tree selection, inline edit, drag/drop and splitter
  behaviour are unchanged; run the normal `TDLTest` suite.

## Files involved

No code is retained. These files were touched during the investigation and all
were reverted:

- `Core/Shared/TreeListSyncer.h`
- `Core/Shared/TreeListSyncer.cpp`
- `Core/ToDoList/TDLTaskTreeCtrl.h`
- `Core/ToDoList/TDLTaskTreeCtrl.cpp`
- `Core/ToDoList/TDLTaskCtrlBase.h`
- `Core/ToDoList/TDLTaskCtrlBase.cpp`
- `Core/ToDoList/TDLTaskListCtrl.h`
- `Core/ToDoList/TDLTaskListCtrl.cpp`

## Verification

- The reverted tree builds (`Unicode Debug|Win32`).
- CUST-004 `ListScrollSync` (Page Down, real sample) still passes:
  `syncErrors=0`, `maxDiff=0`, `artifactSteps=0`.
- `TDLTest`: 310240/310264 pass; the 24 failures are the pre-existing
  `CTDCTaskMatcherTest::IndividualRules` failures already recorded in CUST-001.
- Tree Page Up/Down was validated manually only. There is no automated
  tree-scroll scenario in CUST-004 yet; adding one is a prerequisite for any
  further attempt.
