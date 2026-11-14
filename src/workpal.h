#ifndef PIX86_WORKPAL_H
#define PIX86_WORKPAL_H
#define WORKPAL_COUNT 16
void workpal_init(void);
unsigned char workpal_colour(int slot);
/* Eight stable defaults plus eight recent colours. Never remap artwork. */
void workpal_remember(unsigned char colour);
unsigned char workpal_step(unsigned char colour,int direction);
int workpal_more(int x,int y);
/* Full popup hit geometry returns the exact stored VGA index. */
int workpal_full_hit(int x,int y,unsigned char *colour);
#endif
