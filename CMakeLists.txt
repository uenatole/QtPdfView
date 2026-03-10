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

        src/core/Document.h
        src/core/DocumentParser.h
        src/core/DocumentRenderer.h
        src/core/DocumentFacade.cpp
        src/core/DocumentFacade.h
        src/core/DocumentLink.cpp
        src/core/DocumentLink.h

        src/ui/DocumentView.cpp
        src/ui/DocumentView.h
        src/ui/DocumentPageItem.cpp
        src/ui/DocumentPageItem.h

        src/ui/addons/DocumentSelector.cpp
        src/ui/addons/DocumentSelector.h
        src/ui/addons/DocumentZoomer.cpp
        src/ui/addons/DocumentZoomer.h

        src/core/StandardDocumentParser.cpp
        src/core/StandardDocumentParser.h
        src/core/StandardDocumentRenderer.cpp
        src/core/StandardDocumentRenderer.h

        src/backend/pdf/PdfDocument.cpp
        src/backend/pdf/PdfDocument.h
)

target_link_libraries(QtPdfView PUBLIC Qt::Pdf Qt::Widgets)
target_include_directories(QtPdfView PUBLIC src)

add_executable(example examples/main.cpp)
target_link_libraries(example PRIVATE QtPdfView)
