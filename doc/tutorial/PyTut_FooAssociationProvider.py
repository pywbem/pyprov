"""Python Provider for PyTut_FooAssociation

Instruments the CIM class PyTut_FooAssociation

"""

import pywbem
from pycim import CIMProvider

class PyTut_FooAssociationProvider(CIMProvider):
    """Instrument the CIM class PyTut_FooAssociation 

    Relationship between a PyTut_Foo and a PyTut_FooComponent
    
    """
    #--------------------------------------------------------------------------
    def __init__ (self):
        pass

    #--------------------------------------------------------------------------
    def get_instance (self, env, model, cim_class):

        env.get_logger().log_debug('PyTut_FooAssociationProvider.getInstance '
            'called for class %s' % cim_class.classname)

        ch = env.get_cimom_handle()
        # Verify that the TheFooComp key actually refers to a
        # PyTut_FooComponent object.
        # This GetInstance call should raise a CIM_ERR_NOT_FOUND exception
        # if the PyTut_FooCompoent does not exist
        foocompinst = ch.GetInstance(model['TheFooComp'], LocalOnly=False,
            IncludeQualifiers=False, IncludeClassOrigin=False)

        # Verify that TheFoo key actually refers to a PyTut_Foo object
        # This GetInstance call should raise a CIM_ERR_NOT_FOUND exception
        # if the PyTut_Foo does not exist
        fooinst = ch.GetInstance(model['TheFoo'], LocalOnly=False,
            IncludeQualifiers=False, IncludeClassOrigin=False)

        # Verify that the PyTut_FooComponent.FooCompValue property has the same
        # value as the PyTut_Foo.FooKey property
        if foocompinst['FooCompValue'] != fooinst['FooKey']:
            raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND)

        # Everything checks out, so return the given model instance since it
        # already represents a completely filled out instance
        return model

    #--------------------------------------------------------------------------
    def enum_instances(self, env, model, cim_class, keys_only):

        ch = env.get_cimom_handle()
        logger = env.get_logger()
        
        # Build a dictionary of instance names for the PyTut_Foo class
        fooNames = {}
        try:
            ch.EnumerateInstanceNames('PyTut_Foo', model.path.namespace,
                Handler=lambda iname: fooNames.setdefault(iname['FooKey'], iname))
        except:
            logger.log_error('PyTut_FooAssociation. Got error enumerating '
                'PyTut_Foo instance names')
            return

        ilist = []
        try:
            ch.EnumerateInstances('PyTut_FooComponent', model.path.namespace,
                LocalOnly=False, IncludeQualifiers=False, 
                Handler=lambda inst: inst['FooCompValue'] in fooNames and ilist.append(inst))
        except:
            logger.log_error('PyTut_FooAssociation. Got error enumerating '
                'PyTut_FooComponent instances')
            return

        for inst in ilist:
            if 'TheFooComp' in model:
                model['TheFooComp'] = inst.path
            if 'TheFoo' in model:
                model['TheFoo'] = fooNames[inst['FooCompValue']]
            yield model

    #--------------------------------------------------------------------------
    def set_instance(self, env, instance, previous_instance, cim_class):
        # These are dynamic association, so we don't allow modifications or
        # additions
        raise pywbem.CIMError(pywbem.CIM_ERR_NOT_SUPPORTED)

    #--------------------------------------------------------------------------
    def delete_instance(self, env, instance_name):
        # These are dynamic association, so we don't allow deletion
        raise pywbem.CIMError(pywbem.CIM_ERR_NOT_SUPPORTED)
 
    #--------------------------------------------------------------------------
    def references(self, env, object_name, model, assoc_class, 
                   result_class_name, role, result_role, keys_only):
        logger = env.get_logger()
        if object_name.classname.lower() == 'pytut_foo':
            if role and role.lower() != 'thefoo':
                logger.log_debug('PyTut_FooAssociation. Invalid role: %s' % role)
                return
            if result_role and result_role.lower() != 'thefoocomp':
                logger.log_debug('PyTut_FooAssociation. Invalid result_role: '
                    '%s' % result_role)
                return
            if result_class_name and result_class_name.lower() != 'pytut_foocomponent':
                logger.log_debug('PyTut_FooAssociation. Invalid '
                    'result_class_name: %s' % result_class_name)
                return

            fk = 'FooKey' in object_name.keybindings and object_name['FooKey'] or None
            if not fk:
                logger.log_error('PyTut_FooAssociation. No key value given '
                    'for PyTut_Foo object')
                return

            # Get all instances of PyTut_FooComponent that have a FooCompValue
            # property that matches the key to this PyTut_Foo object

            ch = env.get_cimom_handle()
            ilist = []
            try:
                ch.EnumerateInstances('PyTut_FooComponent', object_name.namespace,
                    LocalOnly=False, IncludeQualifiers=False,
                    Handler=lambda inst: inst['FooCompValue'] == fk and ilist.append(inst))
            except:
                logger.log_error('PyTut_FooAssociation. Got error enumerating '
                    'instances of PyTut_FooComponent')
                return

            # All instances of PyTut_FooComponent that refer to the given
            # PyTut_Foo should be in ilist at this point
            for inst in ilist:
                if 'TheFooComp' in model:
                    model['TheFooComp'] = inst.path
                if 'TheFoo' in model:
                    model['TheFoo'] = object_name
                yield model

        elif object_name.classname.lower() == 'pytut_foocomponent':
            if role and role.lower() != 'thefoocomp':
                logger.log_debug('PyTut_FooAssociation. Invalid role: %s' % role)
                return
            if result_role and result_role.lower() != 'thefoo':
                logger.log_debug('PyTut_FooAssociation. Invalid result_role: '
                    '%s' % result_role)
                return
            if result_class_name and result_class_name.lower() != 'pytut_foo':
                logger.log_debug('PyTut_FooAssociation. Invalid '
                    'result_class_name: %s' % result_class_name)
                return

            ch = env.get_cimom_handle()
            try:
                # Get the instance of the PyTut_FooComponent in order to get
                # its FooCompValue property
                inst = ch.GetInstance(object_name, LocalOnly=False,
                    IncludeQualifiers=False, IncludeClassOrigin=False)
                fooname = pywbem.CIMInstanceName('PyFoo',
                    {'FooKey':inst['FooCompValue']},
                    namespace=object_name.namespace)
                fooinst = ch.GetInstance(fooname, LocalOnly=False,
                    IncludeQualifiers=False, IncludeClassOrigin=False)
                if 'TheFooComp' in model:
                    model['TheFooComp'] = object_name
                if 'TheFoo' in model:
                    model['TheFoo'] = fooinst.path
                yield model
            except:
                pass

## end of class PyTut_FooAssociationProvider

def get_providers(env):
    _pytut_fooassociation_prov = PyTut_FooAssociationProvider()  
    return {'PyTut_FooAssociation': _pytut_fooassociation_prov} 
