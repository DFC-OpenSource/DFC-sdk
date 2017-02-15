"""Common locale-specific character types

Following productions are all based on string module,
with the default locale specified.  The first production
is a single character of the class and the second a
repeating character version:

	digit, digits
	uppercasechar, uppercase
	lowercasechar, lowercase
	letter, letters
	whitespacechar, whitespace
	punctuationchar, punctuation
	octdigit, octdigits
	hexdigit, hexdigits
	printablechar, printable

For Python versions with the constants in the string module:
	ascii_letter, ascii_letters
	ascii_lowercasechar, ascii_lowercase
	ascii_uppercasechar, ascii_uppercase


Following are locale-specific values, both are
single-character values:

	locale_decimal_point -- locale-specific decimal seperator
	locale_thousands_seperator -- locale-specific "thousands" seperator
	
Others:

	EOF -- Matches iff parsing has reached the end of the buffer

There are no interpreters provided (the types are considered
too common to provide meaningful interpreters).
"""
from simpleparse import objectgenerator, common
import string, locale
locale.setlocale(locale.LC_ALL, "" )

c = {}

# string-module items...

for source,single,repeat in [
	("digits","digit","digits"),
	("uppercase", "uppercasechar", "uppercase"),
	("lowercase", "lowercasechar", "lowercase"),
	("letters", "letter", "letters" ),
	("ascii_lowercase", "ascii_lowercasechar", "ascii_lowercase"),
	("ascii_uppercase", "ascii_uppercasechar", "ascii_uppercase"),
	("ascii_letters", "ascii_letter", "ascii_letters" ),
	("whitespace", "whitespacechar", "whitespace"),
	("punctuation", "punctuationchar", "punctuation"),
	("octdigits", "octdigit", "octdigits"),
	("hexdigits", "hexdigit", "hexdigits"),
	("printable", "printablechar", "printable"),
]:
	try:
		value = getattr( string, source )
		c[ single ] = objectgenerator.Range( value = value )
		c[ repeat ] = objectgenerator.Range( value = value, repeating =1 )
	except AttributeError:
		pass

# locale-module items
_lc = locale.localeconv()
c[ "locale_decimal_point" ] = objectgenerator.Literal( value = _lc["decimal_point"] )
c[ "locale_thousands_seperator" ] = objectgenerator.Literal( value = _lc["thousands_sep"] )

del _lc

# common, but not really well defined sets
# this is the set of characters which are interpreted
# specially by Python's string-escaping when they
# follow a \\ char.

from simpleparse.stt import TextTools
c[ "EOF" ] = objectgenerator.Prebuilt( value = (
	(None, TextTools.EOF, TextTools.Here),
) )

common.share( c )
