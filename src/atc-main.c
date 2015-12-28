#include <pebble.h>
#include "atc-main.h"

//remember to include random spawning - fix this

#define KEY_HEADING 1

static Window *window, *heading_window;
static TextLayer *heading_layer, *app_layer;
static Layer *airport_layer, *fix_layer, *plane_layer;
static int selected_plane; // use this to choose plane to edit
static int shown_window = 1;
static int heading_buffer_passed = 0;
static AppTimer *app_timer_text;

//PLANE_1 VARS
static GPath *plane_1_path;
static AppTimer *plane_1_timer;
static int plane_1_angle = 0;
static int plane_1_x = 72;
static int plane_1_y = 150;
static AppTimer *plane_1_app_timer;


static void rotate_plane(int plane, int angle) {
  if (plane == 1) {
    gpath_rotate_to(plane_1_path, angle);
    layer_mark_dirty(window_get_root_layer(window));
    plane_1_angle = angle;
  }
}

static void move_plane(int plane) {
  if (plane == 1) {
    switch(plane_1_angle) {
      case 0:
        plane_1_x += 0;
        plane_1_y -= 4;
        break;
      case 45:
        plane_1_x += 2;
        plane_1_y -= 2;
        break;
      case 90:
        plane_1_x += 4;
        plane_1_y -= 0;
        break;
      case 135:
        plane_1_x += 2;
        plane_1_y += 2;
        break;
      case 180:
        plane_1_x += 0;
        plane_1_y += 4;
        break;
      case 225:
        plane_1_x -= 2;
        plane_1_y += 2;
        break;
      case 270:
        plane_1_x -= 4;
        plane_1_y += 0;
        break;
      case 315:
        plane_1_x -= 2;
        plane_1_y -= 2;
        break;
      default:
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Unknown angle!");
        break;        
    }
    gpath_move_to(plane_1_path, GPoint(plane_1_x, plane_1_y));
    layer_mark_dirty(window_get_root_layer(window));
  }
}

static void plane_1_timer_handler(void *data) {
  move_plane(1);
  plane_1_timer = app_timer_register(2000, plane_1_timer_handler, NULL);
}

static void select_hide_menu();

static void heading_up_handler(ClickRecognizerRef recognizer, void *context) {
  if (atoi(text_layer_get_text(heading_layer)) == 0) {
    text_layer_set_text(heading_layer, "315");
    heading_buffer_passed = 315;
  } else {
    static char heading_buffer_up[8];
    snprintf(heading_buffer_up, sizeof(heading_buffer_up), "%d", atoi(text_layer_get_text(heading_layer)) - 45);
    text_layer_set_text(heading_layer, heading_buffer_up);
    heading_buffer_passed -= 45;
    
  }
}

static void heading_select_handler(ClickRecognizerRef recognizer, void *context) {
  select_hide_menu();
  rotate_plane(1, heading_buffer_passed);
  heading_buffer_passed = 0;
}

static void heading_down_handler(ClickRecognizerRef recognizer, void *context) {
  if (atoi(text_layer_get_text(heading_layer)) == 315) {
    text_layer_set_text(heading_layer, "0");
    heading_buffer_passed = 0;
  } else {
    static char heading_buffer_down[8];
    snprintf(heading_buffer_down, sizeof(heading_buffer_down), "%d", atoi(text_layer_get_text(heading_layer)) + 45);
    text_layer_set_text(heading_layer, heading_buffer_down);
    heading_buffer_passed += 45;
  }
}

static void heading_click_config_provider(void *context) {
	window_single_click_subscribe(BUTTON_ID_UP, heading_up_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, heading_select_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, heading_down_handler);
}
static void select_show_menu() {
  shown_window = 2;
  heading_window = window_create();
  window_set_background_color(heading_window, GColorBlack);
  window_set_click_config_provider(heading_window, heading_click_config_provider);
  heading_layer = text_layer_create(GRect(0, 70, 144, 90));
  text_layer_set_text(heading_layer, "0");
  text_layer_set_text_color(heading_layer, GColorFromHEX(0x00FF00));
  text_layer_set_background_color(heading_layer, GColorBlack);
  text_layer_set_text_alignment(heading_layer, GTextAlignmentCenter);
  layer_add_child(window_get_root_layer(heading_window), text_layer_get_layer(heading_layer));
  window_stack_push(heading_window, false);
}

static void select_hide_menu() {
  text_layer_destroy(heading_layer);
  window_stack_pop(false);
  window_destroy(heading_window);
}

static void plane_update_proc(Layer *layer, GContext *ctx) {
  graphics_context_set_fill_color(ctx, GColorFromHEX(0x00FF00));
  gpath_draw_filled(ctx, plane_1_path);
}

static void airport_draw_proc(Layer *layer, GContext *ctx) {
  graphics_context_set_stroke_color(ctx, GColorFromHEX(0xAAAAAA));
  graphics_context_set_stroke_width(ctx, 3);
  
  GPoint p0 = GPoint(40, 73);
  GPoint p1 = GPoint(70, 68);
  
  
  
  //runway
  graphics_draw_line(ctx, p0, p1);
  
  // FAF ILS
  GPoint IAF = GPoint(110, 60);
  graphics_context_set_stroke_color(ctx, GColorFromHEX(0x00FF00));
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_circle(ctx, IAF, 4);
  
  graphics_draw_line(ctx, p1, IAF);
}

static void fix_draw_proc(Layer *layer, GContext *ctx) {
  //GRect bounds = layer_get_bounds(layer);
  
  GPoint ABB = GPoint(143, 142);
  GPoint LOR = GPoint(126, 1);
  GPoint ASK = GPoint(1, 132);
  
  graphics_context_set_fill_color(ctx, GColorFromHEX(0x00FF00));
  
  graphics_fill_circle(ctx, ABB, 3);
  graphics_fill_circle(ctx, LOR, 3);
  graphics_fill_circle(ctx, ASK, 3);
  
}

static void main_window_load() {
  Layer *window_layer = window_get_root_layer(window);
  
  airport_layer = layer_create(GRect(0, 0, 144, 168));
  layer_set_update_proc(airport_layer, airport_draw_proc);
  layer_add_child(window_layer, airport_layer);
  
  fix_layer = layer_create(GRect(0, 0, 144, 168));
  layer_set_update_proc(fix_layer, fix_draw_proc);
  layer_add_child(window_layer, fix_layer);
  
  plane_layer = layer_create(GRect(0, 0, 144, 168));
  layer_set_update_proc(plane_layer, plane_update_proc);
  layer_add_child(window_layer, plane_layer);
  
  app_layer = text_layer_create(GRect(0, 0, 144, 168));
  text_layer_set_text_alignment(app_layer, GTextAlignmentCenter);
  text_layer_set_text_color(app_layer, GColorFromHEX(0x00FF00));
  text_layer_set_background_color(app_layer, GColorClear);
  text_layer_set_font(app_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  layer_add_child(window_layer, text_layer_get_layer(app_layer));
}

static void main_window_unload() {
  layer_destroy(airport_layer);
  layer_destroy(fix_layer);
  gpath_destroy(plane_1_path);
  text_layer_destroy(app_layer);
}
/*
static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  
}



static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  
}
*/

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  select_show_menu();
}

static int app_progress = 0;

static void app_move(void *data) {
  
  if (app_progress != 3) {
    switch(app_progress) {
      case 0:
        gpath_move_to(plane_1_path, GPoint(110, 60));
        app_progress++;
        plane_1_app_timer = app_timer_register(2000, app_move, NULL);
        break;
      case 1:
        gpath_move_to(plane_1_path, GPoint(96, 63));
        app_progress++;
        plane_1_app_timer = app_timer_register(2000, app_move, NULL);
        break;
      case 2:
        gpath_move_to(plane_1_path, GPoint(83, 66));
        app_progress++;
        plane_1_app_timer = app_timer_register(2000, app_move, NULL);
        break;
    }
    layer_mark_dirty(window_get_root_layer(window));
  } else {
    app_timer_cancel(plane_1_app_timer);
    gpath_move_to(plane_1_path, GPoint(180, 180));
    gpath_destroy(plane_1_path);
    layer_mark_dirty(window_get_root_layer(window));
  }
  
}

static void clear_approach(int plane) {
  rotate_plane(plane, 190);
  app_timer_cancel(plane_1_timer);
  app_move(NULL);
  plane_1_app_timer = app_timer_register(1500, app_move, NULL);
}

static void remove_app_text(void *data) {
  text_layer_set_text(app_layer, "");
}

static void select_long_handler(ClickRecognizerRef recognizer, void *context) {
  if (plane_1_x > 105) {
    if (plane_1_x < 115) {
      if (plane_1_y > 55) {
        if (plane_1_y < 65) {
          clear_approach(1);
          APP_LOG(APP_LOG_LEVEL_DEBUG, "Cleared for approach!");
          text_layer_set_text(app_layer, "Cleared for approach!");
          app_timer_text = app_timer_register(2500, remove_app_text, NULL);
        }
      }
    }
  }
}

static void click_config_provider(void *context) {
	//window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_long_click_subscribe(BUTTON_ID_SELECT, 700, select_long_handler, NULL);
  //window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

static void init() {
  window = window_create();
  
  window_set_background_color(window, GColorBlack);
  
  window_set_window_handlers(window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  
  window_set_click_config_provider(window, click_config_provider);
  
  window_stack_push(window, true);
  
  int temp_loc = rand() % 2;
  
  plane_1_path = gpath_create(&plane_1_points);
  
  switch(temp_loc) {
    case 0:
      plane_1_x = 143;
      plane_1_y = 142;
      gpath_move_to(plane_1_path, GPoint(143, 142));
      rotate_plane(1, 315);
      break;
    case 1:
      plane_1_x = 126;
      plane_1_y = 1;
      gpath_move_to(plane_1_path, GPoint(126, 1));
      rotate_plane(1, 225);
      break;
    case 2:
      plane_1_x = 1;
      plane_1_y = 132;
      gpath_move_to(plane_1_path, GPoint(1, 132));
      rotate_plane(1, 90);
      break;
  }
  
  
  
  plane_1_timer = app_timer_register(2000, plane_1_timer_handler, NULL);
  move_plane(1); 
}

static void deinit() {
  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
  return 0;
}