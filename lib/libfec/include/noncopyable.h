//
// Created by lwj on 2020/2/2.
//

#ifndef LIBFEC_NONCOPYABLE_H
#define LIBFEC_NONCOPYABLE_H

class noncopyable
{
 public:
  noncopyable(const noncopyable&) = delete;
  void operator=(const noncopyable&) = delete;

 protected:
  noncopyable() = default;
  ~noncopyable() = default;
};

#endif //LIBFEC_NONCOPYABLE_H
