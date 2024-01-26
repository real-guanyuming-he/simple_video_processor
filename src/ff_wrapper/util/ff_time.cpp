#include "ff_time.h"

#include <format>

FF_WRAPPER_API std::string ff::time::to_string(const std::string& fmt) const
{
	double total_seconds = to_absolute_double();
	double total_miliseconds = 1e3 * total_seconds;
	double total_microseconds = 1e3 * total_miliseconds;
	double total_minutes = total_seconds / 60.0;
	double total_hours = total_minutes / 60.0;

	// Round total_hours down
	double remaining_hours = (int64_t)total_hours;

	double temp_minutes = 60.0 * (total_hours - remaining_hours);
	double remaining_minutes = (int64_t)temp_minutes;

	double remaining_seconds = 60.0 * (temp_minutes - remaining_minutes);

	// if the total time is negative,
	// then all remaining calculated from (temp - upper_remaining) will be negative
	// address that
	if (total_seconds < 0.0)
	{
		remaining_seconds = -remaining_seconds;
		remaining_minutes = -remaining_minutes;
	}

	return std::vformat
	(
		fmt,
		std::make_format_args
		(
			total_microseconds, total_miliseconds, total_seconds, total_minutes, total_hours,
			remaining_seconds, (int64_t)remaining_minutes, (int64_t)remaining_hours
		)
	);
}
