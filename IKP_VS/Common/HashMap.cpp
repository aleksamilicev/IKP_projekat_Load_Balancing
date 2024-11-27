#include "HashMap.h"
#include <string.h>
#include <stdio.h>

int compare_worker_id(void* key1, void* key2) {
    return strcmp((char*)key1, (char*)key2);
}

void print_worker_entry(void* key, void* value) {
    printf("ID: %s, ", (char*)key);
    display_worker((Worker*)value);
}

void initialize_hash_map(HashMap* hm) {
    initialize_dictionary(hm, sizeof(char) * 10, sizeof(Worker));
}

void add_worker_to_map(HashMap* hm, const Worker* worker) {
    add_entry(hm, (void*)worker->ID, (void*)worker);
}

bool get_worker_from_map(const HashMap* hm, const char* id, Worker* worker) {
    return get_value(hm, (void*)id, (void*)worker, compare_worker_id);
}

void remove_worker_from_map(HashMap* hm, const char* id) {
    remove_entry(hm, (void*)id, compare_worker_id);
}

void display_hash_map(const HashMap* hm) {
    display_dictionary(hm, print_worker_entry);
}
