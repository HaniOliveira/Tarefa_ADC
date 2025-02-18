#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "lib/ssd1306.h"
#include "lib/font.h"

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C
#define JOYSTICK_X_PIN 27 // GPIO para eixo X
#define JOYSTICK_Y_PIN 26 // GPIO para eixo Y
#define JOYSTICK_PB 22    // GPIO para botão do Joystick
#define Botao_A 5         // GPIO para botão A
#define LED_RED 13        // GPIO para LED Vermelho
#define LED_BLUE 12       // GPIO para LED Azul
#define LED_GREEN 11      // GPIO para LED Verde

// Trecho para modo BOOTSEL com botão B
#include "pico/bootrom.h"
#define botaoB 6
void gpio_irq_handler(uint gpio, uint32_t events)
{
  reset_usb_boot(0, 0);
}

void pwm_config_pin(uint gpio)
{
  gpio_set_function(gpio, GPIO_FUNC_PWM);
  uint slice_num = pwm_gpio_to_slice_num(gpio);
  pwm_set_wrap(slice_num, 255); // Define resolução do PWM (0-255)
  pwm_set_enabled(slice_num, true);
}

uint8_t map_adc_to_pwm(uint16_t adc_value, uint16_t center_value)
{
  uint16_t pwm_value;

  if (adc_value < center_value)
  {
    // Mapeia valores abaixo do centro para 0-255
    // Para Y: 1901 até 17
    // Para X: 2038 até 16
    pwm_value = (center_value - adc_value) * 255 / (center_value - 16);
  }
  else
  {
    // Mapeia valores acima do centro para 0-255
    // Para Y: 1901 até 4083
    // Para X: 2038 até 4083
    pwm_value = (adc_value - center_value) * 255 / (4083 - center_value);
  }

  return (uint8_t)pwm_value;
}

// Variáveis globais
bool leds_enabled = true;
bool green_led_state = false;
uint8_t border_style = 0; // 0: sem borda, 1: borda simples, 2: borda dupla

// Função para desenhar bordas
void draw_border(ssd1306_t *ssd, uint8_t style)
{
  switch (style)
  {
  case 1:                                                          // Borda simples
    ssd1306_line(ssd, 0, 0, WIDTH - 1, 0, true);                   // Topo
    ssd1306_line(ssd, 0, HEIGHT - 1, WIDTH - 1, HEIGHT - 1, true); // Base
    ssd1306_line(ssd, 0, 0, 0, HEIGHT - 1, true);                  // Esquerda
    ssd1306_line(ssd, WIDTH - 1, 0, WIDTH - 1, HEIGHT - 1, true);  // Direita
    break;
  case 2: // Borda dupla
    // Borda externa
    ssd1306_line(ssd, 0, 0, WIDTH - 1, 0, true);
    ssd1306_line(ssd, 0, HEIGHT - 1, WIDTH - 1, HEIGHT - 1, true);
    ssd1306_line(ssd, 0, 0, 0, HEIGHT - 1, true);
    ssd1306_line(ssd, WIDTH - 1, 0, WIDTH - 1, HEIGHT - 1, true);
    // Borda interna
    ssd1306_line(ssd, 2, 2, WIDTH - 3, 2, true);
    ssd1306_line(ssd, 2, HEIGHT - 3, WIDTH - 3, HEIGHT - 3, true);
    ssd1306_line(ssd, 2, 2, 2, HEIGHT - 3, true);
    ssd1306_line(ssd, WIDTH - 3, 2, WIDTH - 3, HEIGHT - 3, true);
    break;
  default: // Sem borda
    break;
  }
}

// Handler de interrupção comum para ambos os botões
void gpio_callback(uint gpio, uint32_t events)
{
  static uint32_t last_interrupt_time_a = 0;
  static uint32_t last_interrupt_time_joy = 0;
  uint32_t current_time = to_ms_since_boot(get_absolute_time());

  // Verifica qual botão foi pressionado
  if (gpio == Botao_A)
  {
    // Debounce para botão A
    if (current_time - last_interrupt_time_a > 200)
    {
      // Inverte o estado dos LEDs RGB
      leds_enabled = !leds_enabled;

      // Se os LEDs foram desativados, apaga-os
      if (!leds_enabled)
      {
        pwm_set_gpio_level(LED_RED, 0);
        pwm_set_gpio_level(LED_BLUE, 0);
      }

      last_interrupt_time_a = current_time;
    }
  }
  else if (gpio == JOYSTICK_PB)
  {
    // Debounce para botão do joystick
    if (current_time - last_interrupt_time_joy > 200)
    {
      // Alterna o estado do LED Verde
      green_led_state = !green_led_state;
      pwm_set_gpio_level(LED_GREEN, green_led_state ? 255 : 0);

      // Alterna o estilo da borda
      border_style = (border_style + 1) % 3;

      last_interrupt_time_joy = current_time;
    }
  }
}

int main()
{
  // Inicialização do stdio
  stdio_init_all();

  // Para ser utilizado o modo BOOTSEL com botão B
  gpio_init(botaoB);
  gpio_set_dir(botaoB, GPIO_IN);
  gpio_pull_up(botaoB);
  gpio_set_irq_enabled_with_callback(botaoB, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

  gpio_init(JOYSTICK_PB);
  gpio_set_dir(JOYSTICK_PB, GPIO_IN);
  gpio_pull_up(JOYSTICK_PB);

  gpio_init(Botao_A);
  gpio_set_dir(Botao_A, GPIO_IN);
  gpio_pull_up(Botao_A);

  // Inicialização do I2C
  i2c_init(I2C_PORT, 400 * 1000);
  gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
  gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
  gpio_pull_up(I2C_SDA);
  gpio_pull_up(I2C_SCL);

  // Inicialização do display
  ssd1306_t ssd;
  ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT);
  ssd1306_config(&ssd);
  ssd1306_send_data(&ssd);

  // Limpa o display
  ssd1306_fill(&ssd, false);
  ssd1306_send_data(&ssd);

  // Inicialização do ADC
  adc_init();
  // Inicializa os pinos GPIO para ADC
  adc_gpio_init(JOYSTICK_X_PIN);
  adc_gpio_init(JOYSTICK_Y_PIN);

  // Posição inicial do quadrado (centralizado)
  uint8_t square_x = WIDTH / 2 - 4;
  uint8_t square_y = HEIGHT / 2 - 4;
  const uint8_t SQUARE_SIZE = 8;

  // Mensagem inicial para debug
  ssd1306_draw_string(&ssd, "INICIANDO...", 0, 0);
  ssd1306_send_data(&ssd);
  sleep_ms(2000);

  // Adicione após as outras inicializações
  pwm_config_pin(LED_RED);
  pwm_config_pin(LED_BLUE);
  pwm_config_pin(LED_GREEN);
  pwm_set_gpio_level(LED_GREEN, 0); // Inicia apagado

  // Configuração das interrupções
  gpio_set_irq_enabled_with_callback(Botao_A, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
  gpio_set_irq_enabled(JOYSTICK_PB, GPIO_IRQ_EDGE_FALL, true);

  while (true)
  {
    // Invertendo a ordem de leitura dos pinos e trocando X e Y
    adc_select_input(0);               // JOYSTICK_X_PIN (27) - Movimento horizontal
    uint16_t adc_value_y = adc_read(); // Trocado: usa para Y
    adc_select_input(1);               // JOYSTICK_Y_PIN (26) - Movimento vertical
    uint16_t adc_value_x = adc_read(); // Trocado: usa para X

    // Calcula nova posição do quadrado baseado no joystick
    square_x += ((adc_value_x > 3000) ? 2 : (adc_value_x < 1000) ? -2
                                                                 : 0);
    square_y += ((adc_value_y > 3000) ? -2 : (adc_value_y < 1000) ? 2
                                                                  : 0);

    // Limita a posição do quadrado dentro da tela
    if (square_x < 0)
      square_x = 0;
    if (square_x > WIDTH - SQUARE_SIZE)
      square_x = WIDTH - SQUARE_SIZE;
    if (square_y < 0)
      square_y = 0;
    if (square_y > HEIGHT - SQUARE_SIZE)
      square_y = HEIGHT - SQUARE_SIZE;

    // Limpa o display
    ssd1306_fill(&ssd, false);

    // Desenha a borda atual
    draw_border(&ssd, border_style);

    // Desenha o quadrado na posição atual
    ssd1306_rect(&ssd, square_y, square_x, SQUARE_SIZE, SQUARE_SIZE, true, true);

    // Atualiza o display
    ssd1306_send_data(&ssd);

    // Atualiza os LEDs RGB com PWM apenas se estiverem habilitados
    if (leds_enabled)
    {
      uint8_t red_pwm = map_adc_to_pwm(adc_value_x, 2038);  // Centro do eixo X
      uint8_t blue_pwm = map_adc_to_pwm(adc_value_y, 1901); // Centro do eixo Y

      pwm_set_gpio_level(LED_RED, red_pwm);
      pwm_set_gpio_level(LED_BLUE, blue_pwm);
    }

    sleep_ms(20);
  }

  return 0;
}