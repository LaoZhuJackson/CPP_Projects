#pragma once

#include <cmath>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#include "KICachePolicy.h" // 假设这是一个定义了缓存策略接口的头文件

namespace KamaCache
{
    // 前向声明 KLfuCache 类，以便在 FreqList 中声明为友元
    template<typename Key, typename Value> class KLfuCache;

    // FreqList 类：表示一个具有特定访问频率的节点双向链表
    template<typename Key, typename Value>
    class FreqList {
    private:
        // 节点结构体：存储键、值、频率和前后指针
        struct Node
        {
            int freq; // 节点的访问频率
            Key key;
            Value value;
            std::shared_ptr<Node> pre; // 指向前一个节点的智能指针
            std::shared_ptr<Node> next; // 指向后一个节点的智能指针

            Node() : freq(1), pre(nullptr), next(nullptr) {}
            Node(Key key, Value value) : freq(1), key(key), value(value), pre(nullptr), next(nullptr) {}
        };

        using NodePtr = std::shared_ptr<Node>; // 节点指针类型别名
        int freq_; // 此链表所代表的频率值
        NodePtr head_; // 哨兵头节点，不存储实际数据，用于简化链表操作
        NodePtr tail_; // 哨兵尾节点，不存储实际数据，用于简化链表操作

    public:
        // 构造函数：初始化频率值并创建头尾哨兵节点，连接它们形成一个空链表
        explicit FreqList(int n) : freq_(n) {
            head_ = std::make_shared<Node>();
            tail_ = std::make_shared<Node>();
            head_->next = tail_;
            tail_->pre = head_;
        }

        // isEmpty: 判断此频率链表是否为空（只有哨兵节点）
        bool isEmpty() const {
            return head_->next == tail_;
        }

        // addNode: 将节点添加到此链表的尾部（tail哨兵之前）
        void addNode(NodePtr node) {
            if (!node || !head_ || !tail_) return;

            node->pre = tail_->pre;
            node->next = tail_;
            tail_->pre->next = node;
            tail_->pre = node;
        }

        // removeNode: 从链表中移除指定节点
        void removeNode(NodePtr node) {
            if (!node || !head_ || !tail_) return;
            if (!node->pre || !node->next) return; // 确保节点已在链表中

            node->pre->next = node->next;
            node->next->pre = node->pre;
            // 断开节点与原链表的连接
            node->pre = nullptr;
            node->next = nullptr;
        }

        // getFirstNode: 获取此链表中的第一个实际节点（头哨兵的下一个）
        // 用于淘汰策略（移除最不经常使用的节点，即链表最先插入的节点，LRU思想）
        NodePtr getFirstNode() const { return head_->next; }

        // 声明 KLfuCache 为友元类，允许其访问 FreqList 的私有成员
        friend class KLfuCache<Key, Value>;
    };

    // KLfuCache 类：继承自 KICachePolicy，实现 LFU 缓存策略
    template <typename Key, typename Value>
    class KLfuCache : public KICachePolicy<Key, Value> {
    public:
        // 使用别名简化类型名称
        using Node = typename FreqList<Key, Value>::Node;
        using NodePtr = std::shared_ptr<Node>;
        using NodeMap = std::unordered_map<Key, NodePtr>; // 键到节点指针的映射

        // 构造函数：初始化缓存容量、最小频率、最大平均访问次数等
        KLfuCache(int capacity, int maxAverageNum = 10)
            : capacity_(capacity),
            minFreq_(INT8_MAX), // 初始化为最大可能值，便于后续比较
            maxAverageNum_(maxAverageNum),
            curAverageNum_(0),
            curTotalNum_(0) {}

        // 析构函数：使用默认实现
        ~KLfuCache() override = default;

        // put: 向缓存中插入或更新键值对（线程安全）
        void put(Key key, Value value) override {
            if (capacity_ == 0) return; // 容量为0，直接返回

            std::lock_guard<std::mutex> lock(mutex_); // 加锁
            auto it = nodeMap_.find(key);
            if (it != nodeMap_.end()) {
                // 键已存在，更新其值并增加其访问频率（模拟一次访问）
                it->second->value = value;
                getInternal(it->second, value); // 通过getInternal模拟访问
                return;
            }
            // 键不存在，执行插入操作
            putInternal(key, value);
        }

        // get (重载1): 根据键查找值，通过引用参数返回，返回值表示是否找到（线程安全）
        bool get(Key key, Value& value) override {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = nodeMap_.find(key);
            if (it != nodeMap_.end()) {
                getInternal(it->second, value); // 找到，通过getInternal处理频率更新
                return true;
            }
            return false; // 未找到
        }

        // get (重载2): 根据键查找值，直接返回值（若未找到则返回默认构造的Value）
        Value get(Key key) override {
            Value value;
            get(key, value); // 调用上一个get函数
            return value;
        }

        // purge: 清空缓存，回收所有资源
        void purge() {
            // 注意：此函数未加锁，调用者需确保线程安全
            nodeMap_.clear();
            freqToFreqList_.clear();
        }

    private:
        // putInternal: 内部实现的插入逻辑（假定已持有锁）
        void putInternal(Key key, Value value);
        // getInternal: 内部实现的获取逻辑（假定已持有锁），处理频率提升
        void getInternal(NodePtr node, Value& value);

        // kickOut: 当缓存满时，淘汰最小频率链表中最旧的节点
        void kickOut();
        // removeFromFreqList: 将节点从其当前频率链表中移除
        void removeFromFreqList(NodePtr node);
        // addToFreqList: 将节点添加到其新频率对应的链表中
        void addToFreqList(NodePtr node);

        // addFreqNum: 增加总访问次数并重新计算平均访问次数，检查是否超过阈值
        void addFreqNum();
        // decreaseFreqNum: 减少总访问次数（减少指定数值）并重新计算平均访问次数
        void decreaseFreqNum(int num);
        // handleOverMaxAverageNum: 当平均访问次数超过阈值时，对所有节点的频率进行衰减
        void handleOverMaxAverageNum();
        // updateMinFreq: 更新minFreq_变量，遍历所有频率链表找到最小的非空链表频率
        void updateMinFreq();

    private:
        int capacity_; // 缓存容量（最大键值对数量）
        int minFreq_; // 当前缓存中的所有频率的最小值，用于快速确定淘汰目标
        int maxAverageNum_; // 平均访问次数的上限阈值
        int curAverageNum_; // 当前的平均访问次数 (curTotalNum_ / nodeMap_.size())
        int curTotalNum_; // 总的访问次数累计
        std::mutex mutex_; // 互斥锁，用于保证线程安全
        NodeMap nodeMap_; // 存储键到节点映射的哈希表
        // 存储频率值到对应频率链表的映射。注意：值是指针，需手动管理内存（代码中存在潜在内存泄漏风险）
        std::unordered_map<int, FreqList<Key, Value>*> freqToFreqList_;
    };

    // getInternal 实现：处理缓存命中后的频率提升和节点迁移
    template<typename Key, typename Value>
    void KLfuCache<Key, Value>::getInternal(NodePtr node, Value& value) {
        value = node->value; // 传出值
        removeFromFreqList(node); // 从旧频率链表移除
        node->freq++; // 访问频率+1
        addToFreqList(node); // 添加到新频率链表

        // 如果节点原来的频率等于minFreq_，且原来的链表在移除该节点后变空，
        // 则最小频率需要增加到当前节点的新频率（即minFreq_ + 1）
        if (node->freq - 1 == minFreq_ && freqToFreqList_[node->freq - 1]->isEmpty())
            minFreq_++;

        // 增加总访问次数并检查平均访问次数
        addFreqNum();
    }

    // putInternal 实现：处理新键的插入
    template<typename Key, typename Value>
    void KLfuCache<Key, Value>::putInternal(Key key, Value value) {
        if (nodeMap_.size() == capacity_) {
            kickOut(); // 缓存已满，淘汰一个节点
        }
        NodePtr node = std::make_shared<Node>(key, value); // 创建新节点，频率为1
        nodeMap_[key] = node; // 加入哈希表
        addToFreqList(node); // 加入频率为1的链表
        addFreqNum(); // 增加访问计数（新插入视为一次访问？这里的设计值得商榷）
        minFreq_ = 1; // 新节点频率为1，所以最小频率肯定是1。使用std::min是保守写法，直接赋1更准确。
    }

    // kickOut 实现：淘汰一个节点
    template<typename Key, typename Value>
    void KLfuCache<Key, Value>::kickOut() {
        // 获取最小频率链表中的第一个节点（最久未访问的，LRU策略）
        NodePtr node = freqToFreqList_[minFreq_]->getFirstNode();
        removeFromFreqList(node); // 从链表移除
        nodeMap_.erase(node->key); // 从哈希表移除
        decreaseFreqNum(node->freq); // 减少总访问次数（减去被淘汰节点的频率）
    }

    // removeFromFreqList 实现：从指定频率链表移除节点
    template<typename Key, typename Value>
    void KLfuCache<Key, Value>::removeFromFreqList(NodePtr node) {
        if (!node) return;
        auto freq = node->freq;
        // 查找频率对应的链表并移除节点
        auto listIt = freqToFreqList_.find(freq);
        if (listIt != freqToFreqList_.end() && listIt->second != nullptr) {
            listIt->second->removeNode(node);
        }
        // 注意：此处没有检查链表是否为空并释放链表内存，可能导致内存泄漏。
    }

    // addToFreqList 实现：将节点添加到其频率对应的链表
    template<typename Key, typename Value>
    void KLfuCache<Key, Value>::addToFreqList(NodePtr node) {
        if (!node) return;
        auto freq = node->freq;
        // 如果该频率对应的链表不存在，则创建它
        if (freqToFreqList_.find(freq) == freqToFreqList_.end()) {
            // 使用new创建，需在析构或purge时delete，否则内存泄漏。
            // 更好的做法是使用std::unique_ptr或其他RAII容器。
            freqToFreqList_[freq] = new FreqList<Key, Value>(freq);
        }
        freqToFreqList_[freq]->addNode(node); // 将节点加入链表
    }

    // addFreqNum 实现：更新总访问次数和平均访问次数，并检查是否需要处理超额
    template<typename Key, typename Value>
    void KLfuCache<Key, Value>::addFreqNum() {
        curTotalNum_++; // 总访问次数+1
        if (nodeMap_.empty()) {
            curAverageNum_ = 0;
        } else {
            curAverageNum_ = curTotalNum_ / nodeMap_.size(); // 计算平均访问次数
        }
        // 如果平均访问次数超过阈值，则进行处理（频率衰减）
        if (curAverageNum_ > maxAverageNum_) {
            handleOverMaxAverageNum();
        }
    }

    // decreaseFreqNum 实现：减少总访问次数并重新计算平均访问次数
    template<typename Key, typename Value>
    void KLfuCache<Key, Value>::decreaseFreqNum(int num) {
        curTotalNum_ -= num;
        if (nodeMap_.empty()) {
            curAverageNum_ = 0;
        } else {
            curAverageNum_ = curTotalNum_ / nodeMap_.size();
        }
    }

    // handleOverMaxAverageNum 实现：处理平均访问次数过高的策略（频率衰减）
    template<typename Key, typename Value>
    void KLfuCache<Key, Value>::handleOverMaxAverageNum() {
        if (nodeMap_.empty()) return;
        for (auto it = nodeMap_.begin(); it != nodeMap_.end(); ++it) {
            if (!it->second) continue;
            NodePtr node = it->second;
            removeFromFreqList(node); // 从当前链表移除

            // 频率衰减：每个节点的频率减少 maxAverageNum_ / 2
            node->freq -= maxAverageNum_ / 2;
            if (node->freq < 1) node->freq = 1; // 保证频率不低于1

            addToFreqList(node); // 添加到衰减后的新频率链表
        }
        // 衰减操作后，所有节点的频率都变了，需要重新计算最小频率
        updateMinFreq();
        // 衰减后，总访问次数和平均访问次数也应调整，但此函数中未体现，这是一个逻辑缺陷。
        // 通常需要在衰减后调用 decreaseFreqNum 或类似函数来调整 curTotalNum_。
    }

    // updateMinFreq 实现：遍历所有频率链表，找到最小的非空链表对应的频率
    template<typename Key, typename Value>
    void KLfuCache<Key, Value>::updateMinFreq() {
        minFreq_ = INT8_MAX; // 重置为最大值
        for (const auto& pair : freqToFreqList_) {
            // 检查链表是否存在且非空
            if (pair.second && !pair.second->isEmpty()) {
                minFreq_ = std::min(minFreq_, pair.first);
            }
        }
        // 如果所有链表都为空，则将最小频率重置为1（准备接收新节点）
        if (minFreq_ == INT8_MAX) minFreq_ = 1;
    }

    // KHashLfuCache 类：分片LFU缓存，通过哈希将键分布到多个KLfuCache实例中
    template<typename Key, typename Value>
    class KHashLfuCache {
    public:
        // 构造函数：根据总容量和分片数初始化多个KLfuCache
        KHashLfuCache(size_t capacity, int sliceNum, int maxAverageNum = 10)
            : sliceNum_(sliceNum > 0 ? sliceNum : std::thread::hardware_concurrency()), // 分片数默认为CPU核心数
            capacity_(capacity) {
            // 计算每个分片的容量（向上取整）
            size_t sliceSize = std::ceil(capacity_ / static_cast<double>(sliceNum_));
            for (int i = 0; i < sliceNum_; ++i) {
                // 为每个分片创建KLfuCache实例
                lfuSliceCaches_.emplace_back(new KLfuCache<Key, Value>(sliceSize, maxAverageNum));
            }
        }

        // put: 根据键的哈希值决定放入哪个分片
        void put(Key key, Value value) {
            size_t sliceIndex = Hash(key) % sliceNum_;
            return lfuSliceCaches_[sliceIndex]->put(key, value);
        }

        // get (重载1): 根据键的哈希值决定从哪个分片查找
        void get(Key key, Value& value) {
            size_t sliceIndex = Hash(key) % sliceNum_;
            return lfuSliceCaches_[sliceIndex]->get(key, value);
        }

        // get (重载2): 根据键的哈希值决定从哪个分片查找
        Value get(Key key) {
            Value value;
            get(key, value);
            return value;
        }

        // purge: 清空所有分片缓存
        void purge() {
            for (auto& lfuSliceCache : lfuSliceCaches_) {
                lfuSliceCache->purge();
            }
        }

    private:
        // Hash: 使用std::hash计算键的哈希值
        size_t Hash(Key key) {
            std::hash<Key> hashFunc;
            return hashFunc(key);
        }

    private:
        size_t capacity_; // 总容量
        int sliceNum_; // 分片数量
        // 存储分片缓存实例的向量，使用unique_ptr管理内存
        std::vector<std::unique_ptr<KLfuCache<Key, Value>>> lfuSliceCaches_;
    };

} // namespace KamaCache