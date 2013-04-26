// Implemented by Douwe Maan
// Envisioned as a watchface by Jean-Noël Mattern (Jnm) (See http://forums.getpebble.com/discussion/comment/3538/#Comment_3538)
// Based on the display of the Freebox Revolution (http://www.free.fr/adsl/freebox-revolution.html), which was designed by Philippe Starck.

#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"


#define MY_UUID { 0xA1, 0x23, 0x08, 0x61, 0xD4, 0xEB, 0x4F, 0x6E, 0xA2, 0xD0, 0xEA, 0xA2, 0xA0, 0x77, 0x97, 0xDD }
PBL_APP_INFO(MY_UUID,
             "Revolution", "Douwe Maan",
             1, 3, /* App version */
             RESOURCE_ID_IMAGE_MENU_ICON,
             APP_INFO_WATCH_FACE);


// Settings
#define USE_AMERICAN_DATE_FORMAT      false
#define TIME_SLOT_ANIMATION_DURATION  500

// Magic numbers
#define SCREEN_WIDTH        144
#define SCREEN_HEIGHT       168

#define TIME_IMAGE_WIDTH    70
#define TIME_IMAGE_HEIGHT   70

#define DATE_IMAGE_WIDTH    20
#define DATE_IMAGE_HEIGHT   20

#define SECOND_IMAGE_WIDTH  10
#define SECOND_IMAGE_HEIGHT 10

#define DAY_IMAGE_WIDTH     20
#define DAY_IMAGE_HEIGHT    10

#define MARGIN              1
#define TIME_SLOT_SPACE     2
#define DATE_PART_SPACE     4


// Images
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

#define NUMBER_OF_SECOND_IMAGES 10
const int SECOND_IMAGE_RESOURCE_IDS[NUMBER_OF_SECOND_IMAGES] = {
  RESOURCE_ID_IMAGE_SECOND_0, 
  RESOURCE_ID_IMAGE_SECOND_1, RESOURCE_ID_IMAGE_SECOND_2, RESOURCE_ID_IMAGE_SECOND_3, 
  RESOURCE_ID_IMAGE_SECOND_4, RESOURCE_ID_IMAGE_SECOND_5, RESOURCE_ID_IMAGE_SECOND_6, 
  RESOURCE_ID_IMAGE_SECOND_7, RESOURCE_ID_IMAGE_SECOND_8, RESOURCE_ID_IMAGE_SECOND_9
};

#define NUMBER_OF_DAY_IMAGES  7
const int DAY_IMAGE_RESOURCE_IDS[NUMBER_OF_DAY_IMAGES] = {
  RESOURCE_ID_IMAGE_DAY_0, RESOURCE_ID_IMAGE_DAY_1, RESOURCE_ID_IMAGE_DAY_2, 
  RESOURCE_ID_IMAGE_DAY_3, RESOURCE_ID_IMAGE_DAY_4, RESOURCE_ID_IMAGE_DAY_5, 
  RESOURCE_ID_IMAGE_DAY_6
};


// Main
Window window;
Layer date_container_layer;

#define EMPTY_SLOT -1

// Time
typedef struct TimeSlot {
  int           number;
  BmpContainer  image_container;
  Layer         layer;
  int           state;
  int           new_state;
  bool          animating;
} TimeSlot;

#define NUMBER_OF_TIME_SLOTS 4
TimeSlot time_slots[NUMBER_OF_TIME_SLOTS];

// Date
typedef struct DateSlot {
  int           number;
  BmpContainer  image_container;
  int           state;
} DateSlot;

#define NUMBER_OF_DATE_SLOTS 4
Layer date_layer;
DateSlot date_slots[NUMBER_OF_DATE_SLOTS];

// Seconds
typedef struct SecondSlot {
  int           number;
  BmpContainer  image_container;
  int           state;
} SecondSlot;

#define NUMBER_OF_SECOND_SLOTS 2
Layer seconds_layer;
SecondSlot second_slots[NUMBER_OF_SECOND_SLOTS];

// Day
typedef struct DaySlot {
  BmpContainer  image_container;
  Layer         layer;
  bool          loaded;
} DaySlot;
DaySlot day_slot;


// Display
void display_time(PblTm *tick_time);
void display_date(PblTm *tick_time);
void display_seconds(PblTm *tick_time);
void display_day(PblTm *tick_time);
void draw_date_container(Layer *layer, GContext *ctx);

// Time
void display_time_value(int value, int row_number);
void update_time_slot(TimeSlot *time_slot, int digit_value);
void slide_in_digit_image_into_time_slot(PropertyAnimation *animation, TimeSlot *time_slot, int digit_value);
void slide_out_digit_image_from_time_slot(PropertyAnimation *animation, TimeSlot *time_slot);
void slide_in_animation_stopped(Animation *slide_in_animation, bool finished, void *context);
void slide_out_animation_stopped(Animation *slide_out_animation, bool finished, void *context);
void load_digit_image_into_time_slot(TimeSlot *time_slot, int digit_value);
void unload_digit_image_from_time_slot(TimeSlot *time_slot);
GRect frame_for_time_slot(TimeSlot *time_slot);

// Date
void display_date_value(int value, int part_number);
void update_date_slot(DateSlot *date_slot, int digit_value);
void load_digit_image_into_date_slot(DateSlot *date_slot, int digit_value);
void unload_digit_image_from_date_slot(DateSlot *date_slot);
GRect frame_for_date_slot(DateSlot *date_slot);

// Seconds
void update_second_slot(SecondSlot *second_slot, int digit_value);
void load_digit_image_into_second_slot(SecondSlot *second_slot, int digit_value);
void unload_digit_image_from_second_slot(SecondSlot *second_slot);
GRect frame_for_second_slot(SecondSlot *second_slot);

// Handlers
void pbl_main(void *params);
void handle_init(AppContextRef ctx);
void handle_second_tick(AppContextRef ctx, PebbleTickEvent *event);
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

  display_time_value(hour,              0);
  display_time_value(tick_time->tm_min, 1);
}

void display_date(PblTm *tick_time) {
  int day   = tick_time->tm_mday;
  int month = tick_time->tm_mon + 1;

#if USE_AMERICAN_DATE_FORMAT
  display_date_value(month, 0);
  display_date_value(day,   1);
#else
  display_date_value(day,   0);
  display_date_value(month, 1);
#endif
}

void display_seconds(PblTm *tick_time) {
  int seconds = tick_time->tm_sec;

  seconds = seconds % 100; // Maximum of two digits per row.

  for (int second_slot_number = 1; second_slot_number >= 0; second_slot_number--) {
    SecondSlot *second_slot = &second_slots[second_slot_number];

    update_second_slot(second_slot, seconds % 10);
    
    seconds = seconds / 10;
  }
}

void display_day(PblTm *tick_time) {
  BmpContainer *image_container = &day_slot.image_container;

  if (day_slot.loaded) {
    layer_remove_from_parent(&image_container->layer.layer);
    bmp_deinit_container(image_container);
  }

  bmp_init_container(DAY_IMAGE_RESOURCE_IDS[tick_time->tm_wday], image_container);
  layer_add_child(&day_slot.layer, &image_container->layer.layer);

  day_slot.loaded = true;
}

void draw_date_container(Layer *layer, GContext *ctx) {
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, GRect(0, 0, layer->bounds.size.w, layer->bounds.size.h), 0, GCornerNone);
}

// Time
void display_time_value(int value, int row_number) {
  value = value % 100; // Maximum of two digits per row.

  for (int column_number = 1; column_number >= 0; column_number--) {
    int time_slot_number = (row_number * 2) + column_number;

    TimeSlot *time_slot = &time_slots[time_slot_number];

    update_time_slot(time_slot, value % 10);

    value = value / 10;
  }
}

void update_time_slot(TimeSlot *time_slot, int digit_value) {
  if (time_slot->state == digit_value) {
    return;
  }

  if (time_slot->animating) {
    // Otherwise we'll crash when the animation is replaced by a new animation before we're finished.
    return;
  }

  time_slot->animating = true;

  static PropertyAnimation animations[NUMBER_OF_TIME_SLOTS];
  PropertyAnimation *animation = &animations[time_slot->number];

  if (time_slot->state == EMPTY_SLOT) {
    slide_in_digit_image_into_time_slot(animation, time_slot, digit_value);
  }
  else {
    time_slot->new_state = digit_value;

    slide_out_digit_image_from_time_slot(animation, time_slot);

    animation_set_handlers(&animation->animation, (AnimationHandlers){
      .stopped = (AnimationStoppedHandler)slide_out_animation_stopped
    }, (void *)time_slot);
  }

  animation_schedule(&animation->animation);
}

void slide_in_digit_image_into_time_slot(PropertyAnimation *animation, TimeSlot *time_slot, int digit_value) {
  GRect to_frame = frame_for_time_slot(time_slot);

  int from_x = to_frame.origin.x;
  int from_y = to_frame.origin.y;
  switch (time_slot->number) {
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

  layer_set_frame(&time_slot->layer, from_frame);

  unload_digit_image_from_time_slot(time_slot);
  load_digit_image_into_time_slot(time_slot, digit_value);

  property_animation_init_layer_frame(animation, &time_slot->layer, NULL, &to_frame);
  animation_set_duration( &animation->animation, TIME_SLOT_ANIMATION_DURATION);
  animation_set_curve(    &animation->animation, AnimationCurveLinear);
  animation_set_handlers( &animation->animation, (AnimationHandlers){
    .stopped = (AnimationStoppedHandler)slide_in_animation_stopped
  }, (void *)time_slot);
}

void slide_out_digit_image_from_time_slot(PropertyAnimation *animation, TimeSlot *time_slot) {
  GRect from_frame = frame_for_time_slot(time_slot);

  int to_x = from_frame.origin.x;
  int to_y = from_frame.origin.y;
  switch (time_slot->number) {
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

  property_animation_init_layer_frame(animation, &time_slot->layer, NULL, &to_frame);
  animation_set_duration( &animation->animation, TIME_SLOT_ANIMATION_DURATION);
  animation_set_curve(    &animation->animation, AnimationCurveLinear);

  // Make sure to unload the image when the animation has finished!
}

void slide_in_animation_stopped(Animation *slide_in_animation, bool finished, void *context) {
  TimeSlot *time_slot = (TimeSlot *)context;

  time_slot->animating = false;
}

void slide_out_animation_stopped(Animation *slide_out_animation, bool finished, void *context) {
  TimeSlot *time_slot = (TimeSlot *)context;

  static PropertyAnimation animations[NUMBER_OF_TIME_SLOTS];
  PropertyAnimation *animation = &animations[time_slot->number];

  slide_in_digit_image_into_time_slot(animation, time_slot, time_slot->new_state);
  animation_schedule(&animation->animation);

  time_slot->new_state = EMPTY_SLOT;
}

void load_digit_image_into_time_slot(TimeSlot *time_slot, int digit_value) {
  if (digit_value < 0 || digit_value > 9) {
    return;
  }

  if (time_slot->state != EMPTY_SLOT) {
    return;
  }

  time_slot->state = digit_value;

  bmp_init_container(TIME_IMAGE_RESOURCE_IDS[digit_value], &time_slot->image_container);
  layer_add_child(&time_slot->layer, &time_slot->image_container.layer.layer);
}

void unload_digit_image_from_time_slot(TimeSlot *time_slot) {
  if (time_slot->state == EMPTY_SLOT) {
    return;
  }

  layer_remove_from_parent(&time_slot->image_container.layer.layer);
  bmp_deinit_container(&time_slot->image_container);

  time_slot->state = EMPTY_SLOT;
}

GRect frame_for_time_slot(TimeSlot *time_slot) {
  int x = MARGIN + (time_slot->number % 2) * (TIME_IMAGE_WIDTH + TIME_SLOT_SPACE);
  int y = MARGIN + (time_slot->number / 2) * (TIME_IMAGE_HEIGHT + TIME_SLOT_SPACE);

  return GRect(x, y, TIME_IMAGE_WIDTH, TIME_IMAGE_HEIGHT);
}

// Date
void display_date_value(int value, int part_number) {
  value = value % 100; // Maximum of two digits per row.

  for (int column_number = 1; column_number >= 0; column_number--) {
    int date_slot_number = (part_number * 2) + column_number;

    DateSlot *date_slot = &date_slots[date_slot_number];

    update_date_slot(date_slot, value % 10);

    value = value / 10;
  }
}

void update_date_slot(DateSlot *date_slot, int digit_value) {
  if (date_slot->state == digit_value) {
    return;
  }

  unload_digit_image_from_date_slot(date_slot);
  load_digit_image_into_date_slot(date_slot, digit_value);
}

void load_digit_image_into_date_slot(DateSlot *date_slot, int digit_value) {
  if (digit_value < 0 || digit_value > 9) {
    return;
  }

  if (date_slot->state != EMPTY_SLOT) {
    return;
  }

  date_slot->state = digit_value;

  BmpContainer *image_container = &date_slot->image_container;

  bmp_init_container(DATE_IMAGE_RESOURCE_IDS[digit_value], image_container);
  layer_set_frame(&image_container->layer.layer, frame_for_date_slot(date_slot));
  layer_add_child(&date_layer, &image_container->layer.layer);
}

void unload_digit_image_from_date_slot(DateSlot *date_slot) {
  if (date_slot->state == EMPTY_SLOT) {
    return;
  }

  BmpContainer *image_container = &date_slot->image_container;

  layer_remove_from_parent(&image_container->layer.layer);
  bmp_deinit_container(image_container);

  date_slot->state = EMPTY_SLOT;
}

GRect frame_for_date_slot(DateSlot *date_slot) {
  int x = date_slot->number * (DATE_IMAGE_WIDTH + MARGIN);
  if (date_slot->number >= 2) {
    x += 3; // 3 extra pixels of space between the day and month
  }

  return GRect(x, 0, DATE_IMAGE_WIDTH, DATE_IMAGE_HEIGHT);
}

// Seconds
void update_second_slot(SecondSlot *second_slot, int digit_value) {
  if (second_slot->state == digit_value) {
    return;
  }

  unload_digit_image_from_second_slot(second_slot);
  load_digit_image_into_second_slot(second_slot, digit_value);
}

void load_digit_image_into_second_slot(SecondSlot *second_slot, int digit_value) {
  if (digit_value < 0 || digit_value > 9) {
    return;
  }

  if (second_slot->state != EMPTY_SLOT) {
    return;
  }

  second_slot->state = digit_value;

  BmpContainer *image_container = &second_slot->image_container;

  bmp_init_container(SECOND_IMAGE_RESOURCE_IDS[digit_value], image_container);
  layer_set_frame(&image_container->layer.layer, frame_for_second_slot(second_slot));
  layer_add_child(&seconds_layer, &image_container->layer.layer);
}

void unload_digit_image_from_second_slot(SecondSlot *second_slot) {
  if (second_slot->state == EMPTY_SLOT) {
    return;
  }

  BmpContainer *image_container = &second_slot->image_container;

  layer_remove_from_parent(&image_container->layer.layer);
  bmp_deinit_container(image_container);

  second_slot->state = EMPTY_SLOT;
}

GRect frame_for_second_slot(SecondSlot *second_slot) {
  return GRect(
    second_slot->number * (SECOND_IMAGE_WIDTH + MARGIN), 
    0, 
    SECOND_IMAGE_WIDTH, 
    SECOND_IMAGE_HEIGHT
  );
}

// Handlers
void pbl_main(void *params) {
  PebbleAppHandlers handlers = {
    .init_handler   = &handle_init,
    .deinit_handler = &handle_deinit,

    .tick_info = {
      .tick_handler = &handle_second_tick,
      .tick_units   = SECOND_UNIT
    }
  };

  app_event_loop(params, &handlers);
}

void handle_init(AppContextRef ctx) {
  window_init(&window, "Revolution");
  window_stack_push(&window, true /* Animated */);
  window_set_background_color(&window, GColorBlack);

  resource_init_current_app(&APP_RESOURCES);

  // Time slots
  for (int i = 0; i < NUMBER_OF_TIME_SLOTS; i++) {
    TimeSlot *time_slot = &time_slots[i];
    time_slot->number     = i;
    time_slot->state      = EMPTY_SLOT;
    time_slot->new_state  = EMPTY_SLOT;
    time_slot->animating  = false;
  }

  // Date slots
  for (int i = 0; i < NUMBER_OF_DATE_SLOTS; i++) {
    DateSlot *date_slot = &date_slots[i];
    date_slot->number = i;
    date_slot->state  = EMPTY_SLOT;
  }

  // Second slots
  for (int i = 0; i < NUMBER_OF_SECOND_SLOTS; i++) {
    SecondSlot *second_slot = &second_slots[i];
    second_slot->number = i;
    second_slot->state  = EMPTY_SLOT;
  }

  // Day slot
  day_slot.loaded = false;

  // Root layer
  Layer *root_layer = window_get_root_layer(&window);

  // Time slots
  for (int i = 0; i < NUMBER_OF_TIME_SLOTS; i++) {
    TimeSlot *time_slot = &time_slots[i];
    layer_init(&time_slot->layer, frame_for_time_slot(time_slot));
    layer_add_child(root_layer, &time_slot->layer);
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
  layer_init(&day_slot.layer, day_layer_frame);
  layer_add_child(&date_container_layer, &day_slot.layer);

  // Date
  GRect date_layer_frame = GRectZero;
  date_layer_frame.size.w   = DATE_IMAGE_WIDTH + MARGIN + DATE_IMAGE_WIDTH + DATE_PART_SPACE + DATE_IMAGE_WIDTH + MARGIN + DATE_IMAGE_WIDTH;
  date_layer_frame.size.h   = DATE_IMAGE_HEIGHT;
  date_layer_frame.origin.x = (SCREEN_WIDTH - date_layer_frame.size.w) / 2;
  date_layer_frame.origin.y = date_container_height - DATE_IMAGE_HEIGHT - MARGIN;

  layer_init(&date_layer, date_layer_frame);
  layer_add_child(&date_container_layer, &date_layer);

  // Seconds
  GRect seconds_layer_frame = GRect(
    SCREEN_WIDTH - SECOND_IMAGE_WIDTH - MARGIN - SECOND_IMAGE_WIDTH - MARGIN, 
    date_container_height - SECOND_IMAGE_HEIGHT - MARGIN, 
    SECOND_IMAGE_WIDTH + MARGIN + SECOND_IMAGE_WIDTH, 
    SECOND_IMAGE_HEIGHT
  );
  layer_init(&seconds_layer, seconds_layer_frame);
  layer_add_child(&date_container_layer, &seconds_layer);


  // Display
  PblTm tick_time;
  get_time(&tick_time);

  display_time(&tick_time);
  display_day(&tick_time);
  display_date(&tick_time);
  display_seconds(&tick_time);
}

void handle_second_tick(AppContextRef ctx, PebbleTickEvent *event) {
  display_seconds(event->tick_time);

  if ((event->units_changed & MINUTE_UNIT) == MINUTE_UNIT) {
    display_time(event->tick_time);
  }

  if ((event->units_changed & DAY_UNIT) == DAY_UNIT) {
    display_day(event->tick_time);
    display_date(event->tick_time);
  }
}

void handle_deinit(AppContextRef ctx) {
  for (int i = 0; i < NUMBER_OF_TIME_SLOTS; i++) {
    unload_digit_image_from_time_slot(&time_slots[i]);
  }
  for (int i = 0; i < NUMBER_OF_DATE_SLOTS; i++) {
    unload_digit_image_from_date_slot(&date_slots[i]);
  }
  for (int i = 0; i < NUMBER_OF_SECOND_SLOTS; i++) {
    unload_digit_image_from_second_slot(&second_slots[i]);
  }

  if (day_slot.loaded) {
    bmp_deinit_container(&day_slot.image_container);
  }
}