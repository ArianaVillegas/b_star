#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <utec/memory/bstar.h>
#include <fmt/core.h>

struct MemoryBasedBtree : public ::testing::Test
{
};

TEST_F(MemoryBasedBtree, TestA) {
    using namespace utec::memory;
    
    bstar<int> bt;
    std::string values = "zxcnmvafjdaqpirue";
    for(auto c : values) {
       bt.insert((int)c);
       bt.print();
    }
    bt.print();
}
 
 
