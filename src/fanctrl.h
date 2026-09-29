#ifndef LENOVO_FAN_CONTROL_H
#define LENOVO_FAN_CONTROL_H

#ifdef __cplusplus
extern "C" {
#endif

extern volatile int is_keep_fan_running;

enum FanMode { NORMAL, FAST };

int fan_control(enum FanMode mode);
int read_state(void);
void keep_fan_running(void);

#ifdef __cplusplus
}
#endif

#endif