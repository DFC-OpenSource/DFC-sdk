"""Phonetic spellings for character values

At the moment, only contains the "military alphabet"
(Alpha, Bravo ... Yankee, Zulu), which is used as
alternative timezone names by the military and apparently
some aviation groups.  Note, these are fairly common spellings,
but they aren't necessarily going to match a particular
usage.  I may have missed some of the possibilities...

	military_alphabet_char -- fully spelled out versions of
		the Alpha, Bravo ... Yankee, Zulu phonetic alphabet,
		including a few minor variations in spelling such as
		Xray and X-ray.  All characters use title-caps format,
		so Zulu, not zulu will match.
	military_alphabet_char_lower -- as for above, but with
		lowercased versions of the above

No interpreters are provided.  Taking the first character of
the name will always give you the equivalent character uppercase
for the military_alphabet_char and lowercase for the
military_alphabet_char_lower.
"""
from simpleparse import objectgenerator, common
import string

c = {}

# note that Juliette comes before Juliet, because
# otherwise Juliette could never match in an FOGroup!
_letters = string.split( """Alpha
Bravo
Charlie
Delta
Echo Echo
Foxtrot
Golf Gulf
Hotel
India
Juliette Juliet
Kilo
Lima
Mike
November
Oscar
Papa
Quebec
Romeo
Sierra
Tango
Uniform
Victor
Whiskey
Xray X-ray
Yankee
Zulu""")

set1,set2 = [], []
for item in _letters:
	set1.append(
		objectgenerator.Literal( value=item)
	)
	set2.append(
		objectgenerator.Literal( value=string.lower(item))
	)

military_alphabet_char = objectgenerator.FirstOfGroup(
	children = set1
)
military_alphabet_char_lower = objectgenerator.FirstOfGroup(
	children = set2
)
del set1, set2

c[ "military_alphabet_char" ] = military_alphabet_char
c[ "military_alphabet_char_lower" ] = military_alphabet_char_lower

common.share( c )

