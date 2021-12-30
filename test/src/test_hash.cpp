#include "test.h"

namespace test_hash
{
using namespace zpp::bits::literals;
using namespace std::literals;

static_assert(""_sha1 ==
              "da39a3ee5e6b4b0d3255bfef95601890afd80709"_decode_hex);
static_assert(
    ""_sha256 ==
    "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"_decode_hex);

static_assert("The quick brown fox jumps over the lazy dog"_sha1 ==
              "2fd4e1c67a2d28fced849ee1bb76e7391b93eb12"_decode_hex);

static_assert(
    "The quick brown fox jumps over the lazy dog"_sha256 ==
    "d7a8fbb307d7809469ca9abcb0082e4f8d5651e46d3cdb762d02d0bf37c9e592"_decode_hex);
} // namespace test_hash
