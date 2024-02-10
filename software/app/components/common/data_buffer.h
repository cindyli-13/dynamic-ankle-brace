#pragma once

#include "assert.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

// Implemented as circular buffer using FreeRTOS queue
template <typename T, size_t LEN>
class DataBuffer {
 public:
  DataBuffer() {
    assert(handle_ = xQueueCreateStatic(LEN, sizeof(T), storage_, &buffer_));
  }

  bool send(T& data) {
    if (uxQueueSpacesAvailable(handle_) == 0) {
      T dummy;
      xQueueReceive(handle_, &dummy, portMAX_DELAY);  // TODO: add timeout?
    }
    // TODO: add timeout?
    return xQueueSend(handle_, &data, portMAX_DELAY) == pdTRUE;
  }

  bool receive(T& data) {
    // TODO: add timeout?
    return xQueueReceive(handle_, &data, portMAX_DELAY) == pdTRUE;
  }

 private:
  QueueHandle_t handle_;
  StaticQueue_t buffer_;
  uint8_t storage_[LEN * sizeof(T)];
};
