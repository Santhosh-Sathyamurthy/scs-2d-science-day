#ifndef YDS_ALLOCATOR_H
#define YDS_ALLOCATOR_H

#include <malloc.h>
#include <stdlib.h>

// Platform detection for aligned allocation
#if defined(_WIN32) || defined(_WIN64)
    #ifndef YDS_ALIGNED_MALLOC
        #define YDS_ALIGNED_MALLOC(size, alignment) _aligned_malloc(size, alignment)
    #endif
    #ifndef YDS_ALIGNED_FREE
        #define YDS_ALIGNED_FREE(ptr) _aligned_free(ptr)
    #endif
#else
    #include <cstdlib>
    #ifndef YDS_ALIGNED_MALLOC
        #define YDS_ALIGNED_MALLOC(size, alignment) aligned_alloc(alignment, size)
    #endif
    #ifndef YDS_ALIGNED_FREE
        #define YDS_ALIGNED_FREE(ptr) free(ptr)
    #endif
#endif

class ysAllocator {
public:
    template <int Alignment>
    static void *BlockAllocate(int size) {
        return YDS_ALIGNED_MALLOC(size, Alignment);
    }

    static void BlockFree(void *block, int alignment) {
        if (alignment != 1) {
            YDS_ALIGNED_FREE(block);
        }
        else {
            ::free(block);
        }
    }

    template <typename T_Create, int Alignment>
    static T_Create *TypeAllocate(int n = 1, bool construct = true) {
        void *block = BlockAllocate<Alignment>(sizeof(T_Create) * n);
        T_Create *typedArray = reinterpret_cast<T_Create *>(block);

        if (construct) {
            for (int i = 0; i < n; i++) {
                new(typedArray + i) T_Create;
            }
        }

        return typedArray;
    }

    template <typename T_Free>
    static void TypeFree(T_Free *data, int n = 1, bool destroy = true, int alignment = 1) {
        void *block = reinterpret_cast<void *>(data);

        if (destroy) {
            for (int i = 0; i < n; i++) {
                data[i].~T_Free();
            }
        }

        BlockFree(block, alignment);
    }
};

// Template specialization for alignment=1 (moved outside class scope for GCC compliance)
template <>
inline void *ysAllocator::BlockAllocate<1>(int size) {
    return ::malloc(size);
}

#endif /* YDS_ALLOCATOR_H */