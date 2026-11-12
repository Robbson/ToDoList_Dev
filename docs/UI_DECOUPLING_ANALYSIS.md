# Business Logic ↔ MFC View Coupling — Analysis and UI-Replacement Feasibility

> Independent analysis document. It does not replace [ARCHITECTURE.md](ARCHITECTURE.md);
> it examines one question: **how much application logic is entangled with MFC window
> code, what has to be rewritten to leave MFC, and could wxWidgets or Qt replace the
> UI layer?**
>
> Scope: `Core/ToDoList`, `Core/ToDoListLib`, `Core/Interfaces`, `Core/Shared`,
> `Core/3rdParty`. `Core/TDLTest` is used as evidence of what is already testable
> headlessly. Plugins are considered only through their contracts.

---

## 1. Method and terminology

The analysis distinguishes three kinds of dependency, because they carry very different
porting costs:

| Dependency | Meaning | Porting impact |
|---|---|---|
| **Framework dependency** | Uses MFC as the application framework: `CWinApp`, `CFrameWnd`, message maps, `CWnd`, DDX, command routing, OLE drop targets. | Must be redesigned; cannot be shimmed. |
| **Widget dependency** | Uses MFC/Win32 widgets for rendering: `CDialog`, `CListCtrl`, `CTreeCtrl`, `CDC`, owner-draw, timers, `HWND`. | Must be reimplemented in the new toolkit. |
| **Library dependency** | Uses MFC only as a utility library: `CString`, `CArray`, `CMap`, `COleDateTime`, `CXmlFileEx`. | Mechanically portable or shimmable; no windows involved. |

"Business logic" here means: task data and hierarchy, undo, recurrence, calculations,
filtering/matching/sorting, persistence and file format, import/export, preferences
semantics, reminders and time-tracking rules.

Measurements were taken by scanning the tree with regex counts for MFC window symbols
(`CWnd`, `CDialog`, `CDC`, `CListCtrl`, `CTreeCtrl`, `CFrameWnd`, `CWinApp`), MFC utility
types (`CString`, `CArray`, `CMap`, `COleDateTime`), command/message-map entries, and by
reading the class headers listed in §11. File sizes are used as a rough ownership proxy,
not as LOC.

---

## 2. Executive summary

**The coupling is highly asymmetric, and that is good news.**

1. **The data and semantics layer is already free of view code.** `CToDoCtrlData`,
   `ToDoCtrlDataUtils`, `ToDoitem`, `CToDoCtrlUndo`, `CTDCFilter`, `CTaskFile` and the
   recurrence/time-log helpers contain **zero** references to `CWnd`, `CDialog`, `CDC`,
   `AfxMessageBox`, `theApp`, `PostMessage` or `SendMessage`. They depend on MFC only as a
   utility library (`CString`, `CArray`, `CMap`, `COleDateTime`); `CTaskFile` additionally
   inherits `CXmlFileEx` for XML I/O. The existing `TDLTest` suite already exercises this
   layer headlessly.
2. **The application and tasklist controllers are fused into MFC windows.** `CToDoCtrl`
   (a `CRuntimeDlg`) is the real tasklist controller: 149 direct `m_data` calls, all
   `SetSelectedTask*` commands, file load/save/archive orchestration, undo activation,
   notifications, wait cursors and confirmation dialogs. `CToDoListWnd` (a `CFrameWnd`)
   is the application controller: file lifecycle, session restore, auto-save, preferences,
   plugin management and ~370 command/update handlers.
3. **There is no view abstraction.** No interface separates "tasklist view" from
   "tasklist data/controller". The only meaningful UI seams are
   `ITDCAttributeEditor` (attribute-editor abstraction) and the HWND-based plugin
   interfaces (`IUIExtension`, `IContentControl`).
4. **Most of the porting cost is presentation, not business rules.**
   Tasklist view/controller code plus shell code is roughly **1.5 MB** of source in the
   sampled files, versus roughly **660 KB** of data/semantics/persistence logic.
   The custom task grid, attribute editor, shell and 51 dialog/preference files are the
   expensive part; the task database and its semantics are the reusable part.
5. **A wxWidgets or Qt UI is technically feasible**, but not as a drop-in replacement.
   It requires (a) extracting controllers out of `CToDoCtrl`/`CToDoListWnd`, (b) defining
   view/dialog-service contracts, (c) reimplementing the two custom-drawn surfaces (task
   grid and attribute editor) and the shell, and (d) replacing MFC utility types or
   keeping an MFC-static-library shim. Plugins can survive because their contracts are
   Win32/HWND-based, not MFC-based.

**Verdict:** the application is a good candidate for a *strangler* migration, not for a
big-bang rewrite. Core logic should be extracted and reused; the UI layer should be
rewritten. Qt is the better fit for the data-heavy task grid; wxWidgets is closer to the
existing Win32 behaviour but still requires the same custom controls.

---

## 3. Measurements

### 3.1 MFC symbol footprint

| Tree | Files scanned | Files using MFC **window** types | Files using MFC **utility** types |
|---|---:|---:|---:|
| `Core/ToDoList` | 347 | 168 (48%) | 281 (81%) |
| `Core/Shared` | 313 | 184 (59%) | 220 (70%) |
| `Core/Interfaces` | 42 | 10 (24%) | 24 (57%) |
| `Core/3rdParty` | 178 | 45 (25%) | 104 (58%) |

`Core/Shared` is effectively an MFC widget/utility library (17 classes derive directly
from `CDialog`/`CDialogEx`/`CPropertyPage`/`CWnd`/`CTreeCtrl`/`CListCtrl`/`CEdit`/
`CComboBox`/`CTabCtrl`; 46 unique headers include `<afx...>`), not a portable utility layer.

### 3.2 Where the code lives (sampled `.cpp` sizes)

| Group | Representative files | Size |
|---|---|---:|
| Tasklist view + controller | `ToDoCtrl`, `TabbedToDoCtrl`, `FilteredToDoCtrl`, `TDLTaskCtrlBase`, tree/list/attribute/comments/layout | ~976 KB |
| Shell | `ToDoListWnd`, `ToDoList`, `ToDoCtrlMgr`, filter bar, menu/toolbar/status | ~555 KB |
| Data core | `ToDoitem`, `ToDoCtrlData`, undo, structs, custom attrs, recurrence | ~297 KB |
| Semantics | `ToDoCtrlDataUtils`, `TDCFilter` | ~215 KB |
| Persistence | `TaskFile`, multi-file, backup, time log | ~149 KB |
| Dialogs + preferences | 51 unique files (35 `*Dlg*.cpp`, 17 `Preferences*.cpp`) | ~596 KB |

Other facts used below:

- `ToDoListWnd.cpp` is ~390 KB with ~370 `ON_COMMAND`/`ON_UPDATE_COMMAND_UI` entries and
  ~240 message-map entries.
- `TDCMsg.h` declares **58** registered `WM_*` messages used as an internal event bus.
- The XML/plugin boundary `ITaskList.h` contains **no** MFC types (it includes only
  `IEnums.h` and Win32 types).
- `IUIExtension` uses `HWND`, `HICON`, `MSG`, `POINT`, `RECT`, `HFONT` — no MFC types.

### 3.3 Class inheritance (the structural cause)

```
CToDoListApp : CWinApp                                  (ToDoList.h:31)
CToDoListWnd : CFrameWnd, CDialogHelper                 (ToDoListWnd.h:70)
CToDoCtrlMgr             owns a CTabCtrlEx&             (ToDoCtrlMgr.h:36)
CToDoCtrl : CRuntimeDlg                                 (ToDoCtrl.h:50)
CTabbedToDoCtrl : CToDoCtrl                             (TabbedToDoCtrl.h:44)
CFilteredToDoCtrl : CTabbedToDoCtrl                     (FilteredToDoCtrl.h:17)
CTDLTaskCtrlBase : CWnd, CTreeListSyncer                (TDLTaskCtrlBase.h:46)
CTDLTaskAttributeListCtrl : CInputListCtrl, ITDCAttributeEditor (TDLTaskAttributeListCtrl.h:36)
CToDoCtrlReminders : CTDLShowReminderDlg                (ToDoCtrlReminders.h:29)
```

The controller hierarchy *is* the view hierarchy. That is the central coupling problem.

---

## 4. Layer-by-layer findings

### 4.1 Data model — clean (library-level MFC only)

`TODOITEM` and `TODOSTRUCTURE` (`ToDoitem.h:162`, `ToDoitem.h:332`) store fields and
hierarchy separately. `CToDoCtrlData` (`ToDoCtrlData.h:43`) owns `CToDoCtrlDataItems`
(a `CMap`) and `CToDoCtrlDataStructure` (`ToDoCtrlData.h:249-251`), plus undo. Scanning
`ToDoitem.cpp`, `ToDoCtrlData.cpp`, `ToDoCtrlUndo.*` found **no** window, message, dialog,
cursor or application-object references. The data layer also does not emit notifications:
the view does that after the mutation (see §4.3).

Residual dependencies: `CString`, `CArray`, `CMap`, `COleDateTime`, and `CBinaryData`
from `Core/Shared`. These are replaceable mechanically, or can be retained behind a
typedef/facade while MFC is still linked.

### 4.2 Semantics, filtering, persistence — clean, but presentation-aware

`ToDoCtrlDataUtils.cpp` (198 KB) has no UI references, but it is not pure domain logic:

- `CTDCTaskFormatter` produces display strings and paths.
- `CTDCTaskCalculator` implements rollups, completion, overdue and custom calculations.
- `CTDCTaskMatcher` / `CTDCTaskComparer` implement filter/find and sorting.
- `CTDCTaskExporter` converts the model to `CTaskFile`.
- `CTDCTaskColumnSizer` calculates column widths.

This is exactly the layer a new UI wants, but its output types (`CString`,
`COleDateTime`) and friend-coupling to `CToDoCtrlData` mean it is a **presentation model**,
not a toolkit-neutral service API. It is reusable with a type-porting pass.

`CTDCFilter` (`TDCFilter.h:13`) is a clean state machine over `TDCFILTER`/`SEARCHPARAMS`
and already builds the query that `CTDCTaskMatcher` evaluates. Its UI partner
(`CTDLFilterBar`) only maps controls to that model via DDX
(`TDLFilterBar.cpp:120-145`, `TDLFilterBar.cpp:475`). This is one of the easiest seams to
cut: keep `CTDCFilter`, rewrite the bar.

`CTaskFile` (`TaskFile.h:86`) implements `ITASKLISTBASE` **and** inherits `CXmlFileEx`.
It has no window references, but it is MFC/MSXML-bound through `CXmlFileEx`. The plugin
contract it implements is MFC-free, which makes it a viable persistence API for a new UI.

### 4.3 `CToDoCtrl` — the primary god object

`CToDoCtrl` is both the tasklist view host and the tasklist controller. Evidence:

- It owns the data and all panes (`ToDoCtrl.h:469-528`): `CToDoCtrlData m_data`,
  `CTDLTaskTreeCtrl`, `CTDLTaskAttributeCtrl`, `CTDLCommentsCtrl`, `CToDoCtrlLayout`,
  `CTDCTimeTracking`, `CTDCSourceControl`, `CTDCFindReplace`.
- Every edit command lives here and interleaves model mutation, undo, tree update and
  modified-state notification. Example `SetSelectedTaskTitle`
  (`ToDoCtrl.cpp:1613-1644`):

  ```cpp
  IMPLEMENT_DATA_UNDO_EDIT(m_data);
  ...
  HandleModResult(dwTaskID, m_data.SetTaskTitle(dwTaskID, sTitle), aModTaskIDs);
  ...
  m_taskTree.Tree().SetItemText(GetSelectedItem(), sTitle);
  m_taskTree.InvalidateSelection();
  SetModified(TDCA_TASKNAME, aModTaskIDs);
  ```

- Persistence orchestration is also here: `Save` (`ToDoCtrl.cpp:4237-4323`) asserts a live
  window, checks source-control state, shows a confirmation dialog through `CMessageBox`
  (`ToDoCtrl.cpp:4295`), and `SaveTaskfile` uses `CWaitCursor`
  (`ToDoCtrl.cpp:4332`). `Load`, `Archive*`, `Paste*`, `CreateNewTask*` and
  `BuildTasksForSave` are in the same class.
- Notification is direct window messaging: `SetModified` sends `WM_TDCN_MODIFY` to the
  parent (`ToDoCtrl.cpp:5294`); selection changes are posted as
  `WM_TDCN_SELECTIONCHANGE` (`ToDoCtrl.cpp:7365-7367`).

Complexity to decouple: **very high**. This class must be split into a headless
`TasklistController` (data + selection + undo + command semantics + persistence
orchestration) and an `ITasklistView` implementation (panes, editing, rendering,
dialogs). It is not a mechanical refactor: the command surface is large, and undo/notify
ordering is observable.

### 4.4 `CTabbedToDoCtrl` and `CFilteredToDoCtrl` — view coordination

`CTabbedToDoCtrl` (179 KB) coordinates tree, list, view tabs and extension windows:
cross-view selection, sort state, rebuild of the flat list from visible tree IDs, and
lazy creation/destruction of `IUIExtensionWindow`s. This is genuine view-model logic but
implemented as window methods.

`CFilteredToDoCtrl` (37 KB) adds `CTDCFilter m_filter`, filtered visibility, the
"filter by selection" snapshot, and rebuilds presentations. It does not duplicate task
data, which keeps the seam clean. Filter *state* is reusable; the rebuild/sort/selection
side effects are not.

Complexity: **high** for the tabbed coordinator, **medium** for the filtered wrapper
(filter model already exists).

### 4.5 Task grid — where most rendering cost sits

`CTDLTaskCtrlBase` (152 KB) implements the shared task surface: two panes (title +
attribute columns), column catalogue, owner-draw, header handling, sorting, selection,
row colours and notification plumbing. It contains `CDC`-based drawing and virtual
selection queries used as business predicates (`SelectionHasDependencies`,
`SelectionHasLockedParents`, … in `TDLTaskCtrlBase.h:117-157`). The tree and list
variants wrap `CTreeCtrl`/`CListCtrl` and maintain `HTREEITEM`↔task-ID maps.

`CTreeListSyncer` (`Core/Shared/TreeListSyncer.cpp`, 99 KB) is the two-window scroll,
selection and splitter synchronisation engine. This is the CUST-002/CUST-003 hotspot.

This is the **most expensive single rewrite** after the shell: a model/view task grid
with hierarchical rows, frozen title column, attribute columns, grouping, sorting,
inline editing, custom colours, drag/drop, tooltips and DPI/dark-mode behaviour.

### 4.6 Attribute editor — second custom-drawn surface

`CTDLTaskAttributeListCtrl` (`TDLTaskAttributeListCtrl.h:36`) is a property-grid-like
`CInputListCtrl` that hosts many MFC combo/edit controls and implements
`ITDCAttributeEditor` (`ITDCAttributeEditor.h:12`). It interleaves:

- attribute value aggregation for multi-selection (`CTDCMultiTasker`),
- per-attribute edit semantics and validation,
- auto-list data,
- date/time/cost/dependency/icon editors,
- owner-draw and grouping.

There is a second implementation (`CTDLTaskAttributeBarCtrl`, legacy mode) behind the
same interface. That interface is the **best existing seam** for a new editor: a Qt/wx
attribute editor can implement `ITDCAttributeEditor`-equivalent contracts, or the
controller can own the interface and the view can be swapped.

### 4.7 Shell and command architecture

`CToDoListWnd` (390 KB) is the application controller but is a `CFrameWnd`. It owns the
menu, toolbars, status bar, tab strip, filter bar, find dialog, time tracker, reminders,
tray icon and update handling, and implements file open/save/close, session save/restore,
auto-save, export and preference application. ~370 command and update handlers mean that
**command policy (what is enabled, what it acts on) is encoded in message-map handlers**,
not in a reusable controller.

Additionally, command routing depends on MFC's `CFrameWnd` chain and `CCmdTarget`
update-UI mechanism. A new toolkit needs a command registry, action enablement model and
a target resolver (active tasklist vs shell).

### 4.8 Cross-cutting services trapped in UI classes

| Service | Current shape | Problem |
|---|---|---|
| Reminders | `CToDoCtrlReminders : CTDLShowReminderDlg` (`ToDoCtrlReminders.h:29`) owns reminder storage, timer, snooze logic, Stickies integration and the reminder window. | A business service is a dialog. |
| Time tracking | `CTDCTimeTracking` (`TDCTimeTracking.h:25`) takes `CToDoCtrlData` **and** `CTreeSelectionHelper` (`TDCTimeTracking.h:29`); `TDLTimeTrackerDlgData` is named "data" but holds `const CToDoCtrl*`. | Selection (view concept) is a constructor dependency; dialog data references the view. |
| Source control | `CTDCSourceControl` takes `const CToDoCtrl&`, reads/writes files and drives check-in/out. | Business service depends on the view object. |
| Find/replace | `CTDCFindReplace` derives from shared `FIND_STATE` and holds a `CToDoCtrl&`. | State machine is reusable; the engine is bound to the view. |
| Threaded export | `CTDLThreadedExporterWnd : CWnd` runs `AfxBeginThread` and posts completion to a window. | Worker orchestration is a window. |
| Drag/drop & clipboard | `TDCTaskListDropTarget` + `taskclipboard` use OLE/MFC and custom formats. | Toolkit-specific, but format/model can be reused. |

### 4.9 Preferences, localisation, theming

- `CPreferences` implements `IPreferences` over registry/INI and is free of windows, but
  uses `CString`, `CDWordArray` and `CWinApp`-adjacent profile APIs. Preferences **pages**
  (17 files) are MFC dialogs containing validation and policy.
- `CLocalizer` exposes `CWnd`/`HMENU`-based helpers in addition to the toolkit-neutral
  `ITransText` contract. A new UI can reuse the translation DLL but needs a new UI-text
  adapter (resource IDs → toolkit strings).
- `UITHEME` (`Core/Interfaces/UITHEME.h`) is plain `COLORREF` data. The theming engine
  (`DarkMode.cpp`, `GraphicsMisc.cpp`, `Themed.cpp`) is inherently toolkit-specific.

### 4.10 Plugin and extension boundary — the surviving seam

`Core/Interfaces` is the most portable part of the app:

- `ITaskList.h` (19 KB) defines the task API with **no MFC types**.
- `IUIExtension.h` creates extension windows from `HWND hwndParent`
  (`IUIExtension.h:125`) and returns `HWND` (`IUIExtension.h:377`); it also uses
  `MSG*`, `HICON`, `POINT`, `RECT`, `HFONT`.
- `IContentControl.h` is HWND/message based (registered `WM_ICC_*` messages).
- Managers (`UIExtensionMgr`, `ImportExportMgr`, `TasklistStorageMgr`, `ContentMgr`)
  are Win32 `LoadLibrary`/interface-version based; only `ContentMgr` makes one incidental
  `CWnd::ModifyStyleEx` call.

Consequence: a Qt or wx shell can host existing plugin windows as native child windows
(`QWindow::fromWinId` / `createWindowContainer`, `wxNativeWindow`) and keep the plugin
ABI. The plugin contracts are therefore **not** a blocker, provided the new shell keeps
a Win32 message pump and forwards `MSG`-based calls where the contract requires it.

### 4.11 `Core/Shared` and third-party layer

`Core/Shared` (2.5 MB, 313 files) is not a neutral utility library: it contains
`TreeListCtrl`, `TreeListSyncer`, `InputListCtrl`, `TabCtrlEx`, `EnListCtrl`,
`EnHeaderCtrl`, `DarkMode`, `GraphicsMisc`, rich-edit wrappers, browsers, tray icons and
dialog helpers. It must be **replaced**, not ported. Only a subset is portable logic:
`FileMisc`, `DateHelper`, `TimeHelper`, `Misc`, `XmlFile`, `BinaryData`, `Preferences`
helpers.

`Core/3rdParty` is mixed: `Msxml`, `cpp-expression-parser`, `Zlib`, `myspell` and
`Detours` are reusable; 45 of 178 files contain MFC UI types (charts/controls/Office
helpers). The core app depends mainly on the XML/date/utility subset.

---

## 5. Coupling mechanisms catalogue

| Mechanism | Evidence | Porting consequence |
|---|---|---|
| Inheritance from MFC windows/dialogs | `CToDoCtrl : CRuntimeDlg`, `CToDoListWnd : CFrameWnd`, reminder dialog service | Controllers cannot exist without windows. Must be inverted. |
| Direct child-window calls | `m_taskTree.Tree().SetItemText(...)` inside edit commands (`ToDoCtrl.cpp:1640`) | Controller logic must move behind a view interface. |
| Registered window messages as event bus | 58 `WM_*` messages in `TDCMsg.h`; `SendMessage(WM_TDCN_MODIFY)` (`ToDoCtrl.cpp:5294`) | Replace with observer/event interfaces; keep a compatibility adapter for plugins. |
| MFC command routing | ~370 `ON_COMMAND`/`ON_UPDATE_COMMAND_UI` handlers in `ToDoListWnd.cpp` | Need command registry + enablement model + target resolution. |
| DDX/DDV and dialog data exchange | `TDLFilterBar.cpp:120-145`; all preference/dialog pages | Dialog models must be separated from controls. |
| Owner-draw and `CDC` | `TDLTaskCtrlBase.cpp` (25 `CDC` refs), custom columns/headers | Rewrite rendering with toolkit painting/model delegates. |
| MFC timers / idle / tray | `CToDoCtrlReminders`, shell, time tracker | Toolkit event loop + service scheduler. |
| OLE drag/drop & clipboard | `TDCTaskListDropTarget`, `taskclipboard` | Reimplement with toolkit DnD; reuse MIME/format model. |
| MFC utility types everywhere | 281/347 ToDoList files; `ToDoitem.h:24`, `ToDoCtrlDataUtils.h:17` | Type-porting pass or compatibility typedefs. |
| Data definitions carry UI metadata | `CTRLITEM`/`CUSTOMATTRIBCTRLITEM` in `TDCStruct.h:294` (`CWnd*`, `CRuntimeClass*`) | Split model definitions from custom-attribute control layout metadata. |
| UI headers leaking into export | `TaskListExporterBase.cpp:7` includes `tdlrecurringtaskedit.h` | Cosmetic but symptomatic; remove and validate layering. |

---

## 6. Component inventory and decoupling complexity

Legend for **decouple complexity**:
**S** = days (mechanical) · **M** = 1–3 weeks · **H** = 3–8 weeks ·
**VH** = 2–6 months · **X** = fundamental rewrite (>6 months, or only possible as part of
a surface rewrite). Estimates are indicative for one experienced C++/MFC engineer,
including tests, and exclude the cost of building the replacement widget set.

| # | Component | Where | Coupling today | Logic trapped | Decouple complexity |
|---:|---|---|---|---|---|
| 1 | `TODOITEM` / `TODOSTRUCTURE` | `ToDoitem.h/.cpp` (~50 KB) | MFC utility types only; no windows | Task fields, hierarchy, recurrence helpers | **S** |
| 2 | `CToDoCtrlData` | `ToDoCtrlData.h/.cpp` (123 KB) | MFC containers; no notifications, no windows | CRUD, hierarchy, references, undo entry points | **S–M** |
| 3 | `CToDoCtrlUndo` | `ToDoCtrlUndo.*` | MFC containers only | Undo/redo semantics and snapshots | **S** |
| 4 | Attribute/filter/search structs | `TDCStruct.h/.cpp`, `TDCCustomAttributeDef.*` (~112 KB) | Mixed: `CTRLITEM` exposes `CWnd*`/`CRuntimeClass*` (`TDCStruct.h:294`) | Definitions, validation, custom calculations | **M** (split model vs control metadata) |
| 5 | `ToDoCtrlDataUtils` family | `ToDoCtrlDataUtils.h/.cpp` (198 KB) | No windows; `CString`/`COleDateTime`; friends of the data class | Formatting, calculations, matching, sorting, export, multi-selection aggregation, column sizing | **M** (type port + façade) |
| 6 | `CTDCFilter` | `TDCFilter.h/.cpp` | No windows; MFC containers; `CPreferences` | Filter state machine, advanced filters, query building | **S–M** |
| 7 | `CTaskFile` / `CMultiTaskFile` | `TaskFile.*`, `MultiTaskFile.*` (~132 KB) | Inherits `CXmlFileEx`; MFC/MSXML; no windows | XML schema, versioning, import/export to plugin API | **M** (XML layer swap or isolation) |
| 8 | Backup, time-log, file utilities | `TDCFileBackup.*`, `TDCtasktimelog.*` | File APIs + MFC containers | Backup policy, time-log format/analysis | **S** |
| 9 | Recurrence | `TDCRecurrence*`, `TDCRecurrenceHelper.*` | `COleDateTime`, MFC arrays | Occurrence calculation and reuse policy | **S** |
| 10 | Preferences core | `Core/Interfaces/Preferences.*` | Registry/INI + `CString`; no windows | Settings schema and defaults | **M** (API/type port) |
| 11 | Localisation core | `Core/Shared/Localizer.*`, `CEnString` | `ITransText` core is neutral; helpers use `CWnd`/`HMENU` | Translation lookup and text ownership | **M** |
| 12 | Built-in import/export | `TaskList*Exporter/Importer.*`, `TDCImportExportMgr` (~58 KB) | Mostly neutral; one UI header include (`TaskListExporterBase.cpp:7`) | Format conversion and export walking | **S–M** |
| 13 | Plugin managers | `Core/Interfaces/*Mgr.*` | Win32 DLL loading; one incidental `CWnd` call | Discovery, version validation, lifecycle | **S** |
| 14 | Plugin contracts | `ITaskList.h`, `IUIExtension.h`, `IContentControl.h`, `IImportExport.h` | Win32/HWND only, no MFC | API contracts and host messages | **S** (hosting strategy required) |
| 15 | `TDCStruct` control metadata | `TDCStruct.h:294-355` | `CTRLITEM` embeds `CWnd*`, `CRuntimeClass*`; used by dialogs/legacy bar | Custom-attribute control layout | **M** |
| 16 | `CToDoCtrl` | `ToDoCtrl.h/.cpp` (262 KB) | **God object**: view host + controller + persistence orchestrator; 149 `m_data` calls; dialogs/cursors inside commands | Tasklist commands, selection, undo, file lifecycle, archive/paste, modified state | **X** |
| 17 | `CTabbedToDoCtrl` | `TabbedToDoCtrl.*` (179 KB) | Window subclass; coordinates tree/list/tabs/plugin windows | Cross-view selection/sort/state, list rebuild from visible IDs | **VH** |
| 18 | `CFilteredToDoCtrl` | `FilteredToDoCtrl.*` (37 KB) | Window subclass; filter rebuilds mutate view state | Filter application, filtered tasks/columns, selection snapshot | **H** (model part low) |
| 19 | `CTDLTaskCtrlBase` | `TDLTaskCtrlBase.*` (152 KB) | `CWnd` + `CTreeListSyncer`; `CDC` owner-draw; native control plumbing | Column rendering/catalogue, sorting, selection predicates, row colours, header behaviour | **VH** |
| 20 | Tree view | `TDLTaskTreeCtrl.*` (57 KB) | `CTreeCtrl` + `HTREEITEM` map; `CToDoCtrlFind` | Hierarchy presentation, expansion, hit-test, label edit | **H** |
| 21 | Flat list view | `TDLTaskListCtrl.*` (57 KB) | `CListCtrl`; grouping/sort | Grouped/flat presentation and sort indicators | **H** |
| 22 | Attribute editor | `TDLTaskAttributeListCtrl` (137 KB), `TDLTaskAttributeBarCtrl` (43 KB), `TDLTaskAttributeCtrl` (20 KB) | `CInputListCtrl` + many MFC editors; implements `ITDCAttributeEditor`; DDX-like | Multi-selection aggregation, per-attribute edit semantics, auto-list data, validation | **VH** |
| 23 | Comments pane | `TDLCommentsCtrl.*` (16 KB), content plugins | Rich-edit MFC + HWND plugin controls | Content selection/format, spell-check hooks | **H** |
| 24 | Layout/splitters | `ToDoCtrlLayout.*`, shared splitter | `CWnd` panes, `CSimpleSplitter` | Pane arrangement, maximise states, persistence | **M** |
| 25 | Tasklist collection | `CToDoCtrlMgr.*` (36 KB) | Owns `CTabCtrlEx&`, `CFilteredToDoCtrl*`; tab index = identity | Open-list metadata, modified/source-control state, file paths, selection | **H** |
| 26 | Application shell | `ToDoListWnd.*` (390 KB), `TDLMainMenu`, toolbar, status bar, tray | `CFrameWnd`; ~370 command handlers; message maps; resources | File lifecycle, session restore, auto-save, command policy, preferences application | **X** |
| 27 | Dialogs (all) | 51 dialog/preference files (~596 KB) | `CDialog`/`CPropertyPage`, DDX, resources | Validation/policy in ~12 heavy dialogs (find, filter, custom attributes, recurrence, export/import, preferences, columns) | **H** aggregate (mixed; most are thin) |
| 28 | Reminders | `ToDoCtrlReminders.*` (23 KB) + reminder dialogs (~40 KB) | Service is a `CTDLShowReminderDlg` subclass; timer + Stickies + UI | Scheduling, relative dates, snooze, persistence, recurring-task transfer | **H** |
| 29 | Time tracking | `TDCTimeTracking.*`, `TDLTimeTrackerDlg*` (~55 KB) | Model takes `CTreeSelectionHelper`; dialog data holds `CToDoCtrl*` | Tracking session state, elapsed accumulation, reminders, log output | **H** |
| 30 | Source control | `TDCSourceControl.*` (~8 KB) | Takes `const CToDoCtrl&`; performs file I/O | Check-in/out, merge/conflict policy, header fields | **M** |
| 31 | Find/replace | `TDCFindReplace.*`, `TDLFindTasksDlg` (~59 KB) | Shared `FIND_STATE` + `CToDoCtrl&`; dialog builds `SEARCHPARAMS` | Search/replace traversal and result policy | **M–H** |
| 32 | Drag/drop & clipboard | `TDCTaskListDropTarget.*`, `taskclipboard.*` | MFC OLE drop target; HWND; custom clipboard formats | Move/copy/reorder semantics, references, external drops | **H** |
| 33 | Menus/toolbars/status/tray/shortcuts | `TDCMainMenu`, `TDLCustomToolbar`, `TDLStatusBar`, `trayicon`, `ShortcutManager` | MFC `CMenu`/`CToolBar`/`CStatusBar`, accelerators, resource IDs | Command enablement, shortcut mapping, dynamic menus | **H–VH** |
| 34 | `Core/Shared` widget library | 313 files, 2.5 MB | MFC widgets, `CDC`, GDI/GDI+, dark mode, rich edit, browser, OLE | Reusable only partially (files/date/XML/misc) | **X** (replace; do not port) |
| 35 | Theming/dark mode/graphics | `DarkMode.cpp` (53 KB), `GraphicsMisc`, `Themed`, `EnHeaderCtrl` | GDI/GDI+ drawing, MFC control subclassing | Colour model can be reused; rendering cannot | **H** |
| 36 | Tests / UI perf harness | `Core/TDLTest` | Data tests are headless; `CUIPerfTest` drives MFC windows and input | Domain regression suite (reusable); UI harness (rewrite) | **M** |

**Indicative roll-up** (same engineer assumptions, not a schedule): isolating and
type-porting the reusable core **2–4 person-months**; controller/shell extraction
**4–8 person-months**; new shell + task grid + attribute editor + dialogs
**12–24 person-months**; plugin hosting, remaining dialogs and hardening
**3–6 person-months**. A team of 3–4 could plausibly reach feature parity in roughly
9–15 months; one engineer should plan on 2–3+ years. The dominant uncertainty is
task-grid/attribute-editor fidelity and plugin-window hosting, not the business logic.

---

## 7. What must be rewritten vs reused

### Reuse (with a type-porting/façade pass)

- Task model, hierarchy, references, custom-attribute values and calculations.
- Undo/redo, recurrence, filter state machine, matching/sorting/calculators/formatters.
- XML file format, backup, multi-file, time-log, import/export walking.
- Plugin discovery and versioned contracts; the `ITaskList` API is already MFC-free.
- Preferences schema/defaults (new storage backend may be needed).
- Domain test suite (it already runs without a UI).

### Rewrite (no viable reuse)

- Shell/window composition, menus/toolbars/status/tray, command routing and enablement.
- Task grid (tree + list + frozen attribute columns + grouping + owner-draw).
- Attribute editor (property grid and editors) and comments pane.
- Layout/splitters, filter bar, all dialogs and preference pages.
- Drag/drop and clipboard, theming/dark mode, shortcut handling, accessibility.
- Cross-view synchronisation (`CTreeListSyncer` behaviour) in the new widget model.

### Must be extracted first (neither reuse as-is nor rewrite blindly)

- `CToDoCtrl` → `TasklistController` + `ITasklistView` + `IDialogService`.
- `CToDoListWnd` → `ApplicationController` + `IShellView`.
- `CToDoCtrlMgr` → tasklist collection model + tab adapter.
- Reminders/time tracking/source control/find-replace → services that do not take a
  view object or selection helper.
- Notification bus → typed observer interfaces, with a `WM_*` adapter for plugins.

---

## 8. Could wxWidgets or Qt replace the UI layer?

### 8.1 Common prerequisites (toolkit-independent)

1. **Controller extraction.** Without moving command logic out of `CToDoCtrl` and
   `CToDoListWnd`, either toolkit would simply inherit the same god objects.
2. **View contracts.** At minimum: tasklist view, attribute editor, comments/content
   host, filter bar, status/caption, dialog service, command/action registry.
3. **Event model.** Replace the 58-message `WM_*` bus with observers; keep a small
   Win32 adapter that still emits the registered messages for existing plugins.
4. **Type strategy.** Either port `CString`/`CArray`/`CMap`/`COleDateTime` to
   `std::wstring`/`std::vector`/`std::map`/`QDateTime` (recommended long-term), or keep an
   MFC-static-library core and expose a narrow C++ façade. Mixing MFC window classes with
   another toolkit in one process is possible but fragile; a window-free core makes the
   question moot.
5. **Plugin hosting.** Plugin windows are HWND-based; the new shell must embed native
   child windows and forward `MSG`/registered messages. This is straightforward in Qt
   (`QWindow::fromWinId`, `createWindowContainer`) and wx (`wxNativeWindow`), but the
   event pump and focus/accelerator interaction need explicit testing.
6. **Resource/localisation strategy.** The app has a large `resource.h` and translation
   DLL. Qt `.ts`/`QTranslator` or wx `wxTranslations` can replace the UI side, but the
   existing `ITransText` dictionaries and resource IDs are an asset worth bridging.

### 8.2 Qt 6 (Widgets)

**Fit:** good for a data-heavy desktop app with a large hierarchical grid.

| Current | Qt equivalent | Notes |
|---|---|---|
| `CFrameWnd` shell, menus, toolbars, status bar | `QMainWindow`, `QMenuBar`, `QToolBar`, `QStatusBar` | Menu/toolbar customisation must be rebuilt. |
| `CTreeCtrl` + `CListCtrl` task grid | `QTreeView`/`QTableView` + `QAbstractItemModel` + `QStyledItemDelegate` | Best-in-class model/view scaling; frozen columns need two views or a custom view. |
| Attribute editor | `QTableView`/custom widget + delegates, or a property browser | Per-attribute editors map well to delegates/editors. |
| View tabs | `QTabWidget`/`QTabBar` | Plugin HWND tabs need native containers. |
| Splitters/layout | `QSplitter`, `QDockWidget` | Simpler than the custom layout class. |
| Dialogs | `QDialog` + layouts | ~50 dialog/preference files to port; start with the heavy ones. |
| Drag/drop | `QMimeData`/`QDrag` | Reuse the task-clipboard model. |
| Theming | QSS/palette | Dark mode is easier than MFC subclassing, but custom grid painting remains. |
| Shortcuts | `QShortcut`/`QAction` | Needs a command-ID registry. |
| Plugins | native child windows | `createWindowContainer`; verify `MSG` forwarding. |

**Risks:** licensing/build (LGPL, dynamic linking), deployment size, a new model/view
layer to design, and the tempting but wrong idea of porting `CTreeListSyncer` literally
(Qt's model/view removes most of the need).

### 8.3 wxWidgets 3.2

**Fit:** good if the priority is native Win32 look-and-feel and a smaller conceptual
jump from MFC.

| Current | wx equivalent | Notes |
|---|---|---|
| `CFrameWnd` shell | `wxFrame`, `wxMenuBar`, `wxToolBar`, `wxStatusBar` | Closest mapping to MFC. |
| Task grid | `wxDataViewCtrl` (virtual model) or `wxTreeListCtrl`, or custom `wxScrolledWindow` | `wxDataViewCtrl` is workable but weaker than Qt's model/view for very large trees and rich delegates; expect custom drawing anyway. |
| Attribute editor | `wxPropertyGrid` (optional library) or custom grid | `wxPropertyGrid` is a separate component and may not match the current editor. |
| Tabs | `wxNotebook`, `wxAuiNotebook` | Plugin HWNDs via `wxNativeWindow`. |
| Splitters | `wxSplitterWindow` | Direct mapping. |
| Dialogs | `wxDialog` + sizers | Similar effort to Qt. |
| Drag/drop | `wxDataObject` | Reuse model. |
| Theming | native theme + custom paint | Dark mode requires custom work. |

**Risks:** the task grid still needs significant custom code; `wxPropertyGrid`/AUI add
dependencies; large-list performance and accessibility of custom-drawn surfaces must be
validated early. Plugin hosting is viable but less documented than Qt's native-window
embedding.

### 8.4 Recommendation

- **Qt 6 Widgets** if the task grid, multi-selection editing and large-list performance
  are the priority (they are the core UX). The model/view architecture matches the
  existing "one data model, many presentations" invariant better than wx.
- **wxWidgets** only if Win32-native fidelity and a smaller migration step outweigh the
  grid work; it does not remove the need to rewrite the two custom surfaces.
- **Neither toolkit replaces the core work.** In both cases the critical path is controller
  extraction and the task-grid/attribute-editor rewrite, not the choice of framework.

---

## 9. Recommended migration path (strangler)

| Stage | Goal | Exit criteria |
|---|---|---|
| **0. Freeze and measure** | Add build targets `TDCore` (no window classes) and `TDUIMfc`; add dependency checks that fail if core includes `afxwin.h`/`CWnd`. | Core target builds and the domain tests run against it. |
| **1. Event seam** | Introduce observer interfaces for modify/selection/view/filter/time-track; keep `WM_*` emission as an adapter for plugins and existing views. | New code does not need `TDCMsg.h`. |
| **2. Controller extraction** | Move command semantics, selection model, undo triggering and persistence orchestration out of `CToDoCtrl` into `TasklistController`; the existing MFC view becomes an `ITasklistView` implementation. | Controller unit tests run headless; MFC UI still works. |
| **3. Shell extraction** | Move file lifecycle, session, auto-save, command enablement and preference application into `ApplicationController`; `CToDoListWnd` becomes `IShellView`. | Command policy testable without a frame window. |
| **4. New shell + grid** | Build the Qt/wx shell, task grid and attribute editor against the extracted contracts; keep MFC dialogs temporarily via native hosting. | Feature parity for open/edit/save/filter/sort on both UIs. |
| **5. Retire MFC UI** | Port remaining dialogs/preferences, drag/drop, theming, tray, shortcuts; remove `Core/Shared` widget dependency and the MFC UI target. | MFC window classes absent from the application; plugins still load. |

Sequencing matters: attempt the new grid before controller extraction and it will
re-implement the same god object in a different toolkit.

---

## 10. Risks and caveats

- **Undo/notification ordering** is observable behaviour (modified flags, filter refresh,
  selection restore). Extraction must preserve it and needs regression tests.
- **Selection is a domain concept in this app** (filter-by-selection, time tracking,
  drag/drop, command enablement), yet it is currently owned by view helpers. Define a
  `SelectionModel` explicitly.
- **Plugin ABI and `MSG`-based contracts** constrain the event loop. Native-child hosting
  is viable but must be prototyped early, including focus, accelerators, drag/drop and
  `ProcessMessage` hooks.
- **Localisation and resource IDs** are a hidden cost: menus, dialogs, tooltips and
  command text are resource-driven.
- **Custom drawing fidelity**: dark mode, row colours, column reordering, grouping,
  frozen title column, DPI and accessibility are the visible product and the largest
  implementation risk.
- **Performance**: the app is built for large tasklists. Validate the new model/delegate
  approach with representative data (the existing `CUIPerfTest` harness is MFC-bound and
  will not port automatically).
- **File-format compatibility** is not at risk if `CTaskFile` and
  `TasklistSchemaDef.h` are reused, but any type-porting pass must keep the XML output
  byte-compatible.
- **Estimates** in §6 are engineering judgment from the code structure, not measured
  migration data.

---

## 11. Evidence index

Key anchors used in this document:

| Claim | Reference |
|---|---|
| App/frame/controller inheritance | `Core/ToDoList/ToDoList.h:31`, `ToDoListWnd.h:70`, `ToDoCtrl.h:50`, `TabbedToDoCtrl.h:44`, `FilteredToDoCtrl.h:17` |
| One data model owned by the view host | `ToDoCtrl.h:469-528` |
| Data structures and hierarchy | `ToDoitem.h:162`, `ToDoitem.h:332` |
| Data model map/structure | `ToDoCtrlData.h:43`, `ToDoCtrlData.h:249-251` |
| No UI symbols in data/semantics/persistence | zero matches for `CWnd`/`CDialog`/`CDC`/`AfxMessageBox`/`theApp`/`PostMessage`/`SendMessage` in `ToDoCtrlData.cpp`, `ToDoCtrlDataUtils.cpp`, `ToDoitem.cpp`, `TaskFile.cpp` |
| Edit command fuses model + tree + notify | `ToDoCtrl.cpp:1613-1644` |
| Save fuses persistence + confirmation + wait cursor | `ToDoCtrl.cpp:4237-4323`, `ToDoCtrl.cpp:4332`, `ToDoCtrl.cpp:4295` |
| Direct notification via window messages | `ToDoCtrl.cpp:5294`, `ToDoCtrl.cpp:7365-7367`, `TDCMsg.h` (58 messages) |
| UI metadata inside data structs | `TDCStruct.h:294-355` |
| Attribute-editor seam | `ITDCAttributeEditor.h:12`, `TDLTaskAttributeListCtrl.h:36` |
| Task-grid owner-draw and selection predicates | `TDLTaskCtrlBase.h:117-157`, `TDLTaskCtrlBase.cpp` (25 `CDC` refs) |
| Filter model vs filter bar | `TDCFilter.h:13`, `TDLFilterBar.cpp:120-145`, `TDLFilterBar.cpp:475` |
| Plugin contracts are Win32-only | `ITaskList.h` (0 MFC types), `IUIExtension.h:125`, `IUIExtension.h:377`, `IContentControl.h:152` |
| Reminder service is a dialog | `ToDoCtrlReminders.h:29` |
| Time tracking depends on selection helper and view | `TDCTimeTracking.h:29`, `TDLTimeTrackerDlgData.h` (`TRACKTASKLIST::pTDC`) |
| Tasklist manager depends on tab control | `ToDoCtrlMgr.h:36-39`, `ToDoCtrlMgr.h:185` |
| Persistence is an MFC/XML class implementing a neutral API | `TaskFile.h:86` |
| Shell command density | `ToDoListWnd.cpp` (~370 command/update entries, ~240 message-map entries) |
| Type-usage measurements | regex scan described in §1 over `Core/ToDoList`, `Core/Shared`, `Core/Interfaces`, `Core/3rdParty` |