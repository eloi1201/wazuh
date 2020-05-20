#ifndef __WMODULES_SCHEDULING_HELPERS_H__
#define __WMODULES_SCHEDULING_HELPERS_H__

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "shared.h"
#include "wazuh_modules/wmodules.h"

typedef struct test_structure {
    wmodule *module;
    OS_XML xml;
    XML_NODE nodes;
} test_structure;

const XML_NODE string_to_xml_node(const char * string, OS_XML *_lxml);
sched_scan_config init_config_from_string(const char* string);

void disable_forever_loop();
void enable_forever_loop();

/* Sets current simulation time */
void set_current_time(time_t _time);

/**
 *  Since module run is in a loop we pass a function ptr 
 * to use when cut condition is met in wrapped funcion
 * */
void (*check_function_ptr)(const sched_scan_config *scan_config, struct tm *date_array, unsigned int MAX_DATES);

void check_time_interval(const sched_scan_config *scan_config,struct tm *date_array, unsigned int MAX_DATES);
void check_day_of_month(const sched_scan_config *scan_config, struct tm *date_array, unsigned int MAX_DATES);
void check_day_of_week(const sched_scan_config *scan_config, struct tm *date_array, unsigned int MAX_DATES);
void check_time_of_day(const sched_scan_config *scan_config, struct tm *date_array, unsigned int MAX_DATES);

#endif
