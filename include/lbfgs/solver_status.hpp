#ifndef LBFGS_SOLVER_STATUS_HPP
#define LBFGS_SOLVER_STATUS_HPP

#include "solver_error.hpp"

namespace lbfgs
{

enum class solver_convergence : uint8_t
{
    converged = 0, // convergence reached (g_epsilon)
    stopped,       // stoping criterion satisfied (past f decrease less than delta)
    canceled,      // canceled by user
};

struct solver_status
{
    solver_convergence convergence{};
    solver_error error{};
    double initial_cost{};
    double final_cost{};
    int iterations{};
    int line_search_iterations{};
};

}  // namespace lbfgs

#endif
