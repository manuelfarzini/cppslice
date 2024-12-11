#include <cppslice.hpp>
#include <utils/test_classes.hpp>

void test_ctors() {
    Point p;
    std::vector<Point> pp = {p};

    Slice<Point> s1(p);
    Slice<Point> s2;
    Slice<Point> s3(pp);
    std::println("{}", s3[0]->x);

    OnlyCopyable cp1(0);
    OnlyMovable mv1(0);
    std::vector<OnlyCopyable> vcp = {cp1};
    std::vector<OnlyMovable> vmv;
    vmv.emplace_back(std::move(mv1));

    Slice<OnlyCopyable> s4(cp1);
    Slice<OnlyMovable> s5(mv1);
    Slice<OnlyCopyable> s6(vcp);
    Slice<OnlyMovable> s7(vmv);

    Slice<int> s8(1, 2, 3, 4, 5);
    Slice<int> s9{1, 2, 3, 4, 5};
}

int main() {
    test_ctors();
    return 0;
}
