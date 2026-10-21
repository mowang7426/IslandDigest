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
static inline size_t IDExperienceColumns(double width, int grid) {
    if (!grid) return 1;
    return width >= 500 ? 3 : width >= 280 ? 2 : 1;
}
static inline size_t IDExperienceRows(size_t entries, size_t columns) {
    return entries ? (entries + columns - 1) / columns : 1;
}
static inline size_t IDExperienceVisibleRows(size_t rows, double availableHeight, double rowHeight) {
    size_t result = 1;
    for (size_t candidate = 2; candidate <= 3; candidate++) {
        if (candidate <= rows && 60 + candidate * rowHeight + (candidate - 1) * 8 <= availableHeight * 0.45) result = candidate;
    }
    return result;
}
