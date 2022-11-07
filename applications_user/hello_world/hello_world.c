#include <furi.h>
#include <gui/gui.h>
#include <input/input.h>
#include <stdlib.h>

#define TAG "Hello World"
typedef enum {
    EventTypeKey,
    EventTypeTick,
} EventType;

typedef struct {
    EventType type;
    InputEvent input;
} iMetroEvent;

typedef struct {
    uint16_t x;
    uint16_t y;
} iMetroState;

static void hello_world_render_callback(Canvas* const canvas, void* ctx) {
    const iMetroState* imetro_state = acquire_mutex((ValueMutex*)ctx, 25);
    if(imetro_state == NULL) {
        return;
    }
    // border around the edge of the screen
    canvas_draw_frame(canvas, 0, 0, 128, 64);

    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str_aligned(canvas, imetro_state->x, imetro_state->y, AlignRight, AlignBottom, "Hello World");

    release_mutex((ValueMutex*)ctx, imetro_state);
}

static void hello_world_input_callback(InputEvent* input_event, FuriMessageQueue* event_queue) {
    furi_assert(event_queue);

    iMetroEvent event = {.type = EventTypeKey, .input = *input_event};
    furi_message_queue_put(event_queue, &event, FuriWaitForever);
}

static void hello_world_state_init(iMetroState* const imetro_state) {
    imetro_state->x = 50;
    imetro_state->y = 30;
}

int32_t hello_world_app(void* p) {
    UNUSED(p);
    // srand(DWT->CYCCNT); //not sure what it does but was on snake_game. Looks like a randomizer, prob unused here

    FuriMessageQueue* event_queue = furi_message_queue_alloc(8, sizeof(iMetroEvent));

    iMetroState* imetro_state = malloc(sizeof(iMetroState));
    hello_world_state_init(imetro_state);

    ValueMutex state_mutex;
    if (!init_mutex(&state_mutex, imetro_state, sizeof(iMetroState))) {
        FURI_LOG_E("Hello_world", "cannot create mutex\r\n");
        free(imetro_state);
        return 255;
    }

    // Set system callbacks
    ViewPort* view_port = view_port_alloc();
    view_port_draw_callback_set(view_port, hello_world_render_callback, &state_mutex);
    view_port_input_callback_set(view_port, hello_world_input_callback, event_queue);

    // Open GUI and register view_port
    Gui* gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);

    iMetroEvent event;
    for(bool processing = true; processing;) {
        FuriStatus event_status = furi_message_queue_get(event_queue, &event, 100);

        iMetroState* imetro_state = (iMetroState*)acquire_mutex_block(&state_mutex);

        if(event_status == FuriStatusOk) {
            // press events
            if(event.type == EventTypeKey) {
                if(event.input.type == InputTypePress) {
                    switch(event.input.key) {
                    case InputKeyUp:
                        imetro_state->y--;
                        break;
                    case InputKeyDown:
                        imetro_state->y++;
                        break;
                    case InputKeyRight:
                        imetro_state->x++;
                        break;
                    case InputKeyLeft:
                        imetro_state->x--;
                        break;
                    case InputKeyOk:
                        break;
                    case InputKeyBack:
                        processing = false;
                        break;
                    default:
                        break;
                    }
                }
            }
        } else {
            // FURI_LOG_D("Hello_world", "osMessageQueue: event timeout"); //carefull
            // event timeout on test
        }

        view_port_update(view_port);
        release_mutex(&state_mutex, imetro_state);
    }

    view_port_enabled_set(view_port, false);
    gui_remove_view_port(gui, view_port);
    furi_record_close(RECORD_GUI);
    view_port_free(view_port);
    furi_message_queue_free(event_queue);
    //delete_mutex(&state_mutex); //carefull(not tested)
    //free(imetro_state); // carefull

    return 0;
}
