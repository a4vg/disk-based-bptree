
// #include <utecdf/column/column.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <utec/disk/btree.h>
#include <utec/disk/pagemanager.h>

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

TEST_F(DiskBasedBtree, TestA) {
    std::shared_ptr<pagemanager> pm = std::make_shared<pagemanager>("btree.index", PAGE_SIZE);
    std::cout << "PAGE_SIZE: " << PAGE_SIZE << std::endl;
    std::cout << "BTREE_ORDER: " << BTREE_ORDER << std::endl;
    btree<char, BTREE_ORDER> bt(pm);
    std::string values = "zxcnmvafjdaqpirue";
    for(auto c : values) {
       bt.insert(c, (int)c);
       bt.print();
    }
    bt.print();
}

TEST_F(DiskBasedBtree, TestB) {
    std::shared_ptr<pagemanager> pm = std::make_shared<pagemanager>("btree.index", PAGE_SIZE);
    btree<char, BTREE_ORDER> bt(pm);
    std::string values = "iwrvjsptcgzqknleoyuxfadmh";
    for(auto c : values) {
       bt.insert(c, (int)c);
    }

    auto pair_find = bt.find('b');
    bool found = pair_find.first;
    btree<char, BTREE_ORDER>::iterator it = pair_find.second;

    std::cout << "Found 'b'?: " << found << std::endl;
    for (int i=0; it!=bt.end(); it++, ++i){
        std::cout << i << "-" << *it << " ";
    }
    std::cout << std::endl;
}

TEST_F(DiskBasedBtree, TestC) {
    std::shared_ptr<pagemanager> pm = std::make_shared<pagemanager>("btree.index", PAGE_SIZE);
    btree<char, BTREE_ORDER> bt(pm);
    std::string values = "zxcnmvafjdaqpirue";
    for(auto c : values) {
       bt.insert(c, (int)c);
    }

    std::cout << "From 'd' to 's'" << std::endl;
    btree<char, BTREE_ORDER>::iterator it = bt.range_search('d', 's');
    for (int i=0; it!=it.limit(); it++, ++i){
        std::cout << i << "-" << *it << " ";
    }
    std::cout << std::endl;
}
 