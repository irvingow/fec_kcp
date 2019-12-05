//
// Created by lwj on 2019/10/12.
//

#include "timeout_map.h"

TimeOutMap::TimeOutMap(const int32_t &timeout_limit_ms)
    : timeout_limit_ms_(timeout_limit_ms) {}

int32_t TimeOutMap::Update(const int32_t &key, const int64_t &cur_time_ms) {
    std::lock_guard<std::mutex> lck(listAndtimeout_map_mutex_);
    auto iter = timeout_map_.find(key);
    if (iter == timeout_map_.end())
        return -1;
    iter->second.first = cur_time_ms;
    elements_.erase(iter->second.second);
    elements_.push_front(key);
    iter->second.second = elements_.begin();
    return 0;
}

int32_t TimeOutMap::Add(const int32_t &key, const int64_t &cur_time_ms) {
    int32_t ret = Update(key, cur_time_ms);
    if (ret < 0) {
        std::lock_guard<std::mutex> lck(listAndtimeout_map_mutex_);
        elements_.push_front(key);
        timeout_map_[key] = {cur_time_ms, elements_.begin()};
        return 0;
    }
    return ret;
}

int32_t TimeOutMap::Remove(const int32_t &key) {
    std::lock_guard<std::mutex> lck(listAndtimeout_map_mutex_);
    auto iter = timeout_map_.find(key);
    if (iter == timeout_map_.end()) {
        return -1;
    }
    elements_.erase(iter->second.second);
    timeout_map_.erase(key);
    return 0;
}

std::vector<int32_t>
TimeOutMap::GetTimeOutElements(const int64_t &cur_time_ms) {
    std::vector<int32_t> timeout_elements;
    {
        ///注意这个加锁的作用域,否则可能导致死锁
        std::lock_guard<std::mutex> lck(listAndtimeout_map_mutex_);
        ///从后往前遍历,因为elements_中的元素是按更新时间降序排列的
        for (auto iter = elements_.rbegin(); iter != elements_.rend(); ++iter) {
            int64_t last_update_time_ms = timeout_map_[*iter].first;
            if ((cur_time_ms - last_update_time_ms) > timeout_limit_ms_)
                timeout_elements.push_back(*iter);
            else
                break;
        }
    }
    return timeout_elements;
}

