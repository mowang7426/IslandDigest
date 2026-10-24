#ifndef ID_ENUMERATION_ABI_H
#define ID_ENUMERATION_ABI_H
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
/* Parse complete ABI tokens, not a prefix '@' test. Runtime method slot helpers
 * do not consistently handle annotated object encodings; validate the entire
 * zero-argument signature including implicit object self and selector _cmd.
 */
static inline const char *IDEnumerationObjectEnd(const char *type) {
    if (!type) return NULL;
    for (unsigned i = 0; *type && strchr("rnNoORV", *type) && i < 8; i++) type++;
    if (*type++ != '@') return NULL;
    if (*type == '?') return NULL;
    if (*type == '"') {
        type++;
        unsigned length = 0;
        for (; *type && *type != '"' && length < 128; type++, length++) {}
        if (*type != '"' || !length) return NULL;
        type++;
    }
    return type;
}
static inline bool IDEnumerationObjectType(const char *type) {
    const char *end = IDEnumerationObjectEnd(type);
    return end && !*end;
}
static inline const char *IDEnumerationOffsetEnd(const char *type) {
    for (unsigned i = 0; *type >= '0' && *type <= '9' && i < 16; i++) type++;
    return type;
}
static inline bool IDEnumerationGetterABI(const char *encoding) {
    if (!encoding || strnlen(encoding, 512) >= 512) return false;
    const char *cursor = IDEnumerationObjectEnd(encoding);
    if (!cursor) return false;
    cursor = IDEnumerationOffsetEnd(cursor);
    cursor = IDEnumerationObjectEnd(cursor); // implicit self
    if (!cursor) return false;
    cursor = IDEnumerationOffsetEnd(cursor);
    if (*cursor++ != ':') return false; // implicit _cmd, no explicit arguments
    cursor = IDEnumerationOffsetEnd(cursor);
    return !*cursor;
}
#endif
