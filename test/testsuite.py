#!/usr/bin/python

import pywbem

def fix_insts (o):
    if isinstance(o,list):
        [fix_insts(inst) for inst in o]
    else:
        o.path.namespace = 'fudged'
        #for p in o.properties.values():
        #    p.propagated = None

conn = pywbem.WBEMConnection('https://localhost:30927', ('test1', 'pass1'))

dyns = conn.EnumerateInstances('Py_LotsOfDataTypes',LocalOnly=False,
                              IncludeQualifiers=False,IncludeClassOrigin=False)

conn.default_namespace = 'root/py_static'
stats = conn.EnumerateInstances('Py_LotsOfDataTypes',LocalOnly=False,
                                IncludeQualifiers=False,IncludeClassOrigin=False)

fix_insts(dyns)
fix_insts(stats)
dyns.sort()
stats.sort()
if dyns != stats:
    print 'Failed!'
    print 'dyns: ',dyns
    print 'stats: ',stats


iname = pywbem.CIMInstanceName(classname='py_lotsofdatatypes', 
                               namespace='root/cimv2', 
                               keybindings={'Key1':'k1_1', 
                                            'Key2':'k2_1'})

dinst = conn.GetInstance(iname, IncludeQualifiers=False, LocalOnly=False)
iname.namespace = 'root/py_static'
sinst = conn.GetInstance(iname, IncludeQualifiers=False, LocalOnly=False)

fix_insts([dinst, sinst])
if dinst != sinst:
    print 'Failed!'
    print 'dinst', dinst
    print 'sinst', sinst


iname.namespace = 'root/cimv2'
rv, outs = conn.InvokeMethod('MethodTest', iname, s="input_string", 
                             embedded=dinst, embedded_a=stats)

print outs

