#!/usr/bin/env python

#import gmetaddata as gmetad
#import gmonddata as gmond
import pywbem
from lib import wbem_connection
#import telnetlib as telnet
#import elementtree.ElementTree as ET
#import socket
import unittest
import os
import shutil


#This test requires the usage of elementtree


################################################################################
class TestGmetadData(unittest.TestCase):

    def setUp(self):
        unittest.TestCase.setUp(self)
        wconn = wbem_connection.wbem_connection()
        self.conn = wconn._WBEMConnFromOptions()
        for iname in self.conn.EnumerateInstanceNames('TestMethod'):
            self.conn.DeleteInstance(iname)

    def tearDown(self):
        unittest.TestCase.tearDown(self)
        for iname in self.conn.EnumerateInstanceNames('TestMethod'):
            self.conn.DeleteInstance(iname)

    def test_refs(self):
        inst = pywbem.CIMInstance('TestMethod', properties={
                'id':'one', 
                'p_str':'One',
                'p_sint32':pywbem.Sint32(1)})
        self.conn.CreateInstance(inst)

        iname = pywbem.CIMInstanceName('TestMethod', namespace='root/cimv2',
                keybindings={'id':'one'})
        rv, outs = self.conn.InvokeMethod('getStrProp', iname)
        self.assertEquals(rv, 'One')

        self.conn.InvokeMethod('setStrProp', iname, value='won')
        rv, outs = self.conn.InvokeMethod('getStrProp', iname)
        self.assertEquals(rv, 'won')
        inst = self.conn.GetInstance(iname)
        self.assertEquals(inst['p_str'], 'won')

        rv, outs = self.conn.InvokeMethod('getIntProp', iname)
        self.assertEquals(rv, 1)
        self.assertTrue(isinstance(rv, pywbem.Sint32))
        self.assertEquals(inst['p_sint32'], 1)
        self.conn.InvokeMethod('setIntProp', iname, value=pywbem.Sint32(2))
        rv, outs = self.conn.InvokeMethod('getIntProp', iname)
        self.assertEquals(rv, 2)
        self.assertTrue(isinstance(rv, pywbem.Sint32))
        inst = self.conn.GetInstance(iname)
        self.assertEquals(inst['p_sint32'], 2)

        rv, outs = self.conn.InvokeMethod('getObjectPath', 'TestMethod')
        self.assertTrue(isinstance(outs['path'], pywbem.CIMInstanceName))
        self.assertEquals(outs['path']['id'], 'one')

        inst = pywbem.CIMInstance('TestMethod', properties={
                'id':'two', 
                'p_str':'Two',
                'p_sint32':pywbem.Sint32(2)})
        self.conn.CreateInstance(inst)

        rv, outs = self.conn.InvokeMethod('getObjectPaths', 'TestMethod')
        self.assertEquals(len(outs['paths']), 2)
        self.assertTrue(isinstance(outs['paths'][0], pywbem.CIMInstanceName))
        to_delete = outs['paths']

        inst = pywbem.CIMInstance('TestMethod', properties={
                'id':'three', 
                'p_str':'Three',
                'p_sint32':pywbem.Sint32(3)})
        self.conn.CreateInstance(inst)

        iname = pywbem.CIMInstanceName('TestMethod', namespace='root/cimv2', 
                keybindings={'id':'three'})

        inames = self.conn.EnumerateInstanceNames('TestMethod')
        self.assertEquals(len(inames), 3)
        rv, outs = self.conn.InvokeMethod('delObject', 'TestMethod', 
                path=iname)

        inames = self.conn.EnumerateInstanceNames('TestMethod')
        self.assertEquals(len(inames), 2)

        self.conn.CreateInstance(inst)

        '''  # OpenWBEM is broken!  uncomment this for Pegasus. 
        rv, outs = self.conn.InvokeMethod('delObjects', 'TestMethod', 
                paths=to_delete)
        
        inames = self.conn.EnumerateInstanceNames('TestMethod')
        self.assertEquals(len(inames), 1)
        self.assertEquals(inames[0]['id'], 'three')
        '''

    def test_mkUniStr_sint8(self):
        s = 'Scrum Rocks!'
        l = [pywbem.Sint8(ord(x)) for x in s]
        rv, outs = self.conn.InvokeMethod('mkUniStr_sint8', 'TestMethod', 
                cArr=l)
        self.assertEquals(rv, s)

    def test_strCat(self):
        ra = ['one','two','three','four']
        rv, outs = self.conn.InvokeMethod('strCat', 'TestMethod', 
                strs=ra, sep=',')
        self.assertEquals(rv, ','.join(ra))
        self.assertFalse(outs)


    def test_getDate(self):
        dt = pywbem.CIMDateTime.now()
        rv, outs = self.conn.InvokeMethod('getDate', 'TestMethod', 
                datestr=str(dt))
        self.assertFalse(outs)
        self.assertEquals(rv, dt)
        self.assertEquals(str(rv), str(dt))
        self.assertTrue(isinstance(rv, pywbem.CIMDateTime))

    def test_getDates(self):
        dt = pywbem.CIMDateTime.now()
        s1 = str(dt)
        ra = [s1]
        dt = pywbem.CIMDateTime(pywbem.datetime.now() + \
                pywbem.timedelta(seconds=10))
        s2 = str(dt)
        ra.append(s2)
        dt = pywbem.CIMDateTime(pywbem.datetime.now() + \
                pywbem.timedelta(seconds=10))
        s3 = str(dt)
        ra.append(s3)

        rv, outs = self.conn.InvokeMethod('getDates', 'TestMethod', 
                datestrs=ra)
        self.assertTrue(rv)
        self.assertTrue(isinstance(rv, bool))
        self.assertEquals(outs['nelems'], len(ra))
        self.assertTrue(isinstance(outs['nelems'], pywbem.Sint32))

        for i in range(0, len(ra)):
            self.assertTrue(isinstance(outs['elems'][i], pywbem.CIMDateTime))
            self.assertEquals(str(outs['elems'][i]), ra[i])

    def test_minmedmax(self):
        for tstr in ['Sint8', 'Uint8', 'Sint16', 'Uint16', 'Sint32', 'Uint32',
                  'Sint64', 'Uint64', 'Real32', 'Real64']:
            dt = getattr(pywbem, tstr)
            method = 'minmedmax_%s' % tstr
            numlist = [
                dt(2),
                dt(5),
                dt(8),
                dt(1),
                dt(9),
                dt(6),
                dt(4),
                dt(7),
                dt(3),
                ]
            rv, outs = self.conn.InvokeMethod(method, 'TestMethod', 
                    numlist=numlist)
            self.assertTrue(rv)
            self.assertTrue(isinstance(rv, bool))
            self.assertTrue(isinstance(outs['min'], dt))
            self.assertTrue(isinstance(outs['med'], dt))
            self.assertTrue(isinstance(outs['max'], dt))
            self.assertEquals(outs['min'], 1)
            self.assertEquals(outs['max'], 9)
            self.assertEquals(outs['med'], 5)

def get_unit_test():
    return TestGmetadData


if __name__ == '__main__':
    suite = unittest.makeSuite(TestGmetadData)
    unittest.TextTestRunner(verbosity=2).run(suite)

