
#include <stdbool.h>

typedef struct list_entry {
    struct list_entry* next;
    struct list_entry* prev;
} list_entry_t;

typedef list_entry_t list_t;

#define INIT_LIST(var) (list_t){ &var, &var }

#define LIST_ENTRY(ptr, type, member) ((type*)((char*)(ptr) - (char*)offsetof(type, member)))

#define LIST_FIRST_ENTRY(ptr, type, member) LIST_ENTRY((ptr)->next, type, member)
#define LIST_LAST_ENTRY(ptr, type, member) LIST_ENTRY((ptr)->prev, type, member)
#define LIST_NEXT_ENTRY(pos, member) LIST_ENTRY((pos)->member.next, __typeof__(*(pos)), member)
#define LIST_PREV_ENTRY(pos, member) LIST_ENTRY((pos)->member.prev, __typeof__(*(pos)), member)

#define LIST_ENTRY_IS_HEAD(pos, head, member) (&pos->member == (head))

#define LIST_FOR_EACH_ENTRY(pos, head, member) \
	for (pos = LIST_FIRST_ENTRY(head, __typeof__(*pos), member); !LIST_ENTRY_IS_HEAD(pos, head, member); pos = LIST_NEXT_ENTRY(pos, member))

void list_add(list_t* head, list_entry_t* new);
void list_add_tail(list_t* head, list_entry_t* new);
void list_del(list_entry_t* entry);
bool list_is_empty(list_t* head);
list_entry_t* list_pop(list_t* head);
