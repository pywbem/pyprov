#!/usr/bin/python

import pywbem

def test(inst):
    inst['middle'] = 'W'
    print inst.values()
    print inst.properties.values()
    print [x.value for x in inst.properties.values()]
    print inst.tocimxml().toprettyxml()
    #print [x for x in inst.properties.values()]
    return inst


