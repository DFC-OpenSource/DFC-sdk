"""Canonical ISO date format YYYY-MM-DDTHH:mm:SS+HH:mm

This parser is _extremely_ strict, and the dates that match it,
though really easy to work with for the computer, are not particularly
readable.  See the iso_date_loose module for a slightly relaxed
definition which allows the "T" character to be replaced by a
" " character, and allows a space before the timezone offset, as well
as allowing the integer values to use non-0-padded integers.


	ISO_date -- YYYY-MM-DD format, with a month and date optional
	ISO_time -- HH:mm:SS format, with minutes and seconds optional
	ISO_date_time -- YYYY-MM-DD HH:mm:SS+HH:mm format,
		with time optional and TimeZone offset optional

Interpreter:
	MxInterpreter
		Interprets the parse tree as mx.DateTime values
		ISO_date and ISO_time
			returns DateTime objects
		Time only
			returns RelativeDateTime object which, when
			added to a DateTime gives you the given time
			within that day
"""
try:
	from mx import DateTime
	haveMX = 1
except ImportError:
	haveMX = 0
from simpleparse.parser import Parser
from simpleparse import common, objectgenerator
from simpleparse.common import chartypes, numbers
from simpleparse.dispatchprocessor import *

c = {}

declaration ="""
year      := digit,digit,digit,digit
month     := digit,digit
day       := digit,digit

hour      := digit,digit
minute    := digit,digit
second    := digit,digit
offset_sign := [-+]
offset    := offset_sign, hour, time_separator?, minute

<date_separator> := '-'
<time_separator> := ':'

ISO_date   := year, (date_separator, month, (date_separator, day)?)?
ISO_time   := hour, (time_separator, minute, (time_separator, second)?)?
ISO_date_time  := ISO_date, ([T], ISO_time)?, offset?
"""




_p = Parser( declaration )
for name in ["ISO_time","ISO_date", "ISO_date_time"]:
	c[ name ] = objectgenerator.LibraryElement(
		generator = _p._generator,
		production = name,
	)
common.share( c )

if haveMX:
	class MxInterpreter(DispatchProcessor):
		"""Interpret a parsed ISO_date_time_loose in GMT/UTC time or localtime
		"""
		def __init__(
			self,
			inputLocal = 1,
			returnLocal = 1,
		):
			self.inputLocal = inputLocal
			self.returnLocal = returnLocal
		dateName = 'ISO_date'
		timeName = 'ISO_time'
		def ISO_date_time( self, (tag, left, right, sublist), buffer):
			"""Interpret the loose ISO date + time format"""
			set = singleMap( sublist, self, buffer )
			base, time, offset = (
				set.get(self.dateName),
				set.get(self.timeName) or DateTime.RelativeDateTime(hour=0,minute=0,second=0),
				set.get( "offset" ),
			)
			base = base + time
			offset = set.get( "offset" )
			if offset is not None:
				# an explicit timezone was entered, convert to gmt and return as appropriate...
				gmt = base - offset
				if self.returnLocal:
					return gmt.localtime()
				else:
					return gmt
			# was in the default input locale (either gmt or local)
			if self.inputLocal and self.returnLocal:
				return base
			elif not self.inputLocal and not self.returnLocal:
				return base
			elif self.inputLocal and not self.returnLocal:
				# return gmt from local...
				return base.gmtime()
			else:
				return base.localtime()
		def ISO_date( self, (tag, left, right, sublist), buffer):
			"""Interpret the ISO date format"""
			set = {}
			for item in sublist:
				set[ item[0] ] = dispatch( self, item, buffer)
			return DateTime.DateTime(
				set.get("year") or now().year,
				set.get("month") or 1,
				set.get("day") or 1,
			)
		def ISO_time( self, (tag, left, right, sublist), buffer):
			"""Interpret the ISO time format"""
			set = {}
			for item in sublist:
				set[ item[0] ] = dispatch( self, item, buffer)
			return DateTime.RelativeDateTime(
				hour = set.get("hour") or 0,
				minute = set.get("minute") or 0,
				second = set.get("second") or 0,
			)

		integer = numbers.IntInterpreter()
		second =  offset_minute = offset_hour = year = month = day = hour =minute =integer
		
		def offset( self, (tag, left, right, sublist), buffer):
			"""Calculate the time zone offset as a date-time delta"""
			set = singleMap( sublist, self, buffer )
			direction = set.get('offset_sign',1)
			hour = set.get( "hour", 0)
			minute = set.get( "minute", 0)
			delta = DateTime.DateTimeDelta( 0, hour*direction, minute*direction)
			return delta
			
		def offset_sign( self , (tag, left, right, sublist), buffer):
			"""Interpret the offset sign as a multiplier"""
			v = buffer [left: right]
			if v in ' +':
				return 1
			else:
				return -1
				
