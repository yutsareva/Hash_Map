#include <vector>
#include <stdexcept>
#include <memory>

template<class KeyType, class ValueType>
struct node {
    typedef std::pair<const KeyType, ValueType> PairType;
    PairType p;
    node(const KeyType &key, const ValueType &value) : p(key, value) {}
};

template<class KeyType, class ValueType, class Hash = std::hash<KeyType> >
class HashMap {
    typedef node<const KeyType, ValueType> NodeType;
    typedef std::pair<const KeyType, ValueType> PairType;
private:
    constexpr static int DEFAULT_SIZE = 16;
    std::vector<std::shared_ptr<NodeType> > map_;
    std::vector<size_t> used_; // 1 -- used_; 2 -- removed;
    size_t count_;
    size_t mapsize_;
    Hash hasher_;
public:

template<bool is_const>
    class IterType {
        typedef typename std::conditional<is_const, const NodeType, NodeType>::type NodeIterType;
        typedef typename std::conditional<is_const, const HashMap, HashMap>::type HashMapType;
        typedef typename std::conditional<is_const, const PairType, PairType>::type PairIterType;
    public:
        HashMapType *owner_;
        std::shared_ptr<NodeIterType> ptr_;
        size_t pos_;
        IterType() : owner_(nullptr), ptr_(nullptr), pos_(0){};
        IterType(HashMapType *owner, std::shared_ptr<NodeIterType> ptr, size_t pos) : owner_(owner), ptr_(ptr), pos_(pos) {}

        IterType& operator++() {
            auto next = find_next(pos_+1);
            ptr_ = next.first;
            pos_ = next.second;
            return *this;
        }

        IterType operator++(int) {
            auto ret = IterType(owner_, ptr_, pos_);
            ++*this;
            return ret;
        }

        bool operator==(IterType other) const {
            return ptr_ == other.ptr_;
        }

        bool operator!=(IterType other) const {
            return ptr_ != other.ptr_;
        }

        PairIterType& operator*() {
            return (ptr_)->p;
        }

        PairIterType *operator->() {
            return &((ptr_)->p);
        }

        PairIterType& operator*() const{
            return (ptr_)->p;
        }

        PairIterType *operator->() const{
            return &((ptr_)->p);
        }

        std::pair<std::shared_ptr<NodeIterType>, size_t> find_next(size_t num) {
            if (num == owner_->mapsize_-1) {
                return {(owner_->map_[owner_->mapsize_-1]), owner_->mapsize_-1};
            }
            while (owner_->used_[num] != 1) {
                num += 1;
                num %= owner_->mapsize_-1;
                if (num == 0)
                    return {(owner_->map_[owner_->mapsize_-1]), owner_->mapsize_-1};
            }
            return {(owner_->map_[num]), num};
        }
    };

    size_t search_next(size_t num) const{
        while (used_[num] != 1) {
            num += 1;
            num %= mapsize_-1;
            if (num == 0)
                return mapsize_-1;
        }
        return num;
    }

    std::pair<std::shared_ptr<NodeType>, size_t> find_next(size_t num) {
        num = search_next(num);
        return {(map_[num]), num};
    }

    std::pair<std::shared_ptr<const NodeType>, size_t> find_next(size_t num) const{
        num = search_next(num);
        return {(map_[num]), num};
    }

    typedef IterType<true> const_iterator;
    typedef IterType<false> iterator;

    iterator begin() {
        auto beg = find_next(0);
        return iterator(this, beg.first, beg.second);
    }

    iterator end() {
        return iterator(this, map_[mapsize_-1], mapsize_-1);
    }

    const_iterator begin() const{
        auto beg = find_next(0);
        return const_iterator(this, beg.first, beg.second);
    }

    const_iterator end() const{
        return const_iterator(this, map_[mapsize_-1], mapsize_-1);
    }

    HashMap(Hash hasher = Hash()) : hasher_(hasher) {
        map_.resize(DEFAULT_SIZE, nullptr);
        used_.resize(DEFAULT_SIZE);
        mapsize_ = DEFAULT_SIZE;
        count_ = 0;
    }

    template<class InputIt>
    HashMap(InputIt begin, InputIt end, Hash hasher = Hash()) : HashMap(hasher) {
        while (begin != end) {
            insert(begin->first, begin->second);
            ++begin;
        }
    }

    HashMap(const std::initializer_list<PairType >& list, Hash hasher = Hash()) : HashMap(hasher) {
        for (auto elem : list) {
            insert(elem.first, elem.second);
        }
    }

    HashMap& operator=(const HashMap& other) {
        if (this == &other) {
            return *this;
        }
        clear();
        count_ = 0;
        map_.resize(DEFAULT_SIZE);
        used_.resize(DEFAULT_SIZE, 0);
        mapsize_ = DEFAULT_SIZE;
        hasher_ = other.hash_function();
        for (size_t i = 0; i < other.map_.size(); ++i) {
            if (other.used_[i] == 1){
                insert(other.map_[i]->p.first, other.map_[i]->p.second);
            }
        }
        return *this;
    }

    HashMap(const HashMap& other) {
        if (this == &other) {
            return;
        }
        count_ = 0;
        map_.resize(DEFAULT_SIZE);
        used_.resize(DEFAULT_SIZE, 0);
        mapsize_ = DEFAULT_SIZE;
        hasher_ = other.hash_function();
        for (size_t i = 0; i < other.map_.size(); ++i) {
            if (other.used_[i] == 1){
                insert(other.map_[i]->p.first, other.map_[i]->p.second);
            }
        }
    }

    ~HashMap() {
        clear();
    }

    void insert(const KeyType& key, const ValueType& value) {
        ++count_;
        if (count_ > mapsize_/2) {
            rebuild();
            ++count_;
        }
        size_t num = hasher_(key);
        num %= mapsize_-1;
        auto iter = find(key);
        if (iter.ptr_ != map_[mapsize_-1]) {
            --count_;
            return;
        }
        while (used_[num] == 1) {
            num += 1;
            num %= mapsize_-1;
        }
        map_[num] = std::make_shared<NodeType>(key, value);
        used_[num] = 1;
        return;
    }

    void insert(const PairType& p) {
        insert(p.first, p.second);
    }

    void rebuild() {
        count_ = 0;
        mapsize_ *= 2;
        std::vector<std::shared_ptr<NodeType> > newmap(mapsize_);
        swap(map_, newmap);
        std::vector<size_t> newused(mapsize_);
        swap(used_, newused);
        for (size_t i = 0; i + 1 < newmap.size(); ++i) {
            if (newused[i] == 1) {
                insert(newmap[i]->p.first, newmap[i]->p.second);
            }
        }
        for (size_t i = 0; i < newmap.size(); ++i) {
            newmap[i] = nullptr;
            }
    }

    size_t search(const KeyType& key) const {
        size_t num = hasher_(key);
        size_t pos = num;
        num %= mapsize_-1;
        while (used_[num] != 0) {
            if (used_[num] == 1 && (map_[num]->p).first == key) {
                return num;
            }
            num += 1;
            num %= mapsize_-1;
            if (pos == num)
                break;
        }
        return mapsize_-1;
    }

    iterator find(const KeyType& key) {
        size_t num = search(key);
        return iterator(this, map_[num], num);
    }

    const_iterator find(const KeyType& key) const{
        size_t num = search(key);
        return const_iterator(this, map_[num], num);
    }

    void erase(const KeyType& key) {
        auto iter = find(key);
        if (iter == this->end())
            return;
        used_[iter.pos_] = 2;
        map_[iter.pos_] = nullptr;
        --count_;
    }

    size_t size() const {
        return count_;
    }

    bool empty() const {
        return count_ == 0;
    }

    Hash hash_function() const {
        return hasher_;
    }

    ValueType& operator[] (const KeyType& key) {
        insert(key, ValueType());
        return ((find(key).ptr_)->p).second;
    }

    const ValueType& at(const KeyType& key) const{
        auto iter = find(key);
        if (iter == end())
            throw std::out_of_range("");
        return ((iter.ptr_)->p).second;
    }

    void clear() {
        count_ = 0;
        for (size_t i = 0; i < map_.size(); ++i) {
            used_[i] = 0;
                map_[i] = nullptr;
        }
    }
};