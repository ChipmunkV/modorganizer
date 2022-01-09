#ifndef ENV_ENVFS_H
#define ENV_ENVFS_H

#include "thread_utils.h"
#include <condition_variable>
#include <thread>
#ifndef _WIN32
#include <QString>
#include "filetime.h"
#endif
#include "pathstr.h"

namespace env
{

struct File
{
  PathStr name;
  PathStr lcname;
  FILETIME lastModified;
  uint64_t size;

  File(PathStrView name, FILETIME ft, uint64_t size);
};

struct Directory
{
  PathStr name;
  PathStr lcname;

  std::vector<Directory> dirs;
  std::vector<File> files;

  Directory();
  Directory(PathStrView name);
};


template <class T>
class ThreadPool
{
public:
  ThreadPool(std::size_t max=1)
  {
    setMax(max);
  }

  ~ThreadPool()
  {
    stopAndJoin();
  }

  void setMax(std::size_t n)
  {
    m_threads.resize(n);
  }

  void stopAndJoin()
  {
    for (auto& ti : m_threads) {
      ti.stop = true;
      ti.wakeup();
    }

    for (auto& ti : m_threads) {
      if (ti.thread.joinable()) {
        ti.thread.join();
      }
    }
  }

  void waitForAll()
  {
    for (;;) {
      bool done = true;

      for (auto& ti : m_threads) {
        if (ti.busy) {
          done = false;
          break;
        }
      }

      if (done) {
        break;
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }

  T& request()
  {
    if (m_threads.empty()) {
      std::terminate();
    }

    for (;;) {
      for (auto& ti : m_threads) {
        bool expected = false;

        if (ti.busy.compare_exchange_strong(expected, true)) {
          ti.wakeup();
          return ti.o;
        }
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }

  template <class F>
  void forEach(F&& f)
  {
    for (auto& ti : m_threads) {
      f(ti.o);
    }
  }

private:
  struct ThreadInfo
  {
    std::thread thread;
    std::atomic<bool> busy;
    T o;

    std::condition_variable cv;
    std::mutex mutex;
    bool ready;

    std::atomic<bool> stop;

    ThreadInfo()
      : busy(true), ready(false), stop(false)
    {
      thread = MOShared::startSafeThread([&]{ run(); });
    }

    ~ThreadInfo()
    {
      if (thread.joinable()) {
        stop = true;
        wakeup();
        thread.join();
      }
    }

    void wakeup()
    {
      {
        std::scoped_lock lock(mutex);
        ready = true;
      }

      cv.notify_one();
    }

    void run()
    {
      busy = false;

      while (!stop) {
        std::unique_lock lock(mutex);
        cv.wait(lock, [&]{ return ready; });

        if (stop) {
          break;
        }

        o.run();

        ready = false;
        busy = false;
      }
    }
  };

  std::list<ThreadInfo> m_threads;
};


#ifdef _WIN32
using DirStartF = void (void*, PathStrView);
using DirEndF = void (void*, PathStrView);
using FileF = void (void*, PathStrView, FILETIME, uint64_t);
#else
using DirStartF = void (void*, PathStrView);
using DirEndF = void (void*, PathStrView);
using FileF = void (void*, PathStrView, FILETIME, uint64_t);
#endif

void setHandleCloserThreadCount(std::size_t n);


class DirectoryWalker
{
public:
#ifdef _WIN32
  void forEachEntry(
    const PathStr& path, void* cx,
    DirStartF* dirStartF, DirEndF* dirEndF, FileF* fileF);
#else
  void forEachEntry(
    const std::string& path, void* cx,
    DirStartF* dirStartF, DirEndF* dirEndF, FileF* fileF);
#endif

private:
  std::vector<std::unique_ptr<unsigned char[]>> m_buffers;
};


#ifdef _WIN32
void forEachEntry(
  const PathStr& path, void* cx,
  DirStartF* dirStartF, DirEndF* dirEndF, FileF* fileF);
#else
void forEachEntry(
  const std::string& path, void* cx,
  DirStartF* dirStartF, DirEndF* dirEndF, FileF* fileF);
#endif

Directory getFilesAndDirs(const PathStr& path);
Directory getFilesAndDirsWithFind(const PathStr& path);

} // namespace

#endif // ENV_ENVFS_H
