"""Object-oriented tag-table generator objects

The objectgenerator module is the core of the SimpleParse
system, the various element token classes defined here
implement transitions from EBNF-style abstractions into
the low-level (assembly-like) instructions to the
TextTools engine.

Each class within the module is a sub-class of ElementToken,
which provides a number of common facilities, the most
obvious of which is the permute method, which takes care of
the negative, optional, and repeating flags for the normal
case (with character ranges and literals being non-normal).
"""
from simpleparse.stt.TextTools.TextTools import *

### Direct use of BMS is deprecated now...
try:
	TextSearch
except NameError:
	TextSearch = BMS

from simpleparse.error import ParserSyntaxError
import copy

class ElementToken:
	"""Abstract base class for all ElementTokens

	Common Attributes:
	
		negative -- the element token should match
			a character if the "base" definition
			would not match at the current position
		optional -- the element token will match even
			if the base definition would not match
			at the current position
		repeating -- if the element is successfully
			matched, attempt to match it again.
		lookahead -- if true, the scanning position
			of the engine will be reset after the
			element matches
		errorOnFail -- if true, the engine will call the
			object stored in errorOnFail as a text-
			matching object iff the element token fails
			to match.  This is used to signal
			SyntaxErrors.
			
	Attributes only used for top-level Productions:
	
		report -- if true, the production's results
			will be added to the result tree
		expanded -- if true, the production's children's
			results will be added to the result tree
			but the production's own result will be ignored
	"""
	negative = 0
	optional = 0
	repeating = 0
	report = 1
	# note that optional and errorOnFail are mutually exclusive
	errorOnFail = None
	# any item may be marked as expanded,
	# which says that it's a top-level declaration
	# and that links to it should automatically expand
	# as if the name wasn't present...
	expanded = 0
	lookahead = 0
	
	
	def __init__( self, **namedarguments ):
		"""Initialize the object with named attributes

		This method simply takes the named attributes and
		updates the object's dictionary with them
		"""
		self.__dict__.update( namedarguments )
	def toParser( self, generator, noReport=0 ):
		"""Abstract interface for implementing the conversion to a text-tools table

		generator -- an instance of generator.Generator
			which provides various facilities for discovering
			other productions.
		noReport -- if true, we're being called recursively
			for a terminal grammar fragment where one of our
			parents has explicitly suppressed all reporting.

		This method is called by the generator or by
		another element-token's toParser method.
		"""
		raise NotImplementedError( '''Element token generator abstract function called''' )
	def permute( self, basetable ):
		'''Given a positive, required, non-repeating table, convert to appropriately configured table

		This method applies generic logic for applying the
		operational flags to a basic recipe for an element.
		
		It is normally called from the elements-token's own
		toParser method.
		'''
		flags = 0
		if self.lookahead:
			flags = flags + LookAhead
			
		assert len(basetable) == 3, '''Attempt to permute a base table that already has fail flag set, can only permute unadorned tables'''
		if self.negative:
			# negative "matches" if it fails
			# we add in the flags while we're at it...
			basetable = (None, SubTable+flags, (
				basetable + (1,2),
				(None, EOF, Here,2,1), # if we hit eof, this didn't match, otherwise, we matched
				(None, Fail, Here),# either hit eof or matched the client
				(None,Skip,1),
			))
		elif flags:
			# unpack, add the flags, and repack
			tag, command, arg = basetable
			basetable = ( tag, command+flags, arg)
			
		if self.repeating:
			### There are a number of problems with repetition that we'd like to solve
			### via recursive table calls, but those are very expensive in the current
			### implementation, so we need to use something a little more hacky...
			if self.optional:
				return [
					## this would be the "simplistic" implementation...
					## basetable + (1,0)
					## it doesn't work because of cases
					## where all-optional children "succeed" without consuming
					## when within a repeating parent
					## the EOF test isn't enough to fix the problem,
					## as it's only checking a common case, not the underlying failure
					basetable +(2,1), # fail, done, succeed, check for eof and if not, try matching again
					# if we hit eof, no chance of further matches,
					# consider ourselves done
					(None, EOF, Here,-1,1),
				]
			elif self.errorOnFail:
				return [
					basetable+(1,2),
					(None, Call, self.errorOnFail),
					# as for optional...
					basetable +(2,1),
					(None, EOF, Here,-1,1),
				]
			else:
				return [
					basetable,
					# as for optional...
					basetable +(2,1),
					(None, EOF, Here,-1,1),
				]
		else: # single
			if self.optional:
				return [
					basetable +(1,1)
				]
			elif self.errorOnFail:
				return [
					basetable+(1,2),
					(None, Call, self.errorOnFail),
				]
			else: # not optional
				return [
					basetable
				]
	def __repr__( self):
		"""Return a readily recognisable version of ourself"""
		from simpleparse import printers
		return printers.asObject( self )
	def terminal (self, generator):
		"""Determine if this element is terminal for the generator"""
		return 0
		

class Literal( ElementToken ):
	"""Literal string value to be matched

	Literals are one of the most common elements within
	any grammar.  The implementation tries to use the
	most efficient mechanism available for matching/searching
	for a literal value, so the Literal class does not
	use the permute method, instead defining explicit
	parsing methodologies for each flag and value combination

	Literals in the SimpleParse EBNF grammar are defined like so:
		"test", "test"?, "test"*, "test"+
		-"test", -"test"?, -"test"*, -"test"+

	Attributes:
		value -- a string storing the literal's value

	Notes:
		Currently we don't support Unicode literals
		
	See also:
		CILiteral -- case-insensitive Literal values
	"""
	value = ""
	def toParser( self, generator=None, noReport=0 ):
		"""Create the parser for the element token"""
		flags = 0
		if self.lookahead:
			flags = flags + LookAhead
		base = self.baseToParser( generator )
		if flags or self.errorOnFail:
			if self.errorOnFail:
				return [(None, SubTable+flags, tuple(base),1,2),(None, Call, self.errorOnFail)]
			else:
				return [(None, SubTable+flags, tuple(base))]
		else:
			return base
	def baseToParser( self, generator=None ):
		"""Parser generation without considering flag settings"""
		svalue = self.value
		if self.negative:
			if self.repeating: # a repeating negative value, a "search" in effect
				if self.optional: # if fails, then go to end of file
					return [ (None, sWordStart, TextSearch( svalue ),1,2), (None, Move, ToEOF ) ]
				else: # must first check to make sure the current position is not the word, then the same
					return [
						(None, Word, svalue, 2,1),
						(None, Fail, Here),
						(None, sWordStart, TextSearch( svalue ),1,2),
						(None, Move, ToEOF )
					]
					#return [ (None, Word, svalue, 2,1),(None, Fail, Here),(None, WordStart, svalue,1,2), (None, Move, ToEOF ) ]
			else: # a single-character test saying "not a this"
				if self.optional: # test for a success, move back if success, move one forward if failure
					if len(svalue) > 1:
						return [ (None, Word, svalue, 2,1), 
							(None, Skip, -len(svalue), 2,2), # backup if this was the word to start of word, succeed
							(None, Skip, 1 ) ] # else just move one character and succeed
					else: # Uses Is test instead of Word test, should be faster I'd imagine
						return [ (None, Is, svalue, 2,1), 
							(None, Skip, -1, 2,2), # backtrack
							(None, Skip, 1 ) ] # else just move one character and succeed
				else: # must find at least one character not part of the word, so
					if len(svalue) > 1:
						return [ (None, Word, svalue, 2,1), 
							(None, Fail, Here),
							(None, Skip, 1 ) ] # else just move one character and succeed
					else: #must fail if it finds or move one forward
						return [ (None, Is, svalue, 2,1), 
							(None, Fail, Here),
							(None, Skip, 1 ) ] # else just move one character and succeed
		else: # positive
			if self.repeating:
				if self.optional:
					if len(svalue) > 1:
						return [ (None, Word, svalue, 1,0) ]
					else:
						return [ (None, Is, svalue, 1,0) ]
				else: # not optional
					if len(svalue) > 1:
						return [ (None, Word, svalue),(None, Word, svalue,1,0) ]
					else:
						return [ (None, Is, svalue),(None, Is, svalue,1,0) ]
			else: # not repeating
				if self.optional:
					if len(svalue) > 1:
						return [ (None, Word, svalue, 1,1) ]
					else:
						return [ (None, Is, svalue, 1,1) ]
				else: # not optional
					if len(svalue) > 1:
						return [ (None, Word, svalue) ]
					else:
						return [ (None, Word, svalue) ]
	def terminal (self, generator):
		"""Determine if this element is terminal for the generator"""
		return 1

class _Range( ElementToken ):
	"""Range of character values where any one of the characters may match

	The Range token allows you to define a set of characters
	(using a mini-grammar) of which any one may match.  By using
	the repetition flags, it is possible to easily create such
	common structures as "names" and "numbers".  For example:

		name := [a-zA-Z]+
		number := [0-9.eE]+

	(Note: those are not beautifully defined examples :) ).

	The mini-grammar for the simpleparsegrammar is defined as follows:

		'[',CHARBRACE?,CHARDASH?, (CHARRANGE/CHARNOBRACE)*, CHARDASH?,']'
		
	that is, if a literal ']' character is wanted, you must
	define the character as the first item in the range.  A literal
	'-' character must appear as the first character after any
	literal ']' character (or the beginning of the range) or as the
	last character in the range.

	Note: The expansion from the mini-grammar occurs before the
	Range token is created (the simpleparse grammar does the
	expansion), so the value attribute of the token is actually
	the expanded string of characters.
	"""
	value = ""
	requiresExpandedSet = 1
	def toParser( self, generator=None, noReport=0 ):
		"""Create the parser for the element token"""
		flags = 0
		if self.lookahead:
			flags = flags + LookAhead
		base = self.baseToParser( generator )
		if flags or self.errorOnFail:
			if self.errorOnFail:
				return [(None, SubTable+flags, tuple(base),1,2),(None, Call, self.errorOnFail)]
			else:
				return [(None, SubTable+flags, tuple(base))]
		else:
			return base

# this should be a faster and more generic character set
# approach, but there's a bug with mxTextTools b3 which makes
# it non-functional, so for now I'm using the old version.
# Eventually this should also support the Unicode character sets
##try:
##	CharSet
##	class Range( _Range ):
##		"""Range type using the CharSet feature of mx.TextTools 2.1.0
##
##		The CharSet type allows for both Unicode and 256-char strings,
##		so we can use it as our 2.1.0 primary parsing mechanism.
##		It also allows for simpler definitions (doesn't require that
##		we pre-exand the character set).  That's going to require support
##		in the SimpleParse grammar, of course.
##		"""
##		requiresExpandedSet = 0
##		def baseToParser( self, generator=None ):
##			"""Parser generation without considering flag settings"""
##			svalue = self.value
##			print 'generating range for ', repr(svalue)
##			if not svalue:
##				raise ValueError( '''Range defined with no member values, would cause infinite loop %s'''%(self))
##			if self.negative:
##				svalue = '^' + svalue
##			print '  generated', repr(svalue)
##			svalue = CharSet(svalue)
##			if self.repeating:
##				if self.optional:
##					return [ (None, AllInCharSet, svalue, 1 ) ]
##				else: # not optional
##					#return [ (None, AllInSet, svalue ) ]
##					return [ (None, AllInCharSet, svalue ) ]
##			else: # not repeating
##				if self.optional:
##					#return [ (None, IsInSet, svalue, 1 ) ]
##					return [ (None, IsInCharSet, svalue, 1 ) ]
##				else: # not optional
##					#return [ (None, IsInSet, svalue ) ]
##					return [ (None, IsInCharSet, svalue ) ]
##except NameError:
class Range( _Range ):
	"""Range type which doesn't use the CharSet features in mx.TextTools

	This is likely to be much slower than the CharSet version (below), and
	is unable to handle unicode character sets.  However, it will work with
	TextTools 2.0.3, which may be needed in some cases.
	"""
	def baseToParser( self, generator=None ):
		"""Parser generation without considering flag settings"""
		svalue = self.value
		if not svalue:
			raise ValueError( '''Range defined with no member values, would cause infinite loop %s'''%(self))
		if self.negative:
			if self.repeating:
				if self.optional:
					#return [ (None, AllInSet, svalue, 1 ) ]
					return [ (None, AllNotIn, svalue, 1 ) ]
				else: # not optional
					#return [ (None, AllInSet, svalue ) ]
					return [ (None, AllNotIn, svalue ) ]
			else: # not repeating
				if self.optional:
					#return [ (None, IsInSet, svalue, 1 ) ]
					return [ (None, IsNotIn, svalue, 1 ) ]
				else: # not optional
					#return [ (None, IsInSet, svalue ) ]
					return [ (None, IsNotIn, svalue ) ]
		else:
			if self.repeating:
				if self.optional:
					#return [ (None, AllInSet, svalue, 1 ) ]
					return [ (None, AllIn, svalue, 1 ) ]
				else: # not optional
					#return [ (None, AllInSet, svalue ) ]
					return [ (None, AllIn, svalue ) ]
			else: # not repeating
				if self.optional:
					#return [ (None, IsInSet, svalue, 1 ) ]
					return [ (None, IsIn, svalue, 1 ) ]
				else: # not optional
					#return [ (None, IsInSet, svalue ) ]
					return [ (None, IsIn, svalue ) ]
	def terminal (self, generator):
		"""Determine if this element is terminal for the generator"""
		return 1

class Group( ElementToken ):
	"""Abstract base class for all group element tokens

	The primary feature of a group is that it has a set
	of element tokens stored in the attribute "children".
	"""
	children = ()
	terminalValue = None
	def terminal (self, generator):
		"""Determine if this element is terminal for the generator"""
		if self.terminalValue in (0,1):
			return self.terminalValue
		self.terminalValue = 0
		for item in self.children:
			if not item.terminal( generator):
				return self.terminalValue
		self.terminalValue = 1
		return self.terminalValue
	
class SequentialGroup( Group ):
	"""A sequence of element tokens which must match in a particular order

	A sequential group must match each child in turn
	and all children must be satisfied to consider the
	group matched.

	Within the simpleparsegrammar, the sequential group
	is defined like so:
		("a", b, c, "d")
	i.e. a series of comma-separated element token definitions.
	"""
	def toParser( self, generator=None, noReport=0 ):
		elset = []
		for child in self.children:
			elset.extend( child.toParser( generator, noReport ) )
		basic = self.permute( (None, SubTable, tuple( elset)) )
		if len(basic) == 1:
			first = basic[0]
			if len(first) == 3 and first[0] is None and first[1] == SubTable:
				return tuple(first[2])
		return basic
			
class CILiteral( SequentialGroup ):
	"""Case-insensitive Literal values

	The CILiteral is a sequence of literal and
	character-range values, where each element is
	positive and required.  Literal values are
	composed of those characters which are not
	upper-case/lower-case pairs, while the ranges
	are all two-character ranges with the upper
	and lower forms.

	CILiterals in the SimpleParse EBNF grammar are defined like so:
		c"test", c"test"?, c"test"*, c"test"+
		-c"test", -c"test"?, -c"test"*, -c"test"+

	Attributes:
		value -- a string storing the literal's value

	Notes:
		Currently we don't support Unicode literals

		A CILiteral will be *much* slower than a
		regular literal or character range
	"""
	value = ""
	def toParser( self, generator=None, noReport=0 ):
		elset = self.ciParse( self.value )
		if len(elset) == 1:
			# XXX should be compressing these out during optimisation...
			# pointless declaration of case-insensitivity,
			# or a single-character value
			pass
		basic = self.permute( (None, SubTable, tuple( elset)) )
		if len(basic) == 1:
			first = basic[0]
			if len(first) == 3 and first[0] is None and first[1] == SubTable:
				return tuple(first[2])
		return basic
	def ciParse( self, value ):
		"""Break value into set of case-dependent groups..."""
		def equalPrefix( a,b ):
			for x in range(len(a)-1):
				if a[x] != b[x]:
					return x
		result = []
		a,b = value.upper(), value.lower()
		while a and b:
			# is there an equal literal run at the start?
			stringPrefix = equalPrefix( a,b )
			if stringPrefix:
				result.append( (None, Word, a[:stringPrefix]) )
				a,b = a[stringPrefix:],b[stringPrefix:]
			# if we hit the end of the string, that's fine, just return
			if not a and b:
				break
			# otherwise, the next character must be a case-differing pair
			result.append( (None, IsIn, a[0]+b[0]) )
			a,b = a[1:], b[1:]
		return result
		

class ErrorOnFail(ElementToken):
	"""When called as a matching function, raises a SyntaxError

	Attributes:
		expected -- list of strings describing expected productions
		production -- string name of the production that's failing to parse
		message -- overrides default message generation if non-null


	(something,something)+!
	(something,something)!
	(something,something)+!"Unable to parse somethings in my production"
	(something,something)!"Unable to parse somethings in my production"
	
	if string -> give an explicit message (with optional % values)
	else -> use a default string
	
	"""
	production = ""
	message = ""
	expected = ""
	def __call__( self, text, position, end ):
		"""Method called by mxTextTools iff the base production fails"""
		error = ParserSyntaxError( self.message )
		error.message = self.message
		error.production = self.production
		error.expected= self.expected
		error.buffer = text
		error.position = position
		raise error
	def copy( self ):
		import copy
		return copy.copy( self )
	



class FirstOfGroup( Group ):
	"""Set of tokens that matches (and stops searching) with the first successful child

	A FirstOf group attempts to match each child in turn,
	declaring success with the first successful child,
	or failure if none of the children match.

	Within the simpleparsegrammar, the FirstOf group
	is defined like so:
		("a" / b / c / "d")
	i.e. a series of slash-separated element token definitions.
	"""
	def toParser( self, generator=None, noReport=0 ):
		elset = []
		# should catch condition where a child is optional
		# and we are repeating (which causes a crash during
		# parsing), but doing so is rather complex and
		# requires analysis of the whole grammar.
		for el in self.children:
			assert not el.optional, """Optional child of a FirstOf group created, this would cause an infinite recursion in the engine, child was %s"""%el
			dataset = el.toParser( generator, noReport )
			if len( dataset) == 1:# and len(dataset[0]) == 3: # we can alter the jump states with impunity
				elset.append( dataset[0] )
			else: # for now I'm eating the inefficiency and doing an extra SubTable for all elements to allow for easy calculation of jumps within the FO group
				elset.append(  (None, SubTable, tuple( dataset ))  )

		procset = []
		for i in range( len( elset) -1): # note that we have to treat last el specially
			procset.append( elset[i] + (1,len(elset)-i) ) # if success, jump past end
		procset.append( elset[-1] ) # will cause a failure if last element doesn't match
		procset = tuple(procset)

		basetable = (None, SubTable, procset )
		return self.permute( basetable )

class Prebuilt( ElementToken ):
	"""Holder for pre-built TextTools tag tables

	You can pass in a Pre-built tag table when
	creating your grammar, doing so creates
	Prebuilt element tokens which can be referenced
	by the other element tokens in your grammar.
	"""
	value = ()
	def toParser( self, generator=None, noReport=0 ):
		return self.value
class LibraryElement( ElementToken ):
	"""Holder for a prebuilt item with it's own generator"""
	generator = None
	production = ""
	methodSource = None
	def toParser( self, generator=None, noReport=0 ):
		if self.methodSource is None:
			source = generator.methodSource
		else:
			source = self.methodSource
		basetable = self.generator.buildParser( self.production, source )
		try:
			if type(basetable[0]) == type(()):
				if len(basetable) == 1 and len(basetable[0]) == 3:
					basetable = basetable[0]
				else:
					# this is a table that got returned!
					basetable = (None, SubTable, basetable)
			return self.permute( basetable )
		except:
			print basetable
			raise

class Name( ElementToken ):
	"""Reference to another rule in the grammar

	The Name element token allows you to reference another
	production within the grammar.  There are three major
	sub-categories of reference depending on both the Name
	element token and the referenced table's values.

	if the Name token's report attribute is false,
	or the target table's report attribute is false,
	or the Name token negative attribute is true,
		the Name reference will report nothing in the result tree

	if the target's expand attribute is true, however,
		the Name reference will report the children
		of the target production without reporting the
		target production's results (SubTable match)

	finally:
		if the target is not expanded and the Name token
		should report something, the generator object is
		asked to supply the tag object and flags for
		processing the results of the target.  See the
		generator.MethodSource documentation for details.

	Notes:
		expanded and un-reported productions won't get any
		methodsource methods called when 
		they are finished, that's just how I decided to
		do it, not sure if there's some case where you'd
		want it.  As a result, it's possible to have a
		method getting called for one instance (where a
		name ref is reporting) and not for another (where
		the name ref isn't reporting).
	"""
	value = ""
	# following two flags are new ideas in the rewrite...
	report = 1
	def toParser( self, generator, noReport=0 ):
		"""Create the table for parsing a name-reference

		Note that currently most of the "compression" optimisations
		occur here.
		"""
		sindex = generator.getNameIndex( self.value )
		command = TableInList
		target = generator.getRootObjects()[sindex]

		reportSelf = (
			(not noReport) and # parent hasn't suppressed reporting
			self.report and # we are not suppressing ourselves
			target.report and # target doesn't suppress reporting
			(not self.negative) and # we aren't a negation, which doesn't report anything by itself
			(not target.expanded) # we don't report the expanded production
		)
		reportChildren = (
			(not noReport) and # parent hasn't suppressed reporting
			self.report and # we are not suppressing ourselves
			target.report and # target doesn't suppress reporting
			(not self.negative) # we aren't a negation, which doesn't report anything by itself
		)
		if reportSelf:
			svalue = self.value
		else:
			svalue = None

		flags = 0
		if target.expanded:
			# the target is the root of an expandedname declaration
			# so we need to do special processing to make sure that
			# it gets properly reported...
			command = SubTableInList
			tagobject = None
			# check for indirected reference to another name...
		elif not reportSelf:
			tagobject = svalue
		else:
			flags, tagobject = generator.getObjectForName( svalue )
			if flags:
				command = command | flags
		if tagobject is None and not flags:
			if self.terminal(generator):
				if extractFlags(self,reportChildren) != extractFlags(target):
					composite = compositeFlags(self,target, reportChildren)
					partial = generator.getCustomTerminalParser( sindex,composite)
					if partial is not None:
						return partial
					partial = tuple(copyToNewFlags(target, composite).toParser(
						generator,
						not reportChildren
					))
					generator.cacheCustomTerminalParser( sindex,composite, partial)
					return partial
				else:
					partial = generator.getTerminalParser( sindex )
					if partial is not None:
						return partial
					partial = tuple(target.toParser(
						generator,
						not reportChildren
					))
					generator.setTerminalParser( sindex, partial)
					return partial
		# base, required, positive table...
		if (
			self.terminal( generator ) and
			(not flags) and
			isinstance(target, (SequentialGroup,Literal,Name,Range))
		):
			partial = generator.getTerminalParser( sindex )
			if partial is None:
				partial = tuple(target.toParser(
					generator,
					#not reportChildren
				))
				generator.setTerminalParser( sindex, partial)
			if len(partial) == 1 and len(partial[0]) == 3 and (
				partial[0][0] is None or tagobject is None
			):
				# there is a single child
				# it doesn't report anything, or we don't
				partial = (partial[0][0] or tagobject,)+ partial[0][1:]
			else:
				partial = (tagobject, Table, tuple(partial))
			return self.permute( partial )
		basetable = (
			tagobject,
			command, (
				generator.getParserList (),
				sindex,
			)
		)
		return self.permute( basetable )
	terminalValue = None
	def terminal (self, generator):
		"""Determine if this element is terminal for the generator"""
		if self.terminalValue in (0,1):
			return self.terminalValue
		self.terminalValue = 0
		target = generator.getRootObject( self.value )
		if target.terminal( generator):
			self.terminalValue = 1
		return self.terminalValue


def extractFlags( item, report=1 ):
	"""Extract the flags from an item as a tuple"""
	return (
		item.negative,
		item.optional,
		item.repeating,
		item.errorOnFail,
		item.lookahead,
		item.report and report,
	)
def compositeFlags( first, second, report=1 ):
	"""Composite flags from two items into overall flag-set"""
	result = []
	for a,b in map(None, extractFlags(first, report), extractFlags(second, report)):
		result.append( a or b )
	return tuple(result)
def copyToNewFlags( target, flags ):
	"""Copy target using combined flags"""
	new = copy.copy( target )
	for name,value in map(None,
		("negative","optional","repeating","errorOnFail","lookahead",'report'),
		flags,
	):
		setattr(new, name,value)
	return new
