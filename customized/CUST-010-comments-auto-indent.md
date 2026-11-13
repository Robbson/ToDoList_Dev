# CUST-010: Auto-indent in the comments editor

Status: **In progress** (implemented and built; manual typing test pending)

## Goal

Pressing a plain `Enter` in the plain-text comments editor starts the new line with the
same leading whitespace (spaces and tabs) as the current line. On recognised bullet
lines, the marker is replaced by an equivalent run of spaces rather than repeated. In
addition, word-wrapped continuation lines are visually aligned with the first
non-whitespace character of their paragraph, so indentation remains readable when a long
line wraps.

The `Enter` behaviour restores a feature that used to be implemented as a hack in the
older comments control's `ProcessMessage(MSG*)`. The wrapped-line alignment is purely
visual; the underlying text is unchanged.

## Upstream starting point

`CTDLSimpleTextContentCtrl` derives from `CUrlRichEditCtrl`
(`Core/ToDoList/TDLSimpleTextContentCtrl.h`) and already intercepts editing keys in
`CTDLSimpleTextContentCtrl::ProcessMessage()` (`TDLSimpleTextContentCtrl.cpp:172`),
including `VK_TAB`. `CUrlRichEditCtrl` itself has no auto-indent behaviour.

## What changed

Files: `Core/ToDoList/TDLSimpleTextContentCtrl.cpp` and
`Core/ToDoList/TDLSimpleTextContentCtrl.h`.

1. A file-local `GetAutoIndentPrefix()` helper finds the start of the paragraph
   containing the caret by scanning backwards for the previous `\r`/`\n` in chunks. It
   returns the paragraph's leading spaces/tabs plus any recognised bullet marker. The
   markers are listed in `BULLET_PREFIXES` and currently include `- `, `* `, `+ `, `-> `
   and `=> `; add or remove entries there to change the set. Scanning the paragraph
   rather than using `LineFromChar()`/`LineIndex()` avoids treating word-wrapped display
   lines as paragraphs.
2. `ProcessMessage()` handles `WM_KEYDOWN`/`VK_RETURN` when no Ctrl, Alt or Shift is
   held. It inserts `"\r\n"` followed by the prefix from step 1 with `ReplaceSel()` and
   returns `true` so the control does not process the key itself. On a bullet line with
   text, the marker and any extra whitespace after it are replaced by spaces, so the next
   line starts under the bullet text instead of repeating the bullet. Pressing Return on
   an empty bullet adds no bullet prefix.
3. `ApplyAutoIndent()` walks each paragraph and sets the RichEdit hanging indent
   (`PARAFORMAT2.dxOffset` / `PFM_OFFSET`) to the measured width of the paragraph's
   leading spaces/tabs plus any bullet marker. Wrapped lines therefore start where the
   paragraph's text starts. The width is measured with `PointFromChar()` and converted
   from pixels to twips, so spaces, tabs, proportional fonts, bullet markers and the
   control's custom tab stops are handled by RichEdit itself.
4. Formatting runs from a short `TIMER_AUTOINDENT` (50 ms) timer after `EN_CHANGE`,
   `WM_SETTEXT` and `WM_SETFONT`, so it is coalesced while typing and re-applied after a
   font or content change. `EM_SETPARAFORMAT` is sent directly rather than through
   `CRichEditBaseCtrl::SetParaFormat()` to avoid that helper's synthetic `EN_CHANGE`.
   The operation is wrapped in `CRePauseUndo` so the visual formatting does not create
   undo entries, and the selection/first visible line are restored.

## Risks

- The indent is taken from the paragraph containing the selection start. When a
  multi-line selection is replaced by Return, the new line uses the first paragraph's
  indent.
- A paragraph whose leading whitespace exceeds 1024 characters is truncated to the first
  1024 characters, which is well beyond realistic comment indentation.
- Overriding `WM_KEYDOWN` means an `Enter` used to commit an IME composition is handled
  as a normal newline; this matches the previous hack.
- The wrapped-line indent is visual only: `GetWindowText()` is still the content that is
  saved, so no spaces are added to the stored comments and paragraph formatting is lost
  when the text is re-loaded and then re-applied.
- Auto-indent is skipped for comments larger than 1 MB to avoid walking very large
  documents on every edit.
- The paragraph-format pass temporarily moves the caret to each paragraph. Selection and
  the first visible line are restored, but a horizontal scroll position is not explicitly
  restored (word wrap normally makes that irrelevant).
- Shift+Enter, Ctrl+Enter and Alt+Enter keep their existing behaviour.
- Bullet recognition requires the exact marker strings in `BULLET_PREFIXES`, including
  the trailing space. Lines without the trailing space (for example `-item`) are treated
  as ordinary text.
- Bullet markers are recognised literally, not parsed as Markdown; nested markers are
  supported only through the leading spaces/tabs before the marker.
- The Return continuation replaces each marker character with one space and each tab with
  two spaces. This aligns exactly with monospaced/coder fonts; proportional fonts may be
  off by a fraction of a character.

## Verification

- `Core/ToDoListLib/ToDoListLib.vcxproj` builds successfully in `Unicode Debug`.
- Manual verification pending: type an indented line in the comments editor and press
  Enter; the caret should stay at the same indent and wrapped lines should align with the
  text after the indent. Check bullet lines (`- item`, `* item`, `-> item`, `=> item`,
  including indented variants) for both Return continuation and wrapped-line alignment,
  plus an empty bullet followed by Return, font changes, paste, undo/redo and Shift+Enter.
