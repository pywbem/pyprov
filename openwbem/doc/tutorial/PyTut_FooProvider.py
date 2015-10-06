"""Python Provider for PyTut_Foo

Instruments the CIM class PyTut_Foo

"""

import pywbem
from pywbem import CIMProvider

#----------------------------------------
# Instance Data
#----------------------------------------
_FooInsts = {
    'Key1': pywbem.Sint32(1),
    'Key2': pywbem.Sint32(2),
    'Key3': pywbem.Sint32(3)
}
_FooComps = {
    'TheKey1':'Key1', 
    'TheKey2':'Key2', 
    'TheKey3':'Key3',
    'TheKey4':'Key1', 
    'TheKey5':'Key1'
}

#------------------------------------------------------------------------------
# Create the CIMProvider sub-class for the PyTut_Foo class
#------------------------------------------------------------------------------
class PyTut_FooProvider(CIMProvider):

    def __init__ (self):
        pass

    #--------------------------------------------------------------------------
    def enum_instances(self,
                       env,
                       model,
                       cim_class,
                       keys_only):
        logger = env.get_logger();
        logger.log_debug("### enum_instances for PyTut_Foo")
        for k,v in _FooInsts.items():
            if keys_only:
                model['FooKey'] = k
            else:
                if 'FooKey' in model:
                    model['FooKey'] = k
                if 'FooValue' in model:
                    model['FooValue'] = v
            yield model

    #--------------------------------------------------------------------------
    def get_instance (self,
                      env,
                      model,
                      cim_class):
        logger = env.get_logger();
        logger.log_debug("### get_instance for PyTut_Foo")
        try:
            if 'FooValue' in model: 
                model['FooValue'] = _FooInsts[model['FooKey']]
        except KeyError:
            raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND)
        return model

    #--------------------------------------------------------------------------
    def set_instance(self,
                     env,
                     instance,
                     previous_instance,
                     cim_class):
        if not previous_instance:
            # Create new instance
            if not 'FooKey' in instance \
                    or not instance['FooKey']:
                raise pywbem.CIMError(pywbem.CIM_ERR_INVALID_PARAMETER,
                    "'FooKey' property is required")

            if not 'FooValue' in instance:
                raise pywbem.CIMError(pywbem.CIM_ERR_INVALID_PARAMETER,
                    "'FooValue' property is required")

            if instance['FooKey'] in _FooInsts:
                raise pywbem.CIMError(pywbem.CIM_ERR_ALREADY_EXISTS)

            _FooInsts[instance['FooKey']] = instance['FooValue']
            return instance

        # Modification

        if not 'FooKey' in previous_instance \
                or not previous_instance['FooKey']:
            raise pywbem.CIMError(pywbem.CIM_ERR_INVALID_PARAMETER,
                "Unknown instance for modification")
        if not previous_instance['FooKey'] in _FooInsts:
            raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND)

        if not 'FooValue' in instance:
            # Not modifying the only thing that can be modified, so there is
            # nothing to do.
            return instance

        # Modify the dictionary element
        _FooInsts[previous_instance['FooKey']] = instance['FooValue']
        return instance

    #--------------------------------------------------------------------------
    def delete_instance(self,
                        env,
                        instance_name):
        try:
            del _FooInsts[instance_name['FooKey']]
        except KeyError:
            raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND)

    #--------------------------------------------------------------------------
    def cim_method_foomethod(self,
                             env,
                             object_name,
                             method,
                             param_i,
                             param_s):
        out_params = {}
        out_params['i'] = pywbem.Sint32(param_i and -param_i or 0)
        out_params['sa'] = param_s and param_s.split() or ['Nothing']
        rval = 'executed on %s. s: %s' %(`object_name`, param_s)
        return (rval, out_params)

## end of class PyTut_FooProvider

#------------------------------------------------------------------------------
# Create the CIMProvider sub-class for the PyTut_FooComponent class
#------------------------------------------------------------------------------
class PyTut_FooComponentProvider(CIMProvider):

    def __init__ (self):
        pass

    #--------------------------------------------------------------------------
    def get_instance (self,
                      env,
                      model,
                      cim_class):
        try:
            if 'FooCompValue' in model:
                model['FooCompValue'] = _FooComps[model['FooCompKey']]
        except KeyError:
            raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND)
        return model

    #--------------------------------------------------------------------------
    def enum_instances(self,
                       env,
                       model,
                       cim_class,
                       keys_only):
        for k,v in _FooComps.items():
            model['FooCompKey'] = k
            if not keys_only:
                model['FooCompValue'] = v
            yield model

    #--------------------------------------------------------------------------
    def set_instance(self,
                     env,
                     instance,
                     previous_instance,
                     cim_class):
        if not previous_instance:
            # Create new instance
            if not 'FooCompKey' in instance:
                raise pywbem.CIMError(pywbem.CIM_INVALID_PARAMETER,
                    "'FooCompKey' property is required")

            if not 'FooCompValue' in instance:
                raise pywbem.CIMError(pywbem.CIM_INVALID_PARAMETER,
                    "'FooCompValue' property is required")

            _FooComps[instance['FooCompKey']] = instance['FooCompValue']
            return instance

        # Modification
        if not 'FooCompValue' in instance:
            # Not modifying the only thing that can be modified
            return instance

        if 'FooCompKey' in instance:
            fv = instance['FooCompKey']
        elif 'FooCompKey' in previous_instance:
            fv = previous_instance['FooCompKey']
        else:
            raise pywbem.CIMError(pywbem.CIM_INVALID_PARAMETER,
                "'FooCompValue' property not present")
        _FooComps[fv] = instance['FooCompValue']

    #--------------------------------------------------------------------------
    def delete_instance(self,
                        env,
                        instance_name):
        try:
            del _FooComps[instance_name['FooCompKey']]
        except KeyError:
            raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND)

## end of class PyTut_FooProvider

def get_providers(env):
    # Create an instance of the PyTut_Foo instance/method provider
    _pytut_foo_prov = PyTut_FooProvider()
    # Create an instance of the PyTut_FooComponent instance provider
    _pytut_foocomp_prov = PyTut_FooComponentProvider()
    # Set up our py_providers dictionary so the provider proxy knows the CIM
    # classes handled by the provider objects.
    return { 'PyTut_Foo': _pytut_foo_prov,
        'PyTut_FooComponent': _pytut_foocomp_prov }

