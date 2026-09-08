<p align="center">
  <img src="https://raw.githubusercontent.com/OpenTS-Developers/.github/main/assets/opents-logo.png" alt="OpenTS" width="512">
</p>

<p align="center">
  <a href="https://github.com/OpenTS-Developers/OpenTS/releases"><img src="https://img.shields.io/github/downloads/OpenTS-Developers/OpenTS/total?label=downloads" alt="Downloads"></a>
  <a href="https://github.com/OpenTS-Developers/OpenTS/actions/workflows/engine.yml"><img src="https://github.com/OpenTS-Developers/OpenTS/actions/workflows/engine.yml/badge.svg" alt="Engine build"></a>
  <a href="https://opents-developers.github.io/OpenTS/"><img src="https://github.com/OpenTS-Developers/OpenTS/actions/workflows/manual-pages.yml/badge.svg" alt="Manual"></a>
  <a href="https://www.patreon.com/c/ZivDero"><img src="https://img.shields.io/badge/Patreon-ZivDero-F96854?logo=patreon&logoColor=white" alt="Patreon"></a>
</p>

# OpenTS on a controller

This fork of [OpenTS](https://github.com/OpenTS-Developers/OpenTS) adds a
controller scheme to the engine and is played on a Steam Deck. Under it the
shell menus become console screens, the sidebar is a fixed grid of build
sections that slides over the map, and every order on the map is a pad
chord, in the manner of *Command & Conquer: Red Alert Retaliation* on the
PlayStation. The keyboard and mouse scheme is the engine as upstream ships
it: every change is gated on the controller scheme.

[Controller scheme](docs/CONTROLS.md) lists every button, screen and chord.
[Controller support](docs/CONTROLLER.md) records how it is built and what is
next. The fork tracks upstream `main`; engine fixes found along the way go
back upstream as pull requests, and the controller scheme itself stays here
for now.

<p align="center">
  <img src="docs/screenshots/main-menu.jpg" alt="The main menu as a console screen" width="640">
  <img src="docs/screenshots/options.jpg" alt="The options screen" width="640">
  <img src="docs/screenshots/controls.jpg" alt="The Controls screen with the drawn pad and callouts" width="640">
  <img src="docs/screenshots/play-sidebar-in.jpg" alt="Play with the section-grid sidebar slid in" width="640">
  <img src="docs/screenshots/play-sidebar-out.jpg" alt="Play with the sidebar slid out and the Sidebar tab on the bar" width="640">
  <img src="docs/screenshots/pause-menu.jpg" alt="The pause menu" width="640">
</p>

The rest of this page is upstream's.

# OpenTS

OpenTS is a community-led, open-source reconstruction of *Command & Conquer:
Tiberian Sun*. Instead of patching or extending the retail executable, it
rebuilds the engine as a standalone program.

OpenTS gives equal weight to two goals: maintaining a playable engine and
providing a capable platform for modding and engine development. Work on one
goal should not come at the expense of the other.

OpenTS is:

- an independent, community-led source reconstruction targeting Tiberian Sun
  2.03 Firestorm;
- a playable engine based on Electronic Arts' GPL-released source for related
  Command & Conquer games and Tiberian Sun-specific reverse engineering; and
- the active base for maintenance, documentation, modernization, bug fixes,
  and new modding capabilities.

OpenTS is not:

- a remaster or remake;
- an official Electronic Arts source release; or
- a distribution of the original game assets.

OpenTS is an independent community project and is not affiliated with or
endorsed by Electronic Arts.

## Community

- Discord: <https://opents.net/discord>
- Bug reports and proposals:
  [GitHub issues](https://github.com/OpenTS-Developers/OpenTS/issues)

## Downloads

- **Releases** are the recommended builds. Each zip on the
  [releases page](https://github.com/OpenTS-Developers/OpenTS/releases)
  contains `Game.exe`, `Language.dll`, and `Game.pdb`.
- **Nightly builds** are development snapshots from the
  [Engine nightly](https://github.com/OpenTS-Developers/OpenTS/actions/workflows/engine-nightly.yml)
  workflow. Download the latest one without a GitHub account through
  [nightly.link](https://nightly.link/OpenTS-Developers/OpenTS/workflows/engine-nightly/main).
  Nightlies contain the latest merged changes without release validation and
  expire after 90 days.

## Installing

1. Install Tiberian Sun from Command & Conquer The Ultimate Collection on
   Steam or the EA App.
2. Extract the release zip into the Tiberian Sun game directory.
3. Run `Game.exe`.

OpenTS supplies the engine, not the game data: the installation above
provides the original assets. There is no installer, and no extra runtime
library or launch argument is required. Windows is supported; Wine may work,
but there is no supported native Linux build. The engine asks Windows for the
UTF-8 code page, which needs Windows 10 version 1903 or newer. Older Windows
keeps its own code page, so game text still shows, but a path or file name
holding a character that code page lacks may fail.

## Documentation

The [OpenTS manual](https://opents-developers.github.io/OpenTS/) documents
setup, runtime behavior, INI configuration, mapping, and source-level
internals.

## State and plans

Release 0.1.0 runs the full Tiberian Sun 2.03 Firestorm game. The GDI, Nod,
and Firestorm campaigns, skirmish, and save/load have received full
play-through testing. LAN multiplayer has had more limited testing. No
user-visible regression from the original game is currently known. The
renderer uses
[bgfx](https://github.com/bkaradzic/bgfx) and supports modern resolutions
through 4K, including ultrawide.

The first of the project's three development milestones is the current focus:

1. CnCNet and CnCNet client support, including porting the parts of
   [ts-patches](https://github.com/CnCNet/ts-patches) this requires.
2. Feature parity with
   [Vinifera](https://github.com/Vinifera-Developers/Vinifera) and the rest
   of ts-patches.
3. Extending Tiberian Sun with new features, striving toward feature parity
   with Red Alert 2 and Yuri's Revenge, and growing engine capabilities that
   match or exceed the popular Yuri's Revenge engine extensions.

Alongside these goals, the engine is modernized incrementally toward an
entity-component architecture, and new development is shaped so that
migration stays possible. [Project direction](docs/DIRECTION.md) explains the
reasoning.

## Building

OpenTS builds as a 32-bit Windows target with Visual Studio 2022 and CMake.
[Building OpenTS](docs/BUILDING.md) documents the exact requirements,
commands, and outputs.

## Contributing

Bug reports, proposals, documentation improvements, and focused pull requests
are welcome. Review capacity is limited, so discuss non-trivial work with the
maintainers before implementing it. [CONTRIBUTING.md](CONTRIBUTING.md)
explains the current priorities and review policy; source conventions are in
[Style](docs/STYLE.md).

## Origins

OpenTS continues the community reconstruction preserved in the
[TibSun archive](https://github.com/OpenTS-Developers/TibSun), built from
Electronic Arts' published source for related Command & Conquer games and
completed through reverse engineering against the original executable.
[History](docs/HISTORY.md) records the reconstruction's lineage and methods.

## License and acknowledgements

OpenTS is licensed under the GNU General Public License, version 3 or later.
Material derived from Electronic Arts source remains subject to the additional
GPL Section 7 terms in [LICENSE.md](LICENSE.md).
[Third-party notices](THIRD_PARTY_NOTICES.md) identify bundled dependencies
and their licenses.

[ACKNOWLEDGEMENTS.md](ACKNOWLEDGEMENTS.md) thanks the people, projects, and
communities whose work made OpenTS possible.
