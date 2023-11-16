// Hossein Moein
// September 20, 2023
/*
Copyright (c) 2023-2028, Hossein Moein
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
* Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.
* Neither the name of Hossein Moein and/or the TimerAlarm nor the
names of its contributors may be used to endorse or promote products
derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL Hossein Moein BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <cassert>
#include <concepts>
#include <iostream>
#include <list>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

// ----------------------------------------------------------------------------

struct  MutexGuard  {

    explicit
    MutexGuard(std::mutex *l) noexcept : lock_(l) { if (lock_) lock_->lock(); }
    ~MutexGuard() noexcept { if (lock_) lock_->unlock(); }

    MutexGuard() = delete;
    MutexGuard(const MutexGuard &) = delete;
    MutexGuard &operator = (const MutexGuard &) = delete;

private:

    std::mutex  *lock_;
};

// ----------------------------------------------------------------------------

template<typename K>
concept Hashable = requires(K k)  {
    { std::hash<K>{ }(k) } -> std::convertible_to<std::size_t>;
};

// ----------------------------------------------------------------------------

// Least Recently Used cache
//
template<Hashable K, std::copy_constructible V>
class   LRUCache  {

public:

    using key_type = K;
    using value_type = V;
    using size_type = std::size_t;
    using opt_value = std::optional<value_type>;

    explicit
    LRUCache(size_type s, bool multi_thr_safe = false) : cache_size_(s)  {

        map_.reserve(cache_size_);
        if (multi_thr_safe)
            lock_ptr_ = mutex_pt (new std::mutex);
    }
    LRUCache() = delete;
    LRUCache (const LRUCache &) = delete;
    LRUCache (LRUCache &&) = default;
    ~LRUCache () = default;
    LRUCache &operator = (const LRUCache &) = delete;
    LRUCache &operator = (LRUCache &&) = default;

    // Put data into the cache
    //
    void store(const key_type &k, const value_type &v) noexcept  {

        const MutexGuard    guard (lock_ptr_.get());
        auto                iter = map_.find(k);

        if (iter != map_.end())  {  // Data already exists
            data_.splice(data_.begin(), data_, iter->second);
            // Update the value. It might be different.
            //
            iter->second->second = v;
        }
        else  {  // New data
            data_.push_front(std::make_pair(k, v));
            map_.insert(std::make_pair(k, data_.begin()));
            clean_();
        }
    }

    // Get data from the cache
    // It cannot be const because it has to rearrange the order
    //
    opt_value load(const key_type &k) noexcept  {

        opt_value           ret;
        const MutexGuard    guard (lock_ptr_.get());
        const auto          citer = map_.find(k);

        if (citer != map_.end())  {
            ret = opt_value (citer->second->second);
            data_.splice(data_.begin(), data_, citer->second);
        }
        return (ret);
    }

    void clear() noexcept  {

        const MutexGuard    guard (lock_ptr_.get());

        data_.clear();
        map_.clear();
    }
    size_type size() const noexcept  {

        const MutexGuard    guard (lock_ptr_.get());

        return (map_.size());
    }
    bool empty() const noexcept  { return (size() == 0); }
    bool contains(const key_type &k) const noexcept  {

        const MutexGuard    guard (lock_ptr_.get());

        return (map_.contains(k));
    }

    // This is for debugging purposes
    //
    template<typename C>
    requires std::invocable<C, K, V>
    void for_each(C &&callback) const  {

        const MutexGuard    guard (lock_ptr_.get());

        for (const auto &[k, v] : data_)
            callback (k, v);
    }

private:

    inline void clean_() noexcept  {

        if (map_.size() > cache_size_)  {
            map_.erase(data_.back().first);
            data_.pop_back();
        }
    }

    using list_t = std::list<std::pair<key_type, value_type>>;
    using map_t = std::unordered_map<key_type, typename list_t::iterator>;

    using mutex_pt = std::unique_ptr<std::mutex>;

    const size_type cache_size_;
    mutex_pt        lock_ptr_ { };
    list_t          data_ { };
    map_t           map_ { };
};

// ----------------------------------------------------------------------------

// Least Frequently Used cache
//
template<Hashable K, std::copy_constructible V>
class   LFUCache  {

public:

    using key_type = K;
    using value_type = V;
    using size_type = std::size_t;
    using opt_value = std::optional<value_type>;

    explicit
    LFUCache(size_type s, bool multi_thr_safe = false) : cache_size_(s)  {

        data_map_.reserve(cache_size_);
        // freq_map_.reserve(cache_size_ / 10);
        if (multi_thr_safe)
            lock_ptr_ = mutex_pt (new std::mutex);
    }
    LFUCache() = delete;
    LFUCache (const LFUCache &) = delete;
    LFUCache (LFUCache &&) = default;
    ~LFUCache () = default;
    LFUCache &operator = (const LFUCache &) = delete;
    LFUCache &operator = (LFUCache &&) = default;

    // Put data into the cache
    //
    void store(const key_type &k, const value_type &v) noexcept  {

        const MutexGuard    guard (lock_ptr_.get());
        auto                data_map_iter = data_map_.find(k);

        if (data_map_iter == data_map_.end())  {
            clean_();

            auto    &current_list = freq_map_[1];

            current_list.push_front(std::make_pair(k, v));
            data_map_.emplace(k, std::make_pair(current_list.begin(), 1));
            lfu_ = 1;
        }
        else  {
            // Update the value. It might be different.
            //
            data_map_iter->second.first->second = v;
            // Increase the freq of k and adjust lfu_
            //
            increase_freq_(data_map_iter);
        }
    }

    // Get data from the cache
    // It cannot be const because it has to rearrange the order
    //
    opt_value load(const key_type &k) noexcept  {

        opt_value           ret;
        const MutexGuard    guard (lock_ptr_.get());
        auto                data_map_iter = data_map_.find(k);

        if (data_map_iter != data_map_.end())  {
            ret = opt_value (data_map_iter->second.first->second);
            // Increase the freq of k and adjust lfu_
            //
            increase_freq_(data_map_iter);
        }

        return (ret);
    }

    void clear() noexcept  {

        const MutexGuard    guard (lock_ptr_.get());

        data_map_.clear();
        freq_map_.clear();
        lfu_ = 0;
    }
    size_type size() const noexcept  {

        const MutexGuard    guard (lock_ptr_.get());

        return (data_map_.size());
    }
    bool empty() const noexcept  { return (size() == 0); }
    bool contains(const key_type &k) const noexcept  {

        const MutexGuard    guard (lock_ptr_.get());

        return (data_map_.contains(k));
    }

    // This is for debugging purposes
    //
    template<typename C>
    requires std::invocable<C, K, V>
    void for_each(C &&callback) const  {

        const MutexGuard    guard (lock_ptr_.get());

        for (const auto &[k, v] : data_map_)
            callback (k, v.first->second);
    }

    // This is for debugging purposes
    //
    std::optional<size_type> get_freq(const key_type &k) const  {

        std::optional<size_type>    ret { };
        const MutexGuard            guard (lock_ptr_.get());
        const auto                  data_map_citer = data_map_.find(k);

        if (data_map_citer != data_map_.end())
            ret = data_map_citer->second.second;
        return (ret);
    }

private:

    inline void clean_() noexcept  {

        if (data_map_.size() == cache_size_)  {
            auto        freq_map_iter = freq_map_.find(lfu_);
            const auto  &key = freq_map_iter->second.back().first;

            data_map_.erase(key);
            freq_map_iter->second.pop_back();
            if (freq_map_iter->second.empty())
                freq_map_.erase(freq_map_iter);
        }
    }

    inline void increase_freq_(auto &data_map_iter) noexcept  {

        size_type   &current_freq = data_map_iter->second.second;
        auto        freq_map_iter = data_map_iter->second.first;
        auto        &next_freq = freq_map_[current_freq + 1]; // Next freq list
        auto        &current_list = freq_map_[current_freq];

        next_freq.splice(next_freq.begin(), current_list, freq_map_iter);
        if (current_list.empty())
            freq_map_.erase(current_freq);
        if (lfu_ == current_freq)
            lfu_ += 1;
        current_freq += 1;
    }

    using key_val_t = std::pair<key_type, value_type>;
    using list_t = std::list<key_val_t>;
    using iter_freq_t = std::pair<typename list_t::iterator, size_type>;

    using freq_map_t = std::unordered_map<size_type, list_t>;
    using data_map_t = std::unordered_map<key_type, iter_freq_t>;

    using mutex_pt = std::unique_ptr<std::mutex>;

    const size_type cache_size_;
    size_type       lfu_ { 0 };
    mutex_pt        lock_ptr_ { };
    data_map_t      data_map_ { };
    freq_map_t      freq_map_ { };
};

// ----------------------------------------------------------------------------

int main(int, char *[])  {

    using lru_cache_t = LRUCache<std::string, int>;
    using lfu_cache_t = LFUCache<std::string, int>;

    //
    //
    std::cout << "Test the LRU cache ......" << std::endl;

    lru_cache_t lru_cache (5, true);

    assert(lru_cache.size() == 0);

    lru_cache.store("One", 1);
    lru_cache.store("Two", 2);
    lru_cache.store("Three", -3);
    lru_cache.store("One", 1);
    assert(lru_cache.size() == 3);

    lru_cache.store("Four", 4);
    lru_cache.store("Five", 5);
    assert(lru_cache.load("One") == 1);
    assert(lru_cache.load("Two") == 2);
    assert(lru_cache.load("Five") == 5);

    lru_cache.store("Six", 6);
    assert(! lru_cache.load("Three"));
    assert(lru_cache.load("One") == 1);
    assert(lru_cache.load("Two") == 2);
    assert(lru_cache.load("Five") == 5);
    assert(lru_cache.load("Six") == 6);
    assert(lru_cache.size() == 5);

    lru_cache.for_each([](const std::string &k, const int &v)  {
        std::cout << k << " : " << v << std::endl;
    });

    //
    //
    std::cout << "Test the LFU cache ......" << std::endl;

    lfu_cache_t lfu_cache (4, false);

    lfu_cache.store("One", 1);
    lfu_cache.store("Two", 2);
    lfu_cache.store("Three", -3);
    lfu_cache.store("One", 1);
    assert(lfu_cache.size() == 3);
    assert(lfu_cache.get_freq("One") == 2);
    assert(lfu_cache.get_freq("Three") == 1);
    assert(! lfu_cache.get_freq("Six"));

    assert(lfu_cache.load("One") == 1);
    assert(lfu_cache.load("One") == 1);
    assert(lfu_cache.load("Two") == 2);
    assert(lfu_cache.get_freq("One") == 4);
    assert(lfu_cache.get_freq("Two") == 2);

    lfu_cache.store("One", -1);
    assert(lfu_cache.load("One") == -1);
    assert(lfu_cache.get_freq("One") == 6);

    lfu_cache.store("Four", 4);
    lfu_cache.store("Five", 5);
    lfu_cache.store("Six", 6);
    assert(lfu_cache.load("Six") == 6);
    assert(lfu_cache.get_freq("Six") == 2);
    assert(lfu_cache.get_freq("Five") == 1);
    assert(lfu_cache.load("Five") == 5);
    assert(lfu_cache.get_freq("One") == 6);
    assert(lfu_cache.get_freq("Two") == 2);

    lfu_cache.for_each([](const std::string &k, const int &v)  {
        std::cout << k << " : " << v << std::endl;
    });

    return (EXIT_SUCCESS);
}

// ----------------------------------------------------------------------------

// Local Variables:
// mode:C++
// tab-width:4
// c-basic-offset:4
// End:
