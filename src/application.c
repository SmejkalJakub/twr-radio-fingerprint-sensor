#include <application.h>

// LED instance
twr_led_t led;

// Button instance
twr_button_t button;

twr_fingerprint_t fingerprint;

twr_module_relay_t relay;

twr_module_pir_t pir;

bool sensor_on = false;

bool enroll = false;

twr_scheduler_task_id_t deinit_task;

static const twr_radio_sub_t subs[] = {
    {"finger/-/start/enroll", TWR_RADIO_SUB_PT_BOOL, get_enroll_state, (void *) NULL},
};


void button_event_handler(twr_button_t *self, twr_button_event_t event, void *event_param)
{
    if(event == TWR_BUTTON_EVENT_HOLD)
    {
        twr_fingerprint_delete_database(&fingerprint);
    }

}

void lcd_event_handler(twr_module_lcd_event_t event, void *event_param)
{
    if(event == TWR_MODULE_LCD_EVENT_LEFT_CLICK)
    {
        twr_fingerprint_enroll(&fingerprint);
    }
    else if(event == TWR_MODULE_LCD_EVENT_RIGHT_CLICK)
    {
        twr_module_relay_set_state(&relay, true);
        twr_fingerprint_read_finger(&fingerprint);
    }
}

void fingerprint_event_handler(twr_fingerprint_t *self, twr_fingerprint_event_t event, void *event_param)
{
    if(event == TWR_FINGERPRINT_EVENT_FINGERPRINT_MATCHED)
    {
        twr_radio_pub_int("finger/-/id", &event_param);
    }
    else if(event == TWR_FINGERPRINT_EVENT_ENROLL_FIRST)
    {
        twr_led_blink(&led, 2);
    }
    else if(event == TWR_FINGERPRINT_EVENT_ENROLL_SECOND)
    {
        twr_led_blink(&led, 2);
    }

    else if(event == TWR_FINGERPRINT_EVENT_DATABASE_DELETED)
    {
        twr_log_debug("deleted");
    }

    else if(event == TWR_FINGERPRINT_EVENT_UPDATE)
    {
        twr_scheduler_plan_now(deinit_task);
    }
}

void get_enroll_state(uint64_t *id, const char *topic, void *value, void *param)
{
    int _value = *(int *)value;

    if(_value)
    {
        twr_fingerprint_enroll(&fingerprint);
    }
}

void finger_sensor_deinit()
{
    twr_fingerprint_deinit(&fingerprint);

    twr_module_relay_set_state(&relay, true);

    sensor_on = false;
}

void pir_event_handler(twr_module_pir_t *self, twr_module_pir_event_t event, void *event_param)
{
    (void) self;
    (void) event_param;

    if (event == TWR_MODULE_PIR_EVENT_MOTION && !sensor_on)
    {
        twr_radio_pub_int("enroll/request", NULL);

        sensor_on = true;
        twr_module_relay_set_state(&relay, false);

        twr_fingerprint_init(&fingerprint, TWR_UART_UART0);
        twr_fingerprint_set_update_interval(&fingerprint, 1000);
        twr_fingerprint_set_event_handler(&fingerprint, fingerprint_event_handler, NULL);

        if(enroll)
        {
            twr_fingerprint_enroll(&fingerprint);
        }

        twr_scheduler_plan_from_now(deinit_task, 20000);
    }
}


void application_init(void)
{
    // Initialize logging
    twr_log_init(TWR_LOG_LEVEL_DUMP, TWR_LOG_TIMESTAMP_ABS);

    // Initialize LED
    twr_led_init(&led, TWR_GPIO_LED, false, false);
    twr_led_set_mode(&led, TWR_LED_MODE_ON);

    // Initialize button
    twr_button_init(&button, TWR_GPIO_BUTTON, TWR_GPIO_PULL_DOWN, false);
    twr_button_set_event_handler(&button, button_event_handler, NULL);

    twr_module_relay_init(&relay, 0x3B);

    twr_module_pir_init(&pir);
    twr_module_pir_set_sensitivity(&pir, TWR_MODULE_PIR_SENSITIVITY_MEDIUM);
    twr_module_pir_set_event_handler(&pir, pir_event_handler, NULL);

    twr_radio_init(TWR_RADIO_MODE_NODE_SLEEPING);
    twr_radio_set_subs((twr_radio_sub_t *) subs, sizeof(subs)/sizeof(twr_radio_sub_t));
    twr_radio_set_rx_timeout_for_sleeping_node(200);
    twr_radio_pairing_request("fingerprint-sensor", VERSION);

    twr_system_pll_enable();

    deinit_task = twr_scheduler_register(finger_sensor_deinit, NULL, TWR_TICK_INFINITY);
}


