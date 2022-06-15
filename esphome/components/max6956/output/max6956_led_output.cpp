#include "max6956_led_output.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome {
namespace max6956 {

static const char *const TAG = "max6956_led_channel";

void MAX6956LedChannel::write_state(float state) {
  this->parent_->set_pin_brightness(this->pin_, state);
  ESP_LOGD(TAG, "write_state float pin[%u]=%f", this->pin_, state); 
}

void MAX6956LedChannel::write_state(bool state) {
  this->parent_->digital_write(this->pin_, state);
  ESP_LOGD(TAG, "write_state bool pin[%u]=%u", this->pin_, state);
}

void MAX6956LedChannel::setup() {
  ESP_LOGD(TAG, "setup pin %d", this->pin_);
  this->parent_->pin_mode(this->pin_, max6956::FLAG_LED);
  this->turn_off();
}

void MAX6956LedChannel::dump_config() {
  ESP_LOGCONFIG(TAG, "MAX6956 current:");
  ESP_LOGCONFIG(TAG, "  MAX6956 pin: %d", this->pin_);
  LOG_FLOAT_OUTPUT(this);
}


}  // namespace sx1509
}  // namespace esphome
