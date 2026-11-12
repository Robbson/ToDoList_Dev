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

## Files touched in the investigation (now reverted)

- `Core/ToDoList/TDLTaskCtrlBase.h`
- `Core/ToDoList/TDLTaskCtrlBase.cpp`
- `Core/ToDoList/TDLTaskListCtrl.h`
- `Core/ToDoList/TDLTaskListCtrl.cpp`

## Verification

- Both attempted builds completed (`Unicode Debug|Win32` and
  `Unicode Release|Win32`).
- Manual testing confirmed the artefact remains, so the changes were reverted.
- Full `TDLTest` run: 310240/310264 pass; the 24 failures are the pre-existing
  `CTDCTaskMatcherTest::IndividualRules` failures already recorded in CUST-001.
