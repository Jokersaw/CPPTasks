#pragma once
#include <algorithm>
#include <cstddef>
#include <memory>

template <typename T, size_t SMALL_SIZE>
class socow_vector {
public:
  using iterator = T*;
  using const_iterator = const T*;

  using value_type = T;

  using reference = T&;
  using const_reference = const T&;

  using pointer = T*;
  using const_pointer = const T*;

  socow_vector() : _size(0), is_small(true) {}

  socow_vector(const socow_vector& other) : _size(other._size), is_small(other.is_small) {
    if (other.is_small) {
      std::uninitialized_copy_n(other.small_storage, other._size, small_storage);
    } else {
      big_storage = other.big_storage;
      big_storage->counter_++;
    }
  }

  socow_vector& operator=(const socow_vector& other) {
    if (&other != this) {
      if (!is_small && other.is_small) {
        convert_to_small(other.small_storage, other.size());
      } else if (!other.is_small) {
        clear_with_big_storage();
        big_storage = other.big_storage;
        big_storage->counter_++;
        is_small = false;
      } else {
        size_t min_size = std::min(size(), other.size());

        socow_vector temp;
        std::uninitialized_copy_n(other.small_storage, min_size, temp.small_storage);
        temp._size = min_size;

        if (other.size() > size()) {
          std::uninitialized_copy_n(other.small_storage + size(), other.size() - size(), small_storage + size());
        } else if (other.size() < size()) {
          std::destroy(small_storage + other.size(), small_storage + size());
        }
        _size = other.size();
        std::swap_ranges(temp.small_storage, temp.small_storage + temp.size(), small_storage);
      }
      _size = other.size();
    }

    return *this;
  }

  ~socow_vector() {
    clear_with_big_storage();
  }

  T& operator[](size_t index) {
    return data()[index];
  }

  const T& operator[](size_t index) const {
    return data()[index];
  }

  T* data() {
    copy_on_write();
    return is_small ? small_storage : big_storage->data_;
  }

  const T* data() const {
    return is_small ? small_storage : big_storage->data_;
  }

  size_t size() const {
    return _size;
  }

  T& front() {
    return *begin();
  }

  const T& front() const {
    return *begin();
  }

  T& back() {
    return *(end() - 1);
  }

  const T& back() const {
    return *(end() - 1);
  }

  void push_back(const T& element) {
    insert(my_end(), element);
  }

  void pop_back() {
    erase(my_end() - 1);
  }

  bool empty() const {
    return _size == 0;
  }

  size_t capacity() const {
    return is_small ? SMALL_SIZE : big_storage->capacity_;
  }

  void reserve(size_t new_capacity) {
    if (new_capacity < size()) {
      return;
    }
    if (!is_small && new_capacity <= SMALL_SIZE) {
      convert_to_small(big_storage->data_, size());
    } else if (new_capacity > capacity() || is_shared()) {
      expand_storage(new_capacity);
    }
  }

  void shrink_to_fit() {
    if (is_small) {
      return;
    }
    if (_size <= SMALL_SIZE) {
      storage* tmp = big_storage;
      big_storage = nullptr;
      try {
        std::uninitialized_copy_n(tmp->data_, _size, small_storage);
      } catch (...) {
        big_storage = tmp;
        throw;
      }
      release_storage(tmp);
      is_small = true;
    } else if (_size != capacity()) {
      expand_storage(_size);
    }
  }

  void clear() {
    if (is_shared()) {
      release_storage(big_storage);
      is_small = true;
    } else {
      std::destroy(my_begin(), my_end());
    }
    _size = 0;
  }

  void swap(socow_vector& other) {
    if (&other == this) {
      return;
    }
    if (is_small && other.is_small) {
      if (size() < other.size()) {
        other.swap(*this);
      } else {
        std::uninitialized_copy_n(small_storage + other.size(), size() - other.size(),
                                  other.small_storage + other.size());
        std::destroy_n(small_storage + other.size(), size() - other.size());
        std::swap(_size, other._size);
        std::swap_ranges(small_storage, small_storage + other.size(), other.small_storage);
      }
    } else if (!is_small && !other.is_small) {
      std::swap(big_storage, other.big_storage);
      std::swap(_size, other._size);
    } else if (!is_small && other.is_small) {
      socow_vector tmp = *this;
      *this = other;
      other = tmp;
    } else {
      other.swap(*this);
    }
  }

  iterator begin() {
    return data();
  }

  iterator end() {
    return data() + _size;
  }

  const_iterator begin() const {
    return data();
  }

  const_iterator end() const {
    return data() + _size;
  }

  iterator insert(const_iterator pos, const T& value) {
    size_t distance = pos - my_begin();
    if (_size == capacity() || is_shared()) {
      socow_vector temp;
      temp.big_storage = make_new_storage_with_fixed_capacity(_size == capacity() ? capacity() * 2 : capacity());
      temp.is_small = false;
      std::uninitialized_copy_n(my_begin(), distance, temp.my_begin());
      temp._size = distance;
      new (temp.my_begin() + distance) T(value);
      temp._size++;
      std::uninitialized_copy(my_begin() + distance, my_begin() + size(), temp.my_begin() + distance + 1);
      temp._size = size() + 1;
      *this = temp;
    } else {
      new (end()) T(value);
      _size++;
      for (iterator i = end() - 1; i > my_begin() + distance; i--) {
        std::iter_swap(i, i - 1);
      }
    }

    return my_begin() + distance;
  }

  iterator erase(const_iterator pos) {
    return erase(pos, pos + 1);
  }

  iterator erase(const_iterator first, const_iterator last) {
    size_t f_distance = first - my_begin();
    size_t l_distance = last - my_begin();
    if (f_distance == l_distance) {
      return my_begin() + f_distance;
    }
    size_t new_size = size() - (l_distance - f_distance);
    if (is_shared()) {
      socow_vector temp;
      temp.big_storage = make_new_storage_with_fixed_capacity(capacity());
      temp.is_small = false;
      std::uninitialized_copy_n(my_begin(), f_distance, temp.begin());
      temp._size = f_distance;
      std::uninitialized_copy_n(big_storage->data_ + l_distance, new_size - f_distance, temp.my_begin() + f_distance);
      temp._size = new_size;
      *this = temp;
    } else {
      std::swap_ranges(my_begin() + l_distance, my_begin() + size(), my_begin() + f_distance);
      std::destroy(my_begin() + new_size, my_begin() + size());
    }
    _size = new_size;
    return my_begin() + f_distance;
  }

private:
  struct storage {
    size_t counter_;
    size_t capacity_;
    T data_[0];

    explicit storage(size_t capacity_) : counter_(1), capacity_(capacity_) {}

    bool dec() {
      counter_--;
      return counter_ == 0;
    }

    bool is_not_unique() const {
      return counter_ > 1;
    }
  };

  size_t _size;
  bool is_small;

  union {
    T small_storage[SMALL_SIZE];
    storage* big_storage;
  };

  iterator my_begin() {
    return is_small ? small_storage : big_storage->data_;
  }

  iterator my_end() {
    return my_begin() + _size;
  }

  void release_storage(storage* st) {
    if (st->dec()) {
      std::destroy(st->data_, st->data_ + _size);
      operator delete(st);
    }
  }

  void expand_storage(size_t new_capacity) {
    storage* tmp = copy_storage_with_fixed_capacity(new_capacity, _size);
    clear_with_big_storage();
    big_storage = tmp;
    is_small = false;
  }

  static storage* make_new_storage_with_fixed_capacity(size_t new_capacity) {
    storage* ans =
        new (static_cast<storage*>(operator new(sizeof(storage) + new_capacity * sizeof(T)))) storage(new_capacity);
    return ans;
  }

  storage* copy_storage_with_fixed_capacity(size_t new_capacity, size_t cur_size) {
    storage* ans = make_new_storage_with_fixed_capacity(new_capacity);
    try {
      std::uninitialized_copy_n(my_begin(), cur_size, ans->data_);
    } catch (...) {
      operator delete(ans);
      throw;
    }
    return ans;
  }

  void copy_on_write() {
    if (is_shared()) {
      storage* new_storage = copy_storage_with_fixed_capacity(capacity(), _size);
      big_storage->dec();
      big_storage = new_storage;
    }
  }

  void convert_to_small(const T* from, size_t cur_size) {
    storage* save = big_storage;
    try {
      std::uninitialized_copy_n(from, cur_size, small_storage);
    } catch (...) {
      big_storage = save;
      throw;
    }
    release_storage(save);
    is_small = true;
  }

  bool is_shared() const {
    return (!is_small && big_storage->is_not_unique());
  }

  void clear_with_big_storage() {
    if (is_small) {
      std::destroy(my_begin(), my_end());
      return;
    }
    release_storage(big_storage);
  }
};
