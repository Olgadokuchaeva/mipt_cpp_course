#include "conditions.h"

#include "fields.h"
#include "text.h"

namespace nano_edr {

bool FieldEquals::Matches(const Event& event) const {
    const std::string* value = FindField(event, key_);
    if (value == nullptr) {
        return false;
    }
    return EqualsIgnoreCase(*value, value_);
}


bool FieldContains::Matches(const Event& event) const {
    const std::string* value = FindField(event, key_);
    if (value == nullptr) {
        return false;
    }
    return ContainsIgnoreCase(*value, needle_);
}


bool FieldEndsWith::Matches(const Event& event) const {
    const std::string* value = FindField(event, key_);
    if (value == nullptr) {
        return false;
    }
    return EndsWithIgnoreCase(*value, suffix_);
}


bool FieldInList::Matches(const Event& event) const {
    const std::string* value = FindField(event, key_);
    if (value == nullptr) {
        return false;
    }
    for (const std::string& candidate : values_) {
        if (EqualsIgnoreCase(*value, candidate)) {
            return true;
        }
    }
    return false;
}


bool EventTypeIs::Matches(const Event& event) const {
    for (EventType t : types_) {
        if (event.event_type() == t) {
            return true;
        }
    }
    return false;
}


void AllOf::Add(std::unique_ptr<ICondition> condition) {
    parts_.push_back(std::move(condition));
}


bool AllOf::Matches(const Event& event) const {
    for (const auto& part : parts_) {
        if (!part->Matches(event)) {
            return false;
        }
    }
    return true;
}


void AnyOf::Add(std::unique_ptr<ICondition> condition) {
    parts_.push_back(std::move(condition));
}


bool AnyOf::Matches(const Event& event) const {
    for (const auto& part : parts_) {
        if (part->Matches(event)) {
            return true;
        }
    }
    return false;
}

}  // namespace nano_edr