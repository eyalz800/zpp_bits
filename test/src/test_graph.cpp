#include "test.h"

namespace test_graph
{

struct node
{
    constexpr static auto serialize(auto & archive, auto & node)
    {
        return archive(node.value, node.nodes);
    }

    int value;
    std::vector<node> nodes;
};

TEST(test_graph, tree)
{
    auto [data, in, out] = zpp::bits::data_in_out();
    node o =
        {1,
            std::vector {
                node {
                    2,
                    std::vector {
                        node {
                            3,
                            std::vector {
                                node{4, {}}
                            }
                        }
                    }
                },
                node {5, {}}
            }
        };
    out(o).or_throw();

    node i;
    in(i).or_throw();

    EXPECT_EQ(o.value, i.value);
    EXPECT_EQ(o.nodes.size(), i.nodes.size());

    EXPECT_EQ(i.value, 1);
    ASSERT_EQ(i.nodes.size(), 2u);
    EXPECT_EQ(i.nodes[0].value, 2);
    ASSERT_EQ(i.nodes[0].nodes.size(), 1u);
    EXPECT_EQ(i.nodes[0].nodes[0].value, 3);
    ASSERT_EQ(i.nodes[0].nodes[0].nodes.size(), 1u);
    EXPECT_EQ(i.nodes[0].nodes[0].nodes[0].value, 4);
    EXPECT_EQ(i.nodes[0].nodes[0].nodes[0].nodes.size(), 0u);
    EXPECT_EQ(i.nodes[1].value, 5);
    ASSERT_EQ(i.nodes[1].nodes.size(), 0u);
}

} // namespace test_graph
