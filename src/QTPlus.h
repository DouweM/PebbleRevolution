#include <pebble.h>
/*
* User settings
* Show Clock
* Show Weather
* Autohide
*/

// Config
#define QTP_WINDOW_TIMEOUT 2000
#define QTP_K_SHOW_TIME 1
#define QTP_K_SHOW_WEATHER 2
#define QTP_K_AUTOHIDE 4
#define QTP_K_DEGREES_F 8
#define QTP_K_INVERT 16

#define QTP_SCREEN_WIDTH        144
#define QTP_SCREEN_HEIGHT       168

#define QTP_PADDING_Y 5
#define QTP_PADDING_X 5
#define QTP_BT_ICON_SIZE 32
#define QTP_BAT_ICON_SIZE 32
#define QTP_WEATHER_SIZE 32
#define QTP_TIME_HEIGHT 32
#define QTP_BATTERY_BASE_Y 0
#define QTP_BLUETOOTH_BASE_Y QTP_BAT_ICON_SIZE + 5
#define QTP_WEATHER_BASE_Y QTP_BAT_ICON_SIZE + QTP_BT_ICON_SIZE + 10



// Items
static Window *qtp_window;
bool qtp_is_showing;
TextLayer *qtp_battery_text_layer;
TextLayer *qtp_bluetooth_text_layer;
TextLayer *qtp_time_layer;
TextLayer *qtp_temp_layer;
TextLayer *qtp_weather_desc_layer;
AppTimer *qtp_hide_timer;
GBitmap *qtp_bluetooth_image;
GBitmap *qtp_weather_icon;
BitmapLayer *qtp_bluetooth_image_layer;
BitmapLayer *qtp_weather_icon_layer;
GBitmap *qtp_battery_image;
BitmapLayer *qtp_battery_image_layer;
InverterLayer *qtp_inverter_layer;
int qtp_conf;

AppSync qtp_sync;
uint8_t qtp_sync_buffer[120];

enum qtp_weather_key {
	QTP_WEATHER_ICON_KEY = 0x0,         // TUPLE_INT
	QTP_WEATHER_TEMP_F_KEY = 0x1,  // TUPLE_CSTRING
	QTP_WEATHER_CITY_KEY = 0x2,         // TUPLE_CSTRING
	QTP_WEATHER_DESC_KEY = 0x3,         // TUPLE_CSTRING
	QTP_WEATHER_TEMP_C_KEY = 0x4 // TUPLE_CSTRING
};

static const int QTP_WEATHER_ICONS[] = {
	RESOURCE_ID_QTP_IMAGE_CLEAR_DAY, //0
	RESOURCE_ID_QTP_IMAGE_CLEAR_NIGHT, //1
	RESOURCE_ID_QTP_IMAGE_ATMOSPHERE_NIGHT, //2
	RESOURCE_ID_QTP_IMAGE_ATMOSPHERE_DAY, //3
	RESOURCE_ID_QTP_IMAGE_CLOUDY, //4
	RESOURCE_ID_QTP_IMAGE_THUNDERSTORM, //5
	RESOURCE_ID_QTP_IMAGE_RAIN, //6
	RESOURCE_ID_QTP_IMAGE_SNOW, //7
	RESOURCE_ID_QTP_IMAGE_NONE //8
};


// Methods
void qtp_setup();
void qtp_app_deinit();

void qtp_show();
void qtp_hide();
void qtp_timeout();

void qtp_tap_handler(AccelAxisType axis, int32_t direction);
void qtp_click_config_provider(Window *window);
void qtp_back_click_responder(ClickRecognizerRef recognizer, void *context);

void qtp_setup_app_message();
static void qtp_sync_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context);
static void qtp_sync_error_callback(DictionaryResult dict_error, AppMessageResult app_message_error, void *context);

void qtp_update_battery_status(bool mark_dirty);
void qtp_update_bluetooth_status(bool mark_dirty);
void qtp_update_time(bool mark_dirty);
void qtp_update_weather_icon(int icon_index, bool remove_old, bool mark_dirty);

void qtp_init();
void qtp_deinit();

// Helpers
bool qtp_is_show_time();
bool qtp_is_show_weather();
bool qtp_is_autohide();
bool qtp_is_degrees_f();
bool qtp_is_invert();

int qtp_battery_y();
int qtp_bluetooth_y();
int qtp_weather_y();


