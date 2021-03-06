#ifndef SGM_SEGMENT_H
#define SGM_SEGMENT_H

#include "sgm_export.h"
#include "SGMVector.h"

namespace SGM {

    ///////////////////////////////////////////////////////////////////////////
    //
    //  Segment classes for dimensions two and three.
    //
    ///////////////////////////////////////////////////////////////////////////

    class SGM_EXPORT Segment2D
    {
    public:

        Segment2D() = default;

        Segment2D(Point2D const &Start, 
                  Point2D const &End) :
            m_Start(Start), m_End(End) {}

        // Returns true, and the point of intersection, if this
        // segment and the given segment intersect.

        bool Intersect(Segment2D const &Seg,
                       Point2D         &Pos) const;

        // Returns true if the segments overlap at more than one point.

        bool Overlap(Segment2D const &Seg) const;

        // Returns true, and the point of intersection, if this
        // segment and the given ray intersect.

        bool Intersect(Point2D      const &RayOrigin,
                       UnitVector3D const &RayDirection,
                       Point2D            &Pos) const;

        // Returns the length of this segment squared, which is
        // faster than finding length.

        double LengthSquared() const;

        // Returns the length of this segment squared.

        double Length() const;

        // Returns the distance from Pos to this line segments.
        // In addition the function returns the closest point if 
        // pClosePoint points to a Point2D.

        double Distance(Point2D const &Pos,Point2D *pClosePoint=nullptr) const;

        double DistanceSquared(Point2D const &Pos,Point2D *pClosePoint=nullptr) const;

        Point2D m_Start;
        Point2D m_End;
    };

    class SGM_EXPORT Segment3D
    {
    public:

        Segment3D() = default;

        Segment3D(Point3D const &Start, Point3D const &End) :
                m_Start(Start), m_End(End)
        {}

        // Returns the length of this segment squared, which is
        // faster than finding length.

        double LengthSquared() const;

        // Returns the length of this segment squared.

        double Length() const;

        // Returns the closest pairs of points on the two lines defined by
        // this and the given segment.  If the two points are not on the two
        // segments, then false is returned.

        bool Intersect(Segment3D const &Seg,
                       Point3D         &Pos1,
                       Point3D         &Pos2,
                       double          *dS = nullptr,
                       double          *dT = nullptr) const;

        // Returns the distance between the two segments and the point, Pos1,
        // and the point, Pos2 on this segment and the given segment, that come
        // closest to each other.

        double DistanceBetween(Segment3D const &Seg,
                               Point3D         &Pos1,
                               Point3D         &Pos2) const;

        bool PointOnSegment(Point3D const &Pos,
                            double         dTolerance) const;

        Point3D ClosestPoint(Point3D const &Pos) const;

        Point3D m_Start;
        Point3D m_End;
    };

} // namespace SGM

#include "Inline/SGMSegment.inl"

#endif //SGM_SEGMENT_H
