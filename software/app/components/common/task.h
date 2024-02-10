#pragma once

#include "assert.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Wrapper around FreeRTOS task
// Derived classes should implement the init() and run() methods
class Task {
 public:
  struct Config {
    const char* const name;
    const uint32_t stack_depth;
    UBaseType_t priority;
    StackType_t* const stack_buffer;
    const BaseType_t core_id;
  };

  Task(const Config& config, void* const param)
      : config_(&config), param_wrapper_({this, param}) {}

  void create() {
    init();
    assert(handle_ = xTaskCreateStaticPinnedToCore(
               run_wrapper, config_->name, config_->stack_depth,
               &param_wrapper_, config_->priority, config_->stack_buffer,
               &task_buffer_, config_->core_id));
  }

 private:
  struct ParamWrapper {
    Task* task;
    void* param;
  };

  static void run_wrapper(void* param) {
    ParamWrapper* param_wrapper = static_cast<ParamWrapper*>(param);
    param_wrapper->task->run(param_wrapper->param);
  }

  virtual void init() {}
  virtual void run(void* param) = 0;

  const Config* const config_;
  ParamWrapper param_wrapper_;
  StaticTask_t task_buffer_;
  TaskHandle_t handle_;
};
