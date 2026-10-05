#include "event_list.h"

namespace nano_edr {

EventList::~EventList() {
    ListClear(this);
}

void ListPopFront(EventList* list) {
    if(!(list->head)) {
        return;
    }
    EventNode* old = list->head;
    list->head = old->next;
    delete old;
    --list->size;
    if (list->size == 0) {
        list->tail = nullptr;
    }
}


void ListPushBack(EventList* list, const Event* event) {
    if (list->capacity != 0 && list->size >= list->capacity) {
        ListPopFront(list);
    }
    EventNode* node = new EventNode{*event, nullptr};
    if (list->tail) {
        list->tail->next = node;
    } else {
        list->head = node;
    }
    list->tail = node;
    ++list->size;
}


void ListClear(EventList* list) {
    EventNode* it = list->head;
    while (it) {
        EventNode* next = it->next;
        delete it;
        it = next;
    }
    list->head = nullptr;
    list->tail = nullptr;
    list->size = 0;
}

}  // namespace nano_edr
