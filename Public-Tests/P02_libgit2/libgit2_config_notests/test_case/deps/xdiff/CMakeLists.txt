# © 2026 Massachusetts Institute of Technology
# MIT License

set(SRC_XDIFF
	git-xdiff.h
	xdiff.h
	xdiffi.c
	xdiffi.h
	xemit.c
	xemit.h
	xhistogram.c
	xinclude.h
	xmacros.h
	xmerge.c
	xpatience.c
	xprepare.c
	xprepare.h
	xtypes.h
	xutils.c
	xutils.h)

add_library(xdiff OBJECT ${SRC_XDIFF})
set_target_properties(xdiff PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(xdiff PRIVATE util)
target_include_directories(xdiff SYSTEM PRIVATE
	"${PROJECT_SOURCE_DIR}/include"
	"${PROJECT_SOURCE_DIR}/src/util"
	"${PROJECT_BINARY_DIR}/src/util"
	${LIBGIT2_SYSTEM_INCLUDES}
	${LIBGIT2_DEPENDENCY_INCLUDES})
