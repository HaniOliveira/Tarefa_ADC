# Tarefa ADC

Este projeto implementa um sistema interativo usando o Raspberry Pi Pico, que integra um joystick analógico, LEDs RGB, botões e um display OLED.

## Funcionalidades

- Controle de um quadrado no display OLED usando joystick analógico
- LEDs RGB que respondem à posição do joystick
- Três estilos de borda no display (sem borda, borda simples, borda dupla)
- Controles por botões:
  - Botão A: Liga/desliga os LEDs RGB
  - Botão do Joystick: Alterna o LED verde e o estilo da borda

## Componentes Utilizados

- Raspberry Pi Pico
- Display OLED SSD1306
- Joystick analógico
- LEDs RGB
- 2 botões (A e B)
- No meu caso, utilizei a ferramenta educacional BitDogLab

## Pinagem

- I2C Display OLED:
  - SDA: GPIO 14
  - SCL: GPIO 15
- Joystick:
  - Eixo X: GPIO 27
  - Eixo Y: GPIO 26
  - Botão: GPIO 22
- LEDs:
  - Vermelho: GPIO 13
  - Azul: GPIO 12
  - Verde: GPIO 11
- Botões:
  - Botão A: GPIO 5
  - Botão B: GPIO 6

## Como Usar

1. Compile e carregue o código para o Raspberry Pi Pico
2. Use o joystick para mover o quadrado no display
3. Pressione os botões para ativar suas respectivas funções

## Observações

- O sistema utiliza PWM para controle dos LEDs RGB
- O display OLED opera via I2C na frequência de 400kHz
- Implementado sistema de debounce para os botões

## Link do Video de demonstração

https://drive.google.com/file/d/1T0sh0u-YXKgan1dAGr1MkCN40XTM6Slq/view?usp=sharing

OBS: O Led vermelho está aceso bem fraquinho pois o eixo X as vezes trava e não volta 100% ao centro, tentei ajusta a função PWM mas a variação era muito grande e estava comprometendo a intensidade do lED.
# Projeto_Final
