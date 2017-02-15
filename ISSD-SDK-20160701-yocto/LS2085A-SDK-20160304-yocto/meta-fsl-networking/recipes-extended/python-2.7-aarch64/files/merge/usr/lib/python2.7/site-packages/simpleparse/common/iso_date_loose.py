"""Somewhat Looser ISO date format YYYY-MM-DD HH:mm:SS +HH:mm

	ISO_date_loose -- YYYY-MM-DD format, with a month and day optional,
		month or day may be specified without leading 0
	ISO_time_loose -- HH:mm:SS format, with minutes and seconds optional
		all numbers may be specified without leading 0
	ISO_date_time_loose -- YYYY-MM-DD HH:mm:SS +HH:mm format,
		with time optional and TimeZone offset optional,
		same format for date and time as above

Interpreter:
	MxInterpreter
		Interprets the parse tree as mx.DateTime values
		Date and DateTime -> DateTime objects
		Time only -> RelativeDateTime
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
declaration = """
<date_separator> := [-]
<time_separator> := ':'
offset_sign := [-+]

year    := int
month   := int
day     := int
hour    := int
minute  := int
second  := float/int
ISO_date_loose := year, (date_separator, month, (date_separator, day)?)?
ISO_time_loose := hour, (time_separator, minute, (time_separator, second)?)?
offset    := offset_sign, offset_hour, time_separator?, offset_minute?
offset_hour := digit, digit
offset_minute := digit, digit

ISO_date_time_loose  := ISO_date_loose, ([T ], ISO_time_loose)?, [ ]?, offset?
"""

_p = Parser( declaration )
for name in ["ISO_time_loose","ISO_date_time_loose", "ISO_date_loose"]:
	c[ name ] = objectgenerator.LibraryElement(
		generator = _p._generator,
		production = name,
	)
common.share( c )

if haveMX:
	class MxInterpreter(DispatchProcessor):
		"""Interpret a parsed ISO_date_time_loose in GMT/UTC time or localtime
		"""
		int = numbers.IntInterpreter()
		offset_minute = offset_hour = year = month = day = hour = minute = int

		float = numbers.FloatInterpreter()
		second =  float
		
		def __init__(
			self,
			inputLocal = 1,
			returnLocal = 1,
		):
			self.inputLocal = inputLocal
			self.returnLocal = returnLocal
		dateName = 'ISO_date_loose'
		timeName = 'ISO_time_loose'
		def ISO_date_time_loose( self, (tag, left, right, sublist), buffer):
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
		def ISO_date_loose( self, (tag, left, right, sublist), buffer):
			"""Interpret the loose ISO date format"""
			set = singleMap( sublist, self, buffer )
			return DateTime.DateTime(
				set.get("year") or now().year,
				set.get("month") or 1,
				set.get("day") or 1,
			)
		def ISO_time_loose( self, (tag, left, right, sublist), buffer):
			"""Interpret the loose ISO time format"""
			set = singleMap( sublist, self, buffer )
			return DateTime.RelativeDateTime(
				hour = set.get("hour") or 0,
				minute = set.get("minute") or 0,
				second = set.get("second") or 0,
			)

		
		def offset( self, (tag, left, right, sublist), buffer):
			"""Calculate the time zone offset as a date-time delta"""
			set = singleMap( sublist, self, buffer )
			direction = set.get('offset_sign',1)
			hour = set.get( "offset_hour", 0)
			minute = set.get( "offset_minute", 0)
			delta = DateTime.DateTimeDelta( 0, hour*direction, minute*direction)
			return delta
			
		def offset_sign( self , (tag, left, right, sublist), buffer):
			"""Interpret the offset sign as a multiplier"""
			v = buffer [left: right]
			if v in ' +':
				return 1
			else:
				return -1
				
