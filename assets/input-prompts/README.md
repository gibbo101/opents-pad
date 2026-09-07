# Input prompt glyphs

The controller button glyphs the console-style screens draw beside their
prompts. They are the face buttons from Kenney's Input Prompts pack, version
1.5 (https://kenney.nl/assets/input-prompts), released under Creative
Commons Zero; `LICENSE.txt` is the pack's licence text. Each file is the
pack's 64x64 "Default" PNG under a name that says which button it stands for:

| File prefix | Pack folder | Files used |
| --- | --- | --- |
| `xbox_` | Xbox Series | the `xbox_button_color_*` set and `xbox_button_menu` |
| `playstation_` | PlayStation Series | the `playstation_button_color_*` set and `playstation5_button_options` |
| `deck_` | Steam Deck | the `steamdeck_button_*` set and `steamdeck_button_options` |

`accept`, `back`, `third`, and `fourth` are A, B, X, Y on an Xbox pad or the
Deck and cross, circle, square, triangle on a PlayStation pad; `menu` is the
Menu, Options, or Start button.

`python tools/padglyphs.py` bakes these files into `code/padglyphdata.h`,
which is committed so a build needs neither Python nor the PNGs. Run it
again after replacing a file.
