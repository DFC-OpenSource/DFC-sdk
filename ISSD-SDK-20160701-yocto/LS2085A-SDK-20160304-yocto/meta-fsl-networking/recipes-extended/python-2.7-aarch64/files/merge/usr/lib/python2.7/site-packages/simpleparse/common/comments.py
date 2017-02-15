"""Common comment formats

To process, handle the "comment" production,
(the specific named comment formats are all
expanded productions, so you won't get them
returned for processing).

	hash_comment
		# to EOL comments
	slashslash_comment
		// to EOL comments
	semicolon_comment
		; to EOL comments
	slashbang_comment
	c_comment
		non-nesting /* */ comments
	slashbang_nest_comment
	c_nest_comment
		nesting /* /* */ */ comments
"""
from simpleparse.parser import Parser
from simpleparse import common, objectgenerator
from simpleparse.common import chartypes

c = {}

eolcomments = r"""
### comment formats where the comment goes
### from a marker to the end of the line

comment   := -'\012'*
<EOL>       := ('\r'?,'\n')/EOF

>hash_comment< := '#', comment, EOL
>semicolon_comment< := ';', comment, EOL
>slashslash_comment< := '//', comment, EOL
"""

_p = Parser( eolcomments )
for name in ["hash_comment", "semicolon_comment", "slashslash_comment"]:
	c[ name ] = objectgenerator.LibraryElement(
		generator = _p._generator,
		production = name,
	)

ccomments = r"""
### comments in format /* comment */ with no recursion allowed
comment := -"*/"*
>slashbang_comment< := '/*', comment, '*/'
"""
_p = Parser( ccomments )
for name in ["c_comment","slashbang_comment"]:
	c[ name ] = objectgenerator.LibraryElement(
		generator = _p._generator,
		production = "slashbang_comment",
	)

nccomments = r"""
### nestable C comments of form /* comment /* innercomment */ back to previous */
<comment_start>          := '/*'
<comment_stop>           := '*/'
comment                  := (-(comment_stop/comment_start)+/slashbang_nest_comment)*
>slashbang_nest_comment< := comment_start, comment, comment_stop
"""
_p = Parser( nccomments )
for name in ["c_nest_comment","slashbang_nest_comment"]:
	c[ name ] = objectgenerator.LibraryElement(
		generator = _p._generator,
		production = "slashbang_nest_comment",
	)

common.share(c)

