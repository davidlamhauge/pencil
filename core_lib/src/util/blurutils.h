#ifndef BLURUTILS_H
#define BLURUTILS_H

#include <QtMath>
#include <vector>
#include <QRect>
#include <QDebug>

using namespace std;

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
 * \param fLength in the cameras focal length, can vary from 30-300 mm (standard is 50 mm)
 * \param aperture is aperture and can be 2.8 4 5.6 8 11 and 22
 * \return The cameras hyperfocal distance in mm
 */
inline qreal getHyperfocalDistance(qreal fLength, qreal aperture)
{
    return ((fLength*fLength)/(0.03*aperture)) + fLength;
}

/**
 * @brief boxesForGauss
 * @param sigma
 * @param n
 * @return
 */
inline vector<int> boxesForGauss(float sigma, int n)  // standard deviation, number of boxes
{
    auto wIdeal = sqrt((12 * sigma*sigma / n) + 1);  // Ideal averaging filter width
    int wl = floor(wIdeal);
    if (wl % 2 == 0)
        wl--;
    int wu = wl + 2;

    auto mIdeal = (12 * sigma*sigma - n * wl*wl - 4 * n*wl - 3 * n) / (-4 * wl - 4);
    int m = round(mIdeal);
    // var sigmaActual = Math.sqrt( (m*wl*wl + (n-m)*wu*wu - n)/12 );

    vector<int> sizes(n);
    for (auto i = 0; i < n; i++)
        sizes[i] = i < m ? wl : wu;
    return sizes;
}

inline void boxBlurH_4(vector<unsigned char>& scl, vector<unsigned char>& tcl, int w, int h, int r) {
    float iarr = 1.f / (r + r + 1);
    for (auto i = 0; i < h; i++) {
        auto ti = i * w, li = ti, ri = ti + r;
        auto fv = scl[ti], lv = scl[ti + w - 1];
        auto val = (r + 1)*fv;
        for (auto j = 0; j < r; j++) val += scl[ti + j];
        for (auto j = 0; j <= r; j++) { val += scl[ri++] - fv;   tcl[ti++] = round(val*iarr); }
        for (auto j = r + 1; j < w - r; j++) { val += scl[ri++] - scl[li++];   tcl[ti++] = round(val*iarr); }
        for (auto j = w - r; j < w; j++) { val += lv - scl[li++];   tcl[ti++] = round(val*iarr); }
    }
}

inline void boxBlurT_4(vector<unsigned char>& scl, vector<unsigned char>& tcl, int w, int h, int r) {
    float iarr = 1.f / (r + r + 1);
    for (auto i = 0; i < w; i++) {
        auto ti = i, li = ti, ri = ti + r * w;
        auto fv = scl[ti], lv = scl[ti + w * (h - 1)];
        auto val = (r + 1)*fv;
        for (auto j = 0; j < r; j++) val += scl[ti + j * w];
        for (auto j = 0; j <= r; j++) { val += scl[ri] - fv;  tcl[ti] = round(val*iarr);  ri += w; ti += w; }
        for (auto j = r + 1; j < h - r; j++) { val += scl[ri] - scl[li];  tcl[ti] = round(val*iarr);  li += w; ri += w; ti += w; }
        for (auto j = h - r; j < h; j++) { val += lv - scl[li];  tcl[ti] = round(val*iarr);  li += w; ti += w; }
    }
}


inline void boxBlur_4(vector<unsigned char>& scl, vector<unsigned char>& tcl, int w, int h, int r) {
    for (unsigned i = 0; i < scl.size(); i++) tcl[i] = scl[i];
    boxBlurH_4(tcl, scl, w, h, r);
    boxBlurT_4(scl, tcl, w, h, r);
}

/**
 * @brief gaussBlur_4   The '4' because it is Algoritm 4 of fast gaussBlur:
 *                      http://blog.ivank.net/fastest-gaussian-blur.html
 * @param scl   Source channel
 * @param tcl   Target channel
 * @param w     Width
 * @param h     Height
 * @param r     Radius
 */
inline void gaussBlur_4(vector<unsigned char>& scl, vector<unsigned char>& tcl, int w, int h, qreal r)
{
    auto bxs = boxesForGauss(r, 3);
    qDebug() << "bxs: " << bxs ;
    boxBlur_4(scl, tcl, w, h, (bxs[0] - 1) / 2);
    boxBlur_4(tcl, scl, w, h, (bxs[1] - 1) / 2);
    boxBlur_4(scl, tcl, w, h, (bxs[2] - 1) / 2);
}


#endif // BLURUTILS_H
