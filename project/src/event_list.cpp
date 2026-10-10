#include "event_list.h"

namespace nano_edr {

EventList::~EventList() {
    Clear();
}

void EventList::PopFront() {
    if(!head_) {
        return;
    }
    EventNode* old = head_;
    head_ = old->next;
    delete old;
    --size_;
    if (size_ == 0) {
        tail_ = nullptr;
    }
}


void EventList::PushBack(const Event& event) {
    if (capacity_ != 0 && size_ >= capacity_) {
        PopFront();
    }
    EventNode* node = new EventNode(event);
    if (tail_) {
        tail_->next = node;
    } else {
        head_ = node;
    }
    tail_ = node;
    ++size_;
}


void EventList::Clear() {
    EventNode* it = head_;
    while (it) {
        EventNode* next = it->next;
        delete it;
        it = next;
    }
    head_ = nullptr;
    tail_ = nullptr;
    size_ = 0;
}

}  // namespace nano_edr