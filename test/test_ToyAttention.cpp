/* -*- C++ -*- */
//
// test_ToyAttention.cpp
//
// Exercises softmaxRows()/scaledDotProductAttention() -- see
// ToyAttention.h. Uses exact (rational, not just tolerance-checked)
// test cases wherever the math allows it, so a real computation bug
// can't hide behind a loose floating-point tolerance.

#include "MiniTest.h"
#include "ToyAttention.h"
#include <cmath>
#include <stdexcept>

using namespace ModernCommon;

namespace {

  void test_softmaxRowsKnownExactValues(MiniTest& t)
  {
    // e^0 = 1, e^ln(3) = 3 -> weights = [1/4, 3/4] exactly.
    Eigen::MatrixXd scores(1, 2);
    scores << 0.0, std::log(3.0);

    Eigen::MatrixXd weights = softmaxRows(scores);
    MT_CHECK(t, std::abs(weights(0, 0) - 0.25) < 1e-9);
    MT_CHECK(t, std::abs(weights(0, 1) - 0.75) < 1e-9);
  }

  void test_softmaxRowsSumToOne(MiniTest& t)
  {
    Eigen::MatrixXd scores(2, 3);
    scores << 1.0, 2.0, 3.0,
              -1.0, 0.0, 5.0;

    Eigen::MatrixXd weights = softmaxRows(scores);
    for (Eigen::Index i = 0; i < weights.rows(); ++i)
    {
      MT_CHECK(t, std::abs(weights.row(i).sum() - 1.0) < 1e-9);
      for (Eigen::Index j = 0; j < weights.cols(); ++j)
      {
        MT_CHECK(t, weights(i, j) > 0.0);
      }
    }
  }

  void test_attentionOutputShape(MiniTest& t)
  {
    Eigen::MatrixXd Q(2, 4);
    Q.setZero();
    Eigen::MatrixXd K(3, 4);
    K.setZero();
    Eigen::MatrixXd V(3, 5);
    V.setZero();

    Eigen::MatrixXd out = scaledDotProductAttention(Q, K, V);
    MT_CHECK(t, out.rows() == 2); // n_q
    MT_CHECK(t, out.cols() == 5); // d_v
  }

  /// A zero query dots to 0 against every key regardless of K's values,
  /// so softmax is exactly uniform (1/n_k each) and the output is
  /// exactly the mean of V's rows -- no transcendental-function
  /// precision concerns, an exact rational check.
  void test_zeroQueryProducesUniformAttentionOverV(MiniTest& t)
  {
    Eigen::MatrixXd Q(1, 2);
    Q << 0, 0;
    Eigen::MatrixXd K(3, 2);
    K << 1, 2, 3, 4, 5, 6; // values are irrelevant when Q is zero
    Eigen::MatrixXd V(3, 2);
    V << 1, 0,
         0, 1,
         1, 1;

    Eigen::MatrixXd out = scaledDotProductAttention(Q, K, V);
    MT_CHECK(t, std::abs(out(0, 0) - 2.0 / 3.0) < 1e-9); // mean(1,0,1)
    MT_CHECK(t, std::abs(out(0, 1) - 2.0 / 3.0) < 1e-9); // mean(0,1,1)
  }

  /// A query that exactly matches one key (and is orthogonal/far from
  /// the others) should push nearly all attention weight onto that
  /// key's V row.
  void test_matchingQueryConcentratesAttention(MiniTest& t)
  {
    Eigen::MatrixXd Q(1, 3);
    Q << 10, 0, 0; // strongly aligned with K's row 0 only
    Eigen::MatrixXd K(3, 3);
    K << 1, 0, 0,
         0, 1, 0,
         0, 0, 1;
    Eigen::MatrixXd V(3, 3);
    V << 100, 0, 0,
         0, 200, 0,
         0, 0, 300;

    Eigen::MatrixXd out = scaledDotProductAttention(Q, K, V);
    MT_CHECK(t, out(0, 0) > 95.0);  // nearly all weight on V row 0
    MT_CHECK(t, out(0, 1) < 5.0);
    MT_CHECK(t, out(0, 2) < 5.0);
  }

  void test_mismatchedQkInnerDimensionThrows(MiniTest& t)
  {
    Eigen::MatrixXd Q(1, 2);
    Eigen::MatrixXd K(3, 3); // d_k mismatch: 2 vs 3
    Eigen::MatrixXd V(3, 2);

    bool threw = false;
    try { scaledDotProductAttention(Q, K, V); }
    catch (const std::invalid_argument&) { threw = true; }
    MT_CHECK(t, threw);
  }

  void test_mismatchedKvRowCountThrows(MiniTest& t)
  {
    Eigen::MatrixXd Q(1, 2);
    Eigen::MatrixXd K(3, 2);
    Eigen::MatrixXd V(4, 2); // n_k mismatch: K has 3 rows, V has 4

    bool threw = false;
    try { scaledDotProductAttention(Q, K, V); }
    catch (const std::invalid_argument&) { threw = true; }
    MT_CHECK(t, threw);
  }

} // namespace

int main()
{
  MiniTest t("test_ToyAttention");
  MT_RUN(t, test_softmaxRowsKnownExactValues);
  MT_RUN(t, test_softmaxRowsSumToOne);
  MT_RUN(t, test_attentionOutputShape);
  MT_RUN(t, test_zeroQueryProducesUniformAttentionOverV);
  MT_RUN(t, test_matchingQueryConcentratesAttention);
  MT_RUN(t, test_mismatchedQkInnerDimensionThrows);
  MT_RUN(t, test_mismatchedKvRowCountThrows);
  return t.result();
}
