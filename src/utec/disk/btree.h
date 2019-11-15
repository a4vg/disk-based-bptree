#pragma once

#include "pagemanager.h"
#include <memory>
#include <stdexcept>
#include <utility>

namespace utec {
namespace disk {

template <class T, int BTREE_ORDER = 3>
struct Node {
    long page_id{-1};
    long count{0};
    
    long keys[BTREE_ORDER + 1];
    T data[BTREE_ORDER + 1];
    long children[BTREE_ORDER + 2];
    long next{0};

    Node(long page_id) : page_id{page_id} {
      count = 0;
      for (int i = 0; i < BTREE_ORDER + 2; i++) {
        children[i] = 0;
        keys[i] = -1;
      }
    }

    void insert_in_node(int pos, const T &value, long key, bool isLeaf=false) {
      int j = count;
      while (j > pos) {
        data[j] = data[j - 1];
        keys[j] = keys[j - 1];
        children[j + 1] = children[j];
        j--;
      }
      keys[j] = key;
      if (isLeaf) data[j] = value; // store data only if is leaf
      
      children[j + 1] = children[j];

      count++;
    }

    bool is_overflow() { return count > BTREE_ORDER; }
};

template <class T, int BTREE_ORDER = 3>
struct Iterator{
  typedef Node<T, BTREE_ORDER> node;

  long begin[2]; // page pos
  long end[2] = {-1, 0};
  std::shared_ptr<pagemanager> pm;

  Iterator(std::shared_ptr<pagemanager> pm, long begin_page, int begin_pos)
  : pm{pm}{
    begin[0] = begin_page;
    begin[1] = begin_pos;
  }

  Iterator operator++(int){
    Iterator old(pm, begin[0], begin[1]);
    node n{-1};
    pm->recover(begin[0], n);

    if (begin[1] < n.count-1) ++begin[1];
    else{
      begin[0] = n.next;
      begin[1] = 0;
    }

    return old;
  }

  bool operator!=(const Iterator &other) const{
    return begin[0] != other.begin[0] || begin[1] != other.begin[1];
  }

  const T &operator*() const{
    if (begin[0]==0 || (begin[0]==end[0] && begin[1]==end[1]))
      throw std::out_of_range("Iterator reached end");

    node n{-1};
    pm->recover(begin[0], n);
    return n.data[begin[1]];
  }

  Iterator limit(){
    return Iterator(pm, end[0], end[1]);
  }
};

template <class T, int BTREE_ORDER = 3> class btree {
public:
  typedef Node<T, BTREE_ORDER> node;
  typedef Iterator<T, BTREE_ORDER> iterator;
  struct Metadata {
    long root_id{1};
    long count{0};
  } header;

  enum state {
    BT_OVERFLOW,
    BT_UNDERFLOW,
    NORMAL,
  };

private:
  std::shared_ptr<pagemanager> pm;

public:
  btree(std::shared_ptr<pagemanager> pm) : pm{pm} {
    if (pm->is_empty()){
      node root{header.root_id};
      pm->save(root.page_id, root);
      header.count++;
      pm->save(0, header);
    }
    else{
      pm->recover(0, header);
    }
  }

  node new_node() {
    header.count++;
    node ret{header.count};
    pm->save(0, header);
    return ret;
  }

  node read_node(long page_id) {
    node n{-1};
    pm->recover(page_id, n);
    return n;
  }

  bool write_node(long page_id, node n) { pm->save(page_id, n); }

  void insert(const T &value, long key) {
    node root = read_node(header.root_id);
    int state = insert(root, value, key);

    if (state == BT_OVERFLOW) {
      split_root();
    }
  }

  int insert(node &ptr, const T &value, long key, long grandpa_id=0) {
    int pos = 0;
    while (pos < ptr.count && ptr.keys[pos] < value) {
      pos++;
    }
    if (ptr.children[pos] != 0) {
      long page_id = ptr.children[pos];
      node child = read_node(page_id);
      int state = insert(child, value, key, ptr.page_id);
      if (state == BT_OVERFLOW) {
        split(ptr, pos, grandpa_id);
      }
    } else { // is leaf
      ptr.insert_in_node(pos, value, key, ptr.children[pos] == 0);
      write_node(ptr.page_id, ptr);
    }
    return ptr.is_overflow() ? BT_OVERFLOW : NORMAL;
  }

  void split(node &parent, int pos, long grandpa_id=0) {
    node ptr = this->read_node(parent.children[pos]);
    node left = this->new_node();
    node right = this->new_node();

    int iter = 0;
    int i;
    for (i = 0; iter < BTREE_ORDER / 2; i++) {
      left.children[i] = ptr.children[iter];
      left.keys[i] = ptr.keys[iter];
      if (ptr.children[0]==0) std::swap(left.data[i], ptr.data[iter]); // ptr is leaf
      left.count++;
      iter++;
    }
    left.children[i] = ptr.children[iter];

    parent.insert_in_node(pos, ptr.data[iter], ptr.keys[iter]); // will only insert key
                                                                // (parent will never be a leaf)
    if (ptr.children[0]!=0) iter++; // skip middle element only if intermediate node

    for (i = 0; iter < BTREE_ORDER + 1; i++) {
      right.children[i] = ptr.children[iter];
      right.keys[i] = ptr.keys[iter];
      if (ptr.children[0]==0) std::swap(right.data[i], ptr.data[iter]); // ptr is leaf
      right.count++;

      iter++;
    }
    right.children[i] = ptr.children[iter];

    parent.children[pos] = left.page_id;
    parent.children[pos + 1] = right.page_id;

    // TODO: Clean code
    parent.next = 0;
    if (ptr.children[0]==0){ // link nodes if leaf
      left.next = right.page_id;
      right.next = ptr.next;

      if (pos-1<0){ // prev node has different parent
        if (grandpa_id!=0){
          node grandpa = this->read_node(grandpa_id);
          int parentPos = 0;

          for (; grandpa.children[parentPos]!=parent.page_id; ++parentPos);

          if (parentPos!=0){
            node prevParent = this->read_node(grandpa.children[--parentPos]);
            int posPrev=0;
            for (; prevParent.children[posPrev]!=0; ++posPrev);
            node prevChild = this->read_node(prevParent.children[--posPrev]);
            prevChild.next = left.page_id;
            write_node(prevChild.page_id, prevChild); 
          }
        }
      }
      else{
        node prevChild = this->read_node(parent.children[pos-1]);
        prevChild.next = left.page_id;
        write_node(prevChild.page_id, prevChild);
      }
    }

    pm->erase(ptr.page_id, ptr);
    write_node(parent.page_id, parent);
    write_node(left.page_id, left);
    write_node(right.page_id, right);
  }

  void split_root() {
    node ptr = this->read_node(this->header.root_id);
    node left = this->new_node();
    node right = this->new_node();

    int pos = 0;
    int iter = 0;
    int i;
    for (i = 0; iter < BTREE_ORDER / 2; i++) {
      left.children[i] = ptr.children[iter];
      if (ptr.children[0]==0) std::swap(left.data[i], ptr.data[iter]); // ptr (root) is leaf
      left.keys[i] = ptr.keys[iter];
      left.count++;
      iter++;
    }
    left.children[i] = ptr.children[iter];

    if (ptr.children[0]!=0) iter++; // skip middle element only if intermediate node

    for (i = 0; iter < BTREE_ORDER + 1; i++) {
      right.children[i] = ptr.children[iter];
      if (ptr.children[0]==0) std::swap(right.data[i], ptr.data[iter]); // ptr (root) is leaf
      right.keys[i] = ptr.keys[iter];
      right.count++;
      iter++;
    }
    right.children[i] = ptr.children[iter];

    ptr.next = 0;
    if (ptr.children[0]==0){ // ptr (root) is leaf
      left.next = right.page_id;
      right.next = ptr.children[pos + 2];
    }

    ptr.children[pos] = left.page_id;
    ptr.keys[0] = ptr.keys[BTREE_ORDER / 2]; // store key, not data
    ptr.children[pos + 1] = right.page_id;
    ptr.count = 1;

    write_node(ptr.page_id, ptr);
    write_node(left.page_id, left);
    write_node(right.page_id, right);
  }

  iterator end(){
    return iterator(pm, 0, 0);
  }

  std::pair<bool, iterator> find(const T &key) {
    node root = read_node(header.root_id);
    return find(root, key);
  }

  std::pair<bool, iterator> find(node &ptr, long key) {
    int pos = 0;
    while (pos < ptr.count && ptr.keys[pos] < key) {
      pos++;
    }

    if (ptr.children[pos] != 0) {
      long page_id = ptr.children[pos];
      node child = read_node(page_id);
      return find(child, key);
    } else { // is leaf
      int i=0;
      for (; i<ptr.count-1 && ptr.keys[i]<key; ++i);

      int page = ptr.page_id;
      if (ptr.keys[i]!=key) page = ptr.next;

      return std::make_pair(ptr.keys[i]==key, iterator(pm, page, i));
    }
  }

  iterator range_search(const T &begin, const T &end){
    node root = read_node(header.root_id);
    iterator it = find(root, begin).second;
    iterator it_end = find(root, end).second;

    it.end[0] = it_end.begin[0];
    it.end[1] = it_end.begin[1];

    return it;
  }

  void print_tree() {
    node root = read_node(header.root_id);
    print(root, 0);
    std::cout << "________________________\n";
  }

  void print(node &ptr, int level) {
    int i;
    for (i = ptr.count - 1; i >= 0; i--) {
      if (ptr.children[i + 1]) {
        node child = read_node(ptr.children[i + 1]);
        print(child, level + 1);
      }

      for (int k = 0; k < level; k++) {
        std::cout << "    ";
      }

      if (ptr.data[i]!=-1) std::cout << (T)ptr.data[i] << std::endl;
      else std::cout << ptr.keys[i] << std::endl;
    }
    if (ptr.children[i + 1]) {
      node child = read_node(ptr.children[i + 1]);
      print(child, level + 1);
    }
  }

  void print(std::ostringstream &out){
    node ptr = read_node(header.root_id);

    // go down left
    while (ptr.children[0]!=0) ptr = read_node(ptr.children[0]);

    // go throgh linked leaf nodes
    while (ptr.next != 0){
      for (int i=0; i<ptr.count; ++i) out << ptr.data[i];
      ptr = read_node(ptr.next);
    }
    for (int i=0; i<ptr.count; ++i) out << ptr.data[i];
  }
};

} // namespace disk
} // namespace utec