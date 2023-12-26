
set(SOURCES
    common.c
    platform_info.c
    hardware/hardware.c
    table/table.c
    table_round/table_round.c
    no_table/no_table.c
    cpython/cpython.c
    numpy/numpy.c
    imath/imath.c
    tursa/tursa.c
    ryg/ryg.c
    maratyszcza/maratyszcza.c
    maratyszcza_nanfix/maratyszcza_nanfix.c
)

if ( (${ARCH} STREQUAL "x86" ) OR (${ARCH} STREQUAL "x86_64") )
    list(APPEND SOURCES
        x86_cpu_info.c
        maratyszcza_sse2/maratyszcza_sse2.c
        ryg_sse2/ryg_sse2.c
    )

    # MSVC does not need a -mf16c compile flag
    if(NOT MSVC)
        set_property(SOURCE hardware/hardware.c APPEND PROPERTY COMPILE_OPTIONS -mf16c)
        # make sure compiler only uses sse2
        set_property(SOURCE maratyszcza_sse2/maratyszcza_sse2.c APPEND PROPERTY COMPILE_OPTIONS
        -mtune=generic -msse2 -mno-sse3 -mno-sse4 -mno-sse4.2 -mno-avx -mno-avx2)
    endif()

elseif (${ARCH} STREQUAL "arm")
    # arm64 includes neon and fp16
    set_property(SOURCE hardware/hardware.c APPEND PROPERTY COMPILE_OPTIONS
        -mfpu=neon-fp-armv8 -mfloat-abi=hard -mfp16-format=ieee
    )
endif()

add_executable(float2half
    ${SOURCES}
    float2half.c
)

add_executable(half2float
    ${SOURCES}
    half2float.c
)

if (MSVC AND (${ARCH} STREQUAL "x86") )
    # enable large address space support for win32
    # so we can allocate enough memory needed for test
    target_link_options(float2half PRIVATE /LARGEADDRESSAWARE)
    target_link_options(half2float PRIVATE /LARGEADDRESSAWARE)
endif()

install(TARGETS float2half DESTINATION ${CMAKE_INSTALL_BINDIR})
install(TARGETS half2float DESTINATION ${CMAKE_INSTALL_BINDIR})