#ifndef LBFGS_SOLVER_ERROR_HPP
#define LBFGS_SOLVER_ERROR_HPP

#include <string>
#include <system_error>

namespace lbfgs
{
enum class solver_error : uint8_t
{
    success,
    uncharted,                          // unexpected (unknown) error
    invalid_n,                          // invalid number of variables
    invalid_memsize,                    // solver::mem_size
    invalid_gepsilon,                   // solver::g_epsilon
    invalid_testperiod,                 // solver::past
    invalid_delta,                      // solver::delta
    invalid_minstep,                    // solver::min_step
    invalid_maxstep,                    // solver::max_step
    invalid_fdeccoeff,                  // solver::f_dec_coeff
    invalid_scurvcoeff,                 // solver::s_curv_coeff
    invalid_machineprec,                // solver::machine_prec
    invalid_maxlinesearch,              // solver::max_linesearch
    invalid_function_value,             // function value is NaN or Inf
    min_step_too_small,                 // line search step < solver::min_step
    max_step_too_large,                 // line search step > solver::max_step
    max_line_search_iterations_reached, // line search reaches the maximum, assumptions not satisfied or precision not achievable
    max_solver_iterations_reached,      // max number of iterations reached
    width_too_small,                    // relative search interval width is at least machine_prec
    invalid_parameters,                 // logic error (negative line-search step)
    increase_gradient,                  // the current search direction increases the cost function value
};

// forward declaration
inline auto make_error_code(lbfgs::solver_error /*e*/) -> std::error_code;
}  // namespace lbfgs

// register solver_error with the standard error code system
namespace std
{
template<>
struct is_error_code_enum<lbfgs::solver_error> : true_type
{
};  // NOLINT
}  // namespace std

namespace lbfgs::detail
{
class solver_error_category : public std::error_category
{
public:
    auto name() const noexcept -> char const* final { return "lbfgs::solver_error"; }

    auto message(int c) const -> std::string final
    {
        switch (static_cast<lbfgs::solver_error>(c)) {
            case lbfgs::solver_error::success:
                return "optimization successful";
            case lbfgs::solver_error::uncharted:
                return "uncharted error";
            case lbfgs::solver_error::invalid_n:
                return "invalid number of coefficients";
            case lbfgs::solver_error::invalid_memsize:
                return "invalid lbfgs::mem_size";
            case lbfgs::solver_error::invalid_gepsilon:
                return "invalid lbfgs::g_epsilon";
            case lbfgs::solver_error::invalid_testperiod:
                return "invalid lbfgs::past";
            case lbfgs::solver_error::invalid_delta:
                return "invalid lbfgs::delta";
            case lbfgs::solver_error::invalid_minstep:
                return "invalid lbfgs::min_step";
            case lbfgs::solver_error::invalid_maxstep:
                return "invalid lbfgs::max_step";
            case lbfgs::solver_error::invalid_fdeccoeff:
                return "invalid lbfgs::f_dec_coeff";
            case lbfgs::solver_error::invalid_scurvcoeff:
                return "invalid lbfgs::s_curv_coeff";
            case lbfgs::solver_error::invalid_machineprec:
                return "invalid lbfgs::machine_prec";
            case lbfgs::solver_error::invalid_maxlinesearch:
                return "invalid lbfgs::max_linesearch";
            case lbfgs::solver_error::invalid_function_value:
                return "the function value is NaN or Inf";
            case lbfgs::solver_error::min_step_too_small:
                return "the line search step became smaller than lbfgs::min_step";
            case lbfgs::solver_error::max_step_too_large:
                return "the line search step became larger than lbfgs::max_step";
            case lbfgs::solver_error::max_line_search_iterations_reached:
                return "line search reached maximum, assumptions not satisfied or precision not achievable";
            case lbfgs::solver_error::max_solver_iterations_reached:
                return "maximum number of iterations reached";
            case lbfgs::solver_error::width_too_small:
                return "search interval width smaller than machine precision";
            case lbfgs::solver_error::invalid_parameters:
                return "a logic error occurred (invalid settings)";
            case lbfgs::solver_error::increase_gradient:
                return "the current search direction increases the cost function value";
            default:
                return "unknown error";
        }
    }

    // allow comparison with generic error conditions
    auto default_error_condition(int c) const noexcept -> std::error_condition final
    {
        switch (static_cast<solver_error>(c)) {
            case solver_error::uncharted:
            case solver_error::invalid_n:
            case solver_error::invalid_memsize:
            case solver_error::invalid_gepsilon:
            case solver_error::invalid_testperiod:
            case solver_error::invalid_delta:
            case solver_error::invalid_minstep:
            case solver_error::invalid_maxstep:
            case solver_error::invalid_fdeccoeff:
            case solver_error::invalid_scurvcoeff:
            case solver_error::invalid_machineprec:
            case solver_error::invalid_maxlinesearch:
                return make_error_condition(std::errc::invalid_argument);
            case solver_error::invalid_function_value:
            case solver_error::min_step_too_small:
            case solver_error::max_step_too_large:
            case solver_error::max_line_search_iterations_reached:
            case solver_error::max_solver_iterations_reached:
            case solver_error::width_too_small:
            case solver_error::invalid_parameters:
            case solver_error::increase_gradient:
                return make_error_condition(std::errc::result_out_of_range);
            default:
                return {c, *this};
        }
    }
};
}  // namespace lbfgs::detail

namespace lbfgs
{
inline auto solver_error_category() -> detail::solver_error_category&
{
    static detail::solver_error_category c;
    return c;
}

// overload the global make_error_code() free function with our
// custom enum. It will be found via ADL by the compiler if needed.
auto make_error_code(lbfgs::solver_error e) -> std::error_code
{
    return {static_cast<int>(e), lbfgs::solver_error_category()};
}
}  // namespace lbfgs

#endif
