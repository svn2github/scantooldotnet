#ifndef SERIAL_H
#define SERIAL_H

#ifdef ALLEGRO_WINDOWS
   #define COM1   0
   #define COM2   1
   #define COM3   2
   #define COM4   3
   #define COM5   4
   #define COM6   5
   #define COM7   6
   #define COM8   7
   #define BAUD_RATE_9600    9600
   #define BAUD_RATE_38400   38400
#else
   #define DZCOMM_SECONDARY_INCLUDE
   #include <dzcomm.h>
   #define COM1   _com1
   #define COM2   _com2
   #define COM3   _com3
   #define COM4   _com4
   #define COM5   _com5
   #define COM6   _com6
   #define COM7   _com7
   #define COM8   _com8
   #define BAUD_RATE_9600    _9600
   #define BAUD_RATE_38400   _38400
#endif

//read_comport returned data type
#define EMPTY    0
#define DATA     1
#define PROMPT   2

#define SPECIAL_DELIMITER   '\t'

//comport status
#define READY          0
#define NOT_OPEN       1
#define USER_IGNORED   2

//process_response return values
#define HEX_DATA           0
#define BUS_BUSY           1
#define BUS_ERROR          2
#define DATA_ERROR         3
#define DATA_ERROR2        4
#define ERR_NO_DATA        5
#define BUFFER_FULL        6
#define SERIAL_ERROR       7
#define UNKNOWN_CMD        8
#define INTERFACE_ELM320   9
#define INTERFACE_ELM322   10
#define INTERFACE_ELM323   11
#define INTERFACE_ELM327   12
#define RUBBISH            13

// timeouts
#define OBD_REQUEST_TIMEOUT   8000
#define ATZ_TIMEOUT           1200
#define AT_TIMEOUT            130
#define ECU_TIMEOUT           5000

// function prototypes
void serial_module_init();
void serial_module_shutdown();
int open_comport();
void close_comport();
void send_command(const char *command);
int read_comport(char *response);
void start_serial_timer(int delay);
void stop_serial_timer();
int process_response(const char *cmd_sent, char *msg_received);
int find_valid_response(char *buf, char *response, const char *filter, char **stop);
const char *get_protocol_string(int interface_type, int protocol_id);
void display_error_message(int error);

// variables
volatile int serial_time_out;
volatile int serial_timer_running;

struct COMPORT {
   int number;
   int baud_rate;
   int status;    // READY, NOT_OPEN, USER_IGNORED
} comport;

#endif
