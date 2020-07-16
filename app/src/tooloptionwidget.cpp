/*

Pencil - Traditional Animation Software
Copyright (C) 2005-2007 Patrick Corrieri & Pascal Naidon
Copyright (C) 2013-2018 Matt Chiawen Chang

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

*/
#include "tooloptionwidget.h"
#include "ui_tooloptions.h"

#include <QSettings>
#include <QDebug>

#include "spinslider.h"
#include "editor.h"
#include "util.h"
#include "layer.h"
#include "layermanager.h"
#include "toolmanager.h"

ToolOptionWidget::ToolOptionWidget(QWidget* parent) : BaseDockWidget(parent)
{
    setWindowTitle(tr("Options", "Window title of tool option panel like pen width, feather etc.."));

    QWidget* innerWidget = new QWidget;
    setWidget(innerWidget);
    ui = new Ui::ToolOptions;
    ui->setupUi(innerWidget);
}

ToolOptionWidget::~ToolOptionWidget()
{
    delete ui;
}

void ToolOptionWidget::initUI()
{
    QSettings settings(PENCIL2D, PENCIL2D);

    ui->sizeSlider->init(tr("Brush"), SpinSlider::EXPONENT, SpinSlider::INTEGER, 1, 200);
    ui->sizeSlider->setValue(settings.value("brushWidth", "3").toDouble());
    ui->brushSpinBox->setValue(settings.value("brushWidth", "3").toDouble());

    ui->featherSlider->init(tr("Feather"), SpinSlider::LOG, SpinSlider::INTEGER, 1, 99);
    ui->featherSlider->setValue(settings.value("brushFeather", "5").toDouble());
    ui->featherSpinBox->setValue(settings.value("brushFeather", "5").toDouble());

    if (settings.value("ToleranceFillOn", true).toBool())
    {
        ui->bucketFillSlider->init(tr("Color Tolerance"), SpinSlider::LINEAR, SpinSlider::INTEGER, 0, 100);
        ui->bucketFillSlider->setValue(settings.value("ToleranceFill", "50").toInt());
        ui->bucketFillSlider->setToolTip(tr("The extent to which the color variation will be treated as being equal"));
        ui->bucketFillSpinBox->setValue(settings.value("ToleranceFill", "50").toInt());
    }
    else
    {
        ui->bucketFillSlider->init(tr("Transparency level"), SpinSlider::LINEAR, SpinSlider::INTEGER, 0, 100);
        ui->bucketFillSlider->setValue(settings.value("TransparencyFill", "50").toInt());
        ui->bucketFillSlider->setToolTip(tr("The max transparency level of the line, to be filled"));
        ui->bucketFillSpinBox->setValue(settings.value("TransparencyFill", "50").toInt());
    }
}

void ToolOptionWidget::updateUI()
{
    BaseTool* currentTool = editor()->tools()->currentTool();
    Q_ASSERT(currentTool);

    disableAllOptions();

    ui->bucketFillCombo->clear();

    updateBucketComboItems(editor()->layers()->count());
    updateBucketCombo();

    setVisibility(currentTool);

    const Properties& p = currentTool->properties;

    setPenWidth(p.width);
    setPenFeather(p.feather);
    setUseFeather(p.useFeather);
    setPressure(p.pressure);
    setPenInvisibility(p.invisibility);
    setPreserveAlpha(p.preserveAlpha);
    setVectorMergeEnabled(p.vectorMergeEnabled);
    setAA(p.useAA);
    setStabilizerLevel(p.stabilizerLevel);

    QSettings settings(PENCIL2D, PENCIL2D);
    setToleranceOn(settings.value("ToleranceFillOn", true).toBool());
    setTolerance(settings.value("ToleranceFill", 50).toInt());
    setTransparencyFillOn(settings.value("TransparencyFillOn",false).toBool());
    setTransparency(settings.value("TransparencyFill", 50).toInt());
    setFillContour(p.useFillContour);

    ui->rbTolerance->setChecked(p.tolerance_on);
    ui->rbTransparency->setChecked(p.transparency_on);
}

void ToolOptionWidget::createUI()
{}

int ToolOptionWidget::getLayerIndex(QString name)
{
    if (name.isEmpty()) { return -1; }

    LayerManager* lMgr = editor()->layers();
    for (int i = 0; i < lMgr->count(); i++)
    {
        if (lMgr->getLayer(i)->name() == name)
            return i;
    }
    return  -1;
}

void ToolOptionWidget::makeConnectionToEditor(Editor* editor)
{
    auto toolManager = editor->tools();

    connect(ui->useBezierBox, &QCheckBox::clicked, toolManager, &ToolManager::setBezier);
    connect(ui->usePressureBox, &QCheckBox::clicked, toolManager, &ToolManager::setPressure);
    connect(ui->makeInvisibleBox, &QCheckBox::clicked, toolManager, &ToolManager::setInvisibility);
    connect(ui->preserveAlphaBox, &QCheckBox::clicked, toolManager, &ToolManager::setPreserveAlpha);

    connect(ui->sizeSlider, &SpinSlider::valueChanged, toolManager, &ToolManager::setWidth);
    connect(ui->featherSlider, &SpinSlider::valueChanged, toolManager, &ToolManager::setFeather);

    auto spinboxValueChanged = static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged);
    connect(ui->brushSpinBox, spinboxValueChanged, toolManager, &ToolManager::setWidth);
    clearFocusOnFinished(ui->brushSpinBox);
    connect(ui->featherSpinBox, spinboxValueChanged, toolManager, &ToolManager::setFeather);
    clearFocusOnFinished(ui->featherSpinBox);

    connect(ui->useFeatherBox, &QCheckBox::clicked, toolManager, &ToolManager::setUseFeather);

    connect(ui->vectorMergeBox, &QCheckBox::clicked, toolManager, &ToolManager::setVectorMergeEnabled);
    connect(ui->useAABox, &QCheckBox::clicked, toolManager, &ToolManager::setAA);

    connect(ui->inpolLevelsCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), toolManager, &ToolManager::setStabilizerLevel);

    connect(editor->layers(), &LayerManager::currentLayerChanged, this, &ToolOptionWidget::updateBucketCombo);
    connect(editor->layers(), &LayerManager::layerCountChanged, this, &ToolOptionWidget::updateBucketComboItems);
    connect(ui->rbTolerance, &QRadioButton::toggled, editor->tools(), &ToolManager::setToleranceOn);
    connect(ui->rbTransparency, &QRadioButton::toggled, editor->tools(), &ToolManager::setTransparencyFillOn);
    connect(ui->bucketFillCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ToolOptionWidget::updateBucketCombo);
    connect(ui->bucketFillSlider, &SpinSlider::valueChanged, this, &ToolOptionWidget::setBucketSlider);
    connect(ui->bucketFillSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &ToolOptionWidget::setBucketSlider);
    clearFocusOnFinished(ui->bucketFillSpinBox);

    connect(ui->fillContourBox, &QCheckBox::clicked, toolManager, &ToolManager::setUseFillContour);

    connect(toolManager, &ToolManager::toolChanged, this, &ToolOptionWidget::onToolChanged);
    connect(toolManager, &ToolManager::toolPropertyChanged, this, &ToolOptionWidget::onToolPropertyChanged);
}

void ToolOptionWidget::onToolPropertyChanged(ToolType, ToolPropertyType ePropertyType)
{
    const Properties& p = editor()->tools()->currentTool()->properties;

    switch (ePropertyType)
    {
    case WIDTH: setPenWidth(p.width); break;
    case FEATHER: setPenFeather(p.feather); break;
    case USEFEATHER: setUseFeather(p.useFeather); break;
    case PRESSURE: setPressure(p.pressure); break;
    case INVISIBILITY: setPenInvisibility(p.invisibility); break;
    case PRESERVEALPHA: setPreserveAlpha(p.preserveAlpha); break;
    case VECTORMERGE: setVectorMergeEnabled(p.vectorMergeEnabled); break;
    case ANTI_ALIASING: setAA(p.useAA); break;
    case STABILIZATION: setStabilizerLevel(p.stabilizerLevel); break;
    case EXPAND_FILL: setExpandFill(p.expandFill); break;
    case TOLERANCE_ON: setToleranceOn(p.tolerance_on); break;
    case TOLERANCE: setTolerance(static_cast<int>(p.tolerance)); break;
    case TRANSPARENCYFILL_ON: setTransparencyFillOn(p.transparency_on); break;
    case TRANSPARENCYFILL: setTransparency(static_cast<int>(p.transparency)); break;
    case FILLCONTOUR: setFillContour(p.useFillContour); break;
    case BEZIER: setBezier(p.bezier_state); break;
    default:
        Q_ASSERT(false);
        break;
    }
}

void ToolOptionWidget::setVisibility(BaseTool* tool)
{
    ui->sizeSlider->setVisible(tool->isPropertyEnabled(WIDTH));
    ui->brushSpinBox->setVisible(tool->isPropertyEnabled(WIDTH));
    ui->featherSlider->setVisible(tool->isPropertyEnabled(FEATHER));
    ui->featherSpinBox->setVisible(tool->isPropertyEnabled(FEATHER));
    ui->useFeatherBox->setVisible(tool->isPropertyEnabled(USEFEATHER));
    ui->useBezierBox->setVisible(tool->isPropertyEnabled(BEZIER));
    ui->usePressureBox->setVisible(tool->isPropertyEnabled(PRESSURE));
    ui->makeInvisibleBox->setVisible(tool->isPropertyEnabled(INVISIBILITY));
    ui->preserveAlphaBox->setVisible(tool->isPropertyEnabled(PRESERVEALPHA));
    ui->useAABox->setVisible(tool->isPropertyEnabled(ANTI_ALIASING));
    ui->stabilizerLabel->setVisible(tool->isPropertyEnabled(STABILIZATION));
    ui->inpolLevelsCombo->setVisible(tool->isPropertyEnabled(STABILIZATION));
    ui->bucketFillCombo->setVisible(tool->isPropertyEnabled(TOLERANCE) || tool->isPropertyEnabled(TRANSPARENCYFILL));
    ui->expandFillBox->setVisible(tool->isPropertyEnabled(TOLERANCE) || tool->isPropertyEnabled(TRANSPARENCYFILL));
    ui->rbTolerance->setVisible(tool->isPropertyEnabled(TOLERANCE) || tool->isPropertyEnabled(TRANSPARENCYFILL));
    ui->rbTransparency->setVisible(tool->isPropertyEnabled(TOLERANCE) || tool->isPropertyEnabled(TRANSPARENCYFILL));
    ui->bucketFillSlider->setVisible(tool->isPropertyEnabled(TOLERANCE) || tool->isPropertyEnabled(TRANSPARENCYFILL));
    ui->bucketFillSpinBox->setVisible(tool->isPropertyEnabled(TOLERANCE) || tool->isPropertyEnabled(TRANSPARENCYFILL));
    ui->fillContourBox->setVisible(tool->isPropertyEnabled(FILLCONTOUR));

    auto currentLayerType = editor()->layers()->currentLayer()->type();
    auto propertyType = editor()->tools()->currentTool()->type();

    if (currentLayerType == Layer::VECTOR)
    {
        switch (propertyType)
        {
        case SMUDGE:
            ui->sizeSlider->setVisible(false);
            ui->brushSpinBox->setVisible(false);
            ui->usePressureBox->setVisible(false);
            ui->featherSlider->setVisible(false);
            ui->featherSpinBox->setVisible(false);
            ui->useFeatherBox->setVisible(false);
            break;
        case PENCIL:
            ui->sizeSlider->setVisible(false);
            ui->brushSpinBox->setVisible(false);
            ui->usePressureBox->setVisible(false);
            break;
        case BUCKET:
            ui->sizeSlider->setLabel(tr("Stroke Thickness"));
            ui->bucketFillSlider->setVisible(false);
            ui->bucketFillSpinBox->setVisible(false);
            break;
        default:
            ui->sizeSlider->setLabel(tr("Width"));
            ui->bucketFillSlider->setVisible(false);
            ui->bucketFillSpinBox->setVisible(false);
            ui->useAABox->setVisible(false);
            break;
        }
    }
    else
    {
        switch (propertyType)
        {
        case PENCIL:
            ui->fillContourBox->setVisible(false);
            break;
        case BUCKET:
            ui->brushSpinBox->setVisible(false);
            ui->sizeSlider->setVisible(false);
            break;
        default:
            ui->makeInvisibleBox->setVisible(false);
            break;
        }
    }
}

void ToolOptionWidget::updateBucketCombo()
{
    if (editor()->tools()->currentTool()->type() != ToolType::BUCKET) { return; }

    if (ui->bucketFillCombo->currentIndex() < 0) { return; }

    QSignalBlocker b1(ui->expandFillBox);
    QSignalBlocker b2(ui->bucketFillCombo);
    int combo = ui->bucketFillCombo->currentIndex();
    QString name = editor()->layers()->findLayerByName(ui->bucketFillCombo->itemText(combo))->name();
    mCurrentComboLayerIndex = getLayerIndex(name);
    if (mCurrentComboLayerIndex == editor()->currentLayerIndex())
    {
        ui->expandFillBox->setChecked(false);
        ui->expandFillBox->setEnabled(false);
    }
    else
    {
        ui->expandFillBox->setEnabled(true);
    }

    if (editor()->layers()->currentLayer()->type() != Layer::BITMAP)
    {
        ui->bucketLabel->setVisible(false);
        ui->bucketFillCombo->setVisible(false);
        ui->expandFillBox->setVisible(false);
        ui->rbTolerance->setVisible(false);
        ui->rbTransparency->setVisible(false);
        ui->bucketFillSlider->setVisible(false);
        ui->bucketFillSpinBox->setVisible(false);
    }
    else
    {
        ui->bucketLabel->setVisible(true);
        ui->bucketFillCombo->setVisible(true);
        ui->expandFillBox->setVisible(true);
        ui->rbTolerance->setVisible(true);
        ui->rbTransparency->setVisible(true);
        ui->bucketFillSlider->setVisible(true);
        ui->bucketFillSpinBox->setVisible(true);
    }
}

void ToolOptionWidget::updateBucketComboItems(int count)
{
    QSignalBlocker b (ui->bucketFillCombo);
    ui->bucketFillCombo->clear();
    for (int i = 0; i < count; i++)
    {
        if (editor()->layers()->getLayer(i)->type() == Layer::BITMAP)
            ui->bucketFillCombo->addItem(editor()->layers()->getLayer(i)->name());
    }
}

void ToolOptionWidget::onToolChanged(ToolType)
{
    updateUI();
}

void ToolOptionWidget::setPenWidth(qreal width)
{
    QSignalBlocker b(ui->sizeSlider);
    ui->sizeSlider->setEnabled(true);
    ui->sizeSlider->setValue(width);

    QSignalBlocker b2(ui->brushSpinBox);
    ui->brushSpinBox->setEnabled(true);
    ui->brushSpinBox->setValue(width);
}

void ToolOptionWidget::setPenFeather(qreal featherValue)
{
    QSignalBlocker b(ui->featherSlider);
    ui->featherSlider->setEnabled(true);
    ui->featherSlider->setValue(featherValue);

    QSignalBlocker b2(ui->featherSpinBox);
    ui->featherSpinBox->setEnabled(true);
    ui->featherSpinBox->setValue(featherValue);
}

void ToolOptionWidget::setUseFeather(bool useFeather)
{
    QSignalBlocker b(ui->useFeatherBox);
    ui->useFeatherBox->setEnabled(true);
    ui->useFeatherBox->setChecked(useFeather);
}

void ToolOptionWidget::setPenInvisibility(int x)
{
    QSignalBlocker b(ui->makeInvisibleBox);
    ui->makeInvisibleBox->setEnabled(true);
    ui->makeInvisibleBox->setChecked(x > 0);
}

void ToolOptionWidget::setPressure(int x)
{
    QSignalBlocker b(ui->usePressureBox);
    ui->usePressureBox->setEnabled(true);
    ui->usePressureBox->setChecked(x > 0);
}

void ToolOptionWidget::setPreserveAlpha(int x)
{
    QSignalBlocker b(ui->preserveAlphaBox);
    ui->preserveAlphaBox->setEnabled(true);
    ui->preserveAlphaBox->setChecked(x > 0);
}

void ToolOptionWidget::setVectorMergeEnabled(int x)
{
    QSignalBlocker b(ui->vectorMergeBox);
    ui->vectorMergeBox->setEnabled(true);
    ui->vectorMergeBox->setChecked(x > 0);
}

void ToolOptionWidget::setAA(int x)
{
    QSignalBlocker b(ui->useAABox);
    ui->useAABox->setEnabled(true);
    ui->useAABox->setVisible(false);

    auto layerType = editor()->layers()->currentLayer()->type();

    if (layerType == Layer::BITMAP)
    {
        if (x == -1)
        {
            ui->useAABox->setEnabled(false);
            ui->useAABox->setVisible(false);
        }
        else
        {
            ui->useAABox->setVisible(true);
        }
        ui->useAABox->setChecked(x > 0);
    }
}

void ToolOptionWidget::setStabilizerLevel(int x)
{
    ui->inpolLevelsCombo->setCurrentIndex(qBound(0, x, ui->inpolLevelsCombo->count() - 1));
}

void ToolOptionWidget::setExpandFill(bool)
{
    QSettings settings(PENCIL2D, PENCIL2D);
}

void ToolOptionWidget::setToleranceOn(bool b)
{
    QSignalBlocker b1(ui->bucketFillSlider);
    QSignalBlocker b2(ui->bucketFillSpinBox);

    QSettings settings(PENCIL2D, PENCIL2D);
    ui->bucketFillSlider->setLabel(tr("Color Tolerance"));
    ui->bucketFillSlider->setValue(settings.value(SETTING_TOLERANCE_FILL, 50).toInt());
    ui->bucketFillSlider->setToolTip(tr("The extent to which the color variation will be treated as being equal"));
    ui->bucketFillSpinBox->setValue(settings.value(SETTING_TOLERANCE_FILL, 50).toInt());
    settings.setValue("ToleranceOn", b);
    settings.setValue("TransparencyFillOn", !b);
}

void ToolOptionWidget::setTolerance(int tolerance)
{
    QSignalBlocker b(ui->bucketFillSlider);
    ui->bucketFillSlider->setEnabled(true);
    ui->bucketFillSlider->setValue(tolerance);

    QSignalBlocker b2(ui->bucketFillSpinBox);
    ui->bucketFillSpinBox->setEnabled(true);
    ui->bucketFillSpinBox->setValue(tolerance);
}

void ToolOptionWidget::setTransparencyFillOn(bool b)
{
    QSignalBlocker b1(ui->bucketFillSlider);
    QSignalBlocker b2(ui->bucketFillSpinBox);

    QSettings settings(PENCIL2D, PENCIL2D);
    ui->bucketFillSlider->setLabel(tr("Transparency level"));
    ui->bucketFillSlider->setValue(settings.value(SETTING_TRANSPARENCY_FILL, 50).toInt());
    ui->bucketFillSlider->setToolTip(tr("The max transparency level, of the line to be filled"));
    ui->bucketFillSpinBox->setValue(settings.value(SETTING_TRANSPARENCY_FILL, 50).toInt());

    settings.setValue("ToleranceOn", !b);
    settings.setValue("TransparencyFillOn", b);
}

void ToolOptionWidget::setTransparency(int transparency)
{
    QSignalBlocker b(ui->bucketFillSlider);
    ui->bucketFillSlider->setEnabled(true);
    ui->bucketFillSlider->setValue(transparency);

    QSignalBlocker b2(ui->bucketFillSpinBox);
    ui->bucketFillSpinBox->setEnabled(true);
    ui->bucketFillSpinBox->setValue(transparency);
}

void ToolOptionWidget::setBucketSlider(int index)
{
    QSettings settings(PENCIL2D, PENCIL2D);
    if (settings.value(SETTING_TOLERANCE_FILL_ON).toBool())
    {
        editor()->tools()->setTolerance(index);
    }
    else
    {
        editor()->tools()->setTransparencyFill(index);
    }
}

void ToolOptionWidget::setFillContour(int useFill)
{
    QSignalBlocker b(ui->fillContourBox);
    ui->fillContourBox->setEnabled(true);
    ui->fillContourBox->setChecked(useFill > 0);
}

void ToolOptionWidget::setBezier(bool useBezier)
{
    QSignalBlocker b(ui->useBezierBox);
    ui->useBezierBox->setChecked(useBezier);
}

void ToolOptionWidget::disableAllOptions()
{
    ui->sizeSlider->hide();
    ui->brushSpinBox->hide();
    ui->featherSlider->hide();
    ui->featherSpinBox->hide();
    ui->useFeatherBox->hide();
    ui->useBezierBox->hide();
    ui->usePressureBox->hide();
    ui->makeInvisibleBox->hide();
    ui->preserveAlphaBox->hide();
    ui->vectorMergeBox->hide();
    ui->useAABox->hide();
    ui->inpolLevelsCombo->hide();
    ui->bucketLabel->hide();
    ui->bucketFillCombo->hide();
    ui->rbTolerance->hide();
    ui->rbTransparency->hide();
    ui->bucketFillSlider->hide();
    ui->bucketFillSpinBox->hide();
    ui->fillContourBox->hide();
}
