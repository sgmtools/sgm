#ifndef SGM_TRANSFORM_INL
#define SGM_TRANSFORM_INL

namespace SGM {

    inline Transform3D::Transform3D()
    {
        m_Matrix[0] = Vector4D(1, 0, 0, 0);
        m_Matrix[1] = Vector4D(0, 1, 0, 0);
        m_Matrix[2] = Vector4D(0, 0, 1, 0);
        m_Matrix[3] = Vector4D(0, 0, 0, 1);
    }

// Returns a transform that scales space by dS.

    inline Transform3D::Transform3D(double dS)
    {
        m_Matrix[0] = Vector4D(dS, 0, 0, 0);
        m_Matrix[1] = Vector4D(0, dS, 0, 0);
        m_Matrix[2] = Vector4D(0, 0, dS, 0);
        m_Matrix[3] = Vector4D(0, 0, 0, 1);
    }

// Returns a transform that translates space by the given vector.

    inline Transform3D::Transform3D(Vector3D
                                    const &Translate)
    {
        m_Matrix[0] = Vector4D(1, 0, 0, Translate.m_x);
        m_Matrix[1] = Vector4D(0, 1, 0, Translate.m_y);
        m_Matrix[2] = Vector4D(0, 0, 1, Translate.m_z);
        m_Matrix[3] = Vector4D(0, 0, 0, 1);
    }

// Returns a transform that scales by different amounts in each of
// the three coordinate axes.

    inline Transform3D::Transform3D(double dXScale,
                                    double dYScale,
                                    double dZScale)
    {
        m_Matrix[0] = Vector4D(dXScale, 0, 0, 0);
        m_Matrix[1] = Vector4D(0, dYScale, 0, 0);
        m_Matrix[2] = Vector4D(0, 0, dZScale, 0);
        m_Matrix[3] = Vector4D(0, 0, 0, 1);
    }

// Returns a transform that rotates space about the origin by
// moving the X, Y and Z axes to the given values.

    inline Transform3D::Transform3D(UnitVector3D const &XAxis,
                                    UnitVector3D const &YAxis,
                                    UnitVector3D const &ZAxis)
    {
        m_Matrix[0] = Vector4D(XAxis.m_x, YAxis.m_x, ZAxis.m_x, 0);
        m_Matrix[1] = Vector4D(XAxis.m_y, YAxis.m_y, ZAxis.m_y, 0);
        m_Matrix[2] = Vector4D(XAxis.m_z, YAxis.m_z, ZAxis.m_z, 0);
        m_Matrix[3] = Vector4D(0, 0, 0, 1);
    }

// Returns a transform that first rotates space and then translates.

    inline Transform3D::Transform3D(UnitVector3D const &XAxis,
                                    UnitVector3D const &YAxis,
                                    UnitVector3D const &ZAxis,
                                    Vector3D const &Translate
    )
    {
        m_Matrix[0] = Vector4D(XAxis.m_x, YAxis.m_x, ZAxis.m_x, Translate.m_x);
        m_Matrix[1] = Vector4D(XAxis.m_y, YAxis.m_y, ZAxis.m_y, Translate.m_y);
        m_Matrix[2] = Vector4D(XAxis.m_z, YAxis.m_z, ZAxis.m_z, Translate.m_z);
        m_Matrix[3] = Vector4D(0, 0, 0, 1);
    }

// Returns the four by four transform from all the data.

    inline Transform3D::Transform3D(Vector4D const &XAxis,
                                    Vector4D const &YAxis,
                                    Vector4D const &ZAxis,
                                    Vector4D const &Translate
    )
    {
        m_Matrix[0] = Vector4D(XAxis.m_x, YAxis.m_x, ZAxis.m_x, Translate.m_x);
        m_Matrix[1] = Vector4D(XAxis.m_y, YAxis.m_y, ZAxis.m_y, Translate.m_y);
        m_Matrix[2] = Vector4D(XAxis.m_z, YAxis.m_z, ZAxis.m_z, Translate.m_z);
        m_Matrix[3] = Vector4D(XAxis.m_w, YAxis.m_w, ZAxis.m_w, Translate.m_w);
    }

    inline double SGM::Transform3D::Scale(SGM::UnitVector3D const &Direction) const
    {
        return ((*this)*Vector3D(Direction)).Magnitude();
    }

    inline double SGM::Transform3D::Scale() const
    {
        double dX=Scale(SGM::UnitVector3D(1.0,0.0,0.0));
        double dY=Scale(SGM::UnitVector3D(0.0,1.0,0.0));
        double dZ=Scale(SGM::UnitVector3D(0.0,0.0,1.0));
        if(SGM::NearEqual(dX,dY,SGM_ZERO,false) && SGM::NearEqual(dZ,dY,SGM_ZERO,false))
            {
            return dX;
            }
        return 0.0;
    }


    ///////////////////////////////////////////////////////////////////////////
    //
    // Transform operators for Point, Vector, UnitVector
    //
    ///////////////////////////////////////////////////////////////////////////

    inline Point3D Point3D::operator*=(Transform3D const &Trans)
    {
        *this = Trans*(*this);
        return *this;
    }

    inline Vector3D Vector3D::operator*=(Transform3D const &Trans)
    {
        *this = Trans*(*this);
        return *this;
    }

    inline UnitVector3D UnitVector3D::operator*=(Transform3D const &Trans)
    {
        *this = Trans*(*this);
        return *this;
    }

} // namespace SGM

#endif //SGM_TRANSFORM_INL
