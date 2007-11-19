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
        logger = env.get_logger()
        logger.log_debug('Entering %s.delete_instance()' \
                % self.__class__.__name__)

        try:
            del g_insts[instance_name['id']]
        except KeyError:
            raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND)
        
    def cim_method_mkunichar(self, env, object_name, method,
                             param_c):

        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_mkunichar()' \
                % self.__class__.__name__)

        # TODO do something
        raise pywbem.CIMError(pywbem.CIM_ERR_METHOD_NOT_AVAILABLE) # Remove to implemented
        out_params = {}
        rval = None # TODO (type pywbem.Char16)
        return (rval, out_params)
        
    def cim_method_mkunistr_char16(self, env, object_name, method,
                                   param_carr):
        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_mkunistr_char16()' \
                % self.__class__.__name__)

        # TODO do something
        raise pywbem.CIMError(pywbem.CIM_ERR_METHOD_NOT_AVAILABLE) # Remove to implemented
        out_params = {}
        rval = None # TODO (type unicode)
        return (rval, out_params)
        
    def cim_method_strsplit(self, env, object_name, method,
                            param_str,
                            param_sep):
        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_strsplit()' \
                % self.__class__.__name__)

        elems = param_str.split(param_sep)
        out_params = {}
        out_params['nelems'] = pywbem.Sint32(len(elems))
        out_params['elems'] = elems
        rval = True
        return (rval, out_params)
        
    def cim_method_strcat(self, env, object_name, method,
                          param_strs,
                          param_sep):
        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_strcat()' \
                % self.__class__.__name__)

        out_params = {}
        rval = param_sep.join(param_strs)
        return (rval, out_params)
        
    def cim_method_mkunichararray(self, env, object_name, method,
                                  param_inarr):
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
        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_mkunistr_sint8()' \
                % self.__class__.__name__)

        rval = ''
        for i in param_carr:
            rval += char(i)
        out_params = {}
        return (rval, out_params)

    def minmedmax(self, dt, env, object_name, method, param_numlist):
        l = param_numlist
        l.sort()
        out_params = {}
        out_params['max'] = l[-1]
        out_params['min'] = l[0]
        ln = len(l)
        if ln % 2 == 0:
            out_params['med'] = dt((l[(ln / 2) - 1] + l[(ln / 2)]) / 2)
        else:
            out_params['med'] = dt(l[ln / 2])
        rval = True
        return (rval, out_params)
        
    def cim_method_minmedmax_real64(self, env, object_name, method,
                                    param_numlist):
        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_minmedmax_real64()' \
                % self.__class__.__name__)

        return self.minmedmax(pywbem.Real64, env, object_name, method, param_numlist)
        
    def cim_method_minmedmax_real32(self, env, object_name, method,
                                    param_numlist):
        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_minmedmax_real32()' \
                % self.__class__.__name__)

        return self.minmedmax(pywbem.Real32, env, object_name, method, param_numlist)
        
    def cim_method_minmedmax_uint8(self, env, object_name, method,
                                   param_numlist):
        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_minmedmax_uint8()' \
                % self.__class__.__name__)

        return self.minmedmax(pywbem.Uint8, env, object_name, method, param_numlist)
        
    def cim_method_minmedmax_sint64(self, env, object_name, method,
                                    param_numlist):
        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_minmedmax_sint64()' \
                % self.__class__.__name__)

        return self.minmedmax(pywbem.Sint64, env, object_name, method, param_numlist)
        
    def cim_method_minmedmax_sint16(self, env, object_name, method,
                                    param_numlist):

        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_minmedmax_sint16()' \
                % self.__class__.__name__)

        return self.minmedmax(pywbem.Sint16, env, object_name, method, param_numlist)
        
    def cim_method_minmedmax_uint16(self, env, object_name, method,
                                    param_numlist):

        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_minmedmax_uint16()' \
                % self.__class__.__name__)

        return self.minmedmax(pywbem.Uint16, env, object_name, method, param_numlist)
        
    def cim_method_minmedmax_uint64(self, env, object_name, method,
                                    param_numlist):
        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_minmedmax_uint64()' \
                % self.__class__.__name__)

        return self.minmedmax(pywbem.Uint64, env, object_name, method, param_numlist)
        
    def cim_method_minmedmax_sint8(self, env, object_name, method,
                                   param_numlist):
        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_minmedmax_sint8()' \
                % self.__class__.__name__)

        return self.minmedmax(pywbem.Sint8, env, object_name, method, param_numlist)
        
    def cim_method_minmedmax_sint32(self, env, object_name, method,
                                    param_numlist):
        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_minmedmax_sint32()' \
                % self.__class__.__name__)

        return self.minmedmax(pywbem.Sint32, env, object_name, method, param_numlist)
        
    def cim_method_minmedmax_uint32(self, env, object_name, method,
                                    param_numlist):

        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_minmedmax_uint32()' \
                % self.__class__.__name__)

        return self.minmedmax(pywbem.Uint32, env, object_name, method, param_numlist)
        
    def cim_method_getdate(self, env, object_name, method,
                           param_datestr):
        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_getdate()' \
                % self.__class__.__name__)

        out_params = {}
        rval = pywbem.CIMDateTime(param_datestr)
        return (rval, out_params)
        
    def cim_method_getdates(self, env, object_name, method,
                            param_datestrs):

        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_getdates()' \
                % self.__class__.__name__)

        out_params = {}
        elems = [pywbem.CIMDateTime(s) for s in param_datestrs]
        out_params['nelems'] = pywbem.Sint32(len(elems))
        out_params['elems'] = l
        rval = True
        return (rval, out_params)
        
    def cim_method_getintprop(self, env, object_name, method):
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
        
    def cim_method_getstrprop(self, env, object_name, method):

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
        
    def cim_method_setstrprop(self, env, object_name, method,
                              param_value):
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
        
    def cim_method_setintprop(self, env, object_name, method,
                              param_value):

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

    def genrandlist_i(self, dt, env, object_name, method, param_nelems, 
                                param_lo, param_hi):
        cnt = 0
        l = []
        while cnt < param_nelems:
            cnt+= 1
            l.append(dt(random.randint(param_lo, param_hi)))
        out_params = {}
        out_params['lo'] = param_lo
        out_params['hi'] = param_hi
        out_params['nlist'] = l
        rval = True
        return (rval, out_params)
        
    def cim_method_genrandlist_sint64(self, env, object_name, method,
                                      param_nelems,
                                      param_lo,
                                      param_hi):
        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_genrandlist_sint64()' \
                % self.__class__.__name__)
        return self.genrandlist_i(pywbem.Sint64, env, object_name, method, 
                                    param_nelems, param_lo, param_hi)

        
    def cim_method_genrandlist_uint16(self, env, object_name, method,
                                      param_nelems,
                                      param_lo,
                                      param_hi):
        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_genrandlist_uint16()' \
                % self.__class__.__name__)
        return self.genrandlist_i(pywbem.Uint16, env, object_name, method, 
                                    param_nelems, param_lo, param_hi)

        
    def cim_method_genrandlist_uint32(self, env, object_name, method,
                                      param_nelems,
                                      param_lo,
                                      param_hi):
        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_genrandlist_uint32()' \
                % self.__class__.__name__)
        return self.genrandlist_i(pywbem.Uint32, env, object_name, method, 
                                    param_nelems, param_lo, param_hi)

    def cim_method_genrandlist_sint8(self, env, object_name, method,
                                     param_nelems,
                                     param_lo,
                                     param_hi):
        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_genrandlist_sint8()' \
                % self.__class__.__name__)
        return self.genrandlist_i(pywbem.Sint8, env, object_name, method, 
                                    param_nelems, param_lo, param_hi)

    def cim_method_genrandlist_uint8(self, env, object_name, method,
                                     param_nelems,
                                     param_lo,
                                     param_hi):
        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_genrandlist_uint8()' \
                % self.__class__.__name__)
        return self.genrandlist_i(pywbem.Uint8, env, object_name, method, 
                                    param_nelems, param_lo, param_hi)

    def cim_method_genrandlist_uint64(self, env, object_name, method,
                                      param_nelems,
                                      param_lo,
                                      param_hi):
        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_genrandlist_uint64()' \
                % self.__class__.__name__)
        return self.genrandlist_i(pywbem.Uint64, env, object_name, method, 
                                    param_nelems, param_lo, param_hi)

    def cim_method_genrandlist_sint32(self, env, object_name, method,
                                      param_nelems,
                                      param_lo,
                                      param_hi):
        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_genrandlist_sint32()' \
                % self.__class__.__name__)
        return self.genrandlist_i(pywbem.Sint32, env, object_name, method, 
                                    param_nelems, param_lo, param_hi)

    def cim_method_genrandlist_sint16(self, env, object_name, method,
                                      param_nelems,
                                      param_lo,
                                      param_hi):
        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_genrandlist_sint16()' \
                % self.__class__.__name__)
        return self.genrandlist_i(pywbem.Sint16, env, object_name, method, 
                                    param_nelems, param_lo, param_hi)

    def genrandlist_r(self, dt, object_name, method, param_nelems, 
                            param_lo, param_hi):
        cnt = 0
        l = []
        range = param_hi - param_lo
        while cnt < param_nelems:
            cnt+= 1
            l.append(dt(random.random() * range + param_lo))
        out_params = {}
        out_params['lo'] = param_lo
        out_params['hi'] = param_hi
        out_params['nlist'] = l
        rval = True
        return (rval, out_params)

    def cim_method_genrandlist_real64(self, env, object_name, method,
                                      param_nelems,
                                      param_lo,
                                      param_hi):

        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_genrandlist_real64()' \
                % self.__class__.__name__)
        return self.genrandlist_r(pywbem.Real64, env, object_name, method, 
                                    param_nelems, param_lo, param_hi)

        
    def cim_method_genrandlist_real32(self, env, object_name, method,
                                      param_nelems,
                                      param_lo,
                                      param_hi):
        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_genrandlist_real32()' \
                % self.__class__.__name__)
        return self.genrandlist_r(pywbem.Real32, env, object_name, method, 
                                    param_nelems, param_lo, param_hi)

    def genrand_i(self, dt, env, object_name, method,
                                  param_max,
                                  param_min):
        out_params = {}
        out_params['success'] = True
        rval = dt(random.randint(param_min, param_max))
        return (rval, out_params)

    def cim_method_genrand_uint64(self, env, object_name, method,
                                  param_max,
                                  param_min):

        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_genrand_uint64()' \
                % self.__class__.__name__)

        return self.genrand_i(pywbem.Uint64, env, object_name, method, 
                param_max, param_min)
        
    def cim_method_genrand_sint8(self, env, object_name, method,
                                 param_max,
                                 param_min):
        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_genrand_sint8()' \
                % self.__class__.__name__)
        return self.genrand_i(pywbem.Sint8, env, object_name, method, 
                param_max, param_min)

    def cim_method_genrand_sint32(self, env, object_name, method,
                                  param_max,
                                  param_min):
        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_genrand_sint32()' \
                % self.__class__.__name__)
        return self.genrand_i(pywbem.Sint32, env, object_name, method, 
                param_max, param_min)

    def cim_method_genrand_uint32(self, env, object_name, method,
                                  param_max,
                                  param_min):
        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_genrand_uint32()' \
                % self.__class__.__name__)
        return self.genrand_i(pywbem.Uint32, env, object_name, method, 
                param_max, param_min)
        
    def cim_method_genrand_sint16(self, env, object_name, method,
                                  param_max,
                                  param_min):
        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_genrand_sint16()' \
                % self.__class__.__name__)
        return self.genrand_i(pywbem.Sint16, env, object_name, method, 
                param_max, param_min)

    def cim_method_genrand_uint16(self, env, object_name, method,
                                  param_max,
                                  param_min):
        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_genrand_uint16()' \
                % self.__class__.__name__)
        return self.genrand_i(pywbem.Uint16, env, object_name, method, 
                param_max, param_min)

    def cim_method_genrand_uint8(self, env, object_name, method,
                                 param_max,
                                 param_min):
        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_genrand_uint8()' \
                % self.__class__.__name__)
        return self.genrand_i(pywbem.Uint8, env, object_name, method, 
                param_max, param_min)
        
    def cim_method_genrand_sint64(self, env, object_name, method,
                                  param_max,
                                  param_min):
        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_genrand_sint64()' \
                % self.__class__.__name__)
        return self.genrand_i(pywbem.Sint64, env, object_name, method, 
                param_max, param_min)

    def genrand_r(self, dt, env, object_name, method,
                                  param_max,
                                  param_min):
        out_params = {}
        out_params['success'] = True
        range = param_max - param_min
        rval = dt(random.random() * range + param_min)
        return (rval, out_params)

    def cim_method_genrand_real32(self, env, object_name, method,
                                  param_max,
                                  param_min):
        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_genrand_real32()' \
                % self.__class__.__name__)
        return self.genrand_r(pywbem.Real32, env, object_name, method, 
                param_max, param_min)

    def cim_method_genrand_real64(self, env, object_name, method,
                                  param_max,
                                  param_min):
        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_genrand_real64()' \
                % self.__class__.__name__)
        return self.genrand_r(pywbem.Real64, env, object_name, method, 
                param_max, param_min)

    def cim_method_delobject(self, env, object_name, method,
                             param_path):
        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_delobject()' \
                % self.__class__.__name__)

        id = param_path['id']
        if id not in g_insts:
            rval = pywbem.Sint32(pywbem.CIM_ERR_NOT_FOUND)
        else:
            del g_insts[id]
            rval = pywbem.Sint32(0)
        return (rval, {})

    def cim_method_delobjects(self, env, object_name, method,
                              param_paths):
        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_delobjects()' \
                % self.__class__.__name__)
        rval = pywbem.Sint32(0)
        for path in param_paths:
            id = path['id']
            if id not in g_insts:
                rval = pywbem.Sint32(pywbem.CIM_ERR_NOT_FOUND)
            else:
                del g_insts[id]
        return (rval, {})
        
    def cim_method_getobjectpath(self, env, object_name, method):
        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_getobjectpath()' \
                % self.__class__.__name__)

        if not g_insts:
            path = None
            rval = pywbem.Sint32(pywbem.CIM_ERR_NOT_FOUND)
        else:
            path = pywbem.CIMInstanceName('TestMethod', 
                    namespace=object_name.namespace,
                    keybindings={'id':g_insts.keys()[0]})
            rval = pywbem.Sint32(0)
        out_params = {}
        out_params['path'] = path
        return (rval, out_params)

    def cim_method_getobjectpaths(self, env, object_name, method):
        logger = env.get_logger()
        logger.log_debug('Entering %s.cim_method_getobjectpaths()' \
                % self.__class__.__name__)

        paths = []
        for key in g_insts.keys():
            paths.append(pywbem.CIMInstanceName('TestMethod', 
                    namespace=object_name.namespace,
                    keybindings={'id':key}))
        out_params = {}
        out_params['paths'] = paths
        rval = pywbem.Sint32(0)
        return (rval, out_params)
        
## end of class TestMethodProvider

def get_providers(env): 
    testmethod_prov = TestMethodProvider(env)  
    return {'TestMethod': testmethod_prov} 
