########################################################################
# Sparse Grid librareis and command line tool
########################################################################

macro(Tasmanian_macro_add_libsparsegrid Tasmanian_shared_or_static)

if (${Tasmanian_shared_or_static} STREQUAL "SHARED")
    set(Tasmanian_libtsg_target_name "Tasmanian_libsparsegrid_shared")
else()
    set(Tasmanian_libtsg_target_name "Tasmanian_libsparsegrid_static")
endif()

if (Tasmanian_ENABLE_CUDA)
    cuda_add_library(${Tasmanian_libtsg_target_name} ${Tasmanian_shared_or_static} ${Tasmanian_source_libsparsegrid} ${Tasmanian_source_libsparsegrid_cuda})
    set_property(TARGET ${Tasmanian_libtsg_target_name} PROPERTY CUDA_STANDARD 11)
else()
    add_library(${Tasmanian_libtsg_target_name} ${Tasmanian_shared_or_static} ${Tasmanian_source_libsparsegrid})
endif()

# Tasmanian_EXTRA_LIBRARIES gives the user an option to force extra dependencies,
# for example, on some systems (e.g., OLCF) find_package(BLAS) fails to
# recognize that libacml_mp requires libgomp, so the build fails with either clang or ENABLE_OPENMP=OFF
# -D Tasmanian_EXTRA_LIBRARIES=/path/to/libgomp.so circumvents the issue
# NOTE: adding Tasmanian_EXTRA_LIBRARIES to SparseGrids will propagate to all other targets
# same holds for Tasmanian_EXTRA_INCLUDE_DIRS
target_link_libraries(${Tasmanian_libtsg_target_name} ${Tasmanian_EXTRA_LIBRARIES})
target_include_directories(${Tasmanian_libtsg_target_name} PUBLIC $<INSTALL_INTERFACE:${Tasmanian_EXTRA_INCLUDE_DIRS}>)
target_include_directories(${Tasmanian_libtsg_target_name} PUBLIC $<BUILD_INTERFACE:${Tasmanian_EXTRA_INCLUDE_DIRS}>)

target_include_directories(${Tasmanian_libtsg_target_name} PUBLIC $<INSTALL_INTERFACE:include>)

target_include_directories(${Tasmanian_libtsg_target_name} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>)
target_include_directories(${Tasmanian_libtsg_target_name} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../configured/>)

set_target_properties(${Tasmanian_libtsg_target_name} PROPERTIES OUTPUT_NAME "tasmaniansparsegrid")

set_property(TARGET ${Tasmanian_libtsg_target_name} PROPERTY CXX_STANDARD 11)

# as MAGMA potentially depends on OpenMP, BLAS, and CuBLAS, it should be set first
if (Tasmanian_ENABLE_MAGMA)
    target_include_directories(${Tasmanian_libtsg_target_name} PUBLIC $<BUILD_INTERFACE:${Tasmanian_MAGMA_INCLUDE_DIRS}/>)

    if (${Tasmanian_shared_or_static} STREQUAL "SHARED")
        target_link_libraries(${Tasmanian_libtsg_target_name} ${Tasmanian_MAGMA_SHARED_LIBRARIES})
    else()
        target_link_libraries(${Tasmanian_libtsg_target_name} ${Tasmanian_MAGMA_LIBRARIES})
    endif()
endif()

if (Tasmanian_ENABLE_BLAS)
    target_link_libraries(${Tasmanian_libtsg_target_name} ${BLAS_LIBRARIES})
endif()

if (Tasmanian_ENABLE_CUDA) # this must come after MAGMA, since MAGMA doesn't come with package config
    target_link_libraries(${Tasmanian_libtsg_target_name} ${CUDA_cusparse_LIBRARY} ${CUDA_CUBLAS_LIBRARIES} ${CUDA_LIBRARIES})

    target_include_directories(${Tasmanian_libtsg_target_name} PUBLIC ${CUDA_INCLUDE_DIRS})
endif()

if (Tasmanian_ENABLE_OPENMP)
    target_link_libraries(${Tasmanian_libtsg_target_name} ${OpenMP_CXX_LIBRARIES})
    target_compile_options(${Tasmanian_libtsg_target_name} PRIVATE ${OpenMP_CXX_FLAGS})
endif()

install(TARGETS "${Tasmanian_libtsg_target_name}"
        EXPORT  "${CMAKE_PROJECT_NAME}-exports"
        RUNTIME DESTINATION "bin"
        LIBRARY DESTINATION "lib"
        ARCHIVE DESTINATION "lib")

endmacro()

########################################################################
# Define source files
# in order to avoid GLOB, list all source files so they can be used for
# add_library and cuda_add_library in both shared and static cases
########################################################################
set(Tasmanian_source_libsparsegrid
        TasmanianSparseGrid.hpp
        TasmanianSparseGrid.cpp
        tsgAcceleratedDataStructures.hpp
        tsgAcceleratedDataStructures.cpp
        tsgCacheLagrange.hpp
        tsgCoreOneDimensional.hpp
        tsgCoreOneDimensional.cpp
        tsgDConstructGridGlobal.hpp
        tsgDConstructGridGlobal.cpp
        tsgCudaMacros.hpp
        tsgEnumerates.hpp
        tsgGridCore.hpp
        tsgGridCore.cpp
        tsgGridGlobal.hpp
        tsgGridGlobal.cpp
        tsgGridWavelet.hpp
        tsgGridWavelet.cpp
        tsgHardCodedTabulatedRules.hpp
        tsgHardCodedTabulatedRules.cpp
        tsgHiddenExternals.hpp
        tsgGridLocalPolynomial.hpp
        tsgGridLocalPolynomial.cpp
        tsgGridSequence.hpp
        tsgGridSequence.cpp
        tsgGridFourier.hpp
        tsgGridFourier.cpp
        tsgIndexManipulator.hpp
        tsgIndexManipulator.cpp
        tsgIndexSets.hpp
        tsgIndexSets.cpp
        tsgLinearSolvers.hpp
        tsgLinearSolvers.cpp
        tsgOneDimensionalWrapper.hpp
        tsgOneDimensionalWrapper.cpp
        tsgRuleLocalPolynomial.hpp
        tsgRuleWavelet.hpp
        tsgRuleWavelet.cpp
        tsgSequenceOptimizer.hpp
        tsgSequenceOptimizer.cpp)

# source files specific to cuda, used for both static and shared libs
set(Tasmanian_source_libsparsegrid_cuda
       tsgCudaKernels.cu
       tsgCudaBasisEvaluations.hpp
       tsgCudaLinearAlgebra.hpp)


########################################################################
# add the tasgrid and examples executables
########################################################################
add_executable(Tasmanian_tasgrid tasgrid_main.cpp
                                 TasmanianSparseGrid.hpp
                                 tasgridExternalTests.hpp
                                 tasgridExternalTests.cpp
                                 tasgridTestFunctions.hpp
                                 tasgridTestFunctions.cpp
                                 tasgridWrapper.hpp
                                 tasgridWrapper.cpp)
add_executable(Tasmanian_gridtest gridtest_main.cpp
                                  TasmanianSparseGrid.hpp
                                  tasgridExternalTests.hpp
                                  tasgridExternalTests.cpp
                                  tasgridTestFunctions.hpp
                                  tasgridTestFunctions.cpp
                                  tasgridUnitTests.hpp
                                  tasgridUnitTests.cpp
                                  tasgridTestInterfaceC.cpp)
add_executable(Tasmanian_example_sparse_grids Examples/example_sparse_grids.cpp)

set_property(TARGET Tasmanian_tasgrid              PROPERTY CXX_STANDARD 11)
set_property(TARGET Tasmanian_gridtest             PROPERTY CXX_STANDARD 11)
set_property(TARGET Tasmanian_example_sparse_grids PROPERTY CXX_STANDARD 11)

set_target_properties(Tasmanian_tasgrid PROPERTIES OUTPUT_NAME "tasgrid")
set_target_properties(Tasmanian_gridtest PROPERTIES OUTPUT_NAME "gridtest")
set_target_properties(Tasmanian_example_sparse_grids PROPERTIES OUTPUT_NAME "example_sparse_grids")

if (Tasmanian_ENABLE_OPENMP AND OpenMP_CXX_LIBRARIES)
    # see the remark in the macro above regarding the use of the OpenMP target
    # also the libs are redundant here, as those will be added by libsparsegrid
    target_link_libraries(Tasmanian_tasgrid ${OpenMP_CXX_LIBRARIES})
    target_compile_options(Tasmanian_tasgrid PRIVATE ${OpenMP_CXX_FLAGS})
    target_link_libraries(Tasmanian_gridtest ${OpenMP_CXX_LIBRARIES})
    target_compile_options(Tasmanian_gridtest PRIVATE ${OpenMP_CXX_FLAGS})
endif()


########################################################################
# add the libraries
########################################################################
if (NOT "${Tasmanian_libs_type}" STREQUAL "STATIC_ONLY")
    Tasmanian_macro_add_libsparsegrid(SHARED)
endif()

if (NOT "${Tasmanian_libs_type}" STREQUAL "SHARED_ONLY")
    Tasmanian_macro_add_libsparsegrid(STATIC)

    # if static libraries are available, executables link statically
    target_link_libraries(Tasmanian_tasgrid              Tasmanian_libsparsegrid_static)
    target_link_libraries(Tasmanian_gridtest             Tasmanian_libsparsegrid_static)
    target_link_libraries(Tasmanian_example_sparse_grids Tasmanian_libsparsegrid_static)
else() # if there are not static libs, use shared libs for the executables
    target_link_libraries(Tasmanian_tasgrid              Tasmanian_libsparsegrid_shared)
    target_link_libraries(Tasmanian_gridtest             Tasmanian_libsparsegrid_shared)
    target_link_libraries(Tasmanian_example_sparse_grids Tasmanian_libsparsegrid_shared)
endif()

# hack a dependency problem in parallel make
# without this, if both static and shared libs are enabled, make -j tries to compile shared cuda kernels twice
# which creates a race condition and the build randomly fails
if ("${Tasmanian_libs_type}" STREQUAL "BOTH")
    add_dependencies(Tasmanian_libsparsegrid_shared Tasmanian_libsparsegrid_static)
endif()

# data file, needed for testing and reference about custom rule definitions
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/GaussPattersonRule.table"  "${CMAKE_CURRENT_BINARY_DIR}/GaussPattersonRule.table" COPYONLY)


########################################################################
# Windows specific support (DLL export/import directives and names)
########################################################################
if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    # setup the name, in MacOSX and Linux static libraries use .a extension and shared libs use .dylib and .so
    # in Windows, shared .dll libs need another file for linking which shares .lib extension with the static libs
    # thus on Windows, we specifically add _static to the name of the lib
    # furthermore, additional defines are needed to suppress extraneous warnings and adjust some system calls
    macro(Tasmanian_macro_sparsegrids_windows_defines Tasmanian_sg_target)
        target_compile_definitions(${Tasmanian_sg_target} PRIVATE -D_TASMANIAN_WINDOWS_) # overwrittes gettime()
        target_compile_definitions(${Tasmanian_sg_target} PRIVATE -D_SCL_SECURE_NO_WARNINGS) # suppresses warnings regarding pointers to the middle of an array
        target_compile_definitions(${Tasmanian_sg_target} PUBLIC  -D_USE_MATH_DEFINES) # needed to include M_PI constant
        target_compile_definitions(${Tasmanian_sg_target} PUBLIC  -D_HAS_ITERATOR_DEBUGGING=0) # needed to prevent crash on using STL vector iterators
    endmacro()

    if (TARGET Tasmanian_libsparsegrid_shared)
        set_target_properties(Tasmanian_libsparsegrid_shared PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
        Tasmanian_macro_sparsegrids_windows_defines(Tasmanian_libsparsegrid_shared)
    endif()

    if (TARGET Tasmanian_libsparsegrid_static)
        Tasmanian_macro_sparsegrids_windows_defines(Tasmanian_libsparsegrid_static)
        set_target_properties(${Tasmanian_libtsg_target_name} PROPERTIES OUTPUT_NAME "tasmaniansparsegrid_static")
    endif()

    Tasmanian_macro_sparsegrids_windows_defines(Tasmanian_tasgrid)
    Tasmanian_macro_sparsegrids_windows_defines(Tasmanian_gridtest)
endif()


########################################################################
# Testing
########################################################################
add_test(SparseGridsAcceleration gridtest acceleration -gpuid ${Tasmanian_TESTS_GPU_ID})
add_test(SparseGridsDomain       gridtest domain)
add_test(SparseGridsRefinement   gridtest refinement)
add_test(SparseGridsGlobal       gridtest global)
add_test(SparseGridsLocal        gridtest local)
add_test(SparseGridsWavelet      gridtest wavelet)
add_test(SparseGridsFourier      gridtest fourier)
add_test(SparseGridsExceptions   gridtest errors)
add_test(SparseGridsAPI          gridtest api)
add_test(SparseGridsC            gridtest c)
if (Tasmanian_TESTS_OMP_NUM_THREADS GREATER 0)
    set_tests_properties(SparseGridsAcceleration SparseGridsDomain SparseGridsRefinement SparseGridsGlobal SparseGridsLocal SparseGridsWavelet
        PROPERTIES
        PROCESSORS "${Tasmanian_TESTS_OMP_NUM_THREADS}"
        ENVIRONMENT "OMP_NUM_THREADS=${Tasmanian_TESTS_OMP_NUM_THREADS}")
endif()


########################################################################
# Install headers and config files
########################################################################
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/"
        DESTINATION include
        FILES_MATCHING PATTERN "*.hpp"
        PATTERN "*.windows.*" EXCLUDE
        PATTERN "Examples" EXCLUDE
        PATTERN "tsgHiddenExternals.hpp" EXCLUDE)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/TasmanianSparseGrid.h"
        DESTINATION include
        PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Examples/"
        DESTINATION "share/Tasmanian/examples/"
        FILES_MATCHING PATTERN "*.cpp"
        PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ)

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/GaussPattersonRule.table"
        DESTINATION "share/Tasmanian/"
        PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ)

install(TARGETS Tasmanian_tasgrid
        EXPORT "${CMAKE_PROJECT_NAME}-exports"
        RUNTIME DESTINATION "bin"
        LIBRARY DESTINATION "lib"
        ARCHIVE DESTINATION "lib")
