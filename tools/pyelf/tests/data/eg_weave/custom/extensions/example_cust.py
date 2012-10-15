##############################################################################
# @LICENCE("Open Kernel Labs, Inc.", "2008")@

from weaver.extensions import Customisable
from weaver.ezxml import Element, str_attr, update_element
from weaver.cells.iguana_cell import IguanaCell
from weaver.cells.iguana_cell import Iguana_el
from weaver.xml_api import Machine_el, Machine, Element, str_attr, update_element


Foo_el = Element("foo",
                 value = (str_attr, "optional"))

MyIguana_el = update_element(Iguana_el, Foo_el)
MyMachine_el = update_element(Machine_el, Foo_el)


class ExampleIguanaCell(IguanaCell):

    element = MyIguana_el

    def collect_xml(self, iguana_el, ignore_name, namespace, machine,
                    pools, kernel, image):
        IguanaCell.collect_xml(self, iguana_el, ignore_name, namespace, machine,
                               pools, kernel, image)
        print "FOO =", iguana_el.find_child("foo").value

class ExampleMachine(Machine):

    element = MyMachine_el

    def collect_xml(self, machine_el, ignore_name):
        Machine.collect_xml(self, machine_el, ignore_name)
        print "machine FOO =", machine_el.find_child("foo").value

class ExampleCustomisable(Customisable):
    def customise(self):
        ExampleIguanaCell.register()
        ExampleMachine.register()

ExampleCustomisable.register("okl4:example")
