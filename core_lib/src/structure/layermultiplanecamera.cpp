#include "layermultiplanecamera.h"

#include <QDebug>

LayerMultiPlaneCamera::LayerMultiPlaneCamera(Object *object) :LayerCamera(object)
{
    setType(LAYER_TYPE::MULTIPLANECAMERA);
    setName(tr("Multiplane Camera"));
    QRect viewRect = getViewRect();
    qDebug() << "viewRect: " << viewRect;
}

LayerMultiPlaneCamera::~LayerMultiPlaneCamera()
{

}
