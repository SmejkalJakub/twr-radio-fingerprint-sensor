#include <twr_fingerprint.h>

#define _TWR_FINGERPRINT_DELAY_RUN 50
#define _TWR_FINGERPRINT_DELAY_READY 50
#define _TWR_FINGERPRINT_DELAY_INITIALIZATION 1000
#define _TWR_FINGERPRINT_DELAY_START_MEASUREMENT 50
#define _TWR_FINGERPRINT_DELAY_MEASUREMENT 200

static void _twr_fingerprint_task_interval(void *param);

static void _twr_fingerprint_task_measure(void *param);

bool fingerValidation = false;

uint16_t start_code = FINGERPRINT_STARTCODE;      ///< "Wakeup" code for packet detection
uint8_t address[4] = {0xFF, 0xFF, 0xFF, 0xFF}; ///< 32-bit Fingerprint sensor address
uint8_t type = 0x01;             ///< Type of packet

uint16_t id;

bool initializing = false;

twr_uart_channel_t _channel;

#define SEND_CMD_PACKET(...) \
    uint8_t data[] = {__VA_ARGS__};\
    twr_fingerprint_write_packet(data, sizeof(data));

void twr_fingerprint_init(twr_fingerprint_t *self, twr_uart_channel_t channel)
{
    memset(self, 0, sizeof(*self));

    self->_channel = channel;
    self->_enroll = false;
    self->_password = 0x0;
    channel = _channel;

    self->_task_id_interval = twr_scheduler_register(_twr_fingerprint_task_interval, self, TWR_TICK_INFINITY);
    self->_task_id_measure = twr_scheduler_register(_twr_fingerprint_task_measure, self, _TWR_FINGERPRINT_DELAY_RUN);

    self->_tick_ready = _TWR_FINGERPRINT_DELAY_RUN;

    twr_uart_init(self->_channel, TWR_UART_BAUDRATE_57600, TWR_UART_SETTING_8N1);

    twr_fifo_init(&(self->_read_fifo), self->_read_fifo_buffer, sizeof(self->_read_fifo_buffer));

    twr_uart_set_async_fifo(self->_channel, NULL, &(self->_read_fifo));

    twr_uart_async_read_start(self->_channel, 1000);

}

void twr_fingerprint_deinit(twr_fingerprint_t *self)
{
    twr_uart_deinit(self->_channel);

    twr_scheduler_unregister(self->_task_id_interval);

    twr_scheduler_unregister(self->_task_id_measure);
}

void twr_fingerprint_set_event_handler(twr_fingerprint_t *self, void (*event_handler)(twr_fingerprint_t *, twr_fingerprint_event_t, void *), void *event_param)
{
    self->_event_handler = event_handler;
    self->_event_param = event_param;
}

void twr_fingerprint_set_update_interval(twr_fingerprint_t *self, twr_tick_t interval)
{
    self->_update_interval = interval;

    if (self->_update_interval == TWR_TICK_INFINITY)
    {
        twr_scheduler_plan_absolute(self->_task_id_interval, TWR_TICK_INFINITY);
    }
    else
    {
        twr_scheduler_plan_relative(self->_task_id_interval, self->_update_interval);

        twr_fingerprint_measure(self);
    }
}

void twr_fingerprint_enroll(twr_fingerprint_t *self)
{
    self->_enroll = true;
    //self->_measurement_active = false;
}

uint8_t twr_fingerprint_check_response(twr_fingerprint_t *self)
{
    if(self->_buffer[0] == (uint8_t)(FINGERPRINT_STARTCODE >> 8) && self->_buffer[1] == (uint8_t)(FINGERPRINT_STARTCODE & 0xFF))
        {
            if(self->_buffer[6] == FINGERPRINT_ACKPACKET)
            {
                return self->_buffer[9];
            }
        }
    return FINGERPRINT_PACKETRECIEVEERR;
}

void twr_fingerprint_read_finger(twr_fingerprint_t *self)
{
    self->_state = TWR_FINGERPRINT_STATE_READY;
    self->_enroll = false;
    self->_measurement_active = false;
}

bool twr_fingerprint_measure(twr_fingerprint_t *self)
{
    if (self->_measurement_active)
    {
        return false;
    }

    self->_measurement_active = true;

    twr_scheduler_plan_absolute(self->_task_id_measure, self->_tick_ready);

    return true;
}

void twr_fingerprint_delete_database(twr_fingerprint_t *self)
{
    self->_delete = true;
}

void twr_fingerprint_get_template_count(twr_fingerprint_t *self)
{
    self->_measurement_active = false;

    self->_state = TWR_FINGERPRINT_STATE_GET_TEMPLATE_COUNT;
}


bool twr_fingerprint_read_data(twr_fingerprint_t *self)
{
    self->_length = twr_uart_async_read(self->_channel, self->_buffer, sizeof(self->_buffer));


    if (self->_length < 3)
    {
        return false;
    }
    self->_response = twr_fingerprint_check_response(self);

    return true;
}

bool twr_fingerprint_write_packet(void *data, size_t length)
{
    uint8_t i = 0;
    uint16_t wire_length = length + 2;
    uint16_t sum = (wire_length >> 8) + (wire_length & 0xFF) + type;
    uint8_t packet[32];
    packet[0] = start_code >> 8;
    packet[1] = start_code;
    packet[2] = address[0];
    packet[3] = address[1];
    packet[4] = address[2];
    packet[5] = address[3];
    packet[6] = type;
    packet[7] = wire_length >> 8;
    packet[8] = wire_length;
    for (i = 0; i < length; i++)
    {
        packet[9 + i] = ((uint8_t *) data)[i];

        sum += ((uint8_t *) data)[i];
    }
    packet[9 + i++] = sum >> 8;
    packet[9 + i++] = sum;

    twr_uart_write(_channel, packet, 9 + i);

    return true;
}

static void _twr_fingerprint_task_interval(void *param)
{
    twr_fingerprint_t *self = param;

    twr_fingerprint_measure(self);

    twr_scheduler_plan_current_relative(self->_update_interval);
}


static void _twr_fingerprint_task_measure(void *param)
{
    twr_fingerprint_t *self = param;

    twr_log_debug("%d", self->_enroll);

start:
    switch (self->_state)
    {
        case TWR_FINGERPRINT_STATE_ERROR:
        {
            self->_measurement_active = false;

            if (self->_event_handler != NULL)
            {
                self->_event_handler(self, TWR_FINGERPRINT_EVENT_ERROR, self->_event_param);
            }

            self->_state = TWR_FINGERPRINT_STATE_INITIALIZE;

            return;
        }
        case TWR_FINGERPRINT_STATE_INITIALIZE:
        {
            self->_state = TWR_FINGERPRINT_STATE_ERROR;

            SEND_CMD_PACKET(FINGERPRINT_VERIFYPASSWORD, (uint8_t)(self->_password >> 24), (uint8_t)(self->_password >> 16),
                  (uint8_t)(self->_password >> 8), (uint8_t)(self->_password & 0xFF));

            self->_state = TWR_FINGERPRINT_STATE_PASSWORD_READ;

            self->_tick_ready = twr_tick_get() + 800;

            if (self->_measurement_active)
            {
                twr_scheduler_plan_current_absolute(self->_tick_ready);
            }

            return;
        }


        case TWR_FINGERPRINT_STATE_PASSWORD_READ:
        {
            if(!twr_fingerprint_read_data(self))
            {
                self->_state = TWR_FINGERPRINT_STATE_INITIALIZE;
                twr_scheduler_plan_current_from_now(_TWR_FINGERPRINT_DELAY_MEASUREMENT);
                return;
            }

            if(self->_response == FINGERPRINT_OK)
            {
                initializing = true;
                self->_state = TWR_FINGERPRINT_STATE_GET_TEMPLATE_COUNT;
                twr_scheduler_plan_current_from_now(_TWR_FINGERPRINT_DELAY_RUN);
                return;
            }
            else
            {
                return;
            }
        }

        case TWR_FINGERPRINT_STATE_READY:
        {
            if(self->_delete)
            {
                self->_state = TWR_FINGERPRINT_STATE_DELETE_DATABASE;
                twr_scheduler_plan_current_from_now(_TWR_FINGERPRINT_DELAY_START_MEASUREMENT);
                return;
            }
            SEND_CMD_PACKET(FINGERPRINT_GETIMAGE);

            self->_state = TWR_FINGERPRINT_STATE_ENROLL_READ;

            twr_scheduler_plan_current_from_now(_TWR_FINGERPRINT_DELAY_START_MEASUREMENT);
            return;
        }

        case TWR_FINGERPRINT_STATE_ENROLL_READ:


            if (self->_event_handler != NULL)
            {
                if(!fingerValidation)
                {
                    self->_event_handler(self, TWR_FINGERPRINT_EVENT_ENROLL_FIRST, self->_event_param);
                }
                else
                {
                    self->_event_handler(self, TWR_FINGERPRINT_EVENT_ENROLL_SECOND, self->_event_param);

                }

            }
            if(!twr_fingerprint_read_data(self))
            {
                twr_scheduler_plan_current_from_now(_TWR_FINGERPRINT_DELAY_MEASUREMENT);
                return;
            }

            if(self->_response != FINGERPRINT_OK)
            {
                self->_state = TWR_FINGERPRINT_STATE_READY;
                twr_scheduler_plan_current_from_now(_TWR_FINGERPRINT_DELAY_READY);
                return;
            }
            else
            {
                self->_state = TWR_FINGERPRINT_STATE_IMAGE2TZ;
                twr_scheduler_plan_current_from_now(_TWR_FINGERPRINT_DELAY_RUN);
                return;
            }


        case TWR_FINGERPRINT_STATE_IMAGE2TZ:
        {
            if((self->_enroll))
            {
                if(!fingerValidation)
                {
                    SEND_CMD_PACKET(FINGERPRINT_IMAGE2TZ, 1);
                }
                else
                {
                    SEND_CMD_PACKET(FINGERPRINT_IMAGE2TZ, 2);
                }
            }
            else
            {
                SEND_CMD_PACKET(FINGERPRINT_IMAGE2TZ, 1);
            }
            self->_state = TWR_FINGERPRINT_STATE_IMAGE2TZ_READ;
            twr_scheduler_plan_current_from_now(_TWR_FINGERPRINT_DELAY_MEASUREMENT);
            return;
        }

        case TWR_FINGERPRINT_STATE_IMAGE2TZ_READ:
        {
            if(!twr_fingerprint_read_data(self))
            {
                twr_scheduler_plan_current_from_now(_TWR_FINGERPRINT_DELAY_MEASUREMENT);
                return;
            }
            if(self->_response == FINGERPRINT_OK)
            {
                if (self->_event_handler != NULL)
                {
                    self->_event_handler(self, TWR_FINGERPRINT_EVENT_REMOVE_FINGER, self->_event_param);
                }
                if(!(self->_enroll))
                {
                    self->_state = TWR_FINGERPRINT_STATE_FINGER_SEARCH;
                    goto start;
                }
                else
                {
                    if(!fingerValidation)
                    {
                        fingerValidation = true;
                        self->_state = TWR_FINGERPRINT_STATE_READY;
                        twr_scheduler_plan_current_from_now(2000);
                        return;
                    }
                    else
                    {
                        fingerValidation = false;
                        if (self->_event_handler != NULL)
                        {
                        self->_event_handler(self, TWR_FINGERPRINT_EVENT_FINGER_ENROLLED, self->_event_param);
                        }
                        self->_state = TWR_FINGERPRINT_STATE_CREATE_MODEL;
                        goto start;
                    }
                }

            }
            else
            {
                self->_state = TWR_FINGERPRINT_STATE_READY;
                self->_enroll = false;
                twr_scheduler_plan_current_from_now(_TWR_FINGERPRINT_DELAY_READY);
                return;
            }
        }

        case TWR_FINGERPRINT_STATE_CREATE_MODEL:
        {
            SEND_CMD_PACKET(FINGERPRINT_REGMODEL);

            self->_state = TWR_FINGERPRINT_STATE_CREATE_MODEL_READ;
            twr_scheduler_plan_current_from_now(_TWR_FINGERPRINT_DELAY_MEASUREMENT);
            return;
        }

        case TWR_FINGERPRINT_STATE_CREATE_MODEL_READ:
        {
            if(!twr_fingerprint_read_data(self))
            {
                twr_scheduler_plan_current_from_now(_TWR_FINGERPRINT_DELAY_MEASUREMENT);
                return;
            }

            if(self->_response == FINGERPRINT_OK)
            {
                self->_state = TWR_FINGERPRINT_STATE_STORE_MODEL;
                goto start;
            }
            else
            {
                return;
            }
        }

        case TWR_FINGERPRINT_STATE_STORE_MODEL:
        {
            SEND_CMD_PACKET(FINGERPRINT_STORE, 0x01, (uint8_t)((self->_template_count + 1) >> 8), (uint8_t)((self->_template_count + 1) & 0xFF));
            self->_state = TWR_FINGERPRINT_STATE_STORE_MODEL_READ;
            twr_scheduler_plan_current_from_now(_TWR_FINGERPRINT_DELAY_MEASUREMENT);
            return;
        }

        case TWR_FINGERPRINT_STATE_STORE_MODEL_READ:
        {
            if(!twr_fingerprint_read_data(self))
            {
                twr_scheduler_plan_current_from_now(_TWR_FINGERPRINT_DELAY_MEASUREMENT);
                return;
            }

            if(self->_response == FINGERPRINT_OK)
            {
                self->_state = TWR_FINGERPRINT_STATE_GET_TEMPLATE_COUNT;
                self->_enroll = false;

                goto start;

            }
            else
            {
                return;
            }
        }

        case TWR_FINGERPRINT_STATE_GET_TEMPLATE_COUNT:
        {
            SEND_CMD_PACKET(FINGERPRINT_TEMPLATECOUNT);

            self->_state = TWR_FINGERPRINT_STATE_GET_TEMPLATE_COUNT_READ;
            twr_scheduler_plan_current_from_now(_TWR_FINGERPRINT_DELAY_MEASUREMENT);
            return;
        }

        case TWR_FINGERPRINT_STATE_GET_TEMPLATE_COUNT_READ:
        {
            if(!twr_fingerprint_read_data(self))
            {
                twr_scheduler_plan_current_from_now(_TWR_FINGERPRINT_DELAY_MEASUREMENT);
                return;
            }

            if(self->_response == FINGERPRINT_OK)
            {
                self->_template_count = self->_buffer[9 + 1];
                self->_template_count <<= 8;
                self->_template_count |= self->_buffer[9 + 2];

                if(initializing)
                {
                    self->_state = TWR_FINGERPRINT_STATE_READY;
                    initializing = false;
                }
                else
                {
                    self->_state = TWR_FINGERPRINT_STATE_UPDATE;
                }

                if (self->_event_handler != NULL)
                {
                    self->_event_handler(self, TWR_FINGERPRINT_EVENT_TEMPLATE_COUNT_READ, self->_event_param);
                }


                goto start;

            }
            else
            {
                return;
            }
        }

        case TWR_FINGERPRINT_STATE_FINGER_SEARCH:
        {
            SEND_CMD_PACKET(FINGERPRINT_HISPEEDSEARCH, 0x01, 0x00, 0x00, 0x00, 0xA3);

            self->_state = TWR_FINGERPRINT_STATE_FINGER_SEARCH_READ;
            twr_scheduler_plan_current_from_now(_TWR_FINGERPRINT_DELAY_MEASUREMENT);
            return;
        }

        case TWR_FINGERPRINT_STATE_FINGER_SEARCH_READ:
        {
            if(!twr_fingerprint_read_data(self))
            {
                twr_scheduler_plan_current_from_now(_TWR_FINGERPRINT_DELAY_MEASUREMENT);
                return;
            }

            if(self->_response == FINGERPRINT_OK)
            {
                self->_finger_ID = self->_buffer[9 + 1];
                self->_finger_ID <<= 8;
                self->_finger_ID |= self->_buffer[9 + 2];

                self->_confidence = self->_buffer[9 + 3];
                self->_confidence <<= 8;
                self->_confidence |= self->_buffer[9 + 4];

                if (self->_event_handler != NULL)
                {
                    self->_event_handler(self, TWR_FINGERPRINT_EVENT_FINGERPRINT_MATCHED, self->_finger_ID);
                }

                self->_state = TWR_FINGERPRINT_STATE_UPDATE;

                goto start;

            }
            else
            {
                self->_state = TWR_FINGERPRINT_STATE_UPDATE;
                goto start;
            }
        }

        case TWR_FINGERPRINT_STATE_DELETE_DATABASE:
        {
            SEND_CMD_PACKET(FINGERPRINT_EMPTY);

            self->_state = TWR_FINGERPRINT_STATE_DELETE_DATABASE_READ;
            twr_scheduler_plan_current_from_now(_TWR_FINGERPRINT_DELAY_MEASUREMENT);
            return;
        }

        case TWR_FINGERPRINT_STATE_DELETE_DATABASE_READ:
        {
            if(!twr_fingerprint_read_data(self))
            {
                twr_scheduler_plan_current_from_now(_TWR_FINGERPRINT_DELAY_MEASUREMENT);
                return;
            }

            if(self->_response == FINGERPRINT_OK)
            {
                self->_delete = false;
                if (self->_event_handler != NULL)
                {
                    self->_event_handler(self, TWR_FINGERPRINT_EVENT_DATABASE_DELETED, NULL);
                }

                self->_state = TWR_FINGERPRINT_STATE_UPDATE;

                goto start;

            }
            else
            {
                return;
            }
        }



        case TWR_FINGERPRINT_STATE_READING:
        {
            self->_state = TWR_FINGERPRINT_STATE_UPDATE;

            goto start;
        }
        case TWR_FINGERPRINT_STATE_UPDATE:
        {
            if (self->_event_handler != NULL)
            {
                self->_event_handler(self, TWR_FINGERPRINT_EVENT_UPDATE, self->_event_param);
            }

            return;
        }
        default:
        {
            self->_state = TWR_FINGERPRINT_STATE_ERROR;

            goto start;
        }
    }
}
