"""Common timezone names (civilian, military and combined)

These productions are a collection of common civilian and
military timezone names.  The list of names is by no means
exhaustive (nor definitive), but it gives most timezones
at least one named value (to make it possible to enter the
name), and it doesn't repeat any names (I hope ;) ).  You
have three major classes of names, civilian (EST, PST, GMT,
UTC), military single-character (A,B,C,D,E...) and military
phonetic spelling (Alpha, Bravo... Zulu).  The military
variants are combined into a single production, however.

	civilian_timezone_name -- the "familiar" timezones, most
		real-world data entry will want to use this as their
		"timezone" definition I'm guessing.
		
	military_timezone_name -- military timezones in the two
		formats outlined above.
		
	timezone_name -- combination of the two above into a
		single production.

Interpreter:

	TimeZoneNameInterpreter -- see below for details, by
		default takes the timezone name and converts to
		a second offset in West-negative format.  Note:
		this is the _opposite_ of the time module, but is
		the more commonly used format AFAIK.  Null matches
		will return a default TimeZone as specified.
"""
from simpleparse import objectgenerator, common
from simpleparse.common import phonetics
import time

c = {}

timezone_data = []
civilian_data = [
	# Basically this defines our recognised input locales,
	# it is by no means exhaustive, but it gives fairly
	# good coverage with minimal overlap
	('NZDT',46800),
	('IDLE',43200),
	('NZST',43200),
	('NZT',43200),
	('AESST',39600),
	('ACSST',37800),
	('CADT',37800),
	('SADT',37800),
	('AEST',36000),
	('EAST',36000),
	('GST',36000),
	('LIGT',36000),
	('ACST',34200),
	('CAST',34200),
	('SAT',34200),
	('AWSST',32400),
	('JST',32400),
	('KST',32400),
	('WDT',32400),
	('MT',30600),
	('AWST',28800),
	('CCT',28800),
	('WADT',28800),
	('WST',28800),
	('JT',27000),
	('WAST',25200),
	('IT',12600),
	('BT',10800),
	('EETDST',10800),
	('MSK', 10800),
	('CETDST',7200),
	('EET',7200),
	('FWT',7200),
	('IST',7200),
	('MEST',7200),
	('METDST',7200),
	('SST',7200),
	('BST',3600),
	('CET',3600),
	('DNT',3600),
	('DST',3600),
	('FST',3600),
	('MET',3600),
	('MEWT',3600),
	('MEZ',3600),
	('NOR',3600),
	('SET',3600),
	('SWT',3600),
	('WETDST',3600),
	('GMT',0),
	('UTC', 0),
	('WET',0),
	('WAT',-3600),
	('NDT',-5400),
	('AT', -7200),
	('ADT',-10800),
	('NFT',-9000),
	('NST',-9000),
	('AST',-14400),
	('EDT',-14400),
	('ZP4',-14400),
	('CDT',-18000),
	('EST',-18000),
	('ZP5',-18000),
	('CST',-21600),
	('MDT',-21600),
	('ZP6',-21600),
	('MST',-25200),
	('PDT',-25200),
	('PST',-28800),
	('YDT',-28800),
	('HDT',-32400),
	('YST',-32400),
	('AKST',-32400),
	
	('AHST',-36000),
	('HST',-36000),
	('CAT',-36000),
	('NT',-39600),
	('IDLW',-43200),
]
timezone_data = timezone_data + civilian_data
### add military timezones
##A-I then K-Z are used...
## z = 0
## a - i, k-m -> + values up to 12
## n-y - values up to -12
## what a totally messed up system!
## I've checked with a number of sites, they all seem to think
## it works this way... darned if I can figure out why they don't
## make N -12, o -11 etceteras so that z would come in order and you'd
## have a simple progression around the globe... sigh.
zulu_data = [
	('A', 3600), ('B', 7200), ('C', 10800), ('D', 14400), ('E', 18000),
	('F', 21600), ('G', 25200), ('H', 28800), ('I', 32400), ('K', 36000),
	('L', 39600), ('M', 43200),
	('N', -3600), ('O', -7200), ('P', -10800), ('Q', -14400), ('R', -18000),
	('S', -21600), ('T', -25200), ('U', -28800), ('V', -32400), ('W', -36000),
	('X', -39600), ('Y', -43200),
	('Z', 0),
]	
# now add these, plus the expanded versions to the dict above...
# note that we only allow capitalised versions of the military
# zones!
tztemp = []
for key, value in zulu_data:
	for item in phonetics._letters:
		if item[0] == key:
			tztemp.append( (item, value) )
# order is important here, want longer first
zulu_data = tztemp + zulu_data
del tztemp
# and call that done for now, folks...
timezone_data = timezone_data + zulu_data
# the rules are really big, but oh well...
def _build( data ):
	"""Build the name:time map and match rule for each dataset"""
	data = data[:]
	data.sort() # get shortest and least values first forcefully...
	# then reverse that, to get longest first...
	data.reverse()
	names = []
	mapping = {}
	for key,value in data:
		names.append( objectgenerator.Literal(value=key))
		mapping[key] = value
	rule = objectgenerator.FirstOfGroup(
		children = names
	)
	return mapping, rule
zulu_mapping, zulu_rule          = _build( zulu_data )
civilian_mapping, civilian_rule  = _build( civilian_data )
timezone_mapping, timezone_rule  = _build( timezone_data )

c[ "military_timezone_name" ] = zulu_rule
c[ "civilian_timezone_name" ] = civilian_rule
c[ "timezone_name" ] = timezone_rule

common.share(c)

import time
if time.daylight:
	LOCAL_ZONE = time.altzone
else:
	LOCAL_ZONE = time.timezone
# account for time module's different counting procedure...
LOCAL_ZONE = -LOCAL_ZONE

class TimeZoneNameInterpreter:
	"""Intepret a timezone specified as a military or civilian timezone name

	Return value is an offset from UTC given in seconds.
	If a null-match is passed uses the passed defaultZone.
	Returns values in seconds difference from UTC (negative
	West) divided by the passed "seconds" argument.
	"""
	def __init__( self, defaultZone=LOCAL_ZONE, seconds=1.0):
		"""
		defaultZone -- ofset in seconds to be returned if there
			is no value specified (null-match)
		seconds -- divisor applied to the value before returning,
			if you want hours, use 3600.0, if you want minutes,
			use 60.0, if you want days (why?), use 86400.0
		"""
		self.defaultZone = defaultZone
		self.seconds = seconds
	def __call__( self, (tag, left, right, children), buffer ):
		value = buffer[ left: right ]
		if value:
			try:
				return timezone_mapping[ value ]/self.seconds
			except KeyError:
				raise ValueError( "Unrecognised (but parsed!) TimeZone Name %s found at character position %s"%(value, left))
		else:
			return self.defaultZone/self.seconds

