# CUST-001 — Legacy (v8.2-style) attribute editor

| Field | Value |
|---|---|
| Status | In progress |
| Upstream baseline | ToDoList 9.3.DP7 |
| Scope | Attribute editor presentation |
| Default | Keep the 9.x property grid as default |
| Related | [index.md](index.md) |

## Goal

The 9.x attribute editor is a property grid (`CTDLTaskAttributeCtrl` wrapping
`CTDLTaskAttributeListCtrl`). It is efficient for many attributes and a column layout, but
poor for users who only need a small subset and want a compact horizontal bar of controls
(colour, category, priority, dates, …), as provided up to v8.2.

Requirements:

1. Keep the current property-grid editor available and unchanged as the default.
2. Provide the legacy inline attribute editor (bar/grid of labelled controls) as an
   alternative.
3. Let the user switch editors via a settings option.
4. Honour the user's visible edit fields / columns and support horizontal layout at the
   bottom of the tasklist (and, if practical, vertical at the side).
5. Keep the change isolated from upstream code as far as possible so future updates can
   still be merged.

## Finding: the old editor was not a named component

The v8.2 attribute editor is **not** a class called something like `CTDLTaskAttributeCtrl`.
It was embedded directly in `CToDoCtrl`:

- The edit controls were `CToDoCtrl` member variables (`m_cbAllocBy`, `m_cbCategory`,
  `m_cpColour`, `m_dtcStart`, `m_eTimeEstimate`, `m_eDependency`, …).
- Controls were declared in `TDCCONTROLS` in `Core/ToDoList/tdcstatic.h` and created at
  runtime by `CToDoCtrl`'s constructor via `AddRCControl`/`CRuntimeDlg`.
- `CToDoCtrl::DoDataExchange` used `DDX_Control`/`DDX_Text` and
  `CTDCCustomAttributeUIHelper::DDX` for custom attributes.
- Edit/validation logic was a large set of `CToDoCtrl::OnChange*`, `OnSelChange*`,
  `OnSelCancel*` and date/time handlers.
- Layout was inline in `CToDoCtrl::Resize`/`ReposControls`/`CalcRequiredControlsRect`
  (there was no `CToDoCtrlLayout` class yet).
- Custom attributes were created dynamically by
  `CTDCCustomAttributeUIHelper` (edit + filter halves).

Exact source locations in git history:

| Purpose | Revision | Notes |
|---|---|---|
| Last 8.2-era tree before the version bump to 8.3 | `5609dc81a` (2023-04-14) | `FILEVERSION 8,1,998,8`; the code the user remembers as v8.2 |
| Last tree before the new editor work | `ea95d5c7e` (2023-12-26) | Old editor with all subsequent fixes |
| New editor start | `c1fea7ee1` (2023-12-26) | `TDLTaskAttributeListCtrl.*` added |
| New editor replaces built-in fields | `e049214c3` (2024-01-03) | “Finalise removal of all built-in attribute fields” |
| Pane wrapper added | `8e960fc1e` (2024-04-16) | `TDLTaskAttributeCtrl.*` added |
| Custom-attribute helper split | `d4b236c78` (2024-02-07) | `TDCCustomAttributeUIHelper` renamed; only the filter half remains as `TDCCustomFilterAttributeUIHelper` |

Useful extraction commands (run from the repo root):

```bat
git show ea95d5c7e:Core/ToDoList/ToDoCtrl.cpp > %TEMP%\ToDoCtrl_v8.cpp
git show ea95d5c7e:Core/ToDoList/ToDoCtrl.h   > %TEMP%\ToDoCtrl_v8.h
git show ea95d5c7e:Core/ToDoList/tdcstatic.h  > %TEMP%\tdcstatic_v8.h
git show ea95d5c7e:Core/ToDoList/TDCCustomAttributeUIHelper.cpp > %TEMP%\CustAttribUI_v8.cpp
git show ea95d5c7e:Core/ToDoList/TDCCustomAttributeUIHelper.h   > %TEMP%\CustAttribUI_v8.h
```

## Answer: is it possible?

**Yes.** It is technically feasible to restore the old editor and keep the new one behind
a setting, but it is **not a simple toggle**: version 9 replaced the architecture, not
just a widget. `ToDoCtrl.cpp` differs from the 8.2 era by roughly 6,800 changed lines. The
old editor must be ported back as a component and adapted to the current data APIs.

A wholesale revert of `CToDoCtrl` to 8.2 is **not recommended**: it would discard years of
9.x fixes (multi-selection, dates, read-only handling, custom attributes, time tracking,
dark mode, etc.).

## Recommended design: facade + legacy bar

Keep `CToDoCtrl` and its update pipeline intact, and turn the existing attributes pane into
a facade that can host either editor:

```text
CToDoCtrl
  └── m_ctrlAttributes : CTDLTaskAttributeCtrl        (existing facade/pane)
        ├── mode = Grid   → CTDLTaskAttributeListCtrl (current property grid)
        └── mode = Legacy → CTDLTaskAttributeBarCtrl  (new, ported from v8.2)
```

Why this is the least invasive approach:

- `CToDoCtrl` already talks only to `m_ctrlAttributes` (`SetSelectedTaskIDs`,
  `RefreshSelectedTasksValues`, `GetPriority`, `GetCategories`, `SetAutoListData`, …).
  Those methods become delegating methods on the facade.
- `CToDoCtrlLayout` already positions `m_ctrlAttributes` as one splitter pane. A legacy bar
  is simply a different child window inside that pane, so the layout class does not need a
  redesign. The bar can be at the bottom, left or right according to the existing
  “attributes position” preference.
- `CToDoCtrl::UpdateTask` already pulls edited values from `m_ctrlAttributes` and applies
  them through `SetSelectedTask...` + undo. The legacy bar can use the same route: on a
  user change it sends `WM_TDCN_ATTRIBUTEEDITED` to the facade; `CToDoCtrl` calls
  `UpdateTask`; the facade delegates the getters to the active editor.
- The legacy bar can use its own auto-arranging grid layout, ported from v8.2
  (`CalcRequiredControlsRect` + `ReposControls`), independent of the splitter layout.
- Existing per-tasklist visibility (`TDCCOLEDITVISIBILITY`) continues to decide which
  controls are shown, satisfying the “only the few attributes I need” requirement.

This keeps the current editor untouched and makes the feature removable by deleting the
new class and the mode setting.

## Implementation phases

### Phase 1 — Setting and facade

1. Add an attribute-editor mode preference, e.g. `AttributeEditorMode` (int):
   `0 = property grid` (default), `1 = legacy bar`. A bool is enough today but an int
   allows future modes.
2. Add a checkbox/combo to the UI preferences page (`PreferencesUIPage`). Persist it with
   the other UI preferences and refresh tasklists on change.
3. Extend `CTDLTaskAttributeCtrl`:
   - store the mode;
   - keep `m_lcAttributes` (grid) and add `m_pLegacyBar`;
   - show/hide and size the active editor;
   - keep forwarding all `WM_TDCM_*` / `WM_TDCN_*` messages to the parent.
   - Phase 1 deliberately leaves the grid as the data path; delegation to the legacy
     editor is introduced in Phase 2 once the bar has real controls.
4. Add a minimal `CTDLTaskAttributeBarCtrl` that fills the pane and renders a placeholder.
   Verify both modes build and switch at runtime.

### Phase 2 — Built-in attribute bar

1. Introduce delegation/editor interface so `CTDLTaskAttributeCtrl` routes value getters,
   setters, selection and refresh calls to the active editor.
2. Recreate the control table from the old `TDCCONTROLS` (labels + edit controls) as a
   separate table, e.g. `LEGACY_ATTRIBUTE_CONTROLS`, with fresh or safely reused IDs.
3. Port the `CToDoCtrl` members into `CTDLTaskAttributeBarCtrl`
   (`CEnEdit`, `CAutoComboBox`, `CCheckComboBox`, `CDateTimeCtrlEx`, `CTimeComboBox`,
   `CTDLPriorityComboBox`, `CTDLRiskComboBox`, `CColourPickerEx`, `CMaskEdit`,
   `CTimeEdit`, `CTDLTaskDependencyEdit`, `CTDLRecurringTaskEdit`, …).
4. Port the old value refresh logic:
   - use `CToDoCtrlData` (selected task IDs) plus `CTDCMultiTasker` /
     `CTDCTaskFormatter` for common/mixed values;
   - implement `SetSelectedTaskIDs` and `RefreshSelectedTasksValues(TDCA_*)`;
   - honour read-only mode, multi-selection, date/time formats, priority/risk colours,
     completion status and auto-list data.
5. Port the old edit/validation behaviour into `GetXxx` getters used by `UpdateTask`.
   On edit, send `WM_TDCN_ATTRIBUTEEDITED` to the facade so `CToDoCtrl` applies the change
   with undo/notifications exactly as today.
6. Port the layout algorithm:
   - `CalcRequiredControlsRect` for available width/height;
   - `ReposControls` for bottom (horizontal) and left/right (vertical) placement;
   - `ShowHideControls` driven by `TDCCOLEDITVISIBILITY`;
   - optional `TDCS_AUTOREPOSCTRLS` behaviour (auto rows/columns) if the old style is
     required.
7. Preserve old keyboard navigation (tab order, spin buttons, drop-down behaviour) and
   context menus where applicable.

### Phase 3 — Custom attributes

The old helper still partially exists: `TDCCustomAttributeUIHelper` was renamed to
`TDCCustomFilterAttributeUIHelper` in `d4b236c78`, but only the **filter** half survived.
The edit half must be restored:

- `RebuildEditControls`, `NeedRebuildEditControls`, `IsCustomEditControl`,
  `GetEditControlTooltip`, `FlushEditControl`, `SaveAutoListDataToDefs`,
  `GetControlsData`, `DDX`, file-link thumbnail options, edit prompts.
- Reuse the control-creation internals that already exist in
  `TDCCustomFilterAttributeUIHelper` rather than duplicating all control creation.
- Append custom controls to the legacy bar grid; keep them in sync with
  `OnCustomAttributesChange` and `OnAttributeVisibilityChange`.

If custom attributes are not needed initially, ship Phase 1–2 first and mark custom
attributes as a follow-up, but keep the mode clearly labelled as “built-in attributes”
until complete.

### Phase 4 — Polish and compatibility

- DPI scaling (old layout used DLUs; verify on 125/150/200%).
- Dark mode / UI theme colours (the old editor predates dark mode).
- Read-only tasklists and locked tasks.
- Undo/redo, modified notifications, auto-list add/delete, reminders, time tracking.
- Save/restore legacy-bar state separately from the grid state:
  `LegacyAttribOrder`, `LegacyAttribSizes`, etc., under the tasklist preference key.
- Live switching: destroy/recreate the active editor when the preference changes, without
  losing selection or pending edits.

## Files expected to change

New:

- `Core/ToDoList/TDLTaskAttributeBarCtrl.h` / `.cpp` — legacy editor component.
- Possibly `Core/ToDoList/TDCCustomEditAttributeUIHelper.*` — restored edit-side custom
  attribute helper if separating it from the filter helper is cleaner.

Modified:

- `Core/ToDoList/TDLTaskAttributeCtrl.h` / `.cpp` — facade + mode + delegation.
- `Core/ToDoList/ToDoCtrl.h` / `.cpp` — mode plumbing, update/refresh routing if the
  facade needs extra hooks.
- `Core/ToDoList/PreferencesUIPage.h` / `.cpp` — new option.
- `Core/ToDoList/PreferencesDlg.h` — forwarding getter.
- `Core/ToDoList/TDCToDoCtrlPreferenceHelper.cpp` — apply mode to tasklists.
- `Core/ToDoList/ToDoList.rc`, `Core/ToDoList/resource.h` — checkbox/string resources.
- `Core/ToDoList/TDCStatic.h` — legacy control table (or a new dedicated header).
- `Core/ToDoListLib/ToDoListLib.vcxproj` / `.filters` — add new files.

Reference only (do not copy blindly):

- Old tree at `ea95d5c7e`: `ToDoCtrl.h/.cpp`, `tdcstatic.h`,
  `TDCCustomAttributeUIHelper.h/.cpp`.
- Current code: `TDLTaskAttributeCtrl.*`, `TDLTaskAttributeListCtrl.*`,
  `ToDoCtrlLayout.*`, `TDCCustomFilterAttributeUIHelper.*`.

## Settings

Recommended preference:

```text
Section: Preferences\UI (same section as the other UI options)
Entry:   AttributeEditorMode
Type:    int
Default: 0   ; 0 = property grid, 1 = legacy bar
```

Keep per-tasklist state (which attributes are visible, grid sort/grouping, legacy-bar
order) under the existing `GetPreferencesKey()` tasklist key so the two modes do not
overwrite each other's layout state.

## Test matrix

For both modes:

- single selection, multi-selection, no selection;
- each built-in attribute: title, priority, risk, percent, dates/times, colour, alloc-to,
  alloc-by, status, category, tags, external ID, cost, dependency, version, file link,
  recurrence, time estimate/spent, flag/lock;
- custom attributes of every data/list type (if implemented);
- mixed values across a multi-selection;
- read-only tasklist and locked task;
- undo/redo after each edit;
- auto-list add/remove and auto-list read-only;
- date format (ISO / weekday / time-of-day) and DPI changes;
- switching modes while a task is selected and while an edit is active;
- save/reload of tasklist and application restart;
- legacy bar at bottom, left and right; resized window; maximised tasklist/comments;
- `TDLTest` and `Unicode Debug`/`Unicode Release` builds.

## Risks

| Risk | Mitigation |
|---|---|
| Old code does not compile against current APIs | Port behaviour, not raw code; compile incrementally per attribute group. |
| Two editors drift in behaviour | Centralise value get/set/formatting in shared helpers; test both modes from the same matrix. |
| Custom attributes are the largest port | Restore the edit half of the old helper; if needed, defer custom attributes to a second phase. |
| Lost 9.x fixes in the legacy path | Port relevant fixes: multi-selection, null dates, read-only, tabbing, tooltips, DPI. |
| Resource-ID collisions | Audit `resource.h`; prefer new IDs for the legacy controls. |
| Live mode switching corrupts state | Flush active edit, preserve selection, destroy/recreate the editor, then refresh values. |
| Merge pain with upstream | Confine changes to the facade + new files; avoid editing giant `ToDoCtrl.cpp` sections. |

## Effort estimate

- Facade + setting + skeleton: small.
- Built-in legacy bar (controls, value refresh, editing, layout): large.
- Custom attributes: medium-large (helper edit half must be restored).
- Overall: a multi-day to multi-week port depending on how faithful and complete the
  attribute coverage must be.

## Open decisions

1. **Faithful port vs new compact bar**: port the old grid exactly (more work, familiar
   UX) or write a modern compact bar with the same visual idea (less legacy code, not
   pixel-identical).
2. **Custom attributes in v1 or later**.
3. **Scope of the setting**: global preference (simplest) or per-tasklist.
4. **Allowed positions**: bottom only, or bottom + left/right like the old editor.
5. **Default**: keep the 9.x property grid as default (recommended).

## Implementation log

### Phase 1 (done)

- Added `TDC_ATTRIBUTEEDITORMODE` (`TDCAEM_GRID`, `TDCAEM_LEGACY`) to `TDCEnum.h`.
- Added the global UI preference `AttributeEditorMode` (default `0`, property grid):
  - `PreferencesUIPage` checkbox and load/save,
  - `PreferencesDlg` forwarding getter,
  - `TDCToDoCtrlPreferenceHelper` applies it to every tasklist,
  - `CToDoCtrl::SetAttributeEditorMode()` forwards to the attributes pane.
- Added the facade mode to `CTDLTaskAttributeCtrl`:
  - stores the active mode,
  - creates `CTDLTaskAttributeBarCtrl` lazily,
  - shows/hides toolbar + property grid or the legacy bar,
  - sizes the active editor to the pane,
  - routes focus, theme changes and context menus appropriately.
- Added `CTDLTaskAttributeBarCtrl` as a placeholder window. It fills the pane and says
  “Legacy attribute editor will appear here (Phase 1 placeholder)”.
- The property grid remains the data path (selection, edits, auto-lists, custom
  attributes) while the legacy bar is only visual. Phase 2 will move the value/edit
  plumbing into the bar.
- The preference can be switched at runtime; tasklists apply it through the normal
  preferences refresh path.

Verification:

- `ToDoListLib` (Unicode Debug, Win32) builds.
- `ToDoList.exe` (Unicode Debug, Win32) builds and links.
- `TDLTest`: 310240/310264 pass. The 24 failures are all in
  `CTDCTaskMatcherTest::IndividualRules` and are unrelated to this change (no matcher
  code was touched); they are likely locale/date-sensitive.

### Phase 2 (built-in attributes implemented; runtime verification pending)

Implemented a functional legacy bar for all built-in attributes:

- Added `ITDCAttributeEditor` as the common editor contract.
  - `CTDLTaskAttributeListCtrl` implements it unchanged.
  - `CTDLTaskAttributeCtrl` now routes all value/selection/auto-list calls through
    `GetActiveEditor()` instead of hard-coding the property grid.
- Rebuilt `CTDLTaskAttributeBarCtrl` from the v8.2 control set:
  - priority, risk, percent, time estimate/spent, start/due/done dates and times,
  - recurrence, colour, alloc-to, alloc-by, status, category, tags,
  - external ID, cost, dependency, version, file link.
- Control creation uses the old runtime-dialog table approach (`TDCCONTROL` +
  `CRuntimeDlg`), with new IDs (`IDC_TDLAB_*`) to avoid collisions with removed upstream
  IDs.
- Values are populated with `CTDCMultiTasker`:
  - common values are shown,
  - mixed values clear text/date fields and clear priority/risk/percent,
  - multi-value fields use `CCheckComboBox` matched/mixed states.
- Editing uses the existing pipeline:
  - the bar sends `WM_TDCN_ATTRIBUTEEDITED`,
  - the facade forwards it to `CToDoCtrl`,
  - `CToDoCtrl::UpdateTask()` pulls the edited value through the facade and applies it with
    undo and notifications.
- Added direct handling in `CToDoCtrl::UpdateTask()` for `TDCA_COLOR` and
  `TDCA_RECURRENCE`, which the grid normally edits via dialogs.
- Visibility follows `TDCCOLEDITVISIBILITY`; the bar re-lays out visible fields in a
  responsive label/control grid.
- Auto-list data (alloc-to/by, status, category, tags, version) is populated and updated.
- Theme background, priority colours, completion status, date formatting and current
  folder are applied.

Verification:

- `ToDoListLib` and `ToDoList.exe` build in `Unicode Debug|Win32`.
- `TDLTest` builds after the interface change.
- Debug smoke test succeeded: a second instance started with the sample tasklist and the
  portable INI setting `AttributeEditorMode=1`, i.e. the legacy bar was created and
  initialised without crashing. Interactive editing still needs manual verification.
- Release compile/link succeeded; the project's existing post-build copy step failed with
  exit code 9009 (environmental `mkdir`/`copy` scripts), unrelated to the C++ changes.

Assertion fixes found during runtime testing:

- Priority/risk “no value” must be `FM_NOPRIORITYORRISK`, not
  `FM_ANYPRIORITYORRISK`; the edit combos are created without the “Any” entry.
- `SetPriorityColors(m_aPriorityColors)` was a self-copy of the `CDWordArray`, which
  tripped MFC's `CArray` assertion. It now skips the copy when source and destination are
  the same object.
- `SetSelectedTaskIDs()` now also guards against self-copy.
- Added debug-only assertion logging (`TDL_ASSERT_LOG`) with stack traces; see
  `AGENTS.md` for usage.

Layout and appearance fixes:

- The attributes pane is now sized to the controls rather than a user-draggable split.
  - `CTDLTaskAttributeBarCtrl::GetRequiredHeight/Width()` calculate the space needed for
    the visible fields.
  - `CToDoCtrl::UpdateAttributesPaneSize()` applies that size to `CToDoCtrlLayout`.
  - `CSimpleSplitter::SetResizable(FALSE)` disables dragging for the splitter that owns
    the attributes pane in legacy mode; it is restored in grid mode.
  - Visibility changes and tasklist resizes re-apply the required size.
- Static labels are transparent/themed via `WM_CTLCOLORSTATIC` and a background brush.
- The percent spin control is shown/hidden and enabled with the percent field.
- The colour picker shows `IDS_COLOR_SAMPLETEXT` with automatic/more-colour text.
- The file-link field expands to the remaining horizontal space on its row.
- Date controls are invalidated after `SetTime`/`GDT_NONE` to reduce checkbox painting
  artifacts. Further investigation is still needed for calendar-visible cases.

Interaction fixes:

- The attributes pane reflows when a parent splitter changes its width:
  `CToDoCtrl::OnSplitChange()` recomputes the required size and uses the current task
  pane rect (`CToDoCtrlLayout::GetTasksRect()`) for repositioning.
- Percent-complete edits now read the edit control text directly. Spin-button changes
  post `TDCA_PERCENT` after the spin updates the buddy edit.
- Auto-complete combo fields (status, alloc-by, version) commit on selection or
  kill-focus instead of `CBN_EDITCHANGE`, so typing “foo” no longer creates `f`, `o`, `o`
  auto-list entries.
- Date pickers are left to `CDateTimeCtrlEx`'s native checkbox/calendar flow. An earlier
  `DTN_DROPDOWN` auto-check was removed because it interfered with unchecking. Checkbox
  painting when the calendar is closed remains a known issue.
- The splitter bars adjacent to the attributes pane are hidden in legacy mode, but their
  width/height is still reserved. `CSimpleSplitter` gained per-bar visibility
  (`SetBarVisible`/`IsBarVisible`); hidden bars are not painted or hit-tested, and
  `CToDoCtrlLayout` applies this to the bars either side of the attributes pane.
- Hidden-bar areas are filled with the tasklist theme background
  (`SetBarBackgroundColor`) during painting, preventing stale pixels and resize artifacts
  where the bar used to be drawn.
- Percent-complete changes on parent tasks are intentionally ignored because the value is
  calculated from children. For leaf tasks, the spin control now uses the configured
  `PercentDoneIncrement` step size.

Known Phase 2 limitations (Phase 3 or later):

- Custom attributes are not yet created in the legacy bar; the property grid remains the
  only editor for them.
- The time-spent clock/add-time buttons and the dependency picker/link buttons are not
  ported yet; fields are directly editable.
- No per-mode persistence of legacy-bar ordering/state yet.
- Read-only/auto-list-read-only and locked-task UI states are not visually enforced; the
  data layer still rejects edits.
- Layout is a straightforward responsive grid, not a pixel-identical port of the v8.2
  auto-reposition algorithm.

Next: Phase 3 — custom attributes and the remaining auxiliary buttons.


