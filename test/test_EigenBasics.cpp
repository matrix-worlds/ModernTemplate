/* -*- C++ -*- */
//
// test_EigenBasics.cpp
//
// Exercises basic Eigen matrix operations and, concretely, Eigen's
// expression-template mechanism: `A + B` is not a materialized
// Eigen::MatrixXd -- its type is a lazy expression template, proven here
// via a compile-time static_assert rather than just claimed in a
// comment. See doc/eigen-matrix.md for why this matters for performance.

#include "MiniTest.h"
#include <Eigen/Dense>
#include <type_traits>

using namespace ModernCommon; // MiniTest lives here -- see MiniTest.h

namespace {

  void test_basicConstructionAndAccess(MiniTest& t)
  {
    Eigen::MatrixXd m(2, 2);
    m << 1, 2,
         3, 4;
    MT_CHECK(t, m(0, 0) == 1);
    MT_CHECK(t, m(0, 1) == 2);
    MT_CHECK(t, m(1, 0) == 3);
    MT_CHECK(t, m(1, 1) == 4);
  }

  void test_addition(MiniTest& t)
  {
    Eigen::MatrixXd a(2, 2);
    a << 1, 2, 3, 4;
    Eigen::MatrixXd b(2, 2);
    b << 5, 6, 7, 8;

    Eigen::MatrixXd sum = a + b;
    MT_CHECK(t, sum(0, 0) == 6);
    MT_CHECK(t, sum(1, 1) == 12);
  }

  void test_matrixMultiplication(MiniTest& t)
  {
    Eigen::MatrixXd a(2, 3);
    a << 1, 2, 3,
         4, 5, 6;
    Eigen::MatrixXd b(3, 2);
    b << 7, 8,
         9, 10,
         11, 12;

    Eigen::MatrixXd product = a * b;
    MT_CHECK(t, product.rows() == 2);
    MT_CHECK(t, product.cols() == 2);
    // row 0: [1,2,3]·[7,9,11]=58 ; [1,2,3]·[8,10,12]=64
    MT_CHECK(t, product(0, 0) == 58);
    MT_CHECK(t, product(0, 1) == 64);
    // row 1: [4,5,6]·[7,9,11]=139 ; [4,5,6]·[8,10,12]=154
    MT_CHECK(t, product(1, 0) == 139);
    MT_CHECK(t, product(1, 1) == 154);
  }

  void test_transpose(MiniTest& t)
  {
    Eigen::MatrixXd a(2, 3);
    a << 1, 2, 3,
         4, 5, 6;
    Eigen::MatrixXd at = a.transpose();
    MT_CHECK(t, at.rows() == 3);
    MT_CHECK(t, at.cols() == 2);
    MT_CHECK(t, at(2, 0) == 3);
    MT_CHECK(t, at(0, 1) == 4);
  }

  /// The concrete demonstration: A + B * C is NOT three separate
  /// temporaries under the hood -- its type is an unevaluated expression
  /// template, proven at compile time. Evaluation (and the one
  /// MatrixXd allocation it needs) happens only at the final assignment.
  void test_expressionTemplatesAreLazy(MiniTest& t)
  {
    Eigen::MatrixXd a(2, 2);
    a << 1, 0, 0, 1;
    Eigen::MatrixXd b(2, 2);
    b << 2, 0, 0, 2;
    Eigen::MatrixXd c(2, 2);
    c << 1, 1, 1, 1;

    auto expr = a + b * c; // not evaluated yet
    static_assert(!std::is_same_v<decltype(expr), Eigen::MatrixXd>,
                  "a + b*c should be a lazy expression type, not a materialized MatrixXd");

    Eigen::MatrixXd result = expr; // evaluation happens exactly here
    // b*c = [[2,2],[2,2]]; a + that = [[3,2],[2,3]]
    MT_CHECK(t, result(0, 0) == 3);
    MT_CHECK(t, result(0, 1) == 2);
    MT_CHECK(t, result(1, 0) == 2);
    MT_CHECK(t, result(1, 1) == 3);
  }

  /// A toy "dense layer" (y = Wx + b), the same shape as the linear
  /// layers that dominate a transformer's parameter count.
  void test_toyLinearLayer(MiniTest& t)
  {
    Eigen::MatrixXd W(2, 3);
    W << 1, 0, 1,
         0, 1, 1;
    Eigen::VectorXd x(3);
    x << 1, 2, 3;
    Eigen::VectorXd bias(2);
    bias << 0.5, -0.5;

    Eigen::VectorXd y = W * x + bias;
    MT_CHECK(t, y(0) == 4.5); // 1*1+0*2+1*3 + 0.5
    MT_CHECK(t, y(1) == 4.5); // 0*1+1*2+1*3 - 0.5
  }

} // namespace

int main()
{
  MiniTest t("test_EigenBasics");
  MT_RUN(t, test_basicConstructionAndAccess);
  MT_RUN(t, test_addition);
  MT_RUN(t, test_matrixMultiplication);
  MT_RUN(t, test_transpose);
  MT_RUN(t, test_expressionTemplatesAreLazy);
  MT_RUN(t, test_toyLinearLayer);
  return t.result();
}
