// gpu_pipeline_fallback.cpp
// CPU-only pipeline mimicking CUDA triple-buffering using std::thread and a bounded queue.
// Useful on systems without CUDA to illustrate overlapped stages.

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

struct Chunk { size_t id; size_t n; std::vector<float> in, out; };

class BoundedQueue {
 public:
  explicit BoundedQueue(size_t cap): cap_(cap) {}
  void push(Chunk c){ std::unique_lock<std::mutex> lk(m_); cv_full_.wait(lk, [&]{return q_.size()<cap_ || stop_;}); if(stop_) return; q_.push(std::move(c)); lk.unlock(); cv_empty_.notify_one(); }
  bool pop(Chunk& c){ std::unique_lock<std::mutex> lk(m_); cv_empty_.wait(lk, [&]{return !q_.empty() || stop_;}); if(q_.empty()) return false; c = std::move(q_.front()); q_.pop(); lk.unlock(); cv_full_.notify_one(); return true; }
  void stop(){ std::lock_guard<std::mutex> lk(m_); stop_=true; cv_empty_.notify_all(); cv_full_.notify_all(); }
 private:
  size_t cap_; std::queue<Chunk> q_; std::mutex m_; std::condition_variable cv_empty_, cv_full_; bool stop_{false};
};

static void producer(BoundedQueue& q, size_t total, size_t chunk){
  size_t id=0; size_t produced=0; while(produced<total){ size_t n=std::min(chunk, total-produced); Chunk c; c.id=id++; c.n=n; c.in.resize(n); c.out.resize(n); for(size_t i=0;i<n;++i) c.in[i]=(float)(produced+i)*0.001f; q.push(std::move(c)); produced+=n; }
  q.stop();
}

static void worker(BoundedQueue& in, BoundedQueue& out){
  const float a=2.0f,b=1.0f; Chunk c; while(in.pop(c)){ for(size_t i=0;i<c.n;++i) c.out[i]=a*c.in[i]+b; out.push(std::move(c)); } out.stop();
}

static void consumer(BoundedQueue& q, size_t total, size_t chunk){
  size_t consumed=0; Chunk c; while(q.pop(c)){ // mimic D2H sink
    if(c.n>0){ float x=(float)(c.id*chunk)*0.001f; float exp=2.0f*x+1.0f; if(std::abs(c.out[0]-exp)>1e-4f){ std::fprintf(stderr,"Validation failed at chunk %zu\n", c.id); std::exit(1);} }
    consumed+=c.n; }
  std::fprintf(stderr,"CPU pipeline consumed %zu elements\n", consumed);
}

int main(int argc, char** argv){
  size_t total = (argc>1)? std::strtoull(argv[1],nullptr,10) : (size_t)100000000;
  size_t chunk = (argc>2)? std::strtoull(argv[2],nullptr,10) : (size_t)1<<20;
  size_t depth = (argc>3)? std::strtoull(argv[3],nullptr,10) : 3;

  BoundedQueue q12(depth), q23(depth);
  std::thread t1(producer, std::ref(q12), total, chunk);
  std::thread t2(worker, std::ref(q12), std::ref(q23));
  std::thread t3(consumer, std::ref(q23), total, chunk);
  t1.join(); t2.join(); t3.join();
  return 0;
}
