#ifndef DRIVER_STM_H_
#define DRIVER_STM_H_

typedef struct {
        int u8nuScheduling1msFlag;
        int u8nuScheduling10msFlag;
        int u8nuScheduling100msFlag;
        int u8nuScheduling1000msFlag;
} SchedulingFlag;

extern SchedulingFlag stSchedulingInfo;

extern void Driver_Stm_Init(void);


#endif /* DRIVER_STM_H_ */
