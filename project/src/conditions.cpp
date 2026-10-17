#include "conditions.h"

#include "fields.h"
#include "text.h"

namespace nano_edr {

bool FieldEquals::operator()(const Event& event) const {
    const std::string* value = FindField(event, key_);
    if (value == nullptr) {
        return false;
    }
    return EqualsIgnoreCase(*value, value_);
}


bool FieldContains::operator()(const Event& event) const {
    const std::string* value = FindField(event, key_);
    if (value == nullptr) {
        return false;
    }
    return ContainsIgnoreCase(*value, needle_);
}


bool FieldEndsWith::operator()(const Event& event) const {
    const std::string* value = FindField(event, key_);
    if (value == nullptr) {
        return false;
    }
    return EndsWithIgnoreCase(*value, suffix_);
}


bool FieldInList::operator()(const Event& event) const {
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


Condition Condition::Equals(const std::string& key, const std::vector<std::string>& values) {
    return Condition(Match::kEquals, key, values);
}


Condition Condition::Contains(const std::string& key, const std::vector<std::string>& values) {
    return Condition(Match::kContains, key, values);
}


Condition Condition::EndsWith(const std::string& key, const std::vector<std::string>& values) {
    return Condition(Match::kEndsWith, key, values);
}


bool Condition::operator()(const Event& event) const {
    const std::string* value = FindField(event, key_);
    if (value == nullptr) {
        return false;
    }
    for (const std::string& candidate : values_) {
        switch (match_) {
            case Match::kEquals:
                if (EqualsIgnoreCase(*value, candidate)) {
                    return true;
                }
                break;
            case Match::kContains:
                if (ContainsIgnoreCase(*value, candidate)) {
                    return true;
                }
                break;
            case Match::kEndsWith:
                if (EndsWithIgnoreCase(*value, candidate)) {
                    return true;
                }
                break;
        }
    }
    return false;
}

}  // namespace nano_edr