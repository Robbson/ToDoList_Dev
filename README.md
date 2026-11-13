# Better TDL

This is an advanced fork of the latest [Abstractspoon ToDoList](https://github.com/abstractspoon/ToDoList_Dev) application for _Windows_.

But why? Well, the original app focuses on a specific working style that has influenced its development direction, especially in recent years. Some decisions have been made that don't work for everyone, and the user has been given no way to choose what's best for them. The new attribute editor in v9 is a good example. It's based on the idea of a _property grid_, which is great for complex applications with tons of properties. But if you only need to edit a few properties, this thing is a big waste of screen space — especially when you're working with a split view that is evenly shared between the task list and the comments editor. _Better TDL_ gives you the option to switch back to the classic attribute editor approach.

But _Better TDL_ doesn't stop there. It also focuses on issues that haven't been addressed for decades, like performance and user experience. You can read more about that in the features list.

So in the end, _Better TDL_ is just what the name implies. And everything that worked before, including your extensions, still works as expected.

## Features

### 1. The classic attribute editor is back

The old familiar horizontal attribute bar from v8.2 returns as an opt-in alternative to the 9.x property grid. If the grid feels heavy and wastes screen space, you can now have a compact bar that only shows the fields you actually use.

- Turn it on under **Preferences → UI → "Use legacy attribute editor (v8.2 style)"**.
- Works with the built-in task attributes and your custom attributes.
- The panel sizes itself to the visible fields, so no space is wasted.
- Includes the time-tracking, add-time and dependency buttons you know from the classic layout.

### 2. A dramatically faster startup

Opening _Better TDL_ with a dozen saved tasklists used to feel like a freeze. Now it's fast.

- Background tasklists no longer build their entire window up front — they are created only when you first open the tab.
- The pointless "untitled" placeholder list that used to be created and thrown away at startup is gone.
- Result: startup scales almost flat with the number of open documents, and the window appears in a fraction of the time.

### 3. A cleaner comments editor

The comments area gave up its own toolbar row so you get more room to write.

- Insert Date, Insert Time and Insert Date and Time now live on the main toolbar, together with the content-format selector.
- The comments field expands into the space that was reclaimed, giving you more vertical room.
- Format switching, keyboard shortcuts and menu commands all keep working exactly as before.

### 4. Type without the mouse pointer in the way

Just like the classic Windows behaviour: when you start typing, the mouse pointer disappears and only comes back when you move the mouse again.

- Works in the comments editor — both plain text and rich text.
- Works regardless of where the pointer happens to be: over the editor, the toolbar or the window frame.
- Matches the behaviour you already get in the search, title, tags and category fields.

### 5. Quick Find highlights your matches

Searching with Quick Find is now more useful, because you can see exactly where the match is.

- When Quick Find selects a task because the search term appears in its comments, the matching text is highlighted right in the comments editor.
- Works while you type and with Quick Find Next/Previous.
- Supports quoted phrases and multi-word searches the same way Quick Find itself does.

### 6. Smart auto-indent in the comments editor

Writing structured notes feels natural again.

- Pressing **Enter** keeps the indentation of the current line, so lists and nested notes stay aligned.
- Bullet lines (`-`, `*`, `+`, `->`, `=>`) continue neatly under the bullet text instead of repeating the marker.
- Word-wrapped lines are visually aligned with the text after the indent, so long paragraphs stay readable.

### 7. Instant tasklist switching with Ctrl+Tab

Jump between open tasklists the way you do in modern IDEs.

- Hold **Ctrl+Tab** to open a most-recently-used switcher that lists your open tasklists in last-used order.
- The previously used tasklist is preselected, so releasing **Ctrl** switches straight to it.
- Tap **Tab** to move through the list, **Shift+Tab** to go back, **Enter** to accept and **Esc** to cancel.
- Tasklists with unsaved changes are highlighted, and ones that haven't been loaded yet are shown greyed out.

### 8. Built for large tasklists

Performance is treated as a feature, not an afterthought, with an automated UI harness that measures scrolling and resizing of real, large tasklists. It keeps the everyday experience smooth even when your lists grow into the thousands of tasks.
