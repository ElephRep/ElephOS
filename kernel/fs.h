#ifndef FS_H
#define FS_H

#include <stddef.h>

#define FS_MAX_FILES 16
#define FS_NAME_MAX  24
#define FS_DATA_MAX  192

void fs_init(void);
int fs_create(const char* name);
int fs_write(const char* name, const char* data);
const char* fs_read(const char* name);
int fs_delete(const char* name);
int fs_list_name(int index, const char** out_name);
int fs_file_count(void);

#endif
