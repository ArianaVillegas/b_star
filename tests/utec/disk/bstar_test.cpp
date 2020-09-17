
// #include <utecdf/column/column.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <utec/disk/bstar.h>
#include <utec/disk/pagemanager.h>

#include <fmt/core.h>

// PAGE_SIZE 64 bytes
#define PAGE_SIZE  128

// Other examples:
// PAGE_SIZE 1024 bytes => 1Kb
// PAGE_SIZE 1024*1024 bytes => 1Mb

// PAGE_SIZE = 2 * sizeof(long) +  (BSTAR_ORDER + 1) * sizeof(int) + (BSTAR_ORDER + 2) * sizeof(long)  
// PAGE_SIZE = 2 * sizeof(long) +  (BSTAR_ORDER) * sizeof(int) + sizeof(int) + (BSTAR_ORDER) * sizeof(long) + 2 * sizeof(long)
// PAGE_SIZE = (BSTAR_ORDER) * (sizeof(int) + sizeof(long))  + 2 * sizeof(long) + sizeof(int) +  2 * sizeof(long)
//  BSTAR_ORDER = PAGE_SIZE - (2 * sizeof(long) + sizeof(int) +  2 * sizeof(long)) /  (sizeof(int) + sizeof(long))

#define BSTAR_ORDER  ((PAGE_SIZE - (2 * sizeof(long) + sizeof(int) +  2 * sizeof(long)) ) /  (sizeof(int) + sizeof(long)))


struct DiskBasedBstar : public ::testing::Test
{
};
using namespace utec::disk;

TEST_F(DiskBasedBstar, IndexingRandomElements) {
  bool trunc_file = true;
  std::shared_ptr<pagemanager> pm = std::make_shared<pagemanager>("bstar.index", trunc_file);
  std::cout << "PAGE_SIZE: " << PAGE_SIZE << std::endl;
  std::cout << "BSTAR_ORDER: " << BSTAR_ORDER << std::endl;
  bstar<char, BSTAR_ORDER> bt(pm);
  std::string values = "zxcnmvfjdaqpirue";
  for(auto c : values) {
    bt.insert(c);
  }
  bt.print_tree();
  std::ostringstream out;
  bt.print(out);
  std::sort(values.begin(), values.end());
  EXPECT_EQ(out.str(), values.c_str());
}
 
TEST_F(DiskBasedBstar, Persistence) {
  std::shared_ptr<pagemanager> pm = std::make_shared<pagemanager>("bstar.index");
  bstar<char, BSTAR_ORDER> bt(pm);
  std::string values = "123456";
  for(auto c : values) {
    bt.insert(c);
  }
  bt.print_tree();

  std::ostringstream out;
  bt.print(out);
  std::string all_values = "zxcnmvfjdaqpirue123456";
  std::sort(all_values.begin(), all_values.end());
  EXPECT_EQ(out.str(), all_values.c_str());
}


TEST_F(DiskBasedBstar, Iterators) {
  std::shared_ptr<pagemanager> pm = std::make_shared<pagemanager>("bstar.index");
  using char_bstar = bstar<char, BSTAR_ORDER>;
  char_bstar bt(pm);
// TODO:
//  char_bstar::iterator iter =  bt.find('a');
//  for( ; iter != bt.end(); iter++) {
//    std::cout << *iter << ", ";
//  }
}

TEST_F(DiskBasedBstar, Scalability) {
  std::shared_ptr<pagemanager> pm = std::make_shared<pagemanager>("bstar.index");
  bstar<int, BSTAR_ORDER> bt(pm);
  std::fstream random_file;
  random_file.open("random.txt");
  long n;
  while (random_file >> n) {
    bt.insert(c);
  }
}
