#include "StringUtil.hpp"

#include <cstring>

auto HasSuffix(std::string const &str, std::string const &suffix) -> bool {
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

auto HasPrefix(const char *str, const char *pre) -> bool {
    return strncmp(pre, str, strlen(pre)) == 0;
}
