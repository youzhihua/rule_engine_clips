#pragma once
#include <functional>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <list>
#include <limits>

/**
 * FactoryT must have functions:
 *     1. ResourceT *Create()
 *     2. void Destroy(ResourceT *)
 */
template <typename ResourceT, typename FactoryT>
class ResourcePool {
   public:
    // Acquires owner ship of factory.
    ResourcePool(unsigned int capacity, FactoryT *factory);

    ~ResourcePool();

    void set_max_capacity(unsigned int max_capacity) {
        if (max_capacity < _capacity) {
            throw std::invalid_argument("max capacity is lesser than capacity");
        }
        _max_capacity = max_capacity;
    }

    inline void set_need_clear(bool need_clear) {
        _need_clear = need_clear;
    }

    inline unsigned int get_size() {
        return _size;
    }

    ResourcePool(const ResourcePool &) = delete;
    ResourcePool &operator=(const ResourcePool &) = delete;

    // thread safe
    template <typename ResultT>
    ResultT RunWithResource(std::function<ResultT(ResourceT *)> func);

   private:
    ResourceT *GetResource();
    void PutResource(ResourceT *);
    void PutErrorResource(ResourceT *);
    void ClearPool();

    unsigned int _capacity;
    unsigned int _max_capacity;
    bool _need_clear;
    std::unique_ptr<FactoryT> _factory;

    std::mutex _pool_lock;  // protects the following two
    unsigned int _size;
    std::list<ResourceT *> _idle;
};

template <typename ResourceT, typename FactoryT>
ResourcePool<ResourceT, FactoryT>::ResourcePool(unsigned int capacity,
                                                FactoryT *factory)
    : _capacity(capacity),
      _max_capacity(std::numeric_limits<unsigned int>::max()),
      _factory(factory),
      _size(0),
      _need_clear(true) {
    std::lock_guard<std::mutex> guard(_pool_lock);
    for (unsigned int i = 0; i < _capacity; ++i) {
        try {
            _idle.push_front(_factory->Create());
            ++_size;
        } catch (std::exception &e) {
            throw;
        }
    }
}

template <typename ResourceT, typename FactoryT>
ResourcePool<ResourceT, FactoryT>::~ResourcePool() {
    ClearPool();
}

template <typename ResourceT, typename FactoryT>
template <typename ResultT>
ResultT ResourcePool<ResourceT, FactoryT>::RunWithResource(
    std::function<ResultT(ResourceT *)> func) {
    ResourceT *resource = nullptr;
    try {
        resource = GetResource();
        ResultT result = func(resource);
        PutResource(resource);
        return result;
    } catch (std::exception &e) {
        if (resource) {
            PutErrorResource(resource);
            if (_need_clear) {
                ClearPool();
            }
        }
        throw;
    }
}

template <typename ResourceT, typename FactoryT>
ResourceT *ResourcePool<ResourceT, FactoryT>::GetResource() {
    std::lock_guard<std::mutex> guard(_pool_lock);
    ResourceT *resource;
    if (!_idle.empty()) {
        resource = _idle.front();
        _idle.pop_front();
    } else {
        if (_size < _max_capacity) {
            resource = _factory->Create();
            ++_size;
        } else {
            throw std::runtime_error("resource pool reach max capacity");
        }
    }
    return resource;
}

template <typename ResourceT, typename FactoryT>
void ResourcePool<ResourceT, FactoryT>::PutResource(ResourceT *resource) {
    if (resource == nullptr) return;

    std::lock_guard<std::mutex> guard(_pool_lock);
    if (_size >= _capacity && _need_clear) {
        try {
            _factory->Destroy(resource);
        } catch (std::exception &e) {
        }
        --_size;
    } else {
        _idle.push_front(resource);
    }
}

template <typename ResourceT, typename FactoryT>
void ResourcePool<ResourceT, FactoryT>::PutErrorResource(ResourceT *resource) {
    if (resource == nullptr) return;

    std::lock_guard<std::mutex> guard(_pool_lock);
    try {
        _factory->Destroy(resource);
    } catch (std::exception &e) {
    }
    --_size;
}

template <typename ResourceT, typename FactoryT>
void ResourcePool<ResourceT, FactoryT>::ClearPool() {
    std::lock_guard<std::mutex> guard(_pool_lock);
    while (!_idle.empty()) {
        auto resource = _idle.front();
        _idle.pop_front();
        try {
            _factory->Destroy(resource);
        } catch (std::exception &e) {
        }
        --_size;
    }
}