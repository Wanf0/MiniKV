#ifndef STORAGE_H
#define STORAGE_H

typedef struct Storage Storage;

//Create and Release software instances
Storage* storage_create();
void storage_free(Storage* storage);

//Set and Get keys
int storage_set(Storage* storage, const char* key, const char* value);
const char* storage_get(Storage* storage, const char* key);

#endif // !STORAGE_H
