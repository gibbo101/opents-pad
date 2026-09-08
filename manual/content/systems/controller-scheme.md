---
title: Controller scheme
summary: "Presents console-style screens in place of the dialogs while a controller is in use, and drives the tactical pointer, the sidebar, and the game's commands from the controller's buttons."
category: interface-controls
keys:
  - ControlScheme
  - PromptStyle
  - PadPointerSpeed
  - PadFastSpeed
  - PadScrollSpeed
  - PadSnap
  - PadZoomWidth
  - PadZoomHeight
  - PadSidebarSticky
related:
  - type: system
    id: sidebar
  - type: system
    id: waypoint-paths
  - type: command
    id: fixed:console-menu-navigate
  - type: command
    id: fixed:console-menu-accept
  - type: command
    id: fixed:console-menu-back
---

The controller scheme is one of the two schemes [`ControlScheme=`](/keys/controlscheme/) selects. It reads the first connected XInput controller and names its buttons by role: accept, back, a third and a fourth face button, the menu and view buttons, the left and right shoulders and triggers, and the two sticks with their clicks. The keyboard and mouse keep working under it; the keyboard scheme is the game without this page.

## Choosing the scheme

`Auto` selects the controller scheme while a controller is connected. Steam hands a game its virtual controller a moment after the window exists, so `Auto` keeps checking through the startup movies and, when the system lists a controller-class device or a controller was seen earlier, waits up to two seconds for one before the first menu screen; with no such device the shell starts at once. A controller button pressed on a keyboard-scheme menu screen switches to the controller scheme and shows that screen again as its console version. Holding the menu and back buttons together for a second switches to the controller scheme from any screen.

## Screens

Under the controller scheme the shell, the scenario loading screen, the in-game menu, the mission briefing, and the score screens render at 640x400, the size their artwork was drawn for, and the game's own render size returns for play. A console-style screen is a list of rows with one in focus: the d-pad or left stick moves the focus, left and right step a value, a shoulder button or Left Shift steps it five at a time, accept takes the screen or the focused row's own action, and back leaves it. The mouse can point at a row, click it, and click the two prompts.

| Screen | Console-style form |
| --- | --- |
| Game select and the main menu pages | The original artwork with the focus moving between the buttons by position; back takes the page's Back button |
| Select Campaign | A side select with the two emblems and the difficulty; left and right pick the side |
| Skirmish setup and the LAN host lobby | The dialog's settings as rows with a numbered map list and preview; accept from any row starts the game, and the game speed row starts at the saved option |
| LAN game list and guest lobby | The found games, then the host's rows read only with the player's side and color requested from the host |
| Options | Control Scheme, Button Prompts, a Zoom row in place of a resolution list, scale mode, stretch movies, game speed, the three controller paces, Unit Snap, detail, campaign difficulty, cameo text, action lines, tool tips, and rows to the Audio and Controls screens |
| Audio | Music, sound, and voice volumes; in play also shuffle, repeat, the track to play, and Stop Music |
| Controls | A drawn controller with callouts, paged across the face buttons, the shoulders and triggers, and the sticks |
| Load Mission | The saves newest first |
| In-game menu | Game Options, Audio Options, Controls, Mission Briefing, Save Game, Load Game, Restart Mission, Abort Mission, and Return To Mission; the last two ask as a row whose value flips between No and Yes |
| Mission briefing | Accept turns the page; back plays the mission video |
| Score screens | Accept continues; a hall of fame place takes its name on the on-screen keyboard |

Names and LAN chat are typed on an on-screen keyboard: the menu button opens it from any row of the skirmish and LAN games screens and from the save box, accept types the focused key, the third face button deletes, the fourth adds a space, the menu button or accept on Done finishes, and back leaves the text as it was.

The accept and back prompts carry the button's glyph in the set [`PromptStyle=`](/keys/promptstyle/) selects.

## In play

### Pointer

The left stick and the d-pad move the pointer, and accept and back are the left mouse button and a right-button tap at it, so selecting, ordering, deploying, placing, and band boxing follow the mouse paths. The stick moves at a steady pace for its deflection; the d-pad starts at half pace and reaches full pace within 150 ms of a press, so a tap stays within a cell. Both paces are distances on the map, measured against a 600-pixel-high view, so a push covers the same ground at every zoom and on every display. What the pointer cannot travel at the screen's edge scrolls the map at the pointer's pace. The right stick scrolls the map as a right-button drag does, with a squared response.

[`PadPointerSpeed=`](/keys/padpointerspeed/) and [`PadScrollSpeed=`](/keys/padscrollspeed/) scale the pointer and the right stick's scroll from `1` to `10`, `5` being the built-in pace, `10` twice it, and `1` a fifth of it. Holding the right shoulder multiplies the pointer's pace, and with it the edge scroll, by one plus 0.24 times [`PadFastSpeed=`](/keys/padfastspeed/).

### Snap

When the stick and the d-pad come to rest, the game resolves the pointer's map position as it would for a click and, unless the pointer is already over an object, searches the cells around it for the closest unit, infantry, aircraft, or building within [`PadSnap=`](/keys/padsnap/) eighths of a cell, leaving out cloaked objects of other players, and warps the pointer onto its center. The pointer is never pulled while it is traveling or while accept is held.

### Zoom

The game renders at a height from the ladder `480`, `540`, `600`, `660`, `720`, `768`, `840`, `900`, `1080`, `1200`, and `1440`, never above the display's own height, with the width on the display's shape so no display shows bars. The right shoulder with the right stick up zooms in a step and down zooms out, one step per push and another every quarter second the stick stays pushed; each step keeps the center of the view where it was and saves the pair as [`PadZoomWidth=`](/keys/padzoomwidth/) and [`PadZoomHeight=`](/keys/padzoomheight/). A pair that is empty or off the display's shape starts at `768` high, `600` on a Steam Deck. The right stick does not scroll while the right shoulder is held.

The sidebar and the bar across the top of the map are drawn apart from the map at the height of the sidebar's own five-row panel and scaled to fill the display's height, so neither changes size with the zoom, and the map's columns take the shape of the display left beside the sidebar and under the bar.

### Sidebar

The sidebar is a fixed grid: two columns, the player's side on the left and the other side on the right, with rows for structures, infantry, vehicles, and aircraft, and a bottom row holding the current superweapon and a cell that cycles to the next; the four mode buttons and the radar sit above it. It starts away from the map, which takes the whole width under the bar, and slides in over the map in 200 ms when the fourth face button takes the controller onto it; the fourth face button again, or anything that lets the controller go such as placing a building, slides it out. The right shoulder with the fourth face button slides it in sticky, so it stays when the controller leaves, and the same chord releases it; [`PadSidebarSticky=`](/keys/padsidebarsticky/) records that state. While the panel is in, the view may scroll past the map's edge by the strip the panel covers. The bar ends in a Sidebar tab that a mouse click also uses, and the credits are on the panel alone.

Inside the sidebar the d-pad or stick moves between the cells, the mode buttons, and the radar. Accept on a section builds or places its current or last item, or opens the section's grid when it has none; in a grid accept builds the marked item. Back on a building section holds, then cancels; on an idle section it opens the grid; in a grid it cancels the marked build or steps back to the sections. The third face button opens or closes the grid without canceling anything. From the map, the right shoulder with back works the parked cell: place, build the last item again, add one more, or queue the marked item. Placing a building, aiming a superweapon, or picking a mode button returns focus to the map with the ghost, target, or mode cursor on the pointer. On the radar, accept held moves a marker and releasing it jumps the view there. Cheat codes are entered with back on the mode buttons, in solo games only.

### Commands

The other buttons run the game's own named commands, so the keyboard bindings stay the source of their behavior. The third face button cycles the sidebar modes repair, sell, power, waypoint, and off, passing over a mode the game refuses. The stick clicks deploy the selection and center the view on the base. The right trigger scatters, or in waypoint mode takes back the last waypoint while the path has one; with the right shoulder it guards. The left trigger with a face button makes team 1 to 4 from the selection, and the left shoulder with the same selects it, or centers the view on it when pressed twice within 400 ms. The right shoulder with the left shoulder force fires at the pointer, and with the left trigger force moves there, the moment both are held. The view button allies with the selected unit's owner, which the game allows only against a human player in a game with allies on. Accept held still for half a second on one of the player's units selects every unit of its type on screen, and held on a little longer every one on the map; held still on the ground it selects every combat unit on screen, then every one on the map, a combat unit being one that carries a weapon or gains one by deploying.
