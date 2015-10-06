"""Python Provider for Python_Sample_Class

Instruments the CIM class Python_Sample_Class

"""

import pywbem

class Python_Sample_ClassProvider(pywbem.CIMProvider):
    """Instrument the CIM class Python_Sample_Class

    Python Sample Class for Pegasus Python Provider IFC

    """

    def __init__ (self, env):
        logger = env.get_logger()
        logger.log_debug('Initializing provider %s from %s' \
                % (self.__class__.__name__, __file__))

    def get_instance(self, env, model, cim_class):
        logger = env.get_logger()
        logger.log_debug('Entering %s.get_instance()' \
                % self.__class__.__name__)

        model.update_existing(Description="TheDescription for %s" %model['Name'])
        return model

    def enum_instances(self, env, model, cim_class, keys_only):
        logger = env.get_logger()
        logger.log_debug('Entering %s.enum_instances()' \
                % self.__class__.__name__)

        for i in range(5):
            model.update_existing(Name=str(i))
            if keys_only:
                yield model
            else:
                try:
                    yield self.get_instance(env, model, cim_class)
                except pywbem.CIMError, (num, msg):
                    if num not in (pywbem.CIM_ERR_NOT_FOUND,
                            pywbem.CIM_ERR_ACCESS_DENIED):
                        raise

    def set_instance(self, env, instance, previous_instance, cim_class):
        logger = env.get_logger()
        logger.log_debug('Entering %s.set_instance()' \
                % self.__class__.__name__)
        # TODO create or modify the instance
        raise pywbem.CIMError(pywbem.CIM_ERR_NOT_SUPPORTED) # Remove to implement
        return instance

    def delete_instance(self, env, instance_name):
        logger = env.get_logger()
        logger.log_debug('Entering %s.delete_instance()' \
                % self.__class__.__name__)

        # TODO delete the resource
        raise pywbem.CIMError(pywbem.CIM_ERR_NOT_SUPPORTED) # Remove to implement

    def cim_method_cout(self, env, object_name, method, param_postfix):
        logger = env.get_logger()
        logger.log_debug('Entering %s.invoke_method(): %s' %(self.__class__.__name__,method))

        # postfix = params['postfix']
        print ('Entering %s.invoke_method(): %s' % (self.__class__.__name__, method) )
        result = (object_name[Name] + " " + param_postfix)
        out_params = {}
        print result
        return (result, out_params)


## end of class Python_Sample_ClassProvider

class Python_Sample_Class_2Provider(pywbem.CIMProvider):
    """Instrument the CIM class Python_Sample_Class_2

    Python Sample Class for Pegasus Python Provider IFC

    """

    def __init__ (self, env):
	print "In PSC_2::init"
        logger = env.get_logger()
        logger.log_debug('Initializing provider %s from %s' \
                % (self.__class__.__name__, __file__))

    def get_instance(self, env, model, cim_class):
	print "In PSC_2::get_instance"
        logger = env.get_logger()
        logger.log_debug('Entering %s.get_instance()' \
                % self.__class__.__name__)

        model.update_existing(Description="TheDescription for %s" %model['Name'])
        return model

    def enum_instances(self, env, model, cim_class, keys_only):
	print "In PSC_2::enum_instance"
        logger = env.get_logger()
        logger.log_debug('Entering %s.enum_instances()' \
                % self.__class__.__name__)

        for i in range(5):
            model.update_existing(Name=str(i*100))
            if keys_only:
                yield model
            else:
                try:
                    yield self.get_instance(env, model, cim_class)
                except pywbem.CIMError, (num, msg):
                    if num not in (pywbem.CIM_ERR_NOT_FOUND,
                            pywbem.CIM_ERR_ACCESS_DENIED):
                        raise

    def set_instance(self, env, instance, previous_instance, cim_class):
        logger = env.get_logger()
        logger.log_debug('Entering %s.set_instance()' \
                % self.__class__.__name__)
        # TODO create or modify the instance
        raise pywbem.CIMError(pywbem.CIM_ERR_NOT_SUPPORTED) # Remove to implement
        return instance

    def delete_instance(self, env, instance_name):
        logger = env.get_logger()
        logger.log_debug('Entering %s.delete_instance()' \
                % self.__class__.__name__)

        # TODO delete the resource
        raise pywbem.CIMError(pywbem.CIM_ERR_NOT_SUPPORTED) # Remove to implement

    def cim_method_cout(self, env, object_name, method_name, param_prefix):
        logger = env.get_logger()
        logger.log_debug('Entering %s.invoke_method(): %s' %(self.__class__.__name__,methodName))

        print ("Entering %s.invoke_method(): %s" %(self.__class__.__name__,methodName))
        result = (param_prefix + " " + object_name['Name'])
        out_params = {}
        print result
        return (result, out_params)

## end of class Python_Sample_ClassProvider

def get_providers(env): 
    python_sample_class_prov = Python_Sample_ClassProvider(env)
    return {'Python_Sample_Class': python_sample_class_prov, }
    python_sample_class_2_prov = Python_Sample_Class_2Provider(env)
    return {'Python_Sample_Class': python_sample_class_prov, 'Python_Sample_Class_2': python_sample_class_prov }
