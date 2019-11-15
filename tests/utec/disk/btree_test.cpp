
// #include <utecdf/column/column.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <utec/disk/btree.h>
#include <utec/disk/pagemanager.h>
#include <algorithm>

#include <fmt/core.h>

// PAGE_SIZE 64 bytes
#define PAGE_SIZE  64

// Other examples:
// PAGE_SIZE 1024 bytes => 1Kb
// PAGE_SIZE 1024*1024 bytes => 1Mb

// PAGE_SIZE = 2 * sizeof(long) +  (BTREE_ORDER + 1) * sizeof(int) + (BTREE_ORDER + 2) * sizeof(long)  
// PAGE_SIZE = 2 * sizeof(long) +  (BTREE_ORDER) * sizeof(int) + sizeof(int) + (BTREE_ORDER) * sizeof(long) + 2 * sizeof(long)
// PAGE_SIZE = (BTREE_ORDER) * (sizeof(int) + sizeof(long))  + 2 * sizeof(long) + sizeof(int) +  2 * sizeof(long)
//  BTREE_ORDER = PAGE_SIZE - (2 * sizeof(long) + sizeof(int) +  2 * sizeof(long)) /  (sizeof(int) + sizeof(long))

#define BTREE_ORDER   ((PAGE_SIZE - (2 * sizeof(long) + sizeof(int) +  2 * sizeof(long)) ) /  (sizeof(int) + sizeof(long)))

using namespace utec::disk;
    
struct DiskBasedBtree : public ::testing::Test
{
};

struct Student {
  long  id;
  bool passed;
  char name[32];
  char surname[32];
  int  n_credits;
};

struct Pair {
  long id;
  long page_id;
  bool  operator < (Pair& p) const; // compare using only id.
};

void index_records_using_btree_only_one_time () {
  bool trunc_file = true;
  const int page_size = 1024;
  const int btree_order = 82;
  std::shared_ptr<pagemanager> pm = std::make_shared<pagemanager>("btree.index", page_size, trunc_file);
  btree< long, btree_order> bt(pm);
  
  pagemanager record_manager ("students.bin", page_size, trunc_file);
  long page_id;
  Student p {10, false,"alex","orihuela",150};
  page_id = 0;
  record_manager.save(page_id, p);
  bt.insert(page_id, p.id);

  Student p1 {20, true,"Luis","sanchez",100};
  page_id = 1;
  record_manager.save(page_id, p1);
  bt.insert(page_id, p.id);

  Student p2 {30, false,"alvaro","valera",150};
  page_id = 2;
  record_manager.save(page_id, p2);
  bt.insert(page_id, p.id);

  Student p3 {40, true,"javier","Mayori",15};
  page_id = 3;
  record_manager.save(page_id, p3);
  bt.insert(page_id, p.id);
}

// select * from Student where 50 <= id and id <= 100 
void select(){
  const int page_size = 1024;
  const int btree_order = 82;
  pagemanager record_manager ("students.bin", page_size);

  std::shared_ptr<pagemanager> pm = std::make_shared<pagemanager>("btree.index", page_size);
  btree< long, btree_order> bt(pm);
  auto iter = bt.range_search(3, 30);
  for (; iter != bt.end(); iter++) {
    Student s;
    record_manager.recover(*iter, s);
    std::cout << s.id << " " << s.passed << " " << s.name << " " << s.surname << " " << s.n_credits << std::endl;
  }
}
// Compare FileScan in student records vs Select using Btree

TEST_F(DiskBasedBtree, IndexingRandomElements) {
  bool trunc_file = true;
  std::shared_ptr<pagemanager> pm = std::make_shared<pagemanager>("btree.index", PAGE_SIZE, trunc_file);
  std::cout << "PAGE_SIZE: " << PAGE_SIZE << std::endl;
  std::cout << "BTREE_ORDER: " << BTREE_ORDER << std::endl;
  btree<char, BTREE_ORDER> bt(pm);
  std::string values = "zxcnmvfjdaqpirue";
  for(auto c : values) {
    bt.insert(c, (long)c);
  }
  bt.print_tree();
  std::ostringstream out;
  bt.print(out);
  std::sort(values.begin(), values.end());

  EXPECT_EQ(out.str(), values.c_str());
}
 

TEST_F(DiskBasedBtree, Persistence) {
  std::shared_ptr<pagemanager> pm = std::make_shared<pagemanager>("btree.index", PAGE_SIZE);
  btree<char, BTREE_ORDER> bt(pm);
  std::string values = "123456";
  for(auto c : values) {
    bt.insert(c, (long)c);
  }
  bt.print_tree();

  std::ostringstream out;
  bt.print(out);
  std::string all_values = "zxcnmvfjdaqpirue123456";
  std::sort(all_values.begin(), all_values.end());
  EXPECT_EQ(out.str(), all_values.c_str());
}


TEST_F(DiskBasedBtree, Iterators) {
  bool trunc_file = true;
  std::shared_ptr<pagemanager> pm = std::make_shared<pagemanager>("btree.index", PAGE_SIZE, trunc_file);
  btree<int, BTREE_ORDER> bt(pm);

  std::ifstream testFile("1M_numbers.txt");
  std::string all_letters = "";

  int number;
  while (testFile >> number){
    bt.insert(number, (int)number);
    all_letters+=std::to_string(number);
  }

  std::ostringstream out;
  btree<int, BTREE_ORDER>::iterator iter = bt.begin();
  for( ; iter != bt.end(); iter++) {
      out << *iter;
  }
  std::sort(all_letters.begin(), all_letters.end());
  EXPECT_EQ(out.str(), all_letters);
}


TEST_F(DiskBasedBtree, Dictionary) {
  index_records_using_btree_only_one_time();
  select();
}


