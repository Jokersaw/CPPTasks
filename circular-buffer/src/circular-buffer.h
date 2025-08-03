#pragma once

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <iterator>
#include <memory>
#include <utility>

template <typename T>
class circular_buffer {

  template <class E>
  struct basic_buf_iterator {
    using value_type = T;
    using reference = E&;
    using pointer = E*;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::random_access_iterator_tag;

  private:
    circular_buffer* _buffer;
    size_t _index;
    friend circular_buffer;

    basic_buf_iterator(circular_buffer* buffer, size_t index) : _buffer(buffer), _index(index) {}

  public:
    basic_buf_iterator() = default;

    operator basic_buf_iterator<const T>() const {
      return {_buffer, _index};
    }

    basic_buf_iterator& operator++() {
      _index = (_index + 1);
      return *this;
    }

    basic_buf_iterator operator++(int) {
      basic_buf_iterator temp = *this;
      ++(*this);
      return temp;
    }

    basic_buf_iterator& operator+=(difference_type n) {
      _index = (_index + n);
      return *this;
    }

    friend basic_buf_iterator operator+(const basic_buf_iterator& left, difference_type n) {
      basic_buf_iterator temp = left;
      return (temp += n);
    }

    friend basic_buf_iterator operator+(difference_type n, const basic_buf_iterator& right) {
      basic_buf_iterator temp = right;
      return (temp += n);
    }

    basic_buf_iterator& operator-=(difference_type n) {
      return (*this += (-n));
    }

    basic_buf_iterator operator-(difference_type n) const {
      basic_buf_iterator temp = *this;
      return (temp -= n);
    }

    friend difference_type operator-(const basic_buf_iterator& left, const basic_buf_iterator& right) { //? check
      return left._index - right._index;
    }

    reference operator[](difference_type n) const {
      return *(*this + n);
    }

    friend bool operator<(const basic_buf_iterator& left, const basic_buf_iterator& right) {
      return (right - left) > 0;
    }

    friend bool operator>(const basic_buf_iterator& left, const basic_buf_iterator& right) {
      return right < left;
    }

    friend bool operator>=(const basic_buf_iterator& left, const basic_buf_iterator& right) {
      return !(left < right);
    }

    friend bool operator<=(const basic_buf_iterator& left, const basic_buf_iterator& right) {
      return !(left > right);
    }

    basic_buf_iterator& operator--() {
      _index--;
      return *this;
    }

    basic_buf_iterator operator--(int) {
      basic_buf_iterator temp = *this;
      --(*this);
      return temp;
    }

    reference operator*() const {
      return _buffer->operator[](_index);
    }

    pointer operator->() const {
      return &(_buffer->operator[](_index));
    }

    bool operator==(const basic_buf_iterator& other) const {
      return _index == other._index;
    }

    bool operator!=(const basic_buf_iterator& other) const {
      return !(*this == other);
    }
  };

public:
  using value_type = T;

  using reference = T&;
  using const_reference = const T&;

  using pointer = T*;
  using const_pointer = const T*;

  using iterator = basic_buf_iterator<T>;
  using const_iterator = basic_buf_iterator<const T>;

  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

public:
  // O(1), nothrow
  circular_buffer() noexcept : _data(nullptr), _size(0), _capacity(0), _begin_index(0) {}

  // O(n), strong
  circular_buffer(const circular_buffer& other) : circular_buffer(other, other._size) {}

  // O(n), strong
  circular_buffer& operator=(const circular_buffer& other) {
    if (&other != this) {
      circular_buffer tmp(other);
      swap(*this, tmp);
    }
    return *this;
  }

  // O(n), nothrow
  ~circular_buffer() {
    clear();
    operator delete(_data);
  }

  // O(1), nothrow
  size_t size() const noexcept {
    return _size;
  }

  // O(1), nothrow
  bool empty() const noexcept {
    return _size == 0;
  }

  // O(1), nothrow
  size_t capacity() const noexcept {
    return _capacity;
  }

  // O(1), nothrow
  iterator begin() noexcept {
    return iterator(this, 0);
  }

  // O(1), nothrow
  const_iterator begin() const noexcept {
    return const_iterator(const_cast<circular_buffer*>(this), 0);
  }

  // O(1), nothrow
  iterator end() noexcept {
    return iterator(this, size());
  }

  // O(1), nothrow
  const_iterator end() const noexcept {
    return const_iterator(const_cast<circular_buffer*>(this), size());
  }

  // O(1), nothrow
  reverse_iterator rbegin() noexcept {
    return reverse_iterator(end());
  }

  // O(1), nothrow
  const_reverse_iterator rbegin() const noexcept {
    return const_reverse_iterator(end());
  }

  // O(1), nothrow
  reverse_iterator rend() noexcept {
    return reverse_iterator(begin());
  }

  // O(1), nothrow
  const_reverse_iterator rend() const noexcept {
    return const_reverse_iterator(begin());
  }

  // O(1), nothrow
  T& operator[](size_t index) {
    return _data[(_begin_index + index) % _capacity];
  }

  // O(1), nothrow
  const T& operator[](size_t index) const {
    return _data[(_begin_index + index) % _capacity];
  }

  // O(1), nothrow
  T& back() {
    return *(end() - 1);
  }

  // O(1), nothrow
  const T& back() const {
    return *(end() - 1);
  }

  // O(1), nothrow
  T& front() {
    return *(begin());
  }

  // O(1), nothrow
  const T& front() const {
    return *(begin());
  }

  // O(1), strong
  void push_back(const T& value) {
    if (size() == capacity()) {
      circular_buffer tmp(*this, new_capacity());
      tmp.push_back(value);
      swap(*this, tmp);
    } else {
      new (_data + (_begin_index + size()) % _capacity) T(value);
      ++_size;
    }
  }

  // O(1), strong
  void push_front(const T& value) {
    if (size() == capacity()) {
      circular_buffer tmp(*this, new_capacity());
      tmp.push_front(value);
      swap(*this, tmp);
    } else {
      size_t _begin_index_tmp = (_begin_index + capacity() - 1) % capacity();
      new (_data + _begin_index_tmp) T(value);
      _begin_index = _begin_index_tmp;
      ++_size;
    }
  }

  // O(1), nothrow
  void pop_back() {
    assert(size() != 0);
    back().~T();
    --_size;
  }

  // O(1), nothrow
  void pop_front() {
    assert(size() != 0);
    front().~T();
    --_size;
    _begin_index = (_begin_index + 1) % capacity();
  }

  // O(n), strong
  void reserve(size_t desired_capacity) {
    if (desired_capacity > capacity()) {
      circular_buffer tmp(*this, desired_capacity);
      swap(*this, tmp);
    }
  }

  // O(n), basic
  iterator insert(const_iterator pos, const T& value) {
    assert(pos >= begin() && pos <= end());
    size_t distance = pos - begin();
    if (distance >= size() / 2) {
      push_back(value);
      for (iterator i = end() - 1; i > begin() + distance; i--) {
        std::iter_swap(i, i - 1);
      }
    } else {
      push_front(value);
      for (iterator i = begin(); i < pos; i++) {
        std::iter_swap(i, i + 1);
      }
    }
    return begin() + distance;
  }

  // O(n), basic
  iterator erase(const_iterator pos) {
    return erase(pos, pos + 1);
  }

  // O(n), basic
  iterator erase(const_iterator first, const_iterator last) {
    size_t distance = first - begin();
    size_t length = last - first;
    if (last._index >= size() / 2) {
      for (iterator i = begin() + distance; i < end() - length; i++) {
        std::iter_swap(i, i + length);
      }
      while (length--) {
        pop_back();
      }
    } else {
      for (reverse_iterator i = rend() - distance; i != rend(); i++) {
        std::iter_swap(i, i - length);
      }
      while (length--) {
        pop_front();
      }
    }
    return begin() + distance;
  }

  // O(n), nothrow
  void clear() noexcept {
    while (size() != 0) {
      pop_back();
    }
  }

  // O(1), nothrow
  friend void swap(circular_buffer& left, circular_buffer& right) noexcept {
    std::swap(left._data, right._data);
    std::swap(left._begin_index, right._begin_index);
    std::swap(left._size, right._size);
    std::swap(left._capacity, right._capacity);
  }

private:
  circular_buffer(const circular_buffer& other, size_t capacity)
      : _data(capacity == 0 ? nullptr : static_cast<T*>(operator new(sizeof(T) * capacity))),
        _size(other._size),
        _capacity(capacity),
        _begin_index(0) {
    assert(capacity >= other._size);
    try {
      std::uninitialized_copy(other.begin(), other.end(), _data);
    } catch (...) {
      operator delete(_data);
      throw;
    }
  }

  size_t new_capacity() {
    return capacity() == 0 ? 1 : capacity() * 2;
  }

private:
  T* _data;
  size_t _size;
  size_t _capacity;
  std::ptrdiff_t _begin_index;
};
