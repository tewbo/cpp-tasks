#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <memory>
#include <utility>

template <typename T, size_t SMALL_SIZE>
class socow_vector {
public:
  using value_type = T;

  using reference = T&;
  using const_reference = const T&;

  using pointer = T*;
  using const_pointer = const T*;

  using iterator = pointer;
  using const_iterator = const_pointer;

private:
  struct buffer {
    size_t _capacity;
    size_t _refs{1};
    value_type _data[0]{};

    explicit buffer(size_t capacity) : _capacity(capacity) {}
  };

public:
  socow_vector() noexcept {}

  socow_vector(const socow_vector& other) : _is_small(other._is_small), _size(other._size) {
    if (other._is_small) {
      std::uninitialized_copy(other.static_data, other.static_data + other.size(), static_data);
    } else {
      dynamic_data = other.dynamic_data;
      dynamic_data->_refs++;
    }
  }

  socow_vector& operator=(const socow_vector& other) {
    if (&other != this) {
      if (!_is_small && !other._is_small) {
        socow_vector(other).swap(*this);
      } else if (_is_small && other._is_small) {
        assign_small_to_small(other);
      } else if (!_is_small && other._is_small) {
        socow_vector tmp = *this;
        try {
          std::uninitialized_copy(other.static_data, other.static_data + other.size(), static_data);
        } catch (...) {
          dynamic_data = tmp.dynamic_data;
          throw;
        }
        tmp.dynamic_data->_refs--;
        _is_small = true;
      } else {
        clear();
        dynamic_data = other.dynamic_data;
        dynamic_data->_refs++;
        _is_small = false;
      }
      _size = other._size;
    }

    return *this;
  }

  ~socow_vector() noexcept {
    if (!_is_small) {
      decrease_refs();
    } else {
      clear();
    }
  }

  reference operator[](size_t index) {
    assert(index < size());
    return (data())[index];
  }

  const_reference operator[](size_t index) const {
    assert(index < size());
    return (data())[index];
  }

  const_pointer data() const noexcept {
    return _is_small ? static_data : dynamic_data->_data;
  }

  pointer data() {
    unshare();
    return _is_small ? static_data : dynamic_data->_data;
  }

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
    if (size() == capacity() || is_shared()) {
      buffer* dynamic_buf = push_back_with_copy(const_data(), value);
      if (_is_small) {
        std::destroy(static_data, static_data + size());
        _is_small = false;
      } else {
        decrease_refs();
      }
      dynamic_data = dynamic_buf;
    } else {
      new (data() + size()) value_type(value);
    }
    _size++;
  }

  void pop_back() {
    assert(size() != 0);
    if (is_shared()) {
      transform_pop_back();
    } else {
      data()[--_size].~value_type();
    }
  }

  bool empty() const noexcept {
    return !size();
  } // O(1) nothrow

  size_t capacity() const noexcept {
    return _is_small ? SMALL_SIZE : dynamic_data->_capacity;
  }

  void reserve(size_t new_capacity) {
    if (new_capacity < size()) {
      return;
    }
    if (new_capacity > SMALL_SIZE && (new_capacity > capacity() || _is_small || is_shared())) {
      buffer* new_buffer = create_buf(new_capacity);
      data_copy_to_buf(const_data(), new_buffer, size());
      if (!_is_small) {
        decrease_refs();
      } else {
        std::destroy(static_data, static_data + size());
      }
      dynamic_data = new_buffer;
      _is_small = false;
    } else if (new_capacity <= SMALL_SIZE && !_is_small) {
      shrink_to_small();
    }
  }

  void shrink_to_fit() {
    if (size() != capacity() && !_is_small) {
      if (size() > SMALL_SIZE) {
        buffer* new_buffer = create_buf(size());
        data_copy_to_buf(dynamic_data->_data, new_buffer, size());
        decrease_refs();
        dynamic_data = new_buffer;
      } else {
        shrink_to_small();
      }
    }
  }

  void clear() noexcept {
    if (_is_small || dynamic_data->_refs == 1) {
      while (!empty()) {
        pop_back();
      }
    } else {
      dynamic_data->_refs--;
      _size = 0;
      _is_small = true;
    }
  } // O(N) nothrow

  void swap(socow_vector& other) {
    if (this != &other) {
      if (!_is_small && !other._is_small) {
        std::swap(dynamic_data, other.dynamic_data);
        std::swap(_size, other._size);
      } else if (_is_small && other._is_small) {
        swap_small(other);
      } else if (_is_small) {
        swap_small_to_big(*this, other);
      } else {
        swap_small_to_big(other, *this);
      }
    }
  }

  iterator begin() {
    return data();
  } // O(1) nothrow

  iterator end() {
    return begin() + size();
  }

  const_iterator begin() const noexcept {
    return data();
  } // O(1) nothrow

  const_iterator end() const noexcept {
    return begin() + size();
  } // O(1) nothrow

  iterator insert(const_iterator pos, const_reference value) {
    ptrdiff_t diff = pos - const_data();
    if (size() == capacity() || is_shared()) {
      buffer* new_buffer = insert_with_copy(diff, value);
      if (_is_small) {
        std::destroy(static_data, static_data + size());
        _is_small = false;
      } else {
        decrease_refs();
      }
      dynamic_data = new_buffer;
      _size++;
    } else {
      push_back(value);
      for (iterator it = end() - 1; it != begin() + diff; it--) {
        std::iter_swap(it, it - 1);
      }
    }
    return begin() + diff;
  }

  iterator erase(const_iterator pos) {
    return erase(pos, pos + 1);
  }

  iterator erase(const_iterator first, const_iterator last) {
    ptrdiff_t first_pos_index = first - const_data();
    ptrdiff_t last_pos_index = last - const_data();
    if (is_shared()) {
      buffer* new_buffer = create_buf(capacity());
      data_copy_to_buf(dynamic_data->_data, new_buffer, first_pos_index);
      data_copy_to_buf(dynamic_data->_data + last_pos_index, new_buffer, (const_data() + size()) - last,
                       first_pos_index);
      dynamic_data->_refs--;
      _size = size() - (last - first);
      dynamic_data = new_buffer;
    } else {
      iterator non_constant_left = begin() + first_pos_index;
      iterator non_constant_right = begin() + last_pos_index;
      if (first != last) {
        while (non_constant_right != end()) {
          std::iter_swap(non_constant_left++, non_constant_right++);
        }
        for (ptrdiff_t i = 0; i < last - first; i++) {
          pop_back();
        }
      }
    }
    return begin() + first_pos_index;
  }

  friend void swap(socow_vector& lhs, socow_vector& rhs) {
    lhs.swap(rhs);
  }

private:
  void unshare() {
    if (is_shared()) {
      reserve(capacity());
    }
  }

  buffer* push_back_with_copy(pointer source, const_reference value) {
    size_t new_capacity = (size() == capacity() ? capacity() * 2 : capacity());
    buffer* dynamic_buf = create_buf(new_capacity);
    data_copy_to_buf(source, dynamic_buf, size());
    data_copy_to_buf(&value, dynamic_buf, 1, size());
    return dynamic_buf;
  }

  void transform_pop_back() {
    buffer* new_buffer = create_buf(size());
    data_copy_to_buf(dynamic_data->_data, new_buffer, size() - 1);
    dynamic_data->_refs--;
    --_size;
    dynamic_data = new_buffer;
  }

  buffer* insert_with_copy(size_t pos, const_reference value) {
    size_t new_capacity = (size() == capacity() ? capacity() * 2 : capacity());
    buffer* new_buffer = create_buf(new_capacity);
    data_copy_to_buf(const_data(), new_buffer, pos);
    data_copy_to_buf(&value, new_buffer, 1, pos);
    data_copy_to_buf(const_data() + pos, new_buffer, size() - pos, pos + 1);
    return new_buffer;
  }

  static void data_copy_to_buf(const_pointer from, buffer* to, size_t cnt, size_t shift) {
    try {
      std::uninitialized_copy(from, from + cnt, to->_data + shift);
    } catch (...) {
      std::destroy(to->_data, to->_data + shift);
      operator delete(to, buffer_size_bytes(to->_capacity));
      throw;
    }
  }

  static void data_copy_to_buf(const_pointer from, buffer* to, size_t cnt) {
    data_copy_to_buf(from, to, cnt, 0);
  }

  static buffer* create_buf(size_t capacity) {
    auto* new_dynamic_buffer = static_cast<buffer*>(operator new(buffer_size_bytes(capacity)));
    try {
      new (new_dynamic_buffer) buffer(capacity);
    } catch (...) {
      operator delete(new_dynamic_buffer, buffer_size_bytes(new_dynamic_buffer->_capacity));
      throw;
    }
    return new_dynamic_buffer;
  }

  pointer const_data() noexcept {
    return _is_small ? static_data : dynamic_data->_data;
  }

  void assign_small_to_small(const socow_vector& other) {
    socow_vector tmp;
    for (size_t i = 0; i < std::min(size(), other.size()); i++) {
      tmp.push_back(other.static_data[i]);
    }
    size_t size_save = size();
    if (size() < other.size()) {
      std::uninitialized_copy(other.static_data + size(), other.static_data + other.size(), static_data + size());
      _size = other.size();
    }
    for (size_t i = 0; i < std::min(size_save, other.size()); i++) {
      std::swap(static_data[i], tmp.static_data[i]);
    }
    if (size() > other.size()) {
      std::destroy(static_data + other.size(), static_data + size());
    }
  }

  void swap_small(socow_vector& other) {
    if (size() > other.size()) {
      std::uninitialized_copy(static_data + other.size(), static_data + size(), other.static_data + other.size());
      std::destroy(static_data + other.size(), static_data + size());
    } else if (size() < other.size()) {
      other.swap_small(*this);
      return;
    }
    std::swap(_size, other._size);
    for (size_t i = 0; i < std::min(size(), other.size()); i++) {
      std::swap(static_data[i], other.static_data[i]);
    }
  }

  static void swap_small_to_big(socow_vector& lhs, socow_vector& rhs) {
    buffer* dynamic_tmp = rhs.dynamic_data;
    try {
      std::uninitialized_copy(lhs.static_data, lhs.static_data + lhs.size(), rhs.static_data);
    } catch (...) {
      rhs.dynamic_data = dynamic_tmp;
      throw;
    }
    std::destroy(lhs.static_data, lhs.static_data + lhs.size());
    lhs.dynamic_data = dynamic_tmp;
    std::swap(lhs._is_small, rhs._is_small);
    std::swap(lhs._size, rhs._size);
  }

  void decrease_refs() noexcept {
    dynamic_data->_refs--;
    if (dynamic_data->_refs == 0) {
      std::destroy(dynamic_data->_data, dynamic_data->_data + size());
      operator delete(dynamic_data, buffer_size_bytes(dynamic_data->_capacity));
    }
  }

  bool is_shared() {
    return !_is_small && dynamic_data->_refs > 1;
  }

  void shrink_to_small() {
    socow_vector tmp_socow = *this;
    try {
      std::uninitialized_copy(dynamic_data->_data, dynamic_data->_data + size(), static_data);
    } catch (...) {
      dynamic_data = tmp_socow.dynamic_data;
      throw;
    }
    tmp_socow.dynamic_data->_refs--;
    _is_small = true;
  }

  static size_t buffer_size_bytes(size_t capacity) {
    return sizeof(buffer) + sizeof(value_type) * capacity;
  }

private:
  bool _is_small{true};
  size_t _size{0};

  union {
    buffer* dynamic_data;
    value_type static_data[SMALL_SIZE];
  };
};
