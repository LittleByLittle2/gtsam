/* ----------------------------------------------------------------------------

 * GTSAM Copyright 2010, Georgia Tech Research Corporation, 
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * Authors: Frank Dellaert, et al. (see THANKS for the full author list)

 * See LICENSE for the license information

 * -------------------------------------------------------------------------- */

/**
 * @file  Pose2.h
 * @brief 2D Pose
 * @author: Frank Dellaert
 * @author: Richard Roberts
 */

// \callgraph

#pragma once

#include <gtsam/geometry/Point2.h>
#include <gtsam/geometry/Rot2.h>
#include <gtsam/base/Lie.h>

namespace gtsam {

/**
 * A 2D pose (Point2,Rot2)
 * @addtogroup geometry
 * \nosubgrouping
 */
class GTSAM_EXPORT Pose2: public LieGroup<Pose2, 3> {

public:

  /** Pose Concept requirements */
  typedef Rot2 Rotation;
  typedef Point2 Translation;

private:

  Rot2 r_;
  Point2 t_;

public:

  /// @name Standard Constructors
  /// @{

  /** default constructor = origin */
  Pose2() {} // default is origin

  /** copy constructor */
  Pose2(const Pose2& pose) : r_(pose.r_), t_(pose.t_) {}

  /**
   * construct from (x,y,theta)
   * @param x x coordinate
   * @param y y coordinate
   * @param theta angle with positive X-axis
   */
  Pose2(double x, double y, double theta) :
    r_(Rot2::fromAngle(theta)), t_(x, y) {
  }

  /** construct from rotation and translation */
  Pose2(double theta, const Point2& t) :
    r_(Rot2::fromAngle(theta)), t_(t) {
  }

  /** construct from r,t */
  Pose2(const Rot2& r, const Point2& t) : r_(r), t_(t) {}

  /** Constructor from 3*3 matrix */
  Pose2(const Matrix &T) :
    r_(Rot2::atan2(T(1, 0), T(0, 0))), t_(T(0, 2), T(1, 2)) {
    assert(T.rows() == 3 && T.cols() == 3);
  }

  /// @}
  /// @name Advanced Constructors
  /// @{

  /** Construct from canonical coordinates \f$ [T_x,T_y,\theta] \f$ (Lie algebra) */
  Pose2(const Vector& v) {
    *this = Expmap(v);
  }

  /// @}
  /// @name Testable
  /// @{

  /** print with optional string */
  void print(const std::string& s = "") const;

  /** assert equality up to a tolerance */
  bool equals(const Pose2& pose, double tol = 1e-9) const;

  /// @}
  /// @name Group
  /// @{

  /// identity for group operation
  inline static Pose2 identity() { return Pose2(); }

  /// inverse
  Pose2 inverse() const;

  /// compose syntactic sugar
  inline Pose2 operator*(const Pose2& p2) const {
    return Pose2(r_*p2.r(), t_ + r_*p2.t());
  }

  /// @}
  /// @name Lie Group
  /// @{

  ///Exponential map at identity - create a rotation from canonical coordinates \f$ [T_x,T_y,\theta] \f$
  static Pose2 Expmap(const Vector& xi, ChartJacobian H = boost::none);

  ///Log map at identity - return the canonical coordinates \f$ [T_x,T_y,\theta] \f$ of this rotation
  static Vector3 Logmap(const Pose2& p, ChartJacobian H = boost::none);

  /**
   * Calculate Adjoint map
   * Ad_pose is 3*3 matrix that when applied to twist xi \f$ [T_x,T_y,\theta] \f$, returns Ad_pose(xi)
   */
  Matrix3 AdjointMap() const;
  inline Vector Adjoint(const Vector& xi) const {
    assert(xi.size() == 3);
    return AdjointMap()*xi;
  }

  /**
   * Compute the [ad(w,v)] operator for SE2 as in [Kobilarov09siggraph], pg 19
   */
  static Matrix3 adjointMap(const Vector& v);

  /**
   * wedge for SE(2):
   * @param xi 3-dim twist (v,omega) where
   *  omega is angular velocity
   *  v (vx,vy) = 2D velocity
   * @return xihat, 3*3 element of Lie algebra that can be exponentiated
   */
  static inline Matrix3 wedge(double vx, double vy, double w) {
    Matrix3 m;
    m << 0.,-w,  vx,
         w,  0., vy,
         0., 0.,  0.;
    return m;
  }

  /// Left-trivialized derivative of the exponential map
  static Matrix3 dexpL(const Vector3& v);

  /// Left-trivialized derivative inverse of the exponential map
  static Matrix3 dexpInvL(const Vector3& v);

  // Chart at origin, depends on compile-time flag SLOW_BUT_CORRECT_EXPMAP
  struct ChartAtOrigin {
    static Pose2 Retract(const Vector3& v, ChartJacobian H = boost::none);
    static Vector3 Local(const Pose2& r, ChartJacobian H = boost::none);
  };

  using LieGroup<Pose2, 3>::inverse; // version with derivative

  /// @}
  /// @name Group Action on Point2
  /// @{

  /** Return point coordinates in pose coordinate frame */
  Point2 transform_to(const Point2& point,
      OptionalJacobian<2, 3> H1 = boost::none,
      OptionalJacobian<2, 2> H2 = boost::none) const;

  /** Return point coordinates in global frame */
  Point2 transform_from(const Point2& point,
      OptionalJacobian<2, 3> H1 = boost::none,
      OptionalJacobian<2, 2> H2 = boost::none) const;

  /** syntactic sugar for transform_from */
  inline Point2 operator*(const Point2& point) const { return transform_from(point);}

  /// @}
  /// @name Standard Interface
  /// @{

  /// get x
  inline double x()     const { return t_.x(); }

  /// get y
  inline double y()     const { return t_.y(); }

  /// get theta
  inline double theta() const { return r_.theta(); }

  /// translation
  inline const Point2& t() const { return t_; }

  /// rotation
  inline const Rot2&   r() const { return r_; }

  /// translation
  inline const Point2& translation() const { return t_; }

  /// rotation
  inline const Rot2&   rotation() const { return r_; }

  //// return transformation matrix
  Matrix3 matrix() const;

  /**
   * Calculate bearing to a landmark
   * @param point 2D location of landmark
   * @return 2D rotation \f$ \in SO(2) \f$
   */
  Rot2 bearing(const Point2& point,
               OptionalJacobian<1, 3> H1=boost::none, OptionalJacobian<1, 2> H2=boost::none) const;

  /**
   * Calculate bearing to another pose
   * @param point SO(2) location of other pose
   * @return 2D rotation \f$ \in SO(2) \f$
   */
  Rot2 bearing(const Pose2& pose,
               OptionalJacobian<1, 3> H1=boost::none, OptionalJacobian<1, 3> H2=boost::none) const;

  /**
   * Calculate range to a landmark
   * @param point 2D location of landmark
   * @return range (double)
   */
  double range(const Point2& point,
      OptionalJacobian<1, 3> H1=boost::none,
      OptionalJacobian<1, 2> H2=boost::none) const;

  /**
   * Calculate range to another pose
   * @param point 2D location of other pose
   * @return range (double)
   */
  double range(const Pose2& point,
      OptionalJacobian<1, 3> H1=boost::none,
      OptionalJacobian<1, 3> H2=boost::none) const;

  /// @}
  /// @name Advanced Interface
  /// @{

  /**
   * Return the start and end indices (inclusive) of the translation component of the
   * exponential map parameterization
   * @return a pair of [start, end] indices into the tangent space vector
   */
  inline static std::pair<size_t, size_t> translationInterval() { return std::make_pair(0, 1); }

  /**
   * Return the start and end indices (inclusive) of the rotation component of the
   * exponential map parameterization
   * @return a pair of [start, end] indices into the tangent space vector
   */
  static std::pair<size_t, size_t> rotationInterval() { return std::make_pair(2, 2); }

  /// @}

private:

  // Serialization function
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive & ar, const unsigned int version) {
    ar & BOOST_SERIALIZATION_NVP(t_);
    ar & BOOST_SERIALIZATION_NVP(r_);
  }
}; // Pose2

/** specialization for pose2 wedge function (generic template in Lie.h) */
template <>
inline Matrix wedge<Pose2>(const Vector& xi) {
  return Pose2::wedge(xi(0),xi(1),xi(2));
}

/**
 * Calculate pose between a vector of 2D point correspondences (p,q)
 * where q = Pose2::transform_from(p) = t + R*p
 */
typedef std::pair<Point2,Point2> Point2Pair;
GTSAM_EXPORT boost::optional<Pose2> align(const std::vector<Point2Pair>& pairs);

template<>
struct traits_x<Pose2> : public internal::LieGroupTraits<Pose2> {};

} // namespace gtsam

