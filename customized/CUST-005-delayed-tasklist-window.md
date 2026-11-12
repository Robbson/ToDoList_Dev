# CUST-005: Deferred window creation for delay-loaded tasklists

Status: **Done**

## Goal

Multi-document startup was dominated by per-document window construction, not by
reading task data. With 12 restored documents the app spent ~1.4 s per document
building a `CFilteredToDoCtrl` window that the user had not asked to see yet. Opening
the program with 10+ documents felt like a freeze.

`[Preferences] EnableDelayedLoading` already deferred the *task data* for every
non-active document, but it still created the full window (task tree, attribute grid,
comments control, splitters) and laid it out, so it saved almost nothing.

## Upstream starting point

- Restore loop: `CToDoListWnd::OnPostOnCreate`
  (`Core/ToDoList/ToDoListWnd.cpp`), which calls `OpenTaskList` for the active document
  and `DelayOpenTaskList` for the rest.
- `CToDoListWnd::DelayOpenTaskList` called `NewToDoCtrl(FALSE)` (->
  `CToDoCtrl::Create` -> `OnInitDialog`) and then `CToDoCtrl::DelayLoad`, which parsed
  the whole file (`CTaskFile::LoadHeader` -> `CXmlFile::LoadEx`, including the full
  MSXML DOM) just to read the project name, custom attribute definitions and earliest
  due date.
- `CToDoListWnd::AddToDoCtrl` then calculated the control rect and called
  `MoveWindow` even for the delay-loaded, hidden tab, and `AddToDoCtrl` in
  `CToDoCtrlMgr` inserted a tab, checked checkout/read-only state and updated the tab
  image/text.

Measurements (Release, 12 copies of the bundled samples, win32):

| Scenario | Docs | Time to settle |
|---|---|---|
| `biglist.tdl` (1.7 MB) | 1 | ~4.6 s |
| `biglist.tdl` (1.7 MB) | 12 | 21.6 s (delayed) / 22.2 s (not delayed) |
| `Evidence Board.tdl` (7.7 KB) | 12 | ~22.8 s |
| `biglist.tdl` (1.7 MB) | 12 | ~23.5 s |

12 tiny documents cost the same as 12 large documents (about 0.06 s per document for
20 MB of extra XML), so the cost was window construction, not parsing.

Per delayed document, measured with `-g`:

- `CToCtrl::OnInitDialog` 375-437 ms
  (attribute grid 172-203, task tree 109-125, comments 16-31, update controls 16-46),
- `CToCtrl::Create` total 531-563 ms,
- `InitGlobalStyles` 31-47 ms,
- `UpdateToDoCtrlPreferences` 470-750 ms (also applied to hidden documents),
- `DelayLoad` 15-32 ms (the whole-file DOM parse),
- `AddToDoCtrl` 47-78 ms,
- time tracker 16-31 ms.

## What changed

Files:

- `Core/ToDoList/ToDoListWnd.h`
- `Core/ToDoList/ToDoListWnd.cpp`
- `Core/ToDoList/ToDoCtrlMgr.cpp`

Design: a delay-loaded tab now owns the `CFilteredToDoCtrl` **object** but no window.
The object's constructor only initialises members and registers the dialog template
controls, so all the data accessors used by the tab bar, the manager, the time tracker
and the reminder code keep working.

1. `NewToDoCtrl` was split into:
   - `AllocToDoCtrl()` - constructs `CFilteredToDoCtrl` only.
   - `CreateToDoCtrl(pTDC, bEnabled, bSkipPrefs)` - computes the rect, calls
     `Create`, then applies font, global styles, theme and (optionally) the runtime
     preferences. `NewToDoCtrl` is the composition of both, so all existing callers
     behave exactly as before.
2. `DelayOpenTaskList` now calls `AllocToDoCtrl()` instead of `NewToDoCtrl(FALSE)` and
   guards the failure-path `DestroyWindow()`.
3. `VerifyTaskListOpen` calls the new `EnsureToDoCtrlWindow(&tdc)` before loading, which
   creates and sizes the window on first activation.
4. `AddToDoCtrl` skips `CalcToDoCtrlRect`/`MoveWindow` and `ShowWindow` for a
   delay-loaded tab that has no window yet.
5. `OpenTaskList` applies `UpdateToDoCtrlPreferences` for tabs that were delay-loaded,
   preserving the original order (preferences before task data) but moving the cost to
   first activation.
6. `CToDoCtrlMgr::DeleteToDoCtrl` only calls `DestroyWindow()` when there is a window,
   so unactivated tabs can be closed.
7. `CToDoListWnd::UpdateUITheme` skips window-less tasklists; they receive the current
   theme when they are created.

Location-level `CScopedLogTimer`s were added to `DelayOpenTaskList`, `AddToDoCtrl`,
`NewToDoCtrl`/`CreateToDoCtrl` and `EnsureToDoCtrlWindow`, following the existing
"PERMANENT LOGGING" convention. They are gated on `FileMisc::IsLoggingEnabled()`.

## Results

Release, same fixtures and ini as above, timing until the log stopped growing for 4 s
(the 4 s is constant and subtracted):

| Scenario | Docs | Before | After |
|---|---|---|---|
| `Evidence Board.tdl` | 1 | 7.7 s | 7.8 s |
| | 2 | 9.3 s | - |
| | 4 | 12.0 s | - |
| | 12 | 26.4 s | **8.7 s** |
| `biglist.tdl` (1.7 MB) | 12 | 27.1 s | **9.3 s** |

Marginal cost per restored document fell from ~1.5 s to ~85 ms. Startup with 12
documents is roughly 3x faster and now scales almost flat with document count.

The single active document is unchanged (it is loaded normally), so the fixed
startup floor is now the dominant term for a 10-document configuration. The floor was
subsequently measured: the content and UI-extension plugin scans together cost only
~0.14 s, while the "pristine" tasklist that `OnCreate` builds and then discards when a
document is restored costs ~1.1 s. See `PERFORMANCE.md` P0-5 for the follow-up.

## Verification

- Release `TDLTest.exe`: 310937/310961 pass. The 24 failures are the pre-existing
  `CTDCTaskMatcherTest::IndividualRules` locale failures and are unrelated.
- Debug and Release builds: launched with a 6-document ini, walked every tab with
  `ID_VIEW_NEXT`; each tab created its window on first activation and logged
  `CToDoCtrl::Load(<file>)`, titles updated per document, remaining unactivated tabs
  were destroyed on close, process exited cleanly with code 0.
- No new entries in `%TEMP%\ToDoList_asserts.log` from Debug runs (all entries
  predate this change).

## Risks and follow-ups

- Any code that expects a delay-loaded tasklist to have an `HWND` will now see none
  until first activation. The audited paths are the tab bar, `CToDoCtrlMgr`,
  the time tracker, read-only/checkout checks, `SaveAll` (skips unmodified and
  delay-loaded tabs), periodic status timers (already guard on `IsLoaded`) and
  `DeleteToDoCtrl`. New code must use `GetSafeHwnd()` or `IsLoaded()` before touching
  a tasklist window.
- `DelayLoad` still reads and DOM-parses the entire file to obtain the project name,
  custom attributes and earliest due date. That is now only ~15-32 ms per document
  but remains a candidate for a header-prefix read.
- The runtime preferences are now applied at first activation rather than at startup.
  This preserves the observable result for hidden tabs but means a preference change
  made while a tab is still unactivated is applied from the new value (which is what
  the user expects).