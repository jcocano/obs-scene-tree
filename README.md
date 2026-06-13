# obs-tree

A native OBS Studio plugin that adds a **folder tree dock** for organising your
scenes. OBS keeps scenes in a single flat list; `obs-tree` lets you group them
into nested folders and reorder everything by drag & drop, while clicking a
scene still switches to it like the built-in list.

Built against the current OBS frontend API (OBS 32.x, Qt 6).

## Features

- **Custom dock** registered with the modern `obs_frontend_add_dock_by_id` API
  (appears under *Docks* menu).
- **Folders** — create, rename, nest, and delete (deleting a folder re-homes its
  contents, never your scenes).
- **Drag & drop** to move scenes between folders and reorder them. Folders are
  the only valid drop targets; scenes are leaves.
- **Live sync** with OBS via the frontend event callback: scene add / remove /
  rename and active-scene changes are reflected immediately.
- **Stable identity** — scenes are tracked by source UUID
  (`obs_source_get_uuid`), so renaming a scene never loses its place.
- **Persistence** — the folder layout is stored *inside the active scene
  collection* through the frontend save callback, so it travels with the
  collection and is per-collection.

## How it works

| Concern            | API used                                                    |
|--------------------|-------------------------------------------------------------|
| Dock registration  | `obs_frontend_add_dock_by_id` / `obs_frontend_remove_dock`  |
| Scene enumeration  | `obs_frontend_get_scenes`                                   |
| Switch scene       | `obs_get_source_by_uuid` + `obs_frontend_set_current_scene` |
| Stable scene key   | `obs_source_get_uuid`                                       |
| Live updates       | `obs_frontend_add_event_callback`                          |
| Save / load layout | `obs_frontend_add_save_callback` + `obs_frontend_save`      |

The layout is serialised under an `"obs-tree"` object in the scene collection's
save data:

```json
"obs-tree": {
  "version": 1,
  "roots": [
    { "type": "folder", "name": "Intro", "expanded": true,
      "children": [ { "type": "scene", "uuid": "…", "name": "Starting Soon" } ] },
    { "type": "scene", "uuid": "…", "name": "Just Chatting" }
  ]
}
```

## Building (Linux)

Requires `libobs` + `obs-frontend-api` dev files and Qt 6 Widgets.

```sh
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
cmake --install build   # -> ~/.config/obs-studio/plugins/obs-tree/
```

Then **restart OBS** and enable the *Scene Tree* dock from the *Docks* menu.

Override the install location with `-DOBS_PLUGIN_DESTINATION=/path`.

## Project layout

```
src/plugin-main.cpp       module entry: register/unregister the dock
src/scene-tree.{hpp,cpp}  QTreeWidget subclass with drag & drop rules
src/scene-tree-dock.*     dock UI, OBS sync, persistence
data/locale/*.ini         translations (en-US, es-ES)
```
