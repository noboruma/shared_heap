#include "shared_heap.hh"
#include "exceptions.hh"

namespace allocator {

    template<int HEAP_SIZE>
    SharedHeap<HEAP_SIZE>::SharedHeap()
    : _heap(nullptr)
    , _heapfd(-1)
    , _capacity(HEAP_SIZE)
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
        _head = next_aligned_addr<typename BookList::Node>(_heap);
        if(_head->occupy_size == 0 && _head->free_size == 0) {
            _head->occupy_size = 0;
            _head->free_size   = HEAP_SIZE - ((char*)_heap - (char*)_head);
        }
        _end = (char*)_head + _head->free_size;
    }

    template<int HEAP_SIZE>
    void SharedHeap<HEAP_SIZE>::print()
    {
        std::cout <<  std::endl;
        std::cout << "list:" << std::endl;

        typename BookList::iterator nodeIt = _head;
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
        munmap(_heap, HEAP_SIZE);
        if(errno != 0) {
            std::cerr << std::string{} + "unmap error: " + strerror(errno)<<std::endl;
        }
        close(_heapfd);
        if(errno != 0) {
            std::cerr << std::string{} +"close error: " + strerror(errno)<<std::endl;
        }
    }

    template<int HEAP_SIZE>
    void SharedHeap<HEAP_SIZE>::deallocate(void * ptr)
    {
        print();

        typename BookList::iterator nodeIt = _head,
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
                }
                nodeIt = prevNodeIt;
            }
            if(nextNodeIt != _end) {
                if(nextNodeIt->free_size != 0) {
                    nodeIt->free_size +=  nextNodeIt->free_size;
                }
            }
            print();
            return;
        }
        throw exception::OutOfBound("wrong deallocation");
    }
}
