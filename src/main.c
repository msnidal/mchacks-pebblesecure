#include <pebble.h>
  
static Window *s_main_window;
static TextLayer *s_output_layer;
static TextLayer *set_time;
static TextLayer *header_text;
static const uint32_t const segments[] = { 200, 100, 400 };
static VibePattern vibe_pattern = {
    .durations = segments,
    .num_segments = 3,
};


//timer
int TIME=0;
int second=0;
int minute=0;
bool can_tick = false;
// Time since last major movement
static int s_stationary_time = 0;
//combiner
int x_accel, y_accel, z_accel;
int deltaX, deltaY, deltaZ;

//timer
static void update_time() {
    //buffer
    static char TIME_buffer[]="00:00";
    second = (TIME % 60);
    minute = (TIME % 3600)/60;
    
    //time format
    snprintf(TIME_buffer, sizeof("000:000"), "%dm:%ds", minute, second);
    
    // Display this time on the TextLayer
    text_layer_set_text(set_time, TIME_buffer);
    
    // If time is above zero, for each second subtract a second. If time is at zero, send a notification pulse and disable tick
    if (can_tick) {
        if (TIME > 0)
            TIME--;
        else {
            TIME = 0;
            vibes_double_pulse();
            can_tick = false;
        }
    }
}

//main window load
static void main_window_load(Window *window){
    Layer *window_layer = window_get_root_layer(window);
    GRect window_bounds = layer_get_bounds(window_layer);
  
    // Create header text layer update
    s_output_layer = text_layer_create(GRect(0, 26, window_bounds.size.w, window_bounds.size.h));
    text_layer_set_text_alignment(s_output_layer, GTextAlignmentCenter);
    text_layer_set_text(s_output_layer,"Waiting for update...");
    layer_add_child(window_layer, text_layer_get_layer(s_output_layer));
  
    //Create a TextLayer
    header_text = text_layer_create(GRect(0,0,window_bounds.size.w,24));
    text_layer_set_background_color(header_text, GColorBlack);
    text_layer_set_text_color(header_text, GColorClear);
    text_layer_set_text(header_text, "PebSec");
    
    //Set the font
    text_layer_set_font(header_text, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
    text_layer_set_text_alignment(header_text, GTextAlignmentCenter);
    
    //Add the display to the window
    layer_add_child(window_layer, text_layer_get_layer(header_text));
    
    //Create a time TextLayer
    set_time = text_layer_create(GRect(0,50,window_bounds.size.w,window_bounds.size.h));
    text_layer_set_background_color(set_time, GColorClear);
    text_layer_set_text_color(set_time, GColorBlack);
    text_layer_set_text(set_time, "00:00");
    
    //Set the time font
    text_layer_set_font(set_time, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
    text_layer_set_text_alignment(set_time, GTextAlignmentCenter);
    
    //Add the time display to the window
    layer_add_child(window_layer, text_layer_get_layer(set_time));
}

// Pause or release ticking function for centre button
static void Select_Function(ClickRecognizerRef activator, void *directives){
    if (can_tick)
        can_tick = false;
    else
        can_tick = true;
    
}

// Add 5 minutes to a max of 30 min for up button
static void Up_Function(ClickRecognizerRef activator, void *directives){
    if(TIME<3600)
        TIME+=300;
}

// Subtract 5 minutes to a minimum of 0 for down button
static void Down_Function(ClickRecognizerRef activator, void *directives){
    if(TIME>300)
        TIME-=300;
    if (TIME > 0 && TIME <= 300)
        TIME = 0;
}

// Custom event when watch is still for too long
static void Still_Function() {
    vibes_enqueue_custom_pattern(vibe_pattern);
}

static void click_config_provider(Window *window) {
    window_single_click_subscribe(BUTTON_ID_UP, Up_Function);
    window_single_click_subscribe(BUTTON_ID_SELECT, Select_Function);
    window_single_click_subscribe(BUTTON_ID_DOWN, Down_Function);
}

static void main_window_unload(Window *window){
    //Destroy TextLayer
    text_layer_destroy(header_text);
    text_layer_destroy(set_time);
    text_layer_destroy(s_output_layer);
}

//accelerator
static void data_handler(AccelData *data, uint32_t num_samples) 
{
    x_accel = deltaX;
    y_accel = deltaY;
    z_accel = deltaZ;

    deltaX = data[0].x;
    deltaY = data[0].y;
    deltaZ = data[0].z;
    
    x_accel -= deltaX;
    y_accel -= deltaY;
    z_accel -= deltaZ;  
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {  
        update_time();

    if (s_stationary_time > 29) {
        Still_Function();
        s_stationary_time = 0;
    }
    
    if ((x_accel < 20 && x_accel > -20) && (y_accel < 20 && y_accel > -20) && (z_accel < 50 && z_accel > -50))
        s_stationary_time++;
    else
        s_stationary_time = 0;
    
    static char s_buffer[128];
    snprintf(s_buffer, sizeof(s_buffer), "Stationary time: %d",s_stationary_time);
   
    text_layer_set_text(s_output_layer, s_buffer);
    
//    if (s_stationary_time == 120)
//      sentAlert();
}

//init and deinit
static void init() {
  
    // Create main window, provide config
    s_main_window = window_create();
    window_set_click_config_provider(s_main_window, (ClickConfigProvider) click_config_provider);
    window_set_window_handlers(s_main_window, (WindowHandlers){
        .load = main_window_load,
        .unload=main_window_unload
    });
    window_stack_push(s_main_window, true);
    
    update_time();
    
    // Subscribe to second tick event
    tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
    
    // Subscribe to accelerometer data using 10 samples per second to save battery life
    uint32_t num_samples = 1;
    accel_data_service_subscribe(num_samples, data_handler);
    accel_service_set_sampling_rate(ACCEL_SAMPLING_10HZ);
}

static void deinit() {
  // Destroy Window
  window_destroy(s_main_window);

}
int main(void) {
    init();
    app_event_loop();
    deinit();
}
