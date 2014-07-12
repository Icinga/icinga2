/**
 * Sample timeperiods for Icinga 2 requiring
 * 'legacy-timeperiod' template from the Icinga
 * Template Library (ITL).
 * Check the documentation for details.
 */

object TimePeriod "24x7" {
  import "legacy-timeperiod"

  display_name = "Icinga 2 24x7 TimePeriod"
  ranges = {
    "monday" 	= "00:00-24:00"
    "tuesday" 	= "00:00-24:00"
    "wednesday" = "00:00-24:00"
    "thursday" 	= "00:00-24:00"
    "friday" 	= "00:00-24:00"
    "saturday" 	= "00:00-24:00"
    "sunday" 	= "00:00-24:00"
  }
}

object TimePeriod "9to5" {
  import "legacy-timeperiod"

  display_name = "Icinga 2 9to5 TimePeriod"
  ranges = {
    "monday" 	= "09:00-17:00"
    "tuesday" 	= "09:00-17:00"
    "wednesday" = "09:00-17:00"
    "thursday" 	= "09:00-17:00"
    "friday" 	= "09:00-17:00"
  }
}

object TimePeriod "never" {
  import "legacy-timeperiod"

  display_name = "Icinga 2 never TimePeriod"
  ranges = {
  }
}

