#ifndef LRU_CACHE_HPP
#define LRU_CACHE_HPP
#include <iterator>
#include <list>
#include <mutex>
#include <optional>
#include <unordered_map>
template <class T, size_t Capacity = 10> class LRUCache {
  static_assert(Capacity > 0, "LRU capacity must be positive");

public:
  bool put(const T &item) {
    if constexpr (Capacity == 0) {
      return false;
    }
    auto it = _map.find(item);
    if (it != std::end(_map)) {
      _cache.splice(std::begin(this->_cache), this->_cache, it->second);
      return true;
    }
    if (this->_cache.size() >= Capacity) {
      auto last = this->_cache.back();
      this->_map.erase(last);
      this->_cache.pop_back();
    }
    _cache.push_front(item);
    _map[item] = _cache.begin();
    return true;
  }
  std::optional<T> get(const T &item) {
    if (_map.find(item) == _map.end()) {
      return std::nullopt;
    }
    _cache.splice(std::begin(this->_cache), this->_cache, _map[item]);
    return *(_map[item]);
  }
  bool contains(const T &item)  { return _map.find(item) != _map.end(); }
  bool empty() { return this->_cache.empty(); }
  bool erase(const T &item) {
    auto it = _map.find(item);
    if (it != _map.end()) {
      this->_cache.erase(it->second);
      this->_map.erase(item);
      return true;
    }
    return false;
  }
  size_t size() { return this->_cache.size(); }
  constexpr size_t capacity() const noexcept { return Capacity; }

private:
  std::list<T> _cache;
  std::unordered_map<T, typename std::list<T>::iterator> _map;
};
namespace parallel {
template <class T, size_t Capacity> class LRUCacheParallel {
  static_assert(Capacity > 0, "LRU capacity parallel must be positive");

public:
  bool put(const T &item) {
    std::lock_guard<std::mutex> lock(_mutex);
    if constexpr (Capacity == 0) {
      return false;
    }
    auto it = _map.find(item);
    if (it != std::end(_map)) {
      _cache.splice(std::begin(this->_cache), this->_cache, it->second);
      return true;
    }
    if (this->_cache.size() >= Capacity) {
      auto last = this->_cache.back();
      this->_map.erase(last);
      this->_cache.pop_back();
    }
    _cache.push_front(item);
    _map[item] = _cache.begin();
    return true;
  }
  std::optional<T> get(const T &item) {
    std::lock_guard<std::mutex> lock(_mutex);
    if (_map.find(item) == _map.end()) {
      return std::nullopt;
    }
    _cache.splice(std::begin(this->_cache), this->_cache, _map[item]);
    return *(_map[item]);
  }
  bool contains(const T &item)  {
    std::lock_guard<std::mutex> lock(_mutex);
    return _map.find(item) != _map.end();
  }
  bool empty() {
    std::lock_guard<std::mutex> lock(_mutex);
    return this->_cache.empty();
  }
  bool erase(const T &item) {
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _map.find(item);
    if (it != _map.end()) {
      this->_cache.erase(it->second);
      this->_map.erase(item);
      return true;
    }
    return false;
  }
  size_t size() {
    std::lock_guard<std::mutex> lock(_mutex);
    return this->_cache.size();
  }
  constexpr size_t capacity() const noexcept { return Capacity; }

private:
  std::mutex _mutex;
  std::list<T> _cache;
  std::unordered_map<T, typename std::list<T>::iterator> _map;
};
} // namespace parallel
#endif