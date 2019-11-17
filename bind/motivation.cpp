#include <functional>
#include <map>
#include <string>

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