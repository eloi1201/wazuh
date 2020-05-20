#include "shared.h"
#include "wazuh_modules/wmodules.h"


static const char *XML_INTERVAL = "interval";
static const char *XML_SCAN_DAY = "day";
static const char *XML_WEEK_DAY = "wday";
static const char *XML_TIME = "time";

#ifdef UNIT_TESTING
// Remove static for unit testing
#define static
#endif

static int _sched_scan_validate_parameters(sched_scan_config *scan_config);
static time_t _get_next_time(const sched_scan_config *config, const char *MODULE_TAG,  const int run_on_start);

/**
 * Check if the input tag is used for scheduling
 * */
int is_sched_tag(const char* tag){
    return !strcmp(tag, XML_INTERVAL) || !strcmp(tag, XML_SCAN_DAY) || !strcmp(tag, XML_WEEK_DAY) || !strcmp(tag, XML_TIME);
}

/**
 * Initializes sched_scan_config structure with 
 * default values
 * */
void sched_scan_init(sched_scan_config *scan_config){
    scan_config->scan_wday = -1;
    scan_config->scan_day = 0;
    scan_config->scan_time = NULL;
    scan_config->interval = WM_DEF_INTERVAL;
    scan_config->month_interval = false;
    scan_config->time_start = 0;
    scan_config->next_scheduled_scan_time = 0;
}

/**
 * Frees sched_scan_config internal variables
 * */
void sched_scan_free(sched_scan_config *scan_config){
    os_free(scan_config->scan_time);
}

int sched_scan_read(sched_scan_config *scan_config, xml_node **nodes, const char *MODULE_NAME) {
    unsigned i;
    for (i = 0; nodes[i]; i++) {
        if (!strcmp(nodes[i]->element, XML_SCAN_DAY)) { // <day></day>
            if (!OS_StrIsNum(nodes[i]->content)) {
                merror(XML_VALUEERR, nodes[i]->element, nodes[i]->content);
                return (OS_INVALID);
            } else {
                scan_config->scan_day = atoi(nodes[i]->content);
                if (scan_config->scan_day < 1 || scan_config->scan_day > 31) {
                    merror(XML_VALUEERR, nodes[i]->element, nodes[i]->content);
                    return (OS_INVALID);
                }
            }
        } else if (!strcmp(nodes[i]->element, XML_WEEK_DAY)) { // <wday></wday>
            scan_config->scan_wday = w_validate_wday(nodes[i]->content);
            if (scan_config->scan_wday < 0 || scan_config->scan_wday > 6) {
                merror(XML_VALUEERR, nodes[i]->element, nodes[i]->content);
                return OS_INVALID;
            }
        } else if (!strcmp(nodes[i]->element, XML_TIME)) {  // <time></time>
            scan_config->scan_time = w_validate_time(nodes[i]->content);
            if (!scan_config->scan_time) {
                merror(XML_VALUEERR, nodes[i]->element, nodes[i]->content);
                return (OS_INVALID);
            }
        } else if (!strcmp(nodes[i]->element, XML_INTERVAL)) { //<interval></interval>
            char *endptr;
            scan_config->interval = strtoul(nodes[i]->content, &endptr, 0);

            if (scan_config->interval <= 0 || scan_config->interval >= UINT_MAX) {
                merror("Invalid interval value at module '%s'", MODULE_NAME);
                return OS_INVALID;
            }

            switch (*endptr) {
                case 'M':
                    scan_config->month_interval = true;
                    // interval will be considered as a month when this option is active
                    break;
                case 'w':
                    scan_config->interval *= 604800;
                    break;
                case 'd':
                    scan_config->interval *= 86400;
                    break;
                case 'h':
                    scan_config->interval *= 3600;
                    break;
                case 'm':
                    scan_config->interval *= 60;
                    break;
                case 's':
                case '\0':
                    break;
                default:
                    merror("Invalid interval value at module '%s'", MODULE_NAME);
                    return OS_INVALID;
            }
        }
    }

    return _sched_scan_validate_parameters(scan_config);
}

time_t sched_scan_get_time_until_next_scan(sched_scan_config *config, const char *MODULE_TAG,  const int run_on_start) {
    const time_t next_time = _get_next_time(config, MODULE_TAG, run_on_start); 
    config->next_scheduled_scan_time = time(NULL) + next_time;
    return next_time;
}

static time_t _get_next_time(const sched_scan_config *config, const char *MODULE_TAG,  const int run_on_start) {
    if (run_on_start && !config->next_scheduled_scan_time) {
        // If scan on start then initial waiting time is 0
        return 0;
    }

    if (config->scan_day) {
        // Option 1: Day of the month
        return (time_t) get_time_to_month_day(config->scan_day,  config->scan_time, config->interval);
    } else if (config->scan_wday >= 0) {
        unsigned int num_weeks = config->interval / 604800;
        // Option 2: Day of the week
        return (time_t) get_time_to_day(config->scan_wday, config->scan_time, num_weeks, config->next_scheduled_scan_time ? FALSE : TRUE);
    } else if (config->scan_time) {
        unsigned int num_days = config->interval / 86400;
        // Option 3: Time of the day [hh:mm]
        return (time_t) get_time_to_hour(config->scan_time, num_days, config->next_scheduled_scan_time ? FALSE : TRUE);
    } else if (config->interval) {
        // Option 4: Interval of time
        const time_t last_run_time = time(NULL) - config->next_scheduled_scan_time;

        if ((time_t)config->interval >= last_run_time) {
            return  (time_t)config->interval - last_run_time;
        } else if(!config->next_scheduled_scan_time) { 
            // First time defined by run_on_start
            if(run_on_start) {
                return 0;
            } else {  
                return (time_t)config->interval;
            }
        } else {
            mtwarn(MODULE_TAG, "Interval overtaken.");
            return 0;
        }
    } else {
        merror_exit("Invalid Scheduling option for module %s. Exiting.", MODULE_TAG);
    }
    return 0;
}


static int _sched_scan_validate_parameters(sched_scan_config *scan_config) {
    // Validate scheduled scan parameters and interval value
    if (scan_config->scan_day && (scan_config->scan_wday >= 0)) {
        merror("Options 'day' and 'wday' are not compatible.");
        return OS_INVALID;
    } else if (scan_config->scan_day) {
        if (!scan_config->month_interval) {
            mwarn("Interval must be a multiple of one month. New interval value: 1M");
            scan_config->interval = 1; // 1 month
            scan_config->month_interval = true;
        }
        if (!scan_config->scan_time)
            scan_config->scan_time = strdup("00:00");
    } else if (scan_config->scan_wday >= 0) {
        if (w_validate_interval(scan_config->interval, 1) != 0) {
            scan_config->interval = 604800;  // 1 week
            mwarn("Interval must be a multiple of one week. New interval value: 1w");
        }
        if (scan_config->interval == 0)
            scan_config->interval = 604800;
        if (!scan_config->scan_time)
            scan_config->scan_time = strdup("00:00");
    } else if (scan_config->scan_time) {
        if (w_validate_interval(scan_config->interval, 0) != 0) {
            scan_config->interval = WM_DEF_INTERVAL;  // 1 day
            mwarn("Interval must be a multiple of one day. New interval value: 1d");
        }
    } else if (scan_config->month_interval) {
        mwarn("Interval value is in months. Setting scan day to first day of the month.");
        scan_config->scan_day = 1;
        scan_config->scan_time = strdup("00:00");
    }

    return 0;
}

void sched_scan_dump(const sched_scan_config* scan_config, cJSON *cjson_object){
    if (scan_config->interval) cJSON_AddNumberToObject(cjson_object, "interval", scan_config->interval);
    if (scan_config->scan_day) cJSON_AddNumberToObject(cjson_object, "day", scan_config->scan_day);
    switch (scan_config->scan_wday) {
        case 0:
            cJSON_AddStringToObject(cjson_object, "wday", "sunday");
            break;
        case 1:
            cJSON_AddStringToObject(cjson_object, "wday", "monday");
            break;
        case 2:
            cJSON_AddStringToObject(cjson_object, "wday", "tuesday");
            break;
        case 3:
            cJSON_AddStringToObject(cjson_object, "wday", "wednesday");
            break;
        case 4:
            cJSON_AddStringToObject(cjson_object, "wday", "thursday");
            break;
        case 5:
            cJSON_AddStringToObject(cjson_object, "wday", "friday");
            break;
        case 6:
            cJSON_AddStringToObject(cjson_object, "wday", "saturday");
            break;
        default:
            break;
    }
    if (scan_config->scan_time) cJSON_AddStringToObject(cjson_object, "time", scan_config->scan_time);

}
