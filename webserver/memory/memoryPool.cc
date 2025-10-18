
#include "memoryPool.h"

namespace memoryPool
{
MemoryPool::MemoryPool(size_t BlockSize) : BlockSize_(BlockSize){}
MemoryPool::~MemoryPool(){
    //把连续的block删除
    Slot* cur = firstBlock_;
    while(cur){
        Slot* next = cur->next;
        // 等同于 free(reinterpret_cast<void*>(firstBlock_));
        // 转化为 void 指针避免调用析构函数，只用operator delete释放cur指针所指向的整个内存块空间
        operator delete(reinterpret_cast<void*>(cur));
        cur = next;
    }
}
void MemoryPool::init(size_t size){
    assert(size>0);
    SlotSize_ = size;
    firstBlock_ = nullptr;
    curSlot_ = nullptr;
    freeList_ = nullptr;
    lastSlot_ = nullptr;
}
void* MemoryPool::allocate(){
    //优先使用空闲链表中的内存槽
    if(freeList_ != nullptr){
        {
            std::lock_guard<std::mutex> lock(mutexForFreeList_);
            if(freeList_ != nullptr){
                Slot* temp = freeList_;
                freeList_ = freeList_->next;
                return temp;
            }
        }
    }
    Slot* temp;
    {
        std::lock_guard<std::mutex> lock(mutexForBlock_);
        // 当前内存块已无内存槽可用，开辟一块新的内存
        if(curSlot_ >= lastSlot_) allocateNewBlock();
        temp = curSlot_;
        // 这里不能直接 curSlot_ += SlotSize_ 因为curSlot_是Slot*类型，所以需要除以SlotSize_再加1
        curSlot_ += SlotSize_ / sizeof(Slot);
    }
    return temp;
}
void MemoryPool::deallocate(void* ptr){
    if(ptr){
        //回收内存，将内存通过头插法插入到空闲链表中
        std::lock_guard<std::mutex> lock(mutexForFreeList_);
        reinterpret_cast<Slot*>(ptr)->next = freeList_;//新节点的next指向当前头节点
        freeList_ = reinterpret_cast<Slot*>(ptr);//头指针指向新节点
    }
}

void MemoryPool::allocateNewBlock(){
    //std::cout << "申请一块内存块，SlotSize: " << SlotSize_ << std::endl;
    // 头插法插入新的内存块
    void* newBlock = operator new(BlockSize_);
    reinterpret_cast<Slot*>(newBlock)->next = firstBlock_;
    firstBlock_ = reinterpret_cast<Slot*>(newBlock);

    char* body = reinterpret_cast<char*>(newBlock) + sizeof(Slot*);//所以char* + N就是向后移动N个字节
    size_t paddingSize = padPointer(body,SlotSize_);// 计算对齐需要填充内存的大小
    curSlot_ = reinterpret_cast<Slot*>(body + paddingSize);

    //超过该标记位置，则说明该内存块已无内存槽可用，需向系统申请新的内存块
    lastSlot_ = reinterpret_cast<Slot*>(reinterpret_cast<size_t>(newBlock) + BlockSize_ - SlotSize_ + 1);

    freeList_ = nullptr;
}

size_t MemoryPool::padPointer(char* p,size_t align){
    return (align - reinterpret_cast<size_t>(p)) % align;
}
void HashBucket::initMemoryPool(){
    for(int i = 0;i<MEMORY_POOL_NUM;i++){
        getMemoryPool(i).init((i+1)*SLOT_BASE_SIZE);
    }
}

MemoryPool& HashBucket::getMemoryPool(int index){
    static MemoryPool memoryPool[MEMORY_POOL_NUM];// 第一次调用时初始化
    return memoryPool[index];
}

}// namespace memoryPool