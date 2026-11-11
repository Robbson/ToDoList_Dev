# Customization Index

This folder tracks every intentional divergence from upstream ToDoList so changes can be
reviewed, tested and re-applied after upstream updates.

- Upstream baseline: ToDoList **9.3.DP7** (file version `9,2,997,7`), branch `custom` at
  `63fc1aacc`, based on upstream `d3b7a917e`.
- Architecture: [../ARCHITECTURE.md](../ARCHITECTURE.md)
- Data model: [../DATA_MODEL.md](../DATA_MODEL.md)
- Project scope: [../PROJECT_SCOPE.md](../PROJECT_SCOPE.md)

## Folder layout

```text
customized/
  index.md                       this file: conventions, status, change index
  CUST-001-legacy-attribute-editor.md
  CUST-002-<keywords>.md         one file per customization
```

## Conventions

- Give every change an ID: `CUST-001`, `CUST-002`, …
- Name files `CUST-nnn-<one-or-few-keywords>.md`.
- Record: goal, upstream starting point, files touched, settings, risks, test notes.
- Keep custom code in clearly marked blocks/files so future upstream merges remain
  possible.
- Do not rewrite upstream files wholesale when a contained adapter or new class is
  sufficient.
- Update the index and the change file when a change is designed, implemented, revised or
  abandoned.

## Status legend

| Status | Meaning |
|---|---|
| Proposed | Analysed and designed; not implemented |
| In progress | Implementation started |
| Done | Implemented, built and tested |
| Deferred | Deliberately postponed or rejected |

## Change index

| ID | Title | Status | Document |
|---|---|---|---|
| CUST-001 | Legacy (v8.2-style) attribute editor, selectable | In progress (Phase 1 done) | [CUST-001-legacy-attribute-editor.md](CUST-001-legacy-attribute-editor.md) |
