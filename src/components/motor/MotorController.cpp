#include "components/motor/MotorController.h"
#include <hal/nrf_gpio.h>
#include "systemtask/SystemTask.h"
#include "drivers/PinMap.h"

using namespace Pinetime::Controllers;

void MotorController::Init() {
  nrf_gpio_cfg_output(PinMap::Motor);
  nrf_gpio_pin_set(PinMap::Motor);

  shortVib = xTimerCreate("shortVib", 1, pdFALSE, nullptr, StopMotor);
  longVib = xTimerCreate("longVib", pdMS_TO_TICKS(100), pdTRUE, this, Ring);
  notificationVib = xTimerCreate("notificationVib", pdMS_TO_TICKS(2000), pdFALSE, this, DoNotificationStep);
}

void MotorController::Ring(TimerHandle_t xTimer) {
  auto* motorController = static_cast<MotorController*>(pvTimerGetTimerID(xTimer));
  motorController->RunForDuration(50);
}

void MotorController::RunForDuration(uint8_t motorDuration) {
  if (motorDuration > 0 && xTimerChangePeriod(shortVib, pdMS_TO_TICKS(motorDuration), 0) == pdPASS && xTimerStart(shortVib, 0) == pdPASS) {
    nrf_gpio_pin_clear(PinMap::Motor);
  }
}

void MotorController::StartRinging() {
  RunForDuration(50);
  xTimerStart(longVib, 0);
}

void MotorController::StopRinging() {
  xTimerStop(longVib, 0);
  nrf_gpio_pin_set(PinMap::Motor);
}

void MotorController::DoNotification()
{
    printf("In DoNotification()\n");
    isDoingNotification = true;
    xTimerStart(notificationVib, 0);
    printf("End of DoNotification()\n");
}

void MotorController::DoNotificationStep(TimerHandle_t xTimer)
{
    printf("In DoNotificationStep()\n");
    static size_t numTimesCalledThisSegment = 0;
    auto motorController = static_cast<MotorController*>(pvTimerGetTimerID(xTimer));

    printf("Notification pattern = ");
    for (size_t i = 0; i < NOTIFICATION_PATTERN_LENGTH && motorController &&
            motorController->notificationPattern &&
            motorController->notificationPattern[i] != 0;
            ++i)
    {
        printf("%d ", motorController->notificationPattern[i]);
    }
    printf("\n");

    // TODO: Validate we should proceed with notifying
    //
    int8_t notificationElement = motorController->notificationPattern[motorController->notificationPatternNextIndex];
    printf("Current element: %d, (index = %u)\n", notificationElement, motorController->notificationPatternNextIndex);
    if (notificationElement > 0)
    {
        // Start this timer again.
        xTimerStart(motorController->notificationVib, 0);
        // Set motor on
        nrf_gpio_pin_clear(PinMap::Motor);
        // Set counter
        motorController->numTimesNotificationStepCalled++;
        // Check if we should advance the index or not
        printf("Number of times this step was called: %u", motorController->numTimesNotificationStepCalled);
        if (motorController->numTimesNotificationStepCalled >= notificationElement)
        {
            motorController->notificationPatternNextIndex++;
            printf("Advancing index to %u\n", motorController->notificationPatternNextIndex);
            motorController->numTimesNotificationStepCalled = 0;
        }
    }
    else if (notificationElement < 0)
    {
        // We need to call this function again.
        xTimerStart(motorController->notificationVib, 0);
        // Turn motor off; this is a rest.
        nrf_gpio_pin_set(PinMap::Motor);
        motorController->numTimesNotificationStepCalled--; // For ease of comparison
        // Check if we should advance the index or not
        if (motorController->numTimesNotificationStepCalled <= notificationElement)
        {
            motorController->notificationPatternNextIndex++;
            printf("Advancing index to %u\n", motorController->notificationPatternNextIndex);
            motorController->numTimesNotificationStepCalled = 0;
        }
    }
    else // notificationElement == 0
    {
        printf("notificationElement must equal 0\n");
        // Time to end things.
        motorController->isDoingNotification = false;
        motorController->numTimesNotificationStepCalled = 0;
        nrf_gpio_pin_set(PinMap::Motor);
        motorController->notificationPatternNextIndex = 0;
        xTimerStop(xTimer, 0);
    }

#if 0
    if (motorController->isDoingNotification)
    {
        // Notification is over if index >= max or if the next array element is 0.
        if (motorController->notificationPatternNextIndex < NOTIFICATION_PATTERN_LENGTH)
        {
            int8_t patternElement = motorController->notificationPattern[motorController->notificationPatternNextIndex];
            if (patternElement < 0)
            {
                nrf_gpio_pin_set(PinMap::Motor);
                numTimesCalledThisSegment++;
                motorController->notificationPatternNextIndex++;
                printf("patternElement = %d\n", (int)patternElement);
            }
            else if (patternElement == 0)
            {
                motorController->notificationPatternNextIndex = 0;
                motorController->isDoingNotification = false;
                printf("patternElement = %d, stopping notification\n", (int)patternElement);
            }
            else if (patternElement > 0)
            {
                nrf_gpio_pin_clear(PinMap::Motor);
                numTimesCalledThisSegment++;
                motorController->notificationPatternNextIndex++;
                printf("patternElement = %d\n", (int)patternElement);
            }
        }
        else
        {
            motorController->notificationPatternNextIndex++;
            motorController->isDoingNotification = false;
            printf("Reached end of notification pattern, stopping notification\n");
        }
    }
    else
    {
        printf("Stoping notificationVib since motorController is not doing notifications anymore.\n");
        nrf_gpio_pin_set(PinMap::Motor);
        xTimerStop(motorController->notificationVib, 0);
    }

    printf("Ending DoNotificationStep() with isDoingNotification = %s\n", motorController->isDoingNotification? "true" : "false");
    xTimerStop(xTimer, 0);
#endif
}

void MotorController::StopMotor(TimerHandle_t /*xTimer*/) {
  nrf_gpio_pin_set(PinMap::Motor);
}
