#pragma once

#include <QIcon>
#include <QColor>

// Icons for the dock. We reuse OBS Studio's own theme SVGs (tinted to the given
// colour) so the UI is indistinguishable from native OBS controls, and fall
// back to minimal hand-drawn glyphs if those assets can't be located.
namespace obstree {

QIcon folderIcon(const QColor &color); // OBS "group" glyph
QIcon sceneIcon(const QColor &color);  // OBS "scene" glyph (list lines)
QIcon addIcon(const QColor &color);    // OBS "plus"
QIcon removeIcon(const QColor &color); // OBS "trash"
QIcon upIcon(const QColor &color);     // OBS "up" chevron
QIcon downIcon(const QColor &color);   // OBS "down" chevron
QIcon filtersIcon(const QColor &color); // OBS "filter" funnel glyph

} // namespace obstree
