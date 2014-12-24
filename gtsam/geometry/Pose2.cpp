/* ----------------------------------------------------------------------------

 * GTSAM Copyright 2010, Georgia Tech Research Corporation, 
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * Authors: Frank Dellaert, et al. (see THANKS for the full author list)

 * See LICENSE for the license information

 * -------------------------------------------------------------------------- */

/**
 * @file  Pose2.cpp
 * @brief 2D Pose
 */

#include <gtsam/geometry/concepts.h>
#include <gtsam/geometry/Pose2.h>
#include <gtsam/base/Testable.h>
#include <gtsam/base/concepts.h>

#include <boost/foreach.hpp>

#include <cmath>
#include <iostream>
#include <iomanip>

using namespace std;

namespace gtsam {

/** instantiate concept checks */
GTSAM_CONCEPT_POSE_INST(Pose2);

static const Rot2 R_PI_2(Rot2::fromCosSin(0., 1.));

/* ************************************************************************* */
Matrix3 Pose2::matrix() const {
  Matrix2 R = r_.matrix();
  Matrix32 R0;
  R0.block<2,2>(0,0) = R;
  R0.block<1,2>(2,0).setZero();
  Matrix31 T;
  T <<  t_.x(), t_.y(), 1.0;
  Matrix3 RT_;
  RT_.block<3,2>(0,0) = R0;
  RT_.block<3,1>(0,2) = T;
  return RT_;
}

/* ************************************************************************* */
void Pose2::print(const string& s) const {
  cout << s << "(" << t_.x() << ", " << t_.y() << ", " << r_.theta() << ")" << endl;
}

/* ************************************************************************* */
bool Pose2::equals(const Pose2& q, double tol) const {
  return t_.equals(q.t_, tol) && r_.equals(q.r_, tol);
}

/* ************************************************************************* */
Pose2 Pose2::Expmap(const Vector& xi, OptionalJacobian<3, 3> H) {
  if (H) CONCEPT_NOT_IMPLEMENTED;
  assert(xi.size() == 3);
  Point2 v(xi(0),xi(1));
  double w = xi(2);
  if (std::abs(w) < 1e-10)
    return Pose2(xi[0], xi[1], xi[2]);
  else {
    Rot2 R(Rot2::fromAngle(w));
    Point2 v_ortho = R_PI_2 * v; // points towards rot center
    Point2 t = (v_ortho - R.rotate(v_ortho)) / w;
    return Pose2(R, t);
  }
}

/* ************************************************************************* */
Vector3 Pose2::Logmap(const Pose2& p, OptionalJacobian<3, 3> H) {
  if (H) CONCEPT_NOT_IMPLEMENTED;
  const Rot2& R = p.r();
  const Point2& t = p.t();
  double w = R.theta();
  if (std::abs(w) < 1e-10)
    return Vector3(t.x(), t.y(), w);
  else {
    double c_1 = R.c()-1.0, s = R.s();
    double det = c_1*c_1 + s*s;
    Point2 p = R_PI_2 * (R.unrotate(t) - t);
    Point2 v = (w/det) * p;
    return Vector3(v.x(), v.y(), w);
  }
}

/* ************************************************************************* */
Pose2 Pose2::ChartAtOrigin::Retract(const Vector3& v, ChartJacobian H) {
#ifdef SLOW_BUT_CORRECT_EXPMAP
  return Expmap(v, H);
#else
  if (H) *H = Matrix3::Identity();
  return Pose2(v[0], v[1], v[2]);
#endif
}
/* ************************************************************************* */
Vector3 Pose2::ChartAtOrigin::Local(const Pose2& r, ChartJacobian H) {
#ifdef SLOW_BUT_CORRECT_EXPMAP
  return Logmap(r, H);
#else
  if (H) *H = Matrix3::Identity();
  return Vector3(r.x(), r.y(), r.theta());
#endif
}

/* ************************************************************************* */
// Calculate Adjoint map
// Ad_pose is 3*3 matrix that when applied to twist xi, returns Ad_pose(xi)
Matrix3 Pose2::AdjointMap() const {
  double c = r_.c(), s = r_.s(), x = t_.x(), y = t_.y();
  Matrix3 rvalue;
  rvalue <<
      c,  -s,   y,
      s,   c,  -x,
      0.0, 0.0, 1.0;
  return rvalue;
}

/* ************************************************************************* */
Matrix3 Pose2::adjointMap(const Vector& v) {
  // See Chirikjian12book2, vol.2, pg. 36
  Matrix3 ad = zeros(3,3);
  ad(0,1) = -v[2];
  ad(1,0) = v[2];
  ad(0,2) = v[1];
  ad(1,2) = -v[0];
  return ad;
}

/* ************************************************************************* */
Matrix3 Pose2::dexpL(const Vector3& v) {
  double alpha = v[2];
  if (fabs(alpha) > 1e-5) {
    // Chirikjian11book2, pg. 36
    /* !!!Warning!!! Compare Iserles05an, formula 2.42 and Chirikjian11book2 pg.26
     * Iserles' right-trivialization dexpR is actually the left Jacobian J_l in Chirikjian's notation
     * In fact, Iserles 2.42 can be written as:
     *    \dot{g} g^{-1} = dexpR_{q}\dot{q}
     * where q = A, and g = exp(A)
     * and the LHS is in the definition of J_l in Chirikjian11book2, pg. 26.
     * Hence, to compute dexpL, we have to use the formula of J_r Chirikjian11book2, pg.36
     */
    double sZalpha = sin(alpha)/alpha, c_1Zalpha = (cos(alpha)-1)/alpha;
    double v1Zalpha = v[0]/alpha, v2Zalpha = v[1]/alpha;
    return (Matrix(3,3) <<
        sZalpha, -c_1Zalpha, v1Zalpha + v2Zalpha*c_1Zalpha - v1Zalpha*sZalpha,
        c_1Zalpha, sZalpha, -v1Zalpha*c_1Zalpha + v2Zalpha - v2Zalpha*sZalpha,
        0, 0, 1).finished();
  }
  else {
    // Thanks to Krunal: Apply L'Hospital rule to several times to
    // compute the limits when alpha -> 0
    return (Matrix(3,3) << 1,0,-0.5*v[1],
        0,1, 0.5*v[0],
        0,0, 1).finished();
  }
}

/* ************************************************************************* */
Matrix3 Pose2::dexpInvL(const Vector3& v) {
  double alpha = v[2];
  if (fabs(alpha) > 1e-5) {
    double alphaInv = 1/alpha;
    double halfCotHalfAlpha = 0.5*sin(alpha)/(1-cos(alpha));
    double v1 = v[0], v2 = v[1];

    return (Matrix(3,3) <<
        alpha*halfCotHalfAlpha, -0.5*alpha, v1*alphaInv - v1*halfCotHalfAlpha + 0.5*v2,
        0.5*alpha, alpha*halfCotHalfAlpha,  v2*alphaInv - 0.5*v1 - v2*halfCotHalfAlpha,
        0, 0, 1).finished();
  }
  else {
    return (Matrix(3,3) << 1,0, 0.5*v[1],
        0,1, -0.5*v[0],
        0,0, 1).finished();
  }
}

/* ************************************************************************* */
Pose2 Pose2::inverse() const {
  return Pose2(r_.inverse(), r_.unrotate(Point2(-t_.x(), -t_.y())));
}

/* ************************************************************************* */
// see doc/math.lyx, SE(2) section
Point2 Pose2::transform_to(const Point2& point,
    OptionalJacobian<2, 3> H1, OptionalJacobian<2, 2> H2) const {
  Point2 d = point - t_;
  Point2 q = r_.unrotate(d);
  if (!H1 && !H2) return q;
  if (H1) *H1 <<
      -1.0, 0.0,  q.y(),
      0.0, -1.0, -q.x();
  if (H2) *H2 << r_.transpose();
  return q;
}

/* ************************************************************************* */
// see doc/math.lyx, SE(2) section
Point2 Pose2::transform_from(const Point2& p,
    OptionalJacobian<2, 3> H1, OptionalJacobian<2, 2> H2) const {
  const Point2 q = r_ * p;
  if (H1 || H2) {
    const Matrix2 R = r_.matrix();
    Matrix21 Drotate1;
    Drotate1 <<  -q.y(), q.x();
    if (H1) *H1 << R, Drotate1;
    if (H2) *H2 = R; // R
  }
  return q + t_;
}

/* ************************************************************************* */
Rot2 Pose2::bearing(const Point2& point,
    OptionalJacobian<1, 3> H1, OptionalJacobian<1, 2> H2) const {
  // make temporary matrices
  Matrix23 D1; Matrix2 D2;
  Point2 d = transform_to(point, H1 ? &D1 : 0, H2 ? &D2 : 0); // uses pointer version
  if (!H1 && !H2) return Rot2::relativeBearing(d);
  Matrix12 D_result_d;
  Rot2 result = Rot2::relativeBearing(d, D_result_d);
  if (H1) *H1 = D_result_d * (D1);
  if (H2) *H2 = D_result_d * (D2);
  return result;
}

/* ************************************************************************* */
Rot2 Pose2::bearing(const Pose2& pose,
    OptionalJacobian<1, 3> H1, OptionalJacobian<1, 3> H2) const {
  Matrix12 D2;
  Rot2 result = bearing(pose.t(), H1, H2 ? &D2 : 0);
  if (H2) {
    Matrix12 H2_ = D2 * pose.r().matrix();
    *H2 << H2_, Z_1x1;
  }
  return result;
}
/* ************************************************************************* */
double Pose2::range(const Point2& point,
    OptionalJacobian<1,3> H1, OptionalJacobian<1,2> H2) const {
  Point2 d = point - t_;
  if (!H1 && !H2) return d.norm();
  Matrix12 H;
  double r = d.norm(H);
  if (H1) {
      Matrix23 H1_;
      H1_ << -r_.c(),  r_.s(),  0.0,
              -r_.s(), -r_.c(),  0.0;
      *H1 = H * H1_;
  }
  if (H2) *H2 = H;
  return r;
}

/* ************************************************************************* */
double Pose2::range(const Pose2& pose,
    OptionalJacobian<1,3> H1,
    OptionalJacobian<1,3> H2) const {
  Point2 d = pose.t() - t_;
  if (!H1 && !H2) return d.norm();
  Matrix12 H;
  double r = d.norm(H);
  if (H1) {
      Matrix23 H1_;
      H1_ <<
      -r_.c(),  r_.s(),  0.0,
      -r_.s(), -r_.c(),  0.0;
      *H1 = H * H1_;
  }
  if (H2) {
      Matrix23 H2_;
      H2_ <<
      pose.r_.c(), -pose.r_.s(),  0.0,
      pose.r_.s(),  pose.r_.c(),  0.0;
      *H2 = H * H2_;
  }
  return r;
}

/* *************************************************************************
 * New explanation, from scan.ml
 * It finds the angle using a linear method:
 * q = Pose2::transform_from(p) = t + R*p
 * We need to remove the centroids from the data to find the rotation
 * using dp=[dpx;dpy] and q=[dqx;dqy] we have
 *  |dqx|   |c  -s|     |dpx|     |dpx -dpy|     |c|
 *  |   | = |     |  *  |   |  =  |        |  *  | | = H_i*cs
 *  |dqy|   |s   c|     |dpy|     |dpy  dpx|     |s|
 * where the Hi are the 2*2 matrices. Then we will minimize the criterion
 * J = \sum_i norm(q_i - H_i * cs)
 * Taking the derivative with respect to cs and setting to zero we have
 * cs = (\sum_i H_i' * q_i)/(\sum H_i'*H_i)
 * The hessian is diagonal and just divides by a constant, but this
 * normalization constant is irrelevant, since we take atan2.
 * i.e., cos ~ sum(dpx*dqx + dpy*dqy) and sin ~ sum(-dpy*dqx + dpx*dqy)
 * The translation is then found from the centroids
 * as they also satisfy cq = t + R*cp, hence t = cq - R*cp
 */

boost::optional<Pose2> align(const vector<Point2Pair>& pairs) {

  size_t n = pairs.size();
  if (n<2) return boost::none; // we need at least two pairs

  // calculate centroids
  Point2 cp,cq;
  BOOST_FOREACH(const Point2Pair& pair, pairs) {
    cp += pair.first;
    cq += pair.second;
  }
  double f = 1.0/n;
  cp *= f; cq *= f;

  // calculate cos and sin
  double c=0,s=0;
  BOOST_FOREACH(const Point2Pair& pair, pairs) {
    Point2 dq = pair.first  - cp;
    Point2 dp = pair.second - cq;
    c +=  dp.x() * dq.x() + dp.y() * dq.y();
    s +=  dp.y() * dq.x() - dp.x() * dq.y(); // this works but is negative from formula above !! :-(
  }

  // calculate angle and translation
  double theta = atan2(s,c);
  Rot2 R = Rot2::fromAngle(theta);
  Point2 t = cq - R*cp;
  return Pose2(R, t);
}

/* ************************************************************************* */
} // namespace gtsam
