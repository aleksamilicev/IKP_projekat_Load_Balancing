#ifndef DICTIONARY_H
#define DICTIONARY_H

#include <stddef.h>
#include <stdbool.h>

// Dictionary node structure
typedef struct DictNode {
    void* key;
    void* value;
    struct DictNode* next;
} DictNode;

// Dictionary structure
typedef struct Dictionary {
    DictNode* head;
    size_t key_size;
    size_t value_size;
    size_t size;
} Dictionary;

// Function prototypes
void initialize_dictionary(Dictionary* dict, size_t key_size, size_t value_size);
void add_entry(Dictionary* dict, void* key, void* value);
bool get_value(const Dictionary* dict, void* key, void* value, int (*cmp_func)(void*, void*));
void remove_entry(Dictionary* dict, void* key, int (*cmp_func)(void*, void*));
void display_dictionary(const Dictionary* dict, void (*print_func)(void*, void*));
bool is_empty(const Dictionary* dict);
size_t get_dictionary_size(const Dictionary* dict);
void clear_dictionary(Dictionary* dict);

#endif // DICTIONARY_H
