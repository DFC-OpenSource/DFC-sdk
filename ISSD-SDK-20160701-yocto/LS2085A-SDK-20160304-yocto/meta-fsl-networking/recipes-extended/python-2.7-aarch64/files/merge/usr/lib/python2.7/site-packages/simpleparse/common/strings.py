"""Python string parsers with escape characters

Python-string-like operation as much as possible, this includes:
	support for single and double-quoted strings
	support for triple-quoted versions of the same
	support for special character escapes as seen in 8-bit python strings
	support for octal and hexidecimal character escapes


	string_single_quote
	string_double_quote
	string_triple_single
	string_triple_double
		Individual string types with the above features

	string
		Any of the above string types, in a simple FirstOf group
		with the triple-quoted types first, then the single quoted
		i.e. generated with this grammar:

		string_triple_double/string_triple_single/string_double_quote/string_single_quote
		

Interpreters:
	StringInterpreter
		Interprets any/all of the above as a normal (non-Raw) Python
		regular (non-unicode) string.  Hopefully the action is identical
		to doing eval( matchedString, {},{}), without the negative security
		implications of that approach.  Note that you need to make the
		interpreter available under each name you use directly in your
		grammar, so if you use string_single_quote and string_double_quote
		directly, then you need to add:
			string_single_quote = myStringInterpreterInstance
			string_double_quote = myStringInterpreterInstance
		to your processor class.
"""

from simpleparse.parser import Parser
from simpleparse import common, objectgenerator
from simpleparse.common import chartypes
from simpleparse.dispatchprocessor import *
import string

c = {}

stringDeclaration = r"""
# note that non-delimiter can never be hit by non-triple strings
str              :=  delimiter, (char_no_quote/escaped_char/backslash_char/nondelimiter)*,delimiter

escaped_char        :=  '\\',( string_special_escapes / ('x',hex_escaped_char) / octal_escaped_char )
octal_escaped_char  :=  octdigit, octdigit?, octdigit?
hex_escaped_char    :=  hexdigit,hexdigit

backslash_char      :=  "\\" # i.e. a backslash preceding a non-special char

"""

_stringTypeData = [
	("string_double_quote", """
<delimiter>                :=  '"'
nondelimiter               :=  -'"'
char_no_quote              :=  -[\\\\"]+
string_special_escapes     := [\\\\abfnrtv"]
"""),
	("string_single_quote", """
<delimiter>                :=  "'"
nondelimiter               :=  -"'"
char_no_quote              :=  -[\\\\']+
string_special_escapes     := [\\\\abfnrtv']
"""),
	("string_triple_single", """
nondelimiter               :=  -"'''"
<delimiter>                :=  "'''"
char_no_quote              :=  -[\\\\']+
string_special_escapes     := [\\\\abfnrtv']
"""),
	("string_triple_double",'''
nondelimiter               :=  -'"""'
<delimiter>                :=  '"""'
char_no_quote              :=  -[\\\\"]+
string_special_escapes     := [\\\\abfnrtv"]
'''),
]

for name, partial in _stringTypeData:
	_p = Parser( stringDeclaration + partial )
	c[ name ] = objectgenerator.LibraryElement(
		generator = _p._generator,
		production = "str",
	)
common.share( c )
_p = Parser( """
string :=  string_triple_double/string_triple_single/string_double_quote/string_single_quote
""" )
c[ "string"] = objectgenerator.LibraryElement(
	generator = _p._generator,
	production = "string",
)

class StringInterpreter(DispatchProcessor):
	"""Processor for converting parsed string values to their "intended" value

	Basically this processor handles de-escaping and stripping the
	surrounding quotes, so that you get the string as a Python string
	value.  You use the processor by creating an instance of
	StringInterpreter() as an item in another processor's
	methodSource object (often the Parser itself).

	For example:

		class MyProcessor( DispatchProcessor ):
			string = StringInterpreter()
			
			# following would be used if you have, for instance,
			# used string_single_quote in an area where double
			# or triple-quoted strings are not allowed, but have
			# used string in another area.
			string_single_quote = string
	"""
	def string( self, (tag, left, right, sublist), buffer):
		"""Dispatch any of the string types and return the result"""
		return dispatch( self, sublist[0], buffer )

	def string_single_quote( self, (tag, left, right, sublist), buffer):
		return string.join(dispatchList(self, sublist, buffer), "")
	string_double_quote = string_single_quote
	string_triple_single = string_single_quote
	string_triple_double = string_single_quote
		
	def char_no_quote( self, (tag, left, right, sublist), buffer):
		return buffer[left:right]
	nondelimiter = char_no_quote

	def escaped_char( self, (tag, left, right, sublist), buffer):
		return string.join(dispatchList(self,sublist,buffer), "")
	
	def octal_escaped_char(self, (tag, left, right, sublist), buffer):
		return chr(string.atoi( buffer[left:right], 8 ))
	def hex_escaped_char( self, (tag, left, right, sublist), buffer):
		return chr(string.atoi( buffer[left:right], 16 ))
	
	def backslash_char( self, (tag, left, right, sublist), buffer):
		return "\\"

	def string_special_escapes( self, (tag, left, right, sublist), buffer):
		"""Maps "special" escapes to the corresponding characters"""
		return self.specialescapedmap[ buffer[left:right]]
	specialescapedmap = {
	'a':'\a',
	'b':'\b',
	'f':'\f',
	'n':'\n',
	'r':'\r',
	't':'\t',
	'v':'\v',
	'\\':'\\',
	'\n':'',
	'"':'"',
	"'":"'",
	}
	
