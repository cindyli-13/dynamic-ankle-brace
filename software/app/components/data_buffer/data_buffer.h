#pragma once

#include "assert.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

// Note: DataBuffer objects must only be allocated in global static memory
template <typename T>
class DataBuffer {
 public:
  DataBuffer() : _has_data(false) {
    _mutex = xSemaphoreCreateMutexStatic(&_mutex_buffer);
    assert(_mutex);
  }

  bool read(T& data) {
    // TODO: set timeout?
    if (_has_data && xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE) {
      data = *reinterpret_cast<T*>(_data);
      xSemaphoreGive(_mutex);
      return true;
    } else {
      return false;
    }
  }

  bool write(T& data) {
    // TODO: set timeout?
    if (xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE) {
      *reinterpret_cast<T*>(_data) = data;
      _has_data = true;
      xSemaphoreGive(_mutex);
      return true;
    } else {
      return false;
    }
  }

 private:
  alignas(T) uint8_t _data[sizeof(T)];
  bool _has_data;
  StaticSemaphore_t _mutex_buffer;
  SemaphoreHandle_t _mutex;
};
