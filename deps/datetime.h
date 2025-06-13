#ifndef DATETIME_H
#define DATETIME_H

#include "datetime.h"

#include <bitset>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#define API_CALL
#define API_CALL

namespace datetime_utils {

enum weekday {
	sunday,
	monday,
	tuesday,
	wednesday,
	thursday,
	friday,
	saturday
};

enum period {
	undefined,
	AM,
	PM
};

class API_CALL timespan {
public:
	explicit timespan(int days, int hours = 0, int minutes = 0, int seconds = 0);
	int get_days() const;
	int get_hours() const;
	int get_minutes() const;
	int get_seconds() const;
	int get_total_hours() const;
	int get_total_minutes() const;
	int get_total_seconds() const;
	// Operators
	API_CALL friend bool operator<(const timespan &mts, const timespan &ots);
	API_CALL friend bool operator>(const timespan &mts, const timespan &ots);
	API_CALL friend bool operator<=(const timespan &mts, const timespan &ots);
	API_CALL friend bool operator>=(const timespan &mts, const timespan &ots);
	API_CALL friend bool operator==(const timespan &mts, const timespan &ots);
	API_CALL friend bool operator!=(const timespan &mts, const timespan &ots);

private:
	int days;
	int hours;
	int minutes;
	int seconds;
};

class API_CALL datetime {
public:
	datetime();
	datetime(int year, int month, int day, int hour = 0, int minute = 0, int second = 0, period day_period = period::undefined);
	datetime(const datetime &other); // Copy constructor
	datetime &operator=(const datetime &other); // Copy assignment
	datetime(datetime &&other) noexcept; // Move constructor
	datetime &operator=(datetime &&other) noexcept; // Move assignement
	virtual ~datetime();
	API_CALL friend std::ostream &operator<<(std::ostream &os, const datetime &dt);
	API_CALL friend bool operator<(const datetime &mdt, const datetime &odt);
	API_CALL friend bool operator>(const datetime &mdt, const datetime &odt);
	API_CALL friend bool operator<=(const datetime &mdt, const datetime &odt);
	API_CALL friend bool operator>=(const datetime &mdt, const datetime &odt);
	API_CALL friend bool operator==(const datetime &mdt, const datetime &odt);
	API_CALL friend bool operator!=(const datetime &mdt, const datetime &odt);
	API_CALL friend timespan operator-(const datetime &mdt, const datetime &odt);
	std::string to_string() const;
	std::string to_string(const std::string &format) const;
	std::string to_shortdate_string() const;
	int get_year() const;
	int get_month() const;
	int get_day() const;
	int get_hour() const;
	int get_minute() const;
	int get_second() const;
	weekday get_weekday() const;
	void add_years(int nb_years);
	void add_months(int nb_months);
	void add_days(int nb_days);
	void add_hours(int nb_hours);
	void add_minutes(int nb_minutes);
	void add_seconds(int nb_seconds);
	bool is_leapyear();
	static datetime parse(const std::string &format, const std::string &value);
	static bool is_leapyear(int year);

protected:
	const int ONE_DAY = 86400; // 24 hours * 60 mins * 60 secs
	const int ONE_HOUR = 3600; // 60 mins * 60 secs
	const int ONE_MINUTE = 60; // 60 secs
	struct tm *timeInfo = nullptr;
	bool auto_created = true;
	bool _is_leapyear(int year) const;
	static int _parse_intvalue(const std::string &pattern, int index, size_t mask_length, const std::string &parse_str);
	void _copy_from(const tm *otm);
};

namespace crontab {

typedef char unsigned byte;
typedef short unsigned int ushort;

#define SCOPE_OF_YEARS 8

enum field_name {
	second,
	minute,
	hour_of_day,
	day_of_month,
	month,
	day_of_week,
	year,
	fields_count,
	expr = 194 + SCOPE_OF_YEARS * 2 + 1
}; // Adressage : field_name::hour

static const byte field_size[] = { 60, 60, 24, 31, 12, 7, SCOPE_OF_YEARS * 2 + 1 }; // expr == sum ...
static byte field_offset[] = { 0, 0, 0, 1, 1, 0, SCOPE_OF_YEARS };
static const char *week_day[] = { "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT" };
static const char *month_name[] = { "JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC" };
static const byte npos = byte(-1);

API_CALL class cron : std::bitset<field_name::expr> {
	bool _err, _last_is_set;
	ushort _year;
	std::string _expression;

	cron(const std::tm *t) :
			_err(false) {
		init();
		assign(t);
	};
	inline void init(void) {
		time_t rawtime(time(NULL));
		_year = localtime(&rawtime)->tm_year;
	};

	cron &assign(const std::tm *);
	inline cron &assign(const time_t *t) { return assign(localtime(t)); };
	inline cron &operator=(const std::tm *t) { return assign(t); };
	inline cron &operator=(const time_t *t) { return assign(t); };

	byte index(field_name const);
	void set_field(field_name const, std::string, bool);
	inline void set_field(byte const n, std::string s, bool v) { return (set_field(field_name(n), s, v)); };
	inline void set_scope(field_name const nfield, bool v, byte begin, byte end, byte delta = 1) {
		for (byte i(begin); i <= end && !conv_error(); i += delta)
			set_bit(nfield, i - field_offset[nfield], v);
	};

	inline void set_bit(byte const nfield, byte i, bool v) { return set_bit(field_name(nfield), i - field_offset[nfield], v); };
	inline void set_bit(field_name const nfield, byte i, bool v) {
		if (existing_bit(nfield, i))
			this->set(index(nfield) + i, v);
		else
			conv_error(true);
	};
	inline bool set_field(field_name const nfield, bool v) {
		if (!existing_field(nfield))
			return false;
		for (byte i(index(nfield)), j(i + field_size[nfield]); i < j; i++)
			set(i, v);
		return true;
	};
	inline bool set_field(byte const nfield, bool v) { return (set_field(field_name(nfield), v)); };

	inline bool is_set(field_name const nfield, byte i) { return ((existing_field(nfield) && i < field_size[nfield]) ? test(index(nfield) + i) : false); };
	inline bool is_set(byte const nfield, byte i) { return (is_set(field_name(nfield), i - field_offset[nfield])); };
	inline bool is_set(field_name const nfield) {
		if (!existing_field(nfield))
			return false;
		for (byte i(0); i < field_size[nfield]; i++)
			if (!test(index(nfield) + i))
				return false;
		return true;
	};
	inline bool is_set(byte const nfield) { return (is_set(field_name(nfield))); };
	inline bool is_not_set(field_name const nfield) {
		if (existing_field(nfield))
			for (byte i(index(nfield)), j(i + field_size[nfield]); i < j; i++)
				if (test(i))
					return false;
		return true;
	};
	inline byte find_bit(field_name const nfield, byte n = 0) {
		if (!existing_field(nfield))
			return npos;
		for (byte i(n); i < field_size[nfield]; i++)
			if (is_set(nfield, i))
				return (i);
		return npos;
	};
	inline byte find_bit(byte const nfield, byte n = 0) { return (find_bit(field_name(nfield - field_offset[n]), n)); };
	inline bool existing_field(field_name const nfield) { return (nfield <= field_name::year); };
	inline bool existing_field(byte const nfield) { return (nfield <= field_name::year); };
	inline bool existing_bit(field_name const nfield, byte n) { return (existing_field(nfield) && n < field_size[nfield]); };

	inline void conv_error(bool b) { _err = b; };
	inline bool conv_error(void) { return _err; };

	time_t date_around(const std::tm &, bool = true);

	bool split_string(std::string &, std::string, std::string &);
	inline bool is_numeric(std::string &s) {
		for (byte i(0); i < s.size(); i++)
			if (!isdigit(s[i]))
				return false;
		return s.size();
	};
	inline std::string &to_upper(std::string &s) {
		for (auto &x : s)
			x = static_cast<char>(std::toupper(x));
		return s;
	};
	std::string &trim_string(std::string &);
	std::string &normalize_field(field_name const, std::string &);
	inline std::string &normalize_field(byte const nfield, std::string &s) { return (normalize_field(field_name(nfield), s)); };
	cron &init_ref(std::tm &, int *[]);
	int size_of_month(const std::tm &, bool = false);

public:
	inline const std::string expression(void) { return _expression; };
	inline const bool error(void) { return conv_error(); };

	inline cron &clear(void) {
		for (byte i(0); i < field_name::expr; i++)
			set(i, false);
		_expression.clear();
		_last_is_set = false;
		conv_error(true);
		return *this;
	};
	cron &assign(std::string s);
	inline cron &operator=(std::string s) { return assign(s); };

	inline const time_t next_date(std::tm *t) { return date_around(*t); };
	inline const time_t next_date(const time_t *rawtime) { return next_date(localtime(rawtime)); };
	inline const time_t next_date(const time_t &rawtime) { return next_date(localtime(&rawtime)); };
	inline const time_t previous_date(std::tm *t) { return date_around(*t, false); };
	inline const time_t previous_date(const time_t *rawtime) { return previous_date(localtime(rawtime)); };
	inline const time_t previous_date(const time_t &rawtime) { return previous_date(localtime(&rawtime)); };

	cron(void) :
			_err(false) { init(); };
	cron(std::string s) :
			_err(false) {
		init();
		assign(s);
	};
	~cron(void) { clear(); };
};

} //namespace crontab

} // namespace datetime_utils

#endif // DATETIME_H
