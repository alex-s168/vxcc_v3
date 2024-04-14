#ifndef UTILS_H
#define UTILS_H

#include <string.h>
#include <stdbool.h>

static bool strstarts(const char *str, const char *prefix) {
    return strncmp(str, prefix, strlen(prefix)) == 0;
}

#endif //UTILS_H
