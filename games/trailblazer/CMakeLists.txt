include_directories(
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/generated)

ADD_EXECUTABLE(
  trailblazer
  trailblazer.cpp
  )

include_directories(${PROJECT_SOURCE_DIR}/solr)

TARGET_LINK_LIBRARIES(
    trailblazer
    solr
)

# ------------------------------------------------------------
INSTALL(TARGETS trailblazer DESTINATION bin)
# ------------------------------------------------------------
