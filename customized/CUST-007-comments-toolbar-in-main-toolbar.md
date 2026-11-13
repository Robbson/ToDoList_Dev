# CUST-007: Comments editor controls in the main toolbar

Status: **Done**

## Goal

The comments editor kept its own row above the edit field: a "Comments" label, the
content-format combo, and a three-button date/time toolbar. That row consumes vertical
space in the task view. Move all of those controls to the application main toolbar and
let the comments field fill the reclaimed row. Keep the format combo available because
additional content plugins (for example Markdown) may be installed later.

## Upstream starting point

- `CTDLCommentsCtrl` builds the label, `CContentTypeComboBox m_cbCommentsFmt` and a
  toolbar loaded from `IDR_COMMENTS_DATETIME_TOOLBAR`
  (`Core/ToDoList/TDLCommentsCtrl.cpp`).
- `CToDoCtrl` constructed it with `bShowLabel = TRUE`, `bShowToolbar = TRUE`
  (`Core/ToDoList/ToDoCtrl.cpp`).
- `CalcCommentsCtrlRect()` started the content control below the combo row.
- The frame already handled the date/time commands for the active task view
  (`CToDoListWnd::OnEditInsertdate` etc. and `ID_COMMENTS_INSERT*`).

## What changed

Files: `Core/ToDoList/TDLCommentsCtrl.h/.cpp`, `Core/ToDoList/ToDoCtrl.h/.cpp`,
`Core/ToDoList/ToDoListWnd.h/.cpp`, `Core/ToDoList/ToDoList.rc`,
`Core/ToDoList/res/app_toolbar*.bmp`.

1. `CTDLCommentsCtrl` gained a `bShowFormatCombo` constructor option. When false it
   hides the combo but keeps it as the internal format state used by
   `GetSelectedFormat`/`SetSelectedFormat`.
2. `CalcCommentsCtrlRect()` starts the editor at the top of the pane when the combo is
   hidden, so the whole row is reclaimed.
3. `CToDoCtrl` now creates the comments control with no label, no pane toolbar and no
   visible format combo.
4. `CTDLCommentsCtrl::ChangeSelectedFormat()` and `CToDoCtrl::SetCommentsFormat()`
   provide a path to change the format from outside the pane. It reuses the existing
   `OnSelchangeCommentsformat` content-preservation path and notifies `CToDoCtrl`
   exactly as a user selection did.
5. `CToDoCtrl::GetCommentsFormat()` and `CanChangeCommentsFormat()` expose the current
   format and edit state.
6. `CToDoListWnd` owns a `CContentTypeComboBox` that is created as a child of the main
   toolbar, next to the three date/time buttons, following the existing Quick Find combo
   pattern. It is synchronised on task selection, task-view change, toolbar show and
   toolbar command-UI updates.
7. `IDR_APP_TOOLBAR` gained the three `ID_COMMENTS_INSERTDATE/TIME/DATETIME` buttons and
   `app_toolbar_std.bmp` gained the matching images (appended so the existing 22 button
   images keep their indices). `app_toolbar.bmp` is intentionally left unchanged: MFC's
   `CToolBar::LoadToolBar` loads it with `AfxLoadSysColorBitmap`, which requires the
   original 16-colour bitmap, and `CEnToolBar::SetImage` immediately replaces it with the
   standard image at runtime.

## Behaviour

- The comments editor no longer shows the label, format combo or date/time buttons.
- The main toolbar shows Insert Date, Insert Time, Insert Date and Time, then the
  comments content-format combo.
- The date/time buttons remain enabled only when the comments field has focus, matching
  the previous pane toolbar.
- The format combo follows the active task view's selected task(s). A mixed selection
  shows an empty combo; choosing a format applies it to all selected tasks and preserves
  content using the existing conversion path.
- The combo is disabled for read-only or locked selections.
- Existing keyboard shortcuts and menu commands are unchanged.

## Fixes found during interactive testing

- `CContentTypeComboBox::GetSelectedFormat()` returns an item index or `CB_ERR`, not a
  `BOOL`. The first idle implementation used it as a boolean, so index 0 (`Simple Text`)
  looked like failure and the combo was reset on every command-UI update, causing the
  visible flicker. Callers now compare against `CB_ERR` explicitly.
- The same mistake in `OnSelChangeCommentsFormat()` meant selecting the first item
  (`Simple Text`, index 0) did not call `SetCommentsFormat()` at all, so the next update
  restored the previous (Rich Text) selection. Fixed by the same explicit comparison.
- `CToolBar::LoadToolBar()` loads the normal toolbar bitmap with
  `AfxLoadSysColorBitmap`, which does not handle a 24-bit replacement. Converting
  `app_toolbar.bmp` produced the `bartool.cpp:433` assertion at startup. The normal
  bitmap is left as the original 16-colour image; only the standard bitmap is extended.

## Risks

- The main toolbar can be hidden; in that case the date/time controls are only reachable
  through their existing shortcuts and Edit menu items, and the format combo is hidden
  with the toolbar.
- Any code that assumed `CTDLCommentsCtrl` always had a visible format combo must use
  the new `GetSelectedFormat`/`SetSelectedFormat` API instead of looking at the control.
- The toolbar bitmaps: only `app_toolbar_std.bmp` is 400×16; `app_toolbar.bmp` remains
  352×15. Adding future main-toolbar buttons still requires appending images in the same
  order, and the normal bitmap must remain loadable by `AfxLoadSysColorBitmap`.
- The comments label is gone; the edit field is identified by its position and border.

## Verification

- `msbuild Core\ToDoList_Core.sln /t:Build /p:Configuration="Unicode Debug"
  /p:Platform=Win32 /m` succeeds.
- Startup with `TDL_ASSERT_LOG=1` produces no assertions after restoring the original
  `app_toolbar.bmp` (the previous 24-bit version made
  `CToolBar::LoadToolBar` assert at `bartool.cpp:433`).
- Format switching was exercised through the toolbar combo (`Simple Text` ↔ `Rich Text`,
  repeatedly, including selecting the first item): the selection stays where the user
  put it and no assertions are logged.
- Release `TDLTest.exe` was not rerun for this change; the Debug run shows the
  pre-existing 24 `CTDCTaskMatcherTest` locale failures and no new compile warnings in
  the touched files.
