##############################################################################
# Copyright (c) 2007 Open Kernel Labs, Inc. (Copyright Holder).
# All rights reserved.
# 
# 1. Redistribution and use of OKL4 (Software) in source and binary
# forms, with or without modification, are permitted provided that the
# following conditions are met:
# 
#     (a) Redistributions of source code must retain this clause 1
#         (including paragraphs (a), (b) and (c)), clause 2 and clause 3
#         (Licence Terms) and the above copyright notice.
# 
#     (b) Redistributions in binary form must reproduce the above
#         copyright notice and the Licence Terms in the documentation and/or
#         other materials provided with the distribution.
# 
#     (c) Redistributions in any form must be accompanied by information on
#         how to obtain complete source code for:
#        (i) the Software; and
#        (ii) all accompanying software that uses (or is intended to
#        use) the Software whether directly or indirectly.  Such source
#        code must:
#        (iii) either be included in the distribution or be available
#        for no more than the cost of distribution plus a nominal fee;
#        and
#        (iv) be licensed by each relevant holder of copyright under
#        either the Licence Terms (with an appropriate copyright notice)
#        or the terms of a licence which is approved by the Open Source
#        Initative.  For an executable file, "complete source code"
#        means the source code for all modules it contains and includes
#        associated build and other files reasonably required to produce
#        the executable.
# 
# 2. THIS SOFTWARE IS PROVIDED ``AS IS'' AND, TO THE EXTENT PERMITTED BY
# LAW, ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
# PURPOSE, OR NON-INFRINGEMENT, ARE DISCLAIMED.  WHERE ANY WARRANTY IS
# IMPLIED AND IS PREVENTED BY LAW FROM BEING DISCLAIMED THEN TO THE
# EXTENT PERMISSIBLE BY LAW: (A) THE WARRANTY IS READ DOWN IN FAVOUR OF
# THE COPYRIGHT HOLDER (AND, IN THE CASE OF A PARTICIPANT, THAT
# PARTICIPANT) AND (B) ANY LIMITATIONS PERMITTED BY LAW (INCLUDING AS TO
# THE EXTENT OF THE WARRANTY AND THE REMEDIES AVAILABLE IN THE EVENT OF
# BREACH) ARE DEEMED PART OF THIS LICENCE IN A FORM MOST FAVOURABLE TO
# THE COPYRIGHT HOLDER (AND, IN THE CASE OF A PARTICIPANT, THAT
# PARTICIPANT). IN THE LICENCE TERMS, "PARTICIPANT" INCLUDES EVERY
# PERSON WHO HAS CONTRIBUTED TO THE SOFTWARE OR WHO HAS BEEN INVOLVED IN
# THE DISTRIBUTION OR DISSEMINATION OF THE SOFTWARE.
# 
# 3. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR ANY OTHER PARTICIPANT BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
# BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
# OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
# IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

"""
ezxml provides support to parse_spec_xml. It extends the basic XML
parsing facility in python to something that is simpler to specify and
abstract away the DOM API from the user
"""
from xml.dom.minidom import parse, parseString
from xml.parsers.expat import ExpatError
from weaver.extensions import Customisable
import types, os


XMLNS_NAMESPACE = "xmlns"

class ParsedElement(object):
    """This represents an element once it has been parsed."""
    def __init__(self, tagname):
        self.tag = tagname
        self.children = []
        self._path = None

    def __repr__(self):
        """Return marked up parsed element"""
        base = ["<%s " % self.tag]

        for attr in [key for key in self.__dict__ if key
                     not in ["children", "tag", "_path"]]:
            val = getattr(self, attr)

            if type(val) == long:
                base.append('%s="0x%x" ' % (attr, val))
            else:
                base.append('%s="%s" ' % (attr, val))

        base.append(">")

        return ''.join(base)

    def dump_internal(self, indent):
        """
        Internal helper function for dump().  Returns a XML
        representation of the element and its children.  The current
        element is indented by <indent> characters and children are
        intended by <indent> + 4 characters.
        """
        xml_indent = 4 # Number of chars to indent children.

        text = [' ' * indent + self.__repr__() + '\n']

        for child in self.children:
            text.append(child.dump_internal(indent + xml_indent))

        text.append(' ' * indent + '</%s>\n' % self.tag)

        return ''.join(text)

    def dump(self):
        """Return a XML representation of the element and its children."""
        return self.dump_internal(0)

    def find_children(self, tagname):
        """Return all children of the element with the given tagname."""
        return [el for el in self.children if el.tag == tagname]

    def find_child(self, tagname):
        """Return a child of the element with the given tagname."""
        els = self.find_children(tagname)
        if len(els) == 0:
            return None
        else:
            return els[0]

    def find_all_children(self, tagname):
        """
        Return all children of the element, or its children, that
        have the given tagname.
        """
        children = []

        for child in self.children:
            if child.tag == tagname:
                children.append(child)

            children.extend(child.find_all_children(tagname))

        return children

class EzXMLError(Exception):
    """This exception is raised if an error occurs during traversing
    a given dom."""

def process_includes(dom):
    """
    This method walks the DOM and replaces any 'include' elements
    with the parsed contents of the file to which it refers.

    The files included must contain XML fragments, not full XML
    documents.
    """
    def do_incs(elem):
        """
        If the current element is an include then return a new
        include element whose children are the contents of the
        file.

        Otherwise, recursively process the children of this
        element.  Afterwards, is the child is an include element
        replace it with its own children.
        """
        def add_path(elem, path):
            """Propagate path element values to child nodes."""
            elem.path = path

            for child in elem.childNodes:
                if child.nodeType == child.ELEMENT_NODE:
                    add_path(child, path)

        # The file attribute vanishes when the contents are included,
        # so its presence must be checked for.
        if elem.tagName == "include" and elem.hasAttribute('file'):
            include_path = os.path.dirname(str(elem.getAttribute('file')))
            include_file = file(str(elem.getAttribute('file')))
            # The parser requires a root element, so wrap the file
            # contents in another include element.  This will be
            # stripped out below.
            new_dom = parseString('<include>' + include_file.read() + \
                                  '</include>')
            include_file.close()
            add_path(new_dom, include_path)
            return new_dom.documentElement

        # Examine children
        for child in elem.childNodes:
            if child.nodeType == child.ELEMENT_NODE:
                new_child = do_incs(child)

                # Replace the include element
                if new_child.tagName == "include":
                    elem.removeChild(child)

                    for kid in new_child.childNodes:
                        if kid.nodeType == child.ELEMENT_NODE:
                            elem.appendChild(kid)

        return elem

    # Do the work recursively from the top of the tree.
    try:
        do_incs(dom.documentElement)
    except EnvironmentError, ex:
        raise EzXMLError, 'XML include error: "%s": %s.' % \
              (ex.filename, ex.strerror)

class Element(object):
    """Defines a type of element, rather than a particular element
    in an XML file. E.g: Can be used to define what attributes and
    child a given node has, rather than representing a particular node
    with specific attributes and elements."""

    def __init__(self, __name, *children, **attributes):
        """Construct an Element with a given tagname 'name'. Attributes
        is a dictionary of valid attributes. Children is a last of
        child elements."""
        self.name = __name
        self.attributes = attributes

        for attr in self.attributes.values():
            if type(attr) != types.TupleType:
                raise EzXMLError("Attribute descriptors must be tuples")
            if len(attr) != 2:
                raise EzXMLError("Attribute descriptors must " + \
                                 "be 2-element tuples")
            if not callable(attr[0]):
                raise EzXMLError("First element in descriptor " + \
                                 "must be callable")
            if attr[1] not in ["required", "optional"]:
                raise EzXMLError("Must be 'required' or 'optional'")


        self.elements = dict([(el.name, el) for el in children])
        self.extras = None
        self.extra_func = None

    def set_name(self, name):
        """Set the name of the element."""
        self.name = name

    def append_element(self, el):
        """Add subelement descriptions to the element."""
        self.elements[el.name] = el

    def parse_xml_str(self, data):
        """Parse a given XML string. Note this should only be used on the
        top level Element. Not child elements."""
        try:
            dom = parseString(data)
            process_includes(dom)
        except ExpatError, text:
            raise EzXMLError("Failed to parse: %s" % text)
        return self.walkdom(dom.documentElement)

    def parse_xml_file(self, filename):
        """Parse a given XML file. Note this should only be used on the
        top level Element. Not child elements."""
        try:
            dom = parse(filename)
            process_includes(dom)
        except ExpatError, text:
            raise EzXMLError("Failed to parse: %s" % text)
        return self.walkdom(dom.documentElement)

    def walkdom(self, el):
        """This methods walks the DOM and extracts attributes and children
        as it goes. It returns <something>, that provides a high level, pythonic
        representation of the XML file."""
        #pylint: disable-msg=R0912
        ret = ParsedElement(self.name)

        if el.localName != self.name:
            raise EzXMLError("Names don't match: %s %s" % \
                             (el.localName, self.name))

        # check that attributes are the expected. Allowable attributes
        # are specified on instantiation of the Element Object
        if el.attributes:
            for attr in el.attributes.values():
                if attr.namespaceURI is not None and \
                       attr.namespaceURI[:-1].endswith(XMLNS_NAMESPACE) and \
                       el.tagName == "image":
                    # OK, new namespace being defined, so we need to
                    # do some kind of el.blah thing...  We need to get
                    # all the registered custom guys and if we find
                    # one that matches call it's customise method.
                    if attr.localName == "xmlns":
                        assert attr.value == "okl4:elfweaver"
                    elif attr.value in Customisable.CUSTOM_CLASSES:
                        Customisable.CUSTOM_CLASSES[attr.value]().customise()
                    else:
                        raise EzXMLError("Unknown custom namespace: %s" %
                                         attr.value)
                elif attr.localName not in self.attributes:
                    raise EzXMLError('Unknown attribute in "%s" element: "%s". '
                                     ':%s:%s:' %
                                     (el.tagName, attr.localName,
                                      attr.namespaceURI,
                                      XMLNS_NAMESPACE))
                # and set the attributes value as seen
                else:
                    setattr(ret, attr.localName,
                            self.attributes[attr.localName][0](attr.value))

        # Ensure required attributes are present
        for attr_name, descriptor in self.attributes.items():
            if descriptor[1] == "required" and not hasattr(ret, attr_name):
                raise EzXMLError, \
                      'The required attribute "%s" is not specified in ' \
                      'element "%s".' % \
                      (attr_name, el.tagName)

        # Examine children
        for child in el.childNodes:
            if child.nodeType == child.ELEMENT_NODE:
                # ensure child is of an expected type
                if child.tagName == "weavertags":
                    execfile("tools/pyelf/weaver.py")
                    continue

                # parse the child and add an item to the children
                # attribute of the parsed element containing the parsed
                # contents
                #pylint: disable-msg=C0301
                if child.localName in self.elements.keys():
                    ret.children.append(self.elements[child.localName].walkdom(child))
                elif self.extras and child.localName in self.extras.keys():
                    ret.children.append(self.extras[child.localName].walkdom(child))
                else:
                    raise EzXMLError, \
                              "Unknown child node: %s of %s" % \
                              (child.localName, self)

            elif child.nodeType == child.TEXT_NODE:
                # Ignore text nodes for now
                pass
            elif hasattr(child, "COMMENT_NODE") and \
                     child.nodeType == child.COMMENT_NODE:
                # ignore comments
                pass
            else:
                raise EzXMLError("Unhandled node type: %d\n" % child.nodeType)

        ret._path = getattr(el, "path", "")

        return ret

    def __call__(self, *children, **attributes):
        new = ParsedElement(self.name)

        for attr in attributes:
            setattr(new, attr, attributes[attr])

        for child in children:
            new.children.append(child)

        return new

def update_element(el, *children, **attributes):
    """
    Create a copy of el, adding extra children and attributes.
    """
    name = el.name

    all_children = el.elements.values() + list(children)

    all_attributes = el.attributes.copy()
    all_attributes.update(attributes)

    #pylint: disable-msg=W0142
    new_el = Element(name, *all_children, **all_attributes)
    new_el.extras = el.extras
    new_el.extra_func = el.extra_func
    return new_el

def str_attr(attr):
    """Parses a string attribute."""
    return str(attr)

def long_attr(attr):
    """Parse a long attribute"""
    try:
        val = long(attr, 0)
    except ValueError:
        raise EzXMLError("%s did not parse as an integer" % attr)
    return val

def bool_attr(attr):
    """Parse a boolean attribute"""
    if attr.lower() == "true":
        val = True
    elif attr.lower() == "false":
        val = False
    else:
        raise EzXMLError("Must be "\
                         "'true' or 'false'. Not %s" % (attr))
    return val

def size_attr(attr):
    """
    Parse a size attribute.  These attributes are longs in either
    base 10 or base 16.  If there is a suffix of 'K' then the value is
    in kilobytes and if there is a suffix of 'M' then the value is in
    megabytes.
    """
    suffix = attr[-1]
    multiple = 1

    # Detect and strip off the suffix.
    if suffix == 'K' or suffix == 'k':
        multiple = 1024L
        attr_num = attr[:-1]
    elif suffix == 'M' or suffix == 'm':
        multiple = 1024L * 1024L
        attr_num = attr[:-1]
    elif suffix == 'G' or suffix == 'g':
        multiple = 1024L * 1024L * 1024L
        attr_num = attr[:-1]
    else:
        attr_num = attr

    try:
        val = long(attr_num, 0) * multiple
    except ValueError:
        raise EzXMLError('"%s" did not parse as a size value.' % attr)
    return val
