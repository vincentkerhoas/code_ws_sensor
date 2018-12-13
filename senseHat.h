
#include "main.h"

#include <linux/i2c-dev.h>
#include <fcntl.h>


int senseHat_init();
int senseHat_getPressureTemperature(float* press, float* temp);
int senseHat_close();

