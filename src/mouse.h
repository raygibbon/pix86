#ifndef PIX86_MOUSE_H
#define PIX86_MOUSE_H
#define MOUSE_LEFT 1
#define MOUSE_RIGHT 2

typedef struct MouseState {
    int x, y;
    unsigned buttons;
} MouseState;
int mouse_init(void);
void mouse_poll(MouseState *state);
void mouse_shutdown(void);
#endif
