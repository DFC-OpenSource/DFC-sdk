from simpleparse.xml import xml_parser
from simpleparse.parser import Parser
import unittest, string

p = Parser( xml_parser.declaration )

class XMLProductionTests(unittest.TestCase):
	"""Tests that XML grammar productions match appropriate values"""
	### ProductionTests will be added here by loop below...

class ProductionTest:
	def __init__( self, production, should, shouldnot ):
		self.production = production
		self.should = should
		self.shouldnot = shouldnot
	def __call__( self ):
		"""Perform the test"""
		for item in self.should:
			success, children, next = p.parse( item, self.production )
			assert success, """Didn't parse %s as a %s, should have"""%( repr(item), self.production)
			assert next == len(item), """Didn't parse whole of %s as a %s, parsed %s of %s characters, results were:\n%s\nRest was:\n%s"""%( repr(item), self.production, next, len(item), children, item[next:])
		for item in shouldnot:
			success, children, next = p.parse( item, self.production )
			assert not success, """Parsed %s chars of %s as a %s, shouldn't have, result was:\n%s"""%( next, repr(item), self.production, children)

def getSuite():
	return unittest.makeSuite(XMLProductionTests, 'test')



testData = {
	"CharData":(
		[# should match
			"""Type """,
		],
		[# should not match
		],
	),
	"Attribute":(
		[# should match
			"""s=&this;""",
			'''s="&this;"''',
			"""&this;""",
		],
		[# should not match
			# unfinished elements
		],
	),
		
	"element":(
		[# should match
			"""<key &this;/>""",
			"""<key s=&this;/>""",
			"""<key &this;></key>""", 
			"""<key s="&this;"/>""",
			"""<key/>""",
		],
		[# should not match
			# unfinished elements
			"""<key>""",
			"""<key &this;>""", 
			"""<key s=&this;>""",
			# end with no start...
			"""</key>""",
			# malformed end tags
			"""<key></key &this;>""", 
			"""<key></key s=&this;>""",
		],
	),
		
	"content":(
		[# should match
			"""Type <key>less-than</key> (&#x3C;) to save options.
This document was prepared on &docdate; and
is classified &security-level;.""",
			"""<key &this;/>""",
			"""<key s=&this;/>""",
			"""<key &this;></key>""", 
			"""<key s="&this;"/>""",
			"""&this;""",
			"""<key/>""",
		],
		[# should not match
			# unfinished elements
			"""<key>""",
			"""<key &this;>""", 
			"""<key s=&this;>""",
			# end with no start...
			"""</key>""",
			# malformed end tags
			"""<key></key &this;>""", 
			"""<key></key s=&this;>""",
		],
	),
	"AttValue":(
		[# should match
			'''"&this;"''',
		],
		[# should not match
		],
	),
		
	"Name": (
		[# should match
			"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ-:._",
			"_a",
			":a",
			":a",
		],
		[# should not match
			"-a",
			"0",
			"0.0",
			".this",
		],
	),
	"Comment": (
		[# should match
			"<!-- testing -->",
			"<!---->",
			"<!--- -->",
			"<!-- -- -- -->",
			"<!-- - - -->",
			"<!-- declarations for <head> & <body> -->",
		],
		[# should not match
			"<!-- -- -->",
			"<!-->",
			"<!-- B+, B, or B--->",
		],
	),
	"prolog": (
		[ # should match
			"""<?xml version="1.0"?> <!DOCTYPE greeting SYSTEM "hello.dtd">""",
			"""<?xml version="1.0" encoding="UTF-8" ?>
				<!DOCTYPE greeting [
				  <!ELEMENT greeting (#PCDATA)>
			]>""",
			"""<?xml version="1.0" standalone='yes'?>""",
			"""<?xml version="1.0" encoding="UTF-8" ?>
				<!DOCTYPE greeting [
					<!ELEMENT greeting (#PCDATA)>
					<!ATTLIST greeting
						  id      ID      #REQUIRED
						  name    CDATA   #IMPLIED>
			]>""",

			"""<?xml version="1.0" encoding="UTF-8" ?>
				<!DOCTYPE greeting [
				<!-- declare the parameter entity "ISOLat2"... -->
				<!ENTITY % ISOLat2
						 SYSTEM "http://www.xml.com/iso/isolat2-xml.entities" >
				<!-- ... now reference it. -->
				%ISOLat2;
			]>""",
			
		],
		[ # should not match
		],
	),
	
	"ExternalID": (
		[# should match
			'''SYSTEM "hello.dtd"''',
		],
		[# should not match
		],
	),
	"elementdecl": (
		[# should match
			'''<!ELEMENT br EMPTY>''',
			"""<!ELEMENT p (#PCDATA|emph)* >""",
			"""<!ELEMENT container ANY >""",
			"""<!ELEMENT %name.para; %content.para; >""",
			"""<!ELEMENT %name.para; ANY >""",
			"""<!ELEMENT yada (a|b|c|d)>""",
			"""<!ELEMENT %yada; (a|b|c|%others;) >""",
			"""<!ELEMENT %yada; (a|b|c|%others;) >""",
		],
		[# should not match
			"""<!ELEMENT %yada;""",
			"""%yada;""",
			"""<!%yada;>""",
		],
	),
	"elementdecl_pe": (
		[# should match
			""" %name.para; %content.para;""",
		],
		[# should not match
		],
	),
		
	"contentspec": (
		[# should match
			'''EMPTY''',
			'''ANY''',
			'''%content.para;''',
		],
		[# should not match
		],
	),
		
	"AttlistDecl": (
		[# should match
			'''<!ATTLIST termdef
          id      ID      #REQUIRED
          name    CDATA   #IMPLIED>''',
			"""<!ATTLIST list
          type    (bullets|ordered|glossary)  "ordered">""",
			"""<!ATTLIST form
          method  CDATA   #FIXED "POST">""",
		],
		[# should not match
		],
	),
	"AttDef": (
		[# should match
			''' id      ID      #REQUIRED''',
			""" name    CDATA   #IMPLIED""",
			''' type    (bullets|ordered|glossary)  "ordered"''',
			''' method  CDATA   #FIXED "POST"''',
		],
		[# should not match
		],
	),
		
	"EntityDecl": (
		[
			"""<!ENTITY Pub-Status "This is a pre-release of the specification.">""",
			"""<!ENTITY open-hatch
         SYSTEM "http://www.textuality.com/boilerplate/OpenHatch.xml">""",
			"""<!ENTITY open-hatch
         PUBLIC "-//Textuality//TEXT Standard open-hatch boilerplate//EN"
         "http://www.textuality.com/boilerplate/OpenHatch.xml">""",
			"""<!ENTITY hatch-pic
         SYSTEM "../grafix/OpenHatch.gif"
         NDATA gif >""",
		],
		[# should not match
		],
	),
	"EntityDef":(
		[
			'''PUBLIC "-//Textuality//TEXT Standard open-hatch boilerplate//EN"
         "http://www.textuality.com/boilerplate/OpenHatch.xml"''',
		],
		[# should not match
		],
	),
	"PubidLiteral":(
		[
			'''"-//Textuality//TEXT Standard open-hatch boilerplate//EN"''',
		],
		[# should not match
		],
	),
}
for production, (should,shouldnot) in testData.items():
	setattr( XMLProductionTests, 'test'+production, ProductionTest(production, should, shouldnot))

if __name__ == "__main__":
	unittest.main(defaultTest="getSuite")
