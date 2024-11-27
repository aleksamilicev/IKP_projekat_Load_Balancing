#include "Dictionary.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void initialize_dictionary(Dictionary* dict, size_t key_size, size_t value_size) {
    dict->head = NULL;
    dict->key_size = key_size;
    dict->value_size = value_size;
    dict->size = 0;
}

void add_entry(Dictionary* dict, void* key, void* value) {  // obrati paznju na ovaj deo
    DictNode* new_node = (DictNode*)malloc(sizeof(DictNode));
    new_node->key = malloc(dict->key_size);
    new_node->value = malloc(dict->value_size);
    memcpy(new_node->key, key, dict->key_size);
    memcpy(new_node->value, value, dict->value_size);

    new_node->next = dict->head;
    dict->head = new_node;
    dict->size++;
}

bool get_value(const Dictionary* dict, void* key, void* value, int (*cmp_func)(void*, void*)) {
    DictNode* current = dict->head;
    while (current) {
        if (cmp_func(key, current->key) == 0) {
            memcpy(value, current->value, dict->value_size);
            return true;
        }
        current = current->next;
    }
    return false;
}

void remove_entry(Dictionary* dict, void* key, int (*cmp_func)(void*, void*)) {
    DictNode** current = &dict->head;
    while (*current) {
        if (cmp_func(key, (*current)->key) == 0) {
            DictNode* temp = *current;
            *current = (*current)->next;
            free(temp->key);
            free(temp->value);
            free(temp);
            dict->size--;
            return;
        }
        current = &(*current)->next;
    }
}

void display_dictionary(const Dictionary* dict, void (*print_func)(void*, void*)) {
    DictNode* current = dict->head;
    while (current) {
        print_func(current->key, current->value);
        current = current->next;
    }
}

bool is_empty(const Dictionary* dict) {
    return dict->size == 0;
}

size_t get_dictionary_size(const Dictionary* dict) {
    return dict->size;
}

void clear_dictionary(Dictionary* dict) {
    while (dict->head) {
        DictNode* temp = dict->head;
        dict->head = dict->head->next;
        free(temp->key);
        free(temp->value);
        free(temp);
    }
    dict->size = 0;
}
//#include "pch.h"