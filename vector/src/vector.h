#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>

template <typename T>
class vector {
public:
  using value_type = T;

  using reference = T&;
  using const_reference = const T&;

  using pointer = T*;
  using const_pointer = const T*;

  using iterator = pointer;
  using const_iterator = const_pointer;

public:
  vector() noexcept : _data(nullptr), _size(0), _capacity(0) {}

  vector(const vector& other) : vector(other, other._size) {} // O(N) strong

  vector& operator=(const vector& other) {
    if (&other != this) {
      vector(other).swap(*this);
    }
    return *this;
  } // O(N) strong

  ~vector() noexcept {
    clear();
    operator delete(_data);
  } // O(N) nothrow

  reference operator[](size_t index) {
    assert(index < size());
    return data()[index];
  } // O(1) nothrow

  const_reference operator[](size_t index) const {
    assert(index < size());
    return data()[index];
  } // O(1) nothrow

  pointer data() noexcept {
    return _data;
  } // O(1) nothrow

  const_pointer data() const noexcept {
    return _data;
  } // O(1) nothrow

  size_t size() const noexcept {
    return _size;
  } // O(1) nothrow

  reference front() {
    assert(!empty());
    return *begin();
  } // O(1) nothrow

  const_reference front() const {
    assert(!empty());
    return *begin();
  } // O(1) nothrow

  reference back() {
    assert(!empty());
    return *(end() - 1);
  } // O(1) nothrow

  const_reference back() const {
    assert(!empty());
    return *(end() - 1);
  } // O(1) nothrow

  void push_back(const_reference value) {
    if (size() == capacity()) {
      vector tmp(*this, capacity() == 0 ? 1 : capacity() * 2);
      tmp.push_back(value);
      swap(tmp);
    } else {
      new (data() + size()) value_type(value);
      ++_size;
    }
  } // O(1)* strong

  void pop_back() {
    assert(size() != 0);
    data()[--_size].~value_type();
  } // O(1) nothrow

  bool empty() const noexcept {
    return size() == 0;
  } // O(1) nothrow

  size_t capacity() const noexcept {
    return _capacity;
  } // O(1) nothrow

  void reserve(size_t new_capacity) {
    if (new_capacity > capacity()) {
      vector(*this, new_capacity).swap(*this);
    }
  } // O(N) strong

  void shrink_to_fit() {
    if (size() != capacity()) {
      vector(*this, size()).swap(*this);
    }
  } // O(N) strong

  void clear() noexcept {
    while (!empty()) {
      pop_back();
    }
  } // O(N) nothrow

  void swap(vector& other) noexcept {
    std::swap(_data, other._data);
    std::swap(_size, other._size);
    std::swap(_capacity, other._capacity);
  } // O(1) nothrow

  iterator begin() noexcept {
    return data();
  } // O(1) nothrow

  iterator end() noexcept {
    return begin() + size();
  } // O(1) nothrow

  const_iterator begin() const noexcept {
    return data();
  } // O(1) nothrow

  const_iterator end() const noexcept {
    return begin() + size();
  } // O(1) nothrow

  iterator insert(const_iterator pos, const_reference value) {
    ptrdiff_t diff = pos - begin();
    push_back(value);
    iterator back_iterator = end() - 1;
    for (iterator it = begin() + diff; it != back_iterator; it++) {
      std::iter_swap(it, back_iterator);
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
    while (non_constant_right != end()) {
      std::iter_swap(non_constant_left++, non_constant_right++);
    }
    for (ptrdiff_t i = 0; i < last - first; i++) {
      pop_back();
    }
    return begin() + first_pos_index;
  } // O(N) nothrow(swap)

private:
  vector(const vector& other, size_t capacity)
      : _data(capacity == 0 ? nullptr : static_cast<pointer>(operator new(sizeof(value_type) * capacity))),
        _size(other.size()),
        _capacity(capacity) {
    assert(capacity >= other.size());

    for (size_t i = 0; i < size(); ++i) {
      try {
        new (data() + i) value_type(other[i]);
      } catch (...) {
        _size = i;
        clear();
        operator delete(data());
        throw;
      }
    }
  }

private:
  pointer _data;
  size_t _size;
  size_t _capacity;
};
