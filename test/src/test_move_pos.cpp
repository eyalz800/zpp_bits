#include "test.h"

#include <array>
#include <cstdint>
#include <vector>

namespace test_move_pos
{

namespace 
{
    struct A {
        std::int32_t alpha{};
        std::int64_t beta{};
        bool b1;
        bool b2;
        std::int64_t gamma{};
        
        constexpr static auto serialize(auto& archive, auto& self) {
            return archive(self.alpha, self.beta, self.b1, self.b2, self.gamma);
        }
    };


} // end anonymous namespace


TEST(move_pos, data_before_header)
{
    constexpr size_t headerSize = 16;
    std::array<char, headerSize> header;
    header.fill('H');
    EXPECT_EQ(header.size(), headerSize);

    const A dataObject {11, 12, true, false, 999999 };
    std::vector<std::uint8_t> sData;
    zpp::bits::out out(sData);

    constexpr size_t expectedSerializedDataSize 
      = sizeof(A::alpha) + sizeof(A::beta) + sizeof(A::gamma)
      + sizeof(A::b1) + sizeof(A::b2);

    constexpr auto expectedTotalMsgSize = headerSize + expectedSerializedDataSize;

    { // serialize data first...
        ASSERT_EQ(out.position(), 0U);
        ASSERT_EQ(out.position() += headerSize, headerSize);
        ASSERT_NO_THROW( out(dataObject).or_throw() );
    }
    ASSERT_EQ(sData.size(), expectedTotalMsgSize);

    { // .. then fixed header size at the beginning
        ASSERT_NO_THROW(out.reset());
        ASSERT_EQ(out.position(), 0U);
        ASSERT_NO_THROW(out(header).or_throw());
    }
    ASSERT_EQ(sData.size(), expectedTotalMsgSize);

    zpp::bits::in in(sData);
    std::array<char, headerSize> headerIn;
    ASSERT_NO_THROW(in(headerIn).or_throw());

    auto const headerInOk = std::all_of(headerIn.begin(), headerIn.end(),
    [](uint8_t ch){
        return ch == 'H';
    });

    EXPECT_TRUE(headerInOk);

    A inA;
    ASSERT_NO_THROW(in(inA).or_throw());
    EXPECT_EQ(inA.alpha, dataObject.alpha);
    EXPECT_EQ(inA.beta, dataObject.beta);
    EXPECT_EQ(inA.gamma, dataObject.gamma);
    EXPECT_EQ(inA.b1, dataObject.b1);
    EXPECT_EQ(inA.b2, dataObject.b2);
}

} // namespace test_move_pos
