import pywbem 
from optparse import OptionParser

def getWBEMConnParserOptions(parser):
    parser.add_option('', '--host', default='localhost', help='Specify the hosting machine of the CIMOM')
    parser.add_option('-s', '--secure', action='store_true', default=False, help='Use the HTTPS protocol. Default is HTTP')
    parser.add_option('-n', '--namespace', default='root/cimv2', help='Specify the namespace the test runs against')
    parser.add_option('', '--user', default='anonymous', help='Specify the user name used when connection to the CIMOM')
    parser.add_option('', '--password', default='nothing', help='Specify the password for the user')
    parser.add_option('', '--port', default='5989', help='Specify the port for the CIMOM')

class wbem_connection:
    conn = ''
    namesapce = ''

    def __init__(self):
        self.uname = 'pegasus'
        self.passwd = 'novell'
    
    #def get_wbem_connection(*args): 
    def get_wbem_connection(self): 
        """Return a wbem connection to ZOS_SERVER"""
        #if not args:
            #args = ('https://localhost', ('pegasus', 'novell'))
        #conn = pywbem.WBEMConnection(*args)
        conn = pywbem.WBEMConnection('https://localhost', ('pegasus', 'novell'))
        conn.default_namespace = 'root/cimv2'
        return conn

    def get_default_namespace(self):
        return self.namespace

    def set_default_namespace(self, namespace):
        self.namespace = namespace
        conn.default_namespace = self.namespace


    def _WBEMConnFromOptions(self, options=None):
        if options is None:
            parser = OptionParser()
            getWBEMConnParserOptions(parser)
            options, args = parser.parse_args()
        proto = options.secure and 'http' or 'https'
        url = '%s://%s:%s' % (proto, options.host, options.port)
        wconn = pywbem.WBEMConnection(url, (options.user, options.password),
                default_namespace=options.namespace)
        return wconn




