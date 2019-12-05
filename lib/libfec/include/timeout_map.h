//
// Created by lwj on 2019/10/12.
//

#ifndef LIBFEC_TIMEOUT_MAP_H
#define LIBFEC_TIMEOUT_MAP_H

#include <cstdint>
#include <list>
#include <mutex>
#include <unordered_map>
#include <vector>

class TimeOutMap {
 public:
  typedef std::list<int32_t>::iterator Int32ListIter;
  typedef std::unordered_map<int32_t , std::pair<int64_t , std::list<int32_t >::iterator>>::iterator UmpIter;
  explicit TimeOutMap(const int32_t &timeout_limit_ms);
  int32_t Update(const int32_t &key, const int64_t &cur_time_ms);
  int32_t Add(const int32_t &key, const int64_t &cur_time_ms);
  int32_t Remove(const int32_t &key);
  std::vector<int32_t> GetTimeOutElements(const int64_t &cur_time_ms);

 private:
  ///保护下面两个数据结构
  std::mutex listAndtimeout_map_mutex_;
  std::list<int32_t> elements_;
  std::unordered_map<int32_t, std::pair<int64_t , Int32ListIter>> timeout_map_;
  int32_t timeout_limit_ms_;
};

#endif //LIBFEC_TIMEOUT_MAP_H
