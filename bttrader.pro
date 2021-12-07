include( ../examples.pri )

TARGET       = bttrader

INCLUDEPATH += ./

QT += sql opengl

unix {
     LIBS += -lX11
}

RESOURCES = \
    bttrader.qrc

QMAKE_CFLAGS   += -Ofast -g -fopenmp -march=native
QMAKE_CXXFLAGS += -Ofast -g -fopenmp -march=native
QMAKE_LDFLAGS  += -Ofast -g -fopenmp -march=native
QMAKE_LFLAGS   += -Ofast -g -fopenmp -march=native

HEADERS =  \
    plot.h \
    rsi.h \
    movingaverage.h \
    2movingaverage.h \
    3movingaverage.h \
    line.h \
    strategie.h \
    indicator.h \
    scrollbar.h \
    scrollzoomer.h \
    canvaspicker.h \
    autooptimizer.h \
    volatility.h \
    er.h \
    bollingerbands.h \
    pricechannel.h \
    adx.h \
    trend.h \
    dominantcycle.h \
    trendvigor.h \
    snr.h \
    instantaneoustrend.h \
    stochastic.h \
    settings.h \
    datetime.h \
    calculateCache.h \
    bttrader.h \
    dockwidget.h \
    autooptimizeroption.h \
    periodsconfigui.h \
    maconfigui.h \
    slidingstackedwidget.h \
    takeprofitoptions.h \
    rule_evaluation.h \
    intrinsics.h \
    dataconnection.h \
    databrowser.h \
    strategieoverview.h \
    blur.h \
    tabwidget.h \
    verticalscrollarea.h \
    mainwindow.h \
    menucontextmenu.h \
    orientationbutton.h \
    plotimagebackground.h \
    rulelist.h \
    rule.h \
    stoplossoptions.h \
    autooptimizerworker.h \
    transparentblurredwidget.h \
    transparentblurredpopup.h \
    animatedslidewidget.h \
    simulationproperties.h \
    settingsdialog.h \
    main.h

SOURCES = \
    plot.cpp \
    indicator.cpp \
    rsi.cpp \
    movingaverage.cpp \
    2movingaverage.cpp \
    3movingaverage.cpp \
    line.cpp \
    strategie.cpp \
    scrollbar.cpp \
    scrollzoomer.cpp \
    canvaspicker.cpp \
    autooptimizer.cpp \
    volatility.cpp \
    er.cpp \
    bollingerbands.cpp \
    pricechannel.cpp \
    adx.cpp \
    trend.cpp \
    dominantcycle.cpp \
    trendvigor.cpp \
    snr.cpp \
    instantaneoustrend.cpp \
    stochastic.cpp \
    settings.cpp \
    datetime.cpp \
    calculateCache.cpp \
    dockwidget.cpp \
    autooptimizeroption.cpp \
    periodsconfigui.cpp \
    maconfigui.cpp \
    slidingstackedwidget.cpp \
    takeprofitoptions.cpp \
    rule_evaluation.cpp \
    dataconnection.cpp \
    databrowser.cpp \
    strategieoverview.cpp \
    blur.cpp \
    tabwidget.cpp \
    verticalscrollarea.cpp \
    mainwindow.cpp \
    menucontextmenu.cpp \
    orientationbutton.cpp \
    plotimagebackground.cpp \
    rulelist.cpp \
    rule.cpp \
    stoplossoptions.cpp \
    autooptimizerworker.cpp \
    transparentblurredwidget.cpp \
    transparentblurredpopup.cpp \
    animatedslidewidget.cpp \
    simulationproperties.cpp \
    settingsdialog.cpp \
    main.cpp

win32 {
        SOURCES +=
}

unix {
     SOURCES +=
}

FORMS = \
    MovingAverage.ui \
    2movingaverage.ui \
    3movingaverage.ui \
    ma.ui \
    periods.ui \
    line.ui \
    mainwindow.ui \
    transactionstable.ui \
    strategie.ui \
    strategiesetup.ui \
    volatility.ui \
    trend.ui \
    indicator.ui \
    quote.ui \
    sourcepicker.ui \
    rulepopup.ui \
    autooptimizer.ui \
    autooptimizeroption.ui \
    databrowser.ui \
    strategieoverview.ui \
    curveproperties.ui \
    pricecurveproperties.ui \
    simulationproperties.ui \
    settingsdialog.ui
















