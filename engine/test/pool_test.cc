#include "engine/pool.hh"

#include <gtest/gtest.h>

namespace engine {
TEST(ListObjectPool, basic) {
    ListObjectPool<int> list;
    ASSERT_TRUE(list.empty());

    std::vector<ObjectId> ids;
    for (int i = 100; i < 110; ++i) {
        auto [id, object] = list.add(i);
        EXPECT_EQ(object, i);
        ids.emplace_back(id);
    }
    ASSERT_FALSE(list.empty());

    // Make sure none of the IDs are the same
    for (size_t i = 0; i < ids.size(); ++i)
        for (size_t j = 0; j < ids.size(); ++j) {
            if (i == j)
                EXPECT_EQ(ids[i].key, ids[j].key);
            else
                EXPECT_NE(ids[i].key, ids[j].key);
        }

    for (size_t i = 0; i < ids.size(); ++i) {
        EXPECT_EQ(list.get(ids[i]), 100 + i);
    }
}

TEST(ListObjectPool, iterate) {
    ListObjectPool<int> list;
    ASSERT_TRUE(list.empty());

    std::vector<ObjectId> ids;
    for (int i = 100; i < 110; ++i) {
        auto [id, object] = list.add(i);
        EXPECT_EQ(object, i);
        ids.emplace_back(id);
    }
    ASSERT_FALSE(list.empty());

    int i = 0;
    for (auto element : list.iterate()) {
        EXPECT_EQ(*element, 100 + i);
        i++;
    }
}

}  // namespace engine
