"""Dispatch-processor API

This is a post-processing processor API based on dispatching
each element of a result tree in a top-down recursive call
structure.  It is the API used by the SimpleParseGrammar Parser,
and likely will be the default processor for SimpleParse.
"""
from simpleparse.processor import Processor

class DispatchProcessor(Processor):
	"""Dispatch results-tree in a top-down recursive pattern with
	attribute lookup to determine production -> method correspondence.

	To use the class, subclass it, then define methods for
	processing each production.  The methods should take this form:
		def production_name( self, (tag, left, right, children), buffer):
			pass
	Where children may be either a list, or None, and buffer is the
	entire buffer being parsed.
	"""
	def __call__( self, value, buffer ):
		"""Process the results of the parsing run over buffer

		Value can either be: (success, tags, next) for a top-level
		production, or (tag, left, right, children) for a non-top
		production.
		"""
		if len( value ) == 3:
			# is a top-level production
			success, tags, next = value
			if success:
				result = dispatchList( self, tags, buffer )
				return success, result, next
			else:
				return success, tags, next
		else:
			# is a 4-item result tuple/tree
			return dispatch( self, value, buffer )


def dispatch( source, tag, buffer ):
	"""Dispatch on source for tag with buffer

	Find the attribute or key tag[0] of source,
	then call it with (tag, buffer)
	"""
	try:
		function = getattr (source, tag[0])
	except AttributeError:
		try:
			function = source[tag[0]]
		except:
			raise AttributeError( '''No processing function for tag "%s" in object %s! Check the parser definition!'''%(tag[0], repr(source)))
	return function( tag, buffer )

def dispatchList( source, taglist, buffer ):
	"""Dispatch on source for each tag in taglist with buffer"""
	if taglist:
		return map( dispatch, [source]*len(taglist), taglist, [buffer]*len(taglist))
	else:
		return []

try:
	{}.setdefault
except AttributeError:
	def multiMap( taglist ):
		"""Convert the taglist to a mapping from tag-object:[list-of-tags]
		(slower non-setdefault version for older Python versions)"""
		set = {}
		if not taglist:
			return set
		for tag in taglist:
			key = tag[0]
			if source and buffer:
				tag = dispatch( source, tag, buffer )
			set[key] = set.get(key, []) + [tag]
		return set
else:
	def multiMap( taglist, source=None, buffer=None ):
		"""Convert a taglist to a mapping from tag-object:[list-of-tags]
		
		For instance, if you have items of 3 different types, in any order,
		you can retrieve them all sorted by type with multimap( childlist)
		then access them by tagobject key.
		"""
		set = {}
		if not taglist:
			return set
		for tag in taglist:
			key = tag[0]
			if source and buffer:
				tag = dispatch( source, tag, buffer )
			set.setdefault(key,[]).append( tag )
		return set
def singleMap( taglist, source=None, buffer=None ):
	"""Convert a taglist to a mapping from tag-object:tag, overwritting early with late tags"""
	set = {}
	if not taglist:
		return set
	for tag in taglist:
		key = tag[0]
		if source and buffer:
			tag = dispatch( source, tag, buffer )
		set[key] = tag
	return set
	
def getString( (tag, left, right, sublist), buffer):
	"""Return the string value of the tag passed"""
	return buffer[ left:right ]

try:
	from simpleparse.stt.TextTools import countlines
except ImportError:
	def lines( start=None, end=None, buffer=None ):
		"""Return line number in file at character index (string.count version)"""
		return string.count(buffer, '\n', start or 0, end or len(buffer) )
else:
	def lines( start=None, end=None, buffer=None ):
		"""Return line number in file at character index (mx.TextTools version)"""
		return countlines (buffer[start or 0:end or len(buffer)])
	
	
