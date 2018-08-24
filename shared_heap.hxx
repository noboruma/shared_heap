#include "shared_heap.hh"

namespace allocator {

    template<int HEAP_SIZE>
    SharedHeap<HEAP_SIZE>::SharedHeap()
    : _heapfd(-1)
    , _heap(nullptr)
    {
        _heapfd = open("./shared", O_CREAT|O_DIRECT|O_RDWR, S_IRUSR|S_IWUSR);
        if(errno != 0) {
            throw exception::LibcFault(std::string{} + "open error: " + strerror(errno));
        }
        ftruncate(_heapfd, HEAP_SIZE);
        if(errno != 0) {
            throw exception::LibcFault(std::string{} + "ftruncate error: " + strerror(errno));
        }
        _heap = static_cast<char*>(mmap(NULL, HEAP_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, _heapfd, 0));
        if(errno != 0) {
            throw exception::LibcFault(std::string{} + "mmap error: " + strerror(errno));
        }

        _maccess= next_aligned_addr<std::mutex>(_heap);
        _head = next_aligned_addr<typename BookKeeperList::Node>(reinterpret_cast<char*>(_maccess)+sizeof(std::mutex));

        auto lfd = open("./lock", O_CREAT|O_DIRECT|O_RDWR, S_IRUSR|S_IWUSR);
        if(errno != 0) {
            throw exception::LibcFault(std::string{} + "open error: " + strerror(errno));
        }
        lockf(lfd, F_LOCK, 0);
        if(_head->occupy_size == 0 && _head->free_size == 0) {
            _maccess = new (_maccess) std::mutex{};
            _head->occupy_size = 0;
            _head->free_size   = HEAP_SIZE - (_heap - reinterpret_cast<char*>(_head));
        }
        lockf(lfd, F_ULOCK, 0);
        close(lfd);
        _end   = reinterpret_cast<char*>(_head) + HEAP_SIZE - (_heap - reinterpret_cast<char*>(_head));
    }

    template<int HEAP_SIZE>
    void SharedHeap<HEAP_SIZE>::print()
    {
        std::cout <<  std::endl;
        std::cout << "list:" << std::endl;

        typename BookKeeperList::iterator nodeIt = _head;
        while(nodeIt != _end) {
            std::cout << "node: " << &nodeIt << std::endl;
            std::cout << "end: " << (void*)_end << std::endl;
            std::cout << "occupy: " << nodeIt->occupy_size << std::endl;
            std::cout << "free: " << nodeIt->free_size << std::endl;
            std::cout <<  std::endl;
            ++nodeIt;
        }
    }

    template<int HEAP_SIZE>
    SharedHeap<HEAP_SIZE>::~SharedHeap() noexcept {
        if(_heap != nullptr) {
            munmap(_heap, HEAP_SIZE);
            if(errno != 0) {
                std::cerr << std::string{} + "unmap error: " + strerror(errno)<<std::endl;
            }
        }
        if(_heapfd != -1) {
            close(_heapfd);
            if(errno != 0) {
                std::cerr << std::string{} +"close error: " + strerror(errno)<<std::endl;
            }
        }
    }

    template<int HEAP_SIZE>
    void SharedHeap<HEAP_SIZE>::deallocate(void * ptr)
    {
        std::lock_guard<std::mutex> l(*_maccess);
        typename BookKeeperList::iterator nodeIt = _head,
                 prevNodeIt = nodeIt,
                 nextNodeIt = _head;
        ++nextNodeIt;

        while(nodeIt->data() != ptr) {
            prevNodeIt = nodeIt;
            nodeIt = nextNodeIt;
            ++nextNodeIt;
        }
        if (nodeIt != _end) {
            nodeIt->free_size = nodeIt->occupy_size;
            nodeIt->occupy_size = 0;
            if(prevNodeIt != nodeIt) {
                if(prevNodeIt->free_size != 0) {
                    prevNodeIt->free_size += nodeIt->free_size;
                    nodeIt = prevNodeIt;
                }
            }
            if(nextNodeIt != _end) {
                if(nextNodeIt->free_size != 0) {
                    nodeIt->free_size +=  nextNodeIt->free_size;
                }
            }
            return;
        }
        throw exception::OutOfBound("wrong deallocation");
    }

    template<int HEAP_SIZE>
    template<typename T>
    T* SharedHeap<HEAP_SIZE>::next_aligned_addr(char * next_addr)
    {
        void * top = reinterpret_cast<void*>(next_addr);
        size_t capacity =_end-reinterpret_cast<char*>(top);
        auto * next_valid_addr = reinterpret_cast<T*>(std::align(alignof(T), sizeof(T), top, capacity));
        if(next_valid_addr == nullptr) {
            throw exception::OutOfMemory("not enough space");
        }
        return next_valid_addr;
    }

    template<int HEAP_SIZE>
    template<typename T>
    T* SharedHeap<HEAP_SIZE>::allocate(size_t n)
    {
        if(!std::is_standard_layout<T>::value) {
            throw exception::MemoryFormatFault("type not trivial!");
        }
        std::lock_guard<std::mutex> l(*_maccess);
        typename BookKeeperList::iterator it = _head;
        while(it != _end) {
            if(it->free_size > n*sizeof(T) + sizeof(typename BookKeeperList::Node)) {
                auto *top = reinterpret_cast<char*>(it->data());
                auto nextTAddr = next_aligned_addr<T>(top);
                top = reinterpret_cast<char*>(nextTAddr);
                top += sizeof(T)*n;

                auto old_size = it->free_size;
                auto nextNodeAddr = next_aligned_addr<typename BookKeeperList::Node>(top);

                it->occupy_size = reinterpret_cast<char*>(nextNodeAddr) - reinterpret_cast<char*>(&it);
                it->free_size = 0;
                auto *nextNode = new (nextNodeAddr) typename BookKeeperList::Node();
                nextNode->occupy_size = 0;
                nextNode->free_size = old_size - it->occupy_size;
                return nextTAddr;
            }
            ++it;
        }
        throw exception::OutOfMemory("not enough space");
    }
}
