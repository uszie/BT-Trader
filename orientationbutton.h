#ifndef ORIENTATIONBUTTON_H
#define ORIENTATIONBUTTON_H

#include <QPushButton>

class OrientationButton : public QPushButton
{
    public:
        explicit OrientationButton(QWidget *parent = 0);
        explicit OrientationButton(const QString &text, QWidget *parent = 0);
        explicit OrientationButton(const QIcon &icon, const QString &text, QWidget *parent = 0);
        ~OrientationButton();

        Qt::Orientation orientation() const;
        void setOrientation(Qt::Orientation orientation);

        bool mirrored() const;
        void setMirrored(bool mirrored);

        QSize sizeHint() const;
        QSize minimumSizeHint() const;

    protected:
        void paintEvent(QPaintEvent *event);

    private:
        QStyleOptionButton getStyleOption() const;
        void init();

        Qt::Orientation orientation_;
        bool mirrored_;

};

#endif // ORIENTATIONBUTTON_H
