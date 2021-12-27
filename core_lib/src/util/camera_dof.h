#ifndef CAMERA_DOF_H
#define CAMERA_DOF_H

#include <QtMath>

namespace Camera_DOF
{

    float hf_distance = 0.0;

    /** Get Hyperfocal distance
      *
     * \param B Focal length (50mm)
     * \param f f_stop Aperture (1,4 to 22 mm)
     * \return Hyperfocal Distance in millimeters
     */
    inline float getHyperfocalDistance(const float B, const float f)
    {
        return (B*B)/(f*0.03) + B;
    }

    /**  Get dof_near = Depth of Fiels Nearest
     *
     * \param B Focal length (50mm)
     * \param f f_stop Aperture (1,4 to 22 mm)
     * \param d distance setting on camera
     * \return dof_near in millimeters
     */
    inline float getDOF_Near(const float B, const float f, const float d)
    {
        hf_distance = getHyperfocalDistance(B, f);
        return ((hf_distance - B) * d)/(hf_distance + d - 2*B);
    }

    /**  Get dof_far = Depth of Fiels Nearest
     *
     * \param B Focal length (50mm)
     * \param d distance setting on camera
     * \return dof_near in millimeters
     */
    inline float getDOF_far(const float B, const float d)
    {
        return ((hf_distance - B) * d)/(hf_distance - d);
    }

}

#endif // CAMERA_DOF_H
