#pragma once

// Note: not thread safe
class EMAFilter {
 public:
  EMAFilter(float alpha, float initial_value = 0.0f)
      : alpha_(alpha), filtered_value_(initial_value) {}

  inline void update(float measurement) {
    filtered_value_ = (filtered_value_ * alpha_) + (measurement * (1 - alpha_));
  }

  inline float get() { return filtered_value_; }

 private:
  float alpha_;
  float filtered_value_;
};
