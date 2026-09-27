#ifndef SINGLETON_HPP
#define SINGLETON_HPP
template <class T> class SingleTon {
public:
  static T *Instance() {
    static T ins;
    return &ins;
  }
  SingleTon(const SingleTon &) = delete;
  SingleTon(SingleTon &&) = delete;
  SingleTon &operator=(const SingleTon &) = delete;
  SingleTon &operator=(SingleTon &) = delete;

protected:
  SingleTon() = default;
  ~SingleTon() = default;
};
#endif
