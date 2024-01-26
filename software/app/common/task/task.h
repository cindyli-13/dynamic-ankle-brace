#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

class Task {

 public:
  struct Config {
    const char* const name;
    const uint32_t stack_depth;
    void* const param;
    UBaseType_t priority;
    StackType_t* const stack_buffer;
    const BaseType_t core_id;
  };

  Task(const Config& config)
      : _config(&config), _param_wrapper({this, config.param}) {}

  void create() {
    init();
    _handle = xTaskCreateStaticPinnedToCore(
        run_wrapper, _config->name, _config->stack_depth, &_param_wrapper,
        _config->priority, _config->stack_buffer, &_task_buffer,
        _config->core_id);
  }

  virtual void init() {}
  virtual void run(void* param) = 0;

 private:
  struct ParamWrapper {
    Task* task;
    void* param;
  };

  static void run_wrapper(void* param) {
    ParamWrapper* param_wrapper = static_cast<ParamWrapper*>(param);
    param_wrapper->task->run(param_wrapper->param);
  }

  const Config* const _config;
  ParamWrapper _param_wrapper;
  StaticTask_t _task_buffer;
  TaskHandle_t _handle;
};
