#include "icons.hpp"

#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QSvgRenderer>
#include <QFileInfo>
#include <QStringList>

#include <functional>

using DrawFn = std::function<void(QPainter &, const QColor &)>;

namespace {

const int kIconSizes[] = {16, 18, 20, 24, 32, 48, 64};

// ---------------------------------------------------------------------------
// OBS native theme SVGs (preferred)
// ---------------------------------------------------------------------------

// Resolve the directory holding OBS's Dark theme icon SVGs once. The glyphs are
// identical across the Dark/Light folders (only their fill differs, and we
// re-tint anyway), so the Dark folder is fine as the source for either theme.
QString obsIconDir()
{
	static QString cached = [] {
		const QStringList bases = {
			QStringLiteral("/usr/share/obs/obs-studio/themes"),
			QStringLiteral("/usr/local/share/obs/obs-studio/themes"),
			QStringLiteral("/app/share/obs/obs-studio/themes"),
		};
		for (const QString &base : bases) {
			QString dir = base + QStringLiteral("/Dark");
			if (QFileInfo::exists(dir + QStringLiteral("/plus.svg")))
				return dir;
		}
		return QString();
	}();
	return cached;
}

// Render an SVG at a size and recolour it to `color`, preserving its alpha.
QPixmap renderSvg(const QString &file, int size, const QColor &color)
{
	QSvgRenderer renderer(file);
	if (!renderer.isValid())
		return QPixmap();

	QPixmap pm(size, size);
	pm.fill(Qt::transparent);
	QPainter p(&pm);
	p.setRenderHint(QPainter::Antialiasing, true);
	renderer.render(&p, QRectF(0, 0, size, size));
	p.setCompositionMode(QPainter::CompositionMode_SourceIn);
	p.fillRect(pm.rect(), color);
	p.end();
	return pm;
}

QIcon obsIcon(const QString &relPath, const QColor &color)
{
	const QString dir = obsIconDir();
	if (dir.isEmpty())
		return QIcon();
	const QString file = dir + QStringLiteral("/") + relPath;
	if (!QFileInfo::exists(file))
		return QIcon();

	QIcon icon;
	for (int s : kIconSizes) {
		QPixmap pm = renderSvg(file, s, color);
		if (pm.isNull())
			return QIcon();
		icon.addPixmap(pm);
	}
	return icon;
}

// ---------------------------------------------------------------------------
// Hand-drawn fallbacks (only used if OBS assets aren't found)
// ---------------------------------------------------------------------------

QPixmap renderDrawn(int size, const QColor &color, const DrawFn &draw)
{
	QPixmap pm(size, size);
	pm.fill(Qt::transparent);
	QPainter p(&pm);
	p.setRenderHint(QPainter::Antialiasing, true);
	p.scale(size / 100.0, size / 100.0);
	draw(p, color);
	p.end();
	return pm;
}

QIcon drawnIcon(const QColor &color, const DrawFn &draw)
{
	QIcon icon;
	for (int s : kIconSizes)
		icon.addPixmap(renderDrawn(s, color, draw));
	return icon;
}

QPen pen(const QColor &c, qreal w = 8.0)
{
	QPen p(c, w);
	p.setJoinStyle(Qt::RoundJoin);
	p.setCapStyle(Qt::RoundCap);
	return p;
}

void drawFolder(QPainter &p, const QColor &c)
{
	p.setPen(pen(c));
	p.setBrush(Qt::NoBrush);
	QPainterPath body;
	body.addRoundedRect(QRectF(14, 42, 72, 38), 9, 9);
	QPainterPath tab;
	tab.addRoundedRect(QRectF(14, 32, 32, 18), 8, 8);
	p.drawPath(body.united(tab));
}

void drawScene(QPainter &p, const QColor &c)
{
	// A clean 16:9 canvas (no play glyph, so it doesn't read as a video).
	p.setPen(pen(c));
	p.setBrush(Qt::NoBrush);
	p.drawRoundedRect(QRectF(14, 30, 72, 40), 9, 9);
}

void drawPlus(QPainter &p, const QColor &c)
{
	p.setPen(pen(c, 10));
	p.drawLine(QPointF(50, 24), QPointF(50, 76));
	p.drawLine(QPointF(24, 50), QPointF(76, 50));
}

void drawTrash(QPainter &p, const QColor &c)
{
	p.setPen(pen(c));
	p.setBrush(Qt::NoBrush);
	p.drawLine(QPointF(20, 34), QPointF(80, 34));
	QPainterPath handle;
	handle.moveTo(40, 34);
	handle.lineTo(42, 26);
	handle.lineTo(58, 26);
	handle.lineTo(60, 34);
	p.drawPath(handle);
	QPainterPath body;
	body.moveTo(30, 38);
	body.lineTo(34, 78);
	body.quadTo(35, 82, 39, 82);
	body.lineTo(61, 82);
	body.quadTo(65, 82, 66, 78);
	body.lineTo(70, 38);
	p.drawPath(body);
}

void drawChevron(QPainter &p, const QColor &c, bool up)
{
	p.setPen(pen(c, 10));
	if (up) {
		p.drawLine(QPointF(26, 60), QPointF(50, 38));
		p.drawLine(QPointF(50, 38), QPointF(74, 60));
	} else {
		p.drawLine(QPointF(26, 40), QPointF(50, 62));
		p.drawLine(QPointF(50, 62), QPointF(74, 40));
	}
}

} // namespace

namespace obstree {

QIcon folderIcon(const QColor &color)
{
	QIcon i = obsIcon(QStringLiteral("sources/group.svg"), color);
	return i.isNull() ? drawnIcon(color, drawFolder) : i;
}

QIcon sceneIcon(const QColor &color)
{
	QIcon i = obsIcon(QStringLiteral("sources/scene.svg"), color);
	return i.isNull() ? drawnIcon(color, drawScene) : i;
}

QIcon addIcon(const QColor &color)
{
	QIcon i = obsIcon(QStringLiteral("plus.svg"), color);
	return i.isNull() ? drawnIcon(color, drawPlus) : i;
}

QIcon removeIcon(const QColor &color)
{
	QIcon i = obsIcon(QStringLiteral("trash.svg"), color);
	return i.isNull() ? drawnIcon(color, drawTrash) : i;
}

QIcon upIcon(const QColor &color)
{
	QIcon i = obsIcon(QStringLiteral("up.svg"), color);
	return i.isNull() ? drawnIcon(color, [](QPainter &p, const QColor &c) {
		drawChevron(p, c, true);
	}) : i;
}

QIcon downIcon(const QColor &color)
{
	QIcon i = obsIcon(QStringLiteral("down.svg"), color);
	return i.isNull() ? drawnIcon(color, [](QPainter &p, const QColor &c) {
		drawChevron(p, c, false);
	}) : i;
}

} // namespace obstree
