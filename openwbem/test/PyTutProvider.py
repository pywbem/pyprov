"""Python Provider for PyTut_Foo

Instruments the CIM class PyTut_Foo

"""

import pywbem
from pycim import CIMProvider

if __name__ == '__main__':
    print 'This is a CIM provider. It is not meant to be run standalone'
    import sys
    sys.exit(1)

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
    def get_instance (self,
                      env,
                      model,
                      cim_class):
        try:
            if 'FooValue' in model.properties:
                model['FooValue'] = _FooInsts[model['FooKey']]
        except KeyError:
            raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND,'')
        return model

    #--------------------------------------------------------------------------
    def enum_instances(self,
                       env,
                       model,
                       cim_class,
                       keys_only):
        for k,v in _FooInsts.items():
            model['FooKey'] = k
            if not keys_only:
                model['FooValue'] = v
            yield model

    #--------------------------------------------------------------------------
    def set_instance(self,
                     env,
                     instance,
                     previous_instance,
                     cim_class):
        if not previous_instance:
            # Create new instance
            if not 'FooKey' in instance.properties:
                raise pywbem.CIMError(pywbem.CIM_INVALID_PARAMETER,
                    "'FooKey' property is required")

            if not 'FooValue' in instance.properties:
                raise pywbem.CIMError(pywbem.CIM_INVALID_PARAMETER,
                    "'FooValue' property is required")

            _FooInsts[instance['FooKey']] = instance['FooValue']
            return instance

        # Modification
        if not 'FooValue' in instance.properties:
            # Not modifying the only thing that can be modified
            return instance

        if 'FooKey' in instance.properties:
            fv = instance['FooKey']
        elif 'FooKey' in previous_instance.properties:
            fv = previous_instance['FooKey']
        else:
            raise pywbem.CIMError(pywbem.CIM_INVALID_PARAMETER,
                "'FooValue' property not present")
        _FooInsts[fv] = instance['FooValue']

    #--------------------------------------------------------------------------
    def delete_instance(self,
                        env,
                        instance_name):
        try:
            del _FooInsts[instance_name['FooKey']]
        except KeyError:
            raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND, '')

    #--------------------------------------------------------------------------
    def cim_method_foomethod(self,
                             env,
                             object_name,
                             method,
                             param_i,
                             param_s):
        out_params = {}
        out_params['i'] = pywbem.Sint32(-param_i)
        out_params['sa'] = ['some','string','array']
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
            if 'FooCompValue' in model.properties:
                model['FooCompValue'] = _FooComps[model['FooCompKey']]
        except KeyError:
            raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND,'')
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
            if not 'FooCompKey' in instance.properties:
                raise pywbem.CIMError(pywbem.CIM_INVALID_PARAMETER,
                    "'FooCompKey' property is required")

            if not 'FooCompValue' in instance.properties:
                raise pywbem.CIMError(pywbem.CIM_INVALID_PARAMETER,
                    "'FooCompValue' property is required")

            _FooComps[instance['FooCompKey']] = instance['FooCompValue']
            return instance

        # Modification
        if not 'FooCompValue' in instance.properties:
            # Not modifying the only thing that can be modified
            return instance

        if 'FooCompKey' in instance.properties:
            fv = instance['FooCompKey']
        elif 'FooCompKey' in previous_instance.properties:
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
            raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND, '')

## end of class PyTut_FooProvider


#------------------------------------------------------------------------------
# Create the CIMProvider sub-class for the PyTut_FooAssociation class
#------------------------------------------------------------------------------
class PyTut_FooAssociationProvider(CIMProvider):
    def __init__ (self):
        pass

    #--------------------------------------------------------------------------
    def get_instance (self, env, model, cim_class):

        # First verify that TheFoo actually refers to an entry in our
        # _FooInsts dictionary
        lpath = model['TheFoo']
        if not 'FooKey' in lpath.keybindings:
            # TheFoo keyproperty missing the 'FooKey' property
            raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND,'')
        fk = lpath['FooKey']
        if not fk in _FooInsts:
            # We don't have this entry in our dictionary
            raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND,'')

        # Now verity that TheFooComp actually refers to an entry in out
        # _FooComps dictionary
        lpath = model['TheFooComp']
        if not 'FooCompKey' in lpath.keybindings:
            # TheFooComp keyproperty missing the 'FooCompKey' property
            raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND,'')
        fck = lpath['FooCompKey']
        if not fck in _FooComps:
            # We don't have this entry in our dictionary
            raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND,'')

        # Verity that there truely is an association between these 2 objects
        if not _FooComps[fck] in _FooInsts:
            # The PyTut_FooComponent doesn't have a value that refers to the
            # PyTut_Foo object, so there is not association between them.
            raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND,'')

        # Everything checked out, so the given keys must refers to an actual
        # PyTut_FooAssociation object
        return model

    #--------------------------------------------------------------------------
    def enum_instances(self, env, model, cim_class, keys_only):
        for k, v in _FooComps.items():
            if v in _FooInsts:
                model['TheFoo'] = pywbem.CIMInstanceName(classname='PyTut_Foo',
                    namespace=model.path.namespace, keybindings={'FooKey':v})
                model['TheFooComp'] = pywbem.CIMInstanceName(classname='PyTut_FooComponent',
                    namespace=model.path.namespace, keybindings={'FooCompKey':k})
                yield model

    #--------------------------------------------------------------------------
    def set_instance(self, env, instance, previous_instance, cim_class):
        raise pywbem.CIMError(pywbem.CIM_ERR_NOT_SUPPORTED,
            'Cannot create/modify instances of PyTut_FooAssociation')

    #--------------------------------------------------------------------------
    def delete_instance(self, env, instance_name):
        raise pywbem.CIMError(pywbem.CIM_ERR_NOT_SUPPORTED,
            'Cannot delete instances of PyTut_FooAssociation')

    #--------------------------------------------------------------------------
    def references(self, env, object_name, model, assoc_class, 
                   result_class_name, role, result_role,
                   keys_only):

        if object_name.classname.lower() == 'pytut_foo':
            # The target object for the association related call is a PyTut_Foo
            # object.
            model['TheFoo'] = object_name
            for k, v in _FooComps.items():
                if v == object_name['FooKey']:
                    model['TheFooComp'] = pywbem.CIMInstanceName(classname='PyTut_FooComponent',
                        namespace=object_name.namespace, keybindings={'FooCompKey':k})
                    yield model
        elif object_name.classname.lower() == 'pytut_foocomponent':
            # The target object for the association related call is a
            # PyTut_FooComponent object
            model['TheFoo'] = object_name
            thekey = object_name['FooCompKey']
            if not thekey in _FooComps:
                return;
            model['TheFooComp'] = object_name
            thevalue = _FooComps[thekey]
            for k,v in _FooInsts.items():
                if k == thevalue:
                    model['TheFoo'] = pywbem.CIMInstanceName(classname='PyTut_Foo',
                        namespace=object_name.namespace, keybindings={'FooKey':k})
                    yield model

## end of class PyTut_FooAssociationProvider

def get_providers(env):
    # Create an instance of the PyTut_Foo instance/method provider
    _pytut_foo_prov = PyTut_FooProvider()
    # Create an instance of the PyTut_FooComponent instance provider
    _pytut_foocomp_prov = PyTut_FooComponentProvider()
    # Create an instance of the PyTut_FooAssociation associator/instance provider
    _pytut_foo_assoc_prov = PyTut_FooAssociationProvider()
    # Set up our py_providers dictionary so the provider proxy knows the CIM
    # classes handled by the provider objects.
    return  {
        'PyTut_Foo': _pytut_foo_prov,
        'PyTut_FooComponent': _pytut_foocomp_prov,
        'PyTut_FooAssociation': _pytut_foo_assoc_prov }

