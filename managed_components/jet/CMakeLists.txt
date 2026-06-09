# ESP-IDF component CMakeLists.txt for Jet 3D Renderer

file(GLOB JET_SRCS "src/*.cpp")

idf_component_register(
    SRCS ${JET_SRCS}
    INCLUDE_DIRS "src"
)
