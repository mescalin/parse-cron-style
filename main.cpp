#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <map>
#include <string>
#include <vector>

using namespace std;

static bool toint(const char* str, int base, int* val) {
    *val = (int)strtol(str, NULL, base);
    return errno != ERANGE;
}

/*
 * 月 日 周 小时 分钟 秒
 * 年跨度太大，秒太小，都不适合放在配置中
 * 以下说明中的实例皆以 `小时` 为说明
 * * 代表每一单位时刻，例如 * 每一个整点开始时刻
 * , 代表单独时刻, 例如 1,2,6 代表 1点 2点 和 6点
 * - 代表从几到几, 例如 3-7 代表 3点到7点
 * / 代表每多少，例如 /2 代表从 0 点每两小时
 */

class TimeFormat {
public:
    class TimeField {
    public:
        enum {
            NONE = -1,
            ANY = 1, // *
            FIXED = 2, // xx
            RANGE = 3, // xx-xx
            EVERY = 4, // /xx
            SPLITED = 5, // xx,xx,xx
        };

        int type;
        vector<int> vals;

        TimeField() {
            type = NONE;
        }

        bool _parse(const char* fmt);
        bool parse(const char* fmt);
        bool triggerP(int val);
        void print(const char* prefix);
    };

    TimeField month; //[0,11], months since January
    TimeField mday;  //[0,31]
    TimeField wday;  //[0,6], days since Sunday
    TimeField hour;  //[0,23]
    TimeField min;   //[0,59]

    bool parse(const char* fmt);
    bool triggerP(time_t val);
    void print();
};

bool TimeFormat:: parse(const char* fmt) {
    const int expected = 5;
    char buf[expected][256]; //被空格分割的每一项的长度不能超过 256
    int ret = sscanf(fmt, "%s %s %s %s %s %s", buf[0], buf[1], buf[2], buf[3], buf[4]);
    if(ret != expected)
        return false;

    TimeFormat tmp;
    if(!tmp.month.parse(buf[0]))
        return false;
    if(!tmp.mday.parse(buf[1]))
        return false;
    if(!tmp.wday.parse(buf[2]))
        return false;
    if(!tmp.hour.parse(buf[3]))
        return false;
    if(!tmp.min.parse(buf[4]))
        return false;

    *this = tmp; //全部成功才赋值，避免污染 ft
    return true;
}

bool TimeFormat::triggerP(time_t val) {
    struct tm t;
    if(localtime_r(&val, &t) == NULL)
        return false;
    if(!month.triggerP(t.tm_mon))
        return false;
    if(!mday.triggerP(t.tm_mday))
        return false;
    if(!wday.triggerP(t.tm_wday))
        return false;
    if(!hour.triggerP(t.tm_hour))
        return false;
    if(!min.triggerP(t.tm_min))
        return false;
    return true;
}

void TimeFormat::print() {
    month.print("month");
    mday.print("mday");
    wday.print("wday");
    hour.print("hour");
    min.print("min");
}

bool TimeFormat::TimeField::_parse(const char* fmt) {
    if(fmt[0] == '/') { //EVERY
        int val;
        if(!toint(fmt + 1, 10, &val))
            return false;
        if(val == 0)
            return false;
        type = EVERY;
        vals.push_back(val);
    } else if(fmt[0] == '*') { //ANY
        type = ANY;
    } else {
        int lastindex = 0;
        bool founded = false; //found '-' or ','
        int fmt_len = strlen(fmt);
        for(int n = 0; n < fmt_len; ++n) {
            if(fmt[n] == '-') { //RANGE
                founded = true; //set founded
                if(n == 0)
                    return false;

                int val, val2;
                ((char*)fmt)[n] = '\0';
                bool ret = (toint(fmt, 10, &val) && toint(fmt + n + 1, 10, &val2));
                if(!ret)
                    return false;

                vals.push_back(val);
                vals.push_back(val2);
                type = RANGE;

                ((char*)fmt)[n] = '-'; //reset '-'
                break;
            }

            if(fmt[n] == ',' || ((n == fmt_len - 1) && type == SPLITED)) { //SPLITED
                founded = true; //set founded

                const int limit = 5;
                char temp[limit] = {0};
                int len = (fmt[n] == ',') ? (n - lastindex) : (n - lastindex + 1);
                if(len >= limit)
                    return false;
                memcpy(temp, fmt + lastindex, len);
                lastindex = n + 1; //update lastindex

                int val;
                if(!toint(temp, 10, &val))
                    return false;
                vals.push_back(val);
                type = SPLITED;
            }
        } //for(int n = 0; n < strlen(fmt); ++n) {

        if(!founded) { //FIXED
            int val;
            if(!toint(fmt, 10, &val))
                return false;
            vals.push_back(val);
            type = FIXED;
        }
    } //else
    return true;
}

bool TimeFormat::TimeField::parse(const char* fmt) {
    bool ret = _parse(fmt);
    if(!ret) {
        type = NONE;
        vals.clear();
    }
    return ret;
}

bool TimeFormat::TimeField::triggerP(int val) {
    switch(type) {
    case NONE:
        return false;
    case ANY:
        return true;
    case FIXED:
        if(vals.size() != 1)
            return false;
        return val == vals[0];
    case RANGE:
        if(vals.size() != 2)
            return false;
        return (val >= vals[0] && val <= vals[1]);
    case EVERY:
        if(vals.size() != 1)
            return false;
        if(vals[0] == 1)
            return true;
        return val % vals[0] == 0;
    case SPLITED:
        for(vector<int>::iterator it = vals.begin(); it != vals.end(); ++it) {
            if(*it == val)
                return true;
        }
        return false;
    }
    return false;
}

void TimeFormat::TimeField::print(const char* prefix) {
    string str;
    switch(type) {
    case NONE:
        str = "None";
        break;
    case ANY:
        str = "Any";
        break;
    case FIXED:
        str ="Fixed";
        break;
    case RANGE:
        str = "Range";
        break;
    case EVERY:
        str = "Every";
        break;
    case SPLITED:
        str = "Splited";
        break;
    default:
        str = "Unkown";
    }
    printf("%s: type(%s), val(", prefix, str.c_str());
    for(vector<int>::iterator it = vals.begin(); it != vals.end(); ++it) {
        printf("%d,", *it);
    }
    printf(")\n");
}

int main(int argc, const char** argv) {
    const char* fmt = "00-03 * 0 20 /20"; //1-4月 每天 周7 20点 每20分钟
    printf("fmt=%s\n", fmt);

    TimeFormat tf;
    if(!tf.parse(fmt))
        printf("parse error\n");
    tf.print();

    time_t start = time(NULL);
    int second_per_day = 60 * 60 * 24;
    int second_per_week = second_per_day * 7;
    int second_per_month = second_per_day * 31;
    int second_per_year = second_per_day * 366;
    for(time_t t = start; t < start + second_per_year; t += 60) { //单位为分钟
        if(tf.triggerP(t))
            printf("trigger: (%s) :%s", fmt, ctime(&t));
    }
    return 0;
}
