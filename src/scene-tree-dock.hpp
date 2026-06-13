#pragma once

#include <QWidget>
#include <QString>
#include <QIcon>

#include <obs.h>
#include <obs-frontend-api.h>

class SceneTree;
class QTreeWidgetItem;
class QAction;
class QEvent;
class QToolBar;

// Dock widget that renders OBS scenes in a user-organisable folder tree.
//
// Folder layout is persisted inside the active scene collection through the
// frontend save callback, keyed by each scene's stable source UUID so that the
// arrangement survives renames. Scene add/remove/rename and active-scene
// changes are reflected live via the frontend event callback.
class SceneTreeDock : public QWidget {
	Q_OBJECT

public:
	explicit SceneTreeDock(QWidget *parent = nullptr);
	~SceneTreeDock() override;

protected:
	void changeEvent(QEvent *event) override;

private slots:
	void onSelectionChanged();
	void onItemChanged(QTreeWidgetItem *item, int column);
	void onItemsRearranged();
	void addScene();
	void addFolder();
	void addSubfolder();
	void removeSelected();
	void renameSelected();
	void moveUp();
	void moveDown();
	void showContextMenu(const QPoint &pos);

	// Scene-specific actions mirroring OBS's native scene context menu.
	void duplicateScene();
	void copySceneFilters();
	void pasteSceneFilters();
	void moveToTop();
	void moveToBottom();
	void toggleMultiview();
	void screenshotScene();
	void openSceneFilters();

private:
	// Frontend C callbacks -> instance methods (always on the UI thread).
	static void onFrontendEvent(enum obs_frontend_event event, void *ptr);
	static void onFrontendSave(obs_data_t *save_data, bool saving, void *ptr);
	void handleFrontendEvent(enum obs_frontend_event event);

	// Persistence.
	obs_data_t *serialize() const;
	void saveNode(QTreeWidgetItem *item, obs_data_array_t *array) const;
	void loadFrom(obs_data_t *data);
	void loadNode(obs_data_t *node, QTreeWidgetItem *parent);
	void requestSave();

	// Sync with OBS scene list.
	void reconcileScenes();
	void selectCurrentScene();
	// Places a just-created scene into its target folder once OBS surfaces
	// it (the scene-create notification may be delivered asynchronously).
	void applyPendingScene();

	// Item helpers.
	QTreeWidgetItem *makeFolderItem(const QString &name, bool expanded) const;
	QTreeWidgetItem *makeSceneItem(const QString &uuid,
				       const QString &name) const;
	// (Re)build themed icons and re-apply them to the toolbar and items.
	void buildIcons();
	// Style the bottom toolbar to match OBS's native dock toolbars.
	void applyToolbarStyle();
	QTreeWidgetItem *findSceneItem(const QString &uuid) const;
	// Folder that new items should drop into given the current selection
	// (the selected folder, or a selected scene's parent folder), or null
	// for the tree root.
	QTreeWidgetItem *targetFolder() const;
	void createFolder(QTreeWidgetItem *parent);
	QString uniqueSceneName(const QString &base) const;
	void moveSelected(int delta); // reorder current item among its siblings
	void moveToEdge(bool top);    // reorder to first/last among its siblings

	// Returns the AddRef'd OBS source for the selected scene item, or null
	// when the selection is not a scene. Caller must obs_source_release().
	obs_source_t *currentSceneSource() const;

	// Populate the right-click menu for each kind of clicked target.
	void buildSceneMenu(QMenu &menu, QTreeWidgetItem *item);
	void buildFolderMenu(QMenu &menu, QTreeWidgetItem *item);
	void appendCommonTreeActions(QMenu &menu);

	SceneTree *tree = nullptr;
	QToolBar *toolbar = nullptr;

	// Toolbar actions (kept so their icons can be re-themed live).
	QAction *addSceneAction = nullptr;
	QAction *addFolderAction = nullptr;
	QAction *removeAction = nullptr;
	QAction *filtersAction = nullptr;
	QAction *upAction = nullptr;
	QAction *downAction = nullptr;

	// Themed icons, rebuilt on palette/style changes.
	QIcon folderIcon;
	QIcon sceneIcon;
	QIcon addIcon;
	QIcon removeIcon;
	QIcon upIcon;
	QIcon downIcon;
	QIcon filtersIcon;

	// Guards to suppress feedback loops while we mutate the tree ourselves.
	bool suppressSelection = false;
	bool suppressSave = false;

	// A scene created from this dock, awaiting placement once OBS lists it.
	QString pendingSceneUuid;
	QTreeWidgetItem *pendingSceneFolder = nullptr;

	// UUID of the scene whose filters were last copied (for Paste Filters).
	QString copiedFiltersUuid;
};
