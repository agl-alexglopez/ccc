#ifndef CLI
#define CLI

enum conversion_status
{
    CONV_OK,
    CONV_ER,
};

struct int_conversion
{
    enum conversion_status status;
    int conversion;
};

/* Convert the provided text to an integer. Argument must be a valid
   integer greater than or equal to INT_MIN and less than or equal to INT_MAX */
struct int_conversion convert_to_int(const char *);
void quit(const char *, int);

void clear_screen(void);
void clear_line(void);
void set_cursor_position(int row, int col);

#endif
