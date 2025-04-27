#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "lib/ssd1306.h"
#include "lib/font.h"
#include "lib/ws2812.pio.h"

// Definições do display
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C

// GPIO para a matriz de LEDs
#define LED_MATRIX 7
#define LED_COUNT 25 // Número de LEDs na matriz

// Definição de pixel GRB
typedef struct pixel_t {
    uint8_t G, R, B; 
} npLed_t;

npLed_t leds[LED_COUNT]; // Array de LEDs

PIO pio = pio0;
uint sm = 0;

#define ADC_PIN 28 // GPIO para o adc do pino 28
#define ADC_RESOLUTION 4095 // Resolução do ADC (12 bits)
#define RESISTOR 10000   // Resistor de 10k ohm

#define STRING_SIZE 7 // Tamanho máximo da string -> 6 dígitos + '\0'
char ADC[STRING_SIZE]; // Buffer para armazenar a string 
char RES[STRING_SIZE]; // Buffer para armazenar a string

char FAIXAS[3][8] = {" ", " ", " "}; // Faixas de cores
int color[3] = {0, 0, 0}; // Cores das faixas


float adc_to_resistor(uint16_t adc_value) {
    // Converte o valor do ADC para resistência usando a fórmula do divisor de tensão
    float R_x = (RESISTOR * adc_value) / (ADC_RESOLUTION - adc_value);
    return R_x;
}

void display_values(ssd1306_t *ssd, uint32_t adc_value, float R_x) {
    // Limpa o display
    ssd1306_fill(ssd, false); // Limpa o display
    ssd1306_send_data(ssd); // Envia os dados para o display

    // Converte os valores para string
    snprintf(ADC, STRING_SIZE, "%d", adc_value); // Converte o valor do ADC para string
    snprintf(RES, STRING_SIZE, "%.0f", R_x); // Converte o valor da resistência para string

    // Desenha os valores no display
    ssd1306_rect(ssd, 0, 0, WIDTH, HEIGHT, true, false); // Desenha um retângulo ao redor do display
    ssd1306_draw_string(ssd, "Ohmimetro", 28, 2); // Desenha o texto "Ohmimetro" no display
    ssd1306_hline(ssd, 0, WIDTH - 1, 10, true); // Desenha uma linha horizontal no display

    ssd1306_draw_string(ssd, "ADC", 5, 15); // Desenha o texto "ADC" no display
    ssd1306_draw_string(ssd, ADC, 5, 25); // Desenha o valor do ADC no display
    ssd1306_hline(ssd, 0, 64, 35, true); // Desenha uma linha horizontal no display

    ssd1306_draw_string(ssd, "RES", 5, 40); // Desenha o texto "RES" no display
    ssd1306_draw_string(ssd, RES, 5, 52); // Desenha o valor da resistência no display

    ssd1306_vline(ssd, 64, 10, HEIGHT - 1, true); // Desenha uma linha vertical no display

    ssd1306_draw_string(ssd, FAIXAS[0], 67, 15); // Desenha o texto "Faixa" no display
    ssd1306_draw_string(ssd, FAIXAS[1], 67, 33); // Desenha o texto "Faixa" no display
    ssd1306_draw_string(ssd, FAIXAS[2], 67, 50); // Desenha o texto "Faixa" no display
    
    // Envia os dados para o display
    ssd1306_send_data(ssd);
}

void color_table(int value) {
    const char *colors[] = {"PRETO", "MARROM", "VERMLHO", "LARANJA", "AMARELO", "VERDE", "AZUL", "VIOLETA", "CINZA", "BRANCO"};
    int first, second, multiplier = 0;

    // define o multiplicador de acordo com quantas vezes o valor é dividido por 10
    while (value >= 100) {
        value /= 10;
        multiplier++;
    }

    first = value / 10; // Primeiro dígito
    second = value % 10; // Segundo dígito

    strcpy(FAIXAS[0], colors[first]); // Copia a cor correspondente ao primeiro dígito para FAIXAS[0]
    strcpy(FAIXAS[1], colors[second]); // Copia a cor correspondente ao segundo dígito para FAIXAS[1]
    strcpy(FAIXAS[2], colors[multiplier]); // Copia a cor correspondente ao multiplicador para FAIXAS[2]

    // Atualiza as cores das faixas
    color[0] = first;
    color[1] = second;
    color[2] = multiplier;
}

int comercial_value(float R_x) {
    // Valores da serie comercial de resistores E24
    const int commercial_values[] = {
        10, 11, 12, 13, 15, 16, 18, 20, 22, 24, 27, 30,
        33, 36, 39, 43, 47, 51, 56, 62, 68, 75, 82, 91
    };

    // Converte o valor de resistência para inteiro e calcula o multiplicador
    int value = (int) R_x, multiplier = 1;
    while (value >= 100) {
        value /= 10;
        multiplier *= 10;
    }

    // Encontra o valor comercial mais próximo
    int closest_value = commercial_values[0];
    int min_diff = fabs(value - closest_value);

    // Compara o valor medido com os valores comerciais atualizando o mais próximo
    for (int i = 0; i < 24; i++) {
        int diff = fabs(value - commercial_values[i]);
        
        if (diff < min_diff) {
            min_diff = diff;
            closest_value = commercial_values[i]; // Atualiza o valor mais próximo
        }
    }

    return closest_value * multiplier; // Retorna o valor comercial mais próximo
}

void npSetLED(const uint index, const uint8_t r, const uint8_t g, const uint8_t b) {
    leds[index].R = r;
    leds[index].G = g;
    leds[index].B = b;
}

void npWrite() {
    // Escreve cada dado de 8-bits dos pixels em sequência no buffer da máquina PIO.
    for (uint i = 0; i < LED_COUNT; ++i) {
      pio_sm_put_blocking(pio, sm, leds[i].G);
      pio_sm_put_blocking(pio, sm, leds[i].R);
      pio_sm_put_blocking(pio, sm, leds[i].B);
    }
    sleep_us(100); // Espera 100us, sinal de RESET do datasheet.
  }
  

void show_colors() {
    for (uint i = 0; i < 25; i++){
        leds[i].G = 0; // Zera o valor do verde
        leds[i].R = 0; // Zera o valor do vermelho
        leds[i].B = 0; // Zera o valor do azul
    }

    // Define as cores de acordo com a faixa (formato GRB)
    npLed_t colors[10] = {
        {0,   0,  0},    // Preto 
        {8,  12,  0},    // Marrom --> tentei o mais perto possivel do marrom
        {0,  40,  0},    // Vermelho  
        {8,   4,  0},    // Laranja
        {20, 20,  0},    // Amarelo 
        {40,  0,  0},    // Verde 
        {0,   0, 40},    // Azul 
        {0,  20, 40},    // Violeta 
        {20, 20, 20},    // Cinza
        {1,   1,  1}     // Branco
    };

    // Define a cor de acordo com a faixa
    npSetLED(13, colors[color[0]].R, colors[color[0]].G, colors[color[0]].B); // Cor da primeira faixa
    npSetLED(12, colors[color[1]].R, colors[color[1]].G, colors[color[1]].B); // Cor da segunda faixa
    npSetLED(11, colors[color[2]].R, colors[color[2]].G, colors[color[2]].B); // Cor da terceira faixa

    npWrite(); // Envia os dados para a matriz de LEDs
}

int main() {   
    stdio_init_all();

    // I2C Initialisation. Using it at 400Khz.
    i2c_init(I2C_PORT, 400 * 1000);

    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);                    // Set the GPIO pin function to I2C
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);                    // Set the GPIO pin function to I2C
    gpio_pull_up(I2C_SDA);                                        // Pull up the data line
    gpio_pull_up(I2C_SCL);                                        // Pull up the clock line
    ssd1306_t ssd;                                                // Inicializa a estrutura do display
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display
    ssd1306_config(&ssd);                                         // Configura o display
    ssd1306_send_data(&ssd);                                      // Envia os dados para o display

    // Limpa o display. O display inicia com todos os pixels apagados.
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    adc_init();
    adc_gpio_init(ADC_PIN); // GPIO 28 como entrada analógica

    // WS2812
    uint offset = pio_add_program(pio, &ws2818b_program);
    ws2818b_program_init(pio, sm, offset, LED_MATRIX, 800000.f);

    while (true) {
        adc_select_input(2); // Seleciona o ADC para eixo X. O pino 28 como entrada analógica

        // Calcula uma média de 1000 leituras do ADC
        uint32_t adc_value = 0;
        for (int i = 0; i < 1000; i++) {
            adc_value += adc_read();
        }
        adc_value /= 1000;

        // Converte o valor do ADC para resistência
        float R_x = adc_to_resistor(adc_value);
        int commercial_value = comercial_value(R_x); // Calcula o valor comercial da resistência
        
        color_table(commercial_value); // Atualiza as faixas de cores com base na resistência comercial mais próxima
        display_values(&ssd, adc_value, R_x); // Atualiza o display com os valores
        show_colors(commercial_value); // Atualiza a cor da matriz de LEDs com base na resistência comercial mais próxima

        printf("ADC MEDIDO: %d\n", adc_value); // Imprime o valor do ADC
        printf("RESISTENCIA ENCONTRADA: %.2f\n", R_x); // Imprime o valor da resistência
        printf("VALOR COMERCIAL: %d\n", commercial_value); // Imprime o valor comercial da resistência

        sleep_ms(700);
    }
}