// Developed by: E. J. Santos Jr.
// Contact: e.joaojr@gmail.com

#include <stdio.h>
#include <string.h>
#include "esp_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "soc/timer_group_struct.h"
#include "driver/periph_ctrl.h"
#include "driver/timer.h"
#include "driver/gpio.h"

#include "driver/uart.h"

#define TIMER_DIVIDER         16  //  Hardware timer clock divider
#define TIMER_SCALE           (TIMER_BASE_CLK / TIMER_DIVIDER)  // convert counter value to seconds
#define TIMER_INTERVAL0_SEC   (10) // sample test interval for the timer
#define TEST_WITHOUT_RELOAD   0   // testing will be done without auto reload
#define TEST_WITH_RELOAD      1   // testing will be done with auto reload

#define ECHO_TEST_TXD  (GPIO_NUM_17)
#define ECHO_TEST_RXD  (GPIO_NUM_16)
#define ECHO_TEST_RTS  (UART_PIN_NO_CHANGE)
#define ECHO_TEST_CTS  (UART_PIN_NO_CHANGE)

#define BUF_SIZE (1024)

/*
 * A sample structure to pass events
 * from the timer interrupt handler to the main program.
 */
typedef struct {
    int type;  // the type of timer's event
    int timer_group;
    int timer_idx;
    uint64_t timer_counter_value;
} timer_event_t;

xQueueHandle timer_queue;

/*
 * A simple helper function to print the raw timer counter value
 * and the counter value converted to seconds
 */
static void inline print_timer_counter(uint64_t counter_value) {
    printf("Time   : %.8f s\n", (double) counter_value / TIMER_SCALE);
}

/*
 * Timer group0 ISR handler
 *
 * Note:
 * We don't call the timer API here because they are not declared with IRAM_ATTR.
 * If we're okay with the timer irq not being serviced while SPI flash cache is disabled,
 * we can allocate this interrupt without the ESP_INTR_FLAG_IRAM flag and use the normal API.
 */
void IRAM_ATTR timer_group0_isr(void *para) {

    int timer_idx = (int) para;

    /* Retrieve the interrupt status and the counter value
       from the timer that reported the interrupt */
    uint32_t intr_status = TIMERG0.int_st_timers.val;
    TIMERG0.hw_timer[timer_idx].update = 1;
    uint64_t timer_counter_value =
        ((uint64_t) TIMERG0.hw_timer[timer_idx].cnt_high) << 32
        | TIMERG0.hw_timer[timer_idx].cnt_low;

    /* Prepare basic event data
       that will be then sent back to the main program task */
    timer_event_t evt;
    evt.timer_group = 0;
    evt.timer_idx = timer_idx;
    evt.timer_counter_value = timer_counter_value;

    /* Clear the interrupt
       and update the alarm time for the timer with without reload */
    if ((intr_status & BIT(timer_idx)) && timer_idx == TIMER_0) {
        evt.type = TEST_WITHOUT_RELOAD;
        TIMERG0.int_clr_timers.t0 = 1;
        timer_counter_value += (uint64_t) (TIMER_INTERVAL0_SEC * TIMER_SCALE);
        TIMERG0.hw_timer[timer_idx].alarm_high = (uint32_t) (timer_counter_value >> 32);
        TIMERG0.hw_timer[timer_idx].alarm_low = (uint32_t) timer_counter_value;
    }  else {
        evt.type = -1; // not supported even type
    }

    /* After the alarm has been triggered
      we need enable it again, so it is triggered the next time */
    TIMERG0.hw_timer[timer_idx].config.alarm_en = TIMER_ALARM_EN;

    /* Now just send the event data back to the main program task */
    xQueueSendFromISR(timer_queue, &evt, NULL);
}

/*
 * Initialize selected timer of the timer group 0
 *
 * timer_idx - the timer number to initialize
 * auto_reload - should the timer auto reload on alarm?
 * timer_interval_sec - the interval of alarm to set
 */
static void example_tg0_timer_init(int timer_idx, bool auto_reload, double timer_interval_sec) {
    /* Select and initialize basic parameters of the timer */
    timer_config_t config;
    config.divider = TIMER_DIVIDER;
    config.counter_dir = TIMER_COUNT_UP;
    config.counter_en = TIMER_PAUSE;
    config.alarm_en = TIMER_ALARM_EN;
    config.intr_type = TIMER_INTR_LEVEL;
    config.auto_reload = auto_reload;
    timer_init(TIMER_GROUP_0, timer_idx, &config);

    /* Timer's counter will initially start from value below.
       Also, if auto_reload is set, this value will be automatically reload on alarm */
    timer_set_counter_value(TIMER_GROUP_0, timer_idx, 0x00000000ULL);

    /* Configure the alarm value and the interrupt on alarm. */
    timer_set_alarm_value(TIMER_GROUP_0, timer_idx, timer_interval_sec * TIMER_SCALE);
    timer_enable_intr(TIMER_GROUP_0, timer_idx);
    timer_isr_register(TIMER_GROUP_0, timer_idx, timer_group0_isr,
        (void *) timer_idx, ESP_INTR_FLAG_IRAM, NULL);

    timer_start(TIMER_GROUP_0, timer_idx);
}

static void config_uart() {
    // Configure parameters of an UART driver,  communication pins and install the driver
    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    uart_param_config(UART_NUM_2, &uart_config);
    uart_set_pin(UART_NUM_2, ECHO_TEST_TXD, ECHO_TEST_RXD, ECHO_TEST_RTS, ECHO_TEST_CTS);
    uart_driver_install(UART_NUM_2, BUF_SIZE * 2, 0, 0, NULL, 0);
}

static void read_slave_return() {
	// Configure a temporary buffer for the incoming data
	uint8_t *data = (uint8_t *) malloc(BUF_SIZE);
    // Read data from the UART
	int len = uart_read_bytes(UART_NUM_2, data, BUF_SIZE, 20 / portTICK_RATE_MS);
		        
	char return_message[len+1];
	strncpy(return_message, (char*)data, len);
	return_message[len] = '\0';
	printf("%s\n", return_message);
}

static void init_transmitter() {
		// flush serial
        uart_write_bytes(UART_NUM_2, "AT#", 3);
        vTaskDelay(pdMS_TO_TICKS(1000));
		uint8_t *flush = (uint8_t *) malloc(BUF_SIZE);
        uart_read_bytes(UART_NUM_2, flush, BUF_SIZE, 20 / portTICK_RATE_MS);

        // Initialization commands
        const char *commands[] = {
                                "ATZ#",  // reset init
                                "AT+DR=0#",  // using SF10
                                "AT+TXP=0#", // using max tx power
                                "AT+ADR=1#", // using ADR mechanism
                                "AT+SBAND=1#", // using first subband
                                "AT+RX1DL=1000#", // rx window 1 open during 1 second
                                "AT+RX2DL=2000#", // rx window 2 open during 2 seconds
                                "AT+CFM=0#"  // using unconfirmed messages
                                // ... you can add more
        };

        int i;
        for(i = 0; i < sizeof(commands) / sizeof(int); i++) {
                uart_write_bytes(UART_NUM_2, commands[i], strlen(commands[i]));
                printf("%s\n", commands[i]);

                // delay
                vTaskDelay(pdMS_TO_TICKS(500));
                read_slave_return();
        }
}

/*
 * The main task of this example program
 */
static void timer_evt_task(void *arg) {

    while (1) {
        timer_event_t evt;
        xQueueReceive(timer_queue, &evt, portMAX_DELAY);

        /* Print the timer values passed by event */
        printf("\n------- TIMER INTERRUPT --------\n");
        print_timer_counter(evt.timer_counter_value);

        // Creates packet to be sent
        const char* cmd_init = "AT+SEND=1:";
        const char* cmd_terminator = "#";
        
        int temp_value = rand() % 101 - 50; // generates values in range (-50, +50). this value can be replaced by some real reading from temperature sensor
        int hum_value = rand() % 100; // generates values in range (0, 100). this value can be replaced by some real reading from humidity sensor
        
        char payload[10];
        sprintf(payload, "%d,%d", temp_value, hum_value); // new sensors values can be inserted here

        char* command;
        command = malloc(strlen(cmd_init) + strlen(payload) + 1 + 4);

        strcpy(command, cmd_init);
        strcat(command, payload);
        strcat(command, cmd_terminator);

        // Write data to the UART
        uart_write_bytes(UART_NUM_2, command, strlen(command));
        printf("%s\n", command);
        vTaskDelay(pdMS_TO_TICKS(3000));
        read_slave_return();

        free(command);
    }
}

/*
 * In this example, we will test hardware timer0 and timer1 of timer group0.
 */
void app_main() {
    config_uart();
    init_transmitter();

    timer_queue = xQueueCreate(10,  sizeof(timer_event_t));
    example_tg0_timer_init(TIMER_0, TEST_WITHOUT_RELOAD, TIMER_INTERVAL0_SEC);
    xTaskCreate(timer_evt_task, "timer_evt_task", 2048, NULL, 5, NULL);
}

