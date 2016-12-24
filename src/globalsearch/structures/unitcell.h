/**********************************************************************
  Unit Cell - a basic unit cell class.

  Copyright (C) 2016 by Patrick S. Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

 ***********************************************************************/

#ifndef GLOBALSEARCH_UNIT_CELL_H
#define GLOBALSEARCH_UNIT_CELL_H

#include <cmath>

#include <globalsearch/matrix.h>
#include <globalsearch/vector.h>

static const double PI = 3.14159265359;
static const double DEG2RAD = PI / 180.0;
static const double RAD2DEG = 180.0 / PI;

namespace GlobalSearch
{
 /**
  * @class UnitCell unitcell.h
  * @brief A basic unit cell class. Contains a 3x3 matrix.
  */
  class UnitCell
  {
   public:
    /**
     * Constructor. The default matrix is the zero matrix.
     *
     * @param m The matrix with which the cell matrix will be set. The
     *          default cell matrix is the zero matrix.
     */
    UnitCell(const Matrix3& m = Matrix3());

    /**
     * Constructor. This uses cell parameters to create the cell matrix.
     * The A vector will lie along the x-axis. The B vector will lie in
     * the positive y region of the x-y plane. The C vector will then be constrained.
     *
     * @param a The length (Angstroms) of vector A.
     * @param b The length (Angstroms) of vector B.
     * @param c The length (Angstroms) of vector C.
     * @param alpha The angle (degrees) between vectors B and C.
     * @param beta The angle (degrees) between vectors A and C.
     * @param gamma The angle (degrees) between vectors A and B.
     */
    UnitCell(double a, double b, double c,
             double alpha, double beta, double gamma);

    /* Copy constructor. Just copies the data. */
    UnitCell(const UnitCell& other);

    /* Assignment operator. Just copies the data. */
    UnitCell& operator=(const UnitCell& other);

    /**
     * Checks whether the cell is valid or not. If the volume is zero, the
     * cell is considered invalid. Otherwise, it is considered valid.
     *
     * @return True if the cell is valid. False if it is not.
     */
    bool isValid() const { return volume() != 0.0; };

    /**
     * This uses cell parameters to create the cell matrix.
     * The A vector will lie along the x-axis. The B vector will lie in
     * the positive y region of the x-y plane. The C vector will then be
     * constrained.
     *
     * @param a The length (Angstroms) of vector A.
     * @param b The length (Angstroms) of vector B.
     * @param c The length (Angstroms) of vector C.
     * @param alpha The angle (degrees) between vectors B and C.
     * @param beta The angle (degrees) between vectors A and C.
     * @param gamma The angle (degrees) between vectors A and B.
     */
    void setCellParameters(double a, double b, double c,
                           double alpha, double beta, double gamma);

    /**
     * Set the cell matrix as column vectors.
     *
     * @param mat The 3x3 cell matrix to be set in column vector form.
     */
    void setCellMatrix(const Matrix3& mat) { m_cellMatrix = mat; };

    /**
     * Get the cell matrix as column vectors.
     *
     * @return The 3x3 cell matrix in column vector form.
     */
    Matrix3 cellMatrix() const { return m_cellMatrix; };

    /**
     * Get the cell matrix as row vectors.
     *
     * @return The 3x3 cell matrix in row vector form.
     */
    Matrix3 cellMatrixRowVecs() const { return m_cellMatrix.transpose(); };

    /**
     * Set the A vector.
     *
     * @param v The vector with which to set A.
     */
    void setAVector(const Vector3& v) { m_cellMatrix.col(0) = v; };

    /**
     * Set the B vector.
     *
     * @param v The vector with which to set B.
     */
    void setBVector(const Vector3& v) { m_cellMatrix.col(1) = v; };

    /**
     * Set the C vector.
     *
     * @param v The vector with which to set C.
     */
    void setCVector(const Vector3& v) { m_cellMatrix.col(2) = v; };

    /**
     * Set the cell vectors - a, b, and c.
     *
     * @param a The A vector.
     * @param b The B vector.
     * @param c The C vector.
     */
    void setCellVectors(const Vector3& a, const Vector3& b, const Vector3& c);

    /* Returns the A vector */
    Vector3 aVector() const { return m_cellMatrix.col(0); };

    /* Returns the B vector */
    Vector3 bVector() const { return m_cellMatrix.col(1); };

    /* Returns the C vector */
    Vector3 cVector() const { return m_cellMatrix.col(2); };

    /* Returns the length of A in Angstroms */
    double a() const { return m_cellMatrix.col(0).norm(); };

    /* Returns the length of B in Angstroms */
    double b() const { return m_cellMatrix.col(1).norm(); };

    /* Returns the length of C in Angstroms */
    double c() const { return m_cellMatrix.col(2).norm(); };

    /* Returns the angle (degrees) between B and C */
    double alpha() const { return angleDegrees(bVector(), cVector()); };

    /* Returns the angle (degrees) between A and C */
    double beta() const { return angleDegrees(aVector(), cVector()); };

    /* Returns the angle (degrees) between A and B */
    double gamma() const { return angleDegrees(aVector(), bVector()); };

    /* Returns the volume of the unit cell in Angstroms cubed */
    double volume() const;

    /**
     * Converts the @p frac vector into a Cartesian vector.
     *
     * @param cart The fractional vector to be converted.
     *
     * @return The Cartesian (Angstrom) vector.
     */
    Vector3 toCartesian(const Vector3& frac) const;

    /**
     * Converts the @p Cartesian vector into a fractional vector.
     *
     * @param cart The Cartesian vector (Angstroms) to be converted.
     *
     * @return The vector with fractional units.
     */
    Vector3 toFractional(const Vector3& cart) const;

    /**
     * Wrap Cartesian coordinates to be within the unit cell.
     *
     * @param cart The Cartesian (Angstrom) coordinates to be wrapped.
     *
     * @return The wrapped Cartesian coordinates.
     */
    Vector3 wrapCartesian(const Vector3& cart) const;

    /**
     * Wrap fractional coordinates to be within the unit cell.
     *
     * @param frac The fractional coordinates to be wrapped.
     *
     * @return The wrapped fractional coordinates.
     */
    Vector3 wrapFractional(const Vector3& frac) const;

    /**
     * Find the minimum image of a Cartesian vector @p cart.
     * A minimum image has all fractional coordinates between -0.5 and 0.5
     *
     * @param cart The Cartesian (Angstrom) vector whose minimum image we are
     *             finding.
     *
     * @return The minimum image of the Cartesian vector @p cart.
     */
    Vector3 minimumImage(const Vector3& cart) const;

    /**
     * Find the minimum fractional image of a fractional vector @p frac.
     * A minimum image has all fractional coordinates between -0.5 and 0.5.
     *
     * @param frac The fractional vector whose minimum image we are finding.
     *
     * @return The minimum image of the fractional vector @p frac.
     */
    static Vector3 minimumImageFractional(const Vector3& frac);

    /**
     * Find the shortest distance between vectors @a v1 and @a v2.
     *
     * @param v1 The first vector.
     * @param v2 The second vector.
     *
     * @return The shortest distance between the two vectors.
     */
    double distance(const Vector3& v1, const Vector3& v2) const;

    /**
     * Zeroes the unit cell.
     */
    void clear() { m_cellMatrix = Matrix3::Zero(); };

   private:

    /**
     * Get the unsigned angle in radians between two vectors.
     *
     * @param v1 The first vector.
     * @param v2 The second vector.
     *
     * @return The unsigned angle in radians between @p v1 and @p v2.
     */
    static double angle(const Vector3& v1, const Vector3& v2);

    /**
     * Get the unsigned angle in degrees between two vectors.
     *
     * @param v1 The first vector.
     * @param v2 The second vector.
     *
     * @return The unsigned angle in degrees between @p v1 and @p v2.
     */
    static double angleDegrees(const Vector3& v1, const Vector3& v2);

    Matrix3 m_cellMatrix;
  };


  inline UnitCell::UnitCell(const Matrix3& m)
    : m_cellMatrix(m)
  {
  }

  inline UnitCell::UnitCell(double a, double b, double c,
                            double alpha, double beta, double gamma)
    : m_cellMatrix(Matrix3())
  {
    setCellParameters(a, b, c, alpha, beta, gamma);
  }

  inline UnitCell::UnitCell(const UnitCell& other)
    : m_cellMatrix(other.m_cellMatrix)
  {
  }

  inline UnitCell& UnitCell::operator=(const UnitCell& other)
  {
    m_cellMatrix = other.m_cellMatrix;
    return *this;
  }

  // This probably shouldn't be inline, but I'd like to keep this
  // as a header-only file as long as I can.
  inline void UnitCell::setCellParameters(double a, double b,
                                          double c, double alpha,
                                          double beta, double gamma)
  {
    const double cosAlpha = std::cos(alpha * DEG2RAD);
    const double cosBeta  = std::cos(beta * DEG2RAD);
    const double cosGamma = std::cos(gamma * DEG2RAD);
    const double sinGamma = std::sin(gamma * DEG2RAD);

    m_cellMatrix(0, 0) = a;
    m_cellMatrix(1, 0) = 0.0;
    m_cellMatrix(2, 0) = 0.0;

    m_cellMatrix(0, 1) = b * cosGamma;
    m_cellMatrix(1, 1) = b * sinGamma;
    m_cellMatrix(2, 1) = 0.0;

    m_cellMatrix(0, 2) = c * cosBeta;
    m_cellMatrix(1, 2) = c * (cosAlpha - cosBeta * cosGamma) / sinGamma;
    m_cellMatrix(2, 2) = (c / sinGamma) * std::sqrt(
          1.0 - ((cosAlpha * cosAlpha) +
                 (cosBeta * cosBeta) +
                 (cosGamma * cosGamma)) +
          (2.0 * cosAlpha * cosBeta * cosGamma));
  }

  inline void UnitCell::setCellVectors(const Vector3& a, const Vector3& b,
                                       const Vector3& c)
  {
    setAVector(a);
    setBVector(b);
    setCVector(c);
  }

  inline double UnitCell::volume() const
  {
    return std::fabs(aVector().cross(bVector()).dot(cVector()));
  }

  inline Vector3 UnitCell::toCartesian(const Vector3& frac) const
  {
    return m_cellMatrix * frac;
  }

  inline Vector3 UnitCell::toFractional(const Vector3& cart) const
  {
    return m_cellMatrix.inverse() * cart;
  }

  inline Vector3 UnitCell::wrapCartesian(const Vector3& cart) const
  {
    return toCartesian(wrapFractional(toFractional(cart)));
  }

  inline Vector3 UnitCell::wrapFractional(const Vector3& frac) const
  {
    Vector3 ret = Vector3(std::fmod(frac[0], 1.0),
                          std::fmod(frac[1], 1.0),
                          std::fmod(frac[2], 1.0));
    if (ret[0] < 0.0)
      ++ret[0];
    if (ret[1] < 0.0)
      ++ret[1];
    if (ret[2] < 0.0)
      ++ret[2];
    return ret;
  }

  inline Vector3 UnitCell::minimumImage(const Vector3& cart) const
  {
    return toCartesian(minimumImageFractional(toFractional(cart)));
  }

  inline Vector3 UnitCell::minimumImageFractional(const Vector3& frac)
  {
    double x = frac[0] - rint(frac[0]);
    double y = frac[1] - rint(frac[1]);
    double z = frac[2] - rint(frac[2]);
    return Vector3(x, y, z);
  }

  inline double UnitCell::distance(const Vector3& v1, const Vector3& v2) const
  {
    return std::fabs(minimumImage(v1 - v2).norm());
  }

  inline double UnitCell::angle(const Vector3& v1, const Vector3& v2)
  {
    return std::acos(v1.dot(v2) / (v1.norm() * v2.norm()));
  }

  inline double UnitCell::angleDegrees(const Vector3& v1,
                                       const Vector3& v2)
  {
    return angle(v1, v2) * RAD2DEG;
  }

}

#endif
