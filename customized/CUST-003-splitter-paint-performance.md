# CUST-003 — Splitter paint performance (attempted, reverted)

| Field | Value |
|---|---|
| Status | Deferred (all code changes reverted; automated verification needed first) |
| Upstream baseline | ToDoList 9.3.DP7 |
| Scope | Outer splitter, host resize/repaint path, tasklist container |
| Default | No behaviour change; no code retained |
| Related | [index.md](index.md), [../PERFORMANCE.md](../PERFORMANCE.md) |

## Goal

Reduce the splitter-drag artefacts and forced repaint cost described in
`PERFORMANCE.md` (P0-2 to P0-6) **without** stopping live resize, because live pane
movement is the user's manual check for optimisation progress.

## Upstream starting point

- `Core/3rdParty/SimpleSplitter.cpp`
  - `ResizePaneWindows()` erased the whole splitter with `Invalidate(TRUE)`.
  - `OnMouseMove()` forced a synchronous repaint with `UpdateWindow()`.
  - The window was created without `WS_CLIPCHILDREN` and had no
    `WM_ERASEBKGND` handler.
- `Core/ToDoList/ToDoCtrl.cpp` — `OnSize()` called `UpdateWindow()` after every
  resize; `Create()` used `WS_EX_CONTROLPARENT` only.
- `Core/ToDoList/TabbedToDoCtrl.cpp`
  - `ReposTaskCtrl()` called `UpdateWindow()` after moving the task view.
  - `OnSplitChange()` invalidated the whole tabbed control with bErase = TRUE in
    list view.

The splitter is a transparent overlay: it paints only its bars and relies on the
task view's pixels surviving underneath. Any erase of its client blanks that area.

## Experiments and results

All three iterations were built and manually tested; all were reverted.

### 1. Erase/clip cleanup plus deferred `UpdateWindow()` (P0-2 to P0-6)

Changes tried:

- `WM_ERASEBKGND` returns `TRUE`; `WS_CLIPCHILDREN` added.
- `ResizePaneWindows()` invalidated only bar rectangles with `bErase = FALSE`.
- Removed `UpdateWindow()` from `CSimpleSplitter::OnMouseMove`,
  `CToDoCtrl::OnSize` and `CTabbedToDoCtrl::ReposTaskCtrl`.
- `CTabbedToDoCtrl::OnSplitChange` used `Invalidate(FALSE)`.

Result: **worse artefacts**. During a live drag the task view did not repaint at
all. Each intermediate scrollbar position stayed on screen (about one stale copy
per mouse move) when dragging left to right, and a growing blank area appeared in
the other direction.

Cause: while the mouse button is down, `WM_MOUSEMOVE` floods the message queue and
`WM_PAINT` is starved. The splitter's `Invalidate(TRUE)` had been masking this by
erasing the whole overlay, which hid the stale task-view pixels under a background
fill.

### 2. Restore forced updates and force-update the active view

Changes tried:

- Restored all `UpdateWindow()` calls.
- In `ReposTaskCtrl`, additionally called `UpdateAll()` + `UpdateWindow()` on the
  active tree/list view and on the task view host.
- Kept the erase/clip cleanup.

Result: the stale copies were addressed, but the drag still showed intermediate
states. The user's main concern remained: no double buffering, so partial frames
are visible while the panes and task view repaint.

### 3. `WS_EX_COMPOSITED` on the tasklist container

Change tried:

- `CToDoCtrl::Create()` added `WS_EX_COMPOSITED` to compose the container and all
  descendants into complete frames.

Result: **unacceptable regression**. The whole application became much slower, the
attributes table in the task list vanished, and tree/attribute scrolling went
completely out of sync. Reverted immediately.

## Conclusion

- Deferring `UpdateWindow()` while keeping live resize is not viable; it starves
  repaints.
- `WS_EX_COMPOSITED` on the whole `CToDoCtrl` is not viable for this UI: it breaks
  the attribute pane and scroll synchronisation and is much slower.
- The remaining ideas (per-move `WM_SETREDRAW` batching across splitter and task
  view, or limiting composition to a smaller window) cannot be evaluated by
  building and eyeballing the result. A repeatable automated measurement/artefact
  check is required first; see the plan in
  [../PERFORMANCE.md](../PERFORMANCE.md) section 7.4.

## Test notes

- Each iteration compiled successfully with
  `msbuild Core\ToDoList\ToDoList.vcxproj /p:Configuration="Unicode Debug" /p:Platform=Win32`.
- `TDLTest` post-build still reports the pre-existing `CTDCTaskMatcherTest`
  date/locale failures; unrelated.
- No code from this experiment remains in the working tree.
