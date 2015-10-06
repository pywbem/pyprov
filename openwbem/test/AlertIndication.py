import pyprovider
import pywbem, threading, time

TheIndicationThread = None

##############################################################################
class IndicationThread(threading.Thread):


    #######################################################
    def __init__(self, env, sysname):
        threading.Thread.__init__(self)
        self.count = 0
        self.env = env
        self.sysname = sysname
        self.shuttingdown = False
        self._cond = threading.Condition()
        log = self.env.get_logger()
        log.log_debug('###### Python IndicationThread has been created')

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

            self.count += 1

            ci = pywbem.CIMInstance('Py_TestAlert')
            ci['SystemName'] = self.sysname
            ci['MessageID'] = 'PyTestInd:%s' % self.count
            ci['Message'] = 'Hello. This is a python alert indication'
            ci['SystemCreationClassName'] = 'OMC_UnitaryComputerSystem'
            ci['EventID'] = str(self.count)
            ci['EventTime'] = pywbem.CIMDateTime.now()
            ci['AlertType'] = pywbem.Uint16(1)
            ci['Description'] = 'This is a test indication from the provider interface'
            ci['ProviderName'] = 'AlertIndication.py'
            ci['PerceivedSeverity'] = pywbem.Uint16(2)
            ci['OwningEntity'] = 'mine. mine. all mine'
            ci['RecommendedActions'] = ['Have fun', 'Ignore crap like this']
            ci['IndicationTime'] = pywbem.CIMDateTime.now()
            ci['IndicationIdentifier'] = 'Little redundant'
            log.log_debug('##### IndicationThread.run exporting indication')

            ch.export_indication(ci, 'root/cimv2')

        log.log_debug('##### IndicationThread.run returning')

##############################################################################
def activate_filter(env, filter, eventType, namespace, classes, firstActivation):
    logger = env.get_logger();
    logger.log_debug('#### Python activate_filter called')

##############################################################################
def deactivate_filter(env, filter, eventType, namespace, classes, lastActivation):
    logger = env.get_logger();
    logger.log_debug('#### Python deactivate_filter called')

##############################################################################
def authorize_filter(env, filter, eventType, namespace, classes, owner):
    logger = env.get_logger();
    logger.log_debug('#### Python authorize_filter called')

##############################################################################
def get_initial_polling_interval(env):
    logger = env.get_logger()
    logger.log_debug('#### indication python get_initial_polling_interval called...')
    # I'm going to tell the CIMOM to call the poll method in
    # 3 seconds. In the poll method I'll set things up for
    # the indication thread...
    return 3

##############################################################################
def poll(env):
    logger = env.get_logger()
    logger.log_debug('#### indication python poll called...')

    # Get The instance of the OMC_UnitaryComputerSystem so we can use the
    # system name in the indications that will be generated
    ch = env.get_cimom_handle()
    inames = ch.EnumerateInstanceNames('OMC_UnitaryComputerSystem', 'root/cimv2')

    # Make sure we got the instance of OMC_UnitaryComputerSystem
    if not len(inames):
        logger.log_debug('#### No computer system instance names found. '
            'Indications WILL NOT BE GENERATED')
        return 0

    sysname = inames[0]['Name']
    logger.log_debug('#### The system name is %s' % sysname)
    logger.log_debug('#### indication python creating thread object...')
    global TheIndicationThread
    # Save the provider environment so the indication thread can
    # use it to generate indications
    TheIndicationThread = IndicationThread(env, sysname)
    logger.log_debug('#### Threaded python created thread object.')
    logger.log_debug('#### Threaded python starting thread object...')
    TheIndicationThread.start()
    logger.log_debug('#### Threaded python returning 0 from poll')
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

