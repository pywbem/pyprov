
import pywbem
from pycim import CIMProvider


_PyFooInsts = {'Key1': pywbem.Sint32(1), 'Key2': pywbem.Sint32(2), 'Key3': pywbem.Sint32(3)}

_PyFooComps = {'TheKey1':'Key1', 'TheKey2':'Key2', 'TheKey3':'Key3', \
    'TheKey4':'Key1', 'TheKey5':'Key1'}


class PyFooProvider(CIMProvider):
    """Instrument the CIM class PyFoo 

            Test class for a python provider
            
    """

    #########################################################################
    def __init__ (self):
        pass

    #########################################################################
    def get_instance (self, env, model, cim_class):
        """Return an instance of PyFoo

        Keyword arguments:
        env -- Provider Environment
        model -- A template of the CIMInstance to be returned.  The key 
            properties are set on this instance to correspond to the 
            instanceName that was requested.  The properties of the model
            are already filtered according to the PropertyList from the 
            request.
        cim_class -- The CIMClass PyFoo

        """

        try:
            if 'FooValue' in model.properties:
                model['FooValue'] = _PyFooInsts[model['FooKey']]
        except KeyError:
            raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND, '')
        return model

    #########################################################################
    def enum_instances(self, env, model, cim_class, keys_only):
        """ Enumerate instances of PyFoo
        The WBEM operations EnumerateInstances and EnumerateInstanceNames
        are both mapped to this method. 
        This method is a python generator

        Keyword arguments:
        env -- Provider Environment
        model -- A template of the CIMInstances to be generated.  The 
            properties of the model are already filtered according to the 
            PropertyList from the request.
        cim_class -- The CIMClass PyFoo
        keys_only -- A boolean.  True if only the key properties should be
            set on the generated instances.

        """

        for k,v in _PyFooInsts.items():
            model['FooKey'] = k
            if keys_only:
                yield model
            else:
                try:
                    yield self.get_instance(env, model, cim_class)
                except pywbem.CIMError, (num, msg):
                    if num in (pywbem.CIM_ERR_NOT_FOUND, 
                        pywbem.CIM_ERR_ACCESS_DENIED):
                        pass # EnumerateInstances shouldn't return these
                    else:
                        raise

    #########################################################################
    def set_instance(self, env, instance, previous_instance, cim_class):
        """ Return a newly created or modified instance of PyFoo

        Keyword arguments:
        env -- Provider Environment
        instance -- The new CIMInstance.  If modifying an existing instance,
            the properties on this instance have been filtered by the 
            PropertyList from the request.
        previous_instance -- The previous instance if modifying an existing 
            instance.  None if creating a new instance. 
        cim_class -- The CIMClass PyFoo

        Return the new instance.  The keys must be set on the new instance. 

        """

        _PyFooInsts[instance['FooKey']] = instance['FooValue']

        return instance

    #########################################################################
    def delete_instance(self, env, instance_name):
        """ Delete an instance of PyFoo

        Keyword arguments:
        env -- Provider Environment
        instance_name -- A CIMInstanceName specifying the instance of 
            PyFoo to delete.

        """ 
        try:
            del _PyFooInsts[instance_name['FooKey']]
        except KeyError:
            raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND, '')

    #########################################################################
    def cim_method_foomethod(self, env, object_name, method,
                                  param_i,
                                  param_s):
        """Implements PyFoo.FooMethod()

            Test method
            
        Keyword arguments:
        env -- Provider Environment
        object_name -- A CIMInstanceName or CIMCLassName specifying the 
            object on which the method %(mname)s should be invoked
        method -- A CIMMethod representing the method meta-data
        param_i --  The input parameter i (type sint32) 
        param_s --  The input parameter s (type string) 

        Returns a two-tuple containing the return value (type string)
        and a dictionary with the out-parameters

        Output parameters:
        i -- (type sint32) 
        sa -- (type string[]) 

        """

        out_params = {}
        out_params['i'] = pywbem.Sint32(-param_i)
        out_params['sa'] = ['some','string','array']
        rval = 'executed on %s. s: %s' %(`object_name`, param_s)
        return (rval, out_params)
        

## end of class PyFooProvider
class PyFooComponentProvider(CIMProvider):
    """Instrument the CIM class PyFooComponent 

            Another test class to associate to PyFoo. PyFooComponents will
            have associations to PyFoo objects if the PyFooComponents values
            matches the PyFoo key
            
    """

    #########################################################################
    def __init__ (self):
        pass

    #########################################################################
    def get_instance (self, env, model, cim_class):
        """Return an instance of PyFooComponent

        Keyword arguments:
        env -- Provider Environment
        model -- A template of the CIMInstance to be returned.  The key 
            properties are set on this instance to correspond to the 
            instanceName that was requested.  The properties of the model
            are already filtered according to the PropertyList from the 
            request.
        cim_class -- The CIMClass PyFooComponent

        """

        try:
            if 'TheValue' in model.properties:
                model['TheValue'] = _PyFooComps[model['TheKey']]
        except KeyError:
            raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND, '')
        return model

    #########################################################################
    def enum_instances(self, env, model, cim_class, keys_only):
        """ Enumerate instances of PyFooComponent
        The WBEM operations EnumerateInstances and EnumerateInstanceNames
        are both mapped to this method. 
        This method is a python generator

        Keyword arguments:
        env -- Provider Environment
        model -- A template of the CIMInstances to be generated.  The 
            properties of the model are already filtered according to the 
            PropertyList from the request.
        cim_class -- The CIMClass PyFooComponent
        keys_only -- A boolean.  True if only the key properties should be
            set on the generated instances.

        """

        for _ in _PyFooComps.keys():
            model['TheKey'] = _
            if keys_only:
                yield model
            else:
                try:
                    yield self.get_instance(env, model, cim_class)
                except pywbem.CIMError, (num, msg):
                    if num in (pywbem.CIM_ERR_NOT_FOUND, 
                        pywbem.CIM_ERR_ACCESS_DENIED):
                        pass # EnumerateInstances shouldn't return these
                    else:
                        raise

    #########################################################################
    def set_instance(self, env, instance, previous_instance, cim_class):
        """ Return a newly created or modified instance of PyFooComponent

        Keyword arguments:
        env -- Provider Environment
        instance -- The new CIMInstance.  If modifying an existing instance,
            the properties on this instance have been filtered by the 
            PropertyList from the request.
        previous_instance -- The previous instance if modifying an existing 
            instance.  None if creating a new instance. 
        cim_class -- The CIMClass PyFooComponent

        Return the new instance.  The keys must be set on the new instance. 

        """

        _PyFooComps[instance['TheKey']] = instance['TheValue']
        return instance

    #########################################################################
    def delete_instance(self, env, instance_name):
        """ Delete an instance of PyFooComponent

        Keyword arguments:
        env -- Provider Environment
        instance_name -- A CIMInstanceName specifying the instance of 
            PyFooComponent to delete.

        """ 
        try:
            del _PyFooComps[instance_name['TheKey']]
        except KeyError:
            raise pybem.CIMError(pywbem.CIM_ERR_NOT_FOUND, '')

## end of class PyFooComponentProvider

class PyFooAssociationProvider(CIMProvider):
    """Instrument the CIM class PyFooAssociation 

            Relationship between a PyFoo and a PyFooComponent
            
    """

    #########################################################################
    def __init__ (self):
        pass

    #########################################################################
    def get_instance (self, env, model, cim_class):
        """Return an instance of PyFooAssociation

        Keyword arguments:
        env -- Provider Environment
        model -- A template of the CIMInstance to be returned.  The key 
            properties are set on this instance to correspond to the 
            instanceName that was requested.  The properties of the model
            are already filtered according to the PropertyList from the 
            request.
        cim_class -- The CIMClass PyFooAssociation

        """

        return model

    #########################################################################
    def enum_instances(self, env, model, cim_class, keys_only):
        """ Enumerate instances of PyFooAssociation
        The WBEM operations EnumerateInstances and EnumerateInstanceNames
        are both mapped to this method. 
        This method is a python generator

        Keyword arguments:
        env -- Provider Environment
        model -- A template of the CIMInstances to be generated.  The 
            properties of the model are already filtered according to the 
            PropertyList from the request.
        cim_class -- The CIMClass PyFooAssociation
        keys_only -- A boolean.  True if only the key properties should be
            set on the generated instances.

        """

        for comp_k, foo_k in _PyFooComps.items():
            # TODO fetch system resource
            # Key properties    
            model['ThePyFoo'] = pywbem.CIMInstanceName(classname='PyFoo', 
                                                       namespace=model.path.namespace,
                                                       keybindings={'FooKey':foo_k})
            model['TheComp'] = pywbem.CIMInstanceName(classname='PyFooComponent',
                                                       namespace=model.path.namespace,
                                                       keybindings={'TheKey':comp_k})
            if keys_only:
                yield model
            else:
                try:
                    yield self.get_instance(env, model, cim_class)
                except pywbem.CIMError, (num, msg):
                    if num in (pywbem.CIM_ERR_NOT_FOUND, 
                        pywbem.CIM_ERR_ACCESS_DENIED):
                        pass # EnumerateInstances shouldn't return these
                    else:
                        raise

    #########################################################################
    def set_instance(self, env, instance, previous_instance, cim_class):
        """ Return a newly created or modified instance of PyFooAssociation

        Keyword arguments:
        env -- Provider Environment
        instance -- The new CIMInstance.  If modifying an existing instance,
            the properties on this instance have been filtered by the 
            PropertyList from the request.
        previous_instance -- The previous instance if modifying an existing 
            instance.  None if creating a new instance. 
        cim_class -- The CIMClass PyFooAssociation

        Return the new instance.  The keys must be set on the new instance. 

        """

        raise pywbem.CIMError(pywbem.CIM_ERR_NOT_SUPPORTED,'')

    #########################################################################
    def delete_instance(self, env, instance_name):
        """ Delete an instance of PyFooAssociation

        Keyword arguments:
        env -- Provider Environment
        instance_name -- A CIMInstanceName specifying the instance of 
            PyFooAssociation to delete.

        """ 
        raise pywbem.CIMError(pywbem.CIM_ERR_NOT_SUPPORTED,'')
    #########################################################################
    def references(self, env, object_name, model, assoc_class, 
                   result_class_name, role, result_role, keys_only):
        """Instrument PyFooAssociation Associations.
        All four association-related operations (Associators, AssociatorNames, 
        References, ReferenceNames) are mapped to this method. 
        This method is a python generator

        Keyword arguments:
        env -- Provider Environment
        object_name -- A CIMInstanceName that defines the source CIM Object
            whose associated Objects are to be returned.
        model -- A template CIMInstance of PyFooAssociation to serve as a model
            of the objects to be returned.  Only properties present on this
            model need to be returned. 
        assoc_class -- The CIMClass PyFooAssociation
        result_class_name -- If not None, acts as a filter on the returned set 
            of Objects by mandating that each returned Object MUST be either 
            an Instance of this Class (or one of its subclasses) or be this 
            Class (or one of its subclasses).
        role -- If not None, acts as a filter on the returned set of Objects 
            by mandating that each returned Object MUST be associated to the 
            source Object via an Association in which the source Object plays 
            the specified role (i.e. the name of the Property in the 
            Association Class that refers to the source Object MUST match 
            the value of this parameter).
        result_role -- If not None, acts as a filter on the returned set of 
            Objects by mandating that each returned Object MUST be associated 
            to the source Object via an Association in which the returned 
            Object plays the specified role (i.e. the name of the Property in 
            the Association Class that refers to the returned Object MUST 
            match the value of this parameter).
        """
        if object_name.classname.lower() == 'pyfoo':
            model['ThePyFoo'] = object_name
            for k, v in _PyFooComps.items():
                if v == object_name['FooKey']:
                    model['TheComp'] = pywbem.CIMInstanceName(classname='PyFooComponent',
                        namespace=object_name.namespace, keybindings={'TheKey':k})
                    yield model
        elif object_name.classname.lower() == 'pyfoocomponent':
            model['TheComp'] = object_name
            try:
                model['ThePyFoo'] = pywbem.CIMInstanceName(classname='PyFoo',
                    namespace=object_name.namespace,
                    keybindings={'FooKey':_PyFooComps[object_name['TheKey']]})
                yield model
            except KeyError:
                pass
        else:
            raise pywbem.CIMError(pywbem.CIM_ERR_FAILED, '')




## end of class PyFooAssociationProvider

def get_providers(env):
    _pyfooassociation_prov = PyFooAssociationProvider()    
    _pyfoocomponent_prov = PyFooComponentProvider() 
    _py_foo_provider = PyFooProvider()        
    return   {'PyFoo': _py_foo_provider,
              'PyFooComponent': _pyfoocomponent_prov,
              'PyFooAssociation': _pyfooassociation_prov}  


