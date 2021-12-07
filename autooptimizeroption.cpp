
#include "autooptimizeroption.h"
#include "ui_autooptimizeroption.h"
#include "indicator.h"

class AutoOptimizerOption::PrivateData
{
    public:
        Ui::AutoOptimizerOptionUI form;
        Indicator *indicator;
        AutoOptimizerOption::variableTypes varType;
        AutoOptimizerOption::optionTypes optionType;
        unsigned long optionAddress;
        QVariant originalSetting;
        ui4vector originalSettingIntVec;
        f4vector originalSettingFltVec;
        QVector<Dependency *> dependencyList;
        mutable QList<const AutoOptimizerOption *> dependentList;
        AutoOptimizerOption *dependencyOption;
        QVariant dependencyValue;
        mutable quint8 dependenciesChanged;
        bool enabled;
        bool dependenciesForfilled;
        QString Title;
        QStringList valueTitles;
        bool hasDependency;
        bool percentualNotation;
        bool simdOption;
        bool autoAdjust;
        int autoAdjustPeriods;
};

AutoOptimizerOption::AutoOptimizerOption(bool *option, Indicator *indicator, const QStringList &valueTitles)
{
    setup();

    privateData->indicator = indicator;
    privateData->optionAddress = (unsigned long) option;
    privateData->originalSetting = *option;
    privateData->varType = AutoOptimizerOption::Boolean;
    privateData->valueTitles = valueTitles;
    privateData->percentualNotation = false;
    privateData->form.integerWidget->hide();
    privateData->form.doubleWidget->hide();

    if (valueTitles.isEmpty())
        privateData->form.integerTitleWidget->hide();
    else
    {
        privateData->form.booleanWidget->hide();

        QGridLayout *layout = static_cast<QGridLayout *>(privateData->form.integerTitleWidget->layout());
        int column = 0;
        layout->setColumnStretch(2, 10);
        for (int i = 0; i < 2; ++i)
        {
            QCheckBox *checkBox = new QCheckBox(valueTitles.at(i));
            checkBox->setChecked(true);
            layout->addWidget(checkBox, 0, column);
            connect(checkBox, SIGNAL(toggled(bool)), this, SLOT(slotRelayUpdateGuiSignal()));
            column++;
        }
    }
}

AutoOptimizerOption::AutoOptimizerOption(int *option, Indicator *indicator, const QStringList &valueTitles)
{
    setup();

    privateData->indicator = indicator;
    privateData->optionAddress = (unsigned long) option;
    privateData->originalSetting = *option;
    privateData->varType = AutoOptimizerOption::Integer;
    privateData->valueTitles = valueTitles;
    privateData->percentualNotation = false;
    privateData->form.booleanWidget->hide();
    privateData->form.doubleWidget->hide();

    if (valueTitles.isEmpty())
        privateData->form.integerTitleWidget->hide();
    else
    {
        privateData->form.integerWidget->hide();

        QGridLayout *layout = static_cast<QGridLayout *>(privateData->form.integerTitleWidget->layout());
        int column = 0;
        int row = 0;
        layout->setColumnStretch(5, 10);
        for (int i = 0; i < valueTitles.size(); ++i)
        {
            QCheckBox *checkBox = new QCheckBox(valueTitles.at(i));
            checkBox->setChecked(true);
            layout->addWidget(checkBox, row, column);
            connect(checkBox, SIGNAL(toggled(bool)), this, SLOT(slotRelayUpdateGuiSignal()));
            column++;
            if (column == 5)
            {
                column = 0;
                row++;
            }
        }
    }
}

AutoOptimizerOption::AutoOptimizerOption(float *option, Indicator *indicator, bool percentualNotation)
{
    setup();

    privateData->indicator = indicator;
    privateData->optionAddress = (unsigned long) option;
    privateData->originalSetting = *option;
    privateData->varType = AutoOptimizerOption::Double;
    privateData->percentualNotation = percentualNotation;
    privateData->form.booleanWidget->hide();
    privateData->form.integerWidget->hide();
    privateData->form.integerTitleWidget->hide();

    if (percentualNotation)
    {
        privateData->form.doubleMinDoubleSpinBox->setMinimum(0.0);
        privateData->form.doubleMinDoubleSpinBox->setMaximum(100.0);
        privateData->form.doubleMinDoubleSpinBox->setSuffix(" %");
        privateData->form.doubleMaxDoubleSpinBox->setMinimum(0.0);
        privateData->form.doubleMaxDoubleSpinBox->setMaximum(100.0);
        privateData->form.doubleMaxDoubleSpinBox->setSuffix(" %");
        privateData->form.doubleStepSizeDoubleSpinBox->setMinimum(0.0);
        privateData->form.doubleStepSizeDoubleSpinBox->setMaximum(100.0);
        privateData->form.doubleStepSizeDoubleSpinBox->setSuffix(" %");
    }
}

AutoOptimizerOption::~AutoOptimizerOption()
{
    delete privateData;
}

void *AutoOptimizerOption::pointer(void) const
{
    return (void *) privateData->optionAddress;
}
void AutoOptimizerOption::setup(void)
{
    privateData = new PrivateData;
    privateData->dependencyOption = NULL;
    privateData->hasDependency = false;
    privateData->dependenciesChanged = 0b11111111;
    privateData->enabled = true;
    privateData->dependenciesForfilled = true;
    privateData->optionType = AutoOptimizerOption::IndicatorOption;
    privateData->simdOption = false;
    privateData->autoAdjust = false;
    privateData->autoAdjustPeriods = 0;
    privateData->form.setupUi(this);

    connect(privateData->form.booleanOffCheckBox, SIGNAL(toggled(bool)), this, SLOT(slotRelayUpdateGuiSignal()));
    connect(privateData->form.booleanOnCheckBox, SIGNAL(toggled(bool)), this, SLOT(slotRelayUpdateGuiSignal()));
    connect(privateData->form.doubleAccelerationCheckBox, SIGNAL(toggled(bool)), this, SLOT(slotRelayUpdateGuiSignal()));
    connect(privateData->form.doubleMaxDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(slotRelayUpdateGuiSignal()));
    connect(privateData->form.doubleMinDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(slotRelayUpdateGuiSignal()));
    connect(privateData->form.doubleStepSizeDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(slotRelayUpdateGuiSignal()));
    connect(privateData->form.integerAccelerationCheckBox, SIGNAL(toggled(bool)), this, SLOT(slotRelayUpdateGuiSignal()));
    connect(privateData->form.integerMaxSpinBox, SIGNAL(valueChanged(int)), this, SLOT(slotRelayUpdateGuiSignal()));
    connect(privateData->form.integerMinSpinBox, SIGNAL(valueChanged(int)), this, SLOT(slotRelayUpdateGuiSignal()));
    connect(privateData->form.integerStepSizeSpinBox, SIGNAL(valueChanged(int)), this, SLOT(slotRelayUpdateGuiSignal()));
}

QString AutoOptimizerOption::title(void) const
{
    return privateData->Title;
}

void AutoOptimizerOption::setTitle(const QString &title)
{
    privateData->Title = title;
}

AutoOptimizerOption::variableTypes AutoOptimizerOption::varType(void) const
{
    return privateData->varType;
}

AutoOptimizerOption::optionTypes AutoOptimizerOption::type(void) const
{
    return privateData->optionType;
}

void AutoOptimizerOption::setType(AutoOptimizerOption::optionTypes type)
{
    privateData->optionType = type;
}

QVariant AutoOptimizerOption::value(int simdAddress) const
{
    switch (varType())
    {
        case AutoOptimizerOption::Boolean:
            return QVariant(static_cast<float>(*((bool *)privateData->optionAddress)));

        case AutoOptimizerOption::Integer:
            return QVariant(static_cast<float>(*((int *)privateData->optionAddress)));

        case AutoOptimizerOption::Double:
            return QVariant(static_cast<float>(*((float *)privateData->optionAddress)));

        default:
            return QVariant(static_cast<float>(*((int *)privateData->optionAddress)));
    }

    return QVariant(0.0f);
}

void AutoOptimizerOption::setValue(const QVariant &value, int simdAddress)
{
    if (privateData->simdOption)
    {
        bool setAll = false;
        if (simdAddress < 0)
            simdAddress = 0;
        else if (simdAddress > 3)
        {
            setAll = true;
            simdAddress = 3;
        }

        switch (varType())
        {
            case AutoOptimizerOption::Boolean:
                if (setAll)
                    ((ui4vector *) privateData->optionAddress)->v = _mm_set1_epi32(value.toBool() ? 0xffffffff : 0x00000000);
                else
                    ((ui4vector *) privateData->optionAddress)->i[ simdAddress ] = value.toBool() ? 0xffffffff : 0x00000000;
                break;

            case AutoOptimizerOption::Integer:
                if (setAll)
                    ((ui4vector *) privateData->optionAddress)->v = _mm_set1_epi32(value.toInt());
                else
                    ((ui4vector *) privateData->optionAddress)->i[ simdAddress ] = value.toInt();
                break;

            case AutoOptimizerOption::Double:
                if (setAll)
                    ((f4vector *) privateData->optionAddress)->v = _mm_set1_ps(value.toFloat());
                else
                    ((f4vector *) privateData->optionAddress)->f[ simdAddress ] = value.toFloat();
                break;

            default:
                if (setAll)
                    ((ui4vector *) privateData->optionAddress)->v = _mm_set1_epi32(value.toInt());
                else
                    ((ui4vector *) privateData->optionAddress)->i[ simdAddress ] = value.toInt();
                break;
        }
    }
    else
    {
        switch (varType())
        {
            case AutoOptimizerOption::Boolean:
                * ((bool *) privateData->optionAddress) = value.toBool();
                break;

            case AutoOptimizerOption::Integer:
                * ((int *) privateData->optionAddress) = value.toInt();
                break;

            case AutoOptimizerOption::Double:
                * ((float *) privateData->optionAddress) = value.toFloat();
                break;

            default:
                * ((int *) privateData->optionAddress) = value.toInt();
                break;
        }
    }

    QList<const AutoOptimizerOption *>::const_iterator optionsIt = privateData->dependentList.constBegin();
    const QList<const AutoOptimizerOption *>::const_iterator optionsEnd = privateData->dependentList.constEnd();
    for (; optionsIt != optionsEnd; ++optionsIt)
    {
        const AutoOptimizerOption *option = *optionsIt;
        option->setDependenciesChanged();
    }

    emit valueChanged(value);
}

QVariant AutoOptimizerOption::min(void) const
{
    switch (varType())
    {
        case AutoOptimizerOption::Boolean:
            if (privateData->valueTitles.isEmpty())
            {
                if (privateData->form.booleanOffCheckBox->isChecked())
                    return QVariant(0.0f);
                else
                    return QVariant(1.0f);
            }
            else
            {
                for (int i = 0; i < privateData->valueTitles.size(); ++ i)
                {
                    if (valueCheckBoxEnabled(i))
                        return QVariant(static_cast<float>(i));
                }
            }

        case AutoOptimizerOption::Integer:
            if (privateData->valueTitles.isEmpty())
                return QVariant(static_cast<float>(privateData->form.integerMinSpinBox->value()));
            else
            {
                for (int i = 0; i < privateData->valueTitles.size(); ++ i)
                {
                    if (valueCheckBoxEnabled(i))
                        return QVariant(static_cast<float>(i));
                }
            }

            return QVariant(static_cast<float>(privateData->form.integerMinSpinBox->value()));

        case AutoOptimizerOption::Double:
            return QVariant(privateData->percentualNotation ? static_cast<float>(privateData->form.doubleMinDoubleSpinBox->value()) / 100.0f : static_cast<float>(privateData->form.doubleMinDoubleSpinBox->value()));

        default:
            return QVariant(static_cast<float>(privateData->form.integerMinSpinBox->value()));
    }

    return QVariant(static_cast<float>(privateData->form.integerMinSpinBox->value()));
}

void AutoOptimizerOption::setMin(const QVariant &min)
{
    switch (varType())
    {
        case AutoOptimizerOption::Boolean:
            if (min.toInt() <= 0)
                privateData->form.booleanOffCheckBox->setChecked(true);
            else /*if ( min.toInt() >= 1 )*/
                privateData->form.booleanOffCheckBox->setChecked(false);

            if (!privateData->valueTitles.isEmpty())
            {
                for (int i = 0; i < min.toInt(); ++i)
                    setValueCheckBoxEnabled(i, false);

                for (int i = min.toInt(); i <= max().toInt(); ++i)
                    setValueCheckBoxEnabled(i, true);
            }

            break;

        case AutoOptimizerOption::Integer:
            privateData->form.integerMinSpinBox->setValue(min.toInt());
            if (!privateData->valueTitles.isEmpty())
            {
                for (int i = 0; i < min.toInt(); ++i)
                    setValueCheckBoxEnabled(i, false);

                for (int i = min.toInt(); i <= max().toInt(); ++i)
                    setValueCheckBoxEnabled(i, true);
            }

            break;

        case AutoOptimizerOption::Double:
            privateData->form.doubleMinDoubleSpinBox->setValue(privateData->percentualNotation ? min.toFloat() * 100.0 : min.toFloat());
            break;

        default:
            privateData->form.integerMinSpinBox->setValue(min.toInt());
            break;
    }

    emit guiTouched();
}

QVariant AutoOptimizerOption::max(void) const
{
    switch (varType())
    {
        case AutoOptimizerOption::Boolean:
            if (privateData->valueTitles.isEmpty())
            {
                if (privateData->form.booleanOnCheckBox->isChecked())
                    return QVariant(1.0f);
                else
                    return QVariant(0.0f);
            }
            else
            {
                for (int i = privateData->valueTitles.size() - 1; i >= 0 ; --i)
                {
                    if (valueCheckBoxEnabled(i))
                        return QVariant(static_cast<float>(i));
                }
            }

        case AutoOptimizerOption::Integer:
            if (privateData->valueTitles.isEmpty())
                return QVariant(static_cast<float>(privateData->form.integerMaxSpinBox->value()));
            else
            {
                for (int i = privateData->valueTitles.size() - 1; i >= 0 ; --i)
                {
                    if (valueCheckBoxEnabled(i))
                        return QVariant(static_cast<float>(i));
                }
            }

            return QVariant(static_cast<float>(privateData->form.integerMaxSpinBox->value()));

        case AutoOptimizerOption::Double:
            return QVariant(privateData->percentualNotation ? static_cast<float>(privateData->form.doubleMaxDoubleSpinBox->value()) / 100.0f : static_cast<float>(privateData->form.doubleMaxDoubleSpinBox->value()));

        default:
            return QVariant(static_cast<float>(privateData->form.integerMaxSpinBox->value()));
    }

    return QVariant(static_cast<float>(privateData->form.integerMaxSpinBox->value()));
}

void AutoOptimizerOption::setMax(const QVariant &max)
{
    switch (varType())
    {
        case AutoOptimizerOption::Boolean:
            if (max.toInt() >= 1)
                privateData->form.booleanOnCheckBox->setChecked(true);
            else /*if ( max.toInt() == 0 )*/
                privateData->form.booleanOnCheckBox->setChecked(false);

            if (!privateData->valueTitles.isEmpty())
            {
                for (int i = min().toInt(); i <= max.toInt(); ++i)
                    setValueCheckBoxEnabled(i, true);

                for (int i = max.toInt() + 1; i < privateData->valueTitles.size(); ++i)
                    setValueCheckBoxEnabled(i, false);
            }

            break;

        case AutoOptimizerOption::Integer:
            privateData->form.integerMaxSpinBox->setValue(max.toInt());
            if (!privateData->valueTitles.isEmpty())
            {
                for (int i = min().toInt(); i <= max.toInt(); ++i)
                    setValueCheckBoxEnabled(i, true);

                for (int i = max.toInt() + 1; i < privateData->valueTitles.size(); ++i)
                    setValueCheckBoxEnabled(i, false);
            }

            break;

        case AutoOptimizerOption::Double:
            privateData->form.doubleMaxDoubleSpinBox->setValue(privateData->percentualNotation ? max.toFloat() * 100.0 : max.toFloat());
            break;

        default:
            privateData->form.integerMaxSpinBox->setValue(max.toInt());
            break;
    }

    emit guiTouched();
}

void AutoOptimizerOption::updateAutoAdjustRanges(void)
{
    if (varType() != AutoOptimizerOption::Boolean && !autoAdjustEnabled())
        return;

    const float *data = indicator()->priceChart->averagePriceClose->fastData();
    if (!data)
        return;

    float totalMovement = 0;
    int periods = indicator()->priceChart->indexPlot->periods;
    for (int i = 1; i < periods - 1; i++)
    {
        if (privateData->percentualNotation)
        {
            if (data[i] > data[i - 1])
                totalMovement += (data[i] / data[i - 1]) - 1.0;
            else
                totalMovement += (data[i - 1] / data[i]) - 1.0;
        }
        else
        {
            if (data[i] > data[i - 1])
                totalMovement += (data[i] - data[i - 1]);
            else
                totalMovement += (data[i - 1] - data[i]);
        }
    }

    float averageMovement = totalMovement / periods;
    float stepSize = 0.0000025;
    for (float f = 250.0; f >= 0.0000025; f /= 10.0)
    {
        stepSize = averageMovement / f;
        if (stepSize > 1.0 && stepSize < 10.0)
        {
            stepSize = f;
            break;
        }
    }

    float range = stepSize * (float)autoAdjustPeriods();

    setMax(min().toFloat() + range);
    setStepSize(stepSize);
}

bool AutoOptimizerOption::autoAdjustEnabled(void) const
{
    return privateData->autoAdjust;
}

void AutoOptimizerOption::setAutoAdjustEnabled(bool enabled)
{
    switch (varType())
    {
        case AutoOptimizerOption::Boolean:
        case AutoOptimizerOption::Integer:
            privateData->autoAdjust = false;
            break;

        case AutoOptimizerOption::Double:
            privateData->autoAdjust = enabled;
            break;

        default:
            privateData->autoAdjust = false;
            break;
    }

    if (autoAdjustEnabled())
    {
        connect(indicator()->priceChart, SIGNAL(dataSet()), this, SLOT(updateAutoAdjustRanges()), Qt::UniqueConnection);
        updateAutoAdjustRanges();
    }
    else
        disconnect(indicator()->priceChart, SIGNAL(dataSet()), this, SLOT(updateAutoAdjustRanges()));
}

int AutoOptimizerOption::autoAdjustPeriods() const
{
    return privateData->autoAdjustPeriods;
}

void AutoOptimizerOption::setAutoAdjustPeriods(int periods)
{
    privateData->autoAdjustPeriods = periods;
    updateAutoAdjustRanges();
}

QVariant AutoOptimizerOption::stepSize(void) const
{
    switch (varType())
    {
        case AutoOptimizerOption::Boolean:
            return QVariant(1.0f);

        case AutoOptimizerOption::Integer:
            return QVariant(static_cast<float>(privateData->form.integerStepSizeSpinBox->value()));

        case AutoOptimizerOption::Double:
            return QVariant(privateData->percentualNotation ? static_cast<float>(privateData->form.doubleStepSizeDoubleSpinBox->value()) / 100.0f : static_cast<float>(privateData->form.doubleStepSizeDoubleSpinBox->value()));

        default:
            return QVariant(static_cast<float>(privateData->form.integerStepSizeSpinBox->value()));
    }

    return QVariant(static_cast<float>(privateData->form.integerStepSizeSpinBox->value()));
}

void AutoOptimizerOption::setStepSize(const QVariant &stepSize)
{
    switch (varType())
    {
        case AutoOptimizerOption::Boolean:
            break;

        case AutoOptimizerOption::Integer:
            privateData->form.integerStepSizeSpinBox->setValue(stepSize.toInt());
            break;

        case AutoOptimizerOption::Double:
            privateData->form.doubleStepSizeDoubleSpinBox->setValue(privateData->percentualNotation ? stepSize.toFloat() * 100.0 : stepSize.toFloat());
            break;

        default:
            privateData->form.integerStepSizeSpinBox->setValue(stepSize.toInt());
            break;
    }

    emit guiTouched();
}

bool AutoOptimizerOption::useAcceleration(void) const
{
    switch (varType())
    {
        case AutoOptimizerOption::Boolean:
            return false;

        case AutoOptimizerOption::Integer:
            return privateData->form.integerAccelerationCheckBox->isChecked();

        case AutoOptimizerOption::Double:
            return privateData->form.doubleAccelerationCheckBox->isChecked();

        default:
            return privateData->form.integerAccelerationCheckBox->isChecked();
    }

    return privateData->form.integerAccelerationCheckBox->isChecked();
}

void AutoOptimizerOption::setUseAcceleration(bool enabled)
{
    switch (varType())
    {
        case AutoOptimizerOption::Boolean:
            break;

        case AutoOptimizerOption::Integer:
            privateData->form.integerAccelerationCheckBox->setChecked(enabled);
            break;

        case AutoOptimizerOption::Double:
            privateData->form.doubleAccelerationCheckBox->setChecked(enabled);
            break;

        default:
            privateData->form.integerAccelerationCheckBox->setChecked(enabled);
            break;
    }

    emit guiTouched();
}

bool AutoOptimizerOption::useSimd(void) const
{
    return privateData->simdOption;
}

bool AutoOptimizerOption::valueCheckBoxEnabled(const QVariant &value) const
{
    if (privateData->varType == AutoOptimizerOption::Boolean && privateData->valueTitles.isEmpty())
    {
        bool enabled = value.toBool();
        if (enabled && privateData->form.booleanOnCheckBox->isChecked())
            return true;
        else if (!enabled && privateData->form.booleanOffCheckBox->isChecked())
            return true;
        else
            return false;
    }
    else
    {
        if (privateData->valueTitles.isEmpty())
            return true;

        QGridLayout *layout = static_cast<QGridLayout *>(privateData->form.integerTitleWidget->layout());
        if (!layout)
            return false;

        QLayoutItem *layoutItem;
        if (privateData->varType == AutoOptimizerOption::Boolean)
        {
            if (value.toBool() > layout->count())
                return false;

            layoutItem = layout->itemAt(value.toBool());
            if (!layoutItem)
                return false;
        }
        else
        {
            if (value.toInt() > layout->count())
                return false;

            layoutItem = layout->itemAt(value.toInt());
            if (!layoutItem)
                return false;
        }

        QCheckBox *checkBox = static_cast<QCheckBox *>(layoutItem->widget());
        if (!checkBox)
            return false;

        return checkBox->isChecked();
    }

    return true;
}

void AutoOptimizerOption::setValueCheckBoxEnabled(const QVariant &value, bool enabled)
{
    if (privateData->valueTitles.isEmpty())
        return;

    QGridLayout *layout = static_cast<QGridLayout *>(privateData->form.integerTitleWidget->layout());
    if (!layout)
        return;

    QLayoutItem *layoutItem;
    if (varType() == AutoOptimizerOption::Boolean)
    {
        if (value.toBool() > layout->count())
            return;

        layoutItem = layout->itemAt(value.toBool());
        if (!layoutItem)
            return;
    }
    else
    {
        if (value.toInt() > layout->count())
            return;

        layoutItem = layout->itemAt(value.toInt());
        if (!layoutItem)
            return;
    }

    QCheckBox *checkBox = static_cast<QCheckBox *>(layoutItem->widget());
    if (!checkBox)
        return;

    checkBox->setChecked(enabled);

    emit guiTouched();
}

QString AutoOptimizerOption::suffix(void) const
{
    switch (varType())
    {
        case AutoOptimizerOption::Boolean:
            return QString();

        case AutoOptimizerOption::Integer:
            return privateData->form.integerMinSpinBox->suffix();

        case AutoOptimizerOption::Double:
            return privateData->form.doubleMinDoubleSpinBox->suffix();

        default:
            return QString();
    }

    return QString();
}

void AutoOptimizerOption::setSuffix(const QString &suffix)
{
    switch (varType())
    {
        case AutoOptimizerOption::Boolean:
            break;

        case AutoOptimizerOption::Integer:
            privateData->form.integerMinSpinBox->setSuffix(suffix);
            privateData->form.integerMaxSpinBox->setSuffix(suffix);
            privateData->form.integerStepSizeSpinBox->setSuffix(suffix);
            break;

        case AutoOptimizerOption::Double:
            privateData->form.doubleMinDoubleSpinBox->setSuffix(suffix);
            privateData->form.doubleMaxDoubleSpinBox->setSuffix(suffix);
            privateData->form.doubleStepSizeDoubleSpinBox->setSuffix(suffix);
            break;

        default:
            break;
    }

}

QString AutoOptimizerOption::valueTitle(const QVariant &value) const
{
    if (!privateData->valueTitles.isEmpty())
    {
        if (varType() == AutoOptimizerOption::Boolean)
            return privateData->valueTitles.at(value.toBool());
        else
            return privateData->valueTitles.at(value.toInt());
    }

    switch (varType())
    {
        case AutoOptimizerOption::Boolean:
            if (value.toBool() == true)
                return tr("Aan");
            \
            return tr("Uit");

        case AutoOptimizerOption::Integer:
            return QString::number(value.toInt());

        case AutoOptimizerOption::Double:
            return QString::number(value.toFloat(), 'f', 2);

        default:
            return QString::number(value.toInt());
    }

    return QString::number(value.toInt());
}

Indicator *AutoOptimizerOption::indicator(void) const
{
    return privateData->indicator;
}

bool AutoOptimizerOption::enabled(void) const
{
    if (!(privateData->dependenciesChanged & 0x00000010))
        return privateData->enabled;

    privateData->dependenciesChanged &= ~0x00000010;
    if (!hasGuiItemsEnabled())
    {
        privateData->enabled = false;
        return false;
    }

    if (privateData->dependencyList.isEmpty())
    {
        privateData->enabled = true;
        return true;
    }

    for (int i = 0; i < privateData->dependencyList.size(); ++i)
    {
        const Dependency *dependency = privateData->dependencyList.at(i);
        if (dependency->option)
            continue;

        if (!dependencyForfilled(*dependency))
        {
            privateData->enabled = false;
            return false;
        }
    }

    privateData->enabled = true;

    return true;
}

bool AutoOptimizerOption::hasGuiItemsEnabled(void) const
{
    if (privateData->varType == AutoOptimizerOption::Boolean)
    {
        if (privateData->valueTitles.isEmpty())
        {
            if (!privateData->form.booleanOffCheckBox->isChecked() && !privateData->form.booleanOnCheckBox->isChecked())
                return false;
        }
        else
        {
            if (!valueCheckBoxEnabled(false) && !valueCheckBoxEnabled(true))
                return false;
        }
    }
    else if (privateData->varType == AutoOptimizerOption::Integer && !privateData->valueTitles.isEmpty())
    {
        bool somethingIsChecked = false;
        for (int i = 0; i < privateData->valueTitles.size(); ++i)
        {
            if (valueCheckBoxEnabled(i))
            {
                somethingIsChecked = true;
                break;
            }
        }

        if (!somethingIsChecked)
            return false;
    }

    return true;
}

inline void AutoOptimizerOption::setDependenciesChanged(void) const
{
    privateData->dependenciesChanged = 0b11111111;
}

void AutoOptimizerOption::storeOriginalSetting(void)
{
    if (privateData->simdOption)
    {
        switch (varType())
        {
            case AutoOptimizerOption::Boolean:
                privateData->originalSettingIntVec = *((ui4vector *) privateData->optionAddress);
                break;

            case AutoOptimizerOption::Integer:
                privateData->originalSettingIntVec = * ((ui4vector *) privateData->optionAddress);
                break;

            case AutoOptimizerOption::Double:
                privateData->originalSettingFltVec = * ((f4vector *) privateData->optionAddress);
                break;
        }
    }
    else
    {
        switch (varType())
        {
            case AutoOptimizerOption::Boolean:
                privateData->originalSetting = * ((bool *) privateData->optionAddress);
                break;

            case AutoOptimizerOption::Integer:
                privateData->originalSetting = * ((int *) privateData->optionAddress);
                break;

            case AutoOptimizerOption::Double:
                privateData->originalSetting = * ((double *) privateData->optionAddress);
                break;
        }
    }
}

void AutoOptimizerOption::restoreOriginalSetting(void)
{
    if (privateData->simdOption)
    {
        switch (varType())
        {
            case AutoOptimizerOption::Boolean:
                * ((ui4vector *) privateData->optionAddress) = privateData->originalSettingIntVec;
                break;

            case AutoOptimizerOption::Integer:
                * ((ui4vector *) privateData->optionAddress) = privateData->originalSettingIntVec;
                break;

            case AutoOptimizerOption::Double:
                * ((f4vector *) privateData->optionAddress) = privateData->originalSettingFltVec;
                break;
        }
    }
    else
    {
        switch (varType())
        {
            case AutoOptimizerOption::Boolean:
                * ((bool *) privateData->optionAddress) = privateData->originalSetting.toBool();
                break;

            case AutoOptimizerOption::Integer:
                * ((int *) privateData->optionAddress) = privateData->originalSetting.toInt();
                break;

            case AutoOptimizerOption::Double:
                * ((float *) privateData->optionAddress) = privateData->originalSetting.toFloat();
                break;
        }
    }
}

int AutoOptimizerOption::possibleOptionsCount(void) const
{
    int possibleOptions = 0;
    float accelleration;
    if (useAcceleration())
        accelleration = 1.1;
    else
        accelleration = 1;

    switch (varType())
    {
        case AutoOptimizerOption::Boolean:
            for (int i = min().toInt(); i <= max().toInt(); ++i)
            {
                if (valueCheckBoxEnabled(i))
                {
                    ++possibleOptions;
                }
            }
            return possibleOptions;

        case AutoOptimizerOption::Integer:
            for (float i = min().toFloat(); i <= max().toFloat(); i = (int)((i * accelleration) + stepSize().toFloat()))
            {
                if (valueCheckBoxEnabled(i))
                    ++possibleOptions;
            }
            return possibleOptions;

        case AutoOptimizerOption::Double:
            for (float i = min().toFloat(); i <= max().toFloat(); i = (i * accelleration) + stepSize().toFloat())
                ++possibleOptions;
            return possibleOptions;

        default:
            break;
    }

    return possibleOptions;
}

void AutoOptimizerOption::addDependency(const AutoOptimizerOption *option, Dependency::compareType cmpType)
{
    if (!option)
        return;

    AutoOptimizerOption *thisObject = this;
    bool retval;
    if (cmpType == Dependency::IsLower)
        retval = option->containsDependency(*thisObject, Dependency::IsHigher);
    else if (cmpType == Dependency::IsHigher)
        retval = option->containsDependency(*thisObject, Dependency::IsLower);
    else
        retval = option->containsDependency(*thisObject, Dependency::IsEqual);

    if (retval)
        return;

    retval = thisObject->containsDependency(*option, cmpType);
    if (retval)
        return;

    if (option->dependsOn(*thisObject))
        return;

    Dependency *dependency = new Dependency;
    dependency->dependencyOption = (AutoOptimizerOption *) option;
    dependency->option = this;
    dependency->cmpType = cmpType;
    dependency->toggleOptionDisabled = false;

    privateData->dependencyList << dependency;
    option->addDependent(this);
}

void AutoOptimizerOption::addDependency(const AutoOptimizerOption *option, QVariant value, Dependency::compareType cmpType, bool toggleEnabled)
{
    if (!option)
        return;

    AutoOptimizerOption *thisObject = this;
    if (option->dependsOn(*thisObject))
        return;

    Dependency *dependency = new Dependency;
    dependency->dependencyOption = (AutoOptimizerOption *) option;
    dependency->option = NULL;
    if (option->varType() == AutoOptimizerOption::Boolean)
        if (useSimd())
            dependency->value = value.toBool() ? 0xffffffff : 0x00000000;
        else
            dependency->value = value;
    else
        dependency->value = value;
    dependency->cmpType = cmpType;
    dependency->toggleOptionDisabled = toggleEnabled;

    privateData->dependencyList << dependency;
    option->addDependent(this);
}

void AutoOptimizerOption::addDependent(const AutoOptimizerOption *option) const
{
    privateData->dependentList << option;
}

bool AutoOptimizerOption::dependenciesForfilled(void) const
{
    if (!(privateData->dependenciesChanged & 0x00000001))
        return privateData->dependenciesForfilled;

    privateData->dependenciesChanged &= ~0x00000001;
    if (privateData->dependencyList.isEmpty())
    {
        privateData->dependenciesForfilled = true;
        return true;
    }

    QVector<Dependency *>::const_iterator dependencyIt = privateData->dependencyList.constBegin();
    const QVector<Dependency *>::const_iterator dependencyEnd = privateData->dependencyList.constEnd();
    for (; dependencyIt != dependencyEnd; dependencyIt++)
    {
        const Dependency *dependency = *dependencyIt;
        const QVariant &value = !dependency->option ? dependency->value : dependency->option->value();

        switch (dependency->cmpType)
        {
            case Dependency::IsEqual:
                if (value.toFloat() != dependency->dependencyOption->value().toFloat())
                {
                    privateData->dependenciesForfilled = false;
                    return false;
                }
                break;

            case Dependency::IsLower:
                if (value.toFloat() <= dependency->dependencyOption->value().toFloat())
                {
                    privateData->dependenciesForfilled = false;
                    return false;
                }
                break;

            case Dependency::IsHigher:
                if (value.toFloat() >= dependency->dependencyOption->value().toFloat())
                {
                    privateData->dependenciesForfilled = false;
                    return false;
                }
                break;

            default:
                if (dependency->dependencyOption->value().toFloat() != value.toFloat())
                {
                    privateData->dependenciesForfilled = false;
                    return false;
                }
                break;
        }
    }

    privateData->dependenciesForfilled = true;

    return true;
}

bool AutoOptimizerOption::dependencyForfilled(const Dependency &dependency) const
{
    QVariant value;
    if (!dependency.option)
        value = dependency.value;
    else
        value = dependency.option->value();

    switch (dependency.cmpType)
    {
        case Dependency::IsEqual:
            if (dependency.dependencyOption->value() != value)
                return false;
            break;

        case Dependency::IsLower:
            if (dependency.dependencyOption->value().toFloat() <= value.toFloat())
                return false;
            break;

        case Dependency::IsHigher:
            if (dependency.dependencyOption->value().toFloat() >= value.toFloat())
                return false;
            break;

        default:
            if (dependency.dependencyOption->value() != value)
                return false;
            break;
    }

    return true;
}

bool AutoOptimizerOption::hasDependencies(void) const
{
    if (privateData->dependencyList.isEmpty())
        return false;

    return true;
}

bool AutoOptimizerOption::dependsOn(const AutoOptimizerOption &option) const
{

    Dependency *dependency;
    for (int i = 0; i < privateData->dependencyList.size(); ++i)
    {
        dependency = privateData->dependencyList.at(i);
        if (dependency->dependencyOption == &option)
            return true;

        bool result = dependency->dependencyOption->dependsOn(option);
        if (result)
            return true;
    }

    return false;
}

bool AutoOptimizerOption::containsDependency(const AutoOptimizerOption &option, Dependency::compareType cmpType) const
{
    Dependency *dependency;
    for (int i = 0; i < privateData->dependencyList.size(); ++i)
    {
        dependency = privateData->dependencyList.at(i);
        if (dependency->dependencyOption == &option && dependency->cmpType == cmpType)
            return true;
    }

    return false;
}

QVector<Dependency *> AutoOptimizerOption::dependencies(void) const
{
    return privateData->dependencyList;
}

bool AutoOptimizerOption::operator== (const AutoOptimizerOption &other) const
{
    if (other.title() == title() && other.type() == type())
        return true;

    return false;
}

void AutoOptimizerOption::slotRelayUpdateGuiSignal(void) const
{
    emit guiTouched();
}

