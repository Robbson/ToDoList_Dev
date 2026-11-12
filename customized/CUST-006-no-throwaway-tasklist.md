# CUST-006: No throwaway tasklist at startup

Status: **Done**

## Goal

For anyone with saved documents, startup built a complete tasklist window that was
destroyed again as soon as the first real document opened. It was visible as an
"untitled" list with a sample task, then replaced. It cost about one full tasklist
window construction, which after CUST-005 became the largest single startup item.

The placeholder only has a purpose when there is nothing to restore.

## Upstream starting point

- `CToDoListWnd::OnCreate` called `CreateNewTaskList(FALSE, FALSE)` unconditionally,
  before the restore loop, "while we're still hidden" (`ToDoListWnd.cpp`).
- `CToDoListWnd::OnPostOnCreate` shows the frame (`RestoreVisibility`), opens or
  delay-opens each `[Settings] LastFile<n>`, then calls
  `CheckRemovePristineTasklist()`, which deletes the placeholder as soon as a real
  document was selected.

Measured cost of the discarded tasklist (Release, `-g`):

| Item | Time |
|---|---|
| `CreateToDoCtrl` (window) | 484 ms |
| `UpdateToDoCtrlPreferences` | 610 ms |
| `AddToDoCtrl` (select/show) | 344 ms |
| Total | ~1.1–1.5 s |

## What changed

Files: `Core/ToDoList/ToDoListWnd.h`, `Core/ToDoList/ToDoListWnd.cpp`,
`Core/ToDoList/ToDoCtrlMgr.cpp`.

1. `CToDoListWnd::WillRestoreLastSession()` decides whether startup will open a
   document: true when the command line has a file path, or when
   `[Preferences] ReloadTasklists` is on and `[Settings] NumLastFiles > 0`, and not
   `-e`.
2. `OnCreate` creates the initial tasklist only when `WillRestoreLastSession()` is
   false.
3. `OnPostOnCreate` creates it as a fallback *after* the restore attempt if nothing
   was opened, so the "empty new document + sample task" behaviour is preserved for
   `-e`, no session, and reload-disabled starts.
4. `RestoreVisibility(BOOL bShow = TRUE)` now restores position and decides
   visibility but can defer the actual `ShowWindow`. A new `ShowStartupWindow()`
   shows the frame only once there is at least one tasklist, and is called after each
   successful open and after the fallback. The window is therefore never shown in the
   irrelevant empty state that the placeholder used to mask.
5. A zero-tasklist frame is now a legitimate transient state, so the code that
   assumed otherwise was hardened:
   - `CHECKVALIDINDEX`/`CHECKVALIDINDEXRET` in `ToDoCtrlMgr.cpp` only assert when the
     manager is non-empty, so `GetToDoCtrl()` etc. return their documented fallbacks
     during startup.
   - `CToDoListWnd::OnCmdMsg` ignores `CN_UPDATE_COMMAND_UI` while there are no
     tasklists. MFC sends `WM_IDLEUPDATECMDUI` to toolbars even while the frame is
     hidden, and those handlers reached into the (window-less) fallback control.
   - `IDLETASKS::Process` returns early when there are no tasklists.
   - `UpdateTimeTrackerTasks` returns early when nothing is selected.
   - `OpenTaskList`'s "re-add the control if it could not be loaded and no other
     tasklist exists" path now registers that control with the time tracker, which it
     never needed to do while the placeholder always existed.

## Results

Release, poll until all tabs exist (two runs each), same temp ini:

| Documents | Before | After |
|---|---|---|
| 3 × `Evidence Board.tdl` | 5.16 / 5.28 s | **3.56 / 3.38 s** |
| 12 × `Evidence Board.tdl` | 5.00 / 5.05 s | **3.71 / 3.46 s** |

Savings ≈1.5–1.8 s per start, independent of document count (it is one window).
Frame becomes visible at ~0.2 s and content follows as soon as the first tasklist is
ready, instead of showing and then replacing the placeholder.

## Behaviour preserved

- `-e` (start empty): one empty "untitled" tasklist, no sample task.
- No saved session: one "untitled" tasklist with the sample task.
- Reload disabled: same as no session, saved documents are not opened.
- Command-line file: that document opens; no placeholder.
- Session with a missing file: the failed document keeps a tab (upstream behaviour)
  and the time tracker now knows about it.
- Session where every file is missing: one empty "untitled" tasklist.
- Start hidden / to tray: the deferred show respects `m_bVisible` and never shows.

## Verification

- Debug and Release, six startup variants (3 documents, 3 + one missing, no session,
  `-e`, reload disabled, `-f` command line): expected tabs and title for each, clean
  shutdown (exit code 0), and **zero new entries** in `%TEMP%\ToDoList_asserts.log`.
- Release `TDLTest.exe`: 310937/310961, identical to before (the 24 failures are the
  pre-existing `CTDCTaskMatcherTest` locale failures).

## Risks

- The app can now briefly have zero tasklists, before the first one is created. All
  paths that reach into a tasklist during that window were audited for the startup
  cases above; new code should check `GetTDCCount()`/`GetSelToDoCtrl()` before using
  `GetToDoCtrl()` during startup.
- Modal dialogs raised by the restore loop (for example a password prompt) now have a
  hidden owner because the frame is shown only after the first tasklist exists. They
  still appear and behave normally, but are not parented to a visible window.
- `OnCmdMsg` suppresses command-UI updates (not commands) while empty; toolbar button
  states during that brief hidden interval are simply left untouched.