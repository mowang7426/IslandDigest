// Pure deterministic ordering and bounded sizing shared by UIKit and portable tests.
#pragma once
#include <stddef.h>
static inline int IDExperienceOrder(int firstPinned, int secondPinned, int mode,
    size_t firstCount, size_t secondCount, double firstTime, double secondTime,
    int localizedNameOrder, int identifierOrder) {
    if (firstPinned != secondPinned) return firstPinned ? -1 : 1;
    if (mode == 1 && firstCount != secondCount) return firstCount > secondCount ? -1 : 1;
    if (mode == 2 && localizedNameOrder) return localizedNameOrder < 0 ? -1 : 1;
    if (mode == 0 && firstTime != secondTime) return firstTime > secondTime ? -1 : 1;
    return identifierOrder < 0 ? -1 : identifierOrder > 0 ? 1 : 0;
}
static inline size_t IDExperienceGridColumns(double width, int grid, int preference, double textScale) {
    if (!grid) return 1;
    if (!(width > 0) || !(textScale > 0)) return 1;
    // Auto is compact by default, while explicit legacy two/three-column choices
    // stay authoritative. Large text reduces columns before truncation becomes unusable.
    double minimum = textScale > 1.9 ? 210 : textScale > 1.35 ? 128 : 82;
    size_t requested = preference == 2 ? 2 : preference == 3 ? 3 : (width >= 650 ? 5 : 4);
    for (size_t columns = requested; columns > 1; columns--) {
        if ((width - 16 - (columns - 1) * 6) / columns >= minimum) return columns;
    }
    return 1;
}
static inline size_t IDExperienceColumns(double width, int grid) {
    return IDExperienceGridColumns(width, grid, 3, 1);
}
static inline size_t IDExperienceRows(size_t entries, size_t columns) {
    return entries ? (entries + columns - 1) / columns : 1;
}
// Mini paints a smaller tile inside the unchanged accessible button bounds.
// Shared by production constraints and portable geometry regression tests.
typedef struct {
    double hitHeight, insetX, insetY, visualHeight, contentPadding;
} IDCategoryGeometry;
static inline IDCategoryGeometry IDExperienceCategoryGeometry(double rowHeight, int mini) {
    IDCategoryGeometry result;
    result.hitHeight = rowHeight > 44 ? rowHeight : 44;
    result.insetX = mini ? 4 : 0;
    result.insetY = mini ? 6 : 0;
    result.visualHeight = result.hitHeight - 2 * result.insetY;
    result.contentPadding = 5;
    return result;
}
static inline size_t IDExperienceVisibleRows(size_t rows, double availableHeight, double rowHeight) {
    size_t result = 1;
    for (size_t candidate = 2; candidate <= 3; candidate++) {
        if (candidate <= rows && 66 + candidate * rowHeight + (candidate - 1) * 4 <= availableHeight * 0.45) result = candidate;
    }
    return result;
}
