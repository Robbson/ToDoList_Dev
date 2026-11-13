# CUST-009: Highlight Quick Find matches in the comments editor

Status: **Done**

## Goal

When Quick Find selects a task because the search term occurs in its comments, also
highlight the matching text in the comments editor. This applies to typing in the Quick
Find field and to Quick Find Next/Previous.

## Upstream starting point

Quick Find is task-level: `ProcessQuickFindTextChange()` calls
`CFilteredToDoCtrl::SelectNextTask()` (`ToDoListWnd.cpp`), which uses
`SEARCHPARAM`/`FindNextTask()` and then discards the `SEARCHRESULT`, selecting only the
task (`ToDoCtrl.cpp:8872-8878`). The comments editor is a separate RichEdit-based
`CContentCtrl`, so nothing transferred the match position to it.

The editor's own Find works differently: `CRichEditBaseCtrl::FindText()` runs
`EM_FINDTEXTEX` and calls `SetSel()` on the live control (`RichEditBaseCtrl.cpp:942-950`).

## What changed

Files: `Core/ToDoList/TDLCommentsCtrl.h/.cpp`, `Core/ToDoList/ToDoCtrl.h`,
`Core/ToDoList/ToDoListWnd.h/.cpp`.

1. `CTDLCommentsCtrl::FindTextEx()` runs `EM_FINDTEXTEXW` on the comments content
   control, then `EM_HIDESELECTION(FALSE)`, `EM_SETSEL` and `EM_SCROLLCARET`, so the match
   is selected and scrolled into view even though the Quick Find combo keeps the focus.
2. `CTDLCommentsCtrl::FindText()` first tries the whole search string when it contains no
   quotes (so a contiguous phrase still highlights as a phrase). It then parses the string
   with `Misc::ParseSearchString()`; quoted phrases stay intact as one term, unquoted
   words become separate terms. The first term that exists in the comments is
   highlighted.

   This matches Quick Find's semantics:
   - `"Jardy Glaube"` (quoted) only matches the ordered phrase and highlights that phrase;
     `"Glaube Jardy"` finds nothing in the comments.
   - `Jardy Glaube` (unquoted) matches both terms in any order and highlights the first
     term found (`Jardy`).
3. `CToDoCtrl::FindInComments()` exposes the operation.
4. `CToDoListWnd` calls `ScheduleQuickFindCommentsHighlight()` after a successful Quick
   Find, Next or Prev. The actual lookup runs from a one-shot
   `TIMER_HIGHLIGHTQUICKFIND` (50 ms) so the comments control has already been updated
   with the newly selected task's content.

The highlight is a normal RichEdit selection, so it is cleared when the user clicks into
the editor, exactly like the editor's own Find selection.

## Risks

- Only the first matching term is highlighted for multi-term searches; a single RichEdit
  selection cannot represent several disjoint ranges without temporary character
  formatting.
- The highlight is only performed for comments. A Quick Find match that only exists in
  the title (or another attribute) does not highlight anything, because task titles are
  owner-drawn and have no substring-highlight state.
- The deferred timer reuses `m_sQuickFind`; if a new search starts within 50 ms it simply
  highlights the latest term after the current selection settles.

## Verification

- `Unicode Debug` builds successfully.
- Manual verification with `Modern Dancing.tdl`:
  - single-term comments search highlights the term in both plain and rich text;
  - Quick Find Next/Previous highlights the term for each newly selected task;
  - multi-term `Jardy Glaube` highlights the first term found (`Jardy`) in the comment
    containing both terms;
  - quoted `"Jardy Glaube"` highlights the ordered phrase, while `"Glaube Jardy"`
    matches no comments text.
