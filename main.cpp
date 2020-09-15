#include <random>
#include <sstream>
#include "src/utec/memory/bstar.h"
#include "src/utec/disk/bstar.h"

using namespace utec::disk;

int main() {

    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> r(1,100); // distribution in range [1, 6]

    auto pm = std::make_shared<pagemanager>("list.txt", true);
    bstar<int,7> tree(pm);
    for(int i=0; i<100; i++) {
        tree.insert(r(rng));
        //tree.print_tree();
    }
    tree.print_tree();

    return 0;
}
