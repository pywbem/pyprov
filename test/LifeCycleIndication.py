import pywbem, threading, time

_PyFooInsts = {'Key1': pywbem.Sint32(1), 'Key2': pywbem.Sint32(2), 'Key3': pywbem.Sint32(3)}

TheIndicationThread = None

##############################################################################
class IndicationThread(threading.Thread):


    #######################################################
    def __init__(self, env):
        threading.Thread.__init__(self)
        self.count = 0
        self.env = env
        self.shuttingdown = False
        self._cond = threading.Condition()
        log = self.env.get_logger()
        log.log_debug('###### Python life cycle IndicationThread has been created')

    #######################################################
    def shutdown(self):
        log = self.env.get_logger()
        log.log_debug('#### IndicationThread.shutdown called')
        if not self.isAlive():
            return
        self.shuttingdown = True
        self._cond.acquire()
        self._cond.notifyAll()
        self._cond.release()
        self.join()
        log.log_debug('#### IndicationThread.shutdown returning')
        
    #######################################################
    def run(self):
        log = self.env.get_logger()
        log.log_debug('##### IndicationThread.run entered...')
        self.shuttingdown = False
        ch = self.env.get_cimom_handle()
        while not self.shuttingdown:
            # Acquire the lock on the condition variable before we wait on it
            self._cond.acquire()
            # We'll wait on the condition for 5 seconds. Then we'll
            # wake up and generate an indication
            l = self._cond.wait(5.0)
            self._cond.release()
            # If we're shutting down, just break out of this loop
            if self.shuttingdown:
                break

            # Modify 'Key2' of _PyFooInsts and generate a CIM_InstModification
            # life cycle indication

            k = 'Key2'
            v = _PyFooInsts[k];
            cipath = pywbem.CIMInstanceName('PyIndFoo', {'TheKey':k}, namespace='root/cimv2');
            pci = pywbem.CIMInstance('PyIndFoo',
                {'TheKey':k, 'TheValue':pywbem.Sint32(v)}, path=cipath)
            v += 1
            _PyFooInsts[k] = v
            sci = pywbem.CIMInstance('PyIndFoo',
                {'TheKey':k, 'TheValue':pywbem.Sint32(v)}, path=cipath)

            self.count += 1

            ci = pywbem.CIMInstance('CIM_InstModification')
            ci['PreviousInstance'] = pci
            ci['SourceInstance'] = sci
            ci['SourceInstanceModelPath'] = cipath
            ci['IndicationIdentifier'] = 'PyTestInd:%s' % self.count
            ci['IndicationTime'] = pywbem.CIMDateTime.now()
            ci['PerceivedSeverity'] = pywbem.Uint16(2)
            ch.export_indication(ci, 'root/cimv2')

        log.log_debug('##### IndicationThread.run returning')

##############################################################################
def get_initial_polling_interval(env):
    logger = env.get_logger()
    logger.log_debug('#### python life cycle indication get_initial_polling_interval called...')
    # I'm going to tell the CIMOM to call the poll method in
    # 3 seconds. In the poll method I'll set things up for
    # the indication thread...
    #return 3
    return 0

##############################################################################
def poll(env):
    # Should never get called
    logger = env.get_logger()
    logger.log_debug('#### python life cycle indication poll called...')
    # Return 0 so the CIMOM will never call poll again
    return 0

##############################################################################
def shutdown(env):
    logger = env.get_logger()
    print '#### Threaded python shutdown called...'
    global TheIndicationThread
    if TheIndicationThread:
        print '#### Threaded python shutdown is shutting down thread'
        TheIndicationThread.shutdown()
        print '#### Threaded python shutdown. thread has shutdown. returning'
    print '#### Threaded python shutdown returning'


##############################################################################
def MI_enumInstanceNames(env, 
                         ns, 
                         result, 
                         cimClass):
    for k in _PyFooInsts.keys():
        iname = pywbem.CIMInstanceName('PyIndFoo', {'TheKey':k}, namespace=ns)
        result(iname)

##############################################################################
def MI_enumInstances(env, 
                     ns, 
                     result, 
                     localOnly, 
                     deep, 
                     incQuals, 
                     incClassOrigin, 
                     propertyList, 
                     requestedCimClass, 
                     cimClass):
    for k,v in _PyFooInsts.items():
        cipath = pywbem.CIMInstanceName('PyIndFoo', {'TheKey':k}, namespace=ns)
        ci = pywbem.CIMInstance('PyIndFoo',
            {'TheKey':k, 'TheValue':v}, path=cipath)
        result(ci)

##############################################################################
def MI_getInstance(env, 
                   instanceName, 
                   localOnly, 
                   incQuals, 
                   incClassOrigin, 
                   propertyList, 
                   cimClass):
    if instanceName.keybindings.has_key('TheKey'):
        kv = instanceName['TheKey']
        if _PyFooInsts.has_key(kv):
            ci = pywbem.CIMInstance('PyIndFoo',
                {'TheKey':kv, 'TheValue':_PyFooInsts[kv]}, path=instanceName)
            return ci
    raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND)

##############################################################################
def MI_createInstance(env, 
                      instance):
    kv, val = None, None
    if instance.properties.has_key('TheKey'):
        kv = instance['TheKey']
    if not kv:
        raise pywbem.CIMError(pywbem.CIM_ERR_INVALID_PARAMETER,
            "'TheKey' is a required property")

    if instance.properties.has_key('TheValue'):
        val = instance['TheValue']
    if not val:
        raise pywbem.CIMError(pywbem.CIM_ERR_INVALID_PARAMETER,
            "'TheValue' is a required property")

    if _PyFooInsts.has_key(kv):
        raise pywbem.CIMError(pywbem.CIM_ERR_ALREADY_EXISTS)

    _PyFooInsts[kv] = val

    return pywbem.CIMInstanceName('PyIndFoo', {'TheKey':kv},
        namespace=instance.path)

##############################################################################
def MI_modifyInstance(env, 
                      modifiedInstance, 
                      previousInstance, 
                      incQuals, 
                      propertyList, 
                      cimClass):
    if modifiedInstance.properties.has_key('TheKey'):
        kv = modifiedInstance['TheKey']
    if not kv:
        raise pywbem.CIMError(pywbem.CIM_ERR_INVALID_PARAMETER,
            "'TheKey' is a required property")

    if modifiedInstance.properties.has_key('TheValue'):
        val = modifiedInstance['TheValue']
    if not val:
        raise pywbem.CIMError(pywbem.CIM_ERR_INVALID_PARAMETER,
            "'TheValue' is a required property")

    if not _PyFooInsts.has_key(kv):
        raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND)

    _PyFooInsts[kv] = val;

##############################################################################
def MI_deleteInstance(env, 
                      instance_name):
    if instance_name.keybindings.has_key('TheKey'):
        kv = instance_name['TheKey']
        if _PyFooInsts.has_key(kv):
            del(_PyFooInsts[kv])
            return
    raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND)

##############################################################################
def activate_filter(env, filter, eventType, namespace, classes, firstActivation):
    logger = env.get_logger();
    logger.log_debug('#### Python activate_filter called. filter: %s' % filter)
    logger.log_debug('#### Python firstActivation: %d' % firstActivation)
    global TheIndicationThread
    if firstActivation and not TheIndicationThread:
        logger.log_debug('#### first activation. start thread')
        TheIndicationThread = IndicationThread(env)
        logger.log_debug('#### Threaded python created thread object.')
        logger.log_debug('#### Threaded python starting thread object...')
        TheIndicationThread.start()


##############################################################################
def deactivate_filter(env, filter, eventType, namespace, classes, lastActivation):
    logger = env.get_logger();
    logger.log_debug('#### Python deactivate_filter called. filter: %s' % filter)
    logger.log_debug('#### Python lastActivation: %d' % lastActivation)
    global TheIndicationThread
    if lastActivation and TheIndicationThread:
        print '#### deactivate_filter lastActivation. shutdown thread'
        TheIndicationThread.shutdown()
        TheIndicationThread = None
        print '#### Threaded python shutdown. thread has shutdown. returning'

##############################################################################
def authorize_filter(env, filter, eventType, namespace, classes, owner):
    logger = env.get_logger();
    logger.log_debug('#### Python authorize_filter called. filter: %s' % filter)
    logger.log_debug('#### Python authorize_filter owner: %s' % owner)

