#ifndef MATHUTILS_H
#define MATHUTILS_H

#include <QtMath>
#include <QPoint>

namespace MathUtils
{
    /** Get the angle from the difference vector a->b to the x-axis.
     *
     * \param a Start point of vector
     * \param b End point of vector
     * \return Angle in radians from [-pi,+pi]
     */
    inline qreal getDifferenceAngle(const QPointF a, const QPointF b)
    {
        return qAtan2(b.y() - a.y(), b.x() - a.x());
    }

    /** Get the nearest distance, where the object will be i "acceptable" focus
     *
     * \param h is the Hyperfocal distance
     * \param s is the focus distance on the camera
     * \param fLength in the cameras focal length, can vary from 25-300 mm (standard is 50 mm)
     * @return
     */
    inline qreal getDistanceNear(qreal h, qreal s, qreal fLength)
    {
        return (s*(h-fLength))/(h+s-2*fLength);
    }

    /** Get the farthest distance, where the object will be i "acceptable" focus
     *
     * \param h is the Hyperfocal distance
     * \param s is the focus distance on the camera
     * \param fLength in the cameras focal length, can vary from 25-300 mm (standard is 50 mm)
     * @return
     */
    inline qreal getDistanceFar(qreal h, qreal s, qreal fLength)
    {
        return (s*(h-fLength))/(h-s);
    }

    /** Get hyperfocal distance using coc, focal length and f-number.
     *
     * Hyperfoacal distance is a distance beyond which,
     * all objects can be brought into an "acceptable" focus.
     * coc is Circle of confusion (=0.03)
     * \param fLength in the cameras focal length, can vary from 25-300 mm (standard is 50 mm)
     * \param aperture is aperture and can be 2.8 4 5.6 8 11 and 22
     * \return The cameras hyperfocal distance in mm
     */
    inline qreal getHyperfocalDistance(qreal fLength, qreal aperture)
    {
        return ((fLength*fLength)/(0.03*aperture)) + fLength;
    }

    inline qreal getCameraBlur()
    {
        return 0.3;
    }
}

#endif // MATHUTILS_H
