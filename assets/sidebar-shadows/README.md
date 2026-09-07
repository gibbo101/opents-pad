# Sidebar section shadows

Black silhouettes on transparency, one square PNG per slot, drawn over a
section's factory cell on the controller sidebar. `tools/sidebarshadows.py`
bakes whichever of these exist into `code/sidebarshadowdata.h`; a missing
file leaves its cell without a shadow.

Slots: `gdi_structures`, `nod_structures`, `gdi_infantry`, `nod_infantry`,
`gdi_vehicles`, `nod_vehicles`, `gdi_aircraft`, `nod_aircraft`.

Only original artwork belongs here. Do not trace the game's own sprites.
