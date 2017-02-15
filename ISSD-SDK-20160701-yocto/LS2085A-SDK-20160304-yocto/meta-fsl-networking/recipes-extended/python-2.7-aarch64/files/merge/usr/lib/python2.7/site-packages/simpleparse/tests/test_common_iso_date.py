import unittest, string
from simpleparse.parser import Parser
from simpleparse.common import iso_date, iso_date_loose
from mx import DateTime
import time

fulltrans = string.maketrans("","")
tzOffset = DateTime.DateTimeDelta( 0,0,0, time.timezone )

class CommonTests(unittest.TestCase):
	def testISODateLoose( self ):
		"""Test the parsing of ISO date and time formats"""
		values = [
			("2002-02-03", DateTime.DateTime( 2002, 2,3)),
			("2002-02",DateTime.DateTime( 2002, 2)),
			("2002",DateTime.DateTime( 2002)),
			("2002-02-03 04:15", DateTime.DateTime( 2002, 2,3, 4,15)),
			("2002-02-03 04:15:16", DateTime.DateTime( 2002, 2,3, 4,15, 16)),
			("2002-02-03 04:15:16 +00:00", DateTime.DateTime( 2002, 2,3, 4,15, 16)-tzOffset),
			("2002-02-03 4:5", DateTime.DateTime( 2002, 2,3, 4,5)),
			("2002-02-03 4:5:16", DateTime.DateTime( 2002, 2,3, 4,5, 16)),
			("2002-02-03 4:5:16 +00:00", DateTime.DateTime( 2002, 2,3, 4, 5,16)-tzOffset),
		]
		p = Parser ("d:= ISO_date_time_loose", "d")
		proc = iso_date_loose.MxInterpreter()
		for string, date in values:
			success, children, next = p.parse( string, processor = proc)
			assert success, """Unable to parse any of the string %s with the ISO date-time parser"""% (string)
			assert next == len(string),"""Did not finish parsing string %s with the ISO date-time parser, remainder was %s, found was %s"""%( string, string [next:],children)
			assert children [0] == date,"""Returned different date for string %s than expected, got %s, expected %s"""% (string,children [0], date)
	def testISODate( self ):
		"""Test the parsing of ISO date and time formats"""
		values = [
			("2002-02-03", DateTime.DateTime( 2002, 2,3)),
			("2002-02",DateTime.DateTime( 2002, 2)),
			("2002",DateTime.DateTime( 2002)),
			("2002-02-03T04:15", DateTime.DateTime( 2002, 2,3, 4,15)),
			("2002-02-03T04:15:16", DateTime.DateTime( 2002, 2,3, 4,15, 16)),
			("2002-02-03T04:15:16+00:00", DateTime.DateTime( 2002, 2,3, 4,15, 16)-tzOffset),
		]
		p = Parser ("d:= ISO_date_time", "d")
		proc = iso_date.MxInterpreter()
		for string, date in values:
			success, children, next = p.parse( string, processor=proc)
			assert success, """Unable to parse any of the string %s with the ISO date-time parser"""% (string)
			assert next == len(string),"""Did not finish parsing string %s with the ISO date-time parser, remainder was %s, found was %s"""%( string, string [next:],children)
			assert children [0] == date,"""Returned different date for string %s than expected, got %s, expected %s"""% (string,children [0], date)
	def testProductionsStrict( self ):
		for string, production in [
			("2002", "year"),
			("02", "month"),
			("02", "day"),
			("24:00:00", "ISO_time"),
			("02", "ISO_time"),
			(":", "time_separator"),
			("02:02", "ISO_time"),
			("02:02:02", "ISO_time"),
			("2002-02-30", "ISO_date"),
			("2002-02-30", "ISO_date_time"),
			("02", "hour"),
			("02", "minute"),
			("02", "second"),
			("20", "second"),

			("+0500", "offset"),
			("+00:00", "offset"),
			("-", "offset_sign"),
			("-00:00", "offset"),
			("-04:00", "offset"),
			("-0500", "offset"),
			("02:13", "ISO_time"),
			("02:13:16", "ISO_time"),
			("2002-02-01T02:13-0500", "ISO_date_time"),
		]:
			success, children, next = iso_date._p.parse( string,production)
			assert next == len(string), "couldn't parse %s as a %s"%( string, production)
		
	def testProductions2( self ):
		for string, production in [
			("2002", "year"),
			("02", "month"),
			("02", "day"),
			("24:00:00", "ISO_time_loose"),
			("02", "ISO_time_loose"),
			(":", "time_separator"),
			("02:02", "ISO_time_loose"),
			("02:02:02", "ISO_time_loose"),
			("2002-02-30", "ISO_date_loose"),
			("2002-02-30", "ISO_date_time_loose"),
			("2002-2-1", "ISO_date_time_loose"),
			("02", "hour"),
			("02", "minute"),
			("2", "second"),
			("02", "second"),
			("20", "second"),
			("20.", "second"),
			("20.3", "second"),

			("+0500", "offset"),
			("+00:00", "offset"),
			("-", "offset_sign"),
			("-00:00", "offset"),
			("-04:00", "offset"),
			("-0500", "offset"),
			("02:13", "ISO_time_loose"),
			("02:13:16", "ISO_time_loose"),
			("2002-2-1 2:13", "ISO_date_time_loose"),
			("2002-2-1 2:13 -0500", "ISO_date_time_loose"),
			("2002-2-1 2:13 -05:30", "ISO_date_time_loose"),
			("2002-2-1 2:13 +05:30", "ISO_date_time_loose"),
			("2002-2-1 2:13 +00:00", "ISO_date_time_loose"),
			
		]:
			success, children, next = iso_date_loose._p.parse( string,production )
			assert next == len(string), "couldn't parse %s as a %s"%( string, production)
			
		
def getSuite():
	return unittest.makeSuite(CommonTests, 'test')

if __name__ == "__main__":
	unittest.main(defaultTest="getSuite")
