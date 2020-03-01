/*
   Copyright (c) 2020 Stefan Kremser (@Spacehuhn)
   This software is licensed under the MIT License. See the license file for details.
   Source: github.com/spacehuhn/esp8266_deauther
 */

#include "StringList.h"

// ========== StringList ========== //

char* StringList::stringCopy(const char* str, unsigned long len) const {
    char* newstr = (char*)malloc(len+1);

    memcpy(newstr, str, len);
    newstr[len] = '\0';

    return newstr;
}

int StringList::compare(const str_t* a, const String& b) const {
    return strcmp(a->ptr, b.c_str());
}

int StringList::compare(const str_t* a, const str_t* b) const {
    return strcmp(a->ptr, b->ptr);
}

StringList::StringList(int max) : list_max_size(max) {}

StringList::StringList(const String& input, String delimiter) {
    parse(input, delimiter);
}

StringList::~StringList() {
    clear();
}

void StringList::moveFrom(StringList& sl) {
    str_t* ih = sl.list_begin;

    while (ih) {
        if ((list_max_size > 0) && (list_size >= list_max_size)) break;

        // Push to list
        if (!list_begin) {
            list_begin = ih;
            list_end   = ih;
            list_h     = list_begin;
        } else {
            list_end->next = ih;
            list_end       = ih;
        }

        ++(list_size);

        ih = ih->next;
    }

    sl.list_begin = NULL;
    sl.list_end   = NULL;
    sl.list_size  = 0;
    sl.list_h     = NULL;
    sl.list_pos   = 0;
}

bool StringList::push(String str) {
    return push(str.c_str(), str.length());
}

bool StringList::push(const char* str, unsigned long len) {
    if ((list_max_size > 0) && (list_size >= list_max_size)) return false;

    str_t* new_str = (str_t*)malloc(sizeof(str_t));

    new_str->ptr  = stringCopy(str, len);
    new_str->next = NULL;

    if (!list_begin) {
        list_begin = new_str;
        list_end   = new_str;
        list_h     = list_begin;
    } else {
        list_end->next = new_str;
        list_end       = new_str;
    }

    ++list_size;
    return true;
}

String StringList::popFirst() {
    String str(list_begin->ptr);

    str_t* next = list_begin->next;

    free(list_begin);

    if (next) {
        list_begin = next;
        list_h     = list_begin;
    } else {
        list_begin = NULL;
        list_end   = NULL;
        list_h     = NULL;
    }

    list_pos = 0;

    return str;
}

void StringList::parse(const String& input, String delimiter) {
    int len           = input.length();
    int delimiter_len = delimiter.length();
    int j             = 0;
    const char* ptr   = input.c_str();

    for (int i = 0; i <= len; ++i) {
        if ((i-j > 0) && ((i == len) || (input.substring(i, i+delimiter_len) == delimiter))) {
            push(&ptr[j], i-j);
            j = i+delimiter_len;
        }
    }
}

String StringList::get(int i) {
    if (i < list_pos) begin();

    while (list_h && list_pos<i) iterate();

    return String(list_h ? list_h->ptr : "");
}

void StringList::begin() {
    list_h   = list_begin;
    list_pos = 0;
}

String StringList::iterate() {
    String res(list_h ? list_h->ptr : "");

    if (list_h) {
        list_h = list_h->next;
        ++list_pos;
    }

    return res;
}

bool StringList::contains(const String& str) const {
    str_t* tmp = list_begin;

    while (tmp && compare(tmp, str) != 0) {
        tmp = tmp->next;
    }

    return tmp;
}

bool StringList::available() const {
    return list_h;
}

int StringList::size() const {
    return list_size;
}

bool StringList::full() const {
    return list_max_size > 0 && list_size >= list_max_size;
}

void StringList::clear() {
    str_t* tmp = list_begin;

    while (tmp) {
        str_t* to_delete = tmp;
        tmp = tmp->next;
        free(to_delete->ptr);
        free(to_delete);
    }

    list_begin = NULL;
    list_end   = NULL;
    list_size  = 0;

    list_h   = NULL;
    list_pos = 0;
}

// ========== SortedStringList ========== //

SortedStringList::SortedStringList(int max) : StringList(max) {}

bool SortedStringList::push(const char* str, unsigned long len) {
    if ((list_max_size > 0) && (list_size >= list_max_size)) return false;

    str_t* new_str = (str_t*)malloc(sizeof(str_t));

    new_str->ptr  = stringCopy(str, len);
    new_str->next = NULL;

    // Empty list -> insert first element
    if (!list_begin) {
        list_begin = new_str;
        list_end   = new_str;
        list_h     = list_begin;
    } else {
        // Insert at start
        if (compare(list_begin, new_str) > 0) {
            new_str->next = list_begin;
            list_begin    = new_str;
        }
        // Insert at end
        else if (compare(list_end, new_str) < 0) {
            list_end->next = new_str;
            list_end       = new_str;
        }
        // Insert somewhere in the between (insertion sort)
        else {
            str_t* tmp_c = list_begin;
            str_t* tmp_p = NULL;

            int res = compare(tmp_c, new_str);

            while (tmp_c->next && res < 0) {
                tmp_p = tmp_c;
                tmp_c = tmp_c->next;
                res   = compare(tmp_c, new_str);
            }

            // Skip duplicates
            if (res == 0) {
                free(new_str->ptr);
                free(new_str);
                return false;
            } else {
                new_str->next = tmp_c;
                tmp_p->next   = new_str;
            }
        }
    }

    ++list_size;
    return true;
}

bool SortedStringList::contains(const String& str) const {
    if ((list_size == 0) || (compare(list_begin, str) > 0) || (compare(list_end, str) < 0)) {
        return false;
    }

    str_t* tmp = list_begin;

    int res = compare(tmp, str);

    while (tmp->next && res < 0) {
        tmp = tmp->next;
        res = compare(tmp, str);
    }

    return res == 0;
}