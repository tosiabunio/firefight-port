# Fire Fight level format

This is the reference for anyone writing a level editor or a level by hand. It covers every file that makes up a level, what each field means, how the game draws and uses it, and which mistakes the game catches.

The original editor (LED) was not kept, so the source for all of this is the loader and the game code. See [How this was checked](#how-this-was-checked) for what was tested and how.

**Contents**

1. [What makes up a level](#1-what-makes-up-a-level)
2. [Coordinates and planes](#2-coordinates-and-planes)
3. [Text file syntax](#3-text-file-syntax)
4. [The level file: `<world>.def`](#4-the-level-file-worlddef)
5. [What happens to objects at load time](#5-what-happens-to-objects-at-load-time)
6. [Types: what an object does](#6-types-what-an-object-does)
7. [The mission script: `<world><n>m.tdf`](#7-the-mission-script-worldnmtdf)
8. [Sprites: finding and drawing them](#8-sprites-finding-and-drawing-them)
9. [Network levels](#9-network-levels)
10. [Adding a level: a complete example](#10-adding-a-level-a-complete-example)
11. [Testing a level](#11-testing-a-level)
12. [Checks an editor should make](#12-checks-an-editor-should-make)
13. [Where it lives in the source](#13-where-it-lives-in-the-source)

## 1. What makes up a level

A **mission name** is a world name followed by one digit: `green6` is sublevel 6 of world `green`. The game splits the name at its last character, so a sublevel number is always 1–9.

A **world** shares its art and one `.def` file between up to 9 **sublevels**. Only the sublevel being played is loaded.

| File | Contents | Found through |
|---|---|---|
| `data/!global/missions.tdf` | Mission order and music | logical name `level_descript` |
| `data/<world>/<world>.def` | **The level:** type table, sprite table, and each sublevel's objects | `area <world>` → `source` in `<world>.dir` |
| `data/<world>/<world><n>t.tdf` | What the types of this mission do | `<world><n>_types` in `params.dir` |
| `data/<world>/<world><n>m.tdf` | The mission script: objectives, weather, events | `<world><n>_mission` in `params.dir` |
| `data/<world>.dir` | Manifest: palette and sprites of the whole world | file name |
| `data/<world><n>.dir` | Manifest: sprites only this sublevel uses | file name |
| `data/<world><n>s.dir` | Manifest: this mission's speech samples | file name |
| `data/!global/text.txt` | Mission title, briefing and all message text | logical name `game_text` |
| `data/!global/tools.tdf` | Radar colours of each world | logical name `tools` |
| `data/flics/**.flc` | Sprite art, as FLIC animations | named by the manifests |

All of these are found by logical name, except the `*.dir` manifests themselves. Each mission mounts the manifests `update`, `<world>`, `<world><n>`, `params` and, when spoken dialogs are on, `<world><n>s`, merged into one directory. A manifest listed for a mission must exist, but it may be empty. A label may not appear in two of them: the load fails with "label appears more than once".

## 2. Coordinates and planes

**Units.** Positions are in level units, with x to the right and y down.

- One level unit is **2 pixels** of the 640×400 picture, so the player sees 320×200 units at a time.
- Sprite art is in pixels. Section 8 has the conversion.
- Values in a file are whole numbers and may not be negative. A `-` is a syntax error.

**Planes.** A sublevel has 9 planes, drawn from 1 to 8 in order. Within a plane, objects are drawn in file order, so **file order is paint order**.

Each plane has its own scroll multiplier, and a plane's coordinates are in that plane's own space. With the camera at world position `(cx, cy)`, an object appears at:

```
screen_x_units = o.x - int(cx * mulx)
screen_y_units = o.y - int(cy * muly)
```

The roles below are how every shipped level uses the planes. The `active` plane is 5 in all of them.

| Plane | `mul` in shipped levels | Role |
|---|---|---|
| 1 | 0.5 | Far background art. Not drawn at low detail. |
| 2 | 0.65–0.8 | Middle background art. Not drawn at low detail. With `snow` weather it is tinted, and its display flags and mirror are ignored. |
| 3 | 1.0 | **Ground.** Ground-level game objects and all shadows draw just after it. |
| 4 | 1.0 | **Things on the ground.** The player's ship and other flying objects draw just after it. |
| 5 | 1.0 | **Active plane:** walls and everything solid. Only this plane's scenery collides. It draws over the ship. |
| 6 | 1.0 | Overhead art. |
| 7 | 1.0 or 1.3 | Empty in shipped levels. |
| 8 | 1.5 or 1.6 | Foreground parallax. Empty in shipped levels. |
| 9 | 1.0 | **Never drawn.** Holds the reserve described in section 5. |

Game objects are placed relative to the active plane (active − 2 and active − 1), so keep `active` at 5 unless you have a reason not to.

**Background.** Behind plane 1 the game tiles the sprite named `BACK`, picking a random phase for each tile and scrolling with plane 1's multiplier. At low detail it clears to `back_color` instead.

**The edge of a level.** A plane is at most 8192 × `mul` units across. No code keeps the player inside: only solid walls do, so close off the play area. The Debug build fails if the ship gets more than 100 units outside 0–8191.

**The region rule.** The engine files every object into a 64×64 grid by its top-left corner. This must hold, using C's integer division, which truncates toward zero:

```
regs_x = int(8192 * mulx) / 64
regs_y = int(8192 * muly) / 64
0 <= (o.x + phase.l) / regs_x <= 63
0 <= (o.y + phase.u) / regs_y <= 63
```

`phase.l` and `phase.u` are the sprite's bounds (section 8). At `mul` 1.0 this allows a corner from −127 to 8191. The Debug build fails on a violation. The Release build doesn't check: the object lands in the wrong grid cell, or the write lands outside the grid.

## 3. Text file syntax

All these files use one parser (`Text`).

**Encoding.** CP852 with CRLF line endings, byte for byte. Three shipped type names contain Polish letters, for example `animowane_tło` in `brown.def`, so decode and encode as CP852. Don't normalise to UTF-8.

**Comments** start with `;` or `#` and run to the end of the line.

**Labels** contain only `A–Z a–z 0–9 _`, are at most 24 characters and are not case-sensitive. A label may appear only once. The label's punctuation selects one of four value forms:

| Form | Example | Meaning |
|---|---|---|
| `label = text` | `palette = palette` | One value: the rest of the line, trimmed. |
| `label:` | `sprites:` then indented lines | A list, one value per line. See the warning below. |
| `label "a" "b"` | `CheckPoints` then quoted lines | A list of quoted strings. They may span lines. `\n`, `\t` and `""` work inside. |
| `label { ... }` | | One multi-line value. |

**Areas** nest labels. `area name` … `endarea` makes the label inside it `name/label`. Area names are not case-sensitive either.

**Numbers and truth values** read through `Text::value`: `yes`, `on`, `true` are 1; `no`, `off`, `false` are 0; a value containing `0x` is hexadecimal; anything else is decimal.

> **`label:` lists end at the first blank or comment-only line.**
>
> - Blank and comment lines *before* the first value are skipped. That is why the shipped files can have a comment header under the label.
> - After the first value, a blank line or a line holding only a comment ends the list. The next line then gets parsed as a label, which is a syntax error.
> - A comment at the end of a line is fine.
> - **A list must never be empty.** The parser would take the next label as the list's first value. Leave out the section instead.

## 4. The level file: `<world>.def`

The sections may come in any order, and the shipped files use the one shown here. Everything LED wrote as a comment (the usage header, `used 43 times`, `plane #3, size 890`) is optional, and nothing reads it.

```
types:                   ; the type table
  0000 0000 0000 unused
  ...

sprites:                 ; the sprite table
  SECRET obj_secret
  ...

main_level = 1

level1:                  ; one for each sublevel: active plane, then 9 plane lines
  5 4544 492
  0.500 0.500 064 064 0
  ...

level1_plane3:           ; the objects of one plane
  POZIOM3 0 0 5023 1098 0 0 0 zwykle_tlo
  ...

back_color = 00 00 00
palette    = palette
```

**Field separators in this file are spaces only.** A tab is not a separator here and causes an error. Numbers are decimal unless stated otherwise.

### `types:`

One line per type, up to 1024: `SSSS AAAA BBBB name`

| Field | Format | Meaning |
|---|---|---|
| `SSSS` | 4 hex digits, no `0x` | `status` flags (section 6) |
| `AAAA` | 4 hex digits | `aux1` flags (section 6) |
| `BBBB` | 4 hex digits | `aux2`: keep `0000` for new types. Shipped files also hold `0001`, which has no effect. |
| `name` | up to 24 bytes, the rest of the line | The type's name. Avoid spaces, because the same name is used as an area name in the `t.tdf` file. |

- **Type 0 must be a dummy** such as `unused`. A lookup that fails returns 0, and type 0 can't have a builder.
- Names compare without regard to case, and the first match wins. Shipped files do repeat names; only the first entry is ever used.
- Type numbers are stored nowhere in the file, as objects refer to types by name. You may reorder the table, with one exception: a type using the `change_type` digit refers to another type by its distance in the table. No shipped level uses a non-zero one.

### `sprites:`

One line per entry, up to 1024 (the loader doesn't check this limit): `[flag]NAME default_type`

| Part | Meaning |
|---|---|
| flag | Nothing: load the sprite if an object in this sublevel uses it.<br>`*`: always load it. A space may follow.<br>`-`: for the editor only, never loaded by the game. No shipped entry uses this. |
| `NAME` | Up to 24 characters, no spaces. It selects `area <world>/<NAME>` in the manifests. |
| `default_type` | Must exist in `types:`. The game only checks that; it is the type LED gave a newly placed object. |

- **A sprite that no object places must have `*`** if the game refers to it by name: `BACK`, and any sprite named in `ch_sprite`, `anim_sprite`, `shadow`, `ch_shadow` or `anim_shadow`.
- **A `*` sprite must be in the manifests of every sublevel of that world**, or those sublevels won't load. Put such sprites in `<world>.dir`.
- A name may repeat with other default types (`MINNA obj_mine1`, `MINNA obj_mine2`), which gave LED a palette entry per use. Objects bind to the first entry.
- Sprite number 0 can't be looked up by name, so don't put `BACK` or a `ch_sprite` target first.
- An unused entry costs nothing, and its sprite needn't exist in the manifests.

### `main_level`

`main_level = 1`. Read but not used: the mission name selects the sublevel. Optional.

### `level<n>:`

Exactly **10 lines**, for `n` from 1 to 9. With fewer lines the game crashes.

The first line is `active home_x home_y`:

| Field | Meaning |
|---|---|
| `active` | The active plane: 5 in all shipped levels. |
| `home_x`, `home_y` | Where LED centred its view. Read but not used by the game. Ship start positions come from objects (section 6). |

Then come 9 lines for planes 1 to 9: `M.FFF M.FFF gridx gridy reserved`

| Field | Meaning |
|---|---|
| `M.FFF M.FFF` | `mulx`, `muly`. **Always write exactly 3 decimals.** The fraction is read as a whole number of thousandths, so `0.65` loads without complaint but means 0.065. |
| `gridx`, `gridy` | LED's snap grid. The game doesn't use it, but it must be in 4–319 and 4–199, or the load fails. Leading zeros are fine. |
| `reserved` | Empty object slots on this plane. Plane 9 needs 500 (section 5), and all others use 0. |

### `level<n>_plane<p>:`

One line per object. Leave the section out if the plane is empty.

```
SPRITE phase mirror x y link lpos special type
```

| Field | Range | Meaning |
|---|---|---|
| `SPRITE` | | A name in `sprites:`. Otherwise the load fails. |
| `phase` | 0 to phases − 1 | Which frame of the sprite. **Not checked at load.** For some types it also selects the object's kind (section 6). |
| `mirror` | 0–3 | Bit 1 (value 2) flips vertically, within the sprite's rectangle. Bit 0 (value 1) flips horizontally **only if the sprite's target line has the `m+` flag**. No shipped manifest does, so in the shipped game it has no effect. |
| `x`, `y` | ≥ 0 | Position of the sprite's origin, in this plane's units. |
| `link` | 0–65534 | Link group number, 0 for none (section 5). |
| `lpos` | 0–65534 | Position within the link group. 0 if `link` is 0. |
| `special` | 0 or 1 | Difficulty flag (section 5). |
| `type` | | A name in `types:`, the rest of the line. Otherwise the load fails. |

### `back_color`, `palette`

- `back_color = r g b`: three decimal values in 0–63 (VGA scale). Optional, default black. Only seen at low detail.
- `palette = name`: required. It selects `area <world>/<name>` in the manifests, which is `palette` in all shipped worlds. Its `source` is an FLC whose palette becomes the level's palette.

### Limits

| Limit | Value | If exceeded |
|---|---|---|
| Sublevels, planes | 9, 9 | Ignored by the loader |
| Types | 1024 | Load fails |
| Sprite table entries | 1024 | **Not checked**: memory corruption |
| Type or sprite name | 24 bytes | Load fails |
| Objects in a loaded sublevel, reserve included | 65533 | Load fails |
| Moved objects (section 5) | `reserved` − 1 on plane 9 | Debug fails; **Release corrupts memory** |

## 5. What happens to objects at load time

**Numbering.** The loader numbers the sublevel's objects from 1, plane by plane, in file order. Within plane 9 the reserved slots come first. Nothing in the file stores these numbers.

**Links.** Objects with the same non-zero `link` form a ring, in ascending `lpos` order, with the last pointing back to the first. An object's "next" is the following member of its ring.

- Group numbers only have to be unique within a sublevel. Shipped files happen to keep them unique within the whole file.
- A ring may span planes.
- `lpos` values must be unique within a group, or the earlier object drops out of the ring. Gaps are fine.
- A group with one member gets no link at all.

**Difficulty (`special`).** At load, depending on the pilot's skill setting, `normal` or `challenge`:

| Object | Normal | Challenge |
|---|---|---|
| Capsule or collectable, `special` = 1 | kept | removed |
| Anything else, `special` = 1 | removed | kept |
| `special` = 0 | kept | kept |

So `special` on an enemy means "challenge only", and on a capsule it means "normal only". A removed object is also taken out of its ring.

**The plane 9 reserve.** Before play starts, the game **moves** certain objects off planes 1–8 and into the reserved slots of plane 9. Those are the objects whose type name, or whose `type =` value in the `t.tdf` file, begins with `obj_`.

- The object is no longer drawn by its plane, and from then on it is a spawn point for a game object. Its original plane goes into its `aux1` field, and a destructible uses that to draw itself and to decide whether it is solid.
- Objects move in type-table order, then in object order.
- The first reserved slot is skipped, so `reserved` must be greater than the number of moved objects. The Debug build insists on **at least 500**, so always write 500. The most used by a shipped mission is 418 (`green3`).
- A type named `obj_...` that has no builder still gets moved, so its objects silently vanish.
- Don't place objects on plane 9 yourself. No shipped level does.

## 6. Types: what an object does

The type table only holds names and flags. A type's behaviour is decided when the mission loads, in this order:

1. **Area route.** If the mission's `t.tdf` file has `area <typename>` with a `type =` key, that value selects a builder (table B). The type's name is then free, which is why missions can have many differently-configured types such as `destroy6_baza1`.
2. **Name route.** Otherwise, if the type's name starts with a builder prefix, that builder is used (table A). Don't give these a `type =` area: the area route doesn't know them. The Debug build fails, and in the Release build the objects do nothing.
3. **Scenery.** Otherwise the object is plain art, and its hex flags apply.

Prefixes are matched as written and are **case-sensitive**: write them in lower case. `obj_cannon` and `obj_mine` need the exact names listed in table A.

### Scenery flags

Each hex digit is one setting:

```
SSSS  = D S C F        AAAA = f t r c
```

| Digit | Name | Values |
|---|---|---|
| `D` | Display | 0 normal, 1 translucent (engine's 30% table), 2 translucent (70% table), 3 invisible, 4 shadow: darkens what's beneath it |
| `S` | Service: animation run while the object is on screen | 0 none, 1 phases forward, 2 phases backward, 3 random wait, 4 ping-pong, 5 synchronised forward, 6 synchronised backward, 7 synchronised ping-pong, 8 change type at once |
| `C` | Collision, active plane only | 0 none, 1 wall, 2 wall that weapons pass through, 3 wall that hurts |
| `F` | Sync | `F` × 50 ms per phase, for services 5–7 |
| `f` | Frame delay | `f` × 40 ms per phase, for services 1, 2 and 4. Use 0–7: the value is signed. |
| `t` | Time delay | `t` × 80 ms pause at the end of a cycle, for services 1, 2 and 4 |
| `r` | Repeats | Cycles before `c` applies |
| `c` | Change type | Add this to the type number after `r` cycles. 0–7 add that much; 8–15 mean −1 to −8. 0 for none. |

Common shipped values:

| `status` | Used for |
|---|---|
| `0000` | Plain art |
| `0010` | Wall, drawn |
| `3010` | Wall, invisible (draw the art with another object) |
| `3020` | Invisible wall that weapons pass through |
| `4000` | Shadow |
| `0100` with `aux1` `1000` | Looping animation at 40 ms per phase |
| `0502` | Synchronised loop, 100 ms per phase |

**A collision type needs a collision mask.** A sprite used with `C` ≠ 0 on the active plane must have a `collis` entry in its manifest, or the game stops as soon as it tries to use it: "unable to access sprite … collis data not loaded". The same goes for sprites used by doors, switches, destructibles and other solid builders.

**Leave flags at `0000` for builder types.** They don't configure anything there, but the value still seeds the object's runtime `status`, and for example a non-zero status makes an alien start awake.

### Table A: builders chosen by type name

| Type name | Object | `phase` means | `link` means |
|---|---|---|---|
| `obj_alien*` | Enemy ship | Ship kind, below | A ring that wakes it. **Needed in practice**: see below. |
| `obj_cannon1` … `obj_cannon12`, exact | Gun turret, stats from `object.tdf` | Direction: `phase` of 32 | Not used |
| `obj_mine1`, exact | Static mine | First frame | Not used |
| `obj_mine2`, exact | Tracking mine | First frame | Not used |
| `obj_teleport*` | Teleporter. The level sprite isn't what's drawn. | 0 visible, above 0 invisible | **Required.** The next object in the ring is the destination, so a pair sends the ship back and forth. |
| `obj_light*` | Light for `night` and `fog` weather | Brightness | Not used |
| `obj_secret*` | Invisible secret, found when it comes on screen. Counted in the score. | Not used | Not used |
| `obj_smoke*` | Rising smoke | 0 small, 1 big black, 2 big white | Not used |
| `obj_collect*` | Beam-up objective: the ship hovers over it for 2 s, slowly | First frame | Not used |
| `obj_raft*` | Raft that sails south (`y` growing) | First frame | **Required.** When it passes the next object's `y`, an event fires for *that object's type*. Place the target south of the raft. |
| `obj_buoy*` | Floating mine | First frame | Not used |
| `obj_thief*` | Enemy that visits the ring in order | First frame | **Required.** Each waypoint reached fires an event for that waypoint's type. |
| `obj_morph*` | Structure that changes phase as it takes damage | First frame | Not used |
| `obj_brick*` | Brick, in the network bricks game only. Removed otherwise. | Not used | Not used |
| `base*` | Base, in the network bricks game only. Hidden otherwise. | Not used | Section 9 |
| `start_place*` | Where the player starts. Its sprite is drawn as plain art. | First frame | Not used |

Aliens, teleporters and capsules draw their own art from the global sprites. Their level sprite is only what an editor shows and what sets the bounds.

The prefixes `obj_magnes`, `obj_machine` and `obj_photo` exist in the source, but no builder uses them. `obj_raft`, `obj_buoy`, `obj_thief` and `obj_secret` are for single-player: the Debug build fails on them in a network game.

**Ship kinds** (`obj_alien*`'s `phase`). The names are the engine's, and the stats are in `object.tdf`. Phases 8–15 are the same ships, cloaked. Other phases are not valid.

| Phase | Ship | Cloaked |
|---|---|---|
| 0 | `Simple1` | 8 |
| 1 | `Simple1`, special: leaves a capsule | 9 |
| 2 | `Simple3` | 10 |
| 3 | `Taran`, a ram | 11 |
| 4–7 | Mini-bosses 1–4 | 12–15 |

**Waking aliens.** An alien sits parked on the ground until its `status` becomes non-zero. That happens through a ring: an `obj_unblock` trigger or an `enable_*` destructible sets it on the ring's objects, and a `Simple1` passes it on when it takes off. Link every alien to something that wakes it. The shipped levels do this for all but 2 of their 560 aliens.

### Table B: builders chosen by `type =` in `<world><n>t.tdf`

Each is an `area <typename>` in the mission's `t.tdf`. The type's name is free. Keys marked ✱ are required: the load fails without them.

| `type =` begins with | Object | Keys |
|---|---|---|
| `landplace` | Landing pad | None. Its sprite is drawn. The ship lands on it within 40 units at speed 50 or less, and only while its type is the current checkpoint. |
| `door` | Door | `delay`✱ seconds before it toggles back, 0 for never. `open`✱ 0 starts closed, 1 starts open. `serv_type` 0 solid (default), 1 and 2 translucent. |
| `switch` | Switch, set off by the player hitting it | `standtime`✱ seconds. `door_type`✱ the door type it sends to. `door_type2` a second one. |
| `obj_unblock` | Invisible trigger | `serv_type`✱ `visual` (ship within 350 units) or `collis` (ship touches it). It wakes its ring and fires an event. |
| `obj_destroy` | Destructible structure | Below |
| `obj_capsule` | Pick-up. `phase` selects it, below. | `range`✱ |
| `obj_generator` | Spawner. Its ring must hold only aliens, and at least one. | `generate_frq`✱ seconds. `object_count`✱ |
| `obj_mycannon` | Base gun, network bricks game only | `appear_level`✱, `cannon_type`✱, `angle`✱ degrees. Required only in that mode. |

**Doors.** The sprite needs an even number of phases `N`. Phases 0 to N/2 − 1 animate the lower part, drawn on the door's plane. The other half is the upper part, drawn on the plane above. Phase 0 is closed and solid; N/2 − 1 is open, and lets everything through. A door moves when it gets a message for its own type, from a `switch` or a mission's `handle "door <type>"`.

**Destructibles** (`obj_destroy`):

| Key | Meaning |
|---|---|
| `life`✱ | `LOW_SHIELDS`, `MEDIUM_SHIELDS`, `HIGH_SHIELDS` or `IMMORTAL`. An immortal one dies only from a mission's `handle "destroy_obj <type>"`. |
| `frame_delay`✱ | Seconds per animation phase |
| `ch_sprite` | Sprite that replaces it afterwards: the wreck. |
| `anim_sprite` | Sprite played as it dies. Don't combine with `morph`. |
| `morph` | 1: fade into `ch_sprite`. Needs `ch_sprite`. |
| `shadow`, `ch_shadow`, `anim_shadow` | Shadow sprites for the three states |
| `action` | What happens to its ring. `enable_after`, `enable_before`: the ring's objects stay hidden (aliens stay parked) until this object is destroyed (`after`) or first hit (`before`). `disable_after`, `disable_before`: they are removed then, and aliens, destructibles and cannons among them are destroyed. |
| `explode` | 1 big explosion, 2 medium, 0 none (default) |
| `explode_sample`, `hit_sample` | Sample logical names |
| `smoke` | Smoke after destruction: 1 and up selects the kind, 0 none (default) |
| `light` | 1 (default): the wreck is a light source under `night` and `fog` |
| `visible` | 1 (default). 0: not drawn until wrecked, for art drawn by another object. |
| `colis` | 1: solid for a structure below the active plane. Default 0. At or above the active plane it is always solid. Some shipped areas spell it `collis`, which isn't read. |

`range` appears in the shipped areas, but only `obj_capsule` reads it. `unblocked_type` and `event` are not read at all.

The sprites in `ch_sprite`, `anim_sprite` and the shadow keys must be loaded: mark them `*` unless an object places them.

**Capsules** (`obj_capsule`'s `phase`):

| Phase | Gives | Phase | Gives |
|---|---|---|---|
| 0–2 | Weapon 1 (small, medium, large) | 18–20 | Life (small, medium, large) |
| 3–5 | Weapon 2 | 21 | Medikit |
| 6–8 | Weapon 3 | 22 | Cloak |
| 9–11 | Weapon 4 | 23 | Immortality |
| 12–14 | Weapon 5 | 24, 25 | Nothing is created |
| 15–17 | Weapon 6 | 26 | Network mine |
| | | 27 | Golden eye |
| | | 28–30 | Altering (power 0–2) |
| | | 31 | Chaos |

### Common link patterns

| Pattern | Objects in the ring | Effect |
|---|---|---|
| Reward | `obj_destroy` with `action = enable_after`, plus an `obj_capsule` | The capsule appears when the structure dies |
| Ambush | `obj_unblock` plus `obj_alien*` | The aliens take off when the ship comes near |
| Teleporter | Two or more `obj_teleport` | Each sends the ship to the next |
| Spawner | `obj_generator` plus `obj_alien*` | The aliens are spawned in random order |

## 7. The mission script: `<world><n>m.tdf`

```
dx = 4096
dy = 4096
weather = none

CheckPoints
  "first"
  "last"

Bonus
  "optional"

area first
  type     = target
  count    = 1
  radar    = 1
  OBJ_TEXT "green_6_1"
endarea
...
```

**Top-level keys:**

| Key | Meaning |
|---|---|
| `dx`, `dy` | Required, but not otherwise used. Shipped missions say 4096. |
| `weather` | Required: `none`, `night`, `fog`, `rain1`–`rain4`, `snow1`–`snow4`, `smoke_above`, `smoke_below`, or `clouds...`. `smoke` and `clouds` names need an area in `!global/weather.tdf`. |
| `radar` | 0 starts with the radar off. Default 1. |
| `CheckPoints` | The ordered, required objectives: a list of quoted area names. |
| `Bonus` | Optional events: a list of quoted area names. |

**How objectives run.** Game objects fire an event carrying **their type** (table below). Each event goes to the first entry in `CheckPoints` whose `type` list has it, or if none has, to the first such `Bonus` entry. It lowers that entry's `count`, and at 0 the entry completes.

- Any checkpoint can count, not only the first. The first one is what the player is told to do and what the radar points to.
- When the last checkpoint completes, the mission ends if it has `quit = 1`. **The last checkpoint must have `quit`**, or the mission never ends. A `quit` checkpoint that isn't last waits until it is.
- A `Bonus` entry with `quit = 1` that completes while checkpoints remain **fails the mission**, which then restarts. This is how shipped levels make an objective losable.
- The mission also restarts if the player's ship is destroyed, or if a `TIMER` runs out.

> **The last checkpoint's `enable_action` and `disable_action` run when every enemy ship and turret is dead, not when it completes.** Shipped missions use this for the reward at the end: 16 of the 28 mission files put an `enable_action` there. On every other checkpoint and on bonus entries, they run when the entry completes.
>
> A mission with no enemies at all reaches this state at once, and the player gets the "all destroyed" message as it starts.

**Which objects fire events:**

| Object | When |
|---|---|
| `obj_alien*`, `obj_cannon*`, `obj_mine*`, `obj_morph*`, `obj_thief*`, `obj_raft*`, `obj_destroy` | Destroyed |
| `obj_raft*` | Passes its target: fires for the target's type |
| `obj_thief*` | Reaches a waypoint: fires for the waypoint's type |
| `switch`, `obj_unblock` | Set off |
| `obj_collect*` | Beamed up |
| `landplace` | The ship lands on it |

**Keys in a checkpoint or bonus area:**

| Key | Meaning |
|---|---|
| `type` ✱ | The type this entry counts. A quoted list may name several. Must exist in `types:`. |
| `count` ✱ | The number of events needed, or `all` for every object of those types left in the level on this difficulty. |
| `OBJ_TEXT` | ✱ in single-player checkpoints: a label in `text.txt`, shown as the objective. |
| `DISPL_TEXT` | Spoken lines, in threes: `"text label" "sound logical name" "SPEAKER"`. Speakers are `JAXON`, `RILEY`, `STACY` and `SHADOW`. Up to 24 lines. |
| `radar` | Present, whatever its value: the radar points at this type. |
| `set_radar` | 0 or 1: turns the radar off or on when the entry completes. |
| `quit` | 1: see above. |
| `enable_action` | Quoted type list. All objects of these types start hidden and appear when this entry completes. |
| `disable_action` | Quoted type list. All objects of these types are removed when this entry completes. |
| `handle` | Quoted list of messages sent on completion: `"door <type>"` moves those doors; `"destroy_obj <type>"` destroys those objects. |
| `TIMER` | Starts a countdown of this many seconds on completion. At 0 the mission fails. |
| `QUIT_TIMER` | 1: stops the countdown on completion. |
| `REBEL`, `REBEL_QUIT` | Present: start or stop the "rebel ship" sequence on completion. |
| `shake_screen` | Present: shakes the screen for 2 s on completion. |
| `sample` | Plays this sample on completion. |
| `nodelete` | Bonus only, present: the entry resets instead of being removed. |
| `CH_TYPE` | Changes a type's settings on completion, as `"type WHAT n"`, where `WHAT` is `DISP_TYPE`, `COLIS_TYPE`, `FRAME_DELAY`, `TIME_DELAY`, `REPEAT_COUNT`, `SYNC_FRAME`, `CHANGE_TYPE`, `SERV_TYPE` or `BLOCKED`. No shipped mission uses it. |

`event` and `PHOTO` appear in some files or the source, but have no effect.

## 8. Sprites: finding and drawing them

### Manifest entries

The sprite named `NAME` is the area `<world>/<NAME>`, in `<world>.dir` or `<world><n>.dir`. Paths are relative to `data/`, use `\`, are not case-sensitive, and should be resolved in lower case. A leading `-` on a path is ignored.

```
area test                 ; the world's name
  source = test\test.def  ; the level file

  area palette
    source = -flics\green\palette.flc
    target = test\sprites\palette.spp
  endarea

  area skala
    source  = -flics\green\skala.flc     ; default master for every variant
    csource = -flics\green\skala_k.flc   ; collision master
    hires   = test\sprites\skala.sph
    lores   = test\sprites\skala.spl
    collis  = test\sprites\skala.spc
  endarea
endarea
```

| Key | Meaning |
|---|---|
| `hires`, `lores`, `collis` | **Target keys:** the presence of the key turns that variant on. The file named is never read or written. |
| `source` | The FLC used for any variant without its own master |
| `hsource`, `lsource`, `csource` | The master of one variant. Each needs its target key too. |

Flags may follow a target's path: `m+` builds horizontally flipped copies, `o+` makes a one-colour sprite, and `1x1`, `2x1`, `4x1`, `8x1` override the scale.

| Variant | Purpose | Built at | Needed? |
|---|---|---|---|
| `hires` | The picture | 1×1 | Yes |
| `collis` | Collision mask, in one colour | 8×1 | For anything solid (section 6) |
| `lores` | Old low-res art: never drawn, but its size counts towards the bounds | 2×1 | Keep it on existing sprites. Not needed on new ones. |

### The FLC convention

Masters are standard 8-bit FLIC files (`0xAF12`, or `0xAF11`), and each frame of a master is one phase. The game reads chunk types 4, 7, 11, 12, 13, 15 and 16.

**A frame has a one-pixel border, which is not part of the picture.** A frame of `W × H` holds a picture of `sx = W − 2` by `sy = H − 2`:

```
pixel (0, 0)          the key colour: this colour is transparent in this frame
top row, left column  ignored
bottom row            the last non-key pixel in columns 1..sx marks the origin's x: ox = column - 1
right column          the last non-key pixel in rows 1..sy marks the origin's y:    oy = row - 1
```

If either mark is missing, the origin is the centre: `ox = sx / 2`, `oy = sy / 2`. Each frame has its own origin.

An object's `x`, `y` is where this origin goes.

**Colour.** FLC palettes are per file. In the game each pixel is mapped to the closest colour of the level's palette. An editor can simply show the FLC's own colours, which are close enough to place art. Chunk type 4 holds 0–255 values, and the engine drops the low 2 bits; type 11 holds 0–63. Multiply 0–63 values by 4 for display.

### Drawing an object

This draws a plane the way the game does. Take the objects in file order, plane 1 to plane 8.

```python
camera_x, camera_y          # in level units, the top-left of the view
for plane in 1..8:
    for o in objects[plane]:           # file order
        if o was moved to plane 9 or replaced by a game object: continue   # sections 5, 6
        frame = sprite(o.sprite).frames[o.phase]
        px = 2 * (o.x - int(camera_x * mulx[plane])) - frame.ox
        py = 2 * (o.y - int(camera_y * muly[plane])) - frame.oy
        pixels = flip_vertically(frame) if o.mirror & 2 else frame
        draw pixels at (px, py), skipping the key colour
```

For editing you will want to show the moved objects too. Draw them on the plane they were placed on.

### Phase bounds

A phase's bounds are needed for the region rule in section 2, and for what the game treats as visible. They are in level units relative to the origin. Each variant gives four values, and a variant that's absent counts as all zeros:

| Variant | `l` | `r` | `u` | `d` |
|---|---|---|---|---|
| hires | `-ox / 2` | `(sx - ox) / 2` | `-oy / 2` | `(sy - oy) / 2` |
| lores, at 2×1 (`s` = 2) or 1×1 (`s` = 1) | `-(ox / s)` | `-(ox / s) + sx / s` | `-(oy / s)` | `-(oy / s) + sy / s` |
| collis, at 8×1 | `-(ox / 8) * 2` | `(-(ox / 8) + sx / 8) * 2` | `-(oy / 8) * 2` | `(-(oy / 8) + sy / 8) * 2` |

All division truncates toward zero. The phase's `l` and `u` are the minimum over the three variants, and `r` and `d` the maximum. So bounds always include the origin.

A sprite has as many phases as its longest variant's master has frames.

## 9. Network levels

Levels in `Net_order` are played in two modes, chosen by the players. A level should work in both.

| Object | Single-player | Deathmatch | Bricks game |
|---|---|---|---|
| `start_place` | First in object order is the start | Random starts | Removed |
| `landplace` | Landing pad | Removed | Start place and home base |
| `base`, `obj_mycannon` | Removed | Removed | Active |
| `obj_brick` | Removed | Removed | Active |

**Requirements:**

- **Deathmatch:** at least 4 `start_place` objects (shipped: 6–9), and no more than 23 in any mode.
- **Bricks game:** exactly 4 landing pads (the Debug build checks this), each in its own ring. In ring order: `landplace`, then `base`, then the base's guns. For example:

  ```
  LPLACE  0 0 5104 3935 3 0 0 landplace
  BASE    4 0 4948 3904 3 1 0 base
  DZIALO  0 0 5043 3798 3 2 0 obj_laser_2
  DZIALO  0 0 4789 3956 3 3 0 obj_laser_3
  DZIALO2 0 0 5020 4022 3 4 0 obj_rakieta_4
  ```

  `obj_laser_2` and `obj_rakieta_4` are ordinary type names, set up with `type = obj_mycannon` areas. The ring may start anywhere as long as the cyclic order stays.

- **Bricks game:** between 70 and 200 `obj_brick` objects, or none at all (shipped: 104–123). At the start of each game all but 70 are removed at random. Other counts stop the load.

Up to 10 missions can be in `Net_order`. Each needs its own bit in `Net_ident` and a `netsongs` entry.

## 10. Adding a level: a complete example

**Work on a copy.** The `data_files` test checks every file in `data/` against its original hash, and the demos depend on the shipped levels. Copy `data/` somewhere and run the game with `--data <copy>`.

There are three levels of change. Each step below includes those above it.

| Goal | What to do |
|---|---|
| **Replace a mission** | Rewrite its `level<n>` and `level<n>_plane<p>` sections in `<world>.def`, plus `<world><n>m.tdf` and `<world><n>t.tdf`. Add types and sprites to the world's tables as needed. |
| **Add a mission to a world** | Also create `<world><n>.dir` and `<world><n>s.dir` (they may be empty), register the two text files in `params.dir`, add the mission to `missions.tdf`, and add its title and briefing to `text.txt`. |
| **Add a world** | Also create `data/<world>/`, a `<world>.dir` with the palette and sprites, and the radar colours in `tools.tdf`. |

The example below adds a world called `test` with one mission, `test1`, using the art of world `green`. It was run exactly as shown in the Debug build, through to "Mission complete". In it, the ship starts on a pad, shoots a dome to the north (which leaves a crater and releases a medikit), then lands on the pad just to the south.

Save these with CRLF line endings.

**`data/test/test.def`**

```
types:
  0000 0000 0000 unused
  0000 0000 0000 ground
  0010 0000 0000 wall
  0000 0000 0000 start_place
  0000 0000 0000 pad
  0000 0000 0000 dome
  0000 0000 0000 reward
  0000 0000 0000 ambush
  0000 0000 0000 obj_alien
  0000 0000 0000 obj_cannon5

sprites:
  PLK_5 ground
  SKALA wall
  LANDANI start_place
  RUINY1A dome
  BRONIE reward
  UNLOCK ambush
  _WROG obj_alien
  DZIALKO1 obj_cannon5
  *RUINY1Z unused
  *BACK unused

level1:
  5 1300 1200
  0.500 0.500 064 064 0
  0.650 0.650 008 008 0
  1.000 1.000 032 044 0
  1.000 1.000 032 037 0
  1.000 1.000 032 037 0
  1.000 1.000 032 037 0
  1.300 1.300 032 037 0
  1.500 1.500 008 008 0
  1.000 1.000 008 008 500

level1_plane3:
  PLK_5 5 0 1236 1120 0 0 0 ground
  PLK_5 5 0 1300 1120 0 0 0 ground
  PLK_5 5 0 1364 1120 0 0 0 ground
  PLK_5 5 0 1236 1208 0 0 0 ground
  PLK_5 5 0 1300 1208 0 0 0 ground
  PLK_5 5 0 1364 1208 0 0 0 ground
  LANDANI 0 0 1300 1200 0 0 0 start_place
  LANDANI 0 0 1300 1270 0 0 0 pad

level1_plane4:
  BRONIE 21 0 1240 1090 1 1 0 reward   ; a medikit, in link group 1
  UNLOCK 0 0 2100 1200 2 0 0 ambush    ; wakes the alien in link group 2

level1_plane5:
  RUINY1A 0 0 1300 1060 1 0 0 dome
  SKALA 1 0 1060 1400 0 0 0 wall
  SKALA 1 0 1110 1400 0 0 0 wall
  SKALA 1 0 1160 1400 0 0 0 wall
  _WROG 0 0 2200 1150 2 1 0 obj_alien
  DZIALKO1 8 0 700 1100 0 0 0 obj_cannon5

back_color = 0 18 0
palette = palette
```

**`data/test/test1t.tdf`**

```
area dome
  type        = obj_destroy
  life        = LOW_SHIELDS
  action      = enable_after
  explode     = 1
  ch_sprite   = RUINY1Z
  frame_delay = 0.05
endarea

area pad
  type = landplaceA
endarea

area reward
  type  = obj_capsule
  range = 30
endarea

area ambush
  type      = obj_unblock
  serv_type = visual
endarea
```

**`data/test/test1m.tdf`**

```
dx = 4096
dy = 4096
weather = none

CheckPoints
  "kill_dome"
  "go_home"

area kill_dome
  type     = dome
  count    = 1
  radar    = 1
  OBJ_TEXT "green_6_1"
endarea

area go_home
  type     = pad
  count    = 1
  radar    = 1
  quit     = 1
  OBJ_TEXT "green_6_3"
endarea
```

`OBJ_TEXT` borrows labels that already exist in `text.txt`. A real mission would add its own, and add `DISPL_TEXT` lines with samples in `test1s.dir`.

**`data/test.dir`**

```
area test
  source = test\test.def

  area palette
    source = -flics\green\palette.flc
    target = test\sprites\palette.spp
  endarea

  area back
    source = -flics\green\back.flc
    hires  = test\sprites\back.sph
  endarea

  area plk_5
    source = -flics\green\plk_5.flc
    hires  = test\sprites\plk_5.sph
  endarea

  area skala
    source  = -flics\green\skala.flc
    csource = -flics\green\skala_k.flc
    hires   = test\sprites\skala.sph
    collis  = test\sprites\skala.spc
  endarea

  area landani
    source = -flics\green\landani.flc
    hires  = test\sprites\landani.sph
    collis = test\sprites\landani.spc
  endarea

  area ruiny1a
    source = -flics\green\ruiny1a.flc
    hires  = test\sprites\ruiny1a.sph
    collis = test\sprites\ruiny1a.spc
  endarea

  area ruiny1z
    source = -flics\green\ruiny1z.flc
    hires  = test\sprites\ruiny1z.sph
    collis = test\sprites\ruiny1z.spc
  endarea

  area bronie
    source = -flics\!global\bronie.flc
    hires  = test\sprites\bronie.sph
    collis = test\sprites\bronie.spc
  endarea

  area unlock
    source = -flics\green\unlock.flc
    hires  = test\sprites\unlock.sph
    collis = test\sprites\unlock.spc
  endarea

  area _wrog
    source = -flics\!global\_wrog.flc
    hires  = test\sprites\_wrog.sph
    collis = test\sprites\_wrog.spc
  endarea

  area dzialko1
    source = -flics\green\dzialko1.flc
    hires  = test\sprites\dzialko1.sph
    collis = test\sprites\dzialko1.spc
  endarea
endarea
```

**`data/test1.dir` and `data/test1s.dir`:** empty files. The game requires them to exist.

**Add to `data/params.dir`:**

```
test1_mission = test\test1m.tdf
test1_types   = test\test1t.tdf
```

**In `data/!global/missions.tdf`,** add `test1` as a new line under `Missions_order:`, and add a song as a matching line under `songs`. The two lists run in parallel, so insert at the same position.

The mission screen shows one thumbnail per campaign mission, taken by position from `flics/!global/prgnor.flc` and `prgdrk.flc`. These have 19 frames, and there are 18 missions, so **a second added mission needs longer files**, named through `missions_pics_nor` and `missions_pics_dark` in `global.dir`. The game's own limit is 30.

```
  test1
```

```
      "2"
```

**In `data/!global/text.txt`,** add the mission's title and briefing lines. The label is the mission name, and the briefing lines start with `*`, as the shipped ones do:

```
test1 "my first level"

"*DESTROY THE DOME"
"*AND LAND."
```

**In `data/!global/tools.tdf`,** add radar colours inside `area radar`:

```
  testalien=  30,30,30,75
  testtarget= 63,63,63,90
```

## 11. Testing a level

**Use the Debug build.** It reports what the Release build lets through: out-of-range regions, unknown builders, a short plane 9 reserve and more. Load failures are written to the log, `Firefght.log` in the preferences directory (also echoed to stderr), each marked with a `*`:

```
1sp_lev(407)   * undeclared sprite 'level6_plane4:' in level6_plane2
1sp_lev(483)   * can't read level 'GREEN' [Level::load]
```

**Load check.** `check` mode loads the level of every mission in `Missions_order`, stopping at the first failure:

```sh
cwdiags=extended build/macos-clang/source/Debug/firefight \
  --data <copy> --pref /tmp/ffpref --headless --fast check
```

It doesn't cover `Net_order`, so test those by hosting a game. `tests/check_net.cmake` shows how to do that headless.

**Flying it.** Headless, an input script plays the game, and `--shot-every` saves frames to look at. From a new pilot's title screen the first `Return` opens the mission screen and the second starts the first mission:

```
250 key Return down
252 key Return up
400 key Return down
402 key Return up
430 key Space down
640 key Space up
660 key Down down
672 key Down up
```

```sh
build/macos-clang/source/Debug/firefight --data <copy> --pref /tmp/ffpref \
  --headless --fast --input fly.txt --shot-every 30 --quit-frames 1000
```

This script completes the example above; the log then has `all check points have been completed`. Use a fresh `--pref` directory each time, as a saved pilot may start on a different mission. Frames land there as `frame_NNNNNN.bmp`. To play it yourself, drop `--headless --fast --input`, and add `--music <repo>/music` for the soundtrack.

**Demos.** Demo files replay recorded input, and any change to a level they use puts them out of sync. That doesn't matter for your own copy, but keep the shipped levels unchanged if you want `--demo` to keep working.

## 12. Checks an editor should make

**The game stops with a message** when these are wrong. Catching them first gives a better error:

- Every object's sprite and type is declared, and every sprite's default type is declared.
- Numbers are in range, with no negatives, tabs or stray text.
- Each `level<n>` plane line has valid grid values, and a reserve that fits.
- Every sprite that gets loaded is in the manifests, and its FLC exists.
- Every `ch_sprite`-style sprite is loaded (`*` or placed).
- Every required key in the `t.tdf` and `m.tdf` tables is present.
- Every sprite used as solid has a `collis` entry.
- Single-player: at least one `start_place`, and `OBJ_TEXT` on every checkpoint naming a label that exists.
- `obj_generator` rings hold only aliens. `obj_raft` has a link.
- Network: the start place, pad and brick counts of section 9.

**The game silently misbehaves or crashes** when these are wrong, so an editor should always check them:

- `level<n>:` has exactly 10 lines. Too few crashes.
- Multipliers have exactly 3 decimals.
- No `label:` list is empty, and none has a blank or comment line inside it.
- `phase` is below the sprite's phase count.
- The region rule holds on every object.
- Moved objects fit: `reserved` on plane 9 is 500, and at most 499 objects move.
- `obj_cannon` and `obj_mine` types use exact names.
- `obj_teleport` and `obj_generator` have rings. Without one they read an invalid object, or hang.
- Door sprites have an even phase count.
- Each link group has unique `lpos` values.
- The last checkpoint has `quit = 1`.
- Every `obj_alien` is linked to something that wakes it.
- Each type name starting with `obj_` has a builder, or its objects vanish.
- The sprite table has no more than 1024 entries.
- `*BACK` is declared and is not entry 0. Without it the game uses sprite 0 as the background.

**What to preserve.** Keep the file's order of objects within a plane, as it is paint order. Keep CP852 and CRLF. Rewriting a shipped `.def` with no comments and normalised spacing changes nothing for the game, so an editor needn't keep LED's comments or layout.

## 13. Where it lives in the source

| Subject | File |
|---|---|
| Parsing `.def`, links, regions, what gets drawn | `source/engine/1sp/1sp_lev.cpp`, `1sp.h` (`Level`, `Lobject`) |
| Text syntax | `source/engine/1io/1io_txt.cpp` |
| Manifests | `source/engine/1io/1io_io.cpp` |
| Types, builders, `special`, networking setup | `source/game/build.cpp` |
| Moving objects to plane 9, drawing planes | `source/game/world.cpp` |
| Mission scripts | `source/game/gman.cpp` |
| Doors, switches, destructibles, triggers, weather | `source/game/neutral.cpp` |
| Aliens, cannons, mines, raft, thief | `source/game/alien.cpp` |
| Capsules, teleports, landing pads, bases | `source/game/my.cpp` |
| FLC reading and sprite build | `source/engine/1sp/1sp_flic.cpp`, `1sp_lreb.cpp`, `1sp_lmai.cpp` |
| Tunables named here (delays, distances) | `data/!global/main.tdf`, `object.tdf` |

## How this was checked

These were done with a reader and writer written from this document's rules, on macOS in November 2026, against the Debug and Release builds.

- **File format:** all five `.def` files (57,939 objects) were read and rewritten with no comments and normalised spacing. On the rewritten files all 8 attract demos and 4 golden demos replay in sync, and `check` mode loads every campaign mission with the same object counts and the same sprite dump as on the originals. The `net_play` test plays in sync on them too. The other network missions were only parsed.
- **Sprite origins and bounds:** the rule in section 8 reproduces `tests/golden/sprite_bounds.txt` for all 382 sprites, which themselves match the original's caches.
- **Drawing:** a separate program drew `green6` by section 8's rule, using each FLC's own colours and leaving out shadows and translucent objects. 90% of compared pixels were within 16/255 per channel of the game's own frame. The two pictures look the same apart from the shadows and the game's own objects.
- **Example:** section 10 and the input script of section 11 were extracted from this file and run in the Debug build through to mission complete.
- **Errors:** each mistake the document calls an error was made on purpose and the result observed. That covers empty lists, blank and comment lines in lists, tabs, negative numbers, unknown types, short plane lists, short fractions, a small reserve, a missing `BACK` and a solid sprite with no mask.

Behaviour described for individual game objects comes from reading their code. Only the objects in the example were seen working, along with whatever the demos exercise.
