"""XML Parser based (loosely) on the XML Spec's EBNF

This is a hand-coded parser based on the W3C's XML specification,
there was a lot of busy-work rewriting to make the syntax agree,
but also a number of signficant structural changes required by
the limitations of the SimpleParse engine, and the completely
procedural definition of References in the XML spec (the References
don't occur in most places they can occur, and they are seen as
altering the buffer directly as soon as they are encountered, this
isn't something that fits readily into the mx.TextTools engine.

http://www.w3.org/TR/REC-xml#sec-references

Major Deviations from Spec:
	No support for the unicode-style character classes
	No support for UTF-16 (or Unicode at all, for that matter)
	No support for References that alter the production
		being parsed, so you can't have a Reference to an
		item "</this>and<this>" or similar non-structure-
		respecting References.  References have
		particular locations they can occur, and they are
		just ignored elsewhere
	No support for parsing the contents of References within
		the primary parsing pass
	No support for excluded start/end tags
	Comments allowed in both tags and declarations (but not
		inside content-specifiers).
	Allows end tags of the form </>
"""

declaration = """

# Simple (changable) literals
# These should be chosen based on the encoding
# of the file, which is actually embedded in the
# file :(

<S>          := [\x20\x09\x0D\x0A]+
<letter>     := [a-zA-Z]
<namestart>  := letter/[_:]
<namechar>   := letter/digit/[-._:]


# don't change for XML, but would change for SGML or HTML
<Eq>         := '='
<REFO>       := '&'
<PREFO>      := '%'
<REFC>       := ';'
<PIO>        := '<?'
<PIC>        := '?>'
<STagO>      := '<'
<STagC>      := '>'
<ETagO>      := '</'
<ETagC>      := '>'
<EmptyElemTagC> := '/>'


# an XML-comment, note that this follows
# SGML semantics, so that you can embed comment_sets
# in the middle of the various declarations...
>Comment<     := "<!", comment_set,(S?,comment_set)*,S?,">"
>comment_set<   := '--', xml_comment,'--'
xml_comment         := -'--'*

# whitespace in tag (including possible comment)
>TS<             := (Comment/S)+


# general structures
AttValue       :=    ('"', (Reference/ -[&"] )*, '"') / (  "'", (Reference / -[&'])*, "'")

# Names
Name                := namestart, namechar*
Names               := Name, (S,Name)*
Nmtoken             := namechar+
Nmtokens            := Nmtoken, (S,Nmtoken)*

# processing instructions
PI          := PIO, PITarget, S?, PIContent, PIC
PIContent   := -PIC*
PITarget    :=   ?-( [Xx],[Mm],[Ll]), Name


## references
	# character reference
	CharRef              := REFO,'#',('x',hex)/(int),REFC
	# entity reference
	EntityRef            := REFO, Name, REFC
	# parsed entity ref
	PEReference          := PREFO, Name, REFC

Reference    :=    EntityRef / CharRef

Misc := Comment/S

### PROLOG definitions...

	prolog         :=    XMLDecl?, Misc*, (doctypedecl, Misc*)?
	XMLDecl        :=    '<?xml', VersionInfo, EncodingDecl?, SDDecl?, TS?, '?>'
	VersionInfo    :=    TS?, 'version', TS?, Eq, TS?, (('"',VersionNum,'"')/("'",VersionNum,"'"))
	VersionNum     :=    [a-zA-Z0-9_.:-]+


### Document-type declarations (DTDs)

	doctypedecl    :=    '<!DOCTYPE', TS, Name, (TS, ExternalID)?, TS?,('[', (markupdecl / DeclSep)*, ']', TS?)?, '>'

	DeclSep        :=    PEReference / S
	markupdecl     :=    elementdecl / AttlistDecl / EntityDecl / NotationDecl / PI / Comment

	EncodingDecl   :=    TS, 'encoding', Eq, (('"', EncName, '"') / ("'", EncName, "'") )
	EncName        :=    [A-Za-z],[A-Za-z0-9._-]*
	SDDecl         :=    TS, 'standalone', Eq, (("'", ('yes' / 'no'), "'") / ('"', ('yes' / 'no'), '"'))

	ExternalID     :=    ('SYSTEM', TS?, SystemLiteral) / ('PUBLIC', TS?, PubidLiteral, TS?, SystemLiteral ) / PEReference
	NDataDecl      :=    (TS, 'NDATA', TS, Name)/ (TS,PEReference,TS,(Name/ PEReference)?)

	SystemLiteral  :=    ('"', -["]*, '"') / ("'", -[']*, "'") / PEReference
	PubidLiteral   :=    ('"', [\x20\x0D\x0Aa-zA-Z0-9'()+,./:=?;!*#@$_%-]*, '"') / ("'", [\x20\x0D\x0Aa-zA-Z0-9()+,./:=?;!*#@$_%-]*, "'") / PEReference

	PublicID       :=    ('PUBLIC', TS, PubidLiteral) / PEReference


### Element-type declarations
	# hack to try and get PEReference parsing for the "normal case"
	# where the PEReference doesn't change the production level, which
	# seems to be suggested by the spec...
	
	elementdecl    :=    '<!ELEMENT', (
		(TS, Name, TS, contentspec)/
		elementdecl_pe
	), TS?,'>'
	
	>elementdecl_pe< := (TS, PEReference, TS?, contentspec?)
	
	contentspec    :=    'EMPTY' / 'ANY' / Mixed / children
	Mixed          :=    ('(', S?, '#PCDATA', (S?, '|', S?, (Name/PEReference))*, S?, ')*' ) /('(', S?, '#PCDATA', S?, ')')

	repetition_specifier := ('?' / '*' / '+')?
	children       :=    (choice / seq/ PEReference), repetition_specifier
	cp             :=    (choice / seq / Name/ PEReference ), repetition_specifier
	choice         :=    '(', S?, cp, ( S?, '|', S?, cp )+, S?, ')'
	seq            :=    '(', S?, cp, ( S?, ',', S?, cp )*, S?, ')'


### Attribute list declarations...
	AttlistDecl    :=    '<!ATTLIST', TS, ((Name, AttDef*, TS?)/(PEReference, AttDef*, TS?)), '>'
	AttDef         :=    TS, ((Name, TS, AttType, TS, DefaultDecl)/(PEReference, TS?, AttType?, TS?, DefaultDecl?))


	AttType        :=    StringType / TokenizedType / EnumeratedType/ PEReference
	StringType     :=    'CDATA'
	TokenizedType  :=    'ID' / 'IDREF' / 'IDREFS' / 'ENTITY' / 'ENTITIES' / 'NMTOKEN' / 'NMTOKENS'
	EnumeratedType :=    NotationType / Enumeration
	NotationType   :=    'NOTATION', TS, ('(', NameOrList, ')')/PEReference
	Enumeration    :=    '(', (NmTokenOrList/PEReference), ')'
	
	>NameOrList<    :=    S?, (Name/PEReference), (S?, '|', S?, (Name/PEReference))*, S?
	>NmTokenOrList< :=    S?, (Nmtoken/PEReference), (S?, '|', S?, (Nmtoken/PEReference))*, S?


	DefaultDecl    :=    '#REQUIRED' / '#IMPLIED' / ((('#FIXED', TS)/PEReference)?, (AttValue/PEReference)) / PEReference

### Entity declarations
	EntityDecl    :=    GEDecl / PEDecl
	GEDecl        :=    '<!ENTITY', TS, ((Name, TS, EntityDef)/(PEReference,TS?,EntityDef?)), TS?, '>'
	PEDecl        :=    '<!ENTITY', TS, '%', TS, ((Name, TS, PEDef)/(PEReference,TS?,PEDef?)), TS?, '>'
	EntityDef     :=    EntityValue / (ExternalID, NDataDecl?) / PEReference
	PEDef         :=    EntityValue / ExternalID / PEReference
	EntityValue   :=    ('"', (PEReference / Reference / -[%&"])*, '"') /  ("'", (PEReference / Reference / -[%&'])*, "'")

NotationDecl      :=    '<!NOTATION', TS, Name, TS, (ExternalID / PublicID), TS?, '>'

### elements (nodes/tags/you-know :) )
	# limitations in the SimpleParse engine mean that this
	# particular structure will be basically useless...
	element    :=    EmptyElemTag / (STag, content, ETag)

	EmptyElemTag    :=    STagO, Name, (TS, Attribute)*, TS?, EmptyElemTagC
	
	STag       :=    STagO, Name, (TS, Attribute)*, TS?, STagC
	ETag       :=    ETagO, Name?, TS?, ETagC

	content    :=    (element / Reference / CDSect / PI / Comment / CharData)*

	Attribute  :=    (Name, Eq, (AttValue/Reference))/(Reference,(Eq,(AttValue/Reference))?)

	# general content of an element
	CharData   :=    ( -[<&]+ / -(STag / EmptyElemTag / ETag / Reference / CDSect / PI / Comment) )+

	# special non-parsed character data sections
	CDSect     :=    CDStart, CData, CDEnd
	<CDStart>  :=    '<![CDATA['
	CData      :=    -CDEnd*
	<CDEnd>    :=    ']]>'


document       :=    prolog, element, Misc*
"""
from simpleparse.common import numbers, strings, chartypes
