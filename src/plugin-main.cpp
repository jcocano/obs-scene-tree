#include <obs-module.h>
#include <obs-frontend-api.h>

#include <cstring>

#include "scene-tree-dock.hpp"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-tree", "en-US")

#define OBS_TREE_DOCK_ID "obs_tree_dock"

MODULE_EXPORT const char *obs_module_name(void)
{
	return "obs-tree";
}

MODULE_EXPORT const char *obs_module_description(void)
{
	return "Organise scenes into folders with a tree dock.";
}

bool obs_module_load(void)
{
	// Ownership of the widget transfers to OBS, which wraps it in a managed
	// dock (toggle appears under the Docks menu) and deletes it on removal.
	SceneTreeDock *dock = new SceneTreeDock();
	dock->setObjectName(OBS_TREE_DOCK_ID);

	// obs_module_text returns the key unchanged when the locale failed to
	// load; fall back to a readable title so the dock never shows the key.
	const char *title = obs_module_text("ObsTree.DockTitle");
	if (!title || strcmp(title, "ObsTree.DockTitle") == 0)
		title = "Scene Tree";

	bool added = obs_frontend_add_dock_by_id(OBS_TREE_DOCK_ID, title, dock);
	if (!added) {
		delete dock;
		blog(LOG_WARNING, "[obs-tree] failed to register dock");
		return false;
	}

	blog(LOG_INFO, "[obs-tree] loaded (v%s)", PLUGIN_VERSION);
	return true;
}

void obs_module_unload(void)
{
	obs_frontend_remove_dock(OBS_TREE_DOCK_ID);
	blog(LOG_INFO, "[obs-tree] unloaded");
}
