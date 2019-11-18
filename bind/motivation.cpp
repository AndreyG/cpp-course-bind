#include <functional>
#include <map>
#include <string>
#include <memory>

struct Mapper
{
    std::string&       operator()(int i, int j)       { return mapping_[{i, j}]; }
    std::string const& operator()(int i, int j) const { return mapping_.at({ i, j }); }

private:
    std::map<std::pair<int, int>, std::string> mapping_;
};

int main()
{
    auto binder = std::bind(Mapper(), 10, std::placeholders::_1);
    binder(10) = "aba";
}

namespace
{
    using namespace std;

    void f(unique_ptr<int>, unique_ptr<int>);

    struct OneShotCallback
    {
        unique_ptr<int> ptr1;

        void operator() (unique_ptr<int> ptr2) &&
        {
            f(move(ptr1), move(ptr2));
        }
    };

    void test()
    {
        auto lambda = [c = OneShotCallback(), ptr = make_unique<int>()] () mutable
        {
            return std::move(c)(std::move(ptr));
        };
        //auto binder = bind(Callable(), make_unique<int>());
        //move(binder)();
    }
}
