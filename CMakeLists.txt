project(DpGMMs)
cmake_minimum_required(VERSION 2.4.6)
set(CMAKE_COLOR_MAKEFILE ON)
  set(CMAKE_BUILD_TYPE Debug) # Release; debug; ...
  set(CMAKE_BUILD_TYPE Release) # Release; debug; ...
  #ENDIF()

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

# add flags depending on build type
set(CMAKE_CXX_FLAGS_RELEASE "-g -Wall -fopenmp -Wno-deprecated -O2 -finline-functions -DNDEBUG -DCUDA") # -DNDEBUG
set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall -Wno-deprecated  -DCUDA")
set(CUDA_NVCC_FLAGS ${CUDA_NVCC_FLAGS};-gencode arch=compute_20,code=sm_20 -gencode arch=compute_30,code=sm_30 -gencode arch=compute_35,code=\"sm_35,compute_35\")

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

find_package (Eigen3 REQUIRED)
find_package (Boost REQUIRED)
find_package (OpenMP REQUIRED)
find_package (CUDA REQUIRED)

message(${EIGEN3_INCLUDE_DIR})
message(${CUDA_TOOLKIT_ROOT_DIR})

include_directories(
  ./include
  ./cuda
  ${EIGEN3_INCLUDE_DIR}
  ${CUDA_TOOLKIT_ROOT_DIR}/targets/x86_64-linux/include
  /usr/include
  #/usr/include/eigen3
  $ENV{WORKSPACE_HOME}/3rdparty/include
)
link_directories(
  $ENV{WORKSPACE_HOME}/3rdparty/lib
  ${CUDA_TOOLKIT_ROOT_DIR}/targets/x86_64-linux/lib
)

cuda_add_executable(dpStickGMM ./src/dpStickGMM.cpp
  ./src/distribution.cpp
  ./src/cat.cpp
  ./src/mult.cpp
  ./src/niw.cpp
  ./src/iw.cpp
  ./src/dir.cpp
  ./src/normal.cpp
  ./src/niwSphere.cpp
  ./src/niwSphereFull.cpp
  ./src/niwTangent.cpp
  ./src/sphere.cpp
  ./src/sampler.cpp
#  ./src/gpuMatrix.cpp
  ./cuda/choiceMult_kernel.cu
  ./cuda/sphereMixturePdf_kernel.cu
  ./cuda/karcherMeans_kernel.cu
  ./cuda/log_p_kernel.cu
  ./cuda/gmmPdf_kernel.cu
  ./cuda/sufficientStatistics_kernel.cu
)
target_link_libraries(dpStickGMM
  boost_system
  boost_random
  boost_program_options
)

cuda_add_executable(dpDirMM ./src/dpDirMM.cpp
  ./src/distribution.cpp
  ./src/niw.cpp
  ./src/iw.cpp
  ./src/normal.cpp
  ./src/cat.cpp
  ./src/mult.cpp
  ./src/dir.cpp
  ./src/niwSphere.cpp
  ./src/niwSphereFull.cpp
  ./src/niwTangent.cpp
  ./src/sphere.cpp
  ./src/sampler.cpp
  ./cuda/choiceMult_kernel.cu
)
target_link_libraries(dpDirMM
  boost_system
  boost_random
  boost_program_options
)

add_executable(dpSubclusterGMM ./src/dpSubclusterGMM.cpp
  ./src/dpSubclustersMM.cpp
  ./include/dpmmSubclusters/cluster_sampledT.cpp
  ./include/dpmmSubclusters/normal.cpp
  ./include/dpmmSubclusters/niw_sampled.cpp
)
target_link_libraries(dpSubclusterGMM
  boost_system
  boost_random
  boost_program_options
  gsl gslcblas m rt
)

add_executable(dpSubclusterSphereGMM ./src/dpSubclusterSphereGMM.cpp
  ./src/dpSubclustersSphereMM.cpp
  ./include/dpmmSubclusters/cluster_sampledT.cpp
  ./include/dpmmSubclusters/normal.cpp
  ./src/niwSphere_sampled.cpp
)
target_link_libraries(dpSubclusterSphereGMM
  boost_system
  boost_random
  boost_program_options
  gsl gslcblas m rt
)

#cuda_add_executable(realtimeMF_openni
#  ./src/realtimeMF_openni.cpp
#  ./src/cv_helpers.cpp
#  ./src/pcl_helpers.cpp
#  ./src/realtimeMF.cpp
#  ./src/optimizationSO3.cpp
#  ./src/optimizationSO3_approx.cu
#  ./src/convolutionSeparable.cu
#  ./src/convolutionSeparable_small.cu
#  ./src/cuda_pcl_helpers.cu
#  ./src/optimizationSO3.cu)
#target_link_libraries (realtimeMF_openni
#  boost_system boost_thread
#  pcl_common pcl_io pcl_visualization pcl_filters
#  OpenNI XnVNite
#  vtkCommon vtkFiltering vtkRendering vtkGraphics
#  opencv_core opencv_highgui)


add_executable(testDist ./test/distributions.cpp
    ./src/dir.cpp
    ./src/cat.cpp
    ./src/niw.cpp
    ./src/iw.cpp
    ./src/normal.cpp
  ./include/dpmmSubclusters/niw_sampled.cpp
  ./include/dpmmSubclusters/normal.cpp
)
target_link_libraries(testDist
  boost_system
  boost_random
  boost_unit_test_framework
  gsl gslcblas m rt
)

add_executable(generateSphericalData ./src/generateSphericalData.cpp
  ./src/normal.cpp
  ./src/iw.cpp
  ./src/sphere.cpp
)
target_link_libraries(generateSphericalData
  boost_system
  boost_random
  boost_program_options
)

cuda_add_executable(testDirMM ./test/dirMM.cpp
    ./src/dir.cpp
    ./src/cat.cpp
    ./src/niw.cpp
    ./src/iw.cpp
    ./src/normal.cpp
  #  ./src/niwBaseMeasure.cpp
    ./src/niwSphere.cpp
    ./src/sampler.cpp
    ./src/sphere.cpp
  #  ./src/gpuMatrix.cpp
  ./cuda/choiceMult_kernel.cu
  ./cuda/log_p_kernel.cu
  ./cuda/karcherMeans_kernel.cu
  ./cuda/sufficientStatistics_kernel.cu
  ./cuda/sphereMixturePdf_kernel.cu
)
target_link_libraries(testDirMM
  boost_system
  boost_random
  boost_unit_test_framework
)

cuda_add_executable(testSphere ./test/sphere.cpp
    ./src/normal.cpp
    ./src/sphere.cpp
    ./src/sampler.cpp
  ./cuda/log_p_kernel.cu
  ./cuda/karcherMeans_kernel.cu
  ./cuda/sufficientStatistics_kernel.cu
  ./cuda/sphereMixturePdf_kernel.cu
  ./cuda/choiceMult_kernel.cu
)
target_link_libraries(testSphere
  boost_system
  boost_random
  boost_unit_test_framework
)

cuda_add_executable(testSampler ./test/sampler.cpp
  ./src/sampler.cpp
  ./src/gpuMatrix.cpp
  ./cuda/choiceMult_kernel.cu
)
target_link_libraries(testSampler
  boost_system
  boost_random
  boost_unit_test_framework
)

cuda_add_executable(testClData ./test/clData.cpp
  ./src/gpuMatrix.cpp
  ./src/normal.cpp
  ./cuda/log_p_kernel.cu
  ./cuda/karcherMeans_kernel.cu
  ./cuda/sufficientStatistics_kernel.cu
)
target_link_libraries(testClData
  boost_system
  boost_random
  boost_unit_test_framework
)

cuda_add_executable(testSPKM ./test/spkm.cpp
)
target_link_libraries(testSPKM
  boost_system
  boost_random
  boost_unit_test_framework
)

cuda_add_executable(testDpSubclusterMM ./test/dpSubclusterMM.cpp
    ./src/cat.cpp
    ./src/dir.cpp
    ./src/niw.cpp
    ./src/iw.cpp
    ./src/normal.cpp
    ./src/niwSphere.cpp
    ./src/niwSphereFull.cpp
    ./src/niwTangent.cpp
    ./src/sphere.cpp
  ./src/sampler.cpp
  ./src/gpuMatrix.cpp
  ./cuda/choiceMult_kernel.cu
)
target_link_libraries(testDpSubclusterMM
  boost_system
  boost_random
  boost_unit_test_framework
)