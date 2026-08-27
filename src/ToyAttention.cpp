/* -*- C++ -*- */
//
// ToyAttention.cpp
//
// See ToyAttention.h.

#include "ToyAttention.h"
#include <cmath>
#include <stdexcept>

namespace ModernCommon {

Eigen::MatrixXd softmaxRows(const Eigen::MatrixXd& scores)
{
  Eigen::MatrixXd result(scores.rows(), scores.cols());
  for (Eigen::Index i = 0; i < scores.rows(); ++i)
  {
    const double maxVal = scores.row(i).maxCoeff();
    Eigen::RowVectorXd expRow = (scores.row(i).array() - maxVal).exp();
    result.row(i) = expRow / expRow.sum();
  }
  return result;
}

Eigen::MatrixXd scaledDotProductAttention(const Eigen::MatrixXd& Q,
                                           const Eigen::MatrixXd& K,
                                           const Eigen::MatrixXd& V)
{
  if (Q.cols() != K.cols())
  {
    throw std::invalid_argument(
      "scaledDotProductAttention: Q and K must share the same inner dimension (d_k)");
  }
  if (K.rows() != V.rows())
  {
    throw std::invalid_argument(
      "scaledDotProductAttention: K and V must have the same number of rows (n_k)");
  }

  const double dk = static_cast<double>(K.cols());
  Eigen::MatrixXd scores = (Q * K.transpose()) / std::sqrt(dk);
  Eigen::MatrixXd weights = softmaxRows(scores);
  return weights * V;
}

} // namespace ModernCommon
