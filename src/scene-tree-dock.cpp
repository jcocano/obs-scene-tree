#include "scene-tree-dock.hpp"
#include "scene-tree.hpp"
#include "icons.hpp"

#include <QVBoxLayout>
#include <QToolBar>
#include <QAction>
#include <QMenu>
#include <QTreeWidgetItem>
#include <QMessageBox>
#include <QSet>
#include <QPalette>
#include <QEvent>

#include <obs-module.h>

#include <functional>

using namespace obstree;

static QTreeWidgetItem *rootOrParent(QTreeWidgetItem *item)
{
	return item->parent() ? item->parent()
			      : item->treeWidget()->invisibleRootItem();
}

SceneTreeDock::SceneTreeDock(QWidget *parent) : QWidget(parent)
{
	tree = new SceneTree(this);
	tree->setContextMenuPolicy(Qt::CustomContextMenu);
	tree->setFrameShape(QFrame::NoFrame); // blend into the dock, no inset box

	buildIcons();

	// Thin bottom toolbar, mirroring OBS's native scene/source docks.
	toolbar = new QToolBar(this);
	toolbar->setIconSize(QSize(16, 16));
	toolbar->setMovable(false);
	toolbar->setFloatable(false);
	toolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);

	addSceneAction = toolbar->addAction(
		addIcon, QString::fromUtf8(obs_module_text("ObsTree.NewScene")));
	addSceneAction->setToolTip(
		QString::fromUtf8(obs_module_text("ObsTree.NewSceneTip")));

	addFolderAction = toolbar->addAction(
		folderIcon,
		QString::fromUtf8(obs_module_text("ObsTree.NewFolder")));
	addFolderAction->setToolTip(
		QString::fromUtf8(obs_module_text("ObsTree.NewFolderTip")));

	removeAction = toolbar->addAction(
		removeIcon, QString::fromUtf8(obs_module_text("ObsTree.Delete")));
	removeAction->setToolTip(
		QString::fromUtf8(obs_module_text("ObsTree.DeleteTip")));

	// Light-grey separator before the reorder chevrons, mirroring the grouped
	// clusters in OBS's native scene/source dock toolbars ([+ folder trash] |
	// [up down]).
	toolbar->addSeparator();

	// Reorder chevrons sit in the same left-aligned cluster as OBS's native
	// scene/source dock toolbars (no expanding spacer).
	upAction = toolbar->addAction(
		upIcon, QString::fromUtf8(obs_module_text("ObsTree.MoveUp")));
	upAction->setToolTip(QString::fromUtf8(obs_module_text("ObsTree.MoveUp")));

	downAction = toolbar->addAction(
		downIcon, QString::fromUtf8(obs_module_text("ObsTree.MoveDown")));
	downAction->setToolTip(
		QString::fromUtf8(obs_module_text("ObsTree.MoveDown")));

	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);
	layout->addWidget(tree);
	layout->addWidget(toolbar);
	setLayout(layout);

	applyToolbarStyle();

	connect(addSceneAction, &QAction::triggered, this,
		&SceneTreeDock::addScene);
	connect(addFolderAction, &QAction::triggered, this,
		&SceneTreeDock::addFolder);
	connect(removeAction, &QAction::triggered, this,
		&SceneTreeDock::removeSelected);
	connect(upAction, &QAction::triggered, this, &SceneTreeDock::moveUp);
	connect(downAction, &QAction::triggered, this, &SceneTreeDock::moveDown);
	connect(tree, &QTreeWidget::itemSelectionChanged, this,
		&SceneTreeDock::onSelectionChanged);
	connect(tree, &QTreeWidget::itemChanged, this,
		&SceneTreeDock::onItemChanged);
	connect(tree, &SceneTree::itemsRearranged, this,
		&SceneTreeDock::onItemsRearranged);
	connect(tree, &QWidget::customContextMenuRequested, this,
		&SceneTreeDock::showContextMenu);

	obs_frontend_add_event_callback(onFrontendEvent, this);
	obs_frontend_add_save_callback(onFrontendSave, this);
}

SceneTreeDock::~SceneTreeDock()
{
	obs_frontend_remove_save_callback(onFrontendSave, this);
	obs_frontend_remove_event_callback(onFrontendEvent, this);
}

void SceneTreeDock::buildIcons()
{
	const QColor color = palette().color(QPalette::WindowText);
	folderIcon = obstree::folderIcon(color);
	sceneIcon = obstree::sceneIcon(color);
	addIcon = obstree::addIcon(color);
	removeIcon = obstree::removeIcon(color);
	upIcon = obstree::upIcon(color);
	downIcon = obstree::downIcon(color);

	if (addSceneAction)
		addSceneAction->setIcon(addIcon);
	if (addFolderAction)
		addFolderAction->setIcon(folderIcon);
	if (removeAction)
		removeAction->setIcon(removeIcon);
	if (upAction)
		upAction->setIcon(upIcon);
	if (downAction)
		downAction->setIcon(downIcon);

	if (!tree)
		return;

	suppressSave = true;
	std::function<void(QTreeWidgetItem *)> walk = [&](QTreeWidgetItem *node) {
		for (int i = 0; i < node->childCount(); ++i) {
			QTreeWidgetItem *child = node->child(i);
			child->setIcon(0,
				       child->data(0, TypeRole).toInt() ==
						       FolderItem
					       ? folderIcon
					       : sceneIcon);
			walk(child);
		}
	};
	walk(tree->invisibleRootItem());
	suppressSave = false;
}

void SceneTreeDock::applyToolbarStyle()
{
	if (!toolbar)
		return;

	// Replicate OBS's native scene/source dock toolbar: a flat strip the same
	// grey as the panel, topped by a light divider, with each icon sitting in
	// a slightly darker rounded square outlined in light grey. The shades are
	// derived from the panel background so the look holds in both dark and
	// light OBS themes (OBS's own roles don't reach our non-OBSDock widget).
	const QColor bg = palette().color(QPalette::Base);
	const bool dark = bg.lightness() < 128;
	// Darker inset box behind each icon (the "fondo cuadrado mas obscuro").
	const QColor button = dark ? bg.darker(125) : bg.lighter(112);
	// Light-grey divider/outline lines.
	const QColor line = dark ? bg.lighter(160) : bg.darker(125);
	// Hover lifts the box a touch; pressed sinks it.
	const QColor hover = dark ? bg.darker(108) : bg.lighter(106);
	const QColor pressed = dark ? bg.darker(145) : bg.lighter(120);

	toolbar->setStyleSheet(
		QStringLiteral(
			"QToolBar { background: %1; border: none;"
			" border-top: 1px solid %2; padding: 3px 5px;"
			" spacing: 4px; }"
			"QToolBar QToolButton { background: %3;"
			" border: 1px solid %2; border-radius: 4px;"
			" margin: 0px; padding: 4px; }"
			"QToolBar QToolButton:hover { background: %4; }"
			"QToolBar QToolButton:pressed { background: %5; }"
			"QToolBar::separator { background: %2; width: 1px;"
			" margin: 3px 4px; }")
			.arg(bg.name(), line.name(), button.name(),
			     hover.name(), pressed.name()));
}

void SceneTreeDock::changeEvent(QEvent *event)
{
	switch (event->type()) {
	case QEvent::PaletteChange:
	case QEvent::StyleChange:
	case QEvent::ThemeChange:
		buildIcons();       // re-tint icons to the (possibly new) theme
		applyToolbarStyle(); // and re-derive the toolbar colours
		break;
	default:
		break;
	}
	QWidget::changeEvent(event);
}

// ---------------------------------------------------------------------------
// Item helpers
// ---------------------------------------------------------------------------

QTreeWidgetItem *SceneTreeDock::makeFolderItem(const QString &name,
					       bool expanded) const
{
	auto *item = new QTreeWidgetItem;
	item->setText(0, name);
	item->setData(0, TypeRole, FolderItem);
	item->setIcon(0, folderIcon);
	item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable |
		       Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled |
		       Qt::ItemIsEditable);
	item->setExpanded(expanded);
	return item;
}

QTreeWidgetItem *SceneTreeDock::makeSceneItem(const QString &uuid,
					      const QString &name) const
{
	auto *item = new QTreeWidgetItem;
	item->setText(0, name);
	item->setData(0, TypeRole, SceneItem);
	item->setData(0, UuidRole, uuid);
	item->setIcon(0, sceneIcon);
	// No ItemIsDropEnabled: scenes are leaves and cannot receive children.
	item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable |
		       Qt::ItemIsDragEnabled | Qt::ItemIsEditable);
	return item;
}

QTreeWidgetItem *SceneTreeDock::findSceneItem(const QString &uuid) const
{
	QTreeWidgetItem *found = nullptr;
	std::function<void(QTreeWidgetItem *)> walk = [&](QTreeWidgetItem *node) {
		for (int i = 0; i < node->childCount() && !found; ++i) {
			QTreeWidgetItem *child = node->child(i);
			if (child->data(0, TypeRole).toInt() == SceneItem) {
				if (child->data(0, UuidRole).toString() == uuid)
					found = child;
			} else {
				walk(child);
			}
		}
	};
	walk(tree->invisibleRootItem());
	return found;
}

// ---------------------------------------------------------------------------
// Persistence
// ---------------------------------------------------------------------------

void SceneTreeDock::saveNode(QTreeWidgetItem *item,
			     obs_data_array_t *array) const
{
	obs_data_t *node = obs_data_create();

	if (item->data(0, TypeRole).toInt() == FolderItem) {
		obs_data_set_string(node, "type", "folder");
		obs_data_set_string(node, "name",
				    item->text(0).toUtf8().constData());
		obs_data_set_bool(node, "expanded", item->isExpanded());

		obs_data_array_t *children = obs_data_array_create();
		for (int i = 0; i < item->childCount(); ++i)
			saveNode(item->child(i), children);
		obs_data_set_array(node, "children", children);
		obs_data_array_release(children);
	} else {
		obs_data_set_string(node, "type", "scene");
		obs_data_set_string(
			node, "uuid",
			item->data(0, UuidRole).toString().toUtf8().constData());
		obs_data_set_string(node, "name",
				    item->text(0).toUtf8().constData());
	}

	obs_data_array_push_back(array, node);
	obs_data_release(node);
}

obs_data_t *SceneTreeDock::serialize() const
{
	obs_data_t *root = obs_data_create();
	obs_data_set_int(root, "version", 1);

	obs_data_array_t *roots = obs_data_array_create();
	QTreeWidgetItem *inv = tree->invisibleRootItem();
	for (int i = 0; i < inv->childCount(); ++i)
		saveNode(inv->child(i), roots);
	obs_data_set_array(root, "roots", roots);
	obs_data_array_release(roots);

	return root;
}

void SceneTreeDock::loadNode(obs_data_t *node, QTreeWidgetItem *parent)
{
	const char *type = obs_data_get_string(node, "type");

	if (strcmp(type, "folder") == 0) {
		bool expanded = obs_data_get_bool(node, "expanded");
		QTreeWidgetItem *folder = makeFolderItem(
			QString::fromUtf8(obs_data_get_string(node, "name")),
			expanded);
		parent->addChild(folder);
		folder->setExpanded(expanded);

		obs_data_array_t *children = obs_data_get_array(node, "children");
		size_t count = obs_data_array_count(children);
		for (size_t i = 0; i < count; ++i) {
			obs_data_t *child = obs_data_array_item(children, i);
			loadNode(child, folder);
			obs_data_release(child);
		}
		obs_data_array_release(children);
	} else {
		QTreeWidgetItem *scene = makeSceneItem(
			QString::fromUtf8(obs_data_get_string(node, "uuid")),
			QString::fromUtf8(obs_data_get_string(node, "name")));
		parent->addChild(scene);
	}
}

void SceneTreeDock::loadFrom(obs_data_t *data)
{
	suppressSave = true;
	suppressSelection = true;
	tree->clear();

	if (data) {
		obs_data_array_t *roots = obs_data_get_array(data, "roots");
		size_t count = obs_data_array_count(roots);
		for (size_t i = 0; i < count; ++i) {
			obs_data_t *node = obs_data_array_item(roots, i);
			loadNode(node, tree->invisibleRootItem());
			obs_data_release(node);
		}
		obs_data_array_release(roots);
	}

	suppressSelection = false;
	suppressSave = false;

	reconcileScenes();
	selectCurrentScene();
}

void SceneTreeDock::requestSave()
{
	if (suppressSave)
		return;
	// Persist immediately by asking OBS to save the scene collection, which
	// invokes our frontend save callback.
	obs_frontend_save();
}

// ---------------------------------------------------------------------------
// Sync with OBS
// ---------------------------------------------------------------------------

void SceneTreeDock::reconcileScenes()
{
	struct obs_frontend_source_list scenes = {};
	obs_frontend_get_scenes(&scenes);

	QHash<QString, QString> nameByUuid;
	for (size_t i = 0; i < scenes.sources.num; ++i) {
		obs_source_t *src = scenes.sources.array[i];
		const char *uuid = obs_source_get_uuid(src);
		if (!uuid)
			continue;
		nameByUuid.insert(QString::fromUtf8(uuid),
				  QString::fromUtf8(obs_source_get_name(src)));
	}

	suppressSave = true;
	suppressSelection = true;

	// 1. Drop scene items whose source no longer exists; refresh kept names.
	QList<QTreeWidgetItem *> stale;
	std::function<void(QTreeWidgetItem *)> walk = [&](QTreeWidgetItem *node) {
		for (int i = 0; i < node->childCount(); ++i) {
			QTreeWidgetItem *child = node->child(i);
			if (child->data(0, TypeRole).toInt() == SceneItem) {
				QString uuid =
					child->data(0, UuidRole).toString();
				auto it = nameByUuid.find(uuid);
				if (it == nameByUuid.end())
					stale.append(child);
				else
					child->setText(0, it.value());
			} else {
				walk(child);
			}
		}
	};
	walk(tree->invisibleRootItem());
	qDeleteAll(stale);

	// 2. Append scenes that are not yet represented anywhere in the tree.
	for (size_t i = 0; i < scenes.sources.num; ++i) {
		obs_source_t *src = scenes.sources.array[i];
		const char *uuid = obs_source_get_uuid(src);
		if (!uuid)
			continue;
		QString quuid = QString::fromUtf8(uuid);
		if (!findSceneItem(quuid)) {
			QTreeWidgetItem *item = makeSceneItem(
				quuid, nameByUuid.value(quuid));
			tree->invisibleRootItem()->addChild(item);
		}
	}

	suppressSelection = false;
	suppressSave = false;

	obs_frontend_source_list_free(&scenes);

	applyPendingScene();
}

void SceneTreeDock::selectCurrentScene()
{
	obs_source_t *current = obs_frontend_get_current_scene();
	if (!current)
		return;

	QString uuid;
	if (const char *u = obs_source_get_uuid(current))
		uuid = QString::fromUtf8(u);
	obs_source_release(current);

	QTreeWidgetItem *item = findSceneItem(uuid);
	if (!item)
		return;

	suppressSelection = true;
	tree->setCurrentItem(item);
	tree->scrollToItem(item);
	suppressSelection = false;
}

// ---------------------------------------------------------------------------
// Frontend callbacks
// ---------------------------------------------------------------------------

void SceneTreeDock::onFrontendEvent(enum obs_frontend_event event, void *ptr)
{
	static_cast<SceneTreeDock *>(ptr)->handleFrontendEvent(event);
}

void SceneTreeDock::handleFrontendEvent(enum obs_frontend_event event)
{
	switch (event) {
	case OBS_FRONTEND_EVENT_FINISHED_LOADING:
	case OBS_FRONTEND_EVENT_SCENE_LIST_CHANGED:
	case OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGED:
		reconcileScenes();
		selectCurrentScene();
		break;
	case OBS_FRONTEND_EVENT_SCENE_CHANGED:
		selectCurrentScene();
		break;
	case OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGING:
	case OBS_FRONTEND_EVENT_SCENE_COLLECTION_CLEANUP:
		suppressSave = true;
		tree->clear();
		suppressSave = false;
		break;
	default:
		break;
	}
}

void SceneTreeDock::onFrontendSave(obs_data_t *save_data, bool saving, void *ptr)
{
	auto *self = static_cast<SceneTreeDock *>(ptr);
	if (saving) {
		obs_data_t *root = self->serialize();
		obs_data_set_obj(save_data, "obs-tree", root);
		obs_data_release(root);
	} else {
		obs_data_t *root = obs_data_get_obj(save_data, "obs-tree");
		self->loadFrom(root);
		obs_data_release(root);
	}
}

// ---------------------------------------------------------------------------
// UI slots
// ---------------------------------------------------------------------------

void SceneTreeDock::onSelectionChanged()
{
	if (suppressSelection)
		return;

	QTreeWidgetItem *item = tree->currentItem();
	if (!item || item->data(0, TypeRole).toInt() != SceneItem)
		return;

	QString uuid = item->data(0, UuidRole).toString();
	obs_source_t *src =
		obs_get_source_by_uuid(uuid.toUtf8().constData());
	if (src) {
		obs_frontend_set_current_scene(src);
		obs_source_release(src);
	}
}

void SceneTreeDock::onItemChanged(QTreeWidgetItem *item, int)
{
	if (suppressSave)
		return; // programmatic change during reconcile/load

	// Renaming a scene item pushes the new name back to the OBS source.
	if (item && item->data(0, TypeRole).toInt() == SceneItem) {
		QString uuid = item->data(0, UuidRole).toString();
		obs_source_t *src =
			obs_get_source_by_uuid(uuid.toUtf8().constData());
		if (src) {
			obs_source_set_name(
				src, item->text(0).toUtf8().constData());
			obs_source_release(src);
		}
	}

	requestSave();
}

void SceneTreeDock::onItemsRearranged()
{
	requestSave();
}

QTreeWidgetItem *SceneTreeDock::targetFolder() const
{
	// Based on the actual selection: clicking empty space deselects, which
	// is how the user asks for a root-level folder.
	const QList<QTreeWidgetItem *> selected = tree->selectedItems();
	if (selected.isEmpty())
		return nullptr;

	QTreeWidgetItem *item = selected.first();
	if (item->data(0, TypeRole).toInt() == FolderItem)
		return item; // create inside this folder
	return item->parent(); // a scene's parent folder, or null at root
}

QString SceneTreeDock::uniqueSceneName(const QString &base) const
{
	QSet<QString> names;
	struct obs_frontend_source_list scenes = {};
	obs_frontend_get_scenes(&scenes);
	for (size_t i = 0; i < scenes.sources.num; ++i)
		names.insert(QString::fromUtf8(
			obs_source_get_name(scenes.sources.array[i])));
	obs_frontend_source_list_free(&scenes);

	if (!names.contains(base))
		return base;
	for (int i = 2;; ++i) {
		QString candidate = QStringLiteral("%1 %2").arg(base).arg(i);
		if (!names.contains(candidate))
			return candidate;
	}
}

void SceneTreeDock::addScene()
{
	QString name = uniqueSceneName(
		QString::fromUtf8(obs_module_text("ObsTree.NewScene")));

	// Capture the destination folder from the current selection *before* the
	// new scene steals it.
	pendingSceneFolder = targetFolder();

	obs_scene_t *scene = obs_scene_create(name.toUtf8().constData());
	if (!scene) {
		pendingSceneFolder = nullptr;
		return;
	}
	obs_source_t *src = obs_scene_get_source(scene);
	if (const char *u = obs_source_get_uuid(src))
		pendingSceneUuid = QString::fromUtf8(u);
	obs_scene_release(scene);

	// If OBS surfaced the scene synchronously this places it right away;
	// otherwise applyPendingScene() runs again on the scene-list event.
	reconcileScenes();
}

void SceneTreeDock::applyPendingScene()
{
	if (pendingSceneUuid.isEmpty())
		return;

	QTreeWidgetItem *item = findSceneItem(pendingSceneUuid);
	if (!item)
		return; // not listed yet; retried on the next reconcile

	QTreeWidgetItem *folder = pendingSceneFolder;
	pendingSceneUuid.clear();
	pendingSceneFolder = nullptr;

	if (folder) {
		QTreeWidgetItem *oldParent =
			item->parent() ? item->parent()
				       : tree->invisibleRootItem();
		if (oldParent != folder) {
			oldParent->removeChild(item);
			folder->addChild(item);
			folder->setExpanded(true);
		}
	}

	tree->setCurrentItem(item); // switches OBS to the new scene
	requestSave();
	tree->editItem(item, 0);    // let the user name it inline
}

void SceneTreeDock::createFolder(QTreeWidgetItem *parent)
{
	QTreeWidgetItem *folder = makeFolderItem(
		QString::fromUtf8(obs_module_text("ObsTree.NewFolder")), true);

	if (parent) {
		parent->addChild(folder);
		parent->setExpanded(true);
	} else {
		tree->invisibleRootItem()->addChild(folder);
	}

	tree->setCurrentItem(folder);
	requestSave();
	tree->editItem(folder, 0);
}

void SceneTreeDock::addFolder()
{
	// Nest into the selected folder; root if nothing is selected.
	createFolder(targetFolder());
}

void SceneTreeDock::addSubfolder()
{
	createFolder(targetFolder());
}

void SceneTreeDock::renameSelected()
{
	if (QTreeWidgetItem *item = tree->currentItem())
		tree->editItem(item, 0);
}

void SceneTreeDock::moveSelected(int delta)
{
	QTreeWidgetItem *item = tree->currentItem();
	if (!item)
		return;

	QTreeWidgetItem *parent =
		item->parent() ? item->parent() : tree->invisibleRootItem();
	const int from = parent->indexOfChild(item);
	const int to = from + delta;
	if (to < 0 || to >= parent->childCount())
		return;

	const bool expanded = item->isExpanded();
	parent->takeChild(from);     // keeps the item's subtree intact
	parent->insertChild(to, item);
	item->setExpanded(expanded);

	tree->setCurrentItem(item);
	requestSave();
}

void SceneTreeDock::moveUp()
{
	moveSelected(-1);
}

void SceneTreeDock::moveDown()
{
	moveSelected(1);
}

void SceneTreeDock::removeSelected()
{
	QTreeWidgetItem *item = tree->currentItem();
	if (!item)
		return;

	// Folder: re-home its children to the parent so nothing is lost.
	if (item->data(0, TypeRole).toInt() == FolderItem) {
		QTreeWidgetItem *parent = rootOrParent(item);
		int index = parent->indexOfChild(item);
		QList<QTreeWidgetItem *> children = item->takeChildren();
		for (int i = 0; i < children.size(); ++i)
			parent->insertChild(index + 1 + i, children[i]);
		delete item;
		requestSave();
		return;
	}

	// Scene: OBS must always keep at least one scene.
	struct obs_frontend_source_list scenes = {};
	obs_frontend_get_scenes(&scenes);
	size_t sceneCount = scenes.sources.num;
	obs_frontend_source_list_free(&scenes);
	if (sceneCount <= 1) {
		QMessageBox::information(
			this,
			QString::fromUtf8(obs_module_text("ObsTree.DockTitle")),
			QString::fromUtf8(
				obs_module_text("ObsTree.CannotDeleteLast")));
		return;
	}

	auto reply = QMessageBox::question(
		this,
		QString::fromUtf8(obs_module_text("ObsTree.DeleteScene")),
		QString::fromUtf8(obs_module_text("ObsTree.ConfirmDeleteScene"))
			.arg(item->text(0)));
	if (reply != QMessageBox::Yes)
		return;

	QString uuid = item->data(0, UuidRole).toString();
	obs_source_t *src = obs_get_source_by_uuid(uuid.toUtf8().constData());
	if (src) {
		obs_source_remove(src);
		obs_source_release(src);
	}
	// The tree item is dropped by reconcile on the scene-list-changed event.
}

void SceneTreeDock::showContextMenu(const QPoint &pos)
{
	QTreeWidgetItem *item = tree->itemAt(pos);
	if (item)
		tree->setCurrentItem(item); // act on what was right-clicked
	else
		tree->clearSelection(); // empty space -> root-level actions

	QMenu menu(this);

	// "New folder" nests into the clicked folder, or creates at root when
	// clicking empty space.
	QAction *newScene = menu.addAction(
		addIcon, QString::fromUtf8(obs_module_text("ObsTree.NewScene")));
	connect(newScene, &QAction::triggered, this, &SceneTreeDock::addScene);

	QAction *newFolder = menu.addAction(
		folderIcon,
		QString::fromUtf8(obs_module_text("ObsTree.NewFolder")));
	connect(newFolder, &QAction::triggered, this, &SceneTreeDock::addFolder);

	if (item) {
		menu.addSeparator();
		QAction *rename = menu.addAction(
			QString::fromUtf8(obs_module_text("ObsTree.Rename")));
		connect(rename, &QAction::triggered, this,
			&SceneTreeDock::renameSelected);

		QAction *del = menu.addAction(
			removeIcon,
			QString::fromUtf8(obs_module_text("ObsTree.Delete")));
		connect(del, &QAction::triggered, this,
			&SceneTreeDock::removeSelected);
	}

	menu.addSeparator();
	QAction *expand = menu.addAction(
		QString::fromUtf8(obs_module_text("ObsTree.Expand")));
	connect(expand, &QAction::triggered, tree, &QTreeWidget::expandAll);
	QAction *collapse = menu.addAction(
		QString::fromUtf8(obs_module_text("ObsTree.Collapse")));
	connect(collapse, &QAction::triggered, tree, &QTreeWidget::collapseAll);

	menu.exec(tree->viewport()->mapToGlobal(pos));
}
