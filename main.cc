#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

#if __GNUC__ <= 4
namespace std {
    void *align(std::size_t alignment,
                std::size_t size,
                void *&ptr,
                std::size_t &space) {
        std::uintptr_t pn = reinterpret_cast< std::uintptr_t >( ptr );
        std::uintptr_t aligned = ( pn + alignment - 1 ) & - alignment;
        std::size_t padding = aligned - pn;
        if ( space < size + padding ) return nullptr;
        space -= padding;
        return ptr = reinterpret_cast< void * >( aligned );
    }
}
#endif

#include "shared_heap.hh"

#ifdef V1
int main(int argc, const char *argv[])
{
    allocator::SharedHeap<1024> heap;
    heap.print();
    {
        std::vector<int, typename decltype(heap)::SharedAllocator<int>> v(3, int(), heap.getAllocator<int>());
        heap.print();
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
    heap.print();

    return 0;
}
#else
int main(int argc, const char *argv[])
{
    {
        allocator::SharedHeap<1024> heap;
        heap.print();
        {
            std::vector<int, typename decltype(heap)::SharedAllocator<int>> v(3, int(), heap.getAllocator<int>());
            heap.print();
            std::this_thread::sleep_for(std::chrono::seconds(10));
        }
        heap.print();
    }

    return 0;
}
#endif
