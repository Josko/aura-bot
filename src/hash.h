#include <string>

// compile time functions for char*

static constexpr uint_fast64_t HashCode(const char* str, const uint_fast64_t pos, const uint_fast64_t hash)
{
  return str[pos] ? (HashCode(str, pos + 1, 31 * hash + str[pos])) : hash;
}

static constexpr uint_fast64_t HashCode(const char* str)
{
  return HashCode(str, 0, 7);
}

// runtime function for std::string

static uint_fast64_t HashCode(const std::string& str)
{
  return HashCode(str.c_str());
}
