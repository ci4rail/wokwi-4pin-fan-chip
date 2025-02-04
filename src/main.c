#include "wokwi-api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
  pin_t pwm_pin;
  pin_t tacho_pin;
  uint64_t last_pwm_high;
  uint64_t period;
  double duty_cycle;
  timer_t report_timer;
  int report_count;
  buffer_t framebuffer;
  uint32_t width;
  uint32_t height;
  timer_t tacho_timer;
  double rpm;
  bool tacho_state;
  uint32_t break_attr;
} chip_state_t;

typedef struct
{
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t a;
} rgba_t;

static void pwm_pin_change(void *user_data, pin_t pin, uint32_t value);
static void on_report_timer(void *user_data);
static void on_tacho_timer(void *user_data);
void draw_rpm(chip_state_t *chip, double rpm);
double duty_to_rpm(double duty_cycle);
double rpm_to_tacho_period(double rpm);
void tacho_timer_start(chip_state_t *chip);

void chip_init(void)
{
  chip_state_t *chip = malloc(sizeof(chip_state_t));

  chip->pwm_pin = pin_init("PWM", INPUT);
  chip->tacho_pin = pin_init("TACHO", OUTPUT);

  const pin_watch_config_t watch_config = {
      .edge = BOTH,
      .pin_change = pwm_pin_change,
      .user_data = chip,
  };
  pin_watch(chip->pwm_pin, &watch_config);

  const timer_config_t timer_config = {
      .callback = on_report_timer,
      .user_data = chip,
  };
  chip->report_timer = timer_init(&timer_config);
  timer_start(chip->report_timer, 50 * 1000, true);

  const timer_config_t tacho_timer_config = {
      .callback = on_tacho_timer,
      .user_data = chip,
  };
  chip->tacho_timer = timer_init(&tacho_timer_config);

  chip->framebuffer = framebuffer_init(&chip->width, &chip->height);
  //printf("Framebuffer: width=%d, height=%d\n", chip->width, chip->height);

  chip->break_attr = attr_init_float("break", 0);
}

static void pwm_pin_change(void *user_data, pin_t pin, uint32_t value)
{
  chip_state_t *chip = (chip_state_t *)user_data;
  uint64_t now = get_sim_nanos();

  if (pin == chip->pwm_pin)
  {
    if (value == HIGH)
    {
      if (chip->last_pwm_high != 0)
      {
        chip->period = now - chip->last_pwm_high;
      }
      chip->last_pwm_high = now;
    }
    else
    {
      // LOW
      if (chip->period != 0)
      {
        chip->duty_cycle = (now - chip->last_pwm_high) / (double)chip->period;
        double org_rpm = chip->rpm;
        chip->rpm = duty_to_rpm(chip->duty_cycle);
        float break_value = attr_read_float(chip->break_attr);

        if (break_value > 0.0)
        {
          chip->rpm *= (1.0 - break_value);
        }

        if (org_rpm == 0.0 && chip->rpm > 0.0)
        {
          tacho_timer_start(chip);
        }
      }
    }
  }
}

static void on_report_timer(void *user_data)
{
  chip_state_t *chip = (chip_state_t *)user_data;

  if(get_sim_nanos() - chip->last_pwm_high > 1E6)
  {
    chip->duty_cycle = 0.0;
    chip->rpm = 0.0;
  }

  printf("%d: Period: %llu, Duty Cycle: %f RPM: %f\n", chip->report_count, chip->period, chip->duty_cycle, chip->rpm);
  draw_rpm(chip, chip->rpm);
  chip->report_count++;
}

void draw_line(chip_state_t *chip, uint32_t row, rgba_t color)
{
  uint32_t offset = chip->width * 4 * row;
  for (int x = 0; x < chip->width * 4; x += 4)
  {
    buffer_write(chip->framebuffer, offset + x, (uint8_t *)&color, sizeof(color));
  }
}

void draw_rectangle(chip_state_t *chip, uint32_t x, uint32_t y, uint32_t width, uint32_t height, rgba_t color)
{
  for (int row = y; row < y + height; row++)
  {
    draw_line(chip, row, color);
  }
}

void draw_rpm(chip_state_t *chip, double rpm)
{
  rpm /= 6500;
  uint32_t h1 = chip->width * rpm;
  rgba_t color = {0, 0, 255, 255};
  draw_rectangle(chip, 0, 0, chip->width, h1, color);
  rgba_t color2 = {0, 0, 0, 255};
  uint32_t h2 = chip->height - h1;
  draw_rectangle(chip, 0, h1, chip->width, h2, color2);
}

double duty_to_rpm(double duty_cycle)
{
  double rpm;
  if (duty_cycle < 0.25)
  {
    rpm = 0.0;
  }
  else
  {
    rpm = (duty_cycle - 0.25) * (4000 * 1 / 0.75) + 2500;
  }
  return rpm;
}

// 2 ticks per revolution, double frequency for toggling
double rpm_to_tacho_period(double rpm)
{
  return 0.25 / (rpm / 60.0);
}

static void on_tacho_timer(void *user_data)
{
  chip_state_t *chip = (chip_state_t *)user_data;
  if (chip->tacho_state)
  {
    pin_write(chip->tacho_pin, HIGH);
  }
  else
  {
    pin_write(chip->tacho_pin, LOW);
  }
  chip->tacho_state = !chip->tacho_state;
  tacho_timer_start(chip);
}

void tacho_timer_start(chip_state_t *chip)
{
  if (chip->rpm == 0.0)
  {
    return;
  }
  double period = rpm_to_tacho_period(chip->rpm);
  //printf("Tacho period: %f us\n", period * 1E6);
  timer_start(chip->tacho_timer, period * 1E6, false);
}