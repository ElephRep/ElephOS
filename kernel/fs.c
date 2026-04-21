#include "fs.h"
#include "common.h"

struct fs_file {
    int used;
    char name[FS_NAME_MAX];
    char data[FS_DATA_MAX];
};

// Статическая инициализация нулями
static struct fs_file files[FS_MAX_FILES] = {0};

static size_t str_len(const char* s) {
    size_t n = 0;
    while (s[n]) n++;
    return n;
}

static void str_copy(char* dst, const char* src, size_t max) {
    size_t i = 0;
    if (max == 0) return;
    while (src[i] && i < max - 1) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

static int find_index(const char* name) {
    int i;
    for (i = 0; i < FS_MAX_FILES; i++) {
        if (files[i].used && streq(files[i].name, name)) {
            return i;
        }
    }
    return -1;
}

void fs_init(void) {
    int i;
    for (i = 0; i < FS_MAX_FILES; i++) {
        files[i].used = 0;
        files[i].name[0] = '\0';
        files[i].data[0] = '\0';
    }
}

int fs_create(const char* name) {
    int i;
    size_t len = str_len(name);
    if (len == 0 || len >= FS_NAME_MAX) return 0;
    if (find_index(name) >= 0) return 0;

    for (i = 0; i < FS_MAX_FILES; i++) {
        if (!files[i].used) {
            files[i].used = 1;
            str_copy(files[i].name, name, FS_NAME_MAX);
            files[i].data[0] = '\0';
            return 1;
        }
    }
    return 0;
}

int fs_write(const char* name, const char* data) {
    int idx = find_index(name);
    if (idx < 0) return 0;
    str_copy(files[idx].data, data, FS_DATA_MAX);
    return 1;
}

const char* fs_read(const char* name) {
    int idx = find_index(name);
    if (idx < 0) return (const char*)0;
    return files[idx].data;
}

int fs_delete(const char* name) {
    int idx = find_index(name);
    if (idx < 0) return 0;
    files[idx].used = 0;
    files[idx].name[0] = '\0';
    files[idx].data[0] = '\0';
    return 1;
}

int fs_list_name(int index, const char** out_name) {
    int seen = 0;
    int i;
    for (i = 0; i < FS_MAX_FILES; i++) {
        if (!files[i].used) continue;
        if (seen == index) {
            *out_name = files[i].name;
            return 1;
        }
        seen++;
    }
    return 0;
}

int fs_file_count(void) {
    int i;
    int count = 0;
    for (i = 0; i < FS_MAX_FILES; i++) {
        if (files[i].used) count++;
    }
    return count;
}
