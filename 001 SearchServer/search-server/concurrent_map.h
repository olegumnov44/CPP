#pragma once

#include "document.h"
#include "search_server.h"

using namespace std::string_literals;


template <typename Key, typename Value>
class ConcurrentMap {
    friend class SearchServer;
private:
    struct Bucket {
        std::mutex mutex;
        std::map<Key, Value> map;
    };
public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys");

    struct Access {
        std::scoped_lock<std::mutex> guard;
        Value& ref_to_value;

        Access(const Key& key, Bucket& bucket)
            : guard(bucket.mutex)
            , ref_to_value(bucket.map[key])
        {}
    };
    explicit ConcurrentMap(size_t bucket_count)
        : bucket_(bucket_count)
    {}
    Access operator[](const Key& key) {
       auto* bucket = &bucket_[static_cast<uint64_t>(key) % bucket_.size()];
        return {key, *bucket};
    }
    std::map<Key, Value> BuildOrdinaryMap() {
        std::map<Key, Value> result;
        for (auto& [mutex, map] : bucket_) {
            std::lock_guard guard(mutex);
            result.insert(map.begin(), map.end());
        }
        return result;
    }
    void Erase(Key key) {
        auto* it = &bucket_.operator[](key);
        std::lock_guard guard((*it).mutex);
        (*it).map.erase(key);
    }
private:
    std::vector<Bucket> bucket_;
};


// not test
template <typename Key, typename Value>
class ConcurrentSet {
private:
    struct Bucket {
        std::mutex mutex;
        std::set<Key> set;
    };
public:
    //static_assert(std::is_integral_v<Key>, "ConcurrentSet supports only integer keys");
    static_assert(std::is_same<typename std::remove_cv<Key>::type, Key>::value,
    "std::set must have a non-const, non-volatile value_type");
    struct Access {
        std::lock_guard<std::mutex> guard;
        Value& ref_to_value;

        Access(const Key& key, Bucket& bucket)
            : guard(bucket.mutex)
            , ref_to_value(bucket.set[key])
        {}
    };

    explicit ConcurrentSet(size_t bucket_count)
        : bucket_(bucket_count)
    {}

    Access operator[](const Key& key) {
        auto& bucket = bucket_[key];
        return {key, bucket};
    }

    std::set<Key> BuildOrdinarySet() {
        std::set<Key> result;
        for (auto& [mutex, set] : bucket_) {
            std::lock_guard guard(mutex);
            result.insert(set.begin(), set.end());
        }
        return result;       
    }

    std::set<unsigned long>::iterator begin() {
        return bucket_.begin();
    }

    std::set<unsigned long>::iterator end() {
        return bucket_.end();
    }

    const std::set<unsigned long>::iterator cbegin() {
        return bucket_.begin();
    }

    const std::set<unsigned long>::iterator cend() {
        return bucket_.end();
    }

    size_t size(const Key& key) {
        auto& bucket = bucket_[key];
        return bucket.set.size();
    }

    void Erase(const Key& key) {
        auto& bucket = bucket_[key];
        std::lock_guard guard(bucket.mutex);
        bucket.set.erase(key);
    }

private:
    std::vector<Bucket> bucket_;

};
