"""Locale-specific calendar names (day-of-week and month-of-year)

These values are those returned by the calendar module.  Available
productions:

	locale_day_names
	locale_day_names_uc
	locale_day_names_lc
		Names for the days of the week

	locale_day_abbrs
	locale_day_abbrs_uc
	locale_day_abbrs_lc
		Short-forms (3 characters normally) for
		the days of the week.

	locale_month_names
	locale_month_names_uc
	locale_month_names_lc
		Names for the months of the year

	locale_month_abbrs
	locale_month_abbrs_uc
	locale_month_abbrs_lc
		Short-forms (3 characters normally) for
		the months of the year

Interpreters:
	MonthNameInterpreter
	DayNameInterpreter
		Both offer the ability to set an index other
		than the default (of 1) for the first item in
		the list.
"""
import calendar, string
from simpleparse import objectgenerator, common

c = {}

da = calendar.day_abbr[:]
dn = calendar.day_name[:]
ma = calendar.month_abbr[:]
mn = calendar.month_name[:]

def _build( name, set ):
	# make sure longest equal-prefix items are first
	set = set[:]
	set.sort()
	set.reverse()
	l,u,r = [],[],[]
	for item in set:
		l.append( objectgenerator.Literal( value = string.lower(item) ))
		u.append( objectgenerator.Literal( value = string.upper(item) ))
		r.append( objectgenerator.Literal( value = item ))
	c[ name + '_lc' ] = objectgenerator.FirstOfGroup( children = l )
	c[ name + '_uc' ] = objectgenerator.FirstOfGroup( children = u )
	c[ name ] = objectgenerator.FirstOfGroup( children = r )

_build( 'locale_day_names', dn )
_build( 'locale_day_abbrs', da )


_build( 'locale_month_names', mn )
_build( 'locale_month_abbrs', ma )

da = map(string.lower, da )
dn = map(string.lower, dn )
ma = map(string.lower, ma )
mn = map(string.lower, mn )


common.share( c )

class NameInterpreter:
	offset = 1
	def __init__( self, offset = 1 ):
		self.offset = offset
	def __call__( self, (tag, left, right, children), buffer ):
		value = string.lower( buffer[left:right] )
		for table in self.tables:
			try:
				return table.index( value )+ self.offset
			except ValueError:
				pass
		raise ValueError( """Unrecognised (but parsed) %s name %s at character %s"""%( self.nameType, value, left))

class MonthNameInterpreter( NameInterpreter):
	"""Interpret a month-of-year name as an integer index

	Pass an "offset" value to __init__ to use an offset other
	than 1 (Monday = 1), normally 0 (Monday = 0)
	"""
	nameType = "Month"
	tables = (mn,ma)
class DayNameInterpreter( NameInterpreter ):
	"""Interpret a day-of-week name as an integer index

	Pass an "offset" value to __init__ to use an offset other
	than 1 (January = 1), normally 0 (January = 0)
	"""
	nameType = "Day"
	tables = (dn,da)
