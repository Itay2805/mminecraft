#include <stddef.h>
#include "list.h"

static void __list_add(list_entry_t* new, list_entry_t* prev, list_entry_t* next) {
    next->prev = new;
    new->next = next;
    new->prev = prev;
    prev->next = new;
}

void list_add(list_t* head, list_entry_t* new) {
    __list_add(new, head, head->next);
}

void list_add_tail(list_t* head, list_entry_t* new) {
    __list_add(new, head->prev, head);
}

void list_del(list_entry_t* entry) {
    list_entry_t* prev = entry->prev;
    list_entry_t* next = entry->next;
    next->prev = prev;
    prev->next = next;
    entry->next = (list_entry_t*)0xAAAAAAAAAAAAAAAA;
    entry->prev = (list_entry_t*)0xBBBBBBBBBBBBBBBB;
}

bool list_is_empty(list_t* head) {
    return head->next == head;
}

list_entry_t* list_pop(list_t* head) {
    // check if we even have any
    if (list_is_empty(head)) {
        return NULL;
    }

    // take the last one and remove it
    list_entry_t* entry = head->prev;
    list_del(entry);
    return entry;
}
