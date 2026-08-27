/* -*- C++ -*- */
//
// demo_Attention.cpp
//
// A runnable console demonstration of scaledDotProductAttention (see
// ToyAttention.h) on a tiny, readable example: 3 "tokens", 4-dim Q/K/V,
// printing the intermediate scores/weights/output matrices -- the same
// core computation (GEMM + softmax + GEMM) that dominates a
// transformer's attention layer, at a scale small enough to read by eye.
//
// Run: ./demo_Attention

#include "ToyAttention.h"
#include <cmath>
#include <iomanip>
#include <iostream>

using namespace ModernCommon;

namespace {

  void printMatrix(const std::string& label, const Eigen::MatrixXd& m)
  {
    std::cout << label << " (" << m.rows() << "x" << m.cols() << "):\n"
              << std::fixed << std::setprecision(4) << m << "\n\n";
  }

} // namespace

int main()
{
  // 3 tokens; each query exactly (and strongly) matches one orthogonal
  // key, so attention should concentrate almost entirely on that key's
  // V row -- easy to verify by eye in the printed weights matrix below.
  // A large dot-product gap (10 vs 0, not 1 vs 0) is what makes softmax
  // actually sharpen instead of staying close to uniform.
  Eigen::MatrixXd Q(3, 4);
  Q << 10, 0, 0, 0,
       0, 10, 0, 0,
       0, 0, 10, 0;

  Eigen::MatrixXd K(3, 4);
  K << 1, 0, 0, 0,
       0, 1, 0, 0,
       0, 0, 1, 0;

  Eigen::MatrixXd V(3, 4);
  V << 10, 0, 0, 0,
       0, 20, 0, 0,
       0, 0, 30, 0;

  printMatrix("Q", Q);
  printMatrix("K", K);
  printMatrix("V", V);

  Eigen::MatrixXd scores = (Q * K.transpose()) / std::sqrt(static_cast<double>(K.cols()));
  printMatrix("scores = Q*K^T / sqrt(d_k)", scores);

  Eigen::MatrixXd weights = softmaxRows(scores);
  printMatrix("weights = softmax(scores, per row)", weights);

  Eigen::MatrixXd output = scaledDotProductAttention(Q, K, V);
  printMatrix("output = weights * V", output);

  std::cout << "Each query strongly matches one K row, so weights\n"
               "concentrate almost entirely on that row -- token i's\n"
               "output ends up close to V row i (10, 20, 30 respectively).\n";
  return 0;
}
