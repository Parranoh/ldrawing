#include <iostream>
#include <vector>
#include "../src/include/cyclic_list.hpp"

int main(void)
{
    cyclic_list<size_t> a, b;
    auto print = [&]()
    {
        std::cout << "a:";
        for (size_t v : a)
            std::cout << ' ' << v;
        std::cout << "\nb:";
        for (size_t v : b)
            std::cout << ' ' << v;
        std::cout << std::endl;
    };

    a.push_back(1);
    auto two = a.push_back(2);
    a.push_back(3);
    auto five = a.push_back(5);
    a.insert(five, 4);
    a.push_back(6);
    print();

    b.splice(b.begin(), a, two, five);
    print();

    a.push_front(10);
    b.clear();
    print();

    b.splice(b.begin(), a, two, ++five);
    print();

    std::cout << "a, reversed:";
    for (auto it = a.begin(); it != a.end(); --it)
        std::cout << ' ' << *it;
    std::cout << std::endl;

    std::vector<int> vec = { -9, -8, -7, -6 };
    cyclic_list<int> c(vec.begin(), vec.end());
    std::cout << "c:";
    for (auto v : c)
        std::cout << ' ' << v;
    std::cout << std::endl;
    return 0;
}
