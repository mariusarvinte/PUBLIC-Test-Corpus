# © 2026 Massachusetts Institute of Technology
# MIT License

add_definitions(
	-DSTDC
	-DHAVE_SYS_TYPES_H
	-DHAVE_STDINT_H
	-DHAVE_STDDEF_H
)

set(SRC_ZLIB
	adler32.c
	crc32.c
	crc32.h
	deflate.c
	deflate.h
	inffixed.h
	inflate.c
	inflate.h
	inftrees.c
	inftrees.h
	trees.c
	trees.h
	zconf.h
	zlib.h
	zutil.c
	zutil.h)
include_directories(".")
add_library(zlib OBJECT ${SRC_ZLIB})
set_target_properties(zlib PROPERTIES POSITION_INDEPENDENT_CODE ON)
