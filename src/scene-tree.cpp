#include "scene-tree.hpp"

#include <QDragMoveEvent>
#include <QDropEvent>
#include <QStyledItemDelegate>

using namespace obstree;

namespace {

// Ensures the inline rename editor gets its full natural height instead of
// being clipped to a short row, so descenders (g, j, p...) aren't cut off.
class RenameDelegate : public QStyledItemDelegate {
public:
	using QStyledItemDelegate::QStyledItemDelegate;

	void updateEditorGeometry(QWidget *editor,
				  const QStyleOptionViewItem &option,
				  const QModelIndex &) const override
	{
		QRect rect = option.rect;
		const int needed = editor->sizeHint().height();
		if (needed > rect.height()) {
			const int extra = needed - rect.height();
			rect.adjust(0, -(extra / 2), 0, extra - extra / 2);
		}
		editor->setGeometry(rect);
	}
};

} // namespace

SceneTree::SceneTree(QWidget *parent) : QTreeWidget(parent)
{
	setHeaderHidden(true);
	setSelectionMode(QAbstractItemView::SingleSelection);
	setDragEnabled(true);
	setAcceptDrops(true);
	setDropIndicatorShown(true);
	setDragDropMode(QAbstractItemView::InternalMove);
	// Prevent dropping ONTO a leaf and replacing it; we want insertion.
	setDragDropOverwriteMode(false);
	setExpandsOnDoubleClick(false);
	// Rename via double-click or F2 (single click is reserved for switching
	// the active scene, like the native scene list).
	setEditTriggers(QAbstractItemView::DoubleClicked |
			QAbstractItemView::EditKeyPressed);
	setUniformRowHeights(true);
	setAnimated(true);
	// Give rows a little breathing room...
	setStyleSheet("QTreeView::item { min-height: 24px; }");
	// ...and let the rename editor claim its full height so descenders
	// (g, j, p...) are never clipped.
	setItemDelegate(new RenameDelegate(this));
}

// A drop is forbidden when it would land directly on a scene item (scenes are
// leaves and cannot contain children). Folders and the root are valid targets.
bool SceneTree::isDropForbidden(const QPoint &pos) const
{
	if (dropIndicatorPosition() != QAbstractItemView::OnItem)
		return false; // inserting between rows is always fine

	QTreeWidgetItem *target = itemAt(pos);
	if (!target)
		return false;

	return target->data(0, TypeRole).toInt() == SceneItem;
}

void SceneTree::dragMoveEvent(QDragMoveEvent *event)
{
	QTreeWidget::dragMoveEvent(event);
	if (event->isAccepted() && isDropForbidden(event->position().toPoint()))
		event->ignore();
}

void SceneTree::dropEvent(QDropEvent *event)
{
	if (isDropForbidden(event->position().toPoint())) {
		event->ignore();
		return;
	}

	QTreeWidget::dropEvent(event);

	if (event->isAccepted())
		emit itemsRearranged();
}
