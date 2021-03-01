#pragma once

#include <list>
#include <utility>

namespace engine {
struct ObjectId {
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

    virtual std::pair<ObjectId, Object&> add(Object object) = 0;
    virtual void remove(const ObjectId& id) = 0;

    virtual Object& get(const ObjectId& id) = 0;
    virtual const Object& get(const ObjectId& id) const = 0;

    virtual ObjectId first() const = 0;
    virtual ObjectId last() const = 0;
    virtual ObjectId next(const ObjectId& id) const = 0;
};

//
// #############################################################################
//

template <typename Object_>
class ListObjectPool : public AbstractObjectPool<Object_> {
private:
    using iterator = typename std::list<Object_>::iterator;

    ObjectId id_from_it(const iterator& it) const { return ObjectId{*reinterpret_cast<size_t*>(&*it)}; }

    iterator it_from_id(const ObjectId& id) const { return reinterpret_cast<const iterator&>(id.key); }

public:
    using Object = Object_;

    ~ListObjectPool() override = default;

    std::pair<ObjectId, Object&> add(Object object) override {
        Object& emplaced = pool_.emplace_back(std::move(object));
        return {last(), emplaced};
    }

    void remove(const ObjectId& id) override { pool_.erase(it_from_id(id)); }
    Object& get(const ObjectId& id) override { return *it_from_id(id); }
    const Object& get(const ObjectId& id) const override { return *it_from_id(id); }
    ObjectId first() const override { return it_to_id(pool_.begin()); }
    ObjectId last() const override { return it_to_id(pool_.end()); }
    ObjectId next(const ObjectId& id) const override { return it_from_id(it_from_id(id)++); }

private:
    std::list<Object_> pool_;
};
}  // namespace engine
