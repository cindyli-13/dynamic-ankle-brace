#pragma once

#include "assert.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

// Note: DataBuffer objects must only be allocated in global static memory
template <typename T>
class DataBuffer {
 public:
  DataBuffer() : has_data_(false) {
    assert(mut_ = xSemaphoreCreateMutexStatic(&mut_buffer_));
  }

  bool read(T& data) {
    // TODO: set timeout?
    if (has_data_ && xSemaphoreTake(mut_, portMAX_DELAY) == pdTRUE) {
      data = *reinterpret_cast<T*>(data_);
      xSemaphoreGive(mut_);
      return true;
    } else {
      return false;
    }
  }

  bool write(T& data) {
    // TODO: set timeout?
    if (xSemaphoreTake(mut_, portMAX_DELAY) == pdTRUE) {
      *reinterpret_cast<T*>(data_) = data;
      has_data_ = true;
      xSemaphoreGive(mut_);
      return true;
    } else {
      return false;
    }
  }

 private:
  alignas(T) uint8_t data_[sizeof(T)];
  bool has_data_;
  StaticSemaphore_t mut_buffer_;
  SemaphoreHandle_t mut_;
};
