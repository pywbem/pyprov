"""Python Provider for Py_LotsOfDataTypes

Instruments the CIM class Py_LotsOfDataTypes
"""

import pywbem
from pycim import CIMProvider

class Py_LotsOfDataTypesProvider(CIMProvider):
    """Instrument the CIM class Py_LotsOfDataTypes 

            This class extends LogicalElement to abstract the concept of an
            element that is enabled and disabled, such as a LogicalDevice or
            a ServiceAccessPoint.
            
    """

    #########################################################################
    def __init__ (self):
        pass

    #########################################################################
    def get_instance (self, env, model, cim_class):
        """Return an instance of Py_LotsOfDataTypes

        Keyword arguments:
        env -- Provider Environment
        model -- A template of the CIMInstance to be returned.  The key 
            properties are set on this instance to correspond to the 
            instanceName that was requested.  The properties of the model
            are already filtered according to the PropertyList from the 
            request.
        cim_class -- The CIMClass Py_LotsOfDataTypes

        """

        ch = env.get_cimom_handle()

        newPath = model.path.copy()
        newPath.namespace = 'root/py_static'
        other_inst = ch.GetInstance(newPath, LocalOnly=False, 
                                    IncludeQualifiers=False,
                                    IncludeClassOrigin=False)
        for pname in model.properties.keys():
            model.properties[pname] = other_inst.properties[pname]
        print '*** model["embedded"]: ', `model['embedded']`
        insts = []
        ch.EnumerateInstances('PyFoo', 'root/cimv2', LocalOnly=False, 
                              Handler=insts.append)
        #model['embedded_a'] = insts
        return model

    #########################################################################
    def enum_instances(self, env, model, cim_class, keys_only):
        """ Enumerate instances of Py_LotsOfDataTypes
        The WBEM operations EnumerateInstances and EnumerateInstanceNames
        are both mapped to this method. 
        This method is a python generator

        Keyword arguments:
        env -- Provider Environment
        model -- A template of the CIMInstances to be generated.  The 
            properties of the model are already filtered according to the 
            PropertyList from the request.
        cim_class -- The CIMClass Py_LotsOfDataTypes
        keys_only -- A boolean.  True if only the key properties should be
            set on the generated instances.

        """

        ch = env.get_cimom_handle()
        logger = env.get_logger()
        logger.log_debug('** Py_LotsOfDataTypes enum_instances called.')

        insts = []
        ch.EnumerateInstances(model.classname, 'root/py_static', 
                         LocalOnly=False, Handler=insts.append,
                         IncludeQualifiers=False)

        print '*** insts', insts

        for inst in insts:
            for pname in model.properties.keys():
                propagated = model.properties[pname].propagated 
                print "************ inst.prop.propagated:", inst.properties[pname].propagated
                model.properties[pname] = inst.properties[pname]
                #model.properties[pname].propagated = propagated
                print "************ model.prop.propagated:", model.properties[pname].propagated
            yield model

    #########################################################################
    def set_instance(self, env, instance, previous_instance, cim_class):
        """ Return a newly created or modified instance of Py_LotsOfDataTypes

        Keyword arguments:
        env -- Provider Environment
        instance -- The new CIMInstance.  If modifying an existing instance,
            the properties on this instance have been filtered by the 
            PropertyList from the request.
        previous_instance -- The previous instance if modifying an existing 
            instance.  None if creating a new instance. 
        cim_class -- The CIMClass Py_LotsOfDataTypes

        Return the new instance.  The keys must be set on the new instance. 

        """

        # TODO create or modify the instance
        return instance

    #########################################################################
    def delete_instance(self, env, instance_name):
        """ Delete an instance of Py_LotsOfDataTypes

        Keyword arguments:
        env -- Provider Environment
        instance_name -- A CIMInstanceName specifying the instance of 
            Py_LotsOfDataTypes to delete.

        """ 
    #########################################################################
    def cim_method_requeststatechange(self, env, object_name, method,
                                  param_requestedstate,
                                  param_timeoutperiod):
        """Implements Py_LotsOfDataTypes.RequestStateChange()

            Requests that the state of the element be changed to the value
            specified in the RequestedState parameter. When the requested
            state change takes place, the EnabledState and RequestedState of
            the element will be the same. Invoking the RequestStateChange
            method multiple times could result in earlier requests being
            overwritten or lost.  If 0 is returned, then the task completed
            successfully and the use of ConcreteJob was not required. If
            4096 (0x1000) is returned, then the task will take some time to
            complete, ConcreteJob will be created, and its reference
            returned in the output parameter Job. Any other return code
            indicates an error condition.
            
        Keyword arguments:
        env -- Provider Environment
        object_name -- A CIMInstanceName or CIMCLassName specifying the 
            object on which the method %(mname)s should be invoked
        method -- A CIMMethod representing the method meta-data
        param_requestedstate --  The input parameter RequestedState (type uint16) 
            The state requested for the element. This information will be
            placed into the RequestedState property of the instance if the
            return code of the RequestStateChange method is 0 ('Completed
            with No Error'), 3 ('Timeout'), or 4096 (0x1000) ('Job
            Started'). Refer to the description of the EnabledState and
            RequestedState properties for the detailed explanations of the
            RequestedState values.
            
        param_timeoutperiod --  The input parameter TimeoutPeriod (type datetime) 
            A timeout period that specifies the maximum amount of time that
            the client expects the transition to the new state to take. The
            interval format must be used to specify the TimeoutPeriod. A
            value of 0 or a null parameter indicates that the client has no
            time requirements for the transition.  If this property does not
            contain 0 or null and the implementation does not support this
            parameter, a return code of 'Use Of Timeout Parameter Not
            Supported' must be returned.
            

        Returns a two-tuple containing the return value (type uint32)
        and a dictionary with the out-parameters

        Output parameters:
        Job -- (type REF CIM_ConcreteJob (CIMInstanceName)) 
            Reference to the job (can be null if the task is completed).
            

        """

        out_params = {}
        out_params['job'] = pywbem.CIMInstanceName(classname='CIM_ConcreteJob',
                                                   namespace='root/cimv2',
                                                   keybindings={
                          'InstanceID': '%d:%s'%(param_requestedstate,str(param_timeoutperiod))})

        rval = pywbem.Uint32(0)
        return (rval, out_params)
        
    def cim_method_methodtest(self, env, object_name, method,
                              param_paths,
                              param_embedded,
                              param_nullparam,
                              param_uint8array,
                              param_s,
                              param_embedded_a,
                              param_io16):
        """Implements Py_LotsOfDataTypes.MethodTest()

        Keyword arguments:
        env -- Provider Environment
        object_name -- A CIMInstanceName or CIMCLassName specifying the 
            object on which the method %(mname)s should be invoked
        method -- A CIMMethod representing the method meta-data
        param_paths --  The input parameter paths (type REF CIM_System (CIMInstanceName)[]) 
        param_embedded --  The input parameter embedded (type string) 
        param_nullparam --  The input parameter nullParam (type datetime) 
        param_uint8array --  The input parameter uint8array (type uint8[]) 
        param_s --  The input parameter s (type string) (Required)
        param_embedded_a --  The input parameter embedded_a (type string[]) 
        param_io16 --  The input parameter io16 (type sint16) 

        Returns a two-tuple containing the return value (type string)
        and a dictionary with the out-parameters

        Output parameters:
        paths -- (type REF CIM_System (CIMInstanceName)[]) 
        b -- (type boolean) 
        embedded -- (type string) 
        nullParam -- (type datetime) 
        embedded_a -- (type string[]) 
        io16 -- (type sint16) 
        r64 -- (type real64) 
        msg -- (type string) 

        """

        # TODO do something
        insts = [pywbem.CIMInstance(classname='PyFoo', 
                                    properties={'FooValue': pywbem.Sint32(3), 
                                                'FooKey': 'Key3'},
                                    path=pywbem.CIMInstanceName(classname='PyFoo', 
                                                                namespace='root/cimv2')),
                 pywbem.CIMInstance(classname='PyFoo', 
                                    properties={'FooValue': pywbem.Sint32(2), 
                                                'FooKey': 'Key2'},
                                    path=pywbem.CIMInstanceName(classname='PyFoo', 
                                                                namespace='root/cimv2'))]
                 

        out_params = {}
        out_params['paths'] = param_paths
        out_params['b'] = True
        if param_embedded is not None:
            out_params['embedded'] = param_embedded
        else:
            out_params['embedded'] = insts[0]
        out_params['nullparam'] = None
        if param_embedded_a is not None:
            out_params['embedded_a'] = param_embedded_a
        else:
            out_params['embedded_a'] = insts
        out_params['io16'] = pywbem.Sint16(16)
        out_params['r64'] = pywbem.Real64(3.14159)
        out_params['msg'] = 'A message'
        rval = 'Return'+`out_params`
        return (rval, out_params)

## end of class Py_LotsOfDataTypesProvider

def get_providers(env):
    _py_lotsofdatatypes_prov = Py_LotsOfDataTypesProvider()
    return {'Py_LotsOfDataTypes': _py_lotsofdatatypes_prov} 

