#pragma once

#include <stdint.h>

class MCP3424 {
private:
  struct __helper {
    const int channel;
    MCP3424& mcp;
  };

public:
  void begin(bool continuous = true, int user_address = 0);
  long analogRead(int channel);
  void analogReadResolution(int resolution);
  void setGain(int gain);

  inline __helper operator[](int x) { return {x, *this}; }

private:
  // Configuration register bits; §5.2
  enum { G0, G1, S0, S1, OC, C0, C1, RDY };

  // Configuration register masks; §5.2
  static constexpr uint8_t G_mask = (1 << G1) | (1 << G0);
  static constexpr uint8_t S_mask = (1 << S1) | (1 << S0);
  static constexpr uint8_t C_mask = (1 << C1) | (1 << C0);

  // Fixed address bits; §5.3.1
  static constexpr uint8_t device_code = 0b1101;

  // Selectable address bits; §5.3.2
  uint8_t address;

  // Default configuration; §5.2
  uint8_t config = 0b10010000;

  // Write settings data to the configuration register if needed
  void write(uint8_t data, uint8_t mask);

  friend long analogRead(const __helper&);
};

inline long analogRead(const MCP3424::__helper& x) {
  return x.mcp.analogRead(x.channel);
}
