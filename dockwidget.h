#ifndef DOCKWIDGET_H
#define DOCKWIDGET_H

#include <QDockWidget>
#include <QAnimationGroup>

#undef DOCK_USE_PIXMAPS
//#define DOCK_USE_PIXMAPS

class QGLWidget;

class TransparentDock : public QWidget
{
        Q_OBJECT
        Q_PROPERTY(qreal widgetOpacity READ widgetOpacity WRITE setWidgetOpacity)

    public:
        explicit TransparentDock(QDockWidget *dock = 0);
        ~TransparentDock(void);

        void setTitle(const QString &title);
        QWidget *widget(void) const;
        void setWidget(QWidget *widget);
        Qt::DockWidgetArea dockLocation(void) const;
        void setDockLocation(Qt::DockWidgetArea area);
        QWidget *alignWidget(void) const;
        void setAlignWidget(QWidget *widget);
        bool autoHide(void) const;
        void setWindowToDockSpacing(int top, int right, int bottom, int left);
        bool isSlidIn(void) const;
        qreal widgetOpacity(void) const;
        void setWidgetOpacity(qreal opacity);

        const QTime &backgroundRenderTimeStamp(void) const;
#ifdef DOCK_USE_PIXMAPS
        const QPixmap &backgroundPixmap(void) const;
#else
        const QImage &backgroundImage(void) const;
#endif

    public slots:
        void setAutoHide(bool autoHide);
        void toggleVisibility(void);
        void alignToParent(void);
        virtual void setVisible(bool visible);
        void slideInOut(bool visible);
        void updateOutliningGeometry(void);
        void setFloating(bool floating);
        void prepareToRaise(void);
        void doRaise(void);

    signals:
        void slideIn(void);
        void slideOut(void);
        void fadeIn(void);
        void fadeOut(void);
        void visibilityChanged(bool visible);

    protected:
        virtual void paintEvent(QPaintEvent *paintEvent);
        virtual void enterEvent(QEvent *enterEvent);
        virtual void leaveEvent(QEvent *leaveEvent);
        virtual void resizeEvent(QResizeEvent *resizeEvent);
        virtual void moveEvent(QMoveEvent *moveEvent);
        virtual bool eventFilter(QObject *object, QEvent *event);

    private slots:
        void animationFinished(void);
        void testSlot(void);
        void toggleBackgroundRefreshingEnabled(bool on);
        void updatePalette(void);
        void renderBackGround(bool forceRepaint = true);

    private:
        void adjustComboColors(QObject *parent);

        friend class DockWidget;
        class PrivateData;
        PrivateData *privateData;
};

class DockWidget : public QDockWidget
{
        Q_OBJECT

    public:
        explicit DockWidget(QWidget *parent = 0);
        ~DockWidget();

        Qt::DockWidgetArea dockLocation(void) const;
        bool isVisible(void) const;
        TransparentDock *floatWidget(void) const;
        void setFloatAlignWidget(QWidget *widget);
        QWidget *widget(void) const;
        void setWidget(QWidget *widget);
        bool autoHide(void) const;
        void setAutoHide(bool autoHide);

    public slots:
        void setDockLocation(Qt::DockWidgetArea area);
        void setVisible(bool visible);
        void slideInOut(bool visible);

    signals:
        void dockVisibilityChanged(bool visible);

    private slots:
        void handleTopLevelChangedSignal(bool topLevel);
        void updatePalette();

    private:
        class PrivateData;
        PrivateData *privateData;
};

#endif // DOCKWIDGET_H
