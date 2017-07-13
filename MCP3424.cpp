#include "MCP3424.h"
#include <Arduino.h>
#include <Wire.h>

void MCP3424::begin(bool continuous, int user_address) {
  // I2C address; §5.3.1
  address = (device_code << 3) | user_address;

  // General call reset; §5.4.1
  Wire.beginTransmission(0x00);
  Wire.write(0x06);
  Wire.endTransmission();

  if (bitRead(config, OC) != continuous) {
    bitWrite(config, OC, continuous);

    Wire.beginTransmission(address);
    Wire.write(config);
    Wire.endTransmission();
  }
}

long MCP3424::analogRead(int channel) {
  // Configuration register, bits C1–C0; §5.2
  uint8_t C_bits = (channel - 1) << C0;

  write(C_bits, C_mask);

  if (!bitRead(config, OC)) {
    // If we're in one-shot mode, initiate a conversion
    Wire.beginTransmission(address);
    Wire.write(config | (1 << RDY));
    Wire.endTransmission();
  }

  // Configuration register, bits S1–S0; §5.2
  uint8_t S_bits = (config & S_mask) >> S0;
  int resolution = (S_bits + 6) * 2;

  // Conversion data output; §5.3.4
  uint32_t output = 0;

  do {
    // Up to three data bytes and at least one config byte
    Wire.requestFrom(address, 4, 0x00, 0, true);

    // 18-bit conversion mode outputs an extra byte
    if (resolution == 18) {
      output |= (uint32_t)Wire.read() << 16;
    }

    output |= (uint32_t)Wire.read() << 8;
    output |= (uint32_t)Wire.read();

    config = (uint8_t)Wire.read();

    // Loop until new conversion result is read; TABLE 5-2
  } while (bitRead(config, RDY));

  // Normalise the output by adjusting for resolution
  output <<= (32 - resolution);

  // Convert to signed and scale back down to preserving negative values
  return (int32_t)output / (1L << (32 - resolution));
}

void MCP3424::analogReadResolution(int resolution) {
  // Configuration register, bits S1–S0; §5.2
  uint8_t S_bits = ((resolution / 2) + 6) << S0;

  write(S_bits, S_mask);
}

void MCP3424::setGain(int gain) {
  // Configuration register, bits G1–G0; §5.2
  //
  // __builtin_ffs(): Returns one plus the index of the least significant 1-bit of x
  // https://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html#index-_005f_005fbuiltin_005fffs
  uint8_t G_bits = (__builtin_ffs(gain) - 1) << G0;

  write(G_bits, G_mask);
}

// Write settings data to the configuration register if needed
void MCP3424::write(uint8_t data, uint8_t mask) {
  if ((config & mask) != (data & mask)) {
    config = (config & ~mask) | (data & mask);
    
    // Writing to RDY and OC initiates conversions, so we also mask these bits
    uint8_t conversion_mask = (1 << RDY) | (1 << OC);
    
    Wire.beginTransmission(address);
    Wire.write(config & ~conversion_mask);
    Wire.endTransmission();
  }
}
