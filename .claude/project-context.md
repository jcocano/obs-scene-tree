# Contexto del proyecto — obs-tree

Plugin **nativo de OBS Studio** (C++17 / Qt6) que añade un **dock de árbol de
carpetas** para organizar escenas en grupos anidados con drag & drop. OBS las
guarda en una lista plana; este plugin las agrupa sin perder el comportamiento
nativo (clic en escena = cambia a ella).

> Detalle completo (features, APIs usadas, formato de persistencia) en `README.md`.
> Léelo antes de tocar la lógica de sync o persistencia.

## ⚠️ Bucle de trabajo CRÍTICO (si no, los cambios NO se ven)
Un build a secas no basta. Para ver cualquier cambio en OBS hay que **compilar,
instalar y reiniciar OBS**, en ese orden:

```sh
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo   # solo la 1ª vez / si cambia CMake
cmake --build build          # compilar
cmake --install build        # -> ~/.config/obs-studio/plugins/obs-tree/
# luego REINICIAR OBS y activar el dock "Scene Tree" desde el menú Docks
```
Saltarse el `install` o el reinicio = sigues viendo la versión vieja. No lo olvides.

## Principios de diseño (no negociables)
- **Minimalista y nativo a OBS.** La UI debe parecer parte de OBS, no un añadido.
- **No disruptivo.** No romper el flujo nativo: clic en escena sigue cambiando de
  escena; borrar una carpeta re-aloja su contenido, nunca borra escenas.
- Identidad estable por UUID de source (`obs_source_get_uuid`), no por nombre.

## Mapa de fuentes
```
src/plugin-main.cpp        entrada del módulo: registra/desregistra el dock
src/scene-tree.{hpp,cpp}   QTreeWidget con reglas de drag & drop (carpetas=destino, escenas=hoja)
src/scene-tree-dock.*      UI del dock, sync con OBS, persistencia
src/icons.{hpp,cpp}        iconos SVG
data/locale/*.ini          traducciones (en-US, es-ES)
```

## Flujo de cambios (git)
`main` está protegida. Todo cambio va por PR con commits firmados. Reglas
completas en `repo-workflow.md` (cargado junto a este archivo cada sesión).
