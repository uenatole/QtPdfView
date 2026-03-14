add_subdirectory(api)
add_subdirectory(std)
add_subdirectory(backends)

add_library(Document INTERFACE)
add_library(Document::Document ALIAS Document)

target_link_libraries(Document
    INTERFACE
        DocumentAPI
        DocumentSTD
        DocumentBackends
)
