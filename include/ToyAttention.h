/* -*- C++ -*- */
//
// ToyAttention.h
//
// A minimal, single-head scaled dot-product attention (Vaswani et al.,
// "Attention Is All You Need") built on Eigen -- see doc/eigen-matrix.md.
// Not batched, not multi-head, no learned projections: Q/K/V are taken
// as already-projected matrices. This is the same GEMM + softmax + GEMM
// computation that dominates a transformer's attention layer, small
// enough to read and verify by hand.

#ifndef MODERNTEMPLATE_TOYATTENTION_H
#define MODERNTEMPLATE_TOYATTENTION_H

#include <Eigen/Dense>

namespace ModernCommon {

  /// Row-wise softmax: each row of `scores` becomes a probability
  /// distribution (sums to 1, all entries positive). Numerically
  /// stabilized by subtracting each row's max before exponentiating.
  Eigen::MatrixXd softmaxRows(const Eigen::MatrixXd& scores);

  /// scores = Q * K^T / sqrt(d_k); weights = softmaxRows(scores);
  /// returns weights * V. Q is (n_q x d_k), K is (n_k x d_k), V is
  /// (n_k x d_v); the result is (n_q x d_v). Throws
  /// std::invalid_argument if Q/K's inner dimension or K/V's row count
  /// don't match.
  Eigen::MatrixXd scaledDotProductAttention(const Eigen::MatrixXd& Q,
                                             const Eigen::MatrixXd& K,
                                             const Eigen::MatrixXd& V);

} // namespace ModernCommon

#endif // MODERNTEMPLATE_TOYATTENTION_H
