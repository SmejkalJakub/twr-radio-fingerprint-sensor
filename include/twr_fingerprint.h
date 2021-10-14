#ifndef _TWR_FINGERPRINT_H
#define _TWR_FINGERPRINT_H

/*#include <twr_scheduler.h>
#include <twr_uart.h>*/
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

//! @addtogroup twr_fingerprint twr_fingerprint
//! @brief Driver for TMP112 temperature sensor
//! @{

//! @brief Callback events

typedef enum
{
    //! @brief Error event
    TWR_FINGERPRINT_EVENT_ERROR = 0,

    //! @brief Update event
    TWR_FINGERPRINT_EVENT_UPDATE = 1,

    //! @brief Template count event
    TWR_FINGERPRINT_EVENT_TEMPLATE_COUNT_READ = 2,

    //! @brief Fingerprints mached event
    TWR_FINGERPRINT_EVENT_FINGERPRINT_MATCHED = 3,

    //! @brief First Finger Enroll event
    TWR_FINGERPRINT_EVENT_ENROLL_FIRST = 4,

    //! @brief Second Finger Enroll event
    TWR_FINGERPRINT_EVENT_ENROLL_SECOND = 5,

    //! @brief Remove Finger From Sensor event
    TWR_FINGERPRINT_EVENT_REMOVE_FINGER = 6,

    //! @brief Finger Saved event
    TWR_FINGERPRINT_EVENT_FINGER_ENROLLED = 7,

    //! @brief Delete Database event
    TWR_FINGERPRINT_EVENT_DATABASE_DELETED = 8

} twr_fingerprint_event_t;

//! @brief Fingerprint instance

typedef struct twr_fingerprint_t twr_fingerprint_t;

//! @cond

typedef enum
{
    TWR_FINGERPRINT_STATE_ERROR = -1,
    TWR_FINGERPRINT_STATE_INITIALIZE = 0,
    TWR_FINGERPRINT_STATE_PASSWORD_READ = 1,
    TWR_FINGERPRINT_STATE_ENROLL_READ = 2,
    TWR_FINGERPRINT_STATE_IMAGE2TZ = 3,
    TWR_FINGERPRINT_STATE_IMAGE2TZ_READ = 4,
    TWR_FINGERPRINT_STATE_CREATE_MODEL = 5,
    TWR_FINGERPRINT_STATE_CREATE_MODEL_READ = 6,
    TWR_FINGERPRINT_STATE_STORE_MODEL = 7,
    TWR_FINGERPRINT_STATE_STORE_MODEL_READ = 8,
    TWR_FINGERPRINT_STATE_READING = 9,
    TWR_FINGERPRINT_STATE_GET_TEMPLATE_COUNT = 10,
    TWR_FINGERPRINT_STATE_GET_TEMPLATE_COUNT_READ = 11,
    TWR_FINGERPRINT_STATE_FINGER_SEARCH = 12,
    TWR_FINGERPRINT_STATE_FINGER_SEARCH_READ = 13,
    TWR_FINGERPRINT_STATE_DELETE_DATABASE = 14,
    TWR_FINGERPRINT_STATE_DELETE_DATABASE_READ = 15,
    TWR_FINGERPRINT_STATE_UPDATE = 16,
    TWR_FINGERPRINT_STATE_READY = 17
} twr_fingerprint_state_t;

struct twr_fingerprint_t
{
    twr_uart_channel_t _channel;
    twr_scheduler_task_id_t _task_id_interval;
    twr_scheduler_task_id_t _task_id_measure;
    void (*_event_handler)(twr_fingerprint_t *, twr_fingerprint_event_t, void *);
    void *_event_param;
    bool _measurement_active;
    bool _enroll;
    bool _delete;
    twr_tick_t _update_interval;
    twr_fingerprint_state_t _state;
    twr_tick_t _tick_ready;
    uint8_t _read_fifo_buffer[128];
    twr_fifo_t _read_fifo;
    uint32_t _password;
    uint8_t _buffer[16];
    size_t _length;
    uint8_t _response;
    uint16_t _template_count;
    uint16_t _finger_ID;
    uint16_t _confidence;
};

//! @endcond

bool twr_fingerprint_write_packet(void *data, size_t len);

//! @brief Initialize Fingerprint sensor
//! @param[in] self Instance
//! @param[in] i2c_channel I2C channel
//! @param[in] i2c_address I2C device address

void twr_fingerprint_init(twr_fingerprint_t *self, twr_uart_channel_t channel);


//! @brief Deinitialize Fingerprint sensor
//! @param[in] self Instance

void twr_fingerprint_deinit(twr_fingerprint_t *self);

//! @brief Set callback function
//! @param[in] self Instance
//! @param[in] event_handler Function address
//! @param[in] event_param Optional event parameter (can be NULL)

void twr_fingerprint_set_event_handler(twr_fingerprint_t *self, void (*event_handler)(twr_fingerprint_t *, twr_fingerprint_event_t, void *), void *event_param);

//! @brief Set update interval
//! @param[in] self Instance
//! @param[in] interval Measurement interval

void twr_fingerprint_set_update_interval(twr_fingerprint_t *self, twr_tick_t interval);

//! @brief Start measurement manually
//! @param[in] self Instance
//! @return true On success
//! @return false When other measurement is in progress

bool twr_fingerprint_measure(twr_fingerprint_t *self);

//! @brief Start Enrolling
//! @param[in] self Instance
void twr_fingerprint_enroll(twr_fingerprint_t *self);

//! @brief Checks the response validity
//! @param[in] self Instance
//! @return Type of response on Success
//! @return FINGERPRINT_PACKETRECIEVEERR on not valid packet
uint8_t twr_fingerprint_check_response(twr_fingerprint_t *self);

//! @brief Reads packet on UART to the buffer
//! @param[in] self Instance
//! @return true On success
//! @return false when the data is not long enough
bool twr_fingerprint_read_data(twr_fingerprint_t *self);

//! @brief Get number of fingers in database manually
//! @param[in] self Instance
void twr_fingerprint_get_template_count(twr_fingerprint_t *self);

//! @brief Start reading of the finger manually
//! @param[in] self Instance
void twr_fingerprint_read_finger(twr_fingerprint_t *self);

//! @brief Delete database manually
//! @param[in] self Instance
void twr_fingerprint_delete_database(twr_fingerprint_t *self);

#endif // _TWR_TMP112_H
