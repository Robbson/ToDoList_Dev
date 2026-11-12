# AGENTS.md

Starting point for agents working in this repository. Read this first, then
[ARCHITECTURE.md](ARCHITECTURE.md). Detailed references:
[PROJECT_SCOPE.md](PROJECT_SCOPE.md), [DATA_MODEL.md](DATA_MODEL.md) and
[customized/index.md](customized/index.md) (all intentional divergences from upstream).

## Repository at a glance

- Product: **ToDoList**, a Windows desktop task manager written in C++/MFC (Unicode).
- Repo layout: `Core/` (native app, libraries, tools) and `Plugins/` (managed/native
  extensions, separate solution).
- Main solution: `Core/ToDoList_Core.sln`.
- **Start project:** `Core/ToDoList/ToDoList.vcxproj` → `ToDoList.exe`.
- Most implementation code is in `Core/ToDoList/*.cpp` but is compiled into the static
  library `Core/ToDoListLib`; the exe project compiles only `StdAfx.cpp` and
  `ToDoList.cpp` and links the library.
- Platforms: `Win32` only. Configurations: `Unicode Debug`, `Unicode Release`.
  Project toolset: `v143` (VS2022), despite legacy `ToolsVersion="14.0"`.
- Source size: ~133k lines in `Core/ToDoList` (167 `.cpp`, 175 `.h`).

## Scope discipline

Follow the user's analysis focus. Default scope for core work:

**Analyze:** `Core/ToDoList`, `Core/ToDoListLib`, `Core/Interfaces`, `Core/Shared`,
`Core/3rdParty`, `Core/3rdParty/Detours`.

**Include only when required:** `Core/TDLTest`, `Core/TDLUpdate`, and optional runtime
DLL projects (`EncryptDecrypt`, `TransText`, `MySpellCheck`, `RTFContentCtrl`,
`FtpStorage`, import/export DLLs, UI-extension DLLs).

**Ignore unless explicitly asked:** `Core/TDLUninstall`, `Core/TDLTransEdit`, and the
entire `Plugins/` tree. Do not read or modify third-party sources to answer a core
question; use the interfaces and Core call sites instead.

## Build and test

No lint or typecheck target exists. Build is the compile check; tests are the behaviour
check.

```bat
:: from repo root
BuildDebug.bat
BuildRelease.bat
```

```bat
:: single solution build (preferred on a modern machine)
msbuild Core\ToDoList_Core.sln /t:Build /p:Configuration="Unicode Debug" /p:Platform=Win32 /m
```

```bat
:: unit tests
Core\TDLTest\Unicode_Debug\TDLTest.exe
```

```bat
:: UI performance/artefact harness (interactive desktop; takes over the mouse)
:: runs only CUIPerfTest; see customized/CUST-004-ui-perf-harness.md
Core\TDLTest\Unicode_Debug\TDLTest.exe -ui
Core\TDLTest\Unicode_Debug\TDLTest.exe -ui -uiscroll   :: ListScrollSync only
Core\TDLTest\Unicode_Debug\TDLTest.exe -ui -uiscroll -notches 2 -delay 0 -record :: fast CUST-002 check
Core\TDLTest\Unicode_Debug\TDLTest.exe -ui -uisplitter :: SplitterDrag only
Core\TDLTest\Unicode_Debug\TDLTest.exe -ui -uihold     :: leave app open for manual testing
Core\TDLTest\Unicode_Debug\TDLTest.exe -ui -uiscroll -uihold -record :: manual + JPEG frames
Core\TDLTest\Unicode_Debug\TDLTest.exe -ui -uifix      :: fixtures only, no GUI
Core\TDLTest\Unicode_Debug\TDLTest.exe -ui -nodrag     :: locate windows, no input
```

Notes:

- `BuildDebug.bat` builds Core, then runs `TDLTest`, then builds `Plugins/ToDoList_Plugins.sln`.
- The batch files hard-code old MSBuild/VC6 paths; on a modern install invoke MSBuild
  directly or update the paths. Do not “fix” project files just to satisfy the scripts.
- Outputs: `Core/<Project>/Unicode_Debug|Unicode_Release`; `ToDoList.exe` is also copied
  to `Plugins/debug|release`.
- Runtime plugins are loaded from the **executable directory only**. Copying a DLL
  elsewhere will not make it load.
- The `-ui` harness is opt-in, needs a foreground interactive desktop, and is the
  acceptance check for splitter/scroll rendering changes. Normal `TDLTest` runs skip
  it. Debug `TDLTest` logs assertions to `%TEMP%\TDLTest_asserts.log` instead of
  showing a dialog; set `TDL_ASSERT_LOG=0` to restore the dialog.

## Debug assertions

Debug builds show an MFC assertion dialog by default, which blocks automation. To log
assertions instead, set the environment variable before launching a Debug build:

```bat
set TDL_ASSERT_LOG=1
Core\ToDoList\Unicode_Debug\ToDoList.exe
```

- `TDL_ASSERT_LOG=1` writes `%TEMP%\ToDoList_asserts.log`.
- `TDL_ASSERT_LOG=<path>` writes to the given file.
- The log includes a stack trace resolved with `dbghelp`/PDB when available.
- The hook suppresses the dialog and continues execution. This is intended for
  diagnosis, not production.
- Release builds ignore the variable.

## Code conventions

- `.editorconfig`: **tabs**, tab width 4, trim trailing whitespace. Match surrounding
  style; do not reformat untouched code.
- MFC idioms: `CString`, `COleDateTime`, `CArray`/`CMap`, message maps, `CRuntimeDlg`,
  `CEn*` wrappers from `Core/Shared`.
- Hungarian-ish naming is pervasive: `m_` members, `C` classes, `dw`/`n`/`s`/`b`/`a`
  prefixes. Preserve it.
- Do **not** add comments unless they explain non-obvious logic; the codebase is largely
  comment-light.
- Prefer extending existing helper classes (`CToDoCtrlData`, `ToDoCtrlDataUtils`, shared
  controls) over adding parallel implementations.
- Keep the fixed enum order of `TDC_ATTRIBUTE` / `TDC_COLUMN` stable; append, do not
  reorder.

## Orientation map (where to look)

| Question | Read |
|---|---|
| App startup, command line, single-instance | `Core/ToDoList/ToDoList.h/.cpp` |
| Frame, menus, toolbars, status bar, tasklist tabs, commands | `Core/ToDoList/ToDoListWnd.h/.cpp` |
| Which tasklists are open / active tab | `Core/ToDoList/ToDoCtrlMgr.h/.cpp` |
| Tasklist surface, layout, data ownership | `Core/ToDoList/ToDoCtrl.h/.cpp` |
| Tree/list view tabs, extensions | `Core/ToDoList/TabbedToDoCtrl.h/.cpp` |
| Filter state and presentation rebuild | `Core/ToDoList/FilteredToDoCtrl.h/.cpp` |
| Shared task-view rendering/selection | `Core/ToDoList/TDLTaskCtrlBase.h/.cpp` |
| Tree view / flat list view | `TDLTaskTreeCtrl.*`, `TDLTaskListCtrl.*` |
| Task fields and hierarchy | `Core/ToDoList/ToDoitem.h/.cpp` |
| In-memory DB, edits, undo entry points | `Core/ToDoList/ToDoCtrlData.h/.cpp` |
| Formatting/calculation/matching/sorting | `Core/ToDoList/ToDoCtrlDataUtils.h/.cpp` |
| `.tdl` XML read/write | `Core/ToDoList/TaskFile.h/.cpp` |
| Attribute/column enums | `Core/Interfaces/IEnums.h`, `Core/ToDoList/TDCEnum.h` |
| XML vocabulary | `Core/Interfaces/TasklistSchemaDef.h` |
| Notifications | `Core/ToDoList/TDCMsg.h` |
| Plugin contracts | `Core/Interfaces/ITaskList.h`, `IUIExtension.h`, `IImportExport.h`, `ITaskListStorage.h`, `IContentControl.h`, `IPreferences.h`, `ITransText.h`, `IEncryption.h` |
| Preferences/localisation/theming | `Core/Interfaces/Preferences.*`, `Core/Shared/Localizer.*`, `Core/Interfaces/UITHEME.h`, `UIThemeFile.*` |

## Core invariants (high-risk if violated)

1. Each tasklist has exactly one `CToDoCtrlData`. Views render it; they never own tasks.
2. Task identity is the `DWORD` task ID, not a tree/list handle.
3. `TODOITEM` (payload map) and `TODOSTRUCTURE` (hierarchy) must stay consistent on
   add/delete/move/undo.
4. Route edits through `CToDoCtrlData` + undo (`CUndoAction`) so modified flags,
   notifications and redo stay correct.
5. Filters rebuild presentations; they must not mutate the data model.
6. Plugin DLLs are discovered in the exe directory; keep built-in formats/content working
   with no plugins.
7. File-format changes require coordinated updates in `TaskFile`, `TasklistSchemaDef`,
   `ToDoCtrlDataUtils` (export/import) and the custom-attribute code, plus a format
   version bump.

## Working safely

- Read the surrounding class before editing; files are large and heavily cross-referenced.
- Avoid “drive-by” reformatting, renames, or comment additions in giant files
  (`ToDoListWnd.cpp`, `ToDoCtrl.cpp`, `ToDoCtrlDataUtils.cpp`).
- Do not change `.vcxproj`/`.sln` project membership for analysis tasks.
- Do not touch `Plugins/` or third-party sources for core analysis.
- Do not edit generated resource IDs by hand unless the task is explicitly about
  resources.
- After code changes, build the affected configuration and run `TDLTest` when practical.
- Never commit unless the user explicitly asks.

## Customization workflow

This repository is a customized fork. Before changing upstream behaviour:

1. Add or update an entry under `customized/` (index plus `CUST-nnn-<keywords>.md`) with the goal,
   the upstream starting point, files, risks and tests.
2. Prefer an isolated adapter/new class over editing giant upstream files; keep changes
   easy to re-apply after upstream merges.
3. Mark the status (`Proposed`, `In progress`, `Done`, `Deferred`) and update it when the
   change is implemented or revised.
4. Default to preserving upstream behaviour unless the user explicitly asks to change it.

## Analysis output expectations

When asked to analyze or document, prefer producing/updating the root docs:

- `ARCHITECTURE.md` — system structure, flows, invariants.
- `DATA_MODEL.md` — task/attribute/persistence details.
- `PROJECT_SCOPE.md` — included/excluded projects and dependencies.
- `customized/index.md` + `customized/CUST-nnn-*.md` — all customized behaviour and its rationale.
- `AGENTS.md` — this file.

Include concrete `path:line` references for key claims. Keep raw file dumps out of
documentation; summarize with evidence.

