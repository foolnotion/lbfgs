#include <iomanip>

#include <nano/function/util.h>
#include <nano/logger.h>
#include <nano/solver.h>
#include <nano/tensor.h>

#include "lbfgs/solver.hpp"

struct rosenbrock_function final : public nano::function_t
{
    using scalar_t = nano::scalar_t;
    using vector_t = nano::vector_t;

    explicit rosenbrock_function(int dim)
        : nano::function_t("rosenbrock", dim)
    {
        convex(nano::convexity::no);
        smooth(nano::smoothness::no);
    }

    auto clone() const -> nano::rfunction_t final { return std::make_unique<rosenbrock_function>(*this); }

    auto do_eval(eval_t eval) const -> nano::scalar_t final {
        return (*this)(eval.m_x.data(), eval.m_gx.data(), eval.m_x.size());
    }

    auto operator()(nano::scalar_t const* x, nano::scalar_t* gx, int64_t size) const noexcept -> nano::scalar_t {
        auto const n = size;

        nano::scalar_t f {0.0};
        for (auto i = 0L; i < n - 1; ++i) {
            auto const u = x[i];
            auto const v = x[i+1];
            auto const w = u*u-v;

            f += 100 * w*w + (u-1)*(u-1);
        }

        if (gx != nullptr) {
            for (auto i = 0L; i < n - 1; ++i) {
                auto const u = x[i];
                auto const v = x[i+1];

                if (i == 0) {
                    *(gx+i) = 400 * u * (u*u - v) + 2 * (u-1);
                } else {
                    auto const w = x[i-1];
                    *(gx+i) = -200 * (w*w - u) + 400 * (u*u - v) + 2 * (u-1);
                }
            }
            if (n > 1) {
                *(gx+n-1) = -200 * (x[n-2] * x[n-2] - x[n-1]);
            }
        }
        return f;
    }

    auto do_vgrad(nano::vector_t const& x, nano::vector_t* gx) const noexcept -> nano::scalar_t
    {
        auto* p = gx != nullptr ? gx->data() : nullptr;
        return (*this)(x.data(), p, x.size());
    }

    auto operator()(nano::vector_t const& x) const noexcept -> nano::scalar_t
    {
        return (*this)(x.data(), nullptr, x.size());
    }

    auto operator()(Eigen::Ref<Eigen::Matrix<nano::scalar_t, -1, 1>> const x, Eigen::Ref<Eigen::Matrix<nano::scalar_t, -1, 1>> gx) const noexcept -> nano::scalar_t {
        return (*this)(x.data(), gx.data(), x.size());
    }

    auto operator()(nano::vector_t const& x, nano::vector_t& gx) const noexcept -> nano::scalar_t {
        return do_vgrad(x, &gx);
    }
};

auto solve_libnano(auto function, auto x0) {
    using nano::make_random_vector;
    using nano::scalar_t;
    using nano::vector_t;

    auto solver = nano::solver_t::all().get("lbfgs");
    solver->parameter("solver::lbfgs::history") = 6;
    solver->parameter("solver::epsilon") = 1e-6;
    solver->parameter("solver::max_evals") = 100;
    solver->parameter("solver::tolerance") = std::make_tuple(1e-4, 9e-1);
    solver->lsearch0("constant");
    solver->lsearchk("morethuente");


    const auto state = solver->minimize(function, x0, nano::make_null_logger());
    return state.x();
}

namespace {
auto solve_lbfgs(auto const& function, nano::vector_t x0) {
    lbfgs::solver<rosenbrock_function> solver(function);

    Eigen::Map<Eigen::Matrix<nano::scalar_t, -1, 1> const> r0(x0.data(), x0.size());
    if (auto res = solver.optimize(r0)) {
        x0 = res.value();
    }
    return x0; // optimization failed
}
} // namespace

auto main() -> int
{
    auto constexpr dim{3};
    rosenbrock_function function(dim);

    auto n1{0};
    auto n2{0};

    for (auto i = 0; i < 1000; ++i) {
        nano::vector_t x0 = nano::make_random_vector<nano::scalar_t>(dim);

        auto a = solve_libnano(function, x0);
        auto b = solve_lbfgs(function, x0);

        n1 += std::abs(function(a)) < 1e-6;
        n2 += std::abs(function(b)) < 1e-6;
    }

    std::cout << "libnano : minimum found " << n1 << " times\n";
    std::cout << "liblbfgs: minimum found " << n2 << " times\n";

    return 0;
}
