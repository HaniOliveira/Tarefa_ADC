#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
#define JOYSTICK_X_PIN 27
#define JOYSTICK_Y_PIN 26
#define JOYSTICK_PB 22
#define Botao_A 5
#define Botao_B 6 // Agora botão B será usado para desenhar

// Definições dos estados e jogos
#define CURSOR_SIZE 2
#define MENU_STATE 0
#define PAINT_STATE 1
#define SNAKE_STATE 2

// Definições do jogo Snake
#define SNAKE_SIZE 2
#define MAX_SNAKE_LENGTH 100
#define FOOD_SIZE 2

// Adicione estas definições após as outras definições
#define LED_VERDE 2
#define LED_VERMELHO 3

// Estrutura para posição
typedef struct
{
  int x;
  int y;
} Position;

// Variáveis globais
ssd1306_t ssd;
uint8_t game_state = MENU_STATE;
bool pencil_active = false;

// Variáveis do Snake
Position snake[MAX_SNAKE_LENGTH];
Position food;
int snake_length;
int direction; // 0=direita, 1=baixo, 2=esquerda, 3=cima
bool game_over;

// Adicione estas variáveis globais após as outras
uint8_t square_x = 0;
uint8_t square_y = 0;
uint8_t display_buffer[WIDTH * HEIGHT / 8]; // Buffer para armazenar o desenho

// Adicione após as outras inicializações
void gpio_callback(uint gpio, uint32_t events)
{
  if (gpio == Botao_A && game_state == PAINT_STATE)
  {
    ssd1306_fill(&ssd, false);
    memset(display_buffer, 0, sizeof(display_buffer));
    ssd1306_send_data(&ssd);
  }
}

void init_snake()
{
  snake_length = 3;
  direction = 0;
  game_over = false;

  // Posição inicial da cobra
  for (int i = 0; i < snake_length; i++)
  {
    snake[i].x = WIDTH / 2 - (i * SNAKE_SIZE);
    snake[i].y = HEIGHT / 2;
  }

  // Posição inicial da comida
  food.x = (rand() % (WIDTH - FOOD_SIZE));
  food.y = (rand() % (HEIGHT - FOOD_SIZE));
}

void spawn_food()
{
  food.x = (rand() % (WIDTH - FOOD_SIZE));
  food.y = (rand() % (HEIGHT - FOOD_SIZE));
}

bool check_collision()
{
  // Colisão com as paredes
  if (snake[0].x < 0 || snake[0].x >= WIDTH ||
      snake[0].y < 0 || snake[0].y >= HEIGHT)
  {
    return true;
  }

  // Colisão com o próprio corpo
  for (int i = 1; i < snake_length; i++)
  {
    if (snake[0].x == snake[i].x && snake[0].y == snake[i].y)
    {
      return true;
    }
  }
  return false;
}

void update_snake()
{
  // Salva a posição anterior
  Position prev_positions[MAX_SNAKE_LENGTH];
  for (int i = 0; i < snake_length; i++)
  {
    prev_positions[i] = snake[i];
  }

  // Atualiza a cabeça baseado na direção
  switch (direction)
  {
  case 0:
    snake[0].x += SNAKE_SIZE;
    break; // Direita
  case 1:
    snake[0].y += SNAKE_SIZE;
    break; // Baixo
  case 2:
    snake[0].x -= SNAKE_SIZE;
    break; // Esquerda
  case 3:
    snake[0].y -= SNAKE_SIZE;
    break; // Cima
  }

  // Atualiza o resto do corpo
  for (int i = 1; i < snake_length; i++)
  {
    snake[i] = prev_positions[i - 1];
  }

  // Verifica colisão com a comida
  if (abs(snake[0].x - food.x) < SNAKE_SIZE &&
      abs(snake[0].y - food.y) < SNAKE_SIZE)
  {
    if (snake_length < MAX_SNAKE_LENGTH)
    {
      snake[snake_length] = prev_positions[snake_length - 1];
      snake_length++;
    }
    spawn_food();
  }

  // Verifica colisão com paredes ou próprio corpo
  if (check_collision())
  {
    game_over = true;
  }
}

void mostrar_selecao_jogo(const char *jogo)
{
  ssd1306_fill(&ssd, false);
  ssd1306_draw_string(&ssd, "JOGO SELECIONADO:", 5, 10);
  ssd1306_draw_string(&ssd, jogo, 30, 30);
  ssd1306_send_data(&ssd);
  sleep_ms(3000);
  ssd1306_fill(&ssd, false);
  ssd1306_send_data(&ssd);
}

int main()
{
  stdio_init_all();

  // Configuração dos botões
  gpio_init(Botao_B);
  gpio_set_dir(Botao_B, GPIO_IN);
  gpio_pull_up(Botao_B);

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
  ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT);
  ssd1306_config(&ssd);
  ssd1306_fill(&ssd, false);
  ssd1306_send_data(&ssd);

  // Inicialização do ADC
  adc_init();
  adc_gpio_init(JOYSTICK_X_PIN);
  adc_gpio_init(JOYSTICK_Y_PIN);

  // Posição inicial do cursor
  square_x = WIDTH / 2;
  square_y = HEIGHT / 2;

  // Configuração das interrupções
  gpio_set_irq_enabled_with_callback(Botao_A, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);

  // Adicione após as outras inicializações
  gpio_init(LED_VERDE);
  gpio_init(LED_VERMELHO);
  gpio_set_dir(LED_VERDE, GPIO_OUT);
  gpio_set_dir(LED_VERMELHO, GPIO_OUT);
  gpio_put(LED_VERDE, 0);
  gpio_put(LED_VERMELHO, 0);

  // Limpa o buffer de desenho
  memset(display_buffer, 0, sizeof(display_buffer));

  while (true)
  {
    adc_select_input(0);
    uint16_t adc_value_y = adc_read();
    adc_select_input(1);
    uint16_t adc_value_x = adc_read();

    if (game_state == MENU_STATE)
    {
      static uint8_t opcao_menu = 0;

      ssd1306_fill(&ssd, false);
      ssd1306_draw_string(&ssd, "SELECIONE O JOGO:", 5, 10);

      if (opcao_menu == 0)
      {
        ssd1306_draw_string(&ssd, ">", 20, 30);
        ssd1306_draw_string(&ssd, "PAINT", 30, 30);
        ssd1306_draw_string(&ssd, " ", 20, 45);
        ssd1306_draw_string(&ssd, "SNAKE", 30, 45);
      }
      else
      {
        ssd1306_draw_string(&ssd, " ", 20, 30);
        ssd1306_draw_string(&ssd, "PAINT", 30, 30);
        ssd1306_draw_string(&ssd, ">", 20, 45);
        ssd1306_draw_string(&ssd, "SNAKE", 30, 45);
      }

      ssd1306_send_data(&ssd);

      // Navegação do menu
      if (adc_value_y > 3000 && opcao_menu > 0)
      {
        opcao_menu = 0;
        sleep_ms(200);
      }
      if (adc_value_y < 1000 && opcao_menu < 1)
      {
        opcao_menu = 1;
        sleep_ms(200);
      }

      // Seleção do jogo apenas quando o botão do joystick for pressionado
      if (gpio_get(JOYSTICK_PB) == 0)
      {
        if (opcao_menu == 0)
        {
          mostrar_selecao_jogo("PAINT");
          game_state = PAINT_STATE;
          // Reinicializa o buffer e a posição do cursor
          memset(display_buffer, 0, sizeof(display_buffer));
          square_x = WIDTH / 2;
          square_y = HEIGHT / 2;
        }
        else
        {
          mostrar_selecao_jogo("SNAKE");
          game_state = SNAKE_STATE;
          init_snake();
        }
        sleep_ms(500);
      }
    }
    else if (game_state == PAINT_STATE)
    {
      // Movimento do cursor
      if (adc_value_x > 3000)
      {
        square_x++;
        if (square_x >= WIDTH)
          square_x = WIDTH - 1;
      }
      if (adc_value_x < 1000)
      {
        square_x--;
        if (square_x < 0)
          square_x = 0;
      }
      if (adc_value_y > 3000)
      {
        square_y--;
        if (square_y < 0)
          square_y = 0;
      }
      if (adc_value_y < 1000)
      {
        square_y++;
        if (square_y >= HEIGHT)
          square_y = HEIGHT - 1;
      }

      // Não limpa a tela, apenas desenha na posição atual
      ssd1306_rect(&ssd, square_y, square_x, 2, 2, true, true);
      ssd1306_send_data(&ssd);

      sleep_ms(10);
    }
    else if (game_state == SNAKE_STATE)
    {
      if (!game_over)
      {
        // Controle da direção
        if (adc_value_x > 3000 && direction != 2)
          direction = 0; // Direita
        else if (adc_value_x < 1000 && direction != 0)
          direction = 2; // Esquerda
        if (adc_value_y > 3000 && direction != 1)
          direction = 3; // Cima
        else if (adc_value_y < 1000 && direction != 3)
          direction = 1; // Baixo

        update_snake();

        // Desenha o jogo
        ssd1306_fill(&ssd, false);

        // Desenha a cobra
        for (int i = 0; i < snake_length; i++)
        {
          ssd1306_rect(&ssd, snake[i].y, snake[i].x, SNAKE_SIZE, SNAKE_SIZE, true, true);
        }

        // Desenha a comida
        ssd1306_rect(&ssd, food.y, food.x, FOOD_SIZE, FOOD_SIZE, true, true);

        ssd1306_send_data(&ssd);
      }
      else
      {
        // Tela de Game Over
        ssd1306_fill(&ssd, false);
        ssd1306_draw_string(&ssd, "GAME OVER", 30, 20);
        ssd1306_draw_string(&ssd, "PONTOS:", 30, 35);
        char score[10];
        sprintf(score, "%d", snake_length - 3);
        ssd1306_draw_string(&ssd, score, 80, 35);
        ssd1306_send_data(&ssd);

        // Reinicia o jogo ao pressionar o botão
        if (gpio_get(Botao_B) == 0)
        {
          init_snake();
          sleep_ms(500);
        }
      }
      sleep_ms(100); // Controla a velocidade do jogo
    }

    sleep_ms(10);
  }

  return 0;
}