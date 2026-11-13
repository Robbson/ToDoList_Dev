# CUST-011: MRU tasklist switcher for Ctrl+Tab

Status: **In progress** (implemented and built; manual Ctrl+Tab test pending)

## Goal

Replace the sequential `Ctrl+Tab` / `Ctrl+Shift+Tab` tasklist cycling with an
IntelliJ-style most-recently-used switcher. While `Ctrl+Tab` is held, a popup lists the
open tasklists in last-used order. The previously used tasklist is preselected, so
releasing `Ctrl` immediately switches to it. Repeated `Tab` presses move the selection
through the list; `Shift+Tab` moves back. `Enter` accepts and `Esc` cancels.

## Upstream starting point

- `Ctrl+Tab` and `Ctrl+Shift+Tab` are registered to `ID_VIEW_NEXT` and `ID_VIEW_PREV`
  in `CToDoListWnd::InitShortcuts()` (`Core/ToDoList/ToDoListWnd.cpp`).
- `CToDoListWnd::OnViewNextTasklist()` / `OnViewPrevTasklist()` switch to the adjacent
  tab index, wrapping at the ends.
- Tasklist display names are available through
  `CToDoCtrlMgr::GetTabItemText()` (`Core/ToDoList/ToDoCtrlMgr.cpp`).

## What changed

Files: `Core/ToDoList/ToDoListWnd.h/.cpp`,
`Core/ToDoList/TDLTasklistSwitcher.h/.cpp`,
`Core/ToDoListLib/ToDoListLib.vcxproj`, `Core/ToDoListLib/ToDoListLib.vcxproj.filters`.

1. `CToDoListWnd` keeps an in-memory MRU list of `CFilteredToDoCtrl*`. A tasklist is
   promoted to the front whenever it becomes active, either through
   `SelectToDoCtrl()` or a user tab click handled by `OnTabCtrlSelchange()`.
2. `RefreshMRUToDoCtrls()` drops closed tasklists by comparing pointers with the current
   `CToDoCtrlMgr` list (without dereferencing stale pointers) and appends newly opened
   tasklists that are not yet tracked.
3. `CTDLTasklistSwitcher` is a short-lived owner-drawn popup list. It is centred on the
   monitor containing the main window, owns the keyboard while open, and uses a small
   message loop so it can react to `Tab`/`Shift+Tab`, arrow keys, `Enter`, `Esc` and the
   release of `Ctrl`. A 50 ms timer guards against a missed `Ctrl` key-up. Items are
   drawn blue when modified and grey when not yet loaded; the popup remains visible and
   disabled while a selected unloaded tasklist is being opened.
4. `OnViewNextTasklist()` / `OnViewPrevTasklist()` open the switcher when `Ctrl` is
   physically down. Menu/toolbar invocations without `Ctrl` keep the original
   adjacent-tab behaviour.
5. `HandleEscapeTabReturn()` no longer consumes `Ctrl+Tab` while the Quick Find combo has
   focus, so the global switcher shortcut works from there too.

## Fixes found during interactive testing

- The initial owner-drawn implementation let `WM_MEASUREITEM` fall through to the
  underlying `CListBox`, which asserted in `winctrl1.cpp`. The popup now handles
  `WM_MEASUREITEM` itself and supplies the row height.

## Behaviour

- Pressing `Ctrl+Tab` shows the switcher with the previous tasklist selected.
- Releasing `Ctrl` switches to the selected tasklist.
- Pressing `Tab` again advances; `Shift+Tab` retreats. The list wraps.
- `Enter` switches, `Esc` cancels, and clicking an item switches to it.
- Both during startup and after a switch, the active tasklist is the first MRU entry.
- Tasklists with unsaved edits are drawn in the system hotlight (blue) colour. Tasklists
  that have not been lazy-loaded yet are drawn greyed out.
- Selecting a greyed-out tasklist keeps the switcher open and disables it until loading
  (including any password prompt) finishes.

## Risks

- The MRU is per-session only and is not persisted between restarts.
- The "edited" colour uses the same `IsModified()` state as the tab's `*` marker; there
  is no separate edit timestamp.
- The switcher is an owner-drawn listbox popup rather than a native `HMENU`; it
  deliberately avoids a dialog and background dimming.
- Switching via the popup uses the normal `SelectToDoCtrl()` path, so password prompts,
  delayed loading, due-task notification and read-only checks behave as for a tab click.
- A modal nested message loop runs while the switcher is open. It only handles ordinary
  thread messages and reposts `WM_QUIT`; application timers and idle work continue.
- `Alt+Tab` is not intercepted because only `WM_KEYDOWN` (not `WM_SYSKEYDOWN`) is used
  for selection navigation.
- `Ctrl` detection in the command handlers uses the physical key state, so a menu
  invocation while `Ctrl` is held also opens the switcher.

## Verification

- `Core/ToDoListLib/ToDoListLib.vcxproj` builds in `Unicode Debug` (solution build).
- Manual verification pending: open at least three tasklists, switch between them, then
  check quick `Ctrl+Tab` release, repeated `Tab`/`Shift+Tab`, `Enter`, `Esc`, mouse click,
  and that normal menu "Next/Previous TaskList" still cycles sequentially. Also check
  that a modified tasklist is blue, a delay-loaded tasklist is grey, and selecting the
  grey entry keeps the switcher open and disabled until loading completes.
