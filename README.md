# Abstractspoon ToDoList Fork

## Features

### Forward-ported: legacy attribute editor (v8.2 style)

The classic horizontal attribute bar is available again as an opt-in alternative to the
9.x property-grid editor.

- Enable via **Preferences → UI → “Use legacy attribute editor (v8.2 style)”**.
- Supports the built-in task attributes and custom attributes.
- Panel height follows the visible fields; the unused drag handles are hidden.
- Includes time-tracking, add-time and dependency buttons.

**Status: ~90% complete.** Core editing, layout, custom attributes and auxiliary controls
work. Remaining work is polish: date-picker checkbox painting, custom-field visibility and
auto-list persistence, and pixel-perfect parity with v8.2.

Implementation notes: [customized/CUST-001-legacy-attribute-editor.md](customized/CUST-001-legacy-attribute-editor.md).
