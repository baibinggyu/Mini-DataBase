#ifndef LFU_CACHE_HPP
#define LFU_CACHE_HPP
#include <cstddef>
#include <list>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <concepts>
template <class Key>
concept CacheKey = std::copy_constructible<Key> &&
                   std::equality_comparable<Key> && requires(const Key &key) {
                     // std::hash<Key> 必须可以调用，并返回 size_t
                     {
                       std::hash<Key>{}(key)
                     } -> std::convertible_to<std::size_t>;
                   };
template <class T>
concept CacheValue = std::copy_constructible<T> &&
                     std::is_copy_assignable_v<T> && std::destructible<T>;
template <CacheKey Key, CacheValue T, size_t Capacity = 10> class LFUCache {
  static_assert(Capacity > 0, "LFU capacity must be positive");

public:
  bool put(Key key, T item) {
    // 存在
    if (this->_cacheMap.contains(key)) {
      Node &node = this->_cacheMap.find(key)->second;
      node.value = item;
      touch(key);
      return true;
    }
    // 不存在
    // 容量满了
    if (this->_cacheMap.size() >= Capacity) {
      Key keyToDelete = this->_freqMap[this->_minFreq].back();
      this->_freqMap[this->_minFreq].pop_back();
      if (this->_freqMap[this->_minFreq].empty()) {
        this->_freqMap.erase(this->_minFreq);
      }
      this->_cacheMap.erase(keyToDelete);
    }
    this->_freqMap[1].push_front(key);
    this->_cacheMap[key] = Node{item, 1, this->_freqMap[1].begin()};
    this->_minFreq = 1;
    return true;
  }
  bool empty() const noexcept { return this->_cacheMap.empty(); }
  std::optional<T> get(Key key) {
    if (this->_cacheMap.contains(key) == false) {
      return std::nullopt;
    }
    Node &res = this->_cacheMap.find(key)->second;
    touch(key);
    return res.value;
  }
  constexpr size_t capacity() const noexcept { return Capacity; }

private:
  // 这里不管是put还是get，都要更新频率，所以可以把更新频率的逻辑提取出来，放在一个单独的函数中
  // 此外就是key一定要存在于_cacheMap中，所以不需要检查key是否存在
  void touch(Key key) {
    Node &node = this->_cacheMap.find(key)->second;
    size_t freq = node.frequency++;
    typename std::list<Key>::iterator it = node.it;
    this->_freqMap[freq].erase(it);
    if (this->_freqMap[freq].empty()) {
      this->_freqMap.erase(freq);
      if(this->_minFreq == freq) {
        this->_minFreq++;
      }
    }
    this->_freqMap[node.frequency].push_front(key);
    node.it = this->_freqMap[node.frequency].begin();
  }

private:
  struct Node {
    // T如果支持默认构造函数，Node才会支持默认构造函数
    T value;
    size_t frequency;
    typename std::list<Key>::iterator it;
  };
  std::unordered_map<Key, Node> _cacheMap;
  std::unordered_map<std::size_t, typename std::list<Key>> _freqMap;
  std::size_t _minFreq{0};
};
namespace parallel {

template <CacheKey Key, CacheValue T, size_t Capacity = 10> class LFUCacheParallel {
  static_assert(Capacity > 0, "LFU capacity must be positive");

public:
  bool put(Key key, T item) {
      std::lock_guard<std::mutex> lock(_mutex);
    // 存在
    if (this->_cacheMap.contains(key)) {
      Node &node = this->_cacheMap.find(key)->second;
      node.value = item;
      touch(key);
      return true;
    }
    // 不存在
    // 容量满了
    if (this->_cacheMap.size() >= Capacity) {
      Key keyToDelete = this->_freqMap[this->_minFreq].back();
      this->_freqMap[this->_minFreq].pop_back();
      if (this->_freqMap[this->_minFreq].empty()) {
        this->_freqMap.erase(this->_minFreq);
      }
      this->_cacheMap.erase(keyToDelete);
    }
    this->_freqMap[1].push_front(key);
    this->_cacheMap[key] = Node{item, 1, this->_freqMap[1].begin()};
    this->_minFreq = 1;
    return true;
  }
  bool empty() const noexcept { 
    std::lock_guard<std::mutex> lock(_mutex); 
    return this->_cacheMap.empty(); }
  std::optional<T> get(Key key) {
      std::lock_guard<std::mutex> lock(_mutex);
    if (this->_cacheMap.contains(key) == false) {
      return std::nullopt;
    }
    Node &res = this->_cacheMap.find(key)->second;
    touch(key);
    return res.value;
  }
  constexpr size_t capacity() const noexcept { return Capacity; }

private:
  // touch不加锁，避免死锁
  void touch(Key key) {
    Node &node = this->_cacheMap.find(key)->second;
    size_t freq = node.frequency++;
    typename std::list<Key>::iterator it = node.it;
    this->_freqMap[freq].erase(it);
    if (this->_freqMap[freq].empty()) {
      this->_freqMap.erase(freq);
      if(this->_minFreq == freq) {
        this->_minFreq++;
      }
    }
    this->_freqMap[node.frequency].push_front(key);
    node.it = this->_freqMap[node.frequency].begin();
  }

private:
  struct Node {
    // T如果支持默认构造函数，Node才会支持默认构造函数
    T value;
    size_t frequency;
    typename std::list<Key>::iterator it;
  };
  std::unordered_map<Key, Node> _cacheMap;
  std::unordered_map<std::size_t, typename std::list<Key>> _freqMap;
  std::size_t _minFreq{0};
  mutable std::mutex _mutex;
};
} // namespace parallel
#endif