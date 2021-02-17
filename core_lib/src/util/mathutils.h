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
        return 5.3;
    }

    inline qreal p2dGaussianBlur (qreal scl, qreal tcl, qreal w, qreal h, qreal r)
    {
        return 1;
    }
}
    /*
function gaussBlur_4 (scl, tcl, w, h, r) {
    var bxs = boxesForGauss(r, 3);
    boxBlur_4 (scl, tcl, w, h, (bxs[0]-1)/2);
    boxBlur_4 (tcl, scl, w, h, (bxs[1]-1)/2);
    boxBlur_4 (scl, tcl, w, h, (bxs[2]-1)/2);
}
function boxBlur_4 (scl, tcl, w, h, r) {
    for(var i=0; i<scl.length; i++) tcl[i] = scl[i];
    boxBlurH_4(tcl, scl, w, h, r);
    boxBlurT_4(scl, tcl, w, h, r);
}
function boxBlurH_4 (scl, tcl, w, h, r) {
    var iarr = 1 / (r+r+1);
    for(var i=0; i<h; i++) {
        var ti = i*w, li = ti, ri = ti+r;
        var fv = scl[ti], lv = scl[ti+w-1], val = (r+1)*fv;
        for(var j=0; j<r; j++) val += scl[ti+j];
        for(var j=0  ; j<=r ; j++) { val += scl[ri++] - fv       ;   tcl[ti++] = Math.round(val*iarr); }
        for(var j=r+1; j<w-r; j++) { val += scl[ri++] - scl[li++];   tcl[ti++] = Math.round(val*iarr); }
        for(var j=w-r; j<w  ; j++) { val += lv        - scl[li++];   tcl[ti++] = Math.round(val*iarr); }
    }
}
function boxBlurT_4 (scl, tcl, w, h, r) {
    var iarr = 1 / (r+r+1);
    for(var i=0; i<w; i++) {
        var ti = i, li = ti, ri = ti+r*w;
        var fv = scl[ti], lv = scl[ti+w*(h-1)], val = (r+1)*fv;
        for(var j=0; j<r; j++) val += scl[ti+j*w];
        for(var j=0  ; j<=r ; j++) { val += scl[ri] - fv     ;  tcl[ti] = Math.round(val*iarr);  ri+=w; ti+=w; }
        for(var j=r+1; j<h-r; j++) { val += scl[ri] - scl[li];  tcl[ti] = Math.round(val*iarr);  li+=w; ri+=w; ti+=w; }
        for(var j=h-r; j<h  ; j++) { val += lv      - scl[li];  tcl[ti] = Math.round(val*iarr);  li+=w; ti+=w; }
    }
}
*/


#endif // MATHUTILS_H
