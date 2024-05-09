#include <iostream>
#include <vector>
#include <gtest/gtest.h>

#include "field/M31.hpp"
#include "poly_commit/merkle.hpp"

TEST(MERKLE_TREE_TESTS, MERKLE_TREE_CORRECTNESS)
{
    using F = gkr::M31_field::M31;
    using namespace gkr;
    using merkle_tree::MerkleTree;
    using merkle_tree::Proof;

    uint32_t test_size = 4;
    std::vector<F> leaves;
    leaves.reserve(test_size);
    for (uint32_t i = 0; i < test_size; i++)
    {
        leaves.emplace_back(F::random());
    }
    MerkleTree tree = MerkleTree::build_tree<F>(leaves);
    for (uint32_t i = 0; i < test_size; i++)
    {
        uint32_t idx = rand() % test_size;
        Proof proof = tree.prove(idx);
        EXPECT_TRUE(MerkleTree::verify(tree.root(), idx, leaves[idx], proof));    
    }
}