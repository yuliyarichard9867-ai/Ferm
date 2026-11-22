#ifndef UTILS_H
#define UTILS_H

#define MAX_LINE_LEN      256
#define OUTPUT_BUF_LEN    1024

#define OK                0
#define ERR_INPUT        -1
#define ERR_NOT_FOUND    -3

int read_line(char* buf, int size);
int read_int_range(int min, int max, int* out);

void        items_load(const char* filename);
const char* item_name(int id);
int         find_item_id_by_name(const char* query);

void write_diary(const char* farmer_name, int day, int hour,
    const int* inventory, int inv_size);

void process_input_file(void);

#endif /* UTILS_H */