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




class PyIndFooProvider(pywbem.CIMProvider):
    """Instrument the CIM class PyIndFoo 

    Test class for a python life cycle indication provider
    
    """

    def __init__ (self, env):
        logger = env.get_logger()
        logger.log_debug('Initializing provider %s from %s' \
                % (self.__class__.__name__, __file__))
        # If you will be filtering instances yourself according to 
        # property_list, role, result_role, and result_class_name 
        # parameters, set self.filter_results to False
        # self.filter_results = False

    def get_instance(self, env, model, cim_class):
        """Return an instance.

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        model -- A template of the pywbem.CIMInstance to be returned.  The 
            key properties are set on this instance to correspond to the 
            instanceName that was requested.  The properties of the model
            are already filtered according to the PropertyList from the 
            request.  Only properties present in the model need to be
            given values.  If you prefer, you can set all of the 
            values, and the instance will be filtered for you. 
        cim_class -- The pywbem.CIMClass

        Possible Errors:
        CIM_ERR_ACCESS_DENIED
        CIM_ERR_INVALID_PARAMETER (including missing, duplicate, unrecognized 
            or otherwise incorrect parameters)
        CIM_ERR_NOT_FOUND (the CIM Class does exist, but the requested CIM 
            Instance does not exist in the specified namespace)
        CIM_ERR_FAILED (some other unspecified error occurred)

        """
        
        logger = env.get_logger()
        logger.log_debug('Entering %s.get_instance()' \
                % self.__class__.__name__)
        
        try:
            model['TheValue'] = _PyFooInsts[model['TheKey']]
        except KeyError:
            raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND)
        return model

    def enum_instances(self, env, model, cim_class, keys_only):
        """Enumerate instances.

        The WBEM operations EnumerateInstances and EnumerateInstanceNames
        are both mapped to this method. 
        This method is a python generator

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        model -- A template of the pywbem.CIMInstances to be generated.  
            The properties of the model are already filtered according to 
            the PropertyList from the request.  Only properties present in 
            the model need to be given values.  If you prefer, you can 
            always set all of the values, and the instance will be filtered 
            for you. 
        cim_class -- The pywbem.CIMClass
        keys_only -- A boolean.  True if only the key properties should be
            set on the generated instances.

        Possible Errors:
        CIM_ERR_FAILED (some other unspecified error occurred)

        """

        logger = env.get_logger()
        logger.log_debug('Entering %s.enum_instances()' \
                % self.__class__.__name__)

        for k,v in _PyFooInsts.items():
            model['TheKey'] = k
            model['TheValue'] = v
            yield model

    def set_instance(self, env, instance, previous_instance, cim_class):
        """Return a newly created or modified instance.

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        instance -- The new pywbem.CIMInstance.  If modifying an existing 
            instance, the properties on this instance have been filtered by 
            the PropertyList from the request.
        previous_instance -- The previous pywbem.CIMInstance if modifying 
            an existing instance.  None if creating a new instance. 
        cim_class -- The pywbem.CIMClass

        Return the new instance.  The keys must be set on the new instance. 

        Possible Errors:
        CIM_ERR_ACCESS_DENIED
        CIM_ERR_NOT_SUPPORTED
        CIM_ERR_INVALID_PARAMETER (including missing, duplicate, unrecognized 
            or otherwise incorrect parameters)
        CIM_ERR_ALREADY_EXISTS (the CIM Instance already exists -- only 
            valid if previous_instance is None, indicating that the operation
            was CreateInstance)
        CIM_ERR_NOT_FOUND (the CIM Instance does not exist -- only valid 
            if previous_instance is not None, indicating that the operation
            was ModifyInstance)
        CIM_ERR_FAILED (some other unspecified error occurred)

        """

        logger = env.get_logger()
        logger.log_debug('Entering %s.set_instance()' \
                % self.__class__.__name__)

        if previous_instance is None:
            _PyFooInsts[model['TheKey']] = instance['TheValue']
        else:
            if 'TheValue' in instance:
                try:
                    _PyFooInsts[instance['TheKey']] = instance['TheValue']
                except KeyError:
                    raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND)
                ci = pywbem.CIMInstance('CIM_InstModification')
                ci['PreviousInstance'] = previous_instance
                ci['SourceInstance'] = instance
                ci['SourceInstanceModelPath'] = instance.path
                ci['IndicationIdentifier'] = 'PyTestInd:%s' % 'one'
                ci['IndicationTime'] = pywbem.CIMDateTime.now()
                ci['PerceivedSeverity'] = pywbem.Uint16(2)
                ch = env.get_cimom_handle()
                ch.export_indication(ci, 'root/cimv2')
        return instance

    def delete_instance(self, env, instance_name):
        """Delete an instance.

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        instance_name -- A pywbem.CIMInstanceName specifying the instance 
            to delete.

        Possible Errors:
        CIM_ERR_ACCESS_DENIED
        CIM_ERR_NOT_SUPPORTED
        CIM_ERR_INVALID_NAMESPACE
        CIM_ERR_INVALID_PARAMETER (including missing, duplicate, unrecognized 
            or otherwise incorrect parameters)
        CIM_ERR_INVALID_CLASS (the CIM Class does not exist in the specified 
            namespace)
        CIM_ERR_NOT_FOUND (the CIM Class does exist, but the requested CIM 
            Instance does not exist in the specified namespace)
        CIM_ERR_FAILED (some other unspecified error occurred)

        """ 

        logger = env.get_logger()
        logger.log_debug('Entering %s.delete_instance()' \
                % self.__class__.__name__)

        try:
            del _PyFooInsts[instance_name['TheKey']]
        except KeyError:
            raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND)
        
## end of class PyIndFooProvider

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


def get_providers(env): 
    pyindfoo_prov = PyIndFooProvider(env)  
    return {'PyIndFoo': pyindfoo_prov} 

