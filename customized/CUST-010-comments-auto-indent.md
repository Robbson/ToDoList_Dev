# CUST-010: Auto-indent on Return in the comments editor

Status: **In progress** (implemented and built; manual typing test pending)

## Goal

Pressing a plain `Enter` in the plain-text comments editor starts the new line with the
same leading whitespace (spaces and tabs) as the current line. This restores a behaviour
that used to be implemented as a hack in the older comments control's
`ProcessMessage(MSG*)`.

## Upstream starting point

`CTDLSimpleTextContentCtrl` derives from `CUrlRichEditCtrl`
(`Core/ToDoList/TDLSimpleTextContentCtrl.h`) and already intercepts editing keys in
`CTDLSimpleTextContentCtrl::ProcessMessage()` (`TDLSimpleTextContentCtrl.cpp:172`),
including `VK_TAB`. `CUrlRichEditCtrl` itself has no auto-indent behaviour.

## What changed

Files: `Core/ToDoList/TDLSimpleTextContentCtrl.cpp`.

1. A file-local `GetParagraphIndent()` helper finds the start of the paragraph
   containing the caret by scanning backwards for the previous `\r`/`\n` in chunks. It
   then returns the paragraph's leading spaces and tabs. Scanning the paragraph rather
   than using `LineFromChar()`/`LineIndex()` avoids treating word-wrapped display lines
   as paragraphs.
2. `ProcessMessage()` handles `WM_KEYDOWN`/`VK_RETURN` when no Ctrl, Alt or Shift is
   held. It inserts `"\r\n"` followed by the indent with `ReplaceSel()` and returns
   `true` so the control does not process the key itself.

Shift+Enter, Ctrl+Enter and Alt+Enter keep their existing behaviour.

## Risks

- The indent is taken from the paragraph containing the selection start. When a
  multi-line selection is replaced by Return, the new line uses the first paragraph's
  indent.
- A paragraph whose leading whitespace exceeds 1024 characters is truncated to the first
  1024 characters, which is well beyond realistic comment indentation.
- Overriding `WM_KEYDOWN` means an `Enter` used to commit an IME composition is handled
  as a normal newline; this matches the previous hack.

## Verification

- `Core/ToDoListLib/ToDoListLib.vcxproj` builds successfully in `Unicode Debug`.
- Manual verification pending: type an indented line in the comments editor and press
  Enter; the caret should stay at the same indent. Deeper lines, tab indents, wrapped
  lines and Shift+Enter should also be checked.
