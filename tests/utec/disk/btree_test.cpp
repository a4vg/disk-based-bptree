
// #include <utecdf/column/column.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <utec/disk/btree.h>
#include <utec/disk/pagemanager.h>
#include <algorithm>

#include <fmt/core.h>

#define GETORDER(p, d) ((p - (7 * sizeof(long) + d) ) /  (d + sizeof(long)))

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

  Student(){};
  Student(long _id, bool _passed, std::string s_name, std::string s_sur, int _ncredits)
  : id(_id), passed(_passed), n_credits(_ncredits){
    strncpy(name, s_name.c_str(), sizeof(name)-1);
    name[sizeof(name)-1]=0;

    strncpy(surname, s_sur.c_str(), sizeof(surname)-1);
    surname[sizeof(surname)-1]=0;
  }
};

void index_records_using_btree_only_one_time () {
  // Define constants
  const int page_size = 1024;
  const int btree_order = GETORDER(page_size, sizeof(char));

  std::cout << "PAGE_SIZE: " << page_size << std::endl;
  std::cout << "BTREE_ORDER: " << btree_order << std::endl;

  // Create tree
  std::shared_ptr<pagemanager> pm = std::make_shared<pagemanager>("btree.index", page_size, true); // trunc file = true
  btree< long, btree_order> bt(pm);
  
  // Create students from file and insert into btree
  pagemanager record_manager ("students.bin", page_size, true); // trunc file = true
  std::ifstream data("students-shuffled.csv");
  long id;
  bool passed;
  std::string s_name, s_surname;
  int n_credits;

  while (data >> id >> passed >> s_name >> s_surname >> n_credits){
    Student p(id, passed, s_name, s_surname, n_credits);
    long page_id = id;
    record_manager.save(page_id, p);
    bt.insert(page_id, p.id);
  }
}

// select * from Student where x <= id and id < y
void select(long x, long y){
  // Define constants
  const int page_size = 1024;
  const int btree_order = GETORDER(page_size, sizeof(char));

  // Recover tree
  std::shared_ptr<pagemanager> pm = std::make_shared<pagemanager>("btree.index", page_size); // trunc file = false
  btree< long, btree_order> bt(pm);

  // Recover students binary data file
  pagemanager record_manager ("students.bin", page_size); // trunc file = false

  // Select students
  auto iter = bt.range_search(x, y); // [x, y>
  for (; iter != iter.limit(); iter++){
    Student s;
    record_manager.recover(*iter, s);
    std::cout << s.id << " " << s.passed << " " << s.name << " " << s.surname << " " << s.n_credits << std::endl;
  }
}

TEST_F(DiskBasedBtree, IndexingRandomElements) {
  // Define constants
  const int page_size = 64;
  const int btree_order = 2; // GETORDER(page_size, sizeof(char));

  std::cout << "PAGE_SIZE: " << page_size << std::endl;
  std::cout << "BTREE_ORDER: " << btree_order << std::endl;
  
  // Create tree
  std::shared_ptr<pagemanager> pm = std::make_shared<pagemanager>("btree.index", page_size, true); // trunc file = true
  btree<char, btree_order> bt(pm);

  // Insert data into tree
  std::string values = "zxcnmvfjdaqpirue";
  for(auto c : values) {
    bt.insert(c, (long)c);
  }

  std::ostringstream out;
  bt.print(out);
  std::sort(values.begin(), values.end());

  bt.print_tree();
  EXPECT_EQ(out.str(), values.c_str());
}
 

TEST_F(DiskBasedBtree, Persistence) {
  // Define constants
  const int page_size = 64;
  const int btree_order = 2; // GETORDER(page_size, sizeof(char));

  std::cout << "PAGE_SIZE: " << page_size << std::endl;
  std::cout << "BTREE_ORDER: " << btree_order << std::endl;

  // Create tree
  std::shared_ptr<pagemanager> pm = std::make_shared<pagemanager>("btree.index", page_size); // trunc file = false
  btree<char, btree_order> bt(pm);
  
  // Insert data into tree
  std::string values = "123456";
  for(auto c : values) {
    bt.insert(c, (long)c);
  }

  std::ostringstream out;
  bt.print(out);
  std::string all_values = "zxcnmvfjdaqpirue123456";
  std::sort(all_values.begin(), all_values.end());

  bt.print_tree();
  EXPECT_EQ(out.str(), all_values.c_str());
}

TEST_F(DiskBasedBtree, Dictionary) {
  index_records_using_btree_only_one_time();
  select(55, 101); // [55, 100>
}


