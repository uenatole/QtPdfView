option(BUILD_PDF_BACKEND "Build WebEngine's (Qt::Pdf) based PDF backend" ON)

add_library(DocumentBackends INTERFACE)
add_library(Document::Backends ALIAS DocumentBackends)

if(BUILD_PDF_BACKEND)
    add_subdirectory(pdf)
    target_link_libraries(DocumentBackends INTERFACE DocumentBackendPdf)
endif()
