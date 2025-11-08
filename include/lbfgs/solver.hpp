#ifndef LBFGS_SOLVER_HPP
#define LBFGS_SOLVER_HPP

#include <algorithm>
#include <concepts>
#include <exception>
#include <iostream>
#include <limits>
#include <ostream>

#include <Eigen/Dense>
#include <tl/expected.hpp>

#include "solver_error.hpp"
#include "solver_status.hpp"

namespace lbfgs
{
namespace detail
{

template<typename T>
struct lbfgs_iter_state
{
    using scalar_t = T;
    using vector_t = Eigen::Matrix<T, -1, 1>;
    using matrix_t = Eigen::Matrix<T, -1, -1>;

    scalar_t f;  // function value
    vector_t x;  // parameters
    vector_t g;  // gradient

    lbfgs_iter_state() = default;

    explicit lbfgs_iter_state(int64_t n, T v = std::numeric_limits<T>::quiet_NaN())
        : f {v}
        , x {vector_t(n)}
        , g {vector_t(n)}
    {
    }

    template<typename Functor>
    auto update(Functor const& functor, vector_t x_new)
    {
        x = std::move(x_new);
        f = functor(x, g);
    }

    template<typename Functor>
    auto update_with_gradient(Functor const& functor, vector_t x_new)
    {
        x = std::move(x_new);
        f = functor(x, g);
    }

    auto xnorm_inf() const { return x.cwiseAbs().maxCoeff(); }
    auto gnorm_inf() const { return g.cwiseAbs().maxCoeff(); }

    auto converged(scalar_t eps) const { return gnorm_inf() / std::max(scalar_t {1.0}, xnorm_inf()) <= eps; }

    friend auto operator<<(std::ostream& os, lbfgs_iter_state const& state) -> std::ostream&
    {
        os << "f = " << state.f << "\n"
           << "x = " << state.x.transpose() << "\n"
           << "g = " << state.g.transpose();
        return os;
    }
};

template<typename T>
struct lbfgs_memory
{
    using vector_t = Eigen::Matrix<T, -1, 1>;
    using matrix_t = Eigen::Matrix<T, -1, -1>;

    vector_t alpha;
    matrix_t s;
    matrix_t y;
    vector_t ys;

    lbfgs_memory() = default;

    lbfgs_memory(int64_t n, int64_t m)
        : alpha(vector_t::Zero(m))
        , s(matrix_t::Zero(n, m))
        , y(matrix_t::Zero(n, m))
        , ys(vector_t::Zero(m))
    {
    }
};
}  // namespace detail

template<typename Functor>
struct solver
{
    using scalar_t = typename Functor::scalar_t;
    using vector_t = Eigen::Matrix<scalar_t, -1, 1>;
    using vector_ref_t = Eigen::Ref<vector_t>;
    using vector_cref_t = Eigen::Ref<vector_t const> const&;
    using matrix_t = Eigen::Matrix<scalar_t, -1, -1>;

    explicit solver(Functor const& functor)
        : functor_(functor)
    {
    }

    /**
     * The number of corrections to approximate the inverse hessian matrix.
     *  The L-BFGS routine stores the computation results of previous m
     *  iterations to approximate the inverse hessian matrix of the current
     *  iteration. This parameter controls the size of the limited memories
     *  (corrections). The default value is 8. Values less than 3 are
     *  not recommended. Large values will result in excessive computing time.
     */
    int mem_size {8};

    /**
     * Epsilon for grad convergence test. DO NOT USE IT in nonsmooth cases!
     *  Set it to 0.0 and use past-delta-based test for nonsmooth functions.
     *  This parameter determines the accuracy with which the solution is to
     *  be found. A minimization terminates when
     *      ||g(x)||_inf / max(1, ||x||_inf) < g_epsilon,
     *  where ||.||_inf is the infinity norm. The default value is 1.0e-5.
     *  This should be greater than 1.0e-6 in practice because L-BFGS does
     *  not directly reduce first-order residual. It still needs the function
     *  value which can be corrupted by machine_prec when ||g|| is small.
     */
    scalar_t g_epsilon {1.0e-5};

    /**
     * distance for delta-based convergence test.
     *  this parameter determines the distance, in iterations, to compute
     *  the rate of decrease of the cost function. if the value of this
     *  parameter is zero, the library does not perform the delta-based
     *  convergence test. the default value is 3.
     */
    int past {3};

    /**
     * Delta for convergence test.
     *  This parameter determines the minimum rate of decrease of the
     *  cost function. The library stops iterations when the following
     *  condition is met:
     *      |f' - f| / max(1, |f|) < delta,
     *  where f' is the cost value of past iterations ago, and f is
     *  the cost value of the current iteration.
     *  The default value is 1.0e-6.
     */
    scalar_t delta {1.0e-6};

    /**
     * The maximum number of iterations.
     *  The lbfgs_optimize() function terminates an minimization process with
     *  ::LBFGSERR_MAXIMUMITERATION status code when the iteration count
     *  exceedes this parameter. Setting this parameter to zero continues an
     *  minimization process until a convergence or error. The default value
     *  is 0.
     */
    int max_iterations {0};

    /**
     * The maximum number of trials for the line search.
     *  This parameter controls the number of function and gradients evaluations
     *  per iteration for the line search routine. The default value is 64.
     */
    int max_line_search_iterations {64};

    /**
     * The minimum step of the line search routine.
     *  The default value is 1.0e-20. This value need not be modified unless
     *  the exponents are too large for the machine being used, or unless the
     *  problem is extremely badly scaled (in which case the exponents should
     *  be increased).
     */
    scalar_t min_step {1.0e-20};

    /**
     * The maximum step of the line search.
     *  The default value is 1.0e+20. This value need not be modified unless
     *  the exponents are too large for the machine being used, or unless the
     *  problem is extremely badly scaled (in which case the exponents should
     *  be increased).
     */
    scalar_t max_step {1.0e+20};

    /**
     * A parameter to control the accuracy of the line search routine.
     *  The default value is 1.0e-4. This parameter should be greater
     *  than zero and smaller than 1.0.
     */
    scalar_t f_dec_coeff {1.0e-4};

    /**
     * A parameter to control the accuracy of the line search routine.
     *  The default value is 0.9. If the function and gradient
     *  evaluations are inexpensive with respect to the cost of the
     *  iteration (which is sometimes the case when solving very large
     *  problems) it may be advantageous to set this parameter to a small
     *  value. A typical small value is 0.1. This parameter should be
     *  greater than the f_dec_coeff parameter and smaller than 1.0.
     */
    scalar_t s_curv_coeff {0.9};

    /**
     * A parameter to ensure the global convergence for nonconvex functions.
     *  The default value is 1.0e-6. The parameter performs the so called
     *  cautious update for L-BFGS, especially when the convergence is
     *  not sufficient. The parameter must be positive but might as well
     *  be less than 1.0e-3 in practice.
     */
    scalar_t cautious_factor {1.0e-6};

    /**
     * The machine precision for floating-point values. The default is 1.0e-16.
     *  This parameter must be a positive value set by a client program to
     *  estimate the machine precision.
     */
    scalar_t machine_prec {std::numeric_limits<scalar_t>::epsilon()};

    using lbfgs_iter_state = std::tuple<vector_ref_t, vector_ref_t>;

    /**
     * Line search method for smooth or nonsmooth functions.
     *  This function performs line search to find a point that satisfy
     *  both the Armijo condition and the weak Wolfe condition. It is
     *  as robust as the backtracking line search but further applies
     *  to continuous and piecewise smooth functions where the strong
     *  Wolfe condition usually does not hold.
     *
     *  @see
     *      Adrian S. Lewis and Michael L. Overton. Nonsmooth optimization
     *      via quasi-Newton methods. Mathematical Programming, Vol 141,
     *      No 1, pp. 135-163, 2013.
     */
    auto line_search(scalar_t& step) const noexcept -> solver_error
    {
        /* Check the input parameters for errors. */
        if (!(step > scalar_t {0.0})) {
            return solver_error::invalid_parameters;
        }

        /* Compute the initial gradient in the search direction. */
        auto dginit = prev_.g.dot(dir_);
        /* Make sure that s points to a descent direction. */
        if (scalar_t {0.0} < dginit) {
            return solver_error::increase_gradient;
        }

        // while (true)
        bool touched = false;
        auto mu = scalar_t {0.0};
        auto nu = max_step;

        auto finit = curr_.f;
        auto brackt = false;
        for (auto iter = 1;; ++iter) {
            /* Evaluate the function and gradient values. */
            curr_.update(functor_, prev_.x + step * dir_);

            /* Test for errors. */
            if (!std::isfinite(curr_.f)) {
                return solver_error::invalid_function_value;
            }

            /* Check the Armijo condition. */
            if (curr_.f > finit + step * f_dec_coeff * dginit) {
                nu = step;
                brackt = true;
            } else {
                /* Check the weak Wolfe condition. */
                if (curr_.g.dot(dir_) < s_curv_coeff * dginit) {
                    mu = step;
                } else {
                    break;
                }
            }
            if (max_line_search_iterations <= iter) {
                /* Maximum number of iteration. */
                return solver_error::max_line_search_iterations_reached;
            }

            if (brackt && (nu - mu) < machine_prec * nu) {
                /* Relative interval width is at least machine_prec. */
                return solver_error::width_too_small;
            }

            step = brackt ? scalar_t {0.5} * (mu + nu) : step * scalar_t {2.0};

            if (step < min_step) {
                return solver_error::min_step_too_small;
            } /* The step is the minimum value. */
            if (step > max_step) {
                if (touched) {
                    return solver_error::max_step_too_large;
                } /* The step is the maximum value. */
                touched = true; /* The maximum value should be tried once. */
                step = max_step;
            }
        }

        return solver_error::success;  // success
    }

    auto check_parameters() const noexcept -> solver_error
    {
        /* Check the input parameters for errors. */
        if (mem_size <= 0) {
            return solver_error::invalid_memsize;
        }
        if (g_epsilon < 0.0) {
            return solver_error::invalid_gepsilon;
        }
        if (past < 0) {
            return solver_error::invalid_testperiod;
        }
        if (delta < 0.0) {
            return solver_error::invalid_delta;
        }
        if (min_step < 0.0) {
            return solver_error::invalid_minstep;
        }
        if (max_step < min_step) {
            return solver_error::invalid_maxstep;
        }
        if (!(f_dec_coeff > 0.0 && f_dec_coeff < 1.0)) {
            return solver_error::invalid_fdeccoeff;
        }
        if (!(s_curv_coeff < 1.0 && s_curv_coeff > f_dec_coeff)) {
            return solver_error::invalid_scurvcoeff;
        }
        if (!(machine_prec > 0.0)) {
            return solver_error::invalid_machineprec;
        }
        if (max_line_search_iterations <= 0) {
            return solver_error::invalid_maxlinesearch;
        }
        return solver_error::success;
    }

    auto init(int n) const
    {
        curr_ = detail::lbfgs_iter_state<scalar_t>(n);
        prev_ = curr_;

        pf_.resize(std::max(1, past));
        dir_.resize(n);

        lmem_ = detail::lbfgs_memory<scalar_t>(n, mem_size);
    }

    auto optimize(vector_cref_t x0) const noexcept -> tl::expected<vector_t, solver_error>
    {
        if (status.error = check_parameters(); status.error != solver_error::success) {
            return tl::make_unexpected(status.error);
        }

        if (!(x0.size() > 0)) {
            return tl::make_unexpected(solver_error::invalid_n);
        }

        init(x0.size());

        /* Evaluate the function value and its gradient. */
        curr_.update(functor_, x0);
        status.initial_cost = curr_.f;

        /* Store the initial value of the cost function. */
        pf_(0) = curr_.f;

        /*
        Compute the direction;
        we assume the initial hessian matrix H_0 as the identity matrix.
        */
        dir_ = -curr_.g;

        /*
        Make sure that the initial variables are not a stationary point.
        */
        if (curr_.converged(g_epsilon)) {
            /* The initial guess is already a stationary point. */
            return curr_.x;
        }
        /*
           Compute the initial step:
           */
        auto step = scalar_t {1.0} / dir_.norm();

        auto end = 0;
        auto bound = 0;

        for (auto iter = 1;; ++iter) {
            /* Store the current position and gradient vectors. */
            prev_ = curr_;

            /* If the step bound can be provided dynamically, then apply it. */
            step = step < max_step ? step : 0.5 * max_step;

            /* Search for an optimal step.
             * x is passed by reference and will be updated inside the function
             * */

            if (auto res = line_search(step); res != solver_error::success) {
                /* Revert to the previous point. */
                std::swap(curr_, prev_);
                break;
            }

            /*
             * Convergence test.
             * The criterion is given by the following formula:
             * ||g(x)||_inf / max(1, ||x||_inf) < g_epsilon
             * */
            if (curr_.converged(g_epsilon)) {
                status.convergence = solver_convergence::converged;
                status.final_cost = curr_.f;
                return curr_.x;
            }

            /*
             * Test for stopping criterion.
             * The criterion is given by the following formula:
             * |f(past_x) - f(x)| / max(1, |f(x)|) < \delta.
             * */
            if (0 < past) {
                /* We don't test the stopping criterion while k < past. */
                if (past <= iter) {
                    /* The stopping criterion. */
                    auto rate = std::abs(pf_(iter % past) - curr_.f) / std::max(scalar_t {1.0}, std::abs(curr_.f));

                    if (rate < delta) {
                        status.convergence = solver_convergence::stopped;
                        status.final_cost = curr_.f;
                        return curr_.x;
                    }
                }

                /* Store the current value of the cost function. */
                pf_(iter % past) = curr_.f;
            }

            if (max_iterations != 0 && max_iterations <= iter) {
                /* Maximum number of iterations. */
                status.error = solver_error::max_solver_iterations_reached;
                break;
            }

            /* Count the iteration number. */

            /*
             * Update vectors s and y:
             * s_{k+1} = x_{k+1} - x_{k} = \step * d_{k}.
             * y_{k+1} = g_{k+1} - g_{k}.
             * */
            lmem_.s.col(end) = curr_.x - prev_.x;
            lmem_.y.col(end) = curr_.g - prev_.g;

            /*
               Compute scalars ys and yy:
               ys = y^t \cdot s = 1 / \rho.
               yy = y^t \cdot y.
               Notice that yy is used for scaling the hessian matrix H_0
               (Cholesky factor).
               */
            auto ys = lmem_.y.col(end).dot(lmem_.s.col(end));
            auto yy = lmem_.y.col(end).squaredNorm();
            lmem_.ys(end) = ys;

            /* Compute the negative of gradients. */
            dir_ = -curr_.g;

            /*
               Only cautious update is performed here as long as
               (y^t \cdot s) / ||s_{k+1}||^2 > \epsilon * ||g_{k}||^\alpha,
               where \epsilon is the cautious factor and a proposed value
               for \alpha is 1.
               This is not for enforcing the PD of the approxomated Hessian
               since ys > 0 is already ensured by the weak Wolfe condition.
               This is to ensure the global convergence as described in:
               Dong-Hui Li and Masao Fukushima. On the global convergence of
               the BFGS method for nonconvex unconstrained optimization
               problems. SIAM Journal on Optimization, Vol 11, No 4, pp.
               1054-1064, 2011.
               */
            auto cau = lmem_.s.col(end).squaredNorm() * prev_.g.norm() * cautious_factor;

            if (ys > cau) {
                /*
                   Recursive formula to compute dir = -(H \cdot g).
                   This is described in page 779 of:
                   Jorge Nocedal.
                   Updating Quasi-Newton Matrices with Limited Storage.
                   Mathematics of Computation, Vol. 35, No. 151,
                   pp. 773--782, 1980.
                   */
                auto const m = mem_size;
                ++bound;
                bound = m < bound ? m : bound;
                end = (end + 1) % m;

                auto j = end;
                for (auto i = 0; i < bound; ++i) {
                    j = (j + m - 1) % m; /* if (--j == -1) j = m-1; */
                    /* \alpha_{j} = \rho_{j} s^{t}_{j} \cdot q_{k+1}. */
                    lmem_.alpha(j) = lmem_.s.col(j).dot(dir_) / lmem_.ys(j);
                    /* q_{i} = q_{i+1} - \alpha_{i} y_{i}. */
                    dir_ += (-lmem_.alpha(j)) * lmem_.y.col(j);
                }

                dir_ *= ys / yy;

                for (auto i = 0; i < bound; ++i) {
                    /* \beta_{j} = \rho_{j} y^t_{j} \cdot \gamm_{i}. */
                    auto beta = lmem_.y.col(j).dot(dir_) / lmem_.ys(j);
                    /* \gamm_{i+1} = \gamm_{i} + (\alpha_{j} - \beta_{j}) s_{j}.
                     */
                    dir_ += (lmem_.alpha(j) - beta) * lmem_.s.col(j);
                    j = (j + 1) % m; /* if (++j == m) j = 0; */
                }
            }

            /* The search direction d is ready. We try step = 1 first. */
            step = 1.0;
        }  // end while

        /* Return the final value of the cost function. */
        status.final_cost = curr_.f;
        return curr_.x;
    }

    mutable solver_status status {};

    auto current_step_state() const { return curr_; }
    auto previous_step_state() const { return prev_; }

private:
    std::reference_wrapper<Functor const> functor_;  // the function to be minimized
    mutable detail::lbfgs_iter_state<scalar_t> curr_;  // state at current step (function value, parameters, gradient)
    mutable detail::lbfgs_iter_state<scalar_t> prev_;  // previous step state
    mutable vector_t pf_;  // past function values
    mutable vector_t dir_;  // step direction
    mutable detail::lbfgs_memory<scalar_t> lmem_;  // limited memory
};

}  // namespace lbfgs

#endif
