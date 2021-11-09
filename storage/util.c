//
// Created by Anna.Kozhemyako on 06.11.2021.
//

#include "util.h"

bool is_file_empty(FILE *file) {
    if (file != NULL) {
        fseek (file, 0, SEEK_END);
        long size = ftell(file);
        return size == 0;
    }
    return false;
}
