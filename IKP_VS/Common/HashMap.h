#pragma once
#ifndef HASH_MAP_H
#define HASH_MAP_H

#include "Dictionary.h"
#include "../Worker/Worker.h"

typedef Dictionary HashMap;

void initialize_hash_map(HashMap* hm);
void add_worker_to_map(HashMap* hm, const Worker* worker);
bool get_worker_from_map(const HashMap* hm, const char* id, Worker* worker);
void remove_worker_from_map(HashMap* hm, const char* id);
void display_hash_map(const HashMap* hm);

#endif // HASH_MAP_H
