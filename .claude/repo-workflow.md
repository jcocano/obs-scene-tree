# Flujo de trabajo de este repo (obs-scene-tree)

La rama `main` en GitHub (jcocano/obs-scene-tree) está PROTEGIDA. Trabaja siempre así:

## Reglas duras (las impone GitHub, no se pueden saltar)
- **NUNCA hagas push directo a `main`.** Todo cambio entra por Pull Request.
- **NUNCA `--force` / force-push** sobre `main` (bloqueado).
- **NUNCA borres la rama `main`** (bloqueado).
- **Commits FIRMADOS obligatorios** (GPG/SSH). Un commit sin firma no se puede mergear.
- **Historial lineal**: nada de merge commits; usa rebase o squash.
- Las conversaciones del PR deben estar resueltas antes de mergear.

## Flujo correcto para cualquier cambio
1. Crear rama desde `main`:  `git checkout -b feat/<descripcion>`
2. Hacer commits **firmados**:  `git commit -S -m "..."`  (o tener `commit.gpgsign=true`)
3. Push de la rama (no de main):  `git push -u origin feat/<descripcion>`
4. Abrir PR:  `gh pr create --fill --base main`
5. Mergear con squash o rebase:  `gh pr merge --squash`  (aprobaciones requeridas = 0, así que el owner puede mergear sus propios PRs)

## Importante
- Si `git commit` falla por firma, parar y avisar al usuario (hay que configurar `user.signingkey` + `commit.gpgsign`).
- No intentar desactivar la protección de rama salvo que el usuario lo pida explícitamente.
