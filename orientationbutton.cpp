#include "orientationbutton.h"
#include <QPushButton>
#include <QMenu>
#include <QStylePainter>
#include <QStyleOptionButton>

OrientationButton::OrientationButton(QWidget *parent)
    : QPushButton(parent)
{
    init();
}

OrientationButton::OrientationButton(const QString &text, QWidget *parent)
    : QPushButton(text, parent)
{
    init();
}

OrientationButton::OrientationButton(const QIcon &icon, const QString &text, QWidget *parent)
    : QPushButton(icon, text, parent)
{
    init();
}

OrientationButton::~OrientationButton()
{
}

void OrientationButton::init()
{
    orientation_ = Qt::Horizontal;
    mirrored_ = false;
}

Qt::Orientation OrientationButton::orientation() const
{
    return orientation_;
}

void OrientationButton::setOrientation(Qt::Orientation orientation)
{
    orientation_ = orientation;
    switch (orientation)
    {
        case Qt::Horizontal:
            setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
            break;

        case Qt::Vertical:
            setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
            break;
    }
}

bool OrientationButton::mirrored() const
{
    return mirrored_;
}

void OrientationButton::setMirrored(bool mirrored)
{
    mirrored_ = mirrored;
}

QSize OrientationButton::sizeHint() const
{
    QSize size = QPushButton::sizeHint();
    if (orientation_ == Qt::Vertical)
        size.transpose();

    return size;
}

QSize OrientationButton::minimumSizeHint() const
{
    QSize size = QPushButton::minimumSizeHint();
    if (orientation_ == Qt::Vertical)
        size.transpose();

    return size;
}

void OrientationButton::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QStylePainter painter(this);

    switch (orientation_)
    {
        case Qt::Horizontal:
            if (mirrored_)
            {
                painter.rotate(180);
                painter.translate(-width(), -height());
            }
            break;

        case Qt::Vertical:
            if (mirrored_)
            {
                painter.rotate(-90);
                painter.translate(-height(), 0);
            }
            else
            {
                painter.rotate(90);
                painter.translate(0, -width());
            }
            break;
    }

    painter.drawControl(QStyle::CE_PushButton, getStyleOption());
}

QStyleOptionButton OrientationButton::getStyleOption() const
{
    QStyleOptionButton styleOptionButto;
    styleOptionButto.initFrom(this);
    if (orientation_ == Qt::Vertical)
    {
        QSize size = styleOptionButto.rect.size();
        size.transpose();
        styleOptionButto.rect.setSize(size);
    }
    styleOptionButto.features = QStyleOptionButton::None;
    if (isFlat())
        styleOptionButto.features |= QStyleOptionButton::Flat;
    if (menu())
        styleOptionButto.features |= QStyleOptionButton::HasMenu;
    if (autoDefault() || isDefault())
        styleOptionButto.features |= QStyleOptionButton::AutoDefaultButton;
    if (isDefault())
        styleOptionButto.features |= QStyleOptionButton::DefaultButton;
    if (isDown() || (menu() && menu()->isVisible()))
        styleOptionButto.state |= QStyle::State_Sunken;
    if (isChecked())
        styleOptionButto.state |= QStyle::State_On;
    if (!isFlat() && !isDown())
        styleOptionButto.state |= QStyle::State_Raised;
    styleOptionButto.text = text();
    styleOptionButto.icon = icon();
    styleOptionButto.iconSize = iconSize();

    return styleOptionButto;
}
