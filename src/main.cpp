#include "system.hpp"

System device = System();

void setup() {
  device.init();
}

void loop() {
  device.tick();
}
