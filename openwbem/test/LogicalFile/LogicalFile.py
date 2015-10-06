"""Python Provider for CIM_LogicalFile

Instruments the CIM class CIM_LogicalFile

"""

import pywbem
from pycim import CIMProvider
from stat import *
from statvfs import *
from socket import getfqdn
import os
from threading import RLock
import pwd
import grp

_mounts = {}
_mountssum = ''
_guard = RLock()

def _scanmounts():
    """Process /proc/mounts."""
    global _mountssum
    global _mounts
    mfile = open('/proc/mounts')
    # mtime for /proc/mounts doesn't seem to get updated, so we'll 
    # compare the whole file instead
    _guard.acquire()
    contents = mfile.read()
    if contents != _mountssum:
        _mountssum = contents
        _mounts.clear()
        mfile.seek(0)
        for line in mfile:
            line = line.strip().split()
            if line[0] == 'rootfs' or len(line) < 2 or line[1][0] != '/':
                continue
            dev = line[0]
            if dev.startswith('/dev'):
                tcount = 0
                # naive way to not get stuck in link loops. 
                while os.path.islink(dev) and tcount < 10:
                    tcount += 1
                    target = os.readlink(dev)
                    if target[0] != '/':
                        target = os.path.dirname(dev)+'/'+target
                    dev = os.path.abspath(target)
            _mounts[os.stat(line[1])[ST_DEV]] = dev
    _guard.release()
    mfile.close()

_scanmounts()


def get_keys(fname, map, stat=None, linux_file=False):
    """Set the keys on an instance or instance name

    args: 
    fname -- The file name
    map -- the mapping object
    stat -- A stat tuple
    linux_file -- True if the keys are for Py_LinuxFile, false if a 
        CIM_LogicalFile subclass

    """

    if stat is None:
        stat = os.lstat(fname)
    mode = stat[ST_MODE]
    if S_ISDIR(mode):
        ccname = 'Py_LinuxDirectory'
    elif S_ISCHR(mode) or S_ISBLK(mode):
        ccname = 'Py_LinuxDevice'
    elif S_ISREG(mode):
        ccname = 'Py_LinuxDataFile'
    elif S_ISFIFO(mode):
        ccname = 'Py_LinuxFIFOPipeFile'
    elif S_ISLNK(mode):
        ccname = 'Py_LinuxSymbolicLink'
    elif S_ISSOCK(mode):
        ccname = 'Py_LinuxSocketFile'
    else:
        raise pywbem.CIMError(pywbem.CIM_ERR_NOT_FOUND, 
            "Unknown file type for %s" % fname)
    if linux_file:
        map['LFName'] = fname
        map['LFCreationClassName'] = ccname
    else:
        map['Name'] = fname
        map['CreationClassName'] = ccname
    map['CSCreationClassName'] = 'CIM_UnitaryComputerSystem'
    map['CSName'] = getfqdn()
    map['FSCreationClassName'] = 'CIM_FileSystem'
    try:
        _guard.acquire()
        map['FSName'] = _mounts[stat[ST_DEV]]
        _guard.release()
    except KeyError:
        map['FSName'] = '<Unknown>'
        _guard.release()




class Py_LinuxFileProvider(CIMProvider):
    """Instrument the CIM class Py_LinuxFile 

    The UnixFile class holds properties that are valid for various
    subclasses of LogicalFile, in a Unix environment. This is defined as a
    separate and unique class since it is applicable to Unix files,
    directories, etc. It is associated via a FileIdentity relationship to
    these subclasses of LogicalFile. Unless this approach of creating and
    associating a separate class is used, it is necessary to subclass each
    of the inheritance hierarchies under LogicalFile, duplicating the
    properties in this class. The referenced _PC* and _POSIX* constants
    are defined in unistd.h. Some properties indicate whether the UNIX
    implementation support a feature such as asynchronous I/O or priority
    I/O. If supported, sysconf returns the value as defined in the
    appropriate header file such as unistd.h. If a feature is not
    supported, then pathconf returns a -1. In this case, the corresponding
    property should be returned without any value.
    
    """

    class Values_HealthState(object):
        Unknown = pywbem.Uint16(0)
        OK = pywbem.Uint16(5)
        Degraded_Warning = pywbem.Uint16(10)
        Minor_failure = pywbem.Uint16(15)
        Major_failure = pywbem.Uint16(20)
        Critical_failure = pywbem.Uint16(25)
        Non_recoverable_error = pywbem.Uint16(30)
        # DMTF_Reserved = ..

    class Values_OperationalStatus(object):
        Unknown = pywbem.Uint16(0)
        Other = pywbem.Uint16(1)
        OK = pywbem.Uint16(2)
        Degraded = pywbem.Uint16(3)
        Stressed = pywbem.Uint16(4)
        Predictive_Failure = pywbem.Uint16(5)
        Error = pywbem.Uint16(6)
        Non_Recoverable_Error = pywbem.Uint16(7)
        Starting = pywbem.Uint16(8)
        Stopping = pywbem.Uint16(9)
        Stopped = pywbem.Uint16(10)
        In_Service = pywbem.Uint16(11)
        No_Contact = pywbem.Uint16(12)
        Lost_Communication = pywbem.Uint16(13)
        Aborted = pywbem.Uint16(14)
        Dormant = pywbem.Uint16(15)
        Supporting_Entity_in_Error = pywbem.Uint16(16)
        Completed = pywbem.Uint16(17)
        Power_Mode = pywbem.Uint16(18)
        # DMTF_Reserved = ..
        # Vendor_Reserved = 0x8000..

    def __init__ (self):
        pass

    def get_instance (self, env, model, cim_class, stat=None):
        """Return an instance.

        Keyword arguments:
        env -- Provider Environment
        model -- A template of the CIMInstance to be returned.  The key 
            properties are set on this instance to correspond to the 
            instanceName that was requested.  The properties of the model
            are already filtered according to the PropertyList from the 
            request.
        cim_class -- The CIMClass

        Possible Errors:
        CIM_ERR_ACCESS_DENIED
        CIM_ERR_INVALID_PARAMETER (including missing, duplicate, unrecognized 
            or otherwise incorrect parameters)
        CIM_ERR_NOT_FOUND (the CIM Class does exist, but the requested CIM 
            Instance does not exist in the specified namespace)
        CIM_ERR_FAILED (some other unspecified error occurred)

        """

        if stat is None:
       	    stat = os.stat(model['lfname']) 
        statvfs = os.statvfs(model['lfname'])
        mode = stat[ST_MODE]

        model['SetUid'] = bool(mode & S_ISUID)
        model['HealthState'] = self.Values_HealthState.OK
        #model['PosixSyncIo'] = # TODO (type = uint64) 
        #model['StatusDescriptions'] = # TODO (type = string[]) 
        model['GroupWritable'] = bool(mode & S_IWGRP)
        #model['PosixChownRestricted'] = # TODO (type = uint64) 
        model['LinkCount'] = pywbem.Uint64(stat[ST_NLINK])
        model['UserWritable'] = bool(mode & S_IWUSR)
        #model['LinkMax'] = # TODO (type = uint64) 
        model['OperationalStatus'] = [self.Values_OperationalStatus.OK]
        model['SetGid'] = bool(mode & S_ISGID)
        model['SaveText'] = bool(mode & S_ISVTX)
        try:
            gid = grp.getgrgid(stat[ST_GID])[0]
        except KeyError:
            gid = str(stat[ST_GID])
        model['GroupID'] = gid
        model['UserExecutable'] = bool(mode & S_IXUSR)
        #model['Status'] = # TODO (type = string) 
        #model['Description'] = # TODO (type = string) 
        #model['InstallDate'] = # TODO (type = datetime) 
        #model['LastModifiedInode'] = # TODO (type = datetime) 
        model['WorldReadable'] = bool(mode & S_IROTH)
        #model['PosixPrioIo'] = # TODO (type = uint64) 
        #model['PathMax'] = # TODO (type = uint64) 
        model['WorldExecutable'] = bool(mode & S_IXOTH)
        model['GroupReadable'] = bool(mode & S_IRGRP)
        model['NameMax'] = pywbem.Uint64(statvfs[F_NAMEMAX])
        model['Name'] = model['lfname']
        model['ElementName'] = model['name'] == '/' and '/' or \
                    os.path.basename(model['name'])
        try:
            uid = pwd.getpwuid(stat[ST_UID])[0]
        except:
            uid = str(stat[ST_UID])
        model['UserID'] = uid
        model['UserReadable'] = bool(mode & S_IRUSR)
        model['GroupExecutable'] = bool(mode & S_IXGRP)
        #model['Caption'] = # TODO (type = string) 
        #model['PosixAsyncIo'] = # TODO (type = uint64) 
        model['WorldWritable'] = bool(mode & S_IWOTH)
        #model['PosixNoTrunc'] = # TODO (type = uint64) 
        model['FileInodeNumber'] = str(stat[ST_INO])
        return model

    def enum_instances(self, env, model, cim_class, keys_only):
        """ Enumerate instances.

        The WBEM operations EnumerateInstances and EnumerateInstanceNames
        are both mapped to this method. 
        This method is a python generator

        Keyword arguments:
        env -- Provider Environment
        model -- A template of the CIMInstances to be generated.  The 
            properties of the model are already filtered according to the 
            PropertyList from the request.
        cim_class -- The CIMClass
        keys_only -- A boolean.  True if only the key properties should be
            set on the generated instances.

        Possible Errors:
        CIM_ERR_FAILED (some other unspecified error occurred)

        """

        _scanmounts()
        dirs = ['/' + dir for dir in os.listdir('/')]
        for dir in ['/'] + dirs:
            stat = os.lstat(dir)
            get_keys(dir, model, linux_file=True, stat=stat)
            if keys_only:
                yield model
            else:
                try:
                    yield self.get_instance(env, model, cim_class, stat=stat)
                except pywbem.CIMError, (num, msg):
                    if num not in (pywbem.CIM_ERR_NOT_FOUND, 
                                   pywbem.CIM_ERR_ACCESS_DENIED):
                        raise

    def set_instance(self, env, instance, previous_instance, cim_class):
        """ Return a newly created or modified instance.

        Keyword arguments:
        env -- Provider Environment
        instance -- The new CIMInstance.  If modifying an existing instance,
            the properties on this instance have been filtered by the 
            PropertyList from the request.
        previous_instance -- The previous instance if modifying an existing 
            instance.  None if creating a new instance. 
        cim_class -- The CIMClass

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

        # TODO create or modify the instance
        if previous_instance is None:
            raise pywbem.CIMError(pywbem.CIM_ERR_NOT_SUPPORTED, 
                                  'CreateInstance not supported')
        stat = os.lstat(instance['lfname'])
        old_mode = new_mode = S_IMODE(stat[ST_MODE])
        new_uid = stat[ST_UID]
        new_gid = stat[ST_GID]
        if 'UserReadable' in instance.properties:
            new_mode = instance['UserReadable'] and \
                (new_mode | S_IRUSR) or (new_mode & ~S_IRUSR)
        if 'UserWritable' in instance.properties:
            new_mode = instance['UserWritable'] and \
                (new_mode | S_IWUSR) or (new_mode & ~S_IWUSR)
        if 'UserExecutable' in instance.properties:
            new_mode = instance['UserExecutable'] and \
                (new_mode | S_IXUSR) or (new_mode & ~S_IXUSR) 
        if 'GroupReadable' in instance.properties:
            new_mode = instance['GroupReadable'] and \
                (new_mode | S_IRGRP) or (new_mode & ~S_IRGRP)
        if 'GroupWritable' in instance.properties:
            new_mode = instance['GroupWritable'] and \
                (new_mode | S_IWGRP) or (new_mode & ~S_IWGRP)
        if 'GroupExecutable' in instance.properties:
            new_mode = instance['GroupExecutable'] and \
                (new_mode | S_IXGRP) or (new_mode & ~S_IXGRP)
        if 'WorldReadable' in instance.properties:
            new_mode = instance['WorldReadable'] and \
                (new_mode | S_IROTH) or (new_mode & ~S_IROTH)
        if 'WorldWritable' in instance.properties:
            new_mode = instance['WorldWritable'] and \
                (new_mode | S_IWOTH) or (new_mode & ~S_IWOTH)
        if 'WorldExecutable' in instance.properties:
            new_mode = instance['WorldExecutable'] and \
                (new_mode | S_IXOTH) or (new_mode & ~S_IXOTH)
        if 'SetUid' in instance.properties:
            new_mode = instance['SetUid'] and \
                (new_mode | S_ISUID) or (new_mode & ~S_ISUID)
        if 'SetGid' in instance.properties:
            new_mode = instance['SetGid'] and \
                (new_mode | S_ISGID) or (new_mode & ~S_ISGID)
        if 'SaveText' in instance.properties:
            new_mode = instance['SetGid'] and \
                (new_mode | S_ISVTX) or (new_mode & ~S_ISVTX)
        if 'UserID' in instance.properties:
            uid = instance['UserID']
            try:
                new_uid = pwd.getpwnam(uid)[2]
            except KeyError:
                if uid.isdigit():
                    new_uid = int(uid)
                else:
                    raise pywbem.CIMError(pywbem.CIM_ERR_INVALID_PARAMETER,
                                          "Unknown user: %s" % new_uid)
        if 'GroupID' in instance.properties:
            gid = instance['GroupID']
            try:
                new_gid = grp.getgrnam(gid)[2]
            except KeyError:
                if gid.isdigit():
                    new_gid = int(gid)
                else:
                    raise pywbem.CIMError(pywbem.CIM_ERR_INVALID_PARAMETER, 
                                          "Unknown group: %s" % new_gid)
        if new_mode != old_mode:
            os.chmod(instance['lfname'], new_mode)

        if new_uid != stat[ST_UID] or new_gid != stat[ST_GID]:
            os.chown(instance['lfname'], new_uid, new_gid)
        
        return instance

    def delete_instance(self, env, instance_name):
        """ Delete an instance.

        Keyword arguments:
        env -- Provider Environment
        instance_name -- A CIMInstanceName specifying the instance to delete.

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
        raise pywbem.CIMError(pywbem.CIM_ERR_NOT_SUPPORTED, '')

class CIM_LogicalFileProvider(CIMProvider):
    """Instrument the CIM class CIM_LogicalFile 

    A LogicalFile is a named collection of data or executable code, or
    represents a LogicalDevice or Directory. It is located within the
    context of a FileSystem, on a Storage Extent.
    
    """

    def __init__ (self):
        pass

    def get_instance (self, env, model, cim_class, stat=None):
        """Return an instance.

        Keyword arguments:
        env -- Provider Environment
        model -- A template of the CIMInstance to be returned.  The key 
            properties are set on this instance to correspond to the 
            instanceName that was requested.  The properties of the model
            are already filtered according to the PropertyList from the 
            request.
        cim_class -- The CIMClass

        Possible Errors:
        CIM_ERR_ACCESS_DENIED
        CIM_ERR_INVALID_PARAMETER (including missing, duplicate, unrecognized 
            or otherwise incorrect parameters)
        CIM_ERR_NOT_FOUND (the CIM Class does exist, but the requested CIM 
            Instance does not exist in the specified namespace)
        CIM_ERR_FAILED (some other unspecified error occurred)

        """

        logger = env.get_logger()
        logger.log_debug('Entering %s.get_instance()' % self.__class__.__name__)

        if stat is None:
            stat = os.lstat(model['name'])

        mode = stat[ST_MODE]
        model['Writeable'] = bool(mode & S_IWUSR)
        model['HealthState'] = pywbem.Uint16(5)
        #model['StatusDescriptions'] = # TODO (type = string[]) 
        model['Executable'] = bool(mode & S_IXUSR)
        model['Readable'] = bool(mode & S_IRUSR)
        #model['OperationalStatus'] = # TODO (type = uint16[]) 
        model['FileSize'] = pywbem.Uint64(stat[ST_SIZE])
        #model['CompressionMethod'] = # TODO (type = string) 
        #model['Status'] = # TODO (type = string) 
        #model['Description'] = # TODO (type = string) 
        #model['InstallDate'] = # TODO (type = datetime) 
        model['LastModified'] = pywbem.CIMDateTime.fromtimestamp(stat[ST_MTIME])
        #model['InUseCount'] = # TODO (type = uint64) 
        #model['EncryptionMethod'] = # TODO (type = string) 
        model['LastAccessed'] = pywbem.CIMDateTime.fromtimestamp(stat[ST_ATIME])
        model['ElementName'] = model['name'] == '/' and '/' or \
                    os.path.basename(model['name'])
        #model['Caption'] = # TODO (type = string) 
        model['CreationDate'] = pywbem.CIMDateTime.fromtimestamp(stat[ST_CTIME])
        return model

    def enum_instances(self, env, model, cim_class, keys_only):
        """ Enumerate instances.

        The WBEM operations EnumerateInstances and EnumerateInstanceNames
        are both mapped to this method. 
        This method is a python generator

        Keyword arguments:
        env -- Provider Environment
        model -- A template of the CIMInstances to be generated.  The 
            properties of the model are already filtered according to the 
            PropertyList from the request.
        cim_class -- The CIMClass
        keys_only -- A boolean.  True if only the key properties should be
            set on the generated instances.

        Possible Errors:
        CIM_ERR_FAILED (some other unspecified error occurred)

        """

        _scanmounts()
        if model.classname.lower() != 'py_linuxdirectory':
            return
        # Not practical to return all directories, so we only 
        # return the top level dirs.  Other directories and files
        # can be reached by navigation via the DirectoryContainsFile
        # association. 
        dirs = ['/' + dir for dir in os.listdir('/')]
        for dir in ['/'] + dirs:
            stat = os.lstat(dir)
            get_keys(dir, model, stat=stat)
            if keys_only:
                yield model
            else:
                try:
                    yield self.get_instance(env, model, cim_class, stat=stat)
                except pywbem.CIMError, (num, msg):
                    if num not in (pywbem.CIM_ERR_NOT_FOUND, 
                                   pywbem.CIM_ERR_ACCESS_DENIED):
                        raise

    def set_instance(self, env, instance, previous_instance, cim_class):
        """ Return a newly created or modified instance.

        Keyword arguments:
        env -- Provider Environment
        instance -- The new CIMInstance.  If modifying an existing instance,
            the properties on this instance have been filtered by the 
            PropertyList from the request.
        previous_instance -- The previous instance if modifying an existing 
            instance.  None if creating a new instance. 
        cim_class -- The CIMClass

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
        raise pywbem.CIMError(pywbem.CIM_ERR_NOT_SUPPORTED, 
                              'Modify the corresponding Py_LinuxFile instead')


    def delete_instance(self, env, instance_name):
        """ Delete an instance.

        Keyword arguments:
        env -- Provider Environment
        instance_name -- A CIMInstanceName specifying the instance to delete.

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
        raise pywbem.CIMError(pywbem.CIM_ERR_NOT_SUPPORTED, '')

class Py_LinuxFileIdentityProvider(CIMProvider):
    """Instrument the CIM class Py_LinuxFileIdentity 

    CIM_FileIdentity indicates that a UnixFile describes Unix-specific
    aspects of the various subclasses of LogicalFile. The association
    exists since it forces UnixFile to be weak to (scoped by) the
    LogicalFile. This is not true in the association's superclass,
    LogicalIdentity.
    
    """

    def __init__ (self):
        pass

    def get_instance (self, env, model, cim_class):
        """Return an instance.

        Keyword arguments:
        env -- Provider Environment
        model -- A template of the CIMInstance to be returned.  The key 
            properties are set on this instance to correspond to the 
            instanceName that was requested.  The properties of the model
            are already filtered according to the PropertyList from the 
            request.
        cim_class -- The CIMClass

        Possible Errors:
        CIM_ERR_ACCESS_DENIED
        CIM_ERR_INVALID_PARAMETER (including missing, duplicate, unrecognized 
            or otherwise incorrect parameters)
        CIM_ERR_NOT_FOUND (the CIM Class does exist, but the requested CIM 
            Instance does not exist in the specified namespace)
        CIM_ERR_FAILED (some other unspecified error occurred)

        """

        return model

    def enum_instances(self, env, model, cim_class, keys_only):
        """ Enumerate instances.

        The WBEM operations EnumerateInstances and EnumerateInstanceNames
        are both mapped to this method. 
        This method is a python generator

        Keyword arguments:
        env -- Provider Environment
        model -- A template of the CIMInstances to be generated.  The 
            properties of the model are already filtered according to the 
            PropertyList from the request.
        cim_class -- The CIMClass
        keys_only -- A boolean.  True if only the key properties should be
            set on the generated instances.

        Possible Errors:
        CIM_ERR_FAILED (some other unspecified error occurred)

        """

        pass

    def references(self, env, object_name, model, assoc_class, 
                   result_class_name, role, result_role, keys_only):
        """Instrument Associations.

        All four association-related operations (Associators, AssociatorNames, 
        References, ReferenceNames) are mapped to this method. 
        This method is a python generator

        Keyword arguments:
        env -- Provider Environment
        object_name -- A CIMInstanceName that defines the source CIM Object
            whose associated Objects are to be returned.
        model -- A template CIMInstance to serve as a model
            of the objects to be returned.  Only properties present on this
            model need to be returned. 
        assoc_class -- The CIMClass
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
        keys_only -- A boolean.  True if only the key properties should be
            set on the generated instances.

        Possible Errors:
        CIM_ERR_ACCESS_DENIED
        CIM_ERR_NOT_SUPPORTED
        CIM_ERR_INVALID_NAMESPACE
        CIM_ERR_INVALID_PARAMETER (including missing, duplicate, unrecognized 
            or otherwise incorrect parameters)
        CIM_ERR_FAILED (some other unspecified error occurred)

        """

        if object_name.classname.lower() == 'py_linuxfile':
            model['SameElement'] = object_name
            model['SystemElement'] = pywbem.CIMInstanceName(
                    classname=object_name['LFCreationClassName'],
                    namespace=object_name.namespace,
                    keybindings={
                        'Name':object_name['LFName'],
                        'FSName':object_name['FSName'],
                        'FSCreationClassName':object_name['FSCreationClassName'],
                        'CSName':getfqdn(),
                        'CSCreationClassName':object_name['CSCreationClassName'],
                        'CreationClassName':object_name['LFCreationClassName']})
            yield model
        else:
            model['SystemElement'] = object_name
            model['SameElement'] = pywbem.CIMInstanceName(
                    classname='Py_LinuxFile',
                    namespace=object_name.namespace,
                    keybindings={
                        'LFName':object_name['Name'],
                        'FSName':object_name['FSName'],
                        'FSCreationClassName':object_name['FSCreationClassName'],
                        'CSName':getfqdn(),
                        'CSCreationClassName':object_name['CSCreationClassName'],
                        'LFCreationClassName':object_name['CreationClassName']})
            yield model


class Py_LinuxDirectoryContainsFileProvider(CIMProvider):
    """Instrument the CIM class Py_LinuxDirectoryContainsFile 

    Specifies the hierarchical arrangement of LogicalFiles in a Directory.
    
    """

    def __init__ (self):
        pass

    def get_instance (self, env, model, cim_class):
        """Return an instance.

        Keyword arguments:
        env -- Provider Environment
        model -- A template of the CIMInstance to be returned.  The key 
            properties are set on this instance to correspond to the 
            instanceName that was requested.  The properties of the model
            are already filtered according to the PropertyList from the 
            request.
        cim_class -- The CIMClass

        Possible Errors:
        CIM_ERR_ACCESS_DENIED
        CIM_ERR_INVALID_PARAMETER (including missing, duplicate, unrecognized 
            or otherwise incorrect parameters)
        CIM_ERR_NOT_FOUND (the CIM Class does exist, but the requested CIM 
            Instance does not exist in the specified namespace)
        CIM_ERR_FAILED (some other unspecified error occurred)

        """

        return model

    def enum_instances(self, env, model, cim_class, keys_only):
        """ Enumerate instances.

        The WBEM operations EnumerateInstances and EnumerateInstanceNames
        are both mapped to this method. 
        This method is a python generator

        Keyword arguments:
        env -- Provider Environment
        model -- A template of the CIMInstances to be generated.  The 
            properties of the model are already filtered according to the 
            PropertyList from the request.
        cim_class -- The CIMClass
        keys_only -- A boolean.  True if only the key properties should be
            set on the generated instances.

        Possible Errors:
        CIM_ERR_FAILED (some other unspecified error occurred)

        """

        while False: # TODO more instances?
            # TODO fetch system resource
            # Key properties    
            #model['GroupComponent'] = # TODO (type = REF Py_LinuxDirectory (CIMInstanceName))    
            #model['PartComponent'] = # TODO (type = REF CIM_LogicalFile (CIMInstanceName))
            if keys_only:
                yield model
            else:
                try:
                    yield self.get_instance(env, model, cim_class)
                except pywbem.CIMError, (num, msg):
                    if num not in (pywbem.CIM_ERR_NOT_FOUND, 
                                   pywbem.CIM_ERR_ACCESS_DENIED):
                        raise

 
    def references(self, env, object_name, model, assoc_class, 
                   result_class_name, role, result_role, keys_only):
        """Instrument Associations.

        All four association-related operations (Associators, AssociatorNames, 
        References, ReferenceNames) are mapped to this method. 
        This method is a python generator

        Keyword arguments:
        env -- Provider Environment
        object_name -- A CIMInstanceName that defines the source CIM Object
            whose associated Objects are to be returned.
        model -- A template CIMInstance to serve as a model
            of the objects to be returned.  Only properties present on this
            model need to be returned. 
        assoc_class -- The CIMClass
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
        keys_only -- A boolean.  True if only the key properties should be
            set on the generated instances.

        Possible Errors:
        CIM_ERR_ACCESS_DENIED
        CIM_ERR_NOT_SUPPORTED
        CIM_ERR_INVALID_NAMESPACE
        CIM_ERR_INVALID_PARAMETER (including missing, duplicate, unrecognized 
            or otherwise incorrect parameters)
        CIM_ERR_FAILED (some other unspecified error occurred)

        """

        _scanmounts()
        objname = object_name['name']
        if (not role or role.lower() == 'partcomponent') \
                and objname != '/':
            model['partcomponent'] = object_name
            kbs = {}
            get_keys(os.path.dirname(objname), kbs)
            model['groupcomponent'] = pywbem.CIMInstanceName(
                    classname=kbs['CreationClassName'], 
                    namespace=object_name.namespace, 
                    keybindings=kbs)
            yield model
        if (not role or role.lower() == 'groupcomponent') \
                and object_name.classname.lower() == 'py_linuxdirectory':
            model['groupcomponent'] = object_name
            for dirent in os.listdir(objname):
                fname = objname
                if fname[-1] != '/':
                    fname += '/'
                fname = os.path.abspath(fname+dirent)
                kbs = {}
                get_keys(fname, kbs)
                model['partcomponent'] = pywbem.CIMInstanceName(
                        classname=kbs['CreationClassName'],
                        namespace=object_name.namespace,
                        keybindings=kbs)
                yield model


def get_providers(env):
    _cim_logicalfile_prov = CIM_LogicalFileProvider()
    _py_linuxfile_prov = Py_LinuxFileProvider()     
    _py_linuxfileidentity_prov = Py_LinuxFileIdentityProvider()
    _py_linuxdirectorycontainsfile_prov = Py_LinuxDirectoryContainsFileProvider() 

    return {'Py_LinuxFile': _py_linuxfile_prov,
            'Py_LinuxFileIdentity': _py_linuxfileidentity_prov,
            'Py_LinuxDirectoryContainsFile': _py_linuxdirectorycontainsfile_prov,
            'Py_LinuxDataFile':_cim_logicalfile_prov, 
            'Py_LinuxDeviceFile':_cim_logicalfile_prov, 
            'Py_LinuxDirectory':_cim_logicalfile_prov, 
            'Py_LinuxFIFOPipeFile':_cim_logicalfile_prov, 
            'Py_LinuxSymbolicLink':_cim_logicalfile_prov,
            'Py_LinuxSocketFile':_cim_logicalfile_prov}


