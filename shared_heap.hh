#ifndef SHARED_HEAP
#define SHARED_HEAP

#include "exceptions.hh"

#include <cstdlib>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <linux/memfd.h>
#include <memory>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <mutex>

namespace allocator {
    template<int HEAP_SIZE=1024>
    struct SharedHeap {
        public:
            template<typename T>
            struct SharedAllocator {

                typedef size_t    size_type;
                typedef T*        pointer;
                typedef const T*  const_pointer;
                typedef T&        reference;
                typedef const T&  const_reference;
                typedef T         value_type;

                SharedAllocator(SharedHeap &parent) : _parent(parent) {}
                T* allocate(size_t n, const void * hint=0) {
                    std::cout << "allocate " << n << std::endl;
                    return _parent.template allocate<T>(n);
                }
                void deallocate(void * p, std::size_t n) {
                    std::cout << "deallocate" << p << std::endl;
                    _parent.deallocate(p);
                }
                pointer           address(reference x) const { return &x; }
                const_pointer     address(const_reference x) const { return &x; }
                SharedAllocator<T>&  operator=(const SharedAllocator&) { return *this; }
                void              construct(pointer p, const T& val) { new ((T*) p) T(val); }
                void              destroy(pointer p) { p->~T(); }
                size_type         max_size() const { return size_t(-1); }

                template <class U>
                struct rebind { typedef SharedAllocator<U> other; };

                template <class U>
                SharedAllocator(const SharedAllocator<U>&) {}

                template <class U>
                SharedAllocator& operator=(const SharedAllocator<U>&) { return *this; }
                SharedHeap &_parent;
            };
            template<typename T>
            SharedAllocator<T> getAllocator() {return SharedAllocator<T>(*this); }

        public:
            SharedHeap();
            SharedHeap(SharedHeap const &)           = delete;
            SharedHeap(SharedHeap &&)                = default;
            SharedHeap& operator=(SharedHeap const&) = delete;
            SharedHeap& operator=(SharedHeap &&)     = default;

            void print();

            ~SharedHeap() noexcept;

        protected:
            void deallocate(void* ptr);

            template<typename T>
            T* next_aligned_addr(char * next_addr);

            template<typename T>
            T* allocate(size_t n);

            struct BookKeeperList {
                struct Node {
                    size_t free_size;
                    size_t occupy_size;

                    void* data() {
                        return reinterpret_cast<char*>(this) + sizeof(Node);
                    }
                };
                struct iterator {
                    iterator(Node* ptr) : node (ptr){}
                    Node& operator*() {
                        return *node;
                    }
                    Node* operator&() {
                        return node;
                    }
                    Node* operator->() {
                        return node;
                    }
                    void operator++() {
                        node = reinterpret_cast<Node*>(reinterpret_cast<char*>(node) + node->occupy_size + node->free_size);
                    }
                    bool operator!=(iterator& other){
                        return node != other.node;
                    }
                    bool operator!=(Node & other){
                        return node != other;
                    }
                    bool operator!=(char * other){
                        return (char*)node != other;
                    }
                    Node * node;
                };
            };

        private:
            int   _heapfd;
            char *_heap;
            typename BookKeeperList::Node *_head;
            char *_end;
            std::mutex *_maccess;

    };
}//!allocator

#include "shared_heap.hxx"

#endif
