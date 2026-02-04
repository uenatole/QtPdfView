cmake_minimum_required(VERSION 4.1)
project(MyPdfView)

set(CMAKE_CXX_STANDARD 20)

set(FEATURE_qtwebengine_build OFF CACHE BOOL "" FORCE)
set(QT_GENERATE_SBOM OFF CACHE BOOL "" FORCE)
set(QT_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(QT_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(3rdparty/qtwebengine)

find_package(Qt6 COMPONENTS Widgets)

add_executable(MyPdfView
        pdf/PdfView.cpp
        pdf/PdfView.h
        pdf/PdfViewPageItem.cpp
        pdf/PdfViewPageItem.h
        pdf/PdfViewPageProvider.cpp
        pdf/PdfViewPageProvider.h
        main.cpp)

target_link_libraries(MyPdfView PRIVATE Qt::Pdf Qt::Widgets)
