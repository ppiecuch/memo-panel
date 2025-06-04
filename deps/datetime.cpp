#include "datetime.h"

namespace datetime_utils {

/// Datetime

datetime::datetime() :
		timeInfo(new tm()) {
	// Default constructor get current date and time
	time_t rawtime;
	time(&rawtime);
	struct tm *tm_now = localtime(&rawtime);
	_copy_from(tm_now);
}

datetime::datetime(int year,
		int month,
		int day,
		int hour,
		int minute,
		int second,
		period day_period) {
	// Check out of range limit
	if (month < 1 || month > 12) {
		throw std::invalid_argument("month must be between 1 and 12");
	}
	if (day < 1) {
		throw std::invalid_argument("day is out of range");
	}
	if ((month == 1 || month == 3 || month == 5 || month == 7 || month == 8 || month == 10 || month == 12) && day > 31) {
		throw std::invalid_argument("day is out of range");
	}
	if ((month == 4 || month == 6 || month == 9 || month == 11) && day > 30) {
		throw std::invalid_argument("day is out of range");
	}
	if (month == 2 && this->_is_leapyear(year) && day > 29) {
		throw std::invalid_argument("day is out of range");
	}
	if (month == 2 && !this->_is_leapyear(year) && day > 28) {
		throw std::invalid_argument("day is out of range");
	}
	if (day_period == period::undefined) {
		if (hour < 0 || hour > 23) {
			throw std::invalid_argument("hour must be between 0 and 23");
		}
	} else {
		if (day_period != period::AM && day_period != period::PM) {
			throw std::invalid_argument("the selected period is out of range");
		}
		if (hour < 1 || hour > 12) {
			throw std::invalid_argument("hour must be between 1 and 12");
		}
		// Adjust to 24 hour format
		if (hour == 12 && day_period == period::AM) {
			hour = 0;
		} else if (day_period == period::PM && hour < 12) {
			hour = hour + 12;
		}
	}
	if (minute < 0 || minute > 59) {
		throw std::invalid_argument("minute must be between 0 and 59");
	}
	if (second < 0 || second > 59) {
		throw std::invalid_argument("second must be between 0 and 59");
	}
	timeInfo = new tm();
	timeInfo->tm_year = year - 1900;
	timeInfo->tm_mon = month - 1;
	timeInfo->tm_mday = day;
	timeInfo->tm_hour = hour;
	timeInfo->tm_min = minute;
	timeInfo->tm_sec = second;
	timeInfo->tm_isdst = -1;
	mktime(timeInfo);
}

// Copy constructor
datetime::datetime(const datetime &other) {
	timeInfo = new tm();
	_copy_from(other.timeInfo);
}

// Copy assignment
datetime &datetime::operator=(const datetime &other) {
	if (this != &other) {
		_copy_from(other.timeInfo);
	}
	return *this;
}

datetime::~datetime() {
	delete timeInfo;
}

// Move constructor
datetime::datetime(datetime &&other) noexcept
		:
		timeInfo(other.timeInfo) {
	// Release the data pointer from the source object so that the destructor
	// does not free the memory multiple times.
	other.timeInfo = nullptr;
}

// Move assignement operator
datetime &datetime::operator=(datetime &&other) noexcept {
	if (this != &other) {
		delete timeInfo;
		// Copy the data pointer and its length from the source object.
		timeInfo = other.timeInfo;
		// Release the data pointer from the source object so that
		// the destructor does not free the memory multiple times.
		other.timeInfo = nullptr;
	}
	return *this;
}

bool datetime::_is_leapyear(int year) const {
	return ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0));
}

std::string datetime::to_string() const {
	char retVal[128] = "";
	snprintf(static_cast<char *>(retVal), 128, "%d-%02d-%02d %02d:%02d:%02d",
			get_year(),
			get_month(),
			get_day(),
			get_hour(),
			get_minute(),
			get_second());
	return std::string(static_cast<char *>(retVal));
}

std::string datetime::to_string(const std::string &format) const {
	std::string retVal;

	if (strcmp(format.c_str(), "") == 0) {
		throw std::invalid_argument("format");
	}
	std::string pattern_temp;
	for (unsigned int index_char = 0; index_char < format.length(); index_char++) {
		bool is_letter = false;
		// Check if the character is a valid pattern char
		if ((format[index_char] >= 'a' && format[index_char] <= 'z') ||
				(format[index_char] >= 'A' && format[index_char] <= 'Z')) {
			is_letter = true;
			if (pattern_temp.length() == 0) {
				pattern_temp += format[index_char];
			} else if (format[index_char] == pattern_temp[pattern_temp.length() - 1]) {
				// If the pattern has not changed
				pattern_temp += format[index_char];
			}
		}
		// Check if the pattern has not changed
		if (pattern_temp.length() > 0 && (format[index_char] != pattern_temp[pattern_temp.length() - 1] || index_char == format.length() - 1)) {
			char value_converted[20] = "";
			if (pattern_temp == "yyyy") {
				snprintf(static_cast<char *>(value_converted), 20, "%04d", this->get_year());
				retVal += static_cast<char *>(value_converted);
			} else if (pattern_temp == "yy") {
				snprintf(static_cast<char *>(value_converted), 20, "%02d", this->get_year() % 100);
				retVal += static_cast<char *>(value_converted);
			} else if (pattern_temp == "MM") {
				snprintf(static_cast<char *>(value_converted), 20, "%02d", this->get_month());
				retVal += static_cast<char *>(value_converted);
			} else if (pattern_temp == "M") {
				snprintf(static_cast<char *>(value_converted), 20, "%01d", this->get_month());
				retVal += static_cast<char *>(value_converted);
			} else if (pattern_temp == "dd") {
				snprintf(static_cast<char *>(value_converted), 20, "%02d", this->get_day());
				retVal += static_cast<char *>(value_converted);
			} else if (pattern_temp == "d") {
				snprintf(static_cast<char *>(value_converted), 20, "%01d", this->get_day());
				retVal += static_cast<char *>(value_converted);
			} else if (pattern_temp == "HH") {
				snprintf(static_cast<char *>(value_converted), 20, "%02d", this->get_hour());
				retVal += static_cast<char *>(value_converted);
			} else if (pattern_temp == "H") {
				snprintf(static_cast<char *>(value_converted), 20, "%01d", this->get_hour());
				retVal += static_cast<char *>(value_converted);
			} else if (pattern_temp == "hh") {
				int instance_hour = this->get_hour();
				if (instance_hour == 0) {
					retVal += "12";
				} else if (instance_hour > 12) {
					snprintf(static_cast<char *>(value_converted), 20, "%02d", instance_hour - 12);
					retVal += static_cast<char *>(value_converted);
				} else {
					snprintf(static_cast<char *>(value_converted), 20, "%02d", instance_hour);
					retVal += static_cast<char *>(value_converted);
				}
			} else if (pattern_temp == "h") {
				int instance_hour = this->get_hour();
				if (instance_hour == 0) {
					retVal += "12";
				} else if (instance_hour > 12) {
					snprintf(static_cast<char *>(value_converted), 20, "%01d", instance_hour - 12);
					retVal += static_cast<char *>(value_converted);
				} else {
					snprintf(static_cast<char *>(value_converted), 20, "%01d", instance_hour);
					retVal += static_cast<char *>(value_converted);
				}
			} else if (pattern_temp == "mm") {
				snprintf(static_cast<char *>(value_converted), 20, "%02d", this->get_minute());
				retVal += static_cast<char *>(value_converted);
			} else if (pattern_temp == "m") {
				snprintf(static_cast<char *>(value_converted), 20, "%01d", this->get_minute());
				retVal += static_cast<char *>(value_converted);
			} else if (pattern_temp == "ss") {
				snprintf(static_cast<char *>(value_converted), 20, "%02d", this->get_second());
				retVal += static_cast<char *>(value_converted);
			} else if (pattern_temp == "s") {
				snprintf(static_cast<char *>(value_converted), 20, "%01d", this->get_second());
				retVal += static_cast<char *>(value_converted);
			} else if (pattern_temp == "tt") {
				if (this->get_hour() >= 12) {
					retVal += "PM";
				} else {
					retVal += "AM";
				}
			}

			pattern_temp = "";
		}

		if (!is_letter) {
			retVal += format[index_char];
		} else if (pattern_temp.length() == 0) {
			pattern_temp += format[index_char];
		}
	}
	return std::string(retVal);
}

std::string datetime::to_shortdate_string() const {
	char retVal[128] = "";
	snprintf(static_cast<char *>(retVal), 128, "%d-%02d-%02d",
			get_year(),
			get_month(),
			get_day());
	return std::string(static_cast<char *>(retVal));
}

int datetime::get_year() const { return timeInfo->tm_year + 1900; }

int datetime::get_month() const { return timeInfo->tm_mon + 1; }

int datetime::get_day() const { return timeInfo->tm_mday; }

int datetime::get_hour() const { return timeInfo->tm_hour; }

int datetime::get_minute() const { return timeInfo->tm_min; }

int datetime::get_second() const { return timeInfo->tm_sec; }

weekday datetime::get_weekday() const { return static_cast<weekday>(timeInfo->tm_wday); }

void datetime::add_years(int nb_years) { timeInfo->tm_year += nb_years; }

void datetime::add_months(int nb_months) {
	// Get number of year
	auto nb_year = static_cast<int>(ceil(nb_months / 12));
	int nb_months_final = nb_months % 12;

	if (timeInfo->tm_mon + nb_months_final > 11) { // tm_mon is from 0 to 11
		nb_year++;
		nb_months_final = (timeInfo->tm_mon + nb_months_final) - 12;
		timeInfo->tm_mon = nb_months_final;
	} else {
		timeInfo->tm_mon += nb_months_final;
	}

	timeInfo->tm_year += nb_year;
}

void datetime::add_days(int nb_days) { add_seconds(nb_days * ONE_DAY); }

void datetime::add_hours(int nb_hours) { add_seconds(nb_hours * ONE_HOUR); }

void datetime::add_minutes(int nb_minutes) { add_seconds(nb_minutes * ONE_MINUTE); }

void datetime::add_seconds(int nb_seconds) {
	struct tm *tm_new_time;
	time_t new_seconds = mktime(timeInfo) + nb_seconds;
	delete timeInfo;

	tm_new_time = localtime(&new_seconds);

	timeInfo = new tm();
	_copy_from(tm_new_time);
}

bool datetime::is_leapyear() {
	return _is_leapyear(get_year());
}

bool datetime::is_leapyear(int year) {
	datetime dateTemp(year, 1, 1);
	return dateTemp.is_leapyear();
}

void datetime::_copy_from(const tm *otm) {
	timeInfo->tm_year = otm->tm_year;
	timeInfo->tm_mon = otm->tm_mon;
	timeInfo->tm_mday = otm->tm_mday;
	timeInfo->tm_hour = otm->tm_hour;
	timeInfo->tm_min = otm->tm_min;
	timeInfo->tm_sec = otm->tm_sec;
	timeInfo->tm_isdst = otm->tm_isdst;
	timeInfo->tm_wday = otm->tm_wday;
	timeInfo->tm_yday = otm->tm_yday;
}

datetime datetime::parse(const std::string &format, const std::string &value) {
	int year = 1970, month = 1, day = 1, hour = 0, minute = 0, second = 0;

	if (strcmp(format.c_str(), "") == 0) {
		throw std::invalid_argument("format");
	}

	std::string pattern_temp;
	int pattern_firstindex = 0;
	bool is_letter = false;
	period day_period = period::undefined;
	for (unsigned int index_char = 0; index_char < format.length(); index_char++) {
		// Check if the character is a valid pattern char
		if ((format[index_char] >= 'a' && format[index_char] <= 'z') ||
				(format[index_char] >= 'A' && format[index_char] <= 'Z')) {
			is_letter = true;
			if (pattern_temp.length() == 0) {
				pattern_temp += format[index_char];
				pattern_firstindex = index_char;
			} else if (format[index_char] == pattern_temp[pattern_temp.length() - 1]) {
				// If the pattern has not changed
				pattern_temp += format[index_char];
			}
		}
		// Check if the pattern has not changed
		if (format[index_char] != pattern_temp[pattern_temp.length() - 1] || index_char == format.length() - 1) {
			if (pattern_firstindex + pattern_temp.length() <= value.length()) { //Ensure that the value if long enough
				int *ptr_date_section = nullptr;
				if (pattern_temp == "yyyy") {
					ptr_date_section = &year;
				}
				if (pattern_temp == "MM") {
					ptr_date_section = &month;
				}
				if (pattern_temp == "dd") {
					ptr_date_section = &day;
				}
				if (pattern_temp == "HH") {
					ptr_date_section = &hour;
				}
				if (pattern_temp == "hh") {
					ptr_date_section = &hour;
					day_period = period::AM; // Set default day period
				}
				if (pattern_temp == "mm") {
					ptr_date_section = &minute;
				}
				if (pattern_temp == "ss") {
					ptr_date_section = &second;
				}
				if (pattern_temp == "tt") { // Day period
					std::string period_str = value.substr(pattern_firstindex, pattern_temp.length());
					if (strcmp(period_str.c_str(), "AM") == 0) {
						day_period = period::AM;
					} else if (strcmp(period_str.c_str(), "PM") == 0) {
						day_period = period::PM;
					} else {
						throw std::invalid_argument("invalid value for period");
					}
				}
				if (ptr_date_section != nullptr) {
					*ptr_date_section = _parse_intvalue(pattern_temp, pattern_firstindex, pattern_temp.length(), value);
				}
			}
			pattern_temp = "";
		}

		if (is_letter && pattern_temp.length() == 0) {
			pattern_temp += format[index_char];
			pattern_firstindex = index_char;
		}
	}

	return datetime(year, month, day, hour, minute, second, day_period);
}

int datetime::_parse_intvalue(const std::string &pattern,
		int index,
		size_t mask_length,
		const std::string &parse_str) {
	long converted_value;
	int ret_val;
	char *end;
	const char *parse_str_chr;

	std::string value_parsed = parse_str.substr(index, mask_length);
	parse_str_chr = value_parsed.c_str();
	converted_value = strtol(parse_str_chr, &end, 10);
	if (parse_str_chr == end) {
		throw std::runtime_error("Unable to parse the mask " + pattern);
	}
	ret_val = static_cast<int>(converted_value);
	return ret_val;
}

// Operators
std::ostream &operator<<(std::ostream &os, const datetime &dt) {
	char retVal[128] = "";
	snprintf(static_cast<char *>(retVal), 128, "%d-%02d-%02d %02d:%02d:%02d",
			dt.timeInfo->tm_year + 1900,
			dt.timeInfo->tm_mon + 1,
			dt.timeInfo->tm_mday,
			dt.timeInfo->tm_hour,
			dt.timeInfo->tm_min,
			dt.timeInfo->tm_sec);
	os << static_cast<char *>(retVal);

	return os;
}

bool operator<(const datetime &mdt, const datetime &odt) { return mktime(mdt.timeInfo) < mktime(odt.timeInfo); }

bool operator>(const datetime &mdt, const datetime &odt) { return mktime(odt.timeInfo) < mktime(mdt.timeInfo); }

bool operator<=(const datetime &mdt, const datetime &odt) { return !(mktime(mdt.timeInfo) > mktime(odt.timeInfo)); }

bool operator>=(const datetime &mdt, const datetime &odt) { return !(mktime(mdt.timeInfo) < mktime(odt.timeInfo)); }

bool operator==(const datetime &mdt, const datetime &odt) { return mktime(mdt.timeInfo) == mktime(odt.timeInfo); }

bool operator!=(const datetime &mdt, const datetime &odt) { return !(mktime(mdt.timeInfo) == mktime(odt.timeInfo)); }

timespan operator-(const datetime &mdt, const datetime &odt) {
	int days = 0, hours = 0, minutes = 0, seconds = 0;

	// Transfer both dates in a number of days
	time_t time_mdt = mktime(mdt.timeInfo);
	time_t time_odt = mktime(odt.timeInfo);
	double difference = difftime(time_mdt, time_odt) / (60 * 60 * 24);
	days = static_cast<int>(difference);

	if (mdt >= odt) {
		hours = mdt.get_hour() - odt.get_hour();
		seconds = mdt.get_second() - odt.get_second();
		minutes = mdt.get_minute() - odt.get_minute();
		if (seconds < 0) {
			seconds += 60;
			minutes--;
		}
		if (minutes < 0) {
			minutes += 60;
			hours--;
		}
		if (hours < 0) {
			hours += 24;
		}
	} else {
		if (mdt.get_second() > odt.get_second()) {
			seconds = mdt.get_second() - odt.get_second() - 60;
			minutes += 1;
		} else {
			seconds = mdt.get_second() - odt.get_second();
		}

		if (mdt.get_minute() > odt.get_minute()) {
			minutes += mdt.get_minute() - odt.get_minute() - 60;
			hours += 1;
		} else {
			minutes += mdt.get_minute() - odt.get_minute();
		}

		if (mdt.get_hour() > odt.get_hour()) {
			hours += mdt.get_hour() - odt.get_hour() - 24;
		} else {
			hours += mdt.get_hour() - odt.get_hour();
		}
	}

	timespan retVal(days, hours, minutes, seconds);
	return retVal;
}

/// Timespan

timespan::timespan(int days, int hours, int minutes, int seconds) {
	// Check if the values submitted is valid
	if (hours < -23 || hours > 23) {
		throw std::invalid_argument("hours must be between 0 and 23");
	}
	if (minutes < -59 || minutes > 59) {
		throw std::invalid_argument("minutes must be between 0 and 59");
	}
	if (seconds < -59 || seconds > 59) {
		throw std::invalid_argument("seconds must be between 0 and 59");
	}
	this->days = days;
	this->hours = hours;
	this->minutes = minutes;
	this->seconds = seconds;
}

int timespan::get_days() const { return days; }
int timespan::get_hours() const { return hours; }
int timespan::get_minutes() const { return minutes; }
int timespan::get_seconds() const { return seconds; }
int timespan::get_total_hours() const { return (days * 24) + hours; }
int timespan::get_total_minutes() const { return (days * 60 * 24) + (hours * 60) + minutes; }
int timespan::get_total_seconds() const { return (days * 60 * 60 * 24) + (hours * 60 * 60) + (minutes * 60) + seconds; }

bool operator<(const timespan &mts, const timespan &ots) {
	if (mts.days < ots.days) {
		return true;
	}
	if (mts.days > ots.days) {
		return false;
	}
	if (mts.hours < ots.hours) {
		return true;
	}
	if (mts.hours > ots.hours) {
		return false;
	}
	if (mts.minutes < ots.minutes) {
		return true;
	}
	if (mts.minutes > ots.minutes) {
		return false;
	}
	if (mts.seconds < ots.seconds) {
		return true;
	}

	return false;
}

bool operator>(const timespan &mts, const timespan &ots) { return ots < mts; }

bool operator<=(const timespan &mts, const timespan &ots) { return !(mts > ots); }

bool operator>=(const timespan &mts, const timespan &ots) { return !(mts < ots); }

bool operator==(const timespan &mts, const timespan &ots) {
	return mts.days == ots.days &&
			mts.hours == ots.hours &&
			mts.minutes == ots.minutes &&
			mts.seconds == ots.seconds;
}

bool operator!=(const timespan &mts, const timespan &ots) { return !(mts == ots); }

/// Crontab

namespace crontab {

byte cron::index(field_name const nfield) {
	static bool b(false);
	static byte index[field_name::year] = { 0 };
	if (!b) {
		b = true;
		for (byte i(1); i <= field_name::year; i++)
			index[i] = index[i - 1] + field_size[i - 1];
	}
	return index[nfield];
}

cron &cron::assign(std::string s) {
	byte i(0);
	std::string second;
	clear();
	conv_error(false);
	trim_string(s);

	do {
		if (!split_string(s, " ", second)) {
			_expression = s;
			break;
		}
		set_field(i, s, true);
		s = second;
	} while (!conv_error() && ++i <= field_name::year);
	if (i <= field_name::year)
		set_field(field_name::year, true);
	else
		_expression = s;
	for (byte j(0); !conv_error() && j <= field_name::year; j++)
		if (is_not_set(field_name(j))) {
			conv_error(true);
			break;
		}
	return (conv_error() ? clear() : *this);
}

cron &cron::assign(const std::tm *t) { // std::tm to cronFormat:
	std::stringstream s(std::stringstream::out);
	s << (t->tm_sec + field_offset[field_name::second])
	  << " " << (t->tm_min + field_offset[field_name::minute])
	  << " " << (t->tm_hour + field_offset[field_name::hour_of_day])
	  << " " << (t->tm_mday - 1 + field_offset[field_name::day_of_month])
	  << " " << (t->tm_mon + field_offset[field_name::month])
	  << " " << (t->tm_wday + field_offset[field_name::day_of_week])
	  << " " << (1900 + t->tm_year)
	  << " NIL";
	return assign(s.str());
}

void cron::set_field(field_name const nfield, std::string first, bool v) {
	std::string second;
	if (conv_error()) {
		return;
	} else if (split_string(first, ",", second)) {
		set_field(nfield, first, v);
		set_field(nfield, second, v);
	} else if (split_string(first, "-", second)) {
		if (!is_numeric(normalize_field(nfield, first)) || !is_numeric(normalize_field(nfield, second)))
			conv_error(true);
		else
			set_scope(nfield, v, atoi(first.c_str()), atoi(second.c_str()));
	} else if (split_string(first, "/", second)) {
		if ((first.compare("*") != 0 && !is_numeric(normalize_field(nfield, first))) || !is_numeric(second))
			conv_error(true);
		else {
			set_scope(nfield, v, atoi(first.c_str()), field_size[nfield] - 1, atoi(second.c_str()));
		}
	} else if (first.compare("*") == 0) {
		set_field(nfield, v);
	} else if (first.compare("?") == 0) {
		set_field(nfield, v);
	} else if (first.compare("L") == 0) {
		if (nfield != field_name::day_of_week && nfield != field_name::day_of_month)
			conv_error(true);
		else {
			set_bit(field_name(nfield), field_size[nfield] - 1, v);
			if (nfield == field_name::day_of_month)
				_last_is_set = true;
		}
	} else if (first.compare("W") == 0) {
		if (nfield != field_name::day_of_week)
			conv_error(true);
		else {
			set_bit(field_name(nfield), field_size[nfield] - 1, v);
			set_bit(field_name(nfield), field_size[nfield] - 2, v);
		}
	} else if (is_numeric(normalize_field(nfield, first))) {
		set_bit(nfield, atoi(first.c_str()) - field_offset[nfield], v);
	} else
		conv_error(true);
}

std::string &cron::normalize_field(field_name const nfield, std::string &s) {
	std::stringstream str(std::stringstream::out);
	if (field_name(nfield) == field_name::day_of_week) {
		for (byte i(0); i < 7; i++)
			if (to_upper(s).compare(week_day[i]) == 0) {
				str << i + field_offset[nfield];
				s.assign(str.str());
			}
	} else if (field_name(nfield) == field_name::month) {
		for (byte i(0); i < 12; i++)
			if (to_upper(s).compare(month_name[i]) == 0) {
				str << i + field_offset[nfield];
				s.assign(str.str());
			}
	} else if (field_name(nfield) == field_name::year) {
		str << (atoi(s.c_str()) - 1900 - _year + 2 * field_offset[nfield]);
		s.assign(str.str());
	}
	return s;
}

cron &cron::init_ref(std::tm &timeinfo, int *tminfo[]) {
	static cron date;
	mktime(&timeinfo); //call mktime to recompute timeinfo...
	tminfo[0] = &timeinfo.tm_sec;
	tminfo[1] = &timeinfo.tm_min;
	tminfo[2] = &timeinfo.tm_hour;
	tminfo[3] = &timeinfo.tm_mday;
	tminfo[4] = &timeinfo.tm_mon;
	tminfo[5] = &timeinfo.tm_wday;
	tminfo[6] = &timeinfo.tm_year;
	return date.assign(&timeinfo);
}

time_t cron::date_around(const std::tm &timeinfo, bool next) {
	bool match(true);
	if (conv_error())
		return time_t(-1);
	std::tm result(timeinfo);
	for (byte nfield(0); existing_field(nfield); nfield++) {
		int delta(0), *tminfo[7];
		cron &date(init_ref(result, tminfo));
		int month_size((nfield == field_name::day_of_month) ? size_of_month(result) : 0);
		byte i(date.find_bit(field_name(nfield), 0)), j(i);
		if (i == npos)
			return time_t(-1);

		if (is_set(field_name(nfield)) && !(nfield == field_name::year && match)) {
			tminfo[nfield] = 0;
			if (nfield < field_name::year)
				continue;
		} else
			while (true) {
				if (is_set(field_name(nfield), i) || (nfield == field_name::day_of_month && i == month_size - 1 && _last_is_set))
					break;
				delta += (next ? 1 : -1);
				if (((i += (next ? 1 : -1)) >= (month_size ? month_size : field_size[nfield])))
					i = (next ? 0 : ((month_size ? month_size : field_size[nfield]) - 1));
				if (i == j) {
					if (nfield == field_name::day_of_month) {
						delta = (next ? 1 : -1);
						nfield++;
						break;
					} else
						return time_t(-1);
				}
			}

		if (delta) {
			match = false;
			if (nfield == field_name::day_of_week) {
				*tminfo[field_name::day_of_month] += delta;
				nfield -= 2;
			} else {
				*tminfo[nfield] += delta;
			};
			while (existing_field(--nfield)) {
				month_size = ((nfield == field_name::day_of_month) ? size_of_month(result) : 0);
				*tminfo[nfield] = (next ? ((nfield == field_name::day_of_month) ? 1 : 0) : (month_size ? month_size : field_size[nfield] - 1));
			}
		} else if (nfield == field_name::year && (match || !next)) { // Ref date matchs with cron!...
			for (nfield = 0; nfield <= field_name::year; nfield++)
				if (!is_set(nfield)) { // searching for first not "*"
					if (next) {
						if (is_set(nfield, *tminfo[nfield] + (next ? 1 : -1) - (nfield == field_name::day_of_month))) { // previous on cron "* * * A-B(matching) * * * cmd"
							set_field(nfield, false);
							set_bit(nfield, *tminfo[nfield] + (next ? 1 : -1), true);
						} else
							*tminfo[nfield] += 1;
						nfield = -2;
					} else {
						while (existing_field(--nfield))
							*tminfo[nfield] = 0;
						nfield = -3;
						break;
					} // previous on cron "* * * X(matching) * * * cmd"
				}
			if (nfield == byte(-1))
				return time_t(-1); // cron is "* * * * * * * cmd"
		}
	}
	return mktime(&result);
}

bool cron::split_string(std::string &first, std::string pattern, std::string &second) {
	std::size_t index(first.find_first_of(pattern));
	if (index == std::string::npos) {
		second.clear();
		return false;
	}
	second = first.substr(index + 1);
	first.erase(index);
	return (second.size());
}

std::string &cron::trim_string(std::string &s) {
	for (auto &x : s)
		if (x == '\t')
			x = ' ';
	s.erase(0, s.find_first_not_of(' '));
	for (std::size_t i(s.find("  ")); i != std::string::npos; i = s.find("  ")) {
		s.replace(i, 2, " ");
	};
	return s;
}

int cron::size_of_month(const std::tm &v, bool prev) {
	static std::tm t;
	t.tm_isdst = t.tm_wday = t.tm_yday = t.tm_sec = t.tm_min = t.tm_hour = 0;
	t.tm_mday = 31;
	t.tm_mon = v.tm_mon - prev;
	t.tm_year = v.tm_year;
	mktime(&t);
	return (t.tm_mon == v.tm_mon ? 31 : (31 - t.tm_mday));
}
} //namespace crontab

} // namespace datetime_utils
