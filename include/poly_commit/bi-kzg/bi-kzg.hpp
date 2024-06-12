
#include "field/utils.hpp"
#include "field/bn254_fr.hpp"
#include "poly_commit/poly.hpp"

#include "utils/myutil.hpp"

namespace gkr
{

using namespace mcl::bn; 
using FF = bn254fr::BN254_Fr;

struct BiKZGSetup
{
    std::vector<Fr> roots_of_unity_u;
    std::vector<Fr> roots_of_unity_v;
    
    std::vector<Fr> lag_denoms_u;
    std::vector<Fr> lag_denoms_v;
    
    std::vector<G1> g1_pow_lags_u;
    std::vector<G1> g1_pow_lags_v; 
    std::vector<G1> g1_pow_lags_bivariate;
    
    std::vector<G2> g2_pow_secrets;
};

struct BiKZGCommitment
{
    G1 c;
};

struct BiKZGOpening
{
    FF v;
    G1 g1_pow_q1, g1_pow_q2;
};

class BiKZG
{
public:
    void _roots_of_unity(size_t degree, std::vector<Fr> &output)
    {
        output.clear();
        uint32_t log_degree = log_2(degree);
        Fr root = get_primitive_root_of_unity<FF>(log_degree).mcl_data;
        
        output.resize(degree);
        output[0] = Fr::one();
        for (size_t i = 1; i < degree; i++)
        {
            Fr::mul(output[i], output[i - 1], root);
        }
    }

    void __lags_from_denoms(
        size_t degree,
        const Fr &x,
        const std::vector<Fr> &roots_of_unity,
        const std::vector<Fr> &lag_denoms,
        std::vector<Fr> &lag_terms
    )
    {
        Fr tmp;
        // lagrange terms
        Fr prod_of_lag_numerators = FF::ONE.mcl_data;
        for (size_t i = 0; i < degree; i++)
        {
            Fr::sub(tmp, x, roots_of_unity[i]);
            Fr::mul(prod_of_lag_numerators, prod_of_lag_numerators, tmp);
        }
        
        lag_terms.clear();
        lag_terms.resize(degree);
        for (size_t i = 0; i < degree; i++)
        {
            Fr::sub(tmp, x, roots_of_unity[i]);
            Fr::mul(tmp, tmp, lag_denoms[i]);
            Fr::div(lag_terms[i], prod_of_lag_numerators, tmp);
        }
    }

    // lagrange terms
    void _lags(
        size_t degree, 
        const Fr &secret,
        const std::vector<Fr> &roots_of_unity,
        std::vector<Fr> &lag_denoms,
        std::vector<Fr> &lag_terms
    )
    {
        Fr tmp;

        // denominator of lagrange terms
        lag_denoms.clear();
        lag_denoms.resize(degree, FF::ONE.mcl_data);
   
        for (size_t i = 0; i < degree; i++)
        {
            for (size_t j = 0; j < degree; j++)
            {
                if (i == j) continue;

                Fr::sub(tmp, roots_of_unity[i], roots_of_unity[j]);
                Fr::mul(lag_denoms[i], lag_denoms[i], tmp);
            }
        }

        __lags_from_denoms(degree, secret, roots_of_unity, lag_denoms, lag_terms);
    }

    void _g1_pow_lags(
        size_t deg_u,
        size_t deg_v,
        const std::vector<Fr> &lags_u,
        const std::vector<Fr> &lags_v,
        std::vector<G1> &g1_pow_lags_u,
        std::vector<G1> &g1_pow_lags_v,
        std::vector<G1> &g1_pow_lags_bivariate
    )
    {
        Fr tmp;

        g1_pow_lags_u.resize(deg_u);
        for (size_t i = 0; i < deg_u; i++)
        {
            G1::mul(g1_pow_lags_u[i], G1_ONE, lags_u[i]);
        }
        
        g1_pow_lags_v.resize(deg_v);
        for (size_t j = 0; j < deg_v; j++)
        {
            G1::mul(g1_pow_lags_v[j], G1_ONE, lags_v[j]);
        }
        
        g1_pow_lags_bivariate.resize(deg_u * deg_v);
        for (size_t j = 0; j < deg_v; j++)
        {
            for (size_t i = 0; i < deg_u; i++)
            {
                Fr::mul(tmp, lags_u[i], lags_v[j]);
                G1::mul(g1_pow_lags_bivariate[j * deg_u + i], G1_ONE, tmp);
            }
            
        }
    }

    void _g2_pow_secrets(
        const Fr &secret_u,
        const Fr &secret_v,
        std::vector<G2> &g2_pow_secrets
    )
    {
        g2_pow_secrets.resize(2);
        G2::mul(g2_pow_secrets[0], G2_ONE, secret_u);
        G2::mul(g2_pow_secrets[1], G2_ONE, secret_v);
    }

    void _multi_scalar_mul(const std::vector<G1> &base, const std::vector<Fr> &scalars, G1 &sum)
    {
        assert(base.size() == scalars.size());
        sum.clear();

        G1 tmp;
         for (size_t i = 0; i < base.size(); i++)
        {
            G1::mul(tmp, base[i], scalars[i]);
            G1::add(sum, sum, tmp);
        }
    }

    void _fast_multi_scalar_mul(const std::vector<G1> &base, const std::vector<Fr> &scalars, G1 &sum)
    {
        // Warning: here the base maybe normalized, but the value does not change
        G1::mulVec(sum, const_cast<G1*>(base.data()), scalars.data(), scalars.size());
    }

    void _eval(
        const BivariatePoly<FF> &poly, 
        const Fr &x, 
        const Fr &y,
        const std::vector<Fr> &roots_of_unity_u, 
        const std::vector<Fr> &roots_of_unity_v,
        const std::vector<Fr> &lag_denoms_u,
        const std::vector<Fr> &lag_denoms_v,
        // return value:
        Fr &v,
        std::vector<Fr> &lags_u, // l_u(x)
        std::vector<Fr> &lags_v, // l_v(y)
        std::vector<Fr> &partial_eval // evaluate u at x
    )
    {
        Fr tmp;
        v.clear();
        partial_eval.clear();
        partial_eval.resize(poly.deg_v, v); // set zero

        __lags_from_denoms(poly.deg_u, x, roots_of_unity_u, lag_denoms_u, lags_u);
        __lags_from_denoms(poly.deg_v, y, roots_of_unity_v, lag_denoms_v, lags_v);
        
        for (size_t j = 0; j < poly.deg_v; j++)
        {
            for (size_t i = 0; i < poly.deg_u; i++)
            {
                Fr::mul(tmp, lags_u[i], poly.get_evals(j * poly.deg_u + i).mcl_data);
                Fr::add(partial_eval[j], partial_eval[j], tmp);
            } 
            Fr::mul(tmp, partial_eval[j], lags_v[j]);
            Fr::add(v, v, tmp);
        }
        
    }

    void _multi_pairing(const std::vector<G1> &g1s, const std::vector<G2> &g2s, GT &gt)
    {
        assert(g1s.size() == g2s.size());
        millerLoopVec(gt, g1s.data(), g2s.data(), g1s.size());
        finalExp(gt, gt);
    }

public:
    size_t deg_u, deg_v;

    // assuming var_v is the least significant part
    void setup_gen(BiKZGSetup &setup, size_t deg_u_, size_t deg_v_, const FF &secret_u, const FF &secret_v)
    {
        deg_u = deg_u_;
        deg_v = deg_v_;
        
        _roots_of_unity(deg_u, setup.roots_of_unity_u);
        _roots_of_unity(deg_v, setup.roots_of_unity_v);
        
        std::vector<Fr> lag_terms_u, lag_terms_v;
        _lags(deg_u, secret_u.mcl_data, setup.roots_of_unity_u, setup.lag_denoms_u, lag_terms_u);
        _lags(deg_v, secret_v.mcl_data, setup.roots_of_unity_v, setup.lag_denoms_v, lag_terms_v);

        _g1_pow_lags(deg_u, deg_v, lag_terms_u, lag_terms_v, 
            setup.g1_pow_lags_u, setup.g1_pow_lags_v, setup.g1_pow_lags_bivariate);
        _g2_pow_secrets(secret_u.mcl_data, secret_v.mcl_data, setup.g2_pow_secrets);
    }

    void commit(BiKZGCommitment &commitment, const BivariatePoly<FF> &poly, const BiKZGSetup &setup)
    {
        assert(poly.deg_u == deg_u);
        assert(poly.deg_v == deg_v);

        std::vector<Fr> evals;
        evals.resize(deg_u * deg_v);
        for (size_t i = 0; i < deg_u * deg_v; i++)
        {
            evals[i] = poly.get_evals(i).mcl_data;
        }
        
        _fast_multi_scalar_mul(setup.g1_pow_lags_bivariate, evals, commitment.c);
    }

    void open(BiKZGOpening &opening, const BivariatePoly<FF> &poly, const FF &x, const FF &y, const BiKZGSetup &setup)
    {
        Fr tmp;
        std::vector<Fr> partial_evals, lags_u, lags_v;
        _eval(poly, x.mcl_data, y.mcl_data, setup.roots_of_unity_u, setup.roots_of_unity_v, setup.lag_denoms_u, setup.lag_denoms_v, 
            opening.v.mcl_data, lags_u, lags_v, partial_evals);
        
        // quotient of (Y - y)
        assert(partial_evals.size() == deg_v);
        std::vector<Fr> q2_evals(deg_v);
        for (size_t i = 0; i < deg_v; i++)
        {
            Fr::sub(q2_evals[i], partial_evals[i], opening.v.mcl_data);
            Fr::sub(tmp, setup.roots_of_unity_v[i], y.mcl_data);

            Fr::div(q2_evals[i], q2_evals[i], tmp);
        }
        _fast_multi_scalar_mul(setup.g1_pow_lags_v, q2_evals, opening.g1_pow_q2);
        
        // quotient of (X - x)
        std::vector<Fr> q1_evals;
        q1_evals.resize(deg_u * deg_v);

        for (size_t i = 0; i < deg_u; i++)
        {
            Fr::sub(tmp, setup.roots_of_unity_u[i], x.mcl_data);
            Fr::inv(tmp, tmp);
            for (size_t j = 0; j < deg_v; j++)
            {
                size_t idx = j * deg_u + i;
                Fr::sub(q1_evals[idx], poly.get_evals(idx).mcl_data, partial_evals[j]);
                Fr::mul(q1_evals[idx], q1_evals[idx], tmp);
            }
        }
        
        _fast_multi_scalar_mul(setup.g1_pow_lags_bivariate, q1_evals, opening.g1_pow_q1);
    }

    bool verify(BiKZGSetup &setup, BiKZGCommitment &commitment, const FF &x, const FF &y, BiKZGOpening &opening)
    {
        // trivial verification

        Fr tmp;
        std::vector<G1> g1s{3};
        std::vector<G2> g2s{3};

        G1::mul(g1s[0], G1_ONE, opening.v.mcl_data);
        G1::sub(g1s[0], g1s[0], commitment.c);
        g2s[0] = G2_ONE;

        g1s[1] = opening.g1_pow_q1;
        G2::mul(g2s[1], G2_ONE, x.mcl_data);
        G2::sub(g2s[1], setup.g2_pow_secrets[0], g2s[1]);

        g1s[2] = opening.g1_pow_q2;
        G2::mul(g2s[2], G2_ONE, y.mcl_data);
        G2::sub(g2s[2], setup.g2_pow_secrets[1], g2s[2]);

        GT verified;
        _multi_pairing(g1s, g2s, verified);
        return verified == GT_ONE;
    }
};
    
} // namespace gkr
