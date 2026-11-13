# CUST-012 Branding and About dialog

## Goal

Show the fork's product name and release label in the application title and About dialog,
driven by the executable's version resource, and prevent the About copyright notice from
being clipped.

## Upstream starting point

- `CToDoListWnd::GetTitle` hard-coded `"ToDoList %s"`.
- `CToDoListWnd::GetVersion` derived the release label only from the third version
  component (`997`–`1000`); a normal version such as `1.0.0.0` therefore always showed
  "Stable Release".
- The About dialog placed the copyright static on the same row as the title edit, where it
  was wider than the space available.

## Files touched

- `Core/Shared/FileMisc.h`, `Core/Shared/FileMisc.cpp`
  - New `GetAppProductName()` and `GetAppSpecialBuild()` read `ProductName` and
    `SpecialBuild` from the executable's `VS_VERSION_INFO`.
- `Core/ToDoList/ToDoListWnd.cpp`
  - `GetTitle` uses `GetAppProductName()` (falling back to `"ToDoList"`).
  - `GetVersion` prefers `SpecialBuild` for the extended `"<version> (<label>)"` text,
    falling back to the existing pre-release/localised label.
- `Core/ToDoList/ToDoList.rc`
  - `IDD_ABOUT_DIALOG` copyright moved to its own row below the title; the remaining
    controls shifted down and the dialog made 14 DLUs taller.

## Behaviour

- The version resource's `ProductName` (currently "Better TDL") is displayed by the About
  dialog and other callers of `CToDoListWnd::GetTitle`.
- The version resource's `SpecialBuild` (currently "Alpha") is displayed as the release
  label, e.g. `Better TDL 1.0.0.0 (Alpha)`.
- If `SpecialBuild` is empty, the upstream `IDS_*_RELEASE` strings are used.
- The About copyright now spans the full dialog width on its own line.

## Risks

- `CToDoListApp::AfxGetAppName()` returns `GetTitle(FALSE)`, so the process/app name used
  in a few shared helpers now begins with "Better TDL" instead of "ToDoList".
- Installing a release without `SpecialBuild` or `ProductName` falls back gracefully.

## Test notes

- Built `Core/ToDoList/ToDoList.vcxproj` (`Unicode Debug`, `Win32`) successfully.
- Manual check: open **Help > About** and confirm the title/release label and the
  unwrapped copyright line.
