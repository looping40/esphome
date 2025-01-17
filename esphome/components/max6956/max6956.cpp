#include "max6956.h"
#include "esphome/core/log.h"

namespace esphome {
namespace max6956 {

static const char *const TAG = "max6956";

/// Masks for MAX6956 Configuration register
const uint32_t MASK_TRANSITION_DETECTION = 0x80;
const uint32_t MASK_INDIVIDUAL_CURRENT = 0x40;
const uint32_t MASK_NORMAL_OPERATION = 0x01;

const uint32_t MASK_1PORT_VALUE = 0x03;
const uint32_t MASK_PORT_CONFIG = 0x03;
const uint8_t MASK_CONFIG_CURRENT = 0x40;
const uint8_t MASK_CURRENT_PIN = 0x0F;

/**************************************
 *    MAX6956                         *
 **************************************/
void MAX6956::setup() {
  ESP_LOGCONFIG(TAG, "Setting up MAX6956...");
  uint8_t configuration;
  if (!this->read_reg_(MAX6956_CONFIGURATION, &configuration)) {
    this->mark_failed();
    return;
  }

  write_brightness_global_(global_brightness_);
  write_brightness_mode_(brightness_mode_);

  /** TO DO : read transition detection in yaml
      TO DO : read indivdual current in yaml **/
  this->read_reg_(MAX6956_CONFIGURATION, &configuration);
  ESP_LOGD(TAG, "Initial reg[0x%.2X]=0x%.2X", MAX6956_CONFIGURATION, configuration);
  configuration = configuration | MASK_NORMAL_OPERATION;
  this->write_reg_(MAX6956_CONFIGURATION, configuration);

  ESP_LOGCONFIG(TAG, "Enabling normal operation");
  ESP_LOGD(TAG, "setup reg[0x%.2X]=0x%.2X", MAX6956_CONFIGURATION, configuration);
}

bool MAX6956::digital_read(uint8_t pin) {
  uint8_t reg_addr = MAX6956_1PORT_VALUE_START + pin;
  uint8_t value = 0;
  this->read_reg_(reg_addr, &value);
  return (value & MASK_1PORT_VALUE);
}

void MAX6956::digital_write(uint8_t pin, bool value) {
  ESP_LOGCONFIG(TAG, "digital_write pin[%u]=%u", pin, value);
  uint8_t reg_addr = MAX6956_1PORT_VALUE_START + pin;
  this->write_reg_(reg_addr, value);
}

void MAX6956::pin_mode(uint8_t pin, gpio::Flags flags) {
  uint8_t reg_addr = MAX6956_PORT_CONFIG_START + (pin - MAX6956_MIN) / 4;
  uint8_t config = 0;
  uint8_t shift = 2 * (pin % 4);
  uint8_t mode = 0;

  if (flags == gpio::FLAG_INPUT) {
    mode = MAX6956_INPUT;
  } else if (flags == (gpio::FLAG_INPUT | gpio::FLAG_PULLUP)) {
    mode = MAX6956_INPUT_PULLUP;
  } else if (flags == gpio::FLAG_OUTPUT) {
    mode = MAX6956_OUTPUT;
  }

  this->read_reg_(reg_addr, &config);
  config &= ~(MASK_PORT_CONFIG << shift);
  config |= (mode << shift);
  this->write_reg_(reg_addr, config);
  ESP_LOGD(TAG, "pin_mode IO pin[%u] reg[0x%u]=0x%.2X", pin, reg_addr, config);
}

void MAX6956::pin_mode(uint8_t pin, max6956::MAX6956GPIOFlag flags) {
  uint8_t reg_addr = MAX6956_PORT_CONFIG_START + (pin - MAX6956_MIN) / 4;
  uint8_t config = 0;
  uint8_t shift = 2 * (pin % 4);
  uint8_t mode = 0;

  if (flags == max6956::FLAG_LED) {
    mode = MAX6956_LED;
  }

  this->read_reg_(reg_addr, &config);
  // ESP_LOGD(TAG, "initial reg[0x%.2X]=0x%.2X shift=%u", reg_addr, config,
  // shift);
  config &= ~(MASK_PORT_CONFIG << shift);
  config |= (mode << shift);
  this->write_reg_(reg_addr, config);

  ESP_LOGCONFIG(TAG, "pin_mode pin[%u] = LED", pin);
  ESP_LOGD(TAG, "pin_mode LED pin[%u] = reg[0x%.2X] = 0x%.2X", pin, reg_addr, config);
}

void MAX6956::write_brightness_global_(uint8_t current) {
  if (current > 15) {
    ESP_LOGE(TAG, "Global brightness out off range (%u)", current);
    return;
  }
  this->write_reg_(MAX6956_GLOBAL_CURRENT, current);

  ESP_LOGCONFIG(TAG, "set global brightness = %u", current);
  ESP_LOGD(TAG, "set_brightness_global reg[0x%.2X] = 0x%.2X", MAX6956_GLOBAL_CURRENT, current);
}

void MAX6956::write_brightness_mode_(max6956::MAX6956CURRENTMODE brightness_mode) {
  uint8_t reg_addr = MAX6956_CONFIGURATION;
  uint8_t config = 0;

  this->read_reg_(reg_addr, &config);
  config &= ~MASK_CONFIG_CURRENT;
  config |= brightness_mode << 6;
  this->write_reg_(reg_addr, config);

  ESP_LOGCONFIG(TAG, "write_brightness_mode = '%s'", brightness_mode == 0 ? "global" : "segment");
  ESP_LOGD(TAG, "write_brightness_mode reg[0x%.2X] = 0x%.2X", reg_addr, config);
}

void MAX6956::set_pin_brightness(uint8_t pin, float brightness) {
  uint8_t reg_addr = MAX6956_CURRENT_START + (pin - MAX6956_MIN) / 2;
  uint8_t config = 0;
  uint8_t shift = 4 * (pin % 2);
  uint8_t bright = roundf(brightness * 15);
  // ESP_LOGD(TAG, "new brightness=%f, mode=%u", brightness, mode);

  if (prev_bright_[pin - MAX6956_MIN] == bright)
    return;

  prev_bright_[pin - MAX6956_MIN] = bright;

  this->read_reg_(reg_addr, &config);
  config &= ~(MASK_CURRENT_PIN << shift);
  config |= (bright << shift);
  this->write_reg_(reg_addr, config);
  ESP_LOGCONFIG(TAG, "set_pin_brightness(%.2f%%) pin[%u] = %u", brightness, pin, bright);
  ESP_LOGD(TAG, "set_pin_brightness reg=[0x%.2X]=0x%.2X", reg_addr, config);
}

bool MAX6956::read_reg_(uint8_t reg, uint8_t *value) {
  if (this->is_failed())
    return false;

  return this->read_byte(reg, value);
}

bool MAX6956::write_reg_(uint8_t reg, uint8_t value) {
  if (this->is_failed())
    return false;

  return this->write_byte(reg, value);
}

/*void MAX6956::dump_config() {
  ESP_LOGCONFIG(TAG, "MAX6956");
  //ESP_LOGCONFIG(TAG, "  MAX6956 pin: %d", this->pin_);
  //LOG_FLOAT_OUTPUT(this);
}*/

/**************************************
 *    MAX6956GPIOPin                  *
 **************************************/
void MAX6956GPIOPin::setup() { pin_mode(flags_); }
void MAX6956GPIOPin::pin_mode(gpio::Flags flags) { this->parent_->pin_mode(this->pin_, flags); }
bool MAX6956GPIOPin::digital_read() { return this->parent_->digital_read(this->pin_) != this->inverted_; }
void MAX6956GPIOPin::digital_write(bool value) { this->parent_->digital_write(this->pin_, value != this->inverted_); }
std::string MAX6956GPIOPin::dump_summary() const {
  char buffer[32];
  snprintf(buffer, sizeof(buffer), "%u via Max6956", pin_);
  return buffer;
}

}  // namespace max6956
}  // namespace esphome
