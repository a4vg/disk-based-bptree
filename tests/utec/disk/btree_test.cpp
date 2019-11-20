
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
  bool trunc_file = true;
  const int page_size = 1024;
  const int btree_order = 82;
  std::shared_ptr<pagemanager> pm = std::make_shared<pagemanager>("btree.index", page_size, trunc_file);
  btree< long, btree_order> bt(pm);
  
  pagemanager record_manager ("students.bin", page_size, trunc_file);

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

// select * from Student where 50 <= id and id <= 100 
void select(){
  const int page_size = 1024;
  const int btree_order = 82;
  pagemanager record_manager ("students.bin", page_size);

  std::shared_ptr<pagemanager> pm = std::make_shared<pagemanager>("btree.index", page_size);
  btree< long, btree_order> bt(pm);

  auto iter = bt.range_search(3, 100); // range_search(x, y) = [x, y>
  while (iter != iter.limit()){
    Student s;
    record_manager.recover(*iter, s);
    std::cout << s.id << " " << s.passed << " " << s.name << " " << s.surname << " " << s.n_credits << std::endl;
    iter++;
  }
}

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

TEST_F(DiskBasedBtree, Dictionary) {
  index_records_using_btree_only_one_time();
  select();
}


