#ifndef __STAN__PROB__DISTRIBUTIONS__MULTIVARIATE__CONTINUOUS__MULTI_GP_HPP__
#define __STAN__PROB__DISTRIBUTIONS__MULTIVARIATE__CONTINUOUS__MULTI_GP_HPP__

#include <boost/random/normal_distribution.hpp>
#include <boost/random/variate_generator.hpp>

#include <stan/math/matrix_error_handling.hpp>
#include <stan/math/error_handling.hpp>
#include <stan/prob/constants.hpp>
#include <stan/prob/traits.hpp>
#include <stan/agrad/agrad.hpp>
#include <stan/meta/traits.hpp>
#include <stan/agrad/matrix.hpp>
#include <stan/math/matrix/dot_product.hpp>
#include <stan/math/matrix/log.hpp>
#include <stan/math/matrix/log_determinant_spd.hpp>
#include <stan/math/matrix/mdivide_right_spd.hpp>
#include <stan/math/matrix/multiply.hpp>
#include <stan/math/matrix/rows_dot_product.hpp>
#include <stan/math/matrix/subtract.hpp>
#include <stan/math/matrix/sum.hpp>

namespace stan {
  namespace prob {
    // MultiGP(y|Sigma,w)   [y.rows() = w.size(), y.cols() = Sigma.rows();
    //                            Sigma symmetric, non-negative, definite]
    /**
     * The log of a multivariate Gaussian Process for the given y, Sigma, and
     * w.  y is a dxN matrix, where each column is a different observation and each
     * row is a different output dimension.  The Guassian Process is assumed to
     * have a scaled kernel matrix with a different scale for each output dimension.
     * This distribution is equivalent to:
     *    for (i in 1:d) row(y,i) ~ multi_normal(0,(1/w[i])*Sigma).
     *
     * @param y A dxN matrix
     * @param Sigma The NxN kernel matrix
     * @param w A d-dimensional vector of positve inverse scale parameters for each output.
     * @return The log of the multivariate GP density.
     * @throw std::domain_error if Sigma is not square, not symmetric, 
     * or not semi-positive definite.
     * @tparam T_y Type of scalar.
     * @tparam T_covar Type of kernel.
     * @tparam T_w Type of weight.
     */
    template <bool propto,
    typename T_y, typename T_covar, typename T_w,
    class Policy>
    typename boost::math::tools::promote_args<T_y,T_w,T_covar>::type
    multi_gp_log(const Eigen::Matrix<T_y,Eigen::Dynamic,Eigen::Dynamic>& y,
                 const Eigen::Matrix<T_covar,Eigen::Dynamic,Eigen::Dynamic>& Sigma,
                 const Eigen::Matrix<T_w,Eigen::Dynamic,1>& w,
                 const Policy&) {
      static const char* function = "stan::prob::multi_gp_log(%1%)";
      typename boost::math::tools::promote_args<T_y,T_w,T_covar>::type lp(0.0);
      
      using stan::math::log;
      using stan::math::sum;
      using stan::math::check_not_nan;
      using stan::math::check_size_match;
      using stan::math::check_positive;
      using stan::math::check_pos_definite;
      using stan::math::check_finite;
      using stan::math::check_symmetric;
      using stan::math::dot_product;
      using stan::math::rows_dot_product;
      using stan::math::mdivide_right_spd;
      using stan::math::log_determinant_spd;
      
      if (!check_size_match(function, 
                            Sigma.rows(), "Rows of kernel matrix",
                            Sigma.cols(), "columns of kernel matrix",
                            &lp, Policy()))
        return lp;
      if (!check_positive(function, Sigma.rows(), "Kernel matrix rows", &lp, Policy()))
        return lp;
      if (!check_finite(function, Sigma, "Kernel", &lp, Policy())) 
        return lp;
      if (!check_symmetric(function, Sigma, "Kernel matrix", &lp, Policy()))
        return lp;
      if (!check_pos_definite(function, Sigma, "Kernel matrix", &lp, Policy()))
        return lp;
      if (!check_size_match(function, 
                            y.rows(), "Size of random variable",
                            w.size(), "Size of location parameter",
                            &lp, Policy()))
        return lp;
      if (!check_size_match(function, 
                            y.cols(), "Size of random variable",
                            Sigma.rows(), "rows of covariance parameter",
                            &lp, Policy()))
        return lp;
      if (!check_finite(function, w, "Kernel scales", &lp, Policy())) 
        return lp;
      if (!check_positive(function, w, "Kernel scales", &lp, Policy())) 
        return lp;
      if (!check_not_nan(function, y, "Random variable", &lp, Policy())) 
        return lp;
      
      if (y.rows() == 0)
        return lp;
      
      if (include_summand<propto>::value) {
        lp += NEG_LOG_SQRT_TWO_PI * y.rows() * y.cols();
      }
      
      if (include_summand<propto,T_covar>::value) {
        lp -= (0.5 * y.rows()) * log_determinant_spd(Sigma);
      }

      if (include_summand<propto,T_w>::value) {
        lp += (0.5 * y.cols()) * sum(log(w));
      }
      
      if (include_summand<propto,T_y,T_w,T_covar>::value) {
        Eigen::Matrix<typename 
        boost::math::tools::promote_args<T_covar,T_y>::type,
        Eigen::Dynamic, Eigen::Dynamic> y_Kinv(mdivide_right_spd(y,Sigma));

        lp -= 0.5 * dot_product(rows_dot_product(y_Kinv,y),w);
      }
      return lp;
    }
    
    template <bool propto,
    typename T_y, typename T_loc, typename T_covar>
    inline
    typename boost::math::tools::promote_args<T_y,T_loc,T_covar>::type
    multi_gp_log(const Eigen::Matrix<T_y,Eigen::Dynamic,Eigen::Dynamic>& y,
                 const Eigen::Matrix<T_covar,Eigen::Dynamic,Eigen::Dynamic>& Sigma,
                 const Eigen::Matrix<T_loc,Eigen::Dynamic,1>& w) {
      return multi_gp_log<propto>(y,Sigma,w,stan::math::default_policy());
    }
    
    
    template <typename T_y, typename T_loc, typename T_covar, 
    class Policy>
    inline
    typename boost::math::tools::promote_args<T_y,T_loc,T_covar>::type
    multi_gp_log(const Eigen::Matrix<T_y,Eigen::Dynamic,Eigen::Dynamic>& y,
                 const Eigen::Matrix<T_covar,Eigen::Dynamic,Eigen::Dynamic>& Sigma,
                 const Eigen::Matrix<T_loc,Eigen::Dynamic,1>& w,
                 const Policy&){
      return multi_gp_log<false>(y,Sigma,w,Policy());
    }
    
    
    template <typename T_y, typename T_loc, typename T_covar>
    inline
    typename boost::math::tools::promote_args<T_y,T_loc,T_covar>::type
    multi_gp_log(const Eigen::Matrix<T_y,Eigen::Dynamic,Eigen::Dynamic>& y,
                 const Eigen::Matrix<T_covar,Eigen::Dynamic,Eigen::Dynamic>& Sigma,
                 const Eigen::Matrix<T_loc,Eigen::Dynamic,1>& w) {
      return multi_gp_log<false>(y,Sigma,w,stan::math::default_policy());
    }
  }    
}

#endif