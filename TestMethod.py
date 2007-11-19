"""Python Provider for TestMethod

Instruments the CIM class TestMethod

"""

import pywbem
import random

g_insts = {}

class TestMethodProvider(pywbem.CIMProvider):
    """Instrument the CIM class TestMethod 

    Class with several methods to test method provider capabilities.
    
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
            inst = g_insts[model['id']]
        except KeyError:
            raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND)
        
        model.update_existing(p_sint32=inst[1])
        model.update_existing(p_str=inst[0])
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

        for key in g_insts.keys():
            model['id'] = key
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
        if previous_instance is not None:
            if instance['id'] not in g_insts:
                raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND)
        else:
            g_insts[instance['id']] = [None,None]
        try:
            s = instance['p_str']
        except KeyError:
            s = g_insts[instance['id']][0]
        try: 
            i = instance['p_sint32']
        except KeyError:
            i = g_insts[instance['id']][1]
        g_insts[instance['id']] = [s,i]
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
            del g_insts[instance_name['id']]
        except KeyError:
            raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND)
        
    def cim_method_getobjectpathof(self, env, object_name, method,
                                   param_myref):
        """Implements TestMethod.getObjectPathOf()

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        object_name -- A pywbem.CIMInstanceName or pywbem.CIMCLassName 
            specifying the object on which the method getObjectPathOf() 
            should be invoked.
        method -- A pywbem.CIMMethod representing the method meta-data
        param_myref --  The input parameter myref (type REF (pywbem.CIMInstanceName(classname='CIM_Component', ...)) (Required)

        Returns a two-tuple containing the return value (type unicode)
        and a dictionary with the out-parameters

        Output parameters:
        success -- (type bool) 

        Possible Errors:
        CIM_ERR_ACCESS_DENIED
        CIM_ERR_INVALID_PARAMETER (including missing, duplicate, 
            unrecognized or otherwise incorrect parameters)
        CIM_ERR_NOT_FOUND (the target CIM Class or instance does not 
            exist in the specified namespace)
        CIM_ERR_METHOD_NOT_AVAILABLE (the CIM Server is unable to honor 
            the invocation request)
        CIM_ERR_FAILED (some other unspecified error occurred)

        """

        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_getobjectpathof()' \
                % self.__class__.__name__)

        # TODO do something
        raise pywbem.CIMError(pywbem.CIM_ERR_METHOD_NOT_AVAILABLE) # Remove to implemented
        out_params = {}
        #out_params['success'] = # TODO (type bool)
        rval = None # TODO (type unicode)
        return (rval, out_params)
        
    def cim_method_mkunichar(self, env, object_name, method,
                             param_c):
        """Implements TestMethod.mkUniChar()

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        object_name -- A pywbem.CIMInstanceName or pywbem.CIMCLassName 
            specifying the object on which the method mkUniChar() 
            should be invoked.
        method -- A pywbem.CIMMethod representing the method meta-data
        param_c --  The input parameter c (type pywbem.Sint8) (Required)

        Returns a two-tuple containing the return value (type pywbem.Char16)
        and a dictionary with the out-parameters

        Output parameters: none

        Possible Errors:
        CIM_ERR_ACCESS_DENIED
        CIM_ERR_INVALID_PARAMETER (including missing, duplicate, 
            unrecognized or otherwise incorrect parameters)
        CIM_ERR_NOT_FOUND (the target CIM Class or instance does not 
            exist in the specified namespace)
        CIM_ERR_METHOD_NOT_AVAILABLE (the CIM Server is unable to honor 
            the invocation request)
        CIM_ERR_FAILED (some other unspecified error occurred)

        """

        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_mkunichar()' \
                % self.__class__.__name__)

        # TODO do something
        raise pywbem.CIMError(pywbem.CIM_ERR_METHOD_NOT_AVAILABLE) # Remove to implemented
        out_params = {}
        rval = None # TODO (type pywbem.Char16)
        return (rval, out_params)
        
    def cim_method_genrandlist_sint16(self, env, object_name, method,
                                      param_nelems,
                                      param_lo,
                                      param_hi):
        """Implements TestMethod.genRandList_sint16()

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        object_name -- A pywbem.CIMInstanceName or pywbem.CIMCLassName 
            specifying the object on which the method genRandList_sint16() 
            should be invoked.
        method -- A pywbem.CIMMethod representing the method meta-data
        param_nelems --  The input parameter nelems (type pywbem.Sint32) (Required)
        param_lo --  The input parameter lo (type pywbem.Sint16) (Required)
        param_hi --  The input parameter hi (type pywbem.Sint16) (Required)

        Returns a two-tuple containing the return value (type bool)
        and a dictionary with the out-parameters

        Output parameters:
        lo -- (type pywbem.Sint16) (Required)
        hi -- (type pywbem.Sint16) (Required)
        nlist -- (type [pywbem.Sint16,]) 

        Possible Errors:
        CIM_ERR_ACCESS_DENIED
        CIM_ERR_INVALID_PARAMETER (including missing, duplicate, 
            unrecognized or otherwise incorrect parameters)
        CIM_ERR_NOT_FOUND (the target CIM Class or instance does not 
            exist in the specified namespace)
        CIM_ERR_METHOD_NOT_AVAILABLE (the CIM Server is unable to honor 
            the invocation request)
        CIM_ERR_FAILED (some other unspecified error occurred)

        """

        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_genrandlist_sint16()' \
                % self.__class__.__name__)

        cnt = 0
        l = []
        while cnt < param_nelems:
            cnt+= 1
            l.append(pywbem.Sint16(random.randint(param_lo, param_hi)))
        out_params = {}
        out_params['lo'] = param_lo
        out_params['hi'] = param_hi
        out_params['nlist'] = l
        rval = True
        return (rval, out_params)
        
    def cim_method_mkunistr_char16(self, env, object_name, method,
                                   param_carr):
        """Implements TestMethod.mkUniStr_char16()

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        object_name -- A pywbem.CIMInstanceName or pywbem.CIMCLassName 
            specifying the object on which the method mkUniStr_char16() 
            should be invoked.
        method -- A pywbem.CIMMethod representing the method meta-data
        param_carr --  The input parameter cArr (type [pywbem.Char16,]) (Required)

        Returns a two-tuple containing the return value (type unicode)
        and a dictionary with the out-parameters

        Output parameters: none

        Possible Errors:
        CIM_ERR_ACCESS_DENIED
        CIM_ERR_INVALID_PARAMETER (including missing, duplicate, 
            unrecognized or otherwise incorrect parameters)
        CIM_ERR_NOT_FOUND (the target CIM Class or instance does not 
            exist in the specified namespace)
        CIM_ERR_METHOD_NOT_AVAILABLE (the CIM Server is unable to honor 
            the invocation request)
        CIM_ERR_FAILED (some other unspecified error occurred)

        """

        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_mkunistr_char16()' \
                % self.__class__.__name__)

        # TODO do something
        raise pywbem.CIMError(pywbem.CIM_ERR_METHOD_NOT_AVAILABLE) # Remove to implemented
        out_params = {}
        rval = None # TODO (type unicode)
        return (rval, out_params)
        
    def cim_method_genrandlist_sint32(self, env, object_name, method,
                                      param_nelems,
                                      param_lo,
                                      param_hi):
        """Implements TestMethod.genRandList_sint32()

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        object_name -- A pywbem.CIMInstanceName or pywbem.CIMCLassName 
            specifying the object on which the method genRandList_sint32() 
            should be invoked.
        method -- A pywbem.CIMMethod representing the method meta-data
        param_nelems --  The input parameter nelems (type pywbem.Sint32) (Required)
        param_lo --  The input parameter lo (type pywbem.Sint32) (Required)
        param_hi --  The input parameter hi (type pywbem.Sint32) (Required)

        Returns a two-tuple containing the return value (type bool)
        and a dictionary with the out-parameters

        Output parameters:
        lo -- (type pywbem.Sint32) (Required)
        hi -- (type pywbem.Sint32) (Required)
        nlist -- (type [pywbem.Sint32,]) 

        Possible Errors:
        CIM_ERR_ACCESS_DENIED
        CIM_ERR_INVALID_PARAMETER (including missing, duplicate, 
            unrecognized or otherwise incorrect parameters)
        CIM_ERR_NOT_FOUND (the target CIM Class or instance does not 
            exist in the specified namespace)
        CIM_ERR_METHOD_NOT_AVAILABLE (the CIM Server is unable to honor 
            the invocation request)
        CIM_ERR_FAILED (some other unspecified error occurred)

        """

        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_genrandlist_sint32()' \
                % self.__class__.__name__)

        l = []
        cnt = 0
        while cnt < param_nelems:
            cnt+= 1
            l.append(pywbem.Sint32(random.randint(param_lo, param_hi)))
        out_params = {}
        out_params['lo'] = param_lo
        out_params['hi'] = param_hi
        out_params['nlist'] = l
        rval = True
        return (rval, out_params)
        
    def cim_method_strsplit(self, env, object_name, method,
                            param_str,
                            param_sep):
        """Implements TestMethod.strSplit()

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        object_name -- A pywbem.CIMInstanceName or pywbem.CIMCLassName 
            specifying the object on which the method strSplit() 
            should be invoked.
        method -- A pywbem.CIMMethod representing the method meta-data
        param_str --  The input parameter str (type unicode) (Required)
        param_sep --  The input parameter sep (type unicode) 

        Returns a two-tuple containing the return value (type bool)
        and a dictionary with the out-parameters

        Output parameters:
        nelems -- (type pywbem.Sint32) 
        elems -- (type [unicode,]) 

        Possible Errors:
        CIM_ERR_ACCESS_DENIED
        CIM_ERR_INVALID_PARAMETER (including missing, duplicate, 
            unrecognized or otherwise incorrect parameters)
        CIM_ERR_NOT_FOUND (the target CIM Class or instance does not 
            exist in the specified namespace)
        CIM_ERR_METHOD_NOT_AVAILABLE (the CIM Server is unable to honor 
            the invocation request)
        CIM_ERR_FAILED (some other unspecified error occurred)

        """

        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_strsplit()' \
                % self.__class__.__name__)

        elems = param_str.split(param_sep)
        out_params = {}
        out_params['nelems'] = pywbem.Sint32(len(elems))
        out_params['elems'] = elems
        rval = True
        return (rval, out_params)
        
    def cim_method_setstrprop(self, env, object_name, method,
                              param_value):
        """Implements TestMethod.setStrProp()

        Set the value of p_str.  Return the previous value of p_str.
        
        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        object_name -- A pywbem.CIMInstanceName or pywbem.CIMCLassName 
            specifying the object on which the method setStrProp() 
            should be invoked.
        method -- A pywbem.CIMMethod representing the method meta-data
        param_value --  The input parameter value (type unicode) (Required)

        Returns a two-tuple containing the return value (type unicode)
        and a dictionary with the out-parameters

        Output parameters: none

        Possible Errors:
        CIM_ERR_ACCESS_DENIED
        CIM_ERR_INVALID_PARAMETER (including missing, duplicate, 
            unrecognized or otherwise incorrect parameters)
        CIM_ERR_NOT_FOUND (the target CIM Class or instance does not 
            exist in the specified namespace)
        CIM_ERR_METHOD_NOT_AVAILABLE (the CIM Server is unable to honor 
            the invocation request)
        CIM_ERR_FAILED (some other unspecified error occurred)

        """

        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_setstrprop()' \
                % self.__class__.__name__)

        if not hasattr(object_name, 'keybindings'):
            raise pywbem.CIMError(pywbem.CIM_ERR_INVALID_PARAMETER)
        try:
            inst = g_insts[object_name['id']]
        except KeyError:
            raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND)

        rval = inst[0]
        inst[0] = param_value
        out_params = {}
        return (rval, out_params)
        
    def cim_method_strcat(self, env, object_name, method,
                          param_strs,
                          param_sep):
        """Implements TestMethod.strCat()

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        object_name -- A pywbem.CIMInstanceName or pywbem.CIMCLassName 
            specifying the object on which the method strCat() 
            should be invoked.
        method -- A pywbem.CIMMethod representing the method meta-data
        param_strs --  The input parameter strs (type [unicode,]) (Required)
        param_sep --  The input parameter sep (type unicode) 

        Returns a two-tuple containing the return value (type unicode)
        and a dictionary with the out-parameters

        Output parameters: none

        Possible Errors:
        CIM_ERR_ACCESS_DENIED
        CIM_ERR_INVALID_PARAMETER (including missing, duplicate, 
            unrecognized or otherwise incorrect parameters)
        CIM_ERR_NOT_FOUND (the target CIM Class or instance does not 
            exist in the specified namespace)
        CIM_ERR_METHOD_NOT_AVAILABLE (the CIM Server is unable to honor 
            the invocation request)
        CIM_ERR_FAILED (some other unspecified error occurred)

        """

        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_strcat()' \
                % self.__class__.__name__)

        out_params = {}
        rval = param_sep.join(param_strs)
        return (rval, out_params)
        
    def cim_method_genrand_uint16(self, env, object_name, method,
                                  param_max,
                                  param_min):
        """Implements TestMethod.genRand_uint16()

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        object_name -- A pywbem.CIMInstanceName or pywbem.CIMCLassName 
            specifying the object on which the method genRand_uint16() 
            should be invoked.
        method -- A pywbem.CIMMethod representing the method meta-data
        param_max --  The input parameter max (type pywbem.Uint16) 
        param_min --  The input parameter min (type pywbem.Uint16) 

        Returns a two-tuple containing the return value (type pywbem.Uint16)
        and a dictionary with the out-parameters

        Output parameters:
        success -- (type bool) 

        Possible Errors:
        CIM_ERR_ACCESS_DENIED
        CIM_ERR_INVALID_PARAMETER (including missing, duplicate, 
            unrecognized or otherwise incorrect parameters)
        CIM_ERR_NOT_FOUND (the target CIM Class or instance does not 
            exist in the specified namespace)
        CIM_ERR_METHOD_NOT_AVAILABLE (the CIM Server is unable to honor 
            the invocation request)
        CIM_ERR_FAILED (some other unspecified error occurred)

        """

        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_genrand_uint16()' \
                % self.__class__.__name__)

        
        out_params = {}
        out_params['success'] = True
        rval = pywbem.Uint16(random.randint(param_min, param_max))
        return (rval, out_params)
        
    def cim_method_minmedmax_sint8(self, env, object_name, method,
                                   param_numlist):
        """Implements TestMethod.minmedmax_sint8()

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        object_name -- A pywbem.CIMInstanceName or pywbem.CIMCLassName 
            specifying the object on which the method minmedmax_sint8() 
            should be invoked.
        method -- A pywbem.CIMMethod representing the method meta-data
        param_numlist --  The input parameter numlist (type [pywbem.Sint8,]) (Required)

        Returns a two-tuple containing the return value (type bool)
        and a dictionary with the out-parameters

        Output parameters:
        max -- (type pywbem.Sint8) 
        min -- (type pywbem.Sint8) 
        med -- (type pywbem.Sint8) 

        Possible Errors:
        CIM_ERR_ACCESS_DENIED
        CIM_ERR_INVALID_PARAMETER (including missing, duplicate, 
            unrecognized or otherwise incorrect parameters)
        CIM_ERR_NOT_FOUND (the target CIM Class or instance does not 
            exist in the specified namespace)
        CIM_ERR_METHOD_NOT_AVAILABLE (the CIM Server is unable to honor 
            the invocation request)
        CIM_ERR_FAILED (some other unspecified error occurred)

        """

        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_minmedmax_sint8()' \
                % self.__class__.__name__)

        l = param_numlist
        l.sort()
        out_params = {}
        out_params['max'] = l[-1]
        out_params['min'] = l[0]
        ln = len(l)
        if ln % 2 == 0:
            out_params['med'] = pywbem.Sint8(
                    (l[(ln / 2) - 1] + l[(ln / 2)]) / 2)
        else:
            out_params['med'] = pywbem.Sint8(l[ln / 2])
        rval = True
        return (rval, out_params)
        
    def cim_method_genrandlist_uint64(self, env, object_name, method,
                                      param_nelems,
                                      param_lo,
                                      param_hi):
        """Implements TestMethod.genRandList_uint64()

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        object_name -- A pywbem.CIMInstanceName or pywbem.CIMCLassName 
            specifying the object on which the method genRandList_uint64() 
            should be invoked.
        method -- A pywbem.CIMMethod representing the method meta-data
        param_nelems --  The input parameter nelems (type pywbem.Sint32) (Required)
        param_lo --  The input parameter lo (type pywbem.Uint64) (Required)
        param_hi --  The input parameter hi (type pywbem.Uint64) (Required)

        Returns a two-tuple containing the return value (type bool)
        and a dictionary with the out-parameters

        Output parameters:
        lo -- (type pywbem.Uint64) (Required)
        hi -- (type pywbem.Uint64) (Required)
        nlist -- (type [pywbem.Uint64,]) 

        Possible Errors:
        CIM_ERR_ACCESS_DENIED
        CIM_ERR_INVALID_PARAMETER (including missing, duplicate, 
            unrecognized or otherwise incorrect parameters)
        CIM_ERR_NOT_FOUND (the target CIM Class or instance does not 
            exist in the specified namespace)
        CIM_ERR_METHOD_NOT_AVAILABLE (the CIM Server is unable to honor 
            the invocation request)
        CIM_ERR_FAILED (some other unspecified error occurred)

        """

        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_genrandlist_uint64()' \
                % self.__class__.__name__)

        cnt = 0
        l = []
        while cnt < param_nelems:
            cnt+= 1
            l.append(pywbem.Uint64(random.randint(param_lo, param_hi)))
        out_params = {}
        out_params['lo'] = param_lo
        out_params['hi'] = param_hi
        out_params['nlist'] = l
        rval = True
        return (rval, out_params)
        
    def cim_method_genrand_uint8(self, env, object_name, method,
                                 param_max,
                                 param_min):
        """Implements TestMethod.genRand_uint8()

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        object_name -- A pywbem.CIMInstanceName or pywbem.CIMCLassName 
            specifying the object on which the method genRand_uint8() 
            should be invoked.
        method -- A pywbem.CIMMethod representing the method meta-data
        param_max --  The input parameter max (type pywbem.Uint8) 
        param_min --  The input parameter min (type pywbem.Uint8) 

        Returns a two-tuple containing the return value (type pywbem.Uint8)
        and a dictionary with the out-parameters

        Output parameters:
        success -- (type bool) 

        Possible Errors:
        CIM_ERR_ACCESS_DENIED
        CIM_ERR_INVALID_PARAMETER (including missing, duplicate, 
            unrecognized or otherwise incorrect parameters)
        CIM_ERR_NOT_FOUND (the target CIM Class or instance does not 
            exist in the specified namespace)
        CIM_ERR_METHOD_NOT_AVAILABLE (the CIM Server is unable to honor 
            the invocation request)
        CIM_ERR_FAILED (some other unspecified error occurred)

        """

        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_genrand_uint8()' \
                % self.__class__.__name__)

        out_params = {}
        out_params['success'] = True
        rval = pywbem.Uint8(random.randint(param_min, param_max))
        return (rval, out_params)
        
    def cim_method_genrand_real32(self, env, object_name, method,
                                  param_max,
                                  param_min):
        """Implements TestMethod.genRand_real32()

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        object_name -- A pywbem.CIMInstanceName or pywbem.CIMCLassName 
            specifying the object on which the method genRand_real32() 
            should be invoked.
        method -- A pywbem.CIMMethod representing the method meta-data
        param_max --  The input parameter max (type pywbem.Real32) 
        param_min --  The input parameter min (type pywbem.Real32) 

        Returns a two-tuple containing the return value (type pywbem.Real32)
        and a dictionary with the out-parameters

        Output parameters:
        success -- (type bool) 

        Possible Errors:
        CIM_ERR_ACCESS_DENIED
        CIM_ERR_INVALID_PARAMETER (including missing, duplicate, 
            unrecognized or otherwise incorrect parameters)
        CIM_ERR_NOT_FOUND (the target CIM Class or instance does not 
            exist in the specified namespace)
        CIM_ERR_METHOD_NOT_AVAILABLE (the CIM Server is unable to honor 
            the invocation request)
        CIM_ERR_FAILED (some other unspecified error occurred)

        """

        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_genrand_real32()' \
                % self.__class__.__name__)

        out_params = {}
        out_params['success'] = True
        range = param_max - param_min
        rval = pywbem.Real32(random.random() * range + param_min)
        return (rval, out_params)
        
    def cim_method_genrand_sint64(self, env, object_name, method,
                                  param_max,
                                  param_min):
        """Implements TestMethod.genRand_sint64()

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        object_name -- A pywbem.CIMInstanceName or pywbem.CIMCLassName 
            specifying the object on which the method genRand_sint64() 
            should be invoked.
        method -- A pywbem.CIMMethod representing the method meta-data
        param_max --  The input parameter max (type pywbem.Sint64) 
        param_min --  The input parameter min (type pywbem.Sint64) 

        Returns a two-tuple containing the return value (type pywbem.Sint64)
        and a dictionary with the out-parameters

        Output parameters:
        success -- (type bool) 

        Possible Errors:
        CIM_ERR_ACCESS_DENIED
        CIM_ERR_INVALID_PARAMETER (including missing, duplicate, 
            unrecognized or otherwise incorrect parameters)
        CIM_ERR_NOT_FOUND (the target CIM Class or instance does not 
            exist in the specified namespace)
        CIM_ERR_METHOD_NOT_AVAILABLE (the CIM Server is unable to honor 
            the invocation request)
        CIM_ERR_FAILED (some other unspecified error occurred)

        """

        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_genrand_sint64()' \
                % self.__class__.__name__)

        # TODO do something
        out_params = {}
        out_params['success'] = True
        range = param_max - param_min
        rval = pywbem.Real64(random.random() * range + param_min)
        return (rval, out_params)
        
    def cim_method_minmedmax_sint32(self, env, object_name, method,
                                    param_numlist):
        """Implements TestMethod.minmedmax_sint32()

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        object_name -- A pywbem.CIMInstanceName or pywbem.CIMCLassName 
            specifying the object on which the method minmedmax_sint32() 
            should be invoked.
        method -- A pywbem.CIMMethod representing the method meta-data
        param_numlist --  The input parameter numlist (type [pywbem.Sint32,]) (Required)

        Returns a two-tuple containing the return value (type bool)
        and a dictionary with the out-parameters

        Output parameters:
        max -- (type pywbem.Sint32) 
        min -- (type pywbem.Sint32) 
        med -- (type pywbem.Sint32) 

        Possible Errors:
        CIM_ERR_ACCESS_DENIED
        CIM_ERR_INVALID_PARAMETER (including missing, duplicate, 
            unrecognized or otherwise incorrect parameters)
        CIM_ERR_NOT_FOUND (the target CIM Class or instance does not 
            exist in the specified namespace)
        CIM_ERR_METHOD_NOT_AVAILABLE (the CIM Server is unable to honor 
            the invocation request)
        CIM_ERR_FAILED (some other unspecified error occurred)

        """

        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_minmedmax_sint32()' \
                % self.__class__.__name__)

        l = param_numlist
        l.sort()
        out_params = {}
        out_params['max'] = l[-1]
        out_params['min'] = l[0]
        ln = len(l)
        if ln % 2 == 0:
            out_params['med'] = pywbem.Sint32(
                    (l[(ln / 2) - 1] + l[(ln / 2)]) / 2)
        else:
            out_params['med'] = pywbem.Sint32(l[ln / 2])
        rval = True
        return (rval, out_params)
        
    def cim_method_getintprop(self, env, object_name, method):
        """Implements TestMethod.getIntProp()

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        object_name -- A pywbem.CIMInstanceName or pywbem.CIMCLassName 
            specifying the object on which the method getIntProp() 
            should be invoked.
        method -- A pywbem.CIMMethod representing the method meta-data

        Returns a two-tuple containing the return value (type pywbem.Sint32)
        and a dictionary with the out-parameters

        Output parameters: none

        Possible Errors:
        CIM_ERR_ACCESS_DENIED
        CIM_ERR_INVALID_PARAMETER (including missing, duplicate, 
            unrecognized or otherwise incorrect parameters)
        CIM_ERR_NOT_FOUND (the target CIM Class or instance does not 
            exist in the specified namespace)
        CIM_ERR_METHOD_NOT_AVAILABLE (the CIM Server is unable to honor 
            the invocation request)
        CIM_ERR_FAILED (some other unspecified error occurred)

        """

        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_getintprop()' \
                % self.__class__.__name__)

        if not hasattr(object_name, 'keybindings'):
            raise pywbem.CIMError(pywbem.CIM_ERR_INVALID_PARAMETER)
        try:
            inst = g_insts[object_name['id']]
        except KeyError:
            raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND)
        out_params = {}
        rval = inst[1]
        return (rval, out_params)
        
    def cim_method_genrandlist_uint8(self, env, object_name, method,
                                     param_nelems,
                                     param_lo,
                                     param_hi):
        """Implements TestMethod.genRandList_uint8()

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        object_name -- A pywbem.CIMInstanceName or pywbem.CIMCLassName 
            specifying the object on which the method genRandList_uint8() 
            should be invoked.
        method -- A pywbem.CIMMethod representing the method meta-data
        param_nelems --  The input parameter nelems (type pywbem.Sint32) (Required)
        param_lo --  The input parameter lo (type pywbem.Uint8) (Required)
        param_hi --  The input parameter hi (type pywbem.Uint8) (Required)

        Returns a two-tuple containing the return value (type bool)
        and a dictionary with the out-parameters

        Output parameters:
        lo -- (type pywbem.Uint8) (Required)
        hi -- (type pywbem.Uint8) (Required)
        nlist -- (type [pywbem.Uint8,]) 

        Possible Errors:
        CIM_ERR_ACCESS_DENIED
        CIM_ERR_INVALID_PARAMETER (including missing, duplicate, 
            unrecognized or otherwise incorrect parameters)
        CIM_ERR_NOT_FOUND (the target CIM Class or instance does not 
            exist in the specified namespace)
        CIM_ERR_METHOD_NOT_AVAILABLE (the CIM Server is unable to honor 
            the invocation request)
        CIM_ERR_FAILED (some other unspecified error occurred)

        """

        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_genrandlist_uint8()' \
                % self.__class__.__name__)

        cnt = 0
        l = []
        while cnt < param_nelems:
            cnt+= 1
            l.append(pywbem.Uint8(random.randint(param_lo, param_hi)))
        out_params = {}
        out_params['lo'] = param_lo
        out_params['hi'] = param_hi
        out_params['nlist'] = l
        rval = True
        return (rval, out_params)
        
    def cim_method_genrand_uint32(self, env, object_name, method,
                                  param_max,
                                  param_min):
        """Implements TestMethod.genRand_uint32()

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        object_name -- A pywbem.CIMInstanceName or pywbem.CIMCLassName 
            specifying the object on which the method genRand_uint32() 
            should be invoked.
        method -- A pywbem.CIMMethod representing the method meta-data
        param_max --  The input parameter max (type pywbem.Uint32) 
        param_min --  The input parameter min (type pywbem.Uint32) 

        Returns a two-tuple containing the return value (type pywbem.Uint32)
        and a dictionary with the out-parameters

        Output parameters:
        success -- (type bool) 

        Possible Errors:
        CIM_ERR_ACCESS_DENIED
        CIM_ERR_INVALID_PARAMETER (including missing, duplicate, 
            unrecognized or otherwise incorrect parameters)
        CIM_ERR_NOT_FOUND (the target CIM Class or instance does not 
            exist in the specified namespace)
        CIM_ERR_METHOD_NOT_AVAILABLE (the CIM Server is unable to honor 
            the invocation request)
        CIM_ERR_FAILED (some other unspecified error occurred)

        """

        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_genrand_uint32()' \
                % self.__class__.__name__)

        out_params = {}
        out_params['success'] = True
        rval = pywbem.Uint32(random.randint(param_min, param_max))
        return (rval, out_params)
        
    def cim_method_minmedmax_uint64(self, env, object_name, method,
                                    param_numlist):
        """Implements TestMethod.minmedmax_uint64()

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        object_name -- A pywbem.CIMInstanceName or pywbem.CIMCLassName 
            specifying the object on which the method minmedmax_uint64() 
            should be invoked.
        method -- A pywbem.CIMMethod representing the method meta-data
        param_numlist --  The input parameter numlist (type [pywbem.Uint64,]) (Required)

        Returns a two-tuple containing the return value (type bool)
        and a dictionary with the out-parameters

        Output parameters:
        max -- (type pywbem.Uint64) 
        min -- (type pywbem.Uint64) 
        med -- (type pywbem.Uint64) 

        Possible Errors:
        CIM_ERR_ACCESS_DENIED
        CIM_ERR_INVALID_PARAMETER (including missing, duplicate, 
            unrecognized or otherwise incorrect parameters)
        CIM_ERR_NOT_FOUND (the target CIM Class or instance does not 
            exist in the specified namespace)
        CIM_ERR_METHOD_NOT_AVAILABLE (the CIM Server is unable to honor 
            the invocation request)
        CIM_ERR_FAILED (some other unspecified error occurred)

        """

        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_minmedmax_uint64()' \
                % self.__class__.__name__)

        l = param_numlist
        l.sort()
        out_params = {}
        out_params['max'] = l[-1]
        out_params['min'] = l[0]
        ln = len(l)
        if ln % 2 == 0:
            out_params['med'] = pywbem.Uint64(
                    (l[(ln / 2) - 1] + l[(ln / 2)]) / 2)
        else:
            out_params['med'] = pywbem.Uint64(l[ln / 2])
        rval = True
        return (rval, out_params)
        
    def cim_method_genrandlist_real32(self, env, object_name, method,
                                      param_nelems,
                                      param_lo,
                                      param_hi):
        """Implements TestMethod.genRandList_real32()

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        object_name -- A pywbem.CIMInstanceName or pywbem.CIMCLassName 
            specifying the object on which the method genRandList_real32() 
            should be invoked.
        method -- A pywbem.CIMMethod representing the method meta-data
        param_nelems --  The input parameter nelems (type pywbem.Sint32) (Required)
        param_lo --  The input parameter lo (type pywbem.Real32) (Required)
        param_hi --  The input parameter hi (type pywbem.Real32) (Required)

        Returns a two-tuple containing the return value (type bool)
        and a dictionary with the out-parameters

        Output parameters:
        lo -- (type pywbem.Real32) (Required)
        hi -- (type pywbem.Real32) (Required)
        nlist -- (type [pywbem.Real32,]) 

        Possible Errors:
        CIM_ERR_ACCESS_DENIED
        CIM_ERR_INVALID_PARAMETER (including missing, duplicate, 
            unrecognized or otherwise incorrect parameters)
        CIM_ERR_NOT_FOUND (the target CIM Class or instance does not 
            exist in the specified namespace)
        CIM_ERR_METHOD_NOT_AVAILABLE (the CIM Server is unable to honor 
            the invocation request)
        CIM_ERR_FAILED (some other unspecified error occurred)

        """

        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_genrandlist_real32()' \
                % self.__class__.__name__)

        cnt = 0
        l = []
        range = param_hi - param_lo
        while cnt < param_nelems:
            cnt+= 1
            l.append(pywbem.Real32(
                random.random() * range + param_lo))
        out_params = {}
        out_params['lo'] = param_lo
        out_params['hi'] = param_hi
        out_params['nlist'] = l
        rval = True
        return (rval, out_params)
        
    def cim_method_genrand_sint16(self, env, object_name, method,
                                  param_max,
                                  param_min):
        """Implements TestMethod.genRand_sint16()

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        object_name -- A pywbem.CIMInstanceName or pywbem.CIMCLassName 
            specifying the object on which the method genRand_sint16() 
            should be invoked.
        method -- A pywbem.CIMMethod representing the method meta-data
        param_max --  The input parameter max (type pywbem.Sint16) 
        param_min --  The input parameter min (type pywbem.Sint16) 

        Returns a two-tuple containing the return value (type pywbem.Sint16)
        and a dictionary with the out-parameters

        Output parameters:
        success -- (type bool) 

        Possible Errors:
        CIM_ERR_ACCESS_DENIED
        CIM_ERR_INVALID_PARAMETER (including missing, duplicate, 
            unrecognized or otherwise incorrect parameters)
        CIM_ERR_NOT_FOUND (the target CIM Class or instance does not 
            exist in the specified namespace)
        CIM_ERR_METHOD_NOT_AVAILABLE (the CIM Server is unable to honor 
            the invocation request)
        CIM_ERR_FAILED (some other unspecified error occurred)

        """

        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_genrand_sint16()' \
                % self.__class__.__name__)

        out_params = {}
        out_params['success'] = True
        rval = pywbem.Sint16(random.randint(param_min, param_max))
        return (rval, out_params)
        
    def cim_method_getinstanceof(self, env, object_name, method,
                                 param_objpath):
        """Implements TestMethod.getInstanceOf()

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        object_name -- A pywbem.CIMInstanceName or pywbem.CIMCLassName 
            specifying the object on which the method getInstanceOf() 
            should be invoked.
        method -- A pywbem.CIMMethod representing the method meta-data
        param_objpath --  The input parameter objPath (type unicode) (Required)

        Returns a two-tuple containing the return value (type bool)
        and a dictionary with the out-parameters

        Output parameters:
        inst -- (type REF (pywbem.CIMInstanceName(classname='CIM_Component', ...)) 

        Possible Errors:
        CIM_ERR_ACCESS_DENIED
        CIM_ERR_INVALID_PARAMETER (including missing, duplicate, 
            unrecognized or otherwise incorrect parameters)
        CIM_ERR_NOT_FOUND (the target CIM Class or instance does not 
            exist in the specified namespace)
        CIM_ERR_METHOD_NOT_AVAILABLE (the CIM Server is unable to honor 
            the invocation request)
        CIM_ERR_FAILED (some other unspecified error occurred)

        """

        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_getinstanceof()' \
                % self.__class__.__name__)

        # TODO do something
        raise pywbem.CIMError(pywbem.CIM_ERR_METHOD_NOT_AVAILABLE) # Remove to implemented
        out_params = {}
        #out_params['inst'] = # TODO (type REF (pywbem.CIMInstanceName(classname='CIM_Component', ...))
        rval = None # TODO (type bool)
        return (rval, out_params)
        
    def cim_method_genrand_sint32(self, env, object_name, method,
                                  param_max,
                                  param_min):
        """Implements TestMethod.genRand_sint32()

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        object_name -- A pywbem.CIMInstanceName or pywbem.CIMCLassName 
            specifying the object on which the method genRand_sint32() 
            should be invoked.
        method -- A pywbem.CIMMethod representing the method meta-data
        param_max --  The input parameter max (type pywbem.Sint32) 
        param_min --  The input parameter min (type pywbem.Sint32) 

        Returns a two-tuple containing the return value (type pywbem.Sint32)
        and a dictionary with the out-parameters

        Output parameters:
        success -- (type bool) 

        Possible Errors:
        CIM_ERR_ACCESS_DENIED
        CIM_ERR_INVALID_PARAMETER (including missing, duplicate, 
            unrecognized or otherwise incorrect parameters)
        CIM_ERR_NOT_FOUND (the target CIM Class or instance does not 
            exist in the specified namespace)
        CIM_ERR_METHOD_NOT_AVAILABLE (the CIM Server is unable to honor 
            the invocation request)
        CIM_ERR_FAILED (some other unspecified error occurred)

        """

        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_genrand_sint32()' \
                % self.__class__.__name__)

        out_params = {}
        out_params['success'] = True
        rval = pywbem.Sint32(random.randint(param_min, param_max))
        return (rval, out_params)
        
    def cim_method_genrandlist_sint8(self, env, object_name, method,
                                     param_nelems,
                                     param_lo,
                                     param_hi):
        """Implements TestMethod.genRandList_sint8()

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        object_name -- A pywbem.CIMInstanceName or pywbem.CIMCLassName 
            specifying the object on which the method genRandList_sint8() 
            should be invoked.
        method -- A pywbem.CIMMethod representing the method meta-data
        param_nelems --  The input parameter nelems (type pywbem.Sint32) (Required)
        param_lo --  The input parameter lo (type pywbem.Sint8) (Required)
        param_hi --  The input parameter hi (type pywbem.Sint8) (Required)

        Returns a two-tuple containing the return value (type bool)
        and a dictionary with the out-parameters

        Output parameters:
        lo -- (type pywbem.Sint8) (Required)
        hi -- (type pywbem.Sint8) (Required)
        nlist -- (type [pywbem.Sint8,]) 

        Possible Errors:
        CIM_ERR_ACCESS_DENIED
        CIM_ERR_INVALID_PARAMETER (including missing, duplicate, 
            unrecognized or otherwise incorrect parameters)
        CIM_ERR_NOT_FOUND (the target CIM Class or instance does not 
            exist in the specified namespace)
        CIM_ERR_METHOD_NOT_AVAILABLE (the CIM Server is unable to honor 
            the invocation request)
        CIM_ERR_FAILED (some other unspecified error occurred)

        """

        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_genrandlist_sint8()' \
                % self.__class__.__name__)

        cnt = 0
        l = []
        while cnt < param_nelems:
            cnt+= 1
            l.append(pywbem.Sint8(random.randint(param_lo, param_hi)))
        out_params = {}
        out_params['lo'] = param_lo
        out_params['hi'] = param_hi
        out_params['nlist'] = l
        rval = True
        return (rval, out_params)
        
    def cim_method_minmedmax_real32(self, env, object_name, method,
                                    param_numlist):
        """Implements TestMethod.minmedmax_real32()

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        object_name -- A pywbem.CIMInstanceName or pywbem.CIMCLassName 
            specifying the object on which the method minmedmax_real32() 
            should be invoked.
        method -- A pywbem.CIMMethod representing the method meta-data
        param_numlist --  The input parameter numlist (type [pywbem.Real32,]) (Required)

        Returns a two-tuple containing the return value (type bool)
        and a dictionary with the out-parameters

        Output parameters:
        max -- (type pywbem.Real32) 
        min -- (type pywbem.Real32) 
        med -- (type pywbem.Real32) 

        Possible Errors:
        CIM_ERR_ACCESS_DENIED
        CIM_ERR_INVALID_PARAMETER (including missing, duplicate, 
            unrecognized or otherwise incorrect parameters)
        CIM_ERR_NOT_FOUND (the target CIM Class or instance does not 
            exist in the specified namespace)
        CIM_ERR_METHOD_NOT_AVAILABLE (the CIM Server is unable to honor 
            the invocation request)
        CIM_ERR_FAILED (some other unspecified error occurred)

        """

        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_minmedmax_real32()' \
                % self.__class__.__name__)

        l = param_numlist
        l.sort()
        out_params = {}
        out_params['max'] = l[-1]
        out_params['min'] = l[0]
        ln = len(l)
        if ln % 2 == 0:
            out_params['med'] = pywbem.Real32(
                    (l[(ln / 2) - 1] + l[(ln / 2)]) / 2)
        else:
            out_params['med'] = pywbem.Real32(l[ln / 2])
        rval = True
        return (rval, out_params)
        
    def cim_method_minmedmax_uint8(self, env, object_name, method,
                                   param_numlist):
        """Implements TestMethod.minmedmax_uint8()

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        object_name -- A pywbem.CIMInstanceName or pywbem.CIMCLassName 
            specifying the object on which the method minmedmax_uint8() 
            should be invoked.
        method -- A pywbem.CIMMethod representing the method meta-data
        param_numlist --  The input parameter numlist (type [pywbem.Uint8,]) (Required)

        Returns a two-tuple containing the return value (type bool)
        and a dictionary with the out-parameters

        Output parameters:
        max -- (type pywbem.Uint8) 
        min -- (type pywbem.Uint8) 
        med -- (type pywbem.Uint8) 

        Possible Errors:
        CIM_ERR_ACCESS_DENIED
        CIM_ERR_INVALID_PARAMETER (including missing, duplicate, 
            unrecognized or otherwise incorrect parameters)
        CIM_ERR_NOT_FOUND (the target CIM Class or instance does not 
            exist in the specified namespace)
        CIM_ERR_METHOD_NOT_AVAILABLE (the CIM Server is unable to honor 
            the invocation request)
        CIM_ERR_FAILED (some other unspecified error occurred)

        """

        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_minmedmax_uint8()' \
                % self.__class__.__name__)

        l = param_numlist
        l.sort()
        out_params = {}
        out_params['max'] = l[-1]
        out_params['min'] = l[0]
        ln = len(l)
        if ln % 2 == 0:
            out_params['med'] = pywbem.Uint8(
                    (l[(ln / 2) - 1] + l[(ln / 2)]) / 2)
        else:
            out_params['med'] = pywbem.Uint8(l[ln / 2])
        rval = True
        return (rval, out_params)
        
    def cim_method_mkunichararray(self, env, object_name, method,
                                  param_inarr):
        """Implements TestMethod.mkUniCharArray()

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        object_name -- A pywbem.CIMInstanceName or pywbem.CIMCLassName 
            specifying the object on which the method mkUniCharArray() 
            should be invoked.
        method -- A pywbem.CIMMethod representing the method meta-data
        param_inarr --  The input parameter inArr (type [pywbem.Sint8,]) (Required)

        Returns a two-tuple containing the return value (type bool)
        and a dictionary with the out-parameters

        Output parameters:
        outArr -- (type [pywbem.Char16,]) 

        Possible Errors:
        CIM_ERR_ACCESS_DENIED
        CIM_ERR_INVALID_PARAMETER (including missing, duplicate, 
            unrecognized or otherwise incorrect parameters)
        CIM_ERR_NOT_FOUND (the target CIM Class or instance does not 
            exist in the specified namespace)
        CIM_ERR_METHOD_NOT_AVAILABLE (the CIM Server is unable to honor 
            the invocation request)
        CIM_ERR_FAILED (some other unspecified error occurred)

        """

        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_mkunichararray()' \
                % self.__class__.__name__)

        # TODO do something
        raise pywbem.CIMError(pywbem.CIM_ERR_METHOD_NOT_AVAILABLE) # Remove to implemented
        out_params = {}
        #out_params['outarr'] = # TODO (type [pywbem.Char16,])
        rval = None # TODO (type bool)
        return (rval, out_params)
        
    def cim_method_mkunistr_sint8(self, env, object_name, method,
                                  param_carr):
        """Implements TestMethod.mkUniStr_sint8()

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        object_name -- A pywbem.CIMInstanceName or pywbem.CIMCLassName 
            specifying the object on which the method mkUniStr_sint8() 
            should be invoked.
        method -- A pywbem.CIMMethod representing the method meta-data
        param_carr --  The input parameter cArr (type [pywbem.Sint8,]) (Required)

        Returns a two-tuple containing the return value (type unicode)
        and a dictionary with the out-parameters

        Output parameters: none

        Possible Errors:
        CIM_ERR_ACCESS_DENIED
        CIM_ERR_INVALID_PARAMETER (including missing, duplicate, 
            unrecognized or otherwise incorrect parameters)
        CIM_ERR_NOT_FOUND (the target CIM Class or instance does not 
            exist in the specified namespace)
        CIM_ERR_METHOD_NOT_AVAILABLE (the CIM Server is unable to honor 
            the invocation request)
        CIM_ERR_FAILED (some other unspecified error occurred)

        """

        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_mkunistr_sint8()' \
                % self.__class__.__name__)

        rval = ''
        for i in param_carr:
            rval += char(i)
        out_params = {}
        return (rval, out_params)
        
    def cim_method_minmedmax_real64(self, env, object_name, method,
                                    param_numlist):
        """Implements TestMethod.minmedmax_real64()

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        object_name -- A pywbem.CIMInstanceName or pywbem.CIMCLassName 
            specifying the object on which the method minmedmax_real64() 
            should be invoked.
        method -- A pywbem.CIMMethod representing the method meta-data
        param_numlist --  The input parameter numlist (type [pywbem.Real64,]) (Required)

        Returns a two-tuple containing the return value (type bool)
        and a dictionary with the out-parameters

        Output parameters:
        max -- (type pywbem.Real64) 
        min -- (type pywbem.Real64) 
        med -- (type pywbem.Real64) 

        Possible Errors:
        CIM_ERR_ACCESS_DENIED
        CIM_ERR_INVALID_PARAMETER (including missing, duplicate, 
            unrecognized or otherwise incorrect parameters)
        CIM_ERR_NOT_FOUND (the target CIM Class or instance does not 
            exist in the specified namespace)
        CIM_ERR_METHOD_NOT_AVAILABLE (the CIM Server is unable to honor 
            the invocation request)
        CIM_ERR_FAILED (some other unspecified error occurred)

        """

        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_minmedmax_real64()' \
                % self.__class__.__name__)

        l = param_numlist
        l.sort()
        out_params = {}
        out_params['max'] = l[-1]
        out_params['min'] = l[0]
        ln = len(l)
        if ln % 2 == 0:
            out_params['med'] = pywbem.Real64(
                    (l[(ln / 2) - 1] + l[(ln / 2)]) / 2)
        else:
            out_params['med'] = pywbem.Real64(l[ln / 2])
        rval = True
        return (rval, out_params)
        
    def cim_method_genrandlist_uint32(self, env, object_name, method,
                                      param_nelems,
                                      param_lo,
                                      param_hi):
        """Implements TestMethod.genRandList_uint32()

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        object_name -- A pywbem.CIMInstanceName or pywbem.CIMCLassName 
            specifying the object on which the method genRandList_uint32() 
            should be invoked.
        method -- A pywbem.CIMMethod representing the method meta-data
        param_nelems --  The input parameter nelems (type pywbem.Sint32) (Required)
        param_lo --  The input parameter lo (type pywbem.Uint32) (Required)
        param_hi --  The input parameter hi (type pywbem.Uint32) (Required)

        Returns a two-tuple containing the return value (type bool)
        and a dictionary with the out-parameters

        Output parameters:
        lo -- (type pywbem.Uint32) (Required)
        hi -- (type pywbem.Uint32) (Required)
        nlist -- (type [pywbem.Uint32,]) 

        Possible Errors:
        CIM_ERR_ACCESS_DENIED
        CIM_ERR_INVALID_PARAMETER (including missing, duplicate, 
            unrecognized or otherwise incorrect parameters)
        CIM_ERR_NOT_FOUND (the target CIM Class or instance does not 
            exist in the specified namespace)
        CIM_ERR_METHOD_NOT_AVAILABLE (the CIM Server is unable to honor 
            the invocation request)
        CIM_ERR_FAILED (some other unspecified error occurred)

        """

        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_genrandlist_uint32()' \
                % self.__class__.__name__)

        cnt = 0
        l = []
        while cnt < param_nelems:
            cnt+= 1
            l.append(pywbem.Uint32(random.randint(param_lo, param_hi)))
        out_params = {}
        out_params['lo'] = param_lo
        out_params['hi'] = param_hi
        out_params['nlist'] = l
        rval = True
        return (rval, out_params)
        
    def cim_method_genrand_real64(self, env, object_name, method,
                                  param_max,
                                  param_min):
        """Implements TestMethod.genRand_real64()

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        object_name -- A pywbem.CIMInstanceName or pywbem.CIMCLassName 
            specifying the object on which the method genRand_real64() 
            should be invoked.
        method -- A pywbem.CIMMethod representing the method meta-data
        param_max --  The input parameter max (type pywbem.Real64) 
        param_min --  The input parameter min (type pywbem.Real64) 

        Returns a two-tuple containing the return value (type pywbem.Real64)
        and a dictionary with the out-parameters

        Output parameters:
        success -- (type bool) 

        Possible Errors:
        CIM_ERR_ACCESS_DENIED
        CIM_ERR_INVALID_PARAMETER (including missing, duplicate, 
            unrecognized or otherwise incorrect parameters)
        CIM_ERR_NOT_FOUND (the target CIM Class or instance does not 
            exist in the specified namespace)
        CIM_ERR_METHOD_NOT_AVAILABLE (the CIM Server is unable to honor 
            the invocation request)
        CIM_ERR_FAILED (some other unspecified error occurred)

        """

        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_genrand_real64()' \
                % self.__class__.__name__)

        out_params = {}
        out_params['success'] = True
        range = param_max - param_min
        rval = pywbem.Real64(random.random() * range + param_min)
        return (rval, out_params)
        
    def cim_method_genrandlist_sint64(self, env, object_name, method,
                                      param_nelems,
                                      param_lo,
                                      param_hi):
        """Implements TestMethod.genRandList_sint64()

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        object_name -- A pywbem.CIMInstanceName or pywbem.CIMCLassName 
            specifying the object on which the method genRandList_sint64() 
            should be invoked.
        method -- A pywbem.CIMMethod representing the method meta-data
        param_nelems --  The input parameter nelems (type pywbem.Sint32) (Required)
        param_lo --  The input parameter lo (type pywbem.Sint64) (Required)
        param_hi --  The input parameter hi (type pywbem.Sint64) (Required)

        Returns a two-tuple containing the return value (type bool)
        and a dictionary with the out-parameters

        Output parameters:
        lo -- (type pywbem.Sint64) (Required)
        hi -- (type pywbem.Sint64) (Required)
        nlist -- (type [pywbem.Sint64,]) 

        Possible Errors:
        CIM_ERR_ACCESS_DENIED
        CIM_ERR_INVALID_PARAMETER (including missing, duplicate, 
            unrecognized or otherwise incorrect parameters)
        CIM_ERR_NOT_FOUND (the target CIM Class or instance does not 
            exist in the specified namespace)
        CIM_ERR_METHOD_NOT_AVAILABLE (the CIM Server is unable to honor 
            the invocation request)
        CIM_ERR_FAILED (some other unspecified error occurred)

        """

        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_genrandlist_sint64()' \
                % self.__class__.__name__)

        cnt = 0
        l = []
        while cnt < param_nelems:
            cnt+= 1
            l.append(pywbem.Sint64(random.randint(param_lo, param_hi)))
        out_params = {}
        out_params['lo'] = param_lo
        out_params['hi'] = param_hi
        out_params['nlist'] = l
        rval = True
        return (rval, out_params)
        
    def cim_method_genrandlist_uint16(self, env, object_name, method,
                                      param_nelems,
                                      param_lo,
                                      param_hi):
        """Implements TestMethod.genRandList_uint16()

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        object_name -- A pywbem.CIMInstanceName or pywbem.CIMCLassName 
            specifying the object on which the method genRandList_uint16() 
            should be invoked.
        method -- A pywbem.CIMMethod representing the method meta-data
        param_nelems --  The input parameter nelems (type pywbem.Sint32) (Required)
        param_lo --  The input parameter lo (type pywbem.Uint16) (Required)
        param_hi --  The input parameter hi (type pywbem.Uint16) (Required)

        Returns a two-tuple containing the return value (type bool)
        and a dictionary with the out-parameters

        Output parameters:
        lo -- (type pywbem.Uint16) (Required)
        hi -- (type pywbem.Uint16) (Required)
        nlist -- (type [pywbem.Uint16,]) 

        Possible Errors:
        CIM_ERR_ACCESS_DENIED
        CIM_ERR_INVALID_PARAMETER (including missing, duplicate, 
            unrecognized or otherwise incorrect parameters)
        CIM_ERR_NOT_FOUND (the target CIM Class or instance does not 
            exist in the specified namespace)
        CIM_ERR_METHOD_NOT_AVAILABLE (the CIM Server is unable to honor 
            the invocation request)
        CIM_ERR_FAILED (some other unspecified error occurred)

        """

        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_genrandlist_uint16()' \
                % self.__class__.__name__)

        cnt = 0
        l = []
        while cnt < param_nelems:
            cnt+= 1
            l.append(pywbem.Uint16(random.randint(param_lo, param_hi)))
        out_params = {}
        out_params['lo'] = param_lo
        out_params['hi'] = param_hi
        out_params['nlist'] = l
        rval = True
        return (rval, out_params)
        
    def cim_method_minmedmax_sint64(self, env, object_name, method,
                                    param_numlist):
        """Implements TestMethod.minmedmax_sint64()

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        object_name -- A pywbem.CIMInstanceName or pywbem.CIMCLassName 
            specifying the object on which the method minmedmax_sint64() 
            should be invoked.
        method -- A pywbem.CIMMethod representing the method meta-data
        param_numlist --  The input parameter numlist (type [pywbem.Sint64,]) (Required)

        Returns a two-tuple containing the return value (type bool)
        and a dictionary with the out-parameters

        Output parameters:
        max -- (type pywbem.Sint64) 
        min -- (type pywbem.Sint64) 
        med -- (type pywbem.Sint64) 

        Possible Errors:
        CIM_ERR_ACCESS_DENIED
        CIM_ERR_INVALID_PARAMETER (including missing, duplicate, 
            unrecognized or otherwise incorrect parameters)
        CIM_ERR_NOT_FOUND (the target CIM Class or instance does not 
            exist in the specified namespace)
        CIM_ERR_METHOD_NOT_AVAILABLE (the CIM Server is unable to honor 
            the invocation request)
        CIM_ERR_FAILED (some other unspecified error occurred)

        """

        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_minmedmax_sint64()' \
                % self.__class__.__name__)

        l = param_numlist
        l.sort()
        out_params = {}
        out_params['max'] = l[-1]
        out_params['min'] = l[0]
        ln = len(l)
        if ln % 2 == 0:
            out_params['med'] = pywbem.Sint64(
                    (l[(ln / 2) - 1] + l[(ln / 2)]) / 2)
        else:
            out_params['med'] = pywbem.Sint64(l[ln / 2])
        rval = True
        return (rval, out_params)
        
    def cim_method_getdate(self, env, object_name, method,
                           param_datestr):
        """Implements TestMethod.getDate()

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        object_name -- A pywbem.CIMInstanceName or pywbem.CIMCLassName 
            specifying the object on which the method getDate() 
            should be invoked.
        method -- A pywbem.CIMMethod representing the method meta-data
        param_datestr --  The input parameter datestr (type unicode) (Required)

        Returns a two-tuple containing the return value (type pywbem.CIMDateTime)
        and a dictionary with the out-parameters

        Output parameters: none

        Possible Errors:
        CIM_ERR_ACCESS_DENIED
        CIM_ERR_INVALID_PARAMETER (including missing, duplicate, 
            unrecognized or otherwise incorrect parameters)
        CIM_ERR_NOT_FOUND (the target CIM Class or instance does not 
            exist in the specified namespace)
        CIM_ERR_METHOD_NOT_AVAILABLE (the CIM Server is unable to honor 
            the invocation request)
        CIM_ERR_FAILED (some other unspecified error occurred)

        """

        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_getdate()' \
                % self.__class__.__name__)

        out_params = {}
        rval = pywbem.CIMDateTime(param_datestr)
        return (rval, out_params)
        
    def cim_method_genrand_sint8(self, env, object_name, method,
                                 param_max,
                                 param_min):
        """Implements TestMethod.genRand_sint8()

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        object_name -- A pywbem.CIMInstanceName or pywbem.CIMCLassName 
            specifying the object on which the method genRand_sint8() 
            should be invoked.
        method -- A pywbem.CIMMethod representing the method meta-data
        param_max --  The input parameter max (type pywbem.Sint8) 
        param_min --  The input parameter min (type pywbem.Sint8) 

        Returns a two-tuple containing the return value (type pywbem.Sint8)
        and a dictionary with the out-parameters

        Output parameters:
        success -- (type bool) 

        Possible Errors:
        CIM_ERR_ACCESS_DENIED
        CIM_ERR_INVALID_PARAMETER (including missing, duplicate, 
            unrecognized or otherwise incorrect parameters)
        CIM_ERR_NOT_FOUND (the target CIM Class or instance does not 
            exist in the specified namespace)
        CIM_ERR_METHOD_NOT_AVAILABLE (the CIM Server is unable to honor 
            the invocation request)
        CIM_ERR_FAILED (some other unspecified error occurred)

        """

        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_genrand_sint8()' \
                % self.__class__.__name__)

        out_params = {}
        out_params['success'] = True
        rval = pywbem.Sint8(random.randint(param_min, param_max))
        return (rval, out_params)
        
    def cim_method_minmedmax_sint16(self, env, object_name, method,
                                    param_numlist):
        """Implements TestMethod.minmedmax_sint16()

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        object_name -- A pywbem.CIMInstanceName or pywbem.CIMCLassName 
            specifying the object on which the method minmedmax_sint16() 
            should be invoked.
        method -- A pywbem.CIMMethod representing the method meta-data
        param_numlist --  The input parameter numlist (type [pywbem.Sint16,]) (Required)

        Returns a two-tuple containing the return value (type bool)
        and a dictionary with the out-parameters

        Output parameters:
        max -- (type pywbem.Sint16) 
        min -- (type pywbem.Sint16) 
        med -- (type pywbem.Sint16) 

        Possible Errors:
        CIM_ERR_ACCESS_DENIED
        CIM_ERR_INVALID_PARAMETER (including missing, duplicate, 
            unrecognized or otherwise incorrect parameters)
        CIM_ERR_NOT_FOUND (the target CIM Class or instance does not 
            exist in the specified namespace)
        CIM_ERR_METHOD_NOT_AVAILABLE (the CIM Server is unable to honor 
            the invocation request)
        CIM_ERR_FAILED (some other unspecified error occurred)

        """

        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_minmedmax_sint16()' \
                % self.__class__.__name__)

        l = param_numlist
        l.sort()
        out_params = {}
        out_params['max'] = l[-1]
        out_params['min'] = l[0]
        ln = len(l)
        if ln % 2 == 0:
            out_params['med'] = pywbem.Sint16(
                    (l[(ln / 2) - 1] + l[(ln / 2)]) / 2)
        else:
            out_params['med'] = pywbem.Sint16(l[ln / 2])
        rval = True
        return (rval, out_params)
        
    def cim_method_genrandlist_real64(self, env, object_name, method,
                                      param_nelems,
                                      param_lo,
                                      param_hi):
        """Implements TestMethod.genRandList_real64()

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        object_name -- A pywbem.CIMInstanceName or pywbem.CIMCLassName 
            specifying the object on which the method genRandList_real64() 
            should be invoked.
        method -- A pywbem.CIMMethod representing the method meta-data
        param_nelems --  The input parameter nelems (type pywbem.Sint32) (Required)
        param_lo --  The input parameter lo (type pywbem.Real64) (Required)
        param_hi --  The input parameter hi (type pywbem.Real64) (Required)

        Returns a two-tuple containing the return value (type bool)
        and a dictionary with the out-parameters

        Output parameters:
        lo -- (type pywbem.Real64) (Required)
        hi -- (type pywbem.Real64) (Required)
        nlist -- (type [pywbem.Real64,]) 

        Possible Errors:
        CIM_ERR_ACCESS_DENIED
        CIM_ERR_INVALID_PARAMETER (including missing, duplicate, 
            unrecognized or otherwise incorrect parameters)
        CIM_ERR_NOT_FOUND (the target CIM Class or instance does not 
            exist in the specified namespace)
        CIM_ERR_METHOD_NOT_AVAILABLE (the CIM Server is unable to honor 
            the invocation request)
        CIM_ERR_FAILED (some other unspecified error occurred)

        """

        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_genrandlist_real64()' \
                % self.__class__.__name__)

        cnt = 0
        l = []
        range = param_hi - param_lo
        while cnt < param_nelems:
            cnt+= 1
            l.append(pywbem.Real64(
                random.random() * range + param_lo))
        out_params = {}
        out_params['lo'] = param_lo
        out_params['hi'] = param_hi
        out_params['nlist'] = l
        rval = True
        return (rval, out_params)
        
    def cim_method_minmedmax_uint16(self, env, object_name, method,
                                    param_numlist):
        """Implements TestMethod.minmedmax_uint16()

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        object_name -- A pywbem.CIMInstanceName or pywbem.CIMCLassName 
            specifying the object on which the method minmedmax_uint16() 
            should be invoked.
        method -- A pywbem.CIMMethod representing the method meta-data
        param_numlist --  The input parameter numlist (type [pywbem.Uint16,]) (Required)

        Returns a two-tuple containing the return value (type bool)
        and a dictionary with the out-parameters

        Output parameters:
        max -- (type pywbem.Uint16) 
        min -- (type pywbem.Uint16) 
        med -- (type pywbem.Uint16) 

        Possible Errors:
        CIM_ERR_ACCESS_DENIED
        CIM_ERR_INVALID_PARAMETER (including missing, duplicate, 
            unrecognized or otherwise incorrect parameters)
        CIM_ERR_NOT_FOUND (the target CIM Class or instance does not 
            exist in the specified namespace)
        CIM_ERR_METHOD_NOT_AVAILABLE (the CIM Server is unable to honor 
            the invocation request)
        CIM_ERR_FAILED (some other unspecified error occurred)

        """

        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_minmedmax_uint16()' \
                % self.__class__.__name__)

        l = param_numlist
        l.sort()
        out_params = {}
        out_params['max'] = l[-1]
        out_params['min'] = l[0]
        ln = len(l)
        if ln % 2 == 0:
            out_params['med'] = pywbem.Uint16(
                    (l[(ln / 2) - 1] + l[(ln / 2)]) / 2)
        else:
            out_params['med'] = pywbem.Uint16(l[ln / 2])
        rval = True
        return (rval, out_params)
        
    def cim_method_minmedmax_uint32(self, env, object_name, method,
                                    param_numlist):
        """Implements TestMethod.minmedmax_uint32()

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        object_name -- A pywbem.CIMInstanceName or pywbem.CIMCLassName 
            specifying the object on which the method minmedmax_uint32() 
            should be invoked.
        method -- A pywbem.CIMMethod representing the method meta-data
        param_numlist --  The input parameter numlist (type [pywbem.Uint32,]) (Required)

        Returns a two-tuple containing the return value (type bool)
        and a dictionary with the out-parameters

        Output parameters:
        max -- (type pywbem.Uint32) 
        min -- (type pywbem.Uint32) 
        med -- (type pywbem.Uint32) 

        Possible Errors:
        CIM_ERR_ACCESS_DENIED
        CIM_ERR_INVALID_PARAMETER (including missing, duplicate, 
            unrecognized or otherwise incorrect parameters)
        CIM_ERR_NOT_FOUND (the target CIM Class or instance does not 
            exist in the specified namespace)
        CIM_ERR_METHOD_NOT_AVAILABLE (the CIM Server is unable to honor 
            the invocation request)
        CIM_ERR_FAILED (some other unspecified error occurred)

        """

        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_minmedmax_uint32()' \
                % self.__class__.__name__)

        l = param_numlist
        l.sort()
        out_params = {}
        out_params['max'] = l[-1]
        out_params['min'] = l[0]
        ln = len(l)
        if ln % 2 == 0:
            out_params['med'] = pywbem.Uint16(
                    (l[(ln / 2) - 1] + l[(ln / 2)]) / 2)
        else:
            out_params['med'] = pywbem.Uint16(l[ln / 2])
        rval = True
        return (rval, out_params)
        
    def cim_method_getdates(self, env, object_name, method,
                            param_datestrs):
        """Implements TestMethod.getDates()

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        object_name -- A pywbem.CIMInstanceName or pywbem.CIMCLassName 
            specifying the object on which the method getDates() 
            should be invoked.
        method -- A pywbem.CIMMethod representing the method meta-data
        param_datestrs --  The input parameter datestrs (type [unicode,]) (Required)

        Returns a two-tuple containing the return value (type bool)
        and a dictionary with the out-parameters

        Output parameters:
        nelems -- (type pywbem.Sint32) 
        elems -- (type [pywbem.CIMDateTime,]) 

        Possible Errors:
        CIM_ERR_ACCESS_DENIED
        CIM_ERR_INVALID_PARAMETER (including missing, duplicate, 
            unrecognized or otherwise incorrect parameters)
        CIM_ERR_NOT_FOUND (the target CIM Class or instance does not 
            exist in the specified namespace)
        CIM_ERR_METHOD_NOT_AVAILABLE (the CIM Server is unable to honor 
            the invocation request)
        CIM_ERR_FAILED (some other unspecified error occurred)

        """

        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_getdates()' \
                % self.__class__.__name__)

        out_params = {}
        elems = [pywbem.CIMDateTime(s) for s in param_datestrs]
        out_params['nelems'] = pywbem.Sint32(len(elems))
        out_params['elems'] = l
        rval = True
        return (rval, out_params)
        
    def cim_method_getstrprop(self, env, object_name, method):
        """Implements TestMethod.getStrProp()

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        object_name -- A pywbem.CIMInstanceName or pywbem.CIMCLassName 
            specifying the object on which the method getStrProp() 
            should be invoked.
        method -- A pywbem.CIMMethod representing the method meta-data

        Returns a two-tuple containing the return value (type unicode)
        and a dictionary with the out-parameters

        Output parameters: none

        Possible Errors:
        CIM_ERR_ACCESS_DENIED
        CIM_ERR_INVALID_PARAMETER (including missing, duplicate, 
            unrecognized or otherwise incorrect parameters)
        CIM_ERR_NOT_FOUND (the target CIM Class or instance does not 
            exist in the specified namespace)
        CIM_ERR_METHOD_NOT_AVAILABLE (the CIM Server is unable to honor 
            the invocation request)
        CIM_ERR_FAILED (some other unspecified error occurred)

        """

        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_getstrprop()' \
                % self.__class__.__name__)

        if not hasattr(object_name, 'keybindings'):
            raise pywbem.CIMError(pywbem.CIM_ERR_INVALID_PARAMETER)
        try:
            inst = g_insts[object_name['id']]
        except KeyError:
            raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND)
        out_params = {}
        rval = inst[0]
        return (rval, out_params)
        
    def cim_method_genrand_uint64(self, env, object_name, method,
                                  param_max,
                                  param_min):
        """Implements TestMethod.genRand_uint64()

        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        object_name -- A pywbem.CIMInstanceName or pywbem.CIMCLassName 
            specifying the object on which the method genRand_uint64() 
            should be invoked.
        method -- A pywbem.CIMMethod representing the method meta-data
        param_max --  The input parameter max (type pywbem.Uint64) 
        param_min --  The input parameter min (type pywbem.Uint64) 

        Returns a two-tuple containing the return value (type pywbem.Uint64)
        and a dictionary with the out-parameters

        Output parameters:
        success -- (type bool) 

        Possible Errors:
        CIM_ERR_ACCESS_DENIED
        CIM_ERR_INVALID_PARAMETER (including missing, duplicate, 
            unrecognized or otherwise incorrect parameters)
        CIM_ERR_NOT_FOUND (the target CIM Class or instance does not 
            exist in the specified namespace)
        CIM_ERR_METHOD_NOT_AVAILABLE (the CIM Server is unable to honor 
            the invocation request)
        CIM_ERR_FAILED (some other unspecified error occurred)

        """

        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_genrand_uint64()' \
                % self.__class__.__name__)

        out_params = {}
        out_params['success'] = True
        rval = pywbem.Uint64(random.randint(param_min, param_max))
        return (rval, out_params)
        
    def cim_method_setintprop(self, env, object_name, method,
                              param_value):
        """Implements TestMethod.setIntProp()

        Set the value of p_sint32.  Return the previous value of p_sint32.
        
        Keyword arguments:
        env -- Provider Environment (pycimmb.ProviderEnvironment)
        object_name -- A pywbem.CIMInstanceName or pywbem.CIMCLassName 
            specifying the object on which the method setIntProp() 
            should be invoked.
        method -- A pywbem.CIMMethod representing the method meta-data
        param_value --  The input parameter value (type pywbem.Sint32) (Required)

        Returns a two-tuple containing the return value (type pywbem.Sint32)
        and a dictionary with the out-parameters

        Output parameters: none

        Possible Errors:
        CIM_ERR_ACCESS_DENIED
        CIM_ERR_INVALID_PARAMETER (including missing, duplicate, 
            unrecognized or otherwise incorrect parameters)
        CIM_ERR_NOT_FOUND (the target CIM Class or instance does not 
            exist in the specified namespace)
        CIM_ERR_METHOD_NOT_AVAILABLE (the CIM Server is unable to honor 
            the invocation request)
        CIM_ERR_FAILED (some other unspecified error occurred)

        """

        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_setintprop()' \
                % self.__class__.__name__)

        if not hasattr(object_name, 'keybindings'):
            raise pywbem.CIMError(pywbem.CIM_ERR_INVALID_PARAMETER)
        try:
            inst = g_insts[object_name['id']]
        except KeyError:
            raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND)

        rval = inst[1]
        inst[1] = param_value
        out_params = {}
        return (rval, out_params)
        
## end of class TestMethodProvider

def get_providers(env): 
    testmethod_prov = TestMethodProvider(env)  
    return {'TestMethod': testmethod_prov} 
