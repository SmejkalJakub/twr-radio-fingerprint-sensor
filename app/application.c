#include <application.h>

// LED instance
bc_led_t led;

// Button instance
bc_button_t button;

bc_fingerprint_t fingerprint;

bc_module_relay_t relay;

bc_module_pir_t pir;

bool sensor_on = false;

bool enroll = false;

bc_scheduler_task_id_t deinit_task;

static const bc_radio_sub_t subs[] = {
    {"finger/-/start/enroll", BC_RADIO_SUB_PT_BOOL, get_enroll_state, (void *) NULL},
};


void button_event_handler(bc_button_t *self, bc_button_event_t event, void *event_param)
{
    if(event == BC_BUTTON_EVENT_HOLD)
    {
        bc_fingerprint_delete_database(&fingerprint);
    }

}

void lcd_event_handler(bc_module_lcd_event_t event, void *event_param)
{
    if(event == BC_MODULE_LCD_EVENT_LEFT_CLICK)
    {
        bc_fingerprint_enroll(&fingerprint);
    }
    else if(event == BC_MODULE_LCD_EVENT_RIGHT_CLICK)
    {
        bc_module_relay_set_state(&relay, true);
        bc_fingerprint_read_finger(&fingerprint);
    }
}

void fingerprint_event_handler(bc_fingerprint_t *self, bc_fingerprint_event_t event, void *event_param)
{
    if(event == BC_FINGERPRINT_EVENT_FINGERPRINT_MATCHED)
    {
        bc_radio_pub_int("finger/-/id", &event_param);
    }
    else if(event == BC_FINGERPRINT_EVENT_ENROLL_FIRST)
    {
        bc_led_blink(&led, 2);
    }
    else if(event == BC_FINGERPRINT_EVENT_ENROLL_SECOND)
    {
        bc_led_blink(&led, 2);
    }

    else if(event == BC_FINGERPRINT_EVENT_DATABASE_DELETED)
    {
        bc_log_debug("deleted");
    }

    else if(event == BC_FINGERPRINT_EVENT_UPDATE)
    {
        bc_scheduler_plan_now(deinit_task);
    }
}

void get_enroll_state(uint64_t *id, const char *topic, void *value, void *param)
{
    int _value = *(int *)value;

    if(_value)
    {
        bc_fingerprint_enroll(&fingerprint);
    }
}

void finger_sensor_deinit()
{
    bc_fingerprint_deinit(&fingerprint);
    
    bc_module_relay_set_state(&relay, true);

    sensor_on = false;
}

void pir_event_handler(bc_module_pir_t *self, bc_module_pir_event_t event, void *event_param)
{
    (void) self;
    (void) event_param;

    if (event == BC_MODULE_PIR_EVENT_MOTION && !sensor_on)
    {
        bc_radio_pub_int("enroll/request", NULL);

        sensor_on = true;
        bc_module_relay_set_state(&relay, false);

        bc_fingerprint_init(&fingerprint, BC_UART_UART0);
        bc_fingerprint_set_update_interval(&fingerprint, 1000);
        bc_fingerprint_set_event_handler(&fingerprint, fingerprint_event_handler, NULL);

        if(enroll)
        {
            bc_fingerprint_enroll(&fingerprint);
        }

        bc_scheduler_plan_from_now(deinit_task, 20000);
    }
}


void application_init(void)
{
    // Initialize logging
    bc_log_init(BC_LOG_LEVEL_DUMP, BC_LOG_TIMESTAMP_ABS);

    // Initialize LED
    bc_led_init(&led, BC_GPIO_LED, false, false);
    bc_led_set_mode(&led, BC_LED_MODE_ON);

    // Initialize button
    bc_button_init(&button, BC_GPIO_BUTTON, BC_GPIO_PULL_DOWN, false);
    bc_button_set_event_handler(&button, button_event_handler, NULL);

    bc_module_relay_init(&relay, 0x3B);

    bc_module_pir_init(&pir);
    bc_module_pir_set_sensitivity(&pir, BC_MODULE_PIR_SENSITIVITY_MEDIUM);
    bc_module_pir_set_event_handler(&pir, pir_event_handler, NULL);

    bc_radio_init(BC_RADIO_MODE_NODE_SLEEPING);
    bc_radio_set_subs((bc_radio_sub_t *) subs, sizeof(subs)/sizeof(bc_radio_sub_t));
    bc_radio_set_rx_timeout_for_sleeping_node(200);
    bc_radio_pairing_request("fingerprint-sensor", VERSION);

    bc_system_pll_enable();

    deinit_task = bc_scheduler_register(finger_sensor_deinit, NULL, BC_TICK_INFINITY); 
}


