#pragma once

#include <QTreeWidget>

namespace obstree {

// Item type stored in TypeRole.
enum ItemType {
	FolderItem = 1,
	SceneItem = 2,
};

// Custom data roles on column 0.
enum ItemRole {
	TypeRole = Qt::UserRole + 1, // int ItemType
	UuidRole = Qt::UserRole + 2, // QString scene source UUID
};

} // namespace obstree

// QTreeWidget subclass that supports internal drag & drop reordering while
// only allowing folders (and the invisible root) to act as drop targets.
class SceneTree : public QTreeWidget {
	Q_OBJECT

public:
	explicit SceneTree(QWidget *parent = nullptr);

signals:
	// Emitted after a successful internal move so the dock can persist.
	void itemsRearranged();

protected:
	void dragMoveEvent(QDragMoveEvent *event) override;
	void dropEvent(QDropEvent *event) override;

private:
	bool isDropForbidden(const QPoint &pos) const;
};
