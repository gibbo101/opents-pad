# Developer documentation

The developer guides are split by subject:

- [Building OpenTS](BUILDING.md) — supported toolchain, commands, outputs,
  build identity, and continuous integration.
- [Style](STYLE.md) — source formatting, naming, C++ use, and comments.
- [History](HISTORY.md) — source lineage and reconstruction history.
- [Rationale](RATIONALE.md) — reconstruction tools, recovered structure, and
  non-obvious implementation choices.
- [Project direction](DIRECTION.md) — long-term architecture.
- [Controller scheme](CONTROLS.md) — every button in the menus, on the
  map, in the sidebar, and the cheat codes, with a layout diagram.
- [Controller support](CONTROLLER.md) — how the controller scheme is built,
  its findings, and what is next.
- [UI system design](UI_DESIGN.md) — proposed RmlUi and ImGui integration,
  screen-level interchangeable views, and the migration from OwnerDraw.
- [The saved game format](SAVE-FORMAT.md) — the layout of a `.SAV` file: its
  header, listing fields, compressed content, and object records.

See [CONTRIBUTING.md](../CONTRIBUTING.md) for contribution and review rules.
Player and modder documentation is under [manual/](../manual/README.md). When a
guide already covers a subject, link to it instead of copying the same facts.
