#pragma once

#include <cassert>
#include <iostream>
#include <iterator>
#include <utility>

template <typename T>
class set {
  class s_iterator;

  struct sentinel_node {
    sentinel_node* left{nullptr};
    sentinel_node* right{nullptr};
    sentinel_node* parent{nullptr};
    std::vector<s_iterator*> share;

    sentinel_node() noexcept : left(nullptr), right(nullptr), parent(nullptr), share() {}

    sentinel_node(sentinel_node* l, sentinel_node* r, sentinel_node* p) : left(l), right(r), parent(p), share() {}

    virtual ~sentinel_node() {
      for (auto it : share) {
        it->node_ = nullptr;
      }
    }

    void add_iterator(s_iterator* it) {
      share.push_back(it);
    }

    void remove_iterator(s_iterator* it) {
      auto it_pos = std::find(share.begin(), share.end(), it);
      if (it_pos != share.end()) {
        share.erase(it_pos);
      }
    }

    friend void swap_shared(s_iterator* lhs, s_iterator* rhs) {
      auto it1 = std::find(lhs->node_->share.begin(), lhs->node_->share.end(), lhs);
      auto it2 = std::find(rhs->node_->share.begin(), rhs->node_->share.end(), rhs);
      std::iter_swap(it1, it2);
    }

    friend void swap(sentinel_node& lhs, sentinel_node& rhs) noexcept {
      std::swap(lhs.left, rhs.left);
      std::swap(lhs.right, rhs.right);
      std::swap(lhs.parent, rhs.parent);
    }
  };

  struct node : sentinel_node {
    const T value_;

    node(sentinel_node* l, sentinel_node* r, sentinel_node* p, const T& v) : sentinel_node(l, r, p), value_(v) {}

    node(sentinel_node* p, const T& v) : node(nullptr, nullptr, p, v) {}
  };

  class s_iterator {
  public:
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = bool;
    using reference = const T&;
    using pointer = const T*;

  private:
    sentinel_node* node_;
    const set* container_;

    s_iterator(sentinel_node* node, const set* container) : node_(node), container_(container) {
      append_to_node();
    }

    void append_to_node() {
      try {
        if (node_) {
          node_->add_iterator(this);
        }
      } catch (...) {
        node_ = nullptr;
        throw;
      }
    }

    static sentinel_node* find_min(sentinel_node* node) {
      auto current = node;
      while (current->left) {
        current = current->left;
      }
      return current;
    }

    static sentinel_node* find_max(sentinel_node* node) {
      auto current = node;
      while (current->right) {
        current = current->right;
      }
      return current;
    }

    static sentinel_node* find_next(sentinel_node* node) {
      if (node->right) {
        return find_min(node->right);
      }
      sentinel_node* parent = node->parent;
      while (parent && node == parent->right) {
        node = parent;
        parent = parent->parent;
      }
      return parent;
    }

    friend set;

    void base_assert() const {
      assert(node_ && node_->left != node_);
    }

    void move_to_another_node(sentinel_node* new_node) {
      try {
        std::swap(node_, new_node);
        if (node_) {
          node_->add_iterator(this);
        }
      } catch (...) {
        std::swap(node_, new_node);
        throw;
      }
      if (new_node) {
        new_node->remove_iterator(this);
      }
    }

  public:
    s_iterator() : node_(nullptr), container_(nullptr) {}

    s_iterator(const s_iterator& other) : node_(other.node_), container_(other.container_) {
      append_to_node();
    }

    s_iterator& operator=(const s_iterator& other) {
      if (this == &other) {
        return *this;
      }
      container_ = other.container_;
      move_to_another_node(other.node_);
      return *this;
    }

    ~s_iterator() {
      if (node_) {
        node_->remove_iterator(this);
      }
    }

    // s_iterator(const s_iterator& other) = default;

    reference operator*() const {
      base_assert();
      return static_cast<node*>(node_)->value_;
    }

    pointer operator->() const noexcept {
      base_assert();
      return &(static_cast<node*>(node_)->value_);
    }

    s_iterator& operator++() {
      base_assert();
      sentinel_node* new_node = find_next(node_);
      move_to_another_node(new_node);
      return *this;
    }

    s_iterator operator++(int) {
      auto tmp = *this;
      ++*this;
      return tmp;
    }

    s_iterator& operator--() {
      base_assert();
      sentinel_node* new_node = node_;
      if (new_node->left != nullptr) {
        new_node = find_max(node_->left);
      } else {
        sentinel_node* parent = new_node->parent;
        while (parent != nullptr && new_node == parent->left) {
          new_node = parent;
          parent = parent->parent;
        }
        new_node = parent;
      }
      move_to_another_node(new_node);
      base_assert();
      return *this;
    }

    s_iterator operator--(int) {
      auto tmp = *this;
      --*this;
      return tmp;
    }

    friend bool operator==(const s_iterator& left, const s_iterator& right) {
      assert(left.container_ == right.container_);
      assert(left.container_);
      return left.node_ == right.node_;
    }

    friend bool operator!=(const s_iterator& left, const s_iterator& right) {
      return !(left == right);
    }

    friend void swap(s_iterator& left, s_iterator& right) {
      sentinel_node* left_node = left.node_;
      sentinel_node* right_node = right.node_;
      std::swap(left.container_, right.container_);
      std::swap(left.node_, right.node_);

      for (auto it : left_node->share) {
        it->node_ = left_node;
      }
      for (auto it : right_node->share) {
        it->node_ = right_node;
      }
    }
  };

public:
  using value_type = T;

  using reference = T&;
  using const_reference = const T&;

  using pointer = T*;
  using const_pointer = const T*;

  using iterator = s_iterator;
  using const_iterator = s_iterator;

  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

private:
  sentinel_node fake_;
  std::size_t size_{0};

  sentinel_node* take_root() const {
    return fake_.left;
  }

  sentinel_node* take_root() {
    return fake_.left;
  }

  void put_root(sentinel_node* node) {
    fake_.left = node;
  }

  sentinel_node* copy_tree(sentinel_node* other_root) {
    if (!other_root) {
      return nullptr;
    }

    sentinel_node* new_node = nullptr;
    try {
      new_node = new node(nullptr, static_cast<node*>(other_root)->value_);

      if (other_root->left) {
        new_node->left = copy_tree(other_root->left);
        new_node->left->parent = new_node;
      }
      if (other_root->right) {
        new_node->right = copy_tree(other_root->right);
        new_node->right->parent = new_node;
      }
    } catch (...) {
      del_subtree(new_node);
      throw;
    }
    return new_node;
  }

  static sentinel_node* find(sentinel_node* root, const T& val) {
    if (root == nullptr || static_cast<node*>(root)->value_ == val) {
      return root;
    }

    if (val < static_cast<node*>(root)->value_) {
      return find(root->left, val);
    }

    return find(root->right, val);
  }

  sentinel_node* merge(sentinel_node* left, sentinel_node* right) {
    if (!left) {
      return right;
    }
    if (!right) {
      return left;
    }

    sentinel_node* min_right = s_iterator::find_min(right);
    if (min_right == right) {
      min_right->left = left;
      left->parent = min_right;
      return right;
    }

    if (min_right->parent->left == min_right) {
      min_right->parent->left = min_right->right;
      if (min_right->right) {
        min_right->right->parent = min_right->parent;
      }
    } else {
      min_right->parent->right = min_right->right;
      if (min_right->right) {
        min_right->right->parent = min_right->parent;
      }
    }

    min_right->left = left;
    min_right->right = right;
    if (left) {
      left->parent = min_right;
    }
    if (right) {
      right->parent = min_right;
    }

    return min_right;
  }

  void del_subtree(sentinel_node* d_node) {
    if (!d_node) {
      return;
    }
    del_subtree(d_node->left);
    del_subtree(d_node->right);
    delete d_node;
  }

  explicit set(const std::size_t size) noexcept : fake_(sentinel_node()), size_(size) {
    fake_.left = &fake_;
    fake_.parent = &fake_;
  }



public:
  // O(1) nothrow
  set() noexcept : set(0) {}

  // O(n) strong
  set(const set& other) : set(other.size()) {
    try {
      if (!other.empty()) {
        put_root(copy_tree(other.take_root()));
        take_root()->parent = &fake_;
      }
    } catch (...) {
      size_ = 0;
      throw;
    }
  }

  // O(n) strong
  set& operator=(const set& other) {
    if (this == &other) {
      return *this;
    }
    set copy(other);
    swap(*this, copy);
    return *this;
  }

  // O(n) nothrow
  ~set() noexcept {
    clear();
  }

  // O(n) nothrow
  void clear() noexcept {
    if (empty()) {
      return;
    }
    del_subtree(take_root());
    fake_.left = nullptr;
    size_ = 0;
  }

  // O(1) nothrow
  size_t size() const noexcept {
    return size_;
  }

  // O(1) nothrow
  bool empty() const noexcept {
    return size() == 0;
  }

  // nothrow
  const_iterator begin() const noexcept {
    if (empty()) {
      return end();
    }
    return s_iterator(s_iterator::find_min(take_root()), this);
  }

  // nothrow
  const_iterator end() const noexcept {
    return const_iterator(const_cast<sentinel_node*>(&fake_), this);
  }

  // nothrow
  const_reverse_iterator rbegin() const noexcept {
    return reverse_iterator(end());
  }

  // nothrow
  const_reverse_iterator rend() const noexcept {
    return reverse_iterator(begin());
  }

  // O(h) strong
  std::pair<iterator, bool> insert(const T& val) {
    if (empty()) {
      put_root(new node(&fake_, val));
      ++size_;
      fake_.parent = nullptr;
      try {
        return {iterator(take_root(), this), true};
      } catch (...) {
        clear();
        throw;
      }
    }
    sentinel_node* parent = nullptr;
    sentinel_node* current = take_root();

    while (current != nullptr) {
      parent = current;
      if (val < static_cast<node*>(current)->value_) {
        current = current->left;
      } else if (val > static_cast<node*>(current)->value_) {
        current = current->right;
      } else {
        return {iterator(current, this), false};
      }
    }
    node* new_node = nullptr;
    iterator it = iterator();
    try {
      new_node = new node(parent, val);
      it = iterator(new_node, this);

    } catch (...) {
      if (!new_node) {
        delete new_node;
      }
      throw;
    }

    if (val < static_cast<node*>(parent)->value_) {
      parent->left = new_node;
    } else {
      parent->right = new_node;
    }

    ++size_;
    return {it, true};
  }

  // O(h) nothrow
  iterator erase(const_iterator pos) {
    assert(this == pos.container_);
    if (empty()) {
      return end();
    }
    sentinel_node* return_value = s_iterator::find_next(pos.node_);
    sentinel_node* node_to_delete = pos.node_;
    sentinel_node* del_parent = node_to_delete->parent;
    sentinel_node* new_subtree = merge(node_to_delete->left, node_to_delete->right);

    if (del_parent->left == node_to_delete) {
      del_parent->left = new_subtree;
    } else {
      del_parent->right = new_subtree;
    }

    if (new_subtree) {
      new_subtree->parent = del_parent;
    }

    if (take_root() == node_to_delete) {
      put_root(new_subtree ? new_subtree : del_parent);
    }

    delete node_to_delete;
    --size_;

    return iterator(return_value, this);
  }

  // O(h) strong
  const_iterator find(const T& val) const {
    if (empty()) {
      return end();
    }
    auto it = find(take_root(), val);
    return (it ? const_iterator(it, this) : end());
  }

  // O(h) strong
  size_t erase(const T& val) {
    const_iterator it = find(val);
    if (it == end()) {
      return 0;
    }
    erase(it);
    return 1;
  }

  // O(h) strong
  const_iterator lower_bound(const T& val) const {
    if (empty()) {
      return end();
    }
    sentinel_node* current = take_root();
    sentinel_node* result = nullptr;

    while (current != nullptr) {
      if (static_cast<node*>(current)->value_ >= val) {
        result = current;
        current = current->left;
      } else {
        current = current->right;
      }
    }

    return result == nullptr ? end() : const_iterator(result, this);
  }

  // O(h) strong
  const_iterator upper_bound(const T& val) const {
    if (empty()) {
      return end();
    }
    sentinel_node* current = take_root();
    sentinel_node* result = nullptr;

    while (current != nullptr) {
      if (static_cast<node*>(current)->value_ > val) {
        result = current;
        current = current->left;
      } else {
        current = current->right;
      }
    }

    return result == nullptr ? end() : const_iterator(result, this);
  }

  // O(1) nothrow
  friend void swap(set& lhs, set& rhs) noexcept {
    sentinel_node* lhs_root = lhs.take_root();
    sentinel_node* rhs_left = rhs.take_root();
    swap(lhs.fake_, rhs.fake_);
    std::swap(lhs.size_, rhs.size_);
    if (lhs_root) {
      lhs_root->parent = &rhs.fake_;
    }
    if (rhs_left) {
      rhs_left->parent = &lhs.fake_;
    }
  }
};