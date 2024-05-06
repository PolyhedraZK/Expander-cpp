#include <iostream>
#include <cstdlib>
#include <gtest/gtest.h>

#include "field/M31.hpp"

template <typename F>
void test_field_op()
{
    // Test 1: Addition multiplication
    F f = F::random();
    uint mul = rand() % 100;

    F prod_1 = f * F(mul);
    F prod_2 = 0;
    for (uint i = 0; i < mul; ++i)
    {
        prod_2 += f;
    }
    EXPECT_EQ(prod_1, prod_2);

    // Test 2: Inverse
    F f_inv = f.inv();
    EXPECT_EQ(f_inv * f, F::one());
}

template <typename F>
void test_serialization()
{
    F f = F(rand());
    uint8_t *bytes = (uint8_t *) malloc(sizeof(F)); 
    f.to_bytes(bytes);
    F ff;
    ff.from_bytes(bytes);
    EXPECT_EQ(f, ff);
}

TEST(FF_TESTS, MERSEN_EXT_OP_TESTS)
{
    srand(789);
    using F = gkr::M31_field::M31;
    test_field_op<F>();
}

TEST(FF_TESTS, MERSEN_EXT_SERIALIZE_TESTS)
{
    srand(159);
    using F = gkr::M31_field::M31;
    test_serialization<F>();
}

TEST(FF_TESTS, PACKED_MERSEN_EXT_OP_TESTS)
{
    srand(1234);
    using F = gkr::M31_field::VectorizedM31;
    test_field_op<F>();
}

TEST(FF_TESTS, PACKED_MERSEN_EXT_SERIALIZE_TESTS)
{
    srand(189);
    using F = gkr::M31_field::VectorizedM31;
    test_serialization<F>();
}

TEST(FF_TESTS, PACK_UNPACK_TEST)
{
    srand(753);
    using namespace gkr::M31_field;
    using F = gkr::M31_field::M31;
    using PackedF = gkr::M31_field::VectorizedM31;

    unsigned vector_size = 15;
    std::vector<F> field_elements(vector_size);
    for (unsigned i = 0; i < vector_size; i++)
    {
        field_elements[i] = i;
    }
    std::vector<PackedF> packed_field_elements = PackedF::pack_field_elements(field_elements);

    for (unsigned i = 0; i < vector_size; i++)
    {
        field_elements[i] = field_elements[i] * field_elements[i] + field_elements[i];
    }
    for (unsigned i = 0; i < (vector_size + PackedF::pack_size() - 1) / PackedF::pack_size(); i++)
    {
        packed_field_elements[i] = packed_field_elements[i] * packed_field_elements[i] + packed_field_elements[i];
    }
    std::vector<F> unpacked_field_elements = {};
    for (const auto & packed_field_element : packed_field_elements)
    {
        auto unpacked = packed_field_element.unpack();
        unpacked_field_elements.insert(unpacked_field_elements.end(), unpacked.begin(), unpacked.end());
    }

    bool same = true;
    for (unsigned i = 0; i < vector_size; i++)
    {
        same &= field_elements[i] == unpacked_field_elements[i];
    }

    EXPECT_TRUE(same);
}
