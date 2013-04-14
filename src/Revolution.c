// Implemented by Douwe Maan
// Design by Jnm (See http://forums.getpebble.com/discussion/comment/3538/#Comment_3538)

#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"


#define MY_UUID { 0xA1, 0x23, 0x08, 0x61, 0xD4, 0xEB, 0x4F, 0x6E, 0xA2, 0xD0, 0xEA, 0xA2, 0xA0, 0x77, 0x97, 0xDD }
PBL_APP_INFO(MY_UUID,
             "Revolution", "Douwe Maan",
             1, 2, /* App version */
             RESOURCE_ID_IMAGE_MENU_ICON,
             APP_INFO_WATCH_FACE);


#define USE_AMERICAN_DATE false

#define TIME_SLOT_ANIMATION_DURATION 500

#define SCREEN_WIDTH      144
#define SCREEN_HEIGHT     168

#define TIME_IMAGE_WIDTH  70
#define TIME_IMAGE_HEIGHT 70

#define DATE_IMAGE_WIDTH  20
#define DATE_IMAGE_HEIGHT 20

#define YEAR_IMAGE_WIDTH  10
#define YEAR_IMAGE_HEIGHT 10

#define DAY_IMAGE_WIDTH   20
#define DAY_IMAGE_HEIGHT  10

#define MARGIN            1
#define TIME_SLOT_SPACE   2
#define DATE_PART_SPACE   4


#define NUMBER_OF_TIME_IMAGES 10
const int TIME_IMAGE_RESOURCE_IDS[NUMBER_OF_TIME_IMAGES] = {
  RESOURCE_ID_IMAGE_TIME_0, 
  RESOURCE_ID_IMAGE_TIME_1, RESOURCE_ID_IMAGE_TIME_2, RESOURCE_ID_IMAGE_TIME_3, 
  RESOURCE_ID_IMAGE_TIME_4, RESOURCE_ID_IMAGE_TIME_5, RESOURCE_ID_IMAGE_TIME_6, 
  RESOURCE_ID_IMAGE_TIME_7, RESOURCE_ID_IMAGE_TIME_8, RESOURCE_ID_IMAGE_TIME_9
};

#define NUMBER_OF_DATE_IMAGES 10
const int DATE_IMAGE_RESOURCE_IDS[NUMBER_OF_DATE_IMAGES] = {
  RESOURCE_ID_IMAGE_DATE_0, 
  RESOURCE_ID_IMAGE_DATE_1, RESOURCE_ID_IMAGE_DATE_2, RESOURCE_ID_IMAGE_DATE_3, 
  RESOURCE_ID_IMAGE_DATE_4, RESOURCE_ID_IMAGE_DATE_5, RESOURCE_ID_IMAGE_DATE_6, 
  RESOURCE_ID_IMAGE_DATE_7, RESOURCE_ID_IMAGE_DATE_8, RESOURCE_ID_IMAGE_DATE_9
};

#define NUMBER_OF_YEAR_IMAGES 10
const int YEAR_IMAGE_RESOURCE_IDS[NUMBER_OF_YEAR_IMAGES] = {
  RESOURCE_ID_IMAGE_YEAR_0, 
  RESOURCE_ID_IMAGE_YEAR_1, RESOURCE_ID_IMAGE_YEAR_2, RESOURCE_ID_IMAGE_YEAR_3, 
  RESOURCE_ID_IMAGE_YEAR_4, RESOURCE_ID_IMAGE_YEAR_5, RESOURCE_ID_IMAGE_YEAR_6, 
  RESOURCE_ID_IMAGE_YEAR_7, RESOURCE_ID_IMAGE_YEAR_8, RESOURCE_ID_IMAGE_YEAR_9
};

#define NUMBER_OF_DAY_IMAGES  7
const int DAY_IMAGE_RESOURCE_IDS[NUMBER_OF_DAY_IMAGES] = {
  RESOURCE_ID_IMAGE_DAY_0, RESOURCE_ID_IMAGE_DAY_1, RESOURCE_ID_IMAGE_DAY_2, 
  RESOURCE_ID_IMAGE_DAY_3, RESOURCE_ID_IMAGE_DAY_4, RESOURCE_ID_IMAGE_DAY_5, 
  RESOURCE_ID_IMAGE_DAY_6
};


Window window;

Layer date_container_layer;
Layer day_layer;
Layer date_layer;
Layer year_layer;


#define EMPTY_SLOT -1

#define NUMBER_OF_TIME_SLOTS 4
BmpContainer time_image_containers[NUMBER_OF_TIME_SLOTS];
Layer time_slot_layers[NUMBER_OF_TIME_SLOTS];
int time_slot_state[NUMBER_OF_TIME_SLOTS]     = {EMPTY_SLOT, EMPTY_SLOT, EMPTY_SLOT, EMPTY_SLOT};
int new_time_slot_state[NUMBER_OF_TIME_SLOTS] = {EMPTY_SLOT, EMPTY_SLOT, EMPTY_SLOT, EMPTY_SLOT};

#define NUMBER_OF_DATE_SLOTS 4
BmpContainer date_image_containers[NUMBER_OF_DATE_SLOTS];
int date_slot_state[NUMBER_OF_DATE_SLOTS] = {EMPTY_SLOT, EMPTY_SLOT, EMPTY_SLOT, EMPTY_SLOT};

#define NUMBER_OF_YEAR_SLOTS 2
BmpContainer year_image_containers[NUMBER_OF_YEAR_SLOTS];
bool year_images_loaded;

BmpContainer day_image_container;
bool day_image_loaded;


void display_time(PblTm *tick_time);
void display_date(PblTm *tick_time);
void display_year(PblTm *tick_time);
void display_day(PblTm *tick_time);
void draw_date_container(Layer *layer, GContext* ctx);
void display_time_value(int value, int row_number);
void update_time_slot(int time_slot_number, int digit_value);
void slide_in_digit_image_into_time_slot(PropertyAnimation *animation, int time_slot_number, int digit_value);
void slide_out_digit_image_from_time_slot(PropertyAnimation *animation, int time_slot_number);
void slide_out_animation_stopped(Animation *slide_out_animation, bool finished, void *context);
void load_digit_image_into_time_slot(int time_slot_number, int digit_value);
void unload_digit_image_from_time_slot(int time_slot_number);
GRect frame_for_time_slot(int time_slot_number);
void display_date_value(int value, int part_number);
void update_date_slot(int date_slot_number, int digit_value);
void load_digit_image_into_date_slot(int date_slot_number, int digit_value);
void unload_digit_image_from_date_slot(int date_slot_number);
GRect frame_for_date_slot(int date_slot_number);
void pbl_main(void *params);
void handle_init(AppContextRef ctx);
void handle_minute_tick(AppContextRef ctx, PebbleTickEvent *event);
void handle_deinit(AppContextRef ctx);


// Display
void display_time(PblTm *tick_time) {
  int hour = tick_time->tm_hour;

  if (!clock_is_24h_style()) {
    hour = hour % 12;
    if (hour == 0) {
      hour = 12;
    }
  }

  display_time_value(hour, 0);
  display_time_value(tick_time->tm_min, 1);
}

void display_date(PblTm *tick_time) {
  int day   = tick_time->tm_mday;
  int month = tick_time->tm_mon + 1;

#if USE_AMERICAN_DATE
  display_date_value(month, 0);
  display_date_value(day, 1);
#else
  display_date_value(day, 0);
  display_date_value(month, 1);
#endif
}

void display_year(PblTm *tick_time) {
  if (year_images_loaded) {
    for (int i = 0; i < NUMBER_OF_YEAR_SLOTS; i++) {
      bmp_deinit_container(&year_image_containers[i]);
    }
  }

  int year = tick_time->tm_year;

  year = year % 100; // Maximum of two digits per row.

  // Column order is: | Column 0 | Column 1 |
  // (We process the columns in reverse order because that makes
  // extracting the digits from the year easier.)
  for (int column_number = 1; column_number >= 0; column_number--) {
    BmpContainer *image_container = &year_image_containers[column_number];

    GRect frame = GRect(
      column_number * (YEAR_IMAGE_WIDTH + MARGIN), 
      0, 
      YEAR_IMAGE_WIDTH, 
      YEAR_IMAGE_HEIGHT
    );

    bmp_init_container(YEAR_IMAGE_RESOURCE_IDS[year % 10], image_container);
    layer_set_frame(&image_container->layer.layer, frame);
    layer_add_child(&year_layer, &image_container->layer.layer);
    
    year = year / 10;
  }

  year_images_loaded = true;
}

void display_day(PblTm *tick_time) {
  if (day_image_loaded) {
    layer_remove_from_parent(&day_image_container.layer.layer);
    bmp_deinit_container(&day_image_container);
  }

  bmp_init_container(DAY_IMAGE_RESOURCE_IDS[tick_time->tm_wday], &day_image_container);
  layer_add_child(&day_layer, &day_image_container.layer.layer);
  day_image_loaded = true;
}

void draw_date_container(Layer *layer, GContext* ctx) {
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, GRect(0, 0, layer->bounds.size.w, layer->bounds.size.h), 0, GCornerNone);
}

// Time
void display_time_value(int value, int row_number) {
  value = value % 100; // Maximum of two digits per row.

  // Column order is: | Column 0 | Column 1 |
  // (We process the columns in reverse order because that makes
  // extracting the digits from the value easier.)
  for (int column_number = 1; column_number >= 0; column_number--) {
    int time_slot_number = (row_number * 2) + column_number;

    update_time_slot(time_slot_number, value % 10);

    value = value / 10;
  }
}

void update_time_slot(int time_slot_number, int digit_value) {
  if (time_slot_number < 0 || time_slot_number >= NUMBER_OF_TIME_SLOTS) {
    return;
  }

  if (time_slot_state[time_slot_number] == digit_value) {
    return;
  }

  static PropertyAnimation animations[NUMBER_OF_TIME_SLOTS];
  PropertyAnimation *animation = &animations[time_slot_number];

  if (time_slot_state[time_slot_number] == EMPTY_SLOT) {
    slide_in_digit_image_into_time_slot(animation, time_slot_number, digit_value);
  }
  else {
    new_time_slot_state[time_slot_number] = digit_value;

    slide_out_digit_image_from_time_slot(animation, time_slot_number);

    animation_set_handlers(&animation->animation, (AnimationHandlers){
      .stopped = (AnimationStoppedHandler)slide_out_animation_stopped
    }, (void *)(intptr_t)time_slot_number);
  }

  animation_schedule(&animation->animation);
}

void slide_in_digit_image_into_time_slot(PropertyAnimation *animation, int time_slot_number, int digit_value) {
  GRect to_frame = frame_for_time_slot(time_slot_number);

  int from_x = to_frame.origin.x;
  int from_y = to_frame.origin.y;
  switch (time_slot_number) {
    case 0:
      from_x = -TIME_IMAGE_WIDTH;
      break;
    case 1:
      from_y = -TIME_IMAGE_HEIGHT;
      break;
    case 2:
      from_y = SCREEN_WIDTH;
      break;
    case 3:
      from_x = SCREEN_WIDTH;
      break;
  }
  GRect from_frame = GRect(from_x, from_y, TIME_IMAGE_WIDTH, TIME_IMAGE_HEIGHT);

  Layer *time_slot_layer = &time_slot_layers[time_slot_number];

  layer_set_frame(time_slot_layer, from_frame);

  unload_digit_image_from_time_slot(time_slot_number);
  load_digit_image_into_time_slot(time_slot_number, digit_value);

  property_animation_init_layer_frame(animation, time_slot_layer, NULL, &to_frame);
  animation_set_duration(&animation->animation, TIME_SLOT_ANIMATION_DURATION);
  animation_set_curve(&animation->animation, AnimationCurveLinear);
}

void slide_out_digit_image_from_time_slot(PropertyAnimation *animation, int time_slot_number) {
  GRect from_frame = frame_for_time_slot(time_slot_number);

  int to_x = from_frame.origin.x;
  int to_y = from_frame.origin.y;
  switch (time_slot_number) {
    case 0:
      to_y = -TIME_IMAGE_HEIGHT;
      break;
    case 1:
      to_x = SCREEN_WIDTH;
      break;
    case 2:
      to_x = -TIME_IMAGE_WIDTH;
      break;
    case 3:
      to_y = SCREEN_WIDTH;
      break;
  }
  GRect to_frame = GRect(to_x, to_y, TIME_IMAGE_WIDTH, TIME_IMAGE_HEIGHT);

  property_animation_init_layer_frame(animation, &time_slot_layers[time_slot_number], NULL, &to_frame);
  animation_set_duration(&animation->animation, TIME_SLOT_ANIMATION_DURATION);
  animation_set_curve(&animation->animation, AnimationCurveLinear);

  // Make sure to unload the image when the animation has finished!
}

void slide_out_animation_stopped(Animation *slide_out_animation, bool finished, void *context) {
  int time_slot_number = (intptr_t)context;

  static PropertyAnimation animations[NUMBER_OF_TIME_SLOTS];
  PropertyAnimation *animation = &animations[time_slot_number];

  slide_in_digit_image_into_time_slot(animation, time_slot_number, new_time_slot_state[time_slot_number]);
  animation_schedule(&animation->animation);

  new_time_slot_state[time_slot_number] = EMPTY_SLOT;
}

void load_digit_image_into_time_slot(int time_slot_number, int digit_value) {
  if (digit_value < 0 || digit_value > 9) {
    return;
  }

  if (time_slot_state[time_slot_number] != EMPTY_SLOT) {
    return;
  }

  time_slot_state[time_slot_number] = digit_value;

  BmpContainer *image_container = &time_image_containers[time_slot_number];

  bmp_init_container(TIME_IMAGE_RESOURCE_IDS[digit_value], image_container);
  layer_add_child(&time_slot_layers[time_slot_number], &image_container->layer.layer);
}

void unload_digit_image_from_time_slot(int time_slot_number) {
  if (time_slot_state[time_slot_number] == EMPTY_SLOT) {
    return;
  }

  BmpContainer *image_container = &time_image_containers[time_slot_number];

  layer_remove_from_parent(&image_container->layer.layer);
  bmp_deinit_container(image_container);

  time_slot_state[time_slot_number] = EMPTY_SLOT;
}

GRect frame_for_time_slot(int time_slot_number) {
  int x = MARGIN + (time_slot_number % 2) * (TIME_IMAGE_WIDTH + TIME_SLOT_SPACE);
  int y = MARGIN + (time_slot_number / 2) * (TIME_IMAGE_HEIGHT + TIME_SLOT_SPACE);

  return GRect(x, y, TIME_IMAGE_WIDTH, TIME_IMAGE_HEIGHT);
}

// Date
void display_date_value(int value, int part_number) {
  value = value % 100; // Maximum of two digits per row.

  // Column order is: | Column 0 | Column 1 |
  // (We process the columns in reverse order because that makes
  // extracting the digits from the value easier.)
  for (int column_number = 1; column_number >= 0; column_number--) {
    int date_slot_number = (part_number * 2) + column_number;

    update_date_slot(date_slot_number, value % 10);

    value = value / 10;
  }
}

void update_date_slot(int date_slot_number, int digit_value) {
  if (date_slot_number < 0 || date_slot_number >= NUMBER_OF_DATE_SLOTS) {
    return;
  }

  if (date_slot_state[date_slot_number] == digit_value) {
    return;
  }

  unload_digit_image_from_date_slot(date_slot_number);
  load_digit_image_into_date_slot(date_slot_number, digit_value);
}

void load_digit_image_into_date_slot(int date_slot_number, int digit_value) {
  if (digit_value < 0 || digit_value > 9) {
    return;
  }

  if (date_slot_state[date_slot_number] != EMPTY_SLOT) {
    return;
  }

  date_slot_state[date_slot_number] = digit_value;

  BmpContainer *image_container = &date_image_containers[date_slot_number];

  bmp_init_container(DATE_IMAGE_RESOURCE_IDS[digit_value], image_container);
  layer_set_frame(&image_container->layer.layer, frame_for_date_slot(date_slot_number));
  layer_add_child(&date_layer, &image_container->layer.layer);
}

void unload_digit_image_from_date_slot(int date_slot_number) {
  if (date_slot_state[date_slot_number] == EMPTY_SLOT) {
    return;
  }

  BmpContainer *image_container = &date_image_containers[date_slot_number];

  layer_remove_from_parent(&image_container->layer.layer);
  bmp_deinit_container(image_container);

  date_slot_state[date_slot_number] = EMPTY_SLOT;
}

GRect frame_for_date_slot(int date_slot_number) {
  int x = date_slot_number * (DATE_IMAGE_WIDTH + MARGIN);
  if (date_slot_number >= 2) {
    x += 3; // 3 extra pixels of space between the day and month
  }

  return GRect(x, 0, DATE_IMAGE_WIDTH, DATE_IMAGE_HEIGHT);
}

// Handlers
void pbl_main(void *params) {
  PebbleAppHandlers handlers = {
    .init_handler   = &handle_init,
    .deinit_handler = &handle_deinit,

    .tick_info = {
      .tick_handler = &handle_minute_tick,
      .tick_units   = MINUTE_UNIT
    }
  };

  app_event_loop(params, &handlers);
}

void handle_init(AppContextRef ctx) {
  window_init(&window, "Revolution");
  window_stack_push(&window, true /* Animated */);
  window_set_background_color(&window, GColorBlack);

  resource_init_current_app(&APP_RESOURCES);

  // Root layer
  Layer *root_layer = window_get_root_layer(&window);

  // Time slots
  for (int i = 0; i < NUMBER_OF_TIME_SLOTS; i++) {
    layer_init(&time_slot_layers[i], frame_for_time_slot(i));
    layer_add_child(root_layer, &time_slot_layers[i]);
  }

  int date_container_height = SCREEN_HEIGHT - SCREEN_WIDTH;

  // Date container
  layer_init(&date_container_layer, GRect(0, SCREEN_WIDTH, SCREEN_WIDTH, date_container_height));
  layer_set_update_proc(&date_container_layer, &draw_date_container);
  layer_add_child(root_layer, &date_container_layer);

  // Day
  GRect day_layer_frame = GRect(
    MARGIN, 
    date_container_height - DAY_IMAGE_HEIGHT - MARGIN, 
    DAY_IMAGE_WIDTH, 
    DAY_IMAGE_HEIGHT
  );
  layer_init(&day_layer, day_layer_frame);
  layer_add_child(&date_container_layer, &day_layer);

  // Date
  GRect date_layer_frame = GRectZero;
  date_layer_frame.size.w   = DATE_IMAGE_WIDTH + MARGIN + DATE_IMAGE_WIDTH + DATE_PART_SPACE + DATE_IMAGE_WIDTH + MARGIN + DATE_IMAGE_WIDTH;
  date_layer_frame.size.h   = DATE_IMAGE_HEIGHT;
  date_layer_frame.origin.x = (SCREEN_WIDTH - date_layer_frame.size.w) / 2;
  date_layer_frame.origin.y = date_container_height - DATE_IMAGE_HEIGHT - MARGIN;

  layer_init(&date_layer, date_layer_frame);
  layer_add_child(&date_container_layer, &date_layer);

  // Year
  GRect year_layer_frame = GRect(
    SCREEN_WIDTH - YEAR_IMAGE_WIDTH - MARGIN - YEAR_IMAGE_WIDTH - MARGIN, 
    date_container_height - YEAR_IMAGE_HEIGHT - MARGIN, 
    YEAR_IMAGE_WIDTH + MARGIN + YEAR_IMAGE_WIDTH, 
    YEAR_IMAGE_HEIGHT
  );
  layer_init(&year_layer, year_layer_frame);
  layer_add_child(&date_container_layer, &year_layer);


  PblTm tick_time;
  get_time(&tick_time);

  display_day(&tick_time);
  display_date(&tick_time);
  display_year(&tick_time);
  // display_time(&tick_time);
}

void handle_minute_tick(AppContextRef ctx, PebbleTickEvent *event) {
  if ((event->units_changed & DAY_UNIT) == DAY_UNIT) {
    display_day(event->tick_time);
    display_date(event->tick_time);
  }

  if ((event->units_changed & YEAR_UNIT) == YEAR_UNIT) {
    display_year(event->tick_time);
  }

  display_time(event->tick_time);
}

void handle_deinit(AppContextRef ctx) {
  for (int i = 0; i < NUMBER_OF_TIME_SLOTS; i++) {
    unload_digit_image_from_time_slot(i);
  }
  for (int i = 0; i < NUMBER_OF_DATE_SLOTS; i++) {
    unload_digit_image_from_date_slot(i);
  }
  for (int i = 0; i < NUMBER_OF_YEAR_SLOTS; i++) {
    bmp_deinit_container(&year_image_containers[i]);
  }

  bmp_deinit_container(&day_image_container);
}