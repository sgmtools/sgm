#ifndef SGM_POINT_H
#define SGM_POINT_H

#include <cassert>
#include <cstddef>
#include <vector>

#include "sgm_export.h"
#include "SGMEnums.h"
#include "SGMConstants.h"

///////////////////////////////////////////////////////////////////////////////
//
//  Point2D,3D, Vector2D,3D,4D, UnitVector2D,3D,4D and Ray3D 
//
///////////////////////////////////////////////////////////////////////////////

namespace SGM
{
    class Vector2D;
    class Vector3D;
    class Vector4D;
    class Transform3D;

    // Note that for performance reasons the basic data classes DO NOT
    // initialize their data members with the default constructor.

    ///////////////////////////////////////////////////////////////////////////
    //
    //  Point classes 
    //
    ///////////////////////////////////////////////////////////////////////////

    class SGM_EXPORT Point2D
    {
    public:

        enum { N = 2 };

        typedef double type;

        Point2D() {}; // Note that for performance things are intentionally left uninitialized.

        Point2D(double u,double v);

        const double& operator []( const size_t axis ) const { assert(axis < N); return (&m_u)[axis]; }
        double& operator []( const size_t axis )             { assert(axis < N); return (&m_u)[axis]; }

        double Distance(Point2D const &Pos) const;

        double DistanceSquared(Point2D const &Pos) const;

        // Orders by strict dictionary order, i.e. m_u first then m_v if the 
        // m_u's are equal.

        bool operator<(Point2D const &Pos) const;

        Point2D operator+=(Vector2D const &Vec);

        void Swap(Point2D &other);

        double m_u;
        double m_v;
    };

    inline bool operator==(Point2D const &A, Point2D const &B);
    inline bool operator!=(Point2D const &A, Point2D const &B);
    inline std::ostream& operator<<(std::ostream& os, const Point2D& P);

    class SGM_EXPORT Point3D
    {
    public:

        enum { N = 3 };

        typedef double type;

        Point3D() {}; // Note that for performance things are intentionally left uninitialized.

        Point3D(double x,double y,double z):m_x(x),m_y(y),m_z(z) {}

        explicit Point3D(Vector3D const &Vec);

        const double& operator []( const size_t axis ) const { assert(axis < N); return (&m_x)[axis]; }
        double& operator []( const size_t axis )             { assert(axis < N); return (&m_x)[axis]; }

        double Distance(Point3D const &Pos) const;

        double DistanceSquared(Point3D const &Pos) const;

        double DistanceFromOrigin() const;

        double DistanceFromOriginSquared() const;

        Point3D operator+=(Vector3D const &Vec);

        Point3D operator*=(Transform3D const &Trans);

        bool operator<(Point3D const &Pos) const;

        void Swap(Point3D &other);

        double m_x;
        double m_y;
        double m_z;
    };

    inline bool operator==(Point3D const &A, Point3D const &B);
    inline bool operator!=(Point3D const &A, Point3D const &B);
    inline std::ostream& operator<<(std::ostream& os, const Point3D& P);

    class SGM_EXPORT Point4D
    {
    public:

        enum { N = 4 };

        typedef double type;

        Point4D() {}; // Note that for performance things are intentionally left uninitialized.

        Point4D(double x,double y,double z,double w):m_x(x),m_y(y),m_z(z),m_w(w) {}

        const double& operator []( const size_t axis ) const { assert(axis < N); return (&m_x)[axis]; }
        double& operator []( const size_t axis )             { assert(axis < N); return (&m_x)[axis]; }

        Point4D operator+=(Vector4D const &Vec);

        double Distance(Point4D const &Pos) const;

        double DistanceSquared(Point4D const &Pos) const;

        bool operator<(Point4D const &Pos) const;

        void Swap(Point4D &other);

        double m_x;
        double m_y;
        double m_z;
        double m_w;
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    //  Vector classes 
    //
    ///////////////////////////////////////////////////////////////////////////

    class SGM_EXPORT Vector2D
    {
    public:

        enum { N = 2 };

        Vector2D() {}; // Note that for performance things are intentionally left uninitialized.

        Vector2D(double u,double v):m_u(u),m_v(v) {}

        Vector2D operator*(double dScale) const;

        void Swap(Vector2D &other);

        const double& operator []( const size_t axis ) const { assert(axis < N); return (&m_u)[axis]; }
        double& operator []( const size_t axis )             { assert(axis < N); return (&m_u)[axis]; }

        double m_u;
        double m_v;
    };

    class SGM_EXPORT Vector3D
    {
    public:

        enum { N = 3 };

        Vector3D() {}; // Note that for performance things are intentionally left uninitialized.

        Vector3D(double x,double y,double z):m_x(x),m_y(y),m_z(z) {}

        explicit Vector3D(Point3D const &Pos);

        double Magnitude() const;

        double MagnitudeSquared() const;

        Vector3D Orthogonal() const;

        Vector3D operator*(double dScale) const;

        Vector3D operator/(double dScale) const;

        Vector3D operator*=(Transform3D const &Trans);

        const double& operator []( const size_t axis ) const { assert(axis < N); return (&m_x)[axis]; }
        double& operator []( const size_t axis )             { assert(axis < N); return (&m_x)[axis]; }

        void Negate();

        void Swap(Vector3D &other);

        double m_x;
        double m_y;
        double m_z;
    };

    class SGM_EXPORT Vector4D
    {
    public:

        enum { N = 4 };

        Vector4D() {}; // Note that for performance things are intentionally left uninitialized.

        Vector4D(double x,double y,double z,double w):m_x(x),m_y(y),m_z(z),m_w(w) {}

        explicit Vector4D(Point4D const &Pos);

        Vector4D operator*(double dScale) const;

        Vector4D operator/(double dScale) const;

        void Swap(Vector4D &other);

        const double& operator []( const size_t axis ) const { assert(axis < N); return (&m_x)[axis]; }
        double& operator []( const size_t axis )             { assert(axis < N); return (&m_x)[axis]; }

        double m_x;
        double m_y;
        double m_z;
        double m_w;
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    //  Unit vector classes 
    //
    ///////////////////////////////////////////////////////////////////////////

    class SGM_EXPORT UnitVector2D : public Vector2D
    {
    public:

        UnitVector2D() {};

        UnitVector2D(double u,double v);

        UnitVector2D(Vector2D const &Vec);

        double U() const {return m_u;}

        double V() const {return m_v;}

    private:

        using Vector2D::m_u;
        using Vector2D::m_v;
        using Vector2D::operator[];
    };

    class SGM_EXPORT UnitVector3D : public Vector3D
    {
    public:

        UnitVector3D() {};

        UnitVector3D(double x,double y,double z);

        UnitVector3D(Vector3D const &Vec);

        // Returns the a number between zero and pi which is the angle between
        // this vector and the given vector.

        double Angle(UnitVector3D const &Vec) const;

        // Returns the a number between zero and 2*pi which is the angle between
        // this vector and the given vector with respect to the given normal 
        // vector in the right handed direction.

        double Angle(UnitVector3D const &Vec,
                     UnitVector3D const &Norm) const;

        UnitVector3D operator*=(Transform3D const &Trans);

        double X() const {return m_x;}

        double Y() const {return m_y;}

        double Z() const {return m_z;}

    private:

        using Vector3D::m_x;
        using Vector3D::m_y;
        using Vector3D::m_z;
        using Vector3D::operator[];
    };

    // make a unit vector in the direction A->B and return the length |B - A|

    inline double MakeUnitVector3D(Point3D const& A, Point3D const& B, UnitVector3D &U);

    class SGM_EXPORT UnitVector4D : public Vector4D
    {
    public:

        UnitVector4D() {};

        UnitVector4D(Vector4D const &Vec);

        double X() const {return m_x;}

        double Y() const {return m_y;}

        double Z() const {return m_z;}

        double W() const {return m_w;}

    private:

        using Vector4D::m_x;
        using Vector4D::m_y;
        using Vector4D::m_z;
        using Vector4D::m_w;
        using Vector4D::operator[];

    };

    ///////////////////////////////////////////////////////////////////////////
    //
    //  Ray class
    //
    ///////////////////////////////////////////////////////////////////////////

    class Ray3D
    {
    public:

        // The Ray is pure constant value class to store direction and origin, which
        // is used to implement fast ray, line, segment intersections with axis
        // aligned bounding boxes (Interval3D). This allows the inverse direction and
        // sign to be calculated only once in the constructor, and so we disallow
        // changing member variables so we do not violate the inverse direction and sign.

        Ray3D(const Point3D &orig, const UnitVector3D &dir);

        void Swap(Ray3D &other);

        Point3D      m_Origin;
        UnitVector3D m_Direction;

        Vector3D     m_InverseDirection;
        int          m_xSign;
        int          m_ySign;
        int          m_zSign;
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    //  Multi-class operators and functions
    //
    ///////////////////////////////////////////////////////////////////////////

    inline Point2D operator+(Point2D const &Pos,Vector2D const &Vec);

    inline Point2D operator-(Point2D const &Pos,Vector2D const &Vec);

    inline Vector2D operator-(Point2D const &Pos0,Point2D const &Pos1);

    inline Vector2D operator+(Vector2D const &Vec0,Vector2D const &Vec1);

    inline Vector2D operator-(Vector2D const &Vec0,Vector2D const &Vec1);

    inline Vector2D operator*(double dValue,Vector2D const &Vec);

    inline Point3D operator+(Point3D const &Pos,Vector3D const &Vec);

    inline Point3D operator-(Point3D const &Pos,Vector3D const &Vec);

    inline Vector3D operator-(Point3D const &Pos0,Point3D const &Pos1);

    inline Vector3D operator-(Vector3D const &Vec);

    inline Vector3D operator+(Vector3D const &Vec0,Vector3D const &Vec1);

    inline Vector3D operator-(Vector3D const &Vec0,Vector3D const &Vec1);

    inline Vector3D operator*(double dValue,Vector3D const &Vec);

    inline Vector3D operator*(Vector3D const &Vec0,Vector3D const &Vec1);

    inline UnitVector3D operator-(UnitVector3D const &UVec);

    inline Point4D operator+(Point4D const &Pos,Vector4D const &Vec);

    inline Point4D operator-(Point4D const &Pos,Vector4D const &Vec);

    inline Vector4D operator*(double dValue,Vector4D const &Vec);

    inline double operator%(Vector2D const &Vec0,Vector2D const &Vec1);

    inline double operator%(Vector3D const &Vec0,Vector3D const &Vec1);

    ///////////////////////////////////////////////////////////////////////////
    //
    //  MidPoint and NearEqual functions.
    //
    ///////////////////////////////////////////////////////////////////////////

    inline Point3D MidPoint(Point3D const &Pos0,Point3D const &Pos1,double dFraction=0.5);

    inline Point2D MidPoint(Point2D const &Pos0,Point2D const &Pos1,double dFraction=0.5);

    inline bool NearEqual(double d1,double d2,double dTolerance,bool bRelative);

    inline bool NearEqual(Point2D const &Pos1,Point2D const &Pos2,double dTolerance);

    inline bool NearEqual(Point3D const &Pos1,Point3D const &Pos2,double dTolerance);

    inline bool NearEqual(Point4D const &Pos1,Point4D const &Pos2,double dTolerance);

    inline bool NearEqual(Vector3D const &Vec1,Vector3D const &Vec2,double dTolerance);


    ///////////////////////////////////////////////////////////////////////////
    //
    // Functors with operator() for algorithms.
    //
    ///////////////////////////////////////////////////////////////////////////

    struct DistanceSquared
        {
        DistanceSquared() {}

        double operator()(SGM::Point2D const &A, SGM::Point2D const &B) const
            { return A.DistanceSquared(B); }

        double operator()(SGM::Point3D const &A, SGM::Point3D const &B) const
            { return A.DistanceSquared(B); }
        };

} // End of SGM namespace

#include "Inline/SGMVector.inl" // inline implementations

#endif //SGM_POINT_H
