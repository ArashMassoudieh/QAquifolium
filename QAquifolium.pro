#-------------------------------------------------
#
# Project created by QtCreator 2020-03-22T17:57:35
#
#-------------------------------------------------

QT       += core gui opengl printsupport
INCLUDEPATH += ../Aquifolium/include
INCLUDEPATH += ../jsoncpp/include/
INCLUDEPATH += include/

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = QAquifolium
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS Q_version Aquifolium DEBUG

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++14

SOURCES += \
        main.cpp \
        mainwindow.cpp \
    ../Aquifolium/src/Block.cpp \
    ../Aquifolium/src/BTC.cpp \
    ../Aquifolium/src/BTCSet.cpp \
    ../Aquifolium/src/Expression.cpp \
    ../Aquifolium/src/Matrix.cpp \
    ../Aquifolium/src/Matrix_arma.cpp \
    ../Aquifolium/src/MetaModel.cpp \
    ../Aquifolium/src/NormalDist.cpp \
    ../Aquifolium/src/Object.cpp \
    ../Aquifolium/src/Quan.cpp \
    ../Aquifolium/src/QuanSet.cpp \
    ../Aquifolium/src/QuickSort.cpp \
    ../Aquifolium/src/System.cpp \
    ../Aquifolium/src/Vector.cpp \
    ../Aquifolium/src/Vector_arma.cpp \
    ../jsoncpp/src/lib_json/json_reader.cpp \
    ../jsoncpp/src/lib_json/json_value.cpp \
    ../jsoncpp/src/lib_json/json_valueiterator.inl \
    ../jsoncpp/src/lib_json/json_writer.cpp \
    ../Aquifolium/src/Source.cpp \
    ../Aquifolium/src/Rule.cpp \
    ../Aquifolium/src/Objective_Function_Set.cpp \
    ../Aquifolium/src/Objective_Function.cpp \
    ../Aquifolium/src/Precipitation.cpp \
    ../Aquifolium/src/Condition.cpp \
    ../Aquifolium/src/Parameter_Set.cpp \
    ../Aquifolium/src/Parameter.cpp \
    ../Aquifolium/src/ErrorHandler.cpp \
    ../Aquifolium/src/Link.cpp \
    ../Aquifolium/src/GA/DistributionNUnif.cpp \
    propmodel.cpp \
    delegate.cpp \
    utilityfuncs.cpp

HEADERS += \
        mainwindow.h \
    ../Aquifolium/include/Block.h \
    ../Aquifolium/include/BTC.h \
    ../Aquifolium/include/BTCSet.h \
    ../Aquifolium/include/Expression.h \
    ../Aquifolium/include/Link.h \
    ../Aquifolium/include/Matrix.h \
    ../Aquifolium/include/Matrix_arma.h \
    ../Aquifolium/include/MetaModel.h \
    ../Aquifolium/include/NormalDist.h \
    ../Aquifolium/include/Object.h \
    ../Aquifolium/include/Quan.h \
    ../Aquifolium/include/QuanSet.h \
    ../Aquifolium/include/QuickSort.h \
    ../Aquifolium/include/StringOP.h \
    ../Aquifolium/include/System.h \
    ../Aquifolium/include/Vector.h \
    ../Aquifolium/include/Vector_arma.h \
    ../jsoncpp/include/json/allocator.h \
    ../jsoncpp/include/json/assertions.h \
    ../jsoncpp/include/json/autolink.h \
    ../jsoncpp/include/json/config.h \
    ../jsoncpp/include/json/features.h \
    ../jsoncpp/include/json/forwards.h \
    ../jsoncpp/include/json/json.h \
    ../jsoncpp/include/json/reader.h \
    ../jsoncpp/include/json/value.h \
    ../jsoncpp/include/json/version.h \
    ../jsoncpp/include/json/writer.h \
    ../jsoncpp/src/lib_json/json_tool.h \
    ../jsoncpp/src/lib_json/version.h.in \
    enums.h \
    propmodel.h \
    delegate.h \
    utilityfuncs.h \
    ../Aquifolium/include/Parameter.h \
    ../Aquifolium/include/Parameter_Set.h

FORMS += \
        mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# LAPACK — Linear Algebra PACKage lib and include locations

win32 {

    LAPACK_INCLUDE = $$PWD/include
    #64 bits build
    contains(QMAKE_TARGET.arch, x86_64) {
        #debug
        CONFIG(debug, debug|release) {
            LAPACK_LIB_DIR = $$PWD/libs/lapack-blas_lib_win64/debug
            LIBS +=  -L$${LAPACK_LIB_DIR} -llapack_win64_MTd \
                    -lblas_win64_MTd
        }
        #release
        CONFIG(release, debug|release) {
            LAPACK_LIB_DIR = $$PWD/libs/lapack-blas_lib_win64/release
            LIBS +=  -L$${LAPACK_LIB_DIR} -llapack_win64_MT \
                    -lblas_win64_MT
        }
    }

    INCLUDEPATH += $${LAPACK_INCLUDE}

    DEFINES += ARMA_USE_LAPACK ARMA_USE_BLAS

}

linux {
    #sudo apt-get install libblas-dev liblapack-dev
     DEFINES += ARMA_USE_LAPACK ARMA_USE_BLAS
     LIBS += -larmadillo -llapack -lblas
}
