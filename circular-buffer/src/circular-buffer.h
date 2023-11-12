#pragma once

#include <algorithm>
#include <cstdlib>
#include <iterator>
#include <utility>

template <typename T>
class circular_buffer {
  template <typename U>
  struct buffer_iterator {
  public:
    using value_type = T;
    using reference = U&;
    using pointer = U*;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::random_access_iterator_tag;

    buffer_iterator() = default;

    operator buffer_iterator<const U>() const {
      return buffer_iterator<const U>{pointer_data, shift_, module_};
    }

    buffer_iterator& operator++() {
      ++shift_;
      return *this;
    }

    buffer_iterator operator++(int) {
      buffer_iterator tmp = *this;
      ++*this;
      return tmp;
    }

    buffer_iterator& operator--() {
      --shift_;
      return *this;
    }

    buffer_iterator operator--(int) {
      buffer_iterator tmp = *this;
      --*this;
      return tmp;
    }

    buffer_iterator& operator+=(difference_type dif) {
      shift_ += dif;
      return *this;
    }

    buffer_iterator& operator-=(difference_type dif) {
      shift_ -= dif;
      return *this;
    }

    buffer_iterator operator+(difference_type dif) const {
      buffer_iterator tmp = *this;
      tmp += dif;
      return tmp;
    }

    buffer_iterator operator-(difference_type dif) const {
      buffer_iterator tmp = *this;
      tmp -= dif;
      return tmp;
    }

    friend buffer_iterator operator+(difference_type dif, buffer_iterator it) {
      return it + dif;
    }

    friend buffer_iterator operator-(difference_type dif, buffer_iterator it) {
      return it - dif;
    }

    friend difference_type operator-(const buffer_iterator& left, const buffer_iterator& right) {
      return left.shift_ - right.shift_;
    }

    reference operator*() const {
      return pointer_data[shift_ % module_];
    }

    pointer operator->() const {
      return pointer_data + shift_ % module_;
    }

    reference operator[](difference_type dif) const {
      return *(*this + dif);
    }

    bool operator==(const buffer_iterator& other) const {
      return pointer_data == other.pointer_data && shift_ == other.shift_ && module_ == other.module_;
    }

    bool operator!=(const buffer_iterator& other) const {
      return !(*this == other);
    }

    bool operator>(const buffer_iterator& other) const {
      return shift_ > other.shift_;
    }

    bool operator<(const buffer_iterator& other) const {
      return shift_ < other.shift_;
    }

    bool operator<=(const buffer_iterator& other) const {
      return shift_ <= other.shift_;
    }

    bool operator>=(const buffer_iterator& other) const {
      return shift_ >= other.shift_;
    }

  private:
    explicit buffer_iterator(pointer data, size_t index, size_t modulus)
        : pointer_data(data),
          shift_(index),
          module_(modulus) {}

  private:
    friend circular_buffer;
    pointer pointer_data;
    size_t shift_;
    size_t module_;
  };

public:
  using value_type = T;

  using reference = T&;
  using const_reference = const T&;

  using pointer = T*;
  using const_pointer = const T*;

  using iterator = buffer_iterator<T>;
  using const_iterator = buffer_iterator<const T>;

  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

public:
  // O(1), nothrow
  circular_buffer() noexcept : data_(nullptr), size_(0), capacity_(0), front_(0) {}

  // O(n), strong
  circular_buffer(const circular_buffer& other) : circular_buffer(other, other.capacity()) {}

  // O(n), strong
  circular_buffer& operator=(const circular_buffer& other) {
    if (this != &other) {
      circular_buffer tmp(other);
      swap(*this, tmp);
    }
    return *this;
  }

  // O(n), nothrow
  ~circular_buffer() {
    clear();
    operator delete(data_);
  }

  // O(1), nothrow
  size_t size() const noexcept {
    return size_;
  }

  // O(1), nothrow
  bool empty() const noexcept {
    return !size();
  }

  // O(1), nothrow
  size_t capacity() const noexcept {
    return capacity_;
  }

  // O(1), nothrow
  iterator begin() noexcept {
    return iterator(data_, front_, capacity());
  }

  // O(1), nothrow
  const_iterator begin() const noexcept {
    return const_iterator(data_, front_, capacity());
  }

  // O(1), nothrow
  iterator end() noexcept {
    return iterator(data_, front_ + size(), capacity());
  }

  // O(1), nothrow
  const_iterator end() const noexcept {
    return const_iterator(data_, front_ + size(), capacity());
  }

  // O(1), nothrow
  reverse_iterator rbegin() noexcept {
    return std::make_reverse_iterator(end());
  }

  // O(1), nothrow
  const_reverse_iterator rbegin() const noexcept {
    return std::make_reverse_iterator(end());
  }

  // O(1), nothrow
  reverse_iterator rend() noexcept {
    return std::make_reverse_iterator(begin());
  }

  // O(1), nothrow
  const_reverse_iterator rend() const noexcept {
    return std::make_reverse_iterator(begin());
  }

  // O(1), nothrow
  T& operator[](size_t index) {
    return data_[(front_ + index) % capacity_];
  }

  // O(1), nothrow
  const T& operator[](size_t index) const {
    return data_[(front_ + index) % capacity_];
  }

  // O(1), nothrow
  T& back() {
    return *--end();
  }

  // O(1), nothrow
  const T& back() const {
    return *--end();
  }

  // O(1), nothrow
  T& front() {
    return *begin();
  }

  // O(1), nothrow
  const T& front() const {
    return *begin();
  }

  // O(1), strong
  void push_back(const T& val) {
    if (size() == capacity()) {
      circular_buffer new_buf(*this, new_capacity());
      swap(*this, new_buf);
      push_back(val);
    } else {
      new (data_ + (front_ + size()) % capacity()) value_type(val);
      ++size_;
    }
  }

  // O(1), strong
  void push_front(const T& val) {
    if (size() == capacity()) {
      circular_buffer new_buf(*this, new_capacity());
      swap(*this, new_buf);
      push_front(val);
    } else {
      new (data_ + decrease_front()) value_type(val);
      ++size_;
      front_ = decrease_front();
    }
  }

  // O(1), nothrow
  void pop_back() {
    data_[(front_ + size() - 1) % capacity()].~value_type();
    --size_;
  }

  // O(1), nothrow
  void pop_front() {
    data_[front_].~value_type();
    --size_;
    front_ = (front_ + 1) % capacity();
  }

  void reserve(size_t new_capacity) {
    if (new_capacity > capacity()) {
      circular_buffer tmp(*this, new_capacity);
      swap(*this, tmp);
    }
  } // O(N) strong

  iterator insert(const_iterator pos, const_reference value) {
    ptrdiff_t diff = pos - begin();
    if (diff > size() / 2) {
      push_back(value);
      iterator back_iterator = end() - 1;
      for (iterator it = begin() + diff; it != back_iterator; it++) {
        std::iter_swap(it, back_iterator);
      }
    } else {
      push_front(value);
      for (iterator it = begin() + diff; it != begin(); it--) {
        std::iter_swap(it, begin());
      }
    }
    return begin() + diff;
  } // O(N) strong

  iterator erase(const_iterator pos) {
    return erase(pos, pos + 1);
  } // O(N) nothrow(swap)

  iterator erase(const_iterator first, const_iterator last) {
    ptrdiff_t first_pos_index = first - begin();
    ptrdiff_t last_pos_index = last - begin();
    iterator non_constant_left = begin() + first_pos_index;
    iterator non_constant_right = begin() + last_pos_index;
    if (end() - last < first - begin()) {
      while (non_constant_right != end()) {
        std::iter_swap(non_constant_left++, non_constant_right++);
      }
      for (ptrdiff_t i = 0; i < last - first; i++) {
        pop_back();
      }
    } else {
      while (non_constant_left != begin()) {
        std::iter_swap(--non_constant_left, --non_constant_right);
      }
      for (ptrdiff_t i = 0; i < last - first; i++) {
        pop_front();
      }
    }
    return begin() + first_pos_index;
  } // O(N) nothrow(swap)

  // O(n), nothrow
  void clear() noexcept {
    while (!empty()) {
      pop_back();
    }
  }

  friend void swap(circular_buffer& a, circular_buffer& b) noexcept {
    std::swap(a.data_, b.data_);
    std::swap(a.size_, b.size_);
    std::swap(a.capacity_, b.capacity_);
    std::swap(a.front_, b.front_);
  }

private:
  circular_buffer(const circular_buffer& other, size_t capacity)
      : data_(capacity == 0 ? nullptr : static_cast<pointer>(operator new(sizeof(value_type) * capacity))),
        size_(0),
        capacity_(capacity),
        front_(0) {
    for (size_t i = 0; i < other.size(); i++) {
      try {
        push_back(other[i]);
      } catch (...) {
        clear();
        operator delete(data_);
        throw;
      }
    }
  }

  size_t decrease_front() {
    return (front_ + capacity() - 1) % capacity();
  }

  size_t new_capacity() {
    return capacity() * 2 + 1;
  }

private:
  pointer data_;
  size_t size_;
  size_t capacity_;
  size_t front_;
};
