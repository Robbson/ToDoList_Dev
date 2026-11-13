# CUST-008: Hide the mouse pointer while typing in text controls

Status: **Done**

## Goal

Restore the old application behaviour: while typing, the mouse pointer disappears and
only reappears when the mouse is next moved. This should apply regardless of where the
pointer happens to be (over the editor, the toolbar or the frame), as it does in the
standard Search/Tags/Title edit fields.

## Why the standard fields differ

The Search, File link, Tags, Category and Title fields are standard Windows `EDIT`
controls. Windows implements the "Hide pointer while typing" mouse option inside the
common edit control, so those fields work automatically.

The comments editors are custom RichEdit controls:

- simple text: `CTDLSimpleTextContentCtrl`;
- rich text: `CRulerRichEdit` in the RTFContentCtrl plugin.

RichEdit does not implement the mouse-vanish option and re-sets the I-beam cursor on
`WM_SETCURSOR` (and the URL link handler can call `SetCursor()` directly). Hiding the
pointer inside the RichEdit control only worked while the pointer was over that control;
as soon as it was over the toolbar or frame, those windows' `WM_SETCURSOR` handlers showed
it again, producing the flicker.

## What changed

File: `Core/ToDoList/ToDoListWnd.h/.cpp` (the earlier `CUrlRichEditCtrl` attempt was
reverted).

1. `PreTranslateMessage()` watches all queued messages for the application. On
   `WM_CHAR`, Backspace, Delete or Return it checks whether the target is an edit control
   (`CWinClasses::IsEditControl`) and calls `HideMouseCursorWhileTyping()`.
2. `HideMouseCursorWhileTyping()` records the cursor position and drives
   `ShowCursor(FALSE)` until the display counter is negative, hiding the pointer globally
   regardless of which window is underneath. It starts `TIMER_HIDEMOUSECURSOR`.
3. The timer (50 ms) and any `WM_MOUSEMOVE` call `RestoreMouseCursorIfMoved()`. Once the
   pointer has moved more than 2 pixels it drives `ShowCursor(TRUE)` until the counter is
   non-negative and stops the timer, so the normal cursor shape is restored by the usual
   `WM_SETCURSOR` handling.
4. `OnDestroy()` force-restores the cursor so the desktop cannot be left without a
   pointer.

The 2-pixel threshold ignores hand jitter while typing; a deliberate mouse movement
reveals the pointer immediately (timer latency at most 50 ms, usually the same message
cycle via `WM_MOUSEMOVE`).

## Risks

- `ShowCursor` uses a process-wide display counter. The code balances it on restore and
  on window destruction, but other code that calls `ShowCursor` at the same time could
  interact with it.
- Typing in any edit control in the application (including dialogs) hides the pointer,
  which matches the standard-edit behaviour.
- If the application is terminated abnormally while the pointer is hidden, the display
  counter is reset when the process ends.

## Verification

- `Unicode Debug` builds successfully (`Shared`, `RTFContentCtrl`, `ToDoListLib`,
  `ToDoList`).
- With the pointer deliberately placed over the **main toolbar**:
  - simple-text comments: showing before typing, `GetCursorInfo` handle `0` while
    typing, showing again after a 20-pixel mouse move;
  - rich-text comments: the same result after switching the comments format.
- No assertions logged during startup, typing or mouse movement.
