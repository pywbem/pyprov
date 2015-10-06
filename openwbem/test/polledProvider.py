import pywbem, threading, time

_gcond = threading.Condition()

TheThread = None
TheEnv = None

class MyThread(threading.Thread):
    def __init__(self):
        log = TheEnv.get_logger()
        log.log_debug('##### MyThread.__init__ called')
        threading.Thread.__init__(self)
        self.running = False
        self.shuttingdown = False
        log.log_debug('##### MyThread.__init__ returning')

    def shutdown(self):
        if not self.running:
            return
        self.shuttingdown = True
        _gcond.acquire()
        _gcond.notifyAll()
        _gcond.release()
        self.join()

        
    def run(self):
        log = TheEnv.get_logger()
        self.running = True
        self.shuttingdown = False
        while not self.shuttingdown:
            log.log_debug('##### MyThread.run entered...')
            log.log_debug('##### MyThread.run acquiring lock on condition')
            _gcond.acquire()
            log.log_debug('##### MyThread.run waiting for condition for 2 seconds')
            l = _gcond.wait(2.0)
            _gcond.release()
            if self.shuttingdown:
                log.log_debug('##### MyThread.run shut down detected')
                break
            log.log_debug('##### MyThread.run looping')
        self.running = False
        log.log_debug('##### MyThread.run returning')


def get_initial_polling_interval(env):
    logger = env.get_logger()
    print '#### Threaded python creating thread object...'
    global TheThread, TheEnv
    TheEnv = env
    TheThread = MyThread()
    print '#### Threaded python created thread object.'
    print '#### Threaded python starting thread object...'
    TheThread.start()
    print '#### Threaded python returning 0 from get_initial_polling_interval'
    return 0

def poll(env):
    logger = env.get_logger()
    logger.log_debug("##### Python polled provider poll called. SHOULDN'T HAPPEN")
    return 0

def shutdown(env):
    logger = env.get_logger()
    print '#### Threaded python shutdown called. shutting down thread'
    TheThread.shutdown()
    print '#### Threaded python shutdown. thread has shutdown. returning'

