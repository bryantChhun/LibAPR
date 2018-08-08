//
// Created by gonciarz on 8/8/18.
//

#ifndef LIBAPR_CUDAMEMORY_HPP
#define LIBAPR_CUDAMEMORY_HPP

#include <iostream>
#include <cuda_runtime.h>
#include <device_launch_parameters.h>

inline cudaError_t checkCuda(cudaError_t result) {
#if defined(DEBUG) || defined(_DEBUG)
    if (result != cudaSuccess) {
        fprintf(stderr, "CUDA Runtime Error: %s\n", cudaGetErrorString(result));
        assert(result == cudaSuccess);
    }
#endif
    return result;
}

template <typename T>
inline T* getPinnedMemory(size_t aNumOfBytes) {
    T *memory = nullptr;
    cudaError_t result = checkCuda(cudaMallocHost((void**)&memory, aNumOfBytes) );
    std::cout << "Allocating pinned memory " << aNumOfBytes << " at " << (void*)memory << " result " << result << std::endl;
    return memory;
};

template <typename T>
inline void freePinnedMemory(T *aMemory) {
    std::cout << "Freeing pinned memory " << (void*)aMemory << std::endl;
    cudaFreeHost(aMemory);
}


#endif //LIBAPR_CUDAMEMORY_HPP
