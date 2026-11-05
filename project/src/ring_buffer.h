#ifndef NANO_EDR_RING_BUFFER_H
#define NANO_EDR_RING_BUFFER_H

#include <cstddef>

#include "event.h"

namespace nano_edr {

template <typename T, std::size_t N>
class RingBuffer {
 public:
    class Iterator {
     public:
        Iterator(const RingBuffer* ring, std::size_t index)
            : ring_(ring), index_(index) {}

        const T& operator*() const { 
            return (*ring_)[index_]; 
        }

        Iterator& operator++() { 
            ++index_; 
            return *this; 
        
        }
        bool operator!=(const Iterator& other) const {
            return index_ != other.index_;
        }

     private:
        const RingBuffer* ring_;
        std::size_t index_;
    };

    void PushBack(const T& value) {
        if (size_ < N) {
            data_[(head_ + size_) % N] = value;
            ++size_;
        } else {
            data_[head_] = value;
            head_ = (head_ + 1) % N;
        }
    }

    void Clear() { 
        head_ = 0; 
        size_ = 0; 
    }

    std::size_t size() const { 
        return size_; 
    }

    bool empty() const { 
        return size_ == 0; 
    }

    bool full() const { 
        return size_ == N; 
    }

    static constexpr std::size_t capacity() { 
        return N; 
    }

    const T& operator[](std::size_t index) const {
        return data_[(head_ + index) % N];
    }

    const T& back() const { 
        return (*this)[size_ - 1]; 
    }

    Iterator begin() const { 
        return Iterator(this, 0); 
    }

    Iterator end() const { 
        return Iterator(this, size_); 
    }

 private:
    T data_[N]{};
    std::size_t head_ = 0;
    std::size_t size_ = 0;
};

}  // namespace nano_edr

#endif  // NANO_EDR_RING_BUFFER_H