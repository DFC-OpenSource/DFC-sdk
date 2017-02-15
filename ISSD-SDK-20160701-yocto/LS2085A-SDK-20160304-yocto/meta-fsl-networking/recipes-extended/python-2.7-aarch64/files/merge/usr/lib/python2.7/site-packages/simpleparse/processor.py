"""Definitions of the MethodSource and Processor APIs"""

class MethodSource:
	"""Base class for MethodSource objects (including Processors and Parsers)
	Most applications will use either Processor or Parser objects, rather
	than directly using a MethodSource object.

	The MethodSource is basically just a generic object whose attributes
	are accessed during generation and/or post-processing of parse results.
	The following are the special attribute forms for use in 

		_m_productionname -- alters the method used in the TextTools
			engine for storing results.  If this is a callable object,
			then call the object with:
				object( taglist,text,l,r,subtags )
				
			If it is TextTools.AppendToTagobj, then append the result
			tuple to the associated object (_o_productionname).  This
			requires that _o_productionname have an "append" method,
			obviously.

			If it is the constant TextTools.AppendMatch, then append
			the string value which matched the production.
			
			If it is TextTools.AppendTagobj, then append the associated
			tagobject itself to the results tree.
			
		_o_productionname -- with AppendToTagobj, AppendTagobj and
			cases where there is no _m_productionname defined, this
			allows you to provide an explicit tagobject for reporting
			in the results tree/getting called with results.
	"""



class Processor(MethodSource):
	"""Provides definition of a generic processing API

	Basically, a Processor has a method __call__ which takes
	two arguments, a value (which is either a 3-tuple or a 4-tuple
	depending on whether a top-level production is being processed),
	and a pointer to the buffer being parsed.
	"""
	def __call__( self, value, buffer ):
		"""Process the results of a parsing run over buffer"""
		return value
	def __repr__( self ):
		"""Return a representation of the class"""
		return "<%s object @ %s>"%( self.__class__.__name__, id(self))
