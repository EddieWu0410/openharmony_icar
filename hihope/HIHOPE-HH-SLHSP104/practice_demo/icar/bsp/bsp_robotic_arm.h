/*
    @ usage examples ：

    // 1. create a handle
    RoboticArm_Handler * robotic_arm_handler = robotic_arm_init();

    // 2. set initial angle
    robotic_arm_handler->set_angle(RoboticArm_S1, 1, 10);           // set number RoboticArm_S1, rotate 1°, speed 10，fastest speed
    robotic_arm_handler->set_angle(RoboticArm_S2, 1, 10);
    robotic_arm_handler->set_angle(RoboticArm_S3, 1, 10);
    robotic_arm_handler->set_angle(RoboticArm_S4, 1, 10);
    robotic_arm_handler->set_angle(RoboticArm_S5, 1, 10);
    robotic_arm_handler->set_angle(RoboticArm_S6, 1, 10);

    // 3. enable signal output
    robotic_arm_handler->enable();

    while(1) {
        delay_ms(4000);

        // 4. arbitrary angle adjustment
        robotic_arm_handler->set_angle(RoboticArm_S1, -90, 1);      // set number RoboticArm_S1, rotate -90°, speed 1
        robotic_arm_handler->set_angle(RoboticArm_S2, -90, 8);
        robotic_arm_handler->set_angle(RoboticArm_S3, -90, 8);
        robotic_arm_handler->set_angle(RoboticArm_S4, -90, 8);
        robotic_arm_handler->set_angle(RoboticArm_S5, -90, 8);
        robotic_arm_handler->set_angle(RoboticArm_S6, -90, 8);

        delay_ms(3000);

        robotic_arm_handler->set_angle(RoboticArm_S1, 0,  1);
        robotic_arm_handler->set_angle(RoboticArm_S2, 90, 4);
        robotic_arm_handler->set_angle(RoboticArm_S3, 90, 4);
        robotic_arm_handler->set_angle(RoboticArm_S4, 90, 4);
        robotic_arm_handler->set_angle(RoboticArm_S5, 90, 4);
        robotic_arm_handler->set_angle(RoboticArm_S6, 90, 4);
    }
*/


#ifndef BSP_ROBOTIC_ARM_H
#define BSP_ROBOTIC_ARM_H

#include <stdint.h>

/**
  * @brief engine number
  */
typedef enum {
    RoboticArm_S1 = 0L,
    RoboticArm_S2,
    RoboticArm_S3,
    RoboticArm_S4,
    RoboticArm_S5,
    RoboticArm_S6,
    RoboticArm_MAX

}RoboticArm_NO;

/**
  * @brief robotic arm control handle
  */
typedef struct {

    /**
    * @brief enable signal output
    */
    void (*enable)(void);

    /**
    * @brief disable signal output
    */
    void (*disable)(void);

    /**
    * @brief Set the rotation angle of the corresponding engine
    * @param no   : To select the engine number.
    * @param value: The angle value of engine rotation [ -90 - 90 ]
    * @param speed: The rotational speed of the motor [ 0 - 10 ] （0 slowest 10 fastest）
    * @retval 0  is success
    * @retval !0 is parameter error
    */
    uint8_t (*set_angle)(RoboticArm_NO no, int8_t value, uint8_t speed);
}RoboticArm_Handler;

RoboticArm_Handler *robotic_arm_init(void);
void robotic_arm_uninit(void);

#endif // BSP_ROBOTIC_ARM_H