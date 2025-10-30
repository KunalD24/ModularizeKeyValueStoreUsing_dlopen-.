#include <stdio.h>
#include <string.h>
#include "kv_store_dynamic.h"

#define MAX_ENTRIES 100
#define MAX_KEY 256
#define MAX_VALUE 512

typedef struct 
{
    char key[MAX_KEY];
    char value[MAX_VALUE];
} key_value_entry;

static key_value_entry store[MAX_ENTRIES];
static int count = 0;

char *get_value(const char *key)
{
    for (int i = 0; i < count; i++) 
    {
        if (strcmp(store[i].key, key) == 0) 
        {
            return store[i].value;
        }
    }
    return NULL;
}

void set_value(const char *key, const char *value) 
{
    for (int i = 0; i < count; i++) 
    {
        if (strcmp(store[i].key, key) == 0) 
        {
            strncpy(store[i].value, value, sizeof(store[i].value)-1);
            store[i].value[sizeof(store[i].value)-1] = '\0';
            return;
        }
    }
    if (count < MAX_ENTRIES) 
    {
        strncpy(store[count].key, key, sizeof(store[count].key)-1);
        store[count].key[sizeof(store[count].key)-1] = '\0';
        strncpy(store[count].value, value, sizeof(store[count].value)-1);
        store[count].value[sizeof(store[count].value)-1] = '\0';
        count++;
    }
}