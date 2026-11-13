# CUST-008: Hide the mouse pointer while typing in the comments editors

Status: **Done**

## Goal

Restore the old comments-editor behaviour: while typing, the mouse pointer disappears and
only reappears when the mouse is next moved. The current build left the I-beam cursor
visible and flickering while text was typed, in both the simple-text and rich-text
comments editors.

## Upstream starting point

Standard Windows `EDIT` controls implement the "Hide pointer while typing" mouse option
internally. The comments editors are custom RichEdit controls instead:

- simple text: `CTDLSimpleTextContentCtrl` derives from `CUrlRichEditCtrl`;
- rich text: `CRulerRichEdit` (RTFContentCtrl plugin) derives from `CUrlRichEditCtrl`.

`CUrlRichEditCtrl` let the RichEdit class restore the I-beam cursor on every
`WM_SETCURSOR`, and the URL link handling could call `SetCursor()` directly. That kept
re-showing the pointer during typing, which the OS "hide pointer while typing" feature
then hid again, producing the visible flicker.

## What changed

Files: `Core/Shared/UrlRichEditCtrl.h/.cpp`.

1. Added `m_bHideMouseCursor` and `HideMouseCursor()`, which records the current cursor
   position and calls `::SetCursor(NULL)`.
2. `OnKeyDown()`, the existing `OnKeyUp()`, and `OnChangeText()` (when the control has
   focus) call `HideMouseCursor()`.
3. `OnSetCursor()` checks the flag first and, while it is set, calls `::SetCursor(NULL)`
   and returns `TRUE`, so neither the RichEdit class cursor nor the link notification
   can restore the I-beam.
4. `OnMouseMove()` ignores movements of up to 2 pixels (hand jitter while typing). Once
   the pointer really moves it clears the flag and explicitly restores the I-beam; normal
   `WM_SETCURSOR` handling then takes over again (including link cursors).

Because the change is in the shared base class, both the simple-text and rich-text
comments editors behave the same way.

## Risks

- The pointer is hidden globally (not just over the editor) until the next significant
  mouse move, so keyboard-only users will not see it until they move the mouse.
- A deliberate mouse movement of only 1-2 pixels will not reveal the pointer; any larger
  movement will.
- Other `CUrlRichEditCtrl` subclasses (currently only the two comments editors) inherit
  the same behaviour.

## Verification

- `Unicode Debug` builds successfully (`Shared`, `ToDoListLib`, `ToDoList` and
  `RTFContentCtrl`).
- Automated cursor check with the pointer over each editor:
  - simple text: showing -> hidden after a key -> showing after a 20-pixel move;
  - rich text (after switching the comments format): showing -> hidden after a key ->
    showing after a 20-pixel move.
- Both cases reported `GetCursorInfo()` handle `0` while hidden and the I-beam handle
  again after moving.
- No assertions logged during startup, typing or mouse movement.
