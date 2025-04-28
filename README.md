## Projeto: Ohmímetro com Raspberry Pi Pico

Este projeto cria um ohmímetro digital usando o Raspberry Pi Pico, um display OLED e uma matriz de LEDs WS2812.

### O que o projeto faz:

- Mede a resistência de um resistor desconhecido usando um divisor de tensão.

- Mostra no display OLED:

    - O valor do ADC.

    - A resistência calculada.

    - As faixas de cor do resistor.

- Acende LEDs WS2812 para mostrar as cores das faixas do resistor.

### Componentes usados

- Raspberry Pi Pico

- Display OLED 128x64 (SSD1306) via I2C

- Matriz de LEDs WS2812 (25 LEDs)

- Resistor fixo de 10kΩ

- Fios e protoboard

### Ligações principais

| Componente | Pino no Pico |
|------------|--------------|
| Display SDA |	GP14 |
| Display SCL |	GP15 |
| WS2812 Data IN |	GP7 |
| Resistores |	GP28 (ADC2) |

### Como funciona

- O Pico lê o valor analógico do divisor de tensão.

- Converte para resistência.

- Escolhe o valor comercial mais próximo (ex.: 220Ω, 470Ω).

- Mostra no display as informações.

- Acende os LEDs com as cores certas das faixas do resistor.

### Observação

- Foram usados 3 LEDs para representar as 3 primeiras faixas.

- O sistema lê uma média de 1000 amostras para melhorar a precisão.