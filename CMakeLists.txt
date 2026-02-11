cmake_minimum_required(VERSION 4.1)
project(MyPdfView)

set(CMAKE_CXX_STANDARD 20)

set(FEATURE_qtwebengine_build OFF CACHE BOOL "" FORCE)
set(QT_GENERATE_SBOM OFF CACHE BOOL "" FORCE)
set(QT_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(QT_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(3rdparty/qtwebengine)

find_package(Qt6 COMPONENTS Widgets)

add_library(QtPdfView
        src/custom/QCacheExt.h
        src/PdfView.cpp
        src/PdfView.h
        src/PdfPageItem.cpp
        src/PdfPageItem.h
        src/PdfPageProvider.cpp
        src/PdfPageProvider.h)

target_link_libraries(QtPdfView PUBLIC Qt::Pdf Qt::Widgets)
target_include_directories(QtPdfView PUBLIC src)

add_executable(example examples/main.cpp)
target_link_libraries(example PRIVATE QtPdfView)
