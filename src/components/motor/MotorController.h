#pragma once

#include <FreeRTOS.h>
#include <timers.h>
#include <cstdint>

#define NOTIFICATION_PATTERN_LENGTH 12

namespace Pinetime {
  namespace Controllers {

    class MotorController {
    public:
      MotorController()
          : notificationPattern  { 3, -1, 3, -1, 3, -1, 10, 0}
      {
      }

      void Init();
      void RunForDuration(uint8_t motorDuration);
      void StartRinging();
      void StopRinging();

      void DoNotification();

      // Each entry represents the number of timer calls. Negative numbers are pauses, positives are buzzes.
      int8_t notificationPattern[NOTIFICATION_PATTERN_LENGTH];

    private:
      static void Ring(TimerHandle_t xTimer);
      static void DoNotificationStep(TimerHandle_t xTimer);
      static void StopMotor(TimerHandle_t xTimer);
      TimerHandle_t shortVib;
      TimerHandle_t longVib;
      TimerHandle_t notificationVib;
      int8_t numTimesNotificationStepCalled {0};

      size_t notificationPatternNextIndex {0};
      bool isDoingNotification {false};
    };
  }
}
