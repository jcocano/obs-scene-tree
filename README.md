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

## Install (no compiling)

Prebuilt packages for **Linux, Windows and macOS** are published on the
[Releases](https://github.com/jcocano/obs-scene-tree/releases) page, built
automatically by CI for every tagged version. Download the file for your OS:

- **Linux** — `obs-tree-<ver>-x86_64.deb` (Debian/Ubuntu: `sudo apt install ./obs-tree-*.deb`).
  On other distros, extract the archive into `~/.config/obs-studio/plugins/`.
- **Windows** — unzip `obs-tree-<ver>-windows-x64.zip` into your OBS install dir
  (e.g. `C:\Program Files\obs-studio\`). The binaries are **unsigned**, so
  Windows SmartScreen may warn — choose *More info → Run anyway*.
- **macOS** — open `obs-tree-<ver>-macos-universal.pkg`. The package is
  **unsigned/un-notarized**, so the first time, right-click it → *Open* (or run
  `xattr -dr com.apple.quarantine <file>.pkg`) to bypass Gatekeeper.

Then **restart OBS** and enable the *Scene Tree* dock from the *Docks* menu.

## Building from source

This project uses the official
[OBS plugin template](https://github.com/obsproject/obs-plugintemplate)
build system. CMake presets drive per-OS builds; CI downloads pinned OBS +
Qt6 dependencies (see `buildspec.json`).

```sh
cmake --preset ubuntu-x86_64      # or: macos / windows-x64
cmake --build --preset ubuntu-x86_64
```

### Local dev loop (Linux)

To iterate against your own OBS install, build the `dev-install` target — it
copies the module + `data/` into OBS's portable plugin dir (no root needed):

```sh
cmake --preset ubuntu-x86_64
cmake --build build_x86_64 --target dev-install   # -> ~/.config/obs-studio/plugins/obs-tree/
```

Then **restart OBS** (plugins load at startup, not hot-reloaded) and enable the
*Scene Tree* dock. Override the destination with `-DOBS_PLUGIN_DESTINATION=/path`.

## Project layout

```
src/plugin-main.cpp       module entry: register/unregister the dock
src/scene-tree.{hpp,cpp}  QTreeWidget subclass with drag & drop rules
src/scene-tree-dock.*     dock UI, OBS sync, persistence
data/locale/*.ini         translations (en-US, es-ES)
```
