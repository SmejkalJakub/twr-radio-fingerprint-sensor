#ifndef _BC_FINGERPRINT_H
#define _BC_FINGERPRINT_H

/*#include <bc_scheduler.h>
#include <bc_uart.h>*/
#include <bcl.h>


#define FINGERPRINT_OK 0x00
#define FINGERPRINT_PACKETRECIEVEERR 0x01
#define FINGERPRINT_NOFINGER 0x02
#define FINGERPRINT_IMAGEFAIL 0x03
#define FINGERPRINT_IMAGEMESS 0x06
#define FINGERPRINT_FEATUREFAIL 0x07
#define FINGERPRINT_NOMATCH 0x08
#define FINGERPRINT_NOTFOUND 0x09
#define FINGERPRINT_STARTCODE 0xEF01
#define FINGERPRINT_ENROLLMISMATCH 0x0A
#define FINGERPRINT_BADLOCATION 0x0B
#define FINGERPRINT_DBRANGEFAIL 0x0C
#define FINGERPRINT_UPLOADFEATUREFAIL 0x0D
#define FINGERPRINT_PACKETRESPONSEFAIL 0x0E
#define FINGERPRINT_UPLOADFAIL 0x0F
#define FINGERPRINT_DELETEFAIL 0x10
#define FINGERPRINT_DBCLEARFAIL 0x11
#define FINGERPRINT_PASSFAIL 0x13
#define FINGERPRINT_INVALIDIMAGE 0x15
#define FINGERPRINT_FLASHERR 0x18
#define FINGERPRINT_INVALIDREG 0x1A
#define FINGERPRINT_ADDRCODE 0x20
#define FINGERPRINT_PASSVERIFY 0x21

#define FINGERPRINT_COMMANDPACKET 0x1
#define FINGERPRINT_DATAPACKET 0x2
#define FINGERPRINT_ACKPACKET 0x7
#define FINGERPRINT_ENDDATAPACKET 0x8

#define FINGERPRINT_TIMEOUT 0xFF
#define FINGERPRINT_BADPACKET 0xFE

#define FINGERPRINT_GETIMAGE 0x01
#define FINGERPRINT_IMAGE2TZ 0x02
#define FINGERPRINT_REGMODEL 0x05
#define FINGERPRINT_STORE 0x06
#define FINGERPRINT_LOAD 0x07
#define FINGERPRINT_UPLOAD 0x08
#define FINGERPRINT_DELETE 0x0C
#define FINGERPRINT_EMPTY 0x0D
#define FINGERPRINT_SETPASSWORD 0x12
#define FINGERPRINT_VERIFYPASSWORD 0x13
#define FINGERPRINT_HISPEEDSEARCH 0x1B
#define FINGERPRINT_TEMPLATECOUNT 0x1D

//! @addtogroup bc_fingerprint bc_fingerprint
//! @brief Driver for TMP112 temperature sensor
//! @{

//! @brief Callback events

typedef enum
{
    //! @brief Error event
    BC_FINGERPRINT_EVENT_ERROR = 0,

    //! @brief Update event
    BC_FINGERPRINT_EVENT_UPDATE = 1,

    BC_FINGERPRINT_EVENT_TEMPLATE_COUNT_READ = 2,

    BC_FINGERPRINT_EVENT_FINGERPRINT_MACHED = 3,

    BC_FINGERPRINT_EVENT_ENROLL_FIRST = 4,

    BC_FINGERPRINT_EVENT_ENROLL_SECOND = 5,
    
    BC_FINGERPRINT_EVENT_REMOVE_FINGER = 6,

    BC_FINGERPRINT_EVENT_FINGER_ENROLLED = 7,

    BC_FINGERPRINT_EVENT_DATABASE_DELETED = 8

} bc_fingerprint_event_t;

//! @brief TMP112 instance

typedef struct bc_fingerprint_t bc_fingerprint_t;

//! @cond

typedef enum
{
    BC_FINGERPRINT_STATE_ERROR = -1,
    BC_FINGERPRINT_STATE_INITIALIZE = 0,
    BC_FINGERPRINT_STATE_PASSWORD_READ = 1, 
    BC_FINGERPRINT_STATE_ENROLL_READ = 2,
    BC_FINGERPRINT_STATE_IMAGE2TZ = 3,
    BC_FINGERPRINT_STATE_IMAGE2TZ_READ = 4,
    BC_FINGERPRINT_STATE_CREATE_MODEL = 5,
    BC_FINGERPRINT_STATE_CREATE_MODEL_READ = 6,
    BC_FINGERPRINT_STATE_STORE_MODEL = 7,
    BC_FINGERPRINT_STATE_STORE_MODEL_READ = 8,
    BC_FINGERPRINT_STATE_READING = 9,
    BC_FINGERPRINT_STATE_GET_TEMPLATE_COUNT = 10,
    BC_FINGERPRINT_STATE_GET_TEMPLATE_COUNT_READ = 11,
    BC_FINGERPRINT_STATE_FINGER_SEARCH = 12,
    BC_FINGERPRINT_STATE_FINGER_SEARCH_READ = 13,
    BC_FINGERPRINT_STATE_DELETE_DATABASE = 14,
    BC_FINGERPRINT_STATE_DELETE_DATABASE_READ = 15,
    BC_FINGERPRINT_STATE_UPDATE = 16,
    BC_FINGERPRINT_STATE_READY = 17
    ,
} bc_fingerprint_state_t;

struct bc_fingerprint_t
{
    bc_uart_channel_t _channel;
    bc_scheduler_task_id_t _task_id_interval;
    bc_scheduler_task_id_t _task_id_measure;
    void (*_event_handler)(bc_fingerprint_t *, bc_fingerprint_event_t, void *);
    void *_event_param;
    bool _measurement_active;
    bool _enroll;
    bool _delete;
    bc_tick_t _update_interval;
    bc_fingerprint_state_t _state;
    bc_tick_t _tick_ready;
    uint8_t _read_fifo_buffer[128];
    bc_fifo_t _read_fifo;
    uint32_t _password;
    uint8_t _buffer[16];
    size_t _length;
    uint8_t _response;
    uint16_t _template_count;
    uint16_t _finger_ID;
    uint16_t _confidence;
};

//! @endcond

bool bc_fingerprint_write_packet(void *data, size_t len);

//! @brief Initialize TMP112
//! @param[in] self Instance
//! @param[in] i2c_channel I2C channel
//! @param[in] i2c_address I2C device address

void bc_fingerprint_init(bc_fingerprint_t *self, bc_uart_channel_t channel);


//! @brief Deinitialize TMP112
//! @param[in] self Instance

void bc_fingerprint_deinit(bc_fingerprint_t *self);

//! @brief Set callback function
//! @param[in] self Instance
//! @param[in] event_handler Function address
//! @param[in] event_param Optional event parameter (can be NULL)

void bc_fingerprint_set_event_handler(bc_fingerprint_t *self, void (*event_handler)(bc_fingerprint_t *, bc_fingerprint_event_t, void *), void *event_param);

//! @brief Set measurement interval
//! @param[in] self Instance
//! @param[in] interval Measurement interval

void bc_fingerprint_set_update_interval(bc_fingerprint_t *self, bc_tick_t interval);

//! @brief Start measurement manually
//! @param[in] self Instance
//! @return true On success
//! @return false When other measurement is in progress

bool bc_fingerprint_measure(bc_fingerprint_t *self);

void bc_fingerprint_enroll(bc_fingerprint_t *self);

uint8_t bc_fingerprint_check_response(bc_fingerprint_t *self);

bool bc_fingerprint_read_data(bc_fingerprint_t *self);

void bc_fingerprint_get_template_count(bc_fingerprint_t *self);

void bc_fingerprint_read_finger(bc_fingerprint_t *self);

void bc_fingerprint_delete_database(bc_fingerprint_t *self);

#endif // _BC_TMP112_H
