#include <iostream>
#include <future>
#include <memory>
#include "thread_safe_prior_queue.h"
#include <unistd.h>

struct PipelineMessage {
  std::string info;
  bool last_frame;
};
typedef std::pair<int, PipelineMessage> imagePair;
class PairComp {
public:
    bool operator()(const imagePair &n1, const imagePair &n2) const {
        if (n1.first == n2.first) {
            return (n1.first > n2.first);
        }
        return (n1.first > n2.first);
    }
};

class Pipeline {
public:
  Pipeline();
  virtual ~Pipeline() = default;

protected:
  std::unique_ptr<ThreadSafePriorQueue<imagePair, PairComp>> preprocess_ring_;
  std::unique_ptr<ThreadSafePriorQueue<imagePair, PairComp>> postprocess_ring_;
};
Pipeline::Pipeline():preprocess_ring_(std::make_unique<ThreadSafePriorQueue<imagePair, PairComp>>(100)),
      postprocess_ring_(std::make_unique<ThreadSafePriorQueue<imagePair, PairComp>>(100)) {}

class Model : public Pipeline {
 public:
  Model() {}
  ~Model() = default;
  void run(const int run_file);

 private:
  void preProcess(PipelineMessage& str);
  void postProcess(PipelineMessage& str);
  void forward(PipelineMessage& str);
  void runA(const int id);
  void runB();
  void runC();
};

void Model::preProcess(PipelineMessage& str) {
  str.info = "preProcess";
  std::cout<< str.info <<std::endl;
  sleep(20);
}

void Model::forward(PipelineMessage& str) {
  str.info = "forward";
  std::cout<< str.info <<std::endl;
  sleep(1);
}

void Model::postProcess(PipelineMessage& str) {
  str.info = "postProcess";
  std::cout<< str.info <<std::endl;
  sleep(10);
}

void Model::runA(const int id) {
    std::cout<< id <<std::endl;
    std::string str = "test_runA";
    for(int i=0;i<id;i++) {

      PipelineMessage str;
      preProcess(str);

      imagePair temp_pair;
      temp_pair.first = i;
     
      if(i==(id-1)) {
          str.last_frame = true;
      } else {
        str.last_frame = false;
      }
      temp_pair.second = str;
      preprocess_ring_->Enqueue(temp_pair);
    }
}

void Model::runB() {
  while (1) {
    imagePair tempImagePair;
    if(!preprocess_ring_->WaitDequeue(&tempImagePair)) {
        break;
    }
    forward(tempImagePair.second);

    imagePair temp_pair = tempImagePair;
    postprocess_ring_->Enqueue(temp_pair);

    if(tempImagePair.second.last_frame) {
      break;
    }
  }
}

void Model::runC() {
  while (1) {
    imagePair tempImagePair;
    if(!postprocess_ring_->WaitDequeue(&tempImagePair)) {
        break;
    }
    postProcess(tempImagePair.second);
    // 其他操作

    if(tempImagePair.second.last_frame) {
      break;
    }
  }
}

void Model::run(const int run_file) {
  auto future_a = std::async(std::launch::async, [this, &run_file]() { runA(run_file); });
  auto future_b = std::async(std::launch::async, [this]() { runB(); });
  auto future_c = std::async(std::launch::async, [this]() { runC(); });
  future_a.wait();
  future_b.wait();
  future_c.wait();
}

int main(int argc, char **argv) {
  auto model = std::make_unique<Model>();
  const int run_file = 10;
  model->run(run_file);

  return 0;
}
