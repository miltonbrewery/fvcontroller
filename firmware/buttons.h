#ifndef _buttons_h
#define _buttons_h

#define K_UP 0x10
#define K_DOWN 0x20
#define K_ENTER 0x08
#define BUTTONS_MASK (K_UP|K_DOWN|K_ENTER)

extern void buttons_init(void);

/* To be called 10 times per second */
extern void buttons_poll(void);

extern uint8_t get_buttons(void);
extern void ack_buttons(void);

#endif /* _buttons_h */
