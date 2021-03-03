#pragma once

#include <list>
#include <utility>
#include <vector>

namespace engine {
struct ObjectId {
    ObjectId() : key(-1) {}
    explicit ObjectId(size_t key_) : key(key_) {}
    bool operator!=(const ObjectId& rhs) const { return key != rhs.key; }
    size_t key;
};

//
// #############################################################################
//

template <typename Object_>
class AbstractObjectPool {
public:
    using Object = Object_;

    virtual ~AbstractObjectPool() = default;

    ///
    /// @brief Add or remove an object. No active Object references should be invalidated (unless it's been removed)
    ///
    virtual std::pair<ObjectId, Object&> add(Object object) = 0;
    virtual void remove(const ObjectId& id) = 0;

    ///
    /// @brief Get the object referenced by the ID. If the ObjectId isn't valid the reference won't be either.
    ///
    virtual Object& get(const ObjectId& id) = 0;
    virtual const Object& get(const ObjectId& id) const = 0;

    virtual bool empty() const = 0;

    // TODO: something better than the vector
    virtual std::vector<const Object*> iterate() const = 0;
    virtual std::vector<Object*> iterate() = 0;
};

//
// #############################################################################
//

template <typename Object_>
class ListObjectPool : public AbstractObjectPool<Object_> {
private:
    using iterator = typename std::list<Object_>::iterator;
    using const_iterator = typename std::list<Object_>::const_iterator;
    static_assert(sizeof(iterator) <= sizeof(size_t));

    ObjectId id_from_it(const const_iterator& it) const { return ObjectId{*reinterpret_cast<const size_t*>(&it)}; }
    iterator it_from_id(const ObjectId& id) const { return reinterpret_cast<const iterator&>(id.key); }

public:
    using Object = Object_;

    ~ListObjectPool() override = default;

    std::pair<ObjectId, Object&> add(Object object) override {
        Object& emplaced = pool_.emplace_back(std::move(object));
        return {id_from_it(std::prev(pool_.end())), emplaced};
    }

    void remove(const ObjectId& id) override { pool_.erase(it_from_id(id)); }
    Object& get(const ObjectId& id) override { return *it_from_id(id); }
    const Object& get(const ObjectId& id) const override { return *it_from_id(id); }

    bool empty() const override { return pool_.empty(); }

    std::vector<const Object*> iterate() const override {
        std::vector<const Object*> objects;
        objects.reserve(pool_.size());
        for (const auto& object : pool_) objects.emplace_back(&object);
        return objects;
    }

    std::vector<Object*> iterate() override {
        std::vector<Object*> objects;
        objects.reserve(pool_.size());
        for (auto& object : pool_) objects.emplace_back(&object);
        return objects;
    }

private:
    std::list<Object_> pool_;
};
}  // namespace engine
