#pragma once

#include <chrono>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <unordered_map>
#include <type_traits>
#include <utility>
#include <functional>
#include <map>

class TimerQueue {
public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;
    using Duration = Clock::duration;
    using Handler = std::function<void(bool)>;

    TimerQueue() {
        worker_ = std::thread([this] { run(); });
    }

    ~TimerQueue() {
        stop();
        cv_.notify_all();
        if(worker_.joinable())
            worker_.join();
    }

    template<typename Rep, typename Period, typename Callable>
    uint64_t add(std::chrono::duration<Rep, Period> delay, Callable&& handler) {
        static_assert(std::is_invocable<Callable, bool>::value,
                     "Handler must be callable with bool argument");
        
        auto id = ++id_counter_;
        TimePoint expiry = Clock::now() + delay;
        
        {
            std::lock_guard<std::mutex> lock(mutex_);
            // Correction clé ici : utilisation de l'itérateur retourné par emplace
            auto it = timers_.emplace(expiry, TimerTask{id, std::forward<Callable>(handler)});
            id_map_[id] = it;
        }
        
        cv_.notify_one();
        return id;
    }

    bool cancel(uint64_t id) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto map_it = id_map_.find(id);
        if(map_it != id_map_.end()) {
            auto timer_it = map_it->second;
            
            // Déplace le handler avant suppression
            cancel_queue_.emplace_back(std::move(timer_it->second.handler));
            
            // Supprime de la multimap et de la hashmap
            timers_.erase(timer_it);
            id_map_.erase(map_it);
            
            return true;
        }
        return false;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        for(auto& task : timers_)
            cancel_queue_.emplace_back(std::move(task.second.handler));
        timers_.clear();
        id_map_.clear();
    }

private:
    struct TimerTask {
        uint64_t id;
        Handler handler;
    };

    // Comparateur personnalisé pour la multimap
    struct TimerCompare {
        /*bool operator()(const std::pair<TimePoint, TimerTask>& a,
                        const std::pair<TimePoint, TimerTask>& b) const {
            return a.first > b.first;
        }*/
        bool operator()(const TimePoint& a, const TimePoint& b) const {
            return a < b;
        }
    };

    using TimerMultimap = std::multimap<TimePoint, TimerTask, TimerCompare>;

    std::mutex mutex_;
    std::condition_variable_any cv_;
    std::atomic<uint64_t> id_counter_{0};
    std::atomic<bool> running_{true};
    
    TimerMultimap timers_;
    std::unordered_map<uint64_t, TimerMultimap::iterator> id_map_;
    std::vector<Handler> cancel_queue_;
    std::thread worker_;

    void run() {
        while(running_ || !timers_.empty()) {
            std::unique_lock<std::mutex> lock(mutex_);
            
            // Traite d'abord les annulations
            process_cancellations(lock);
            
            if(timers_.empty()) {
                cv_.wait(lock, [this] { return !running_ || !timers_.empty(); });
                continue;
            }

            auto next_time = timers_.begin()->first;
            if(cv_.wait_until(lock, next_time) == std::cv_status::no_timeout) {
                // Réveil prématuré dû à une nouvelle tâche ou annulation
                continue;
            }

            // Exécute toutes les tâches expirées
            auto now = Clock::now();
            while(!timers_.empty() && timers_.begin()->first <= now) {
                auto task = std::move(timers_.begin()->second);
                timers_.erase(timers_.begin());
                id_map_.erase(task.id);
                
                lock.unlock();
                try {
                    task.handler(false); // Exécution normale
                } catch(...) {}
                lock.lock();
            }
        }
    }

    void process_cancellations(std::unique_lock<std::mutex>& lock) {
        for(auto&& handler : cancel_queue_) {
            lock.unlock();
            try {
                handler(true); // Annulation
            } catch(...) {}
            lock.lock();
        }
        cancel_queue_.clear();
    }

    void stop() {
        running_ = false;
        clear();
    }

    // Interdire la copie/déplacement
    TimerQueue(const TimerQueue&) = delete;
    TimerQueue& operator=(const TimerQueue&) = delete;
};
