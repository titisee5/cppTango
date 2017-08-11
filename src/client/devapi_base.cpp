//===================================================================================================================
//
// devapi_base.cpp 	- C++ source code file for TANGO device api
//
// programmer(s)	- Andy Gotz (goetz@esrf.fr)
//
// original 		- March 2001
//
// Copyright (C) :      2001,2002,2003,2004,2005,2006,2007,2008,2009,2010,2011,2012,2013,2014,2015
//						European Synchrotron Radiation Facility
//                      BP 220, Grenoble 38043
//                      FRANCE
//
// This file is part of Tango.
//
// Tango is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Tango is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License along with Tango.
// If not, see <http://www.gnu.org/licenses/>.
//
// $Revision$
//
//===================================================================================================================

#if HAVE_CONFIG_H
#include <ac_config.h>
#endif

#include <tango.h>
#include <tango/client/eventconsumer.h>
#include <tango/client/devapi_utils.tpp>

#ifdef _TG_WINDOWS_
                                                                                                                        #include <sys/timeb.h>
#include <process.h>
#include <ws2tcpip.h>
#else

#include <sys/time.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pwd.h>

#endif /* _TG_WINDOWS_ */

#include <time.h>
#include <signal.h>
#include <algorithm>

using namespace CORBA;

namespace Tango
{

//-----------------------------------------------------------------------------
//
// ConnectionExt class methods:
//
// - assignement operator
//
//-----------------------------------------------------------------------------

Connection::ConnectionExt &
Connection::ConnectionExt::operator=(TANGO_UNUSED(const Connection::ConnectionExt &rval))
{
    return *this;
}


//-----------------------------------------------------------------------------
//
// Connection::Connection() - constructor to manage a connection to a device
//
//-----------------------------------------------------------------------------

Connection::Connection(ORB *orb_in)
    : pasyn_ctr(0), pasyn_cb_ctr(0),
      timeout(CLNT_TIMEOUT),
      version(0), source(Tango::CACHE_DEV), ext(new ConnectionExt()),
      tr_reco(true), prev_failed(false), prev_failed_t0(0.0),
      user_connect_timeout(-1), tango_host_localhost(false)
{

//
// Some default init for access control
//

    check_acc = true;
    access = ACCESS_READ;

//
// If the proxy is created from inside a device server, use the server orb
//

    ApiUtil *au = ApiUtil::instance();
    if ((orb_in == NULL) && (CORBA::is_nil(au->get_orb()) == true))
    {
        if (au->in_server() == true)
            ApiUtil::instance()->set_orb(Util::instance()->get_orb());
        else
            ApiUtil::instance()->create_orb();
    }
    else
    {
        if (orb_in != NULL)
            au->set_orb(orb_in);
    }

//
// Get user connect timeout if one is defined
//

    int ucto = au->get_user_connect_timeout();
    if (ucto != -1)
        user_connect_timeout = ucto;
}

Connection::Connection(bool dummy)
    : ext(Tango_nullptr), tr_reco(true), prev_failed(false), prev_failed_t0(0.0),
      user_connect_timeout(-1), tango_host_localhost(false)
{
    if (dummy)
    {
#ifdef HAS_UNIQUE_PTR
        ext.reset(new ConnectionExt());
#else
        ext = new ConnectionExt();
#endif
    }
}

//-----------------------------------------------------------------------------
//
// Connection::~Connection() - destructor to destroy connection to TANGO device
//
//-----------------------------------------------------------------------------

Connection::~Connection()
{
#ifndef HAS_UNIQUE_PTR
    delete ext;
#endif
}

//-----------------------------------------------------------------------------
//
// Connection::Connection() - copy constructor
//
//-----------------------------------------------------------------------------

Connection::Connection(const Connection &sou)
    : ext(Tango_nullptr)
{
    dbase_used = sou.dbase_used;
    from_env_var = sou.from_env_var;
    host = sou.host;
    port = sou.port;
    port_num = sou.port_num;

    db_host = sou.db_host;
    db_port = sou.db_port;
    db_port_num = sou.db_port_num;

    ior = sou.ior;
    pasyn_ctr = sou.pasyn_ctr;
    pasyn_cb_ctr = sou.pasyn_cb_ctr;

    device = sou.device;
    if (sou.version >= 2)
        device_2 = sou.device_2;

    timeout = sou.timeout;
    connection_state = sou.connection_state;
    version = sou.version;
    source = sou.source;

    check_acc = sou.check_acc;
    access = sou.access;

    tr_reco = sou.tr_reco;
    device_3 = sou.device_3;

    prev_failed = sou.prev_failed;
    prev_failed_t0 = sou.prev_failed_t0;

    device_4 = sou.device_4;

    user_connect_timeout = sou.user_connect_timeout;
    tango_host_localhost = sou.tango_host_localhost;

    device_5 = sou.device_5;
    device_6 = sou.device_6;

#ifdef HAS_UNIQUE_PTR
    if (sou.ext.get() != NULL)
    {
        ext.reset(new ConnectionExt);
        *(ext.get()) = *(sou.ext.get());
    }
#else
                                                                                                                            if (sou.ext != NULL)
	{
		ext = new ConnectionExt();
		*ext = *(sou.ext);
	}
	else
		ext = NULL;
#endif
}

//-----------------------------------------------------------------------------
//
// Connection::operator=() - assignement operator
//
//-----------------------------------------------------------------------------

Connection &Connection::operator=(const Connection &rval)
{
    dbase_used = rval.dbase_used;
    from_env_var = rval.from_env_var;
    host = rval.host;
    port = rval.port;
    port_num = rval.port_num;

    db_host = rval.db_host;
    db_port = rval.db_port;
    db_port_num = rval.db_port_num;

    ior = rval.ior;
    pasyn_ctr = rval.pasyn_ctr;
    pasyn_cb_ctr = rval.pasyn_cb_ctr;

    device = rval.device;
    if (rval.version >= 2)
        device_2 = rval.device_2;

    timeout = rval.timeout;
    connection_state = rval.connection_state;
    version = rval.version;
    source = rval.source;

    check_acc = rval.check_acc;
    access = rval.access;

    tr_reco = rval.tr_reco;
    device_3 = rval.device_3;

    prev_failed = rval.prev_failed;
    prev_failed_t0 = rval.prev_failed_t0;

    device_4 = rval.device_4;

    user_connect_timeout = rval.user_connect_timeout;
    tango_host_localhost = rval.tango_host_localhost;

    device_5 = rval.device_5;
    device_6 = rval.device_6;


#ifdef HAS_UNIQUE_PTR
    if (rval.ext.get() != NULL)
    {
        ext.reset(new ConnectionExt);
        *(ext.get()) = *(rval.ext.get());
    }
    else
        ext.reset(Tango_nullptr);
#else
                                                                                                                            if (rval.ext != NULL)
	{
		ext = new ConnectionExt();
		*ext = *(rval.ext);
	}
	else
		ext = NULL;
#endif

    return *this;
}

//-----------------------------------------------------------------------------
//
// Connection::check_and_reconnect() methods family
//
// Check if a re-connection is needed and if true, try to reconnect.
// These meethods also manage a TangoMonitor object for thread safety.
// Some of them set parameters while the object is locked in order to pass
// them to the caller in thread safe way.
//
//-----------------------------------------------------------------------------

void Connection::check_and_reconnect()
{
    int local_connection_state;
    {
        ReaderLock guard(con_to_mon);
        local_connection_state = connection_state;
    }
    if (local_connection_state != CONNECTION_OK)
    {
        WriterLock guard(con_to_mon);
        if (connection_state != CONNECTION_OK)
            reconnect(dbase_used);
    }
}

void Connection::check_and_reconnect(Tango::DevSource &sou)
{
    int local_connection_state;
    {
        ReaderLock guard(con_to_mon);
        local_connection_state = connection_state;
        sou = source;
    }
    if (local_connection_state != CONNECTION_OK)
    {
        WriterLock guard(con_to_mon);
        if (connection_state != CONNECTION_OK)
            reconnect(dbase_used);
    }
}

void Connection::check_and_reconnect(Tango::AccessControlType &act)
{
    int local_connection_state;
    {
        ReaderLock guard(con_to_mon);
        local_connection_state = connection_state;
        act = access;
    }
    if (local_connection_state != CONNECTION_OK)
    {
        WriterLock guard(con_to_mon);
        if (connection_state != CONNECTION_OK)
        {
            reconnect(dbase_used);
            act = access;
        }
    }
}

void Connection::check_and_reconnect(Tango::DevSource &sou, Tango::AccessControlType &act)
{
    int local_connection_state;
    {
        ReaderLock guard(con_to_mon);
        local_connection_state = connection_state;
        act = access;
        sou = source;
    }
    if (local_connection_state != CONNECTION_OK)
    {
        WriterLock guard(con_to_mon);
        if (connection_state != CONNECTION_OK)
        {
            reconnect(dbase_used);
            act = access;
        }
    }
}

void Connection::set_connection_state(int con)
{
    WriterLock guard(con_to_mon);
    connection_state = con;
}

Tango::DevSource Connection::get_source()
{
    ReaderLock guard(con_to_mon);
    return source;
}

void Connection::set_source(Tango::DevSource sou)
{
    WriterLock guard(con_to_mon);
    source = sou;
}

//-----------------------------------------------------------------------------
//
// Connection::connect() - method to create connection to a TANGO device
//		using its stringified CORBA reference i.e. IOR or corbaloc
//
//-----------------------------------------------------------------------------

void Connection::connect(string &corba_name)
{
    bool retry = true;
    long db_retries = DB_START_PHASE_RETRIES;
    bool connect_to_db = false;

    while (retry == true)
    {
        try
        {

            Object_var obj;
            obj = ApiUtil::instance()->get_orb()->string_to_object(corba_name.c_str());
//TODO this should be dramatically refactored
// some thoughts: extract ConnectionFactory; make Connection interface implemented by Connection_V1, Connection_V2 etc; inject Connection (or Factory) into DeviceProxy
//
// Narrow CORBA string name to CORBA object
// First, try as a Device_5, then as a Device_4, then as .... and finally as a Device
//
// But we have want to know if the connection to the device is OK or not.
// The _narrow() call does not necessary generates a remote call. It all depends on the object IDL type
// stored in the IOR. If in the IOR, the IDL is the same than the one on which the narrow is done (Device_5 on both
// side for instance), then the _narrow call will not generate any remote call and therefore, we don know
// if the connection is OK or NOT. This is the reason of the _non_existent() call.
// In case the IDl in the IOR and in the narrow() call are different, then the _narrow() call try to execute a
// remote _is_a() call and therefore tries to connect to the device. IN this case, the _non_existent() call is
// useless. But because we don want to analyse the IOR ourself, we always call _non_existent()
// Reset the connection timeout only after the _non_existent call.
//

            if (corba_name.find(DbObjName) != string::npos)
                connect_to_db = true;

            if (connect_to_db == false)
            {
                if (user_connect_timeout != -1)
                    omniORB::setClientConnectTimeout(user_connect_timeout);
                else
                    omniORB::setClientConnectTimeout(NARROW_CLNT_TIMEOUT);
            }

            resolve_obj_version(corba_name, obj);

            //
// Warning! Some non standard code (omniORB specific).
// Set a flag if the object is running on a host with several net addresses. This is used during re-connection
// algo.
//

            if (corba_name[0] == 'I' && corba_name[1] == 'O' && corba_name[2] == 'R')
            {
                IOP::IOR ior;
                toIOR(corba_name.c_str(), ior);
                IIOP::ProfileBody pBody;
                IIOP::unmarshalProfile(ior.profiles[0], pBody);

                DevULong total = pBody.components.length();

                for (DevULong index = 0; index < total; index++)
                {
                    IOP::TaggedComponent &c = pBody.components[index];
                    if (c.tag == 3)
                    {
                        ext->has_alt_adr = true;
                        break;
                    }
                    else
                        ext->has_alt_adr = false;
                }
            }

//			if (connect_to_db == false)
//				omniORB::setClientConnectTimeout(0);
            retry = false;

//
// Mark the connection as OK and set timeout to its value
// (The default is 3 seconds)
//

            connection_state = CONNECTION_OK;
            if (timeout != CLNT_TIMEOUT)
                set_timeout_millis(timeout);
        }
        catch (CORBA::SystemException &ce)
        {
//			if (connect_to_db == false)
//				omniORB::setClientConnectTimeout(0);

            TangoSys_OMemStream desc;
            TangoSys_MemStream reason;
            bool db_connect = false;

            desc << "Failed to connect to ";

            string::size_type pos = corba_name.find(':');
            if (pos == string::npos)
            {
                desc << "device " << dev_name() << ends;
                reason << API_CantConnectToDevice << ends;
            }
            else
            {
                string prot = corba_name.substr(0, pos);
                if (prot == "corbaloc")
                {
                    string::size_type tmp = corba_name.find('/');
                    if (tmp != string::npos)
                    {
                        string dev = corba_name.substr(tmp + 1);
                        if (dev == "database")
                        {
                            desc << "database on host ";
                            desc << db_host << " with port ";
                            desc << db_port << ends;
                            reason << "API_CantConnectToDatabase" << ends;
                            db_retries--;
                            if (db_retries != 0)
                                db_connect = true;
                        }
                        else
                        {
                            desc << "device " << dev_name() << ends;
                            if (CORBA::OBJECT_NOT_EXIST::_downcast(&ce) != 0)
                                reason << "API_DeviceNotDefined" << ends;
                            else if (CORBA::TRANSIENT::_downcast(&ce) != 0)
                                reason << "API_ServerNotRunning" << ends;
                            else
                                reason << API_CantConnectToDevice << ends;
                        }
                    }
                    else
                    {
                        desc << "device " << dev_name() << ends;
                        reason << API_CantConnectToDevice << ends;
                    }
                }
                else
                {
                    desc << "device " << dev_name() << ends;
                    reason << API_CantConnectToDevice << ends;
                }
            }

            if (db_connect == false)
            {
                ApiConnExcept::re_throw_exception(ce, reason.str(), desc.str(),
                                                  (const char *) "Connection::connect");
            }
        }
    }
}

void Connection::resolve_obj_version(const string &corba_name, const Object_var &obj)
{
    //TODO extract template or Macro or class hierarchy, 'cauz this code clearly is a duplication
    device_6 = Device_6::_narrow(obj);

    if (not is_nil(device_6))
    {
        version = 6;
        device_6->_non_existent();
        device_5 = Device_6::_duplicate(device_6);
        device_4 = Device_6::_duplicate(device_6);
        device_3 = Device_6::_duplicate(device_6);
        device_2 = Device_6::_duplicate(device_6);
        device = Device_6::_duplicate(device_6);
        return;
    }

    device_5 = Device_5::_narrow(obj);

    if (not is_nil(device_5))
    {
        version = 5;
        device_5->_non_existent();
        device_4 = Device_5::_duplicate(device_5);
        device_3 = Device_5::_duplicate(device_5);
        device_2 = Device_5::_duplicate(device_5);
        device = Device_5::_duplicate(device_5);
        return;
    }

    device_4 = Device_4::_narrow(obj);

    if (not is_nil(device_4))
    {
        version = 4;
        device_4->_non_existent();
        device_3 = Device_4::_duplicate(device_4);
        device_2 = Device_4::_duplicate(device_4);
        device = Device_4::_duplicate(device_4);
        return;
    }

    device_3 = Device_3::_narrow(obj);

    if (not is_nil(device_3))
    {
        version = 3;
        device_3->_non_existent();
        device_2 = Device_3::_duplicate(device_3);
        device = Device_3::_duplicate(device_3);
        return;
    }

    device_2 = Device_2::_narrow(obj);

    if (not is_nil(device_2))
    {
        version = 2;
        device_2->_non_existent();
        device = Device_2::_duplicate(device_2);
        return;
    }

    device = Device::_narrow(obj);

    if (not is_nil(device))
    {
        version = 1;
        device->_non_existent();
        return;
    }

    cerr << "Can't build connection to object " << corba_name << endl;
    connection_state = CONNECTION_NOTOK;

    TangoSys_OMemStream desc;
    desc << "Failed to connect to device " << dev_name();
    desc << " (device nil after _narrowing)" << ends;
    ApiConnExcept::throw_exception((const char *) API_CantConnectToDevice,
                                   desc.str(),
                                   (const char *) "Connection::resolve_obj_version()");
}


//-----------------------------------------------------------------------------
//
// Connection::toIOR() - Convert string IOR to omniORB IOR object
// omniORB specific code !!!
//
//-----------------------------------------------------------------------------


void Connection::toIOR(const char *iorstr, IOP::IOR &ior)
{
    size_t s = (iorstr ? strlen(iorstr) : 0);
    if (s < 4)
        throw CORBA::MARSHAL(0, CORBA::COMPLETED_NO);
    const char *p = iorstr;
    if (p[0] != 'I' ||
        p[1] != 'O' ||
        p[2] != 'R' ||
        p[3] != ':')
        throw CORBA::MARSHAL(0, CORBA::COMPLETED_NO);

    s = (s - 4) / 2;  // how many octets are there in the string
    p += 4;

    cdrMemoryStream buf((DevULong) s, 0);

    for (int i = 0; i < (int) s; i++)
    {
        int j = i * 2;
        DevUChar v;

        if (p[j] >= '0' && p[j] <= '9')
        {
            v = ((p[j] - '0') << 4);
        }
        else if (p[j] >= 'a' && p[j] <= 'f')
        {
            v = ((p[j] - 'a' + 10) << 4);
        }
        else if (p[j] >= 'A' && p[j] <= 'F')
        {
            v = ((p[j] - 'A' + 10) << 4);
        }
        else
            throw CORBA::MARSHAL(0, CORBA::COMPLETED_NO);

        if (p[j + 1] >= '0' && p[j + 1] <= '9')
        {
            v += (p[j + 1] - '0');
        }
        else if (p[j + 1] >= 'a' && p[j + 1] <= 'f')
        {
            v += (p[j + 1] - 'a' + 10);
        }
        else if (p[j + 1] >= 'A' && p[j + 1] <= 'F')
        {
            v += (p[j + 1] - 'A' + 10);
        }
        else
            throw CORBA::MARSHAL(0, CORBA::COMPLETED_NO);

        buf.marshalOctet(v);
    }

    buf.rewindInputPtr();
    DevBoolean b = buf.unmarshalBoolean();
    buf.setByteSwapFlag(b);

    ior.type_id = IOP::IOR::unmarshaltype_id(buf);
    ior.profiles <<= buf;
}


//-----------------------------------------------------------------------------
//
// Connection::reconnect() - reconnect to a CORBA object
//
//-----------------------------------------------------------------------------

void Connection::reconnect(bool db_used)
{
    struct timeval now;
#ifndef _TG_WINDOWS_
    gettimeofday(&now, NULL);
#else
                                                                                                                            struct _timeb now_win;
	_ftime(&now_win);
	now.tv_sec = (unsigned long)now_win.time;
	now.tv_usec = (long)now_win.millitm * 1000;
#endif /* _TG_WINDOWS_ */

    double t = (double) now.tv_sec + ((double) now.tv_usec / 1000000);
    double delay = t - prev_failed_t0;

    if (connection_state != CONNECTION_OK)
    {
        //	Do not reconnect if to soon
        if ((prev_failed == true) && delay < (RECONNECTION_DELAY / 1000))
        {
            TangoSys_OMemStream desc;
            desc << "Failed to connect to device " << dev_name() << endl;
            desc << "The connection request was delayed." << endl;
            desc << "The last connection request was done less than " << RECONNECTION_DELAY << " ms ago" << ends;

            Tango::Except::throw_exception((const char *) API_CantConnectToDevice,
                                           desc.str(),
                                           (const char *) "Connection::reconnect");
        }
    }

    try
    {
        string corba_name;
        if (connection_state != CONNECTION_OK)
        {
            if (db_used == true)
            {
                corba_name = get_corba_name(check_acc);
                if (check_acc == false)
                {
                    ApiUtil *au = ApiUtil::instance();
                    int db_num;
                    if (get_from_env_var() == true)
                        db_num = au->get_db_ind();
                    else
                        db_num = au->get_db_ind(get_db_host(), get_db_port_num());
                    (au->get_db_vect())[db_num]->clear_access_except_errors();
                }
            }
            else
                corba_name = build_corba_name();

            connect(corba_name);
        }

//
// Try to ping the device. With omniORB, it is possible that the first
// real access to the device is done when a call to one of the interface
// operation is done. Do it now.
//

        if (connection_state == CONNECTION_OK)
        {
            try
            {
//				if (user_connect_timeout != -1)
//					omniORB::setClientConnectTimeout(user_connect_timeout);
//				else
//					omniORB::setClientConnectTimeout(NARROW_CLNT_TIMEOUT);
                device->ping();
//				omniORB::setClientConnectTimeout(0);

                prev_failed_t0 = t;
                prev_failed = false;

//
// If the device is the database, call its post-reconnection method
//
// TODO: Implement this with a virtual method in Connection and Database
// class. Doing it now, will break compatibility (one more virtual method)
//

                if (corba_name.find("database") != string::npos)
                {
                    static_cast<Database *>(this)->post_reconnection();
                }
            }
            catch (CORBA::SystemException &ce)
            {
//				omniORB::setClientConnectTimeout(0);
                connection_state = CONNECTION_NOTOK;

                TangoSys_OMemStream desc;
                desc << "Failed to connect to device " << dev_name() << ends;

                ApiConnExcept::re_throw_exception(ce,
                                                  (const char *) API_CantConnectToDevice,
                                                  desc.str(),
                                                  (const char *) "Connection::reconnect");
            }
        }
    }
    catch (DevFailed &)
    {
        prev_failed = true;
        prev_failed_t0 = t;

        throw;
    }
}

//-----------------------------------------------------------------------------
//
// Connection::is_connected() - returns true if connection is in the OK state
//
//-----------------------------------------------------------------------------

bool Connection::is_connected()
{
    bool connected = true;
    ReaderLock guard(con_to_mon);
    if (connection_state != CONNECTION_OK)
        connected = false;
    return connected;
}

//-----------------------------------------------------------------------------
//
// Connection::get_env_var() - Get an environment variable
//
// This method get an environment variable value from different source.
// which are (orderd by piority)
//
// 1 - A real environement variable
// 2 - A file ".tangorc" in the user home directory
// 3 - A file "/etc/tangorc"
//
// in :	- env_var_name : The environment variable name
//
// out : - env_var : The string initialised with the env. variable value
//
// This method returns 0 of the env. variable is found. Otherwise, it returns -1
//
//-----------------------------------------------------------------------------

int Connection::get_env_var(const char *env_var_name, string &env_var)
{
    int ret = -1;
    char *env_c_str;

//
// try to get it as a classical env. variable
//

    env_c_str = getenv(env_var_name);

    if (env_c_str == NULL)
    {
#ifndef _TG_WINDOWS_
        uid_t user_id = geteuid();

        struct passwd pw;
        struct passwd *pw_ptr;
        char buffer[1024];

        if (getpwuid_r(user_id, &pw, buffer, sizeof(buffer), &pw_ptr) != 0)
        {
            return ret;
        }

        if (pw_ptr == NULL)
        {
            return ret;
        }

//
// Try to get it from the user home dir file
//

        string home_file(pw.pw_dir);
        home_file = home_file + "/" + USER_ENV_VAR_FILE;

        int local_ret;
        string local_env_var;
        local_ret = get_env_var_from_file(home_file, env_var_name, local_env_var);

        if (local_ret == 0)
        {
            env_var = local_env_var;
            ret = 0;
        }
        else
        {

//
// Try to get it from a host defined file
//

            home_file = TANGO_RC_FILE;
            local_ret = get_env_var_from_file(home_file, env_var_name, local_env_var);
            if (local_ret == 0)
            {
                env_var = local_env_var;
                ret = 0;
            }
        }
#else
                                                                                                                                char *env_tango_root;

		env_tango_root = getenv(WindowsEnvVariable);
		if (env_tango_root != NULL)
		{
			string home_file(env_tango_root);
			home_file = home_file + "/" + WINDOWS_ENV_VAR_FILE;

			int local_ret;
			string local_env_var;
			local_ret = get_env_var_from_file(home_file,env_var_name,local_env_var);

			if (local_ret == 0)
			{
				env_var = local_env_var;
				ret = 0;
			}
		}

#endif
    }
    else
    {
        env_var = env_c_str;
        ret = 0;
    }

    return ret;
}

//-----------------------------------------------------------------------------
//
// Connection::get_env_var_from_file() - Get an environment variable from a file
//
// in :	- env_var : The environment variable name
//		- f_name : The file name
//
// out : - ret_env_var : The string initialised with the env. variable value
//
// This method returns 0 of the env. variable is found. Otherwise, it returns -1
//
//-----------------------------------------------------------------------------

int Connection::get_env_var_from_file(string &f_name, const char *env_var, string &ret_env_var)
{
    ifstream inFile;
    string file_line;
    string var(env_var);
    int ret = -1;

    inFile.open(f_name.c_str());
    if (!inFile)
    {
        return ret;
    }

    transform(var.begin(), var.end(), var.begin(), ::tolower);

    string::size_type pos_env, pos_comment;

    while (!inFile.eof())
    {
        getline(inFile, file_line);
        transform(file_line.begin(), file_line.end(), file_line.begin(), ::tolower);

        if ((pos_env = file_line.find(var)) != string::npos)
        {
            pos_comment = file_line.find('#');
            if ((pos_comment != string::npos) && (pos_comment < pos_env))
                continue;

            string::size_type pos;
            if ((pos = file_line.find('=')) != string::npos)
            {
                string tg_host = file_line.substr(pos + 1);
                string::iterator end_pos = remove(tg_host.begin(), tg_host.end(), ' ');
                tg_host.erase(end_pos, tg_host.end());

                ret_env_var = tg_host;
                ret = 0;
                break;
            }
        }
    }

    inFile.close();
    return ret;
}

//-------------------------------------------------------------------------------------------------------------------
//
// method:
//		Connection::get_fqdn()
//
// description:
// 		This method gets the host fully qualified domain name (from DNS) and modified the passed string accordingly
//
// argument:
// 		in/out :
//			- the_host: The original host name
//
//------------------------------------------------------------------------------------------------------------------

void Connection::get_fqdn(string &the_host)
{

//
// If the host name we received is the name of the host we are running on,
// set a flag
//

    char buffer[80];
    bool local_host = false;

    if (gethostname(buffer, 80) == 0)
    {
        if (::strcmp(buffer, the_host.c_str()) == 0)
            local_host = true;
    }

    struct addrinfo hints;

    memset(&hints, 0, sizeof(struct addrinfo));

    hints.ai_flags = AI_ADDRCONFIG;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo *info;
    struct addrinfo *ptr;
    char tmp_host[512];
    bool host_found = false;
    vector<string> ip_list;

//
// If we are running on local host, get IP address(es) from NIC board
//

    if (local_host == true)
    {
        ApiUtil *au = ApiUtil::instance();
        au->get_ip_from_if(ip_list);
        hints.ai_flags |= AI_NUMERICHOST;
    }
    else
        ip_list.push_back(the_host);

//
// Try to get FQDN
//

    size_t i;
    for (i = 0; i < ip_list.size() && !host_found; i++)
    {
        int result = getaddrinfo(ip_list[i].c_str(), NULL, &hints, &info);

        if (result == 0)
        {
            ptr = info;
            int nb_loop = 0;
            string myhost;
            string::size_type pos;

            while (ptr != NULL)
            {
                if (getnameinfo(ptr->ai_addr, ptr->ai_addrlen, tmp_host, 512, 0, 0, NI_NAMEREQD) == 0)
                {
                    nb_loop++;
                    myhost = tmp_host;
                    pos = myhost.find('.');
                    if (pos != string::npos)
                    {
                        string canon = myhost.substr(0, pos);
                        if (canon == the_host)
                        {
                            the_host = myhost;
                            host_found = true;
                            break;
                        }
                    }
                }
                ptr = ptr->ai_next;
            }
            freeaddrinfo(info);

            if (host_found == false && nb_loop == 1 && i == (ip_list.size() - 1))
            {
                the_host = myhost;
            }
        }
    }
}

//-----------------------------------------------------------------------------
//
// Connection::get_timeout_millis() - public method to get timeout on a TANGO device
//
//-----------------------------------------------------------------------------

int Connection::get_timeout_millis()
{
    ReaderLock guard(con_to_mon);
    return timeout;
}


//-----------------------------------------------------------------------------
//
// Connection::set_timeout_millis() - public method to set timeout on a TANGO device
//
//-----------------------------------------------------------------------------

void Connection::set_timeout_millis(int millisecs)
{
    WriterLock guard(con_to_mon);

    timeout = millisecs;

    try
    {
        if (connection_state != CONNECTION_OK)
            reconnect(dbase_used);

        switch (version)
        {
            case 6:
                omniORB::setClientCallTimeout(device_6, millisecs);
            case 5:
                omniORB::setClientCallTimeout(device_5, millisecs);
            case 4:
                omniORB::setClientCallTimeout(device_4, millisecs);
            case 3:
                omniORB::setClientCallTimeout(device_3, millisecs);
            case 2:
                omniORB::setClientCallTimeout(device_2, millisecs);
            default:
                omniORB::setClientCallTimeout(device, millisecs);
        }
    }
    catch (Tango::DevFailed &)
    {}
}


//-----------------------------------------------------------------------------
//
// Connection::command_inout() - public method to execute a command on a TANGO device
//
//-----------------------------------------------------------------------------


DeviceData Connection::command_inout(string &command)
{
    DeviceData data_in;

    return (command_inout(command, data_in));
}

//-----------------------------------------------------------------------------
//
// Connection::command_inout() - public method to execute a command on a TANGO device
//
//-----------------------------------------------------------------------------


DeviceData Connection::command_inout(string &command, DeviceData &data_in)
{
//
// We are using a pointer to an Any as the return value of the command_inout
// call. This is because the assignament to the Any_var any in the
// DeviceData object in faster in this case (no copy).
// Don't forget that the any_var in the DeviceData takes ownership of the
// memory allocated
//

    DeviceData data_out;
    int ctr = 0;
    DevSource local_source;
    AccessControlType local_act;

    while (ctr < 2)
    {
        try
        {
            check_and_reconnect(local_source, local_act);

//
// Manage control access in case the access right
// is READ_ONLY. We need to check if the command is a
// "READ" command or not
//

            if (local_act == ACCESS_READ)
            {
                ApiUtil *au = ApiUtil::instance();

                vector<Database *> &v_d = au->get_db_vect();
                Database *db;
                if (v_d.empty() == true)
                    db = static_cast<Database *>(this);
                else
                {
                    int db_num;

                    if (get_from_env_var() == true)
                        db_num = au->get_db_ind();
                    else
                        db_num = au->get_db_ind(get_db_host(), get_db_port_num());
                    db = v_d[db_num];
/*					if (db->is_control_access_checked() == false)
						db = static_cast<Database *>(this);*/
                }

//
// If the command is not allowed, throw exception
// Also throw exception if it was not possible to get the list
// of allowed commands from the control access service
//
// The ping rule is simply to send to the client correct
// error message in case of re-connection
//

                string d_name = dev_name();

                if (db->is_command_allowed(d_name, command) == false)
                {
                    try
                    {
                        Device_var dev = Device::_duplicate(device);
                        dev->ping();
                    }
                    catch (...)
                    {
                        set_connection_state(CONNECTION_NOTOK);
                        throw;
                    }

                    DevErrorList &e = db->get_access_except_errors();
/*					if (e.length() != 0)
					{
						DevFailed df(e);
						throw df;
					}*/

                    TangoSys_OMemStream desc;
                    if (e.length() == 0)
                        desc << "Command " << command << " on device " << dev_name() << " is not authorized"
                             << ends;
                    else
                    {
                        desc << "Command " << command << " on device " << dev_name()
                             << " is not authorized because an error occurs while talking to the Controlled Access Service"
                             << ends;
                        string ex(e[0].desc);
                        if (ex.find("defined") != string::npos)
                            desc << "\n" << ex;
                        desc << ends;
                    }

                    NotAllowedExcept::throw_exception((const char *) API_ReadOnlyMode, desc.str(),
                                                      (const char *) "Connection::command_inout()");
                }
            }

//
// Now, try to execute the command
//

            CORBA::Any *received;
            if (version >= 4)
            {
                ClntIdent ci;
                ApiUtil *au = ApiUtil::instance();
                ci.cpp_clnt(au->get_client_pid());

                Device_4_var dev = Device_4::_duplicate(device_4);
                received = dev->command_inout_4(command.c_str(), data_in.any, local_source, ci);
            }
            else if (version >= 2)
            {
                Device_2_var dev = Device_2::_duplicate(device_2);
                received = dev->command_inout_2(command.c_str(), data_in.any, local_source);
            }
            else
            {
                Device_var dev = Device::_duplicate(device);
                received = dev->command_inout(command.c_str(), data_in.any);
            }

            ctr = 2;
            data_out.any = received;
        }
        catch (Tango::ConnectionFailed &e)
        {
            TangoSys_OMemStream desc;
            desc << "Failed to execute command_inout on device " << dev_name();
            desc << ", command " << command << ends;
            ApiConnExcept::re_throw_exception(e, (const char *) API_CommandFailed,
                                              desc.str(), (const char *) "Connection::command_inout()");
        }
        catch (Tango::DevFailed &e)
        {
            TangoSys_OMemStream desc;
            desc << "Failed to execute command_inout on device " << dev_name();
            desc << ", command " << command << ends;

            if (::strcmp(e.errors[0].reason, DEVICE_UNLOCKED_REASON) == 0)
                DeviceUnlockedExcept::re_throw_exception(e, (const char *) DEVICE_UNLOCKED_REASON,
                                                         desc.str(), (const char *) "Connection::command_inout()");
            else
                Except::re_throw_exception(e, (const char *) API_CommandFailed,
                                           desc.str(), (const char *) "Connection::command_inout()");
        }
        catch (CORBA::TRANSIENT &trans)
        {
            TRANSIENT_NOT_EXIST_EXCEPT_CMD(trans);
        }
        catch (CORBA::OBJECT_NOT_EXIST &one)
        {
            if (one.minor() == omni::OBJECT_NOT_EXIST_NoMatch || one.minor() == 0)
            {
                TRANSIENT_NOT_EXIST_EXCEPT_CMD(one);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute command_inout on device " << dev_name();
                desc << ", command " << command << ends;
                ApiCommExcept::re_throw_exception(one,
                                                  (const char *) "API_CommunicationFailed",
                                                  desc.str(),
                                                  (const char *) "Connection::command_inout()");
            }
        }
        catch (CORBA::COMM_FAILURE &comm)
        {
            if (comm.minor() == omni::COMM_FAILURE_WaitingForReply)
            {
                TRANSIENT_NOT_EXIST_EXCEPT_CMD(comm);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute command_inout on device " << dev_name();
                desc << ", command " << command << ends;
                ApiCommExcept::re_throw_exception(comm,
                                                  (const char *) "API_CommunicationFailed",
                                                  desc.str(),
                                                  (const char *) "Connection::command_inout()");
            }
        }
        catch (CORBA::SystemException &ce)
        {
            set_connection_state(CONNECTION_NOTOK);

            TangoSys_OMemStream desc;
            desc << "Failed to execute command_inout on device " << dev_name();
            desc << ", command " << command << ends;
            ApiCommExcept::re_throw_exception(ce,
                                              (const char *) "API_CommunicationFailed",
                                              desc.str(),
                                              (const char *) "Connection::command_inout()");
        }
    }

    return data_out;
}

//-----------------------------------------------------------------------------
//
// Connection::command_inout() - public method to execute a command on a TANGO device
//				 using low level CORBA types
//
//-----------------------------------------------------------------------------


CORBA::Any_var Connection::command_inout(string &command, CORBA::Any &any)
{
    int ctr = 0;
    Tango::DevSource local_source;
    Tango::AccessControlType local_act;

    while (ctr < 2)
    {
        try
        {
            check_and_reconnect(local_source, local_act);

//
// Manage control access in case the access right
// is READ_ONLY. We need to check if the command is a
// "READ" command or not
//

            if (local_act == ACCESS_READ)
            {
                ApiUtil *au = ApiUtil::instance();

                vector<Database *> &v_d = au->get_db_vect();
                Database *db;
                if (v_d.empty() == true)
                    db = static_cast<Database *>(this);
                else
                {
                    int db_num;

                    if (get_from_env_var() == true)
                        db_num = au->get_db_ind();
                    else
                        db_num = au->get_db_ind(get_db_host(), get_db_port_num());
                    db = v_d[db_num];
/*					if (db->is_control_access_checked() == false)
						db = static_cast<Database *>(this);*/
                }

//
// If the command is not allowed, throw exception
// Also throw exception if it was not possible to get the list
// of allowed commands from the control access service
//
// The ping rule is simply to send to the client correct
// error message in case of re-connection
//

                string d_name = dev_name();
                if (db->is_command_allowed(d_name, command) == false)
                {
                    try
                    {
                        Device_var dev = Device::_duplicate(device);
                        dev->ping();
                    }
                    catch (...)
                    {
                        set_connection_state(CONNECTION_NOTOK);
                        throw;
                    }

                    DevErrorList &e = db->get_access_except_errors();
/*					if (e.length() != 0)
					{
						DevFailed df(e);
						throw df;
					}*/

                    TangoSys_OMemStream desc;
                    if (e.length() == 0)
                        desc << "Command " << command << " on device " << dev_name() << " is not authorized"
                             << ends;
                    else
                    {
                        desc << "Command " << command << " on device " << dev_name()
                             << " is not authorized because an error occurs while talking to the Controlled Access Service"
                             << ends;
                        string ex(e[0].desc);
                        if (ex.find("defined") != string::npos)
                            desc << "\n" << ex;
                        desc << ends;
                    }

                    NotAllowedExcept::throw_exception((const char *) API_ReadOnlyMode, desc.str(),
                                                      (const char *) "Connection::command_inout()");
                }
            }

            if (version >= 4)
            {
                ClntIdent ci;
                ApiUtil *au = ApiUtil::instance();
                ci.cpp_clnt(au->get_client_pid());

                Device_4_var dev = Device_4::_duplicate(device_4);
                return (dev->command_inout_4(command.c_str(), any, local_source, ci));
            }
            else if (version >= 2)
            {
                Device_2_var dev = Device_2::_duplicate(device_2);
                return (dev->command_inout_2(command.c_str(), any, local_source));
            }
            else
            {
                Device_var dev = Device::_duplicate(device);
                return (dev->command_inout(command.c_str(), any));
            }
            ctr = 2;
        }
        catch (Tango::ConnectionFailed &e)
        {
            TangoSys_OMemStream desc;
            desc << "Failed to execute command_inout on device " << dev_name();
            desc << ", command " << command << ends;
            ApiConnExcept::re_throw_exception(e, (const char *) API_CommandFailed,
                                              desc.str(), (const char *) "Connection::command_inout()");
        }
        catch (Tango::DevFailed &e)
        {
            TangoSys_OMemStream desc;
            desc << "Failed to execute command_inout on device " << dev_name();
            desc << ", command " << command << ends;

            if (::strcmp(e.errors[0].reason, DEVICE_UNLOCKED_REASON) == 0)
                DeviceUnlockedExcept::re_throw_exception(e, (const char *) DEVICE_UNLOCKED_REASON,
                                                         desc.str(), (const char *) "Connection::command_inout()");
            else
                Except::re_throw_exception(e, (const char *) API_CommandFailed,
                                           desc.str(), (const char *) "Connection::command_inout()");
        }
        catch (CORBA::TRANSIENT &trans)
        {
            TRANSIENT_NOT_EXIST_EXCEPT_CMD(trans);
        }
        catch (CORBA::OBJECT_NOT_EXIST &one)
        {
            if (one.minor() == omni::OBJECT_NOT_EXIST_NoMatch || one.minor() == 0)
            {
                TRANSIENT_NOT_EXIST_EXCEPT_CMD(one);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute command_inout on device " << dev_name();
                desc << ", command " << command << ends;
                ApiCommExcept::re_throw_exception(one,
                                                  (const char *) "API_CommunicationFailed",
                                                  desc.str(),
                                                  (const char *) "Connection::command_inout()");
            }
        }
        catch (CORBA::COMM_FAILURE &comm)
        {
            if (comm.minor() == omni::COMM_FAILURE_WaitingForReply)
            {
                TRANSIENT_NOT_EXIST_EXCEPT_CMD(comm);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute command_inout on device " << dev_name();
                desc << ", command " << command << ends;
                ApiCommExcept::re_throw_exception(comm,
                                                  (const char *) "API_CommunicationFailed",
                                                  desc.str(),
                                                  (const char *) "Connection::command_inout()");
            }
        }
        catch (CORBA::SystemException &ce)
        {
            set_connection_state(CONNECTION_NOTOK);

            TangoSys_OMemStream desc;
            desc << "Failed to execute command_inout on device " << dev_name();
            desc << ", command " << command << ends;
            ApiCommExcept::re_throw_exception(ce,
                                              (const char *) "API_CommunicationFailed",
                                              desc.str(),
                                              (const char *) "Connection::command_inout()");
        }
    }

//
// Just to make VC++ quiet (will never reach this code !)
//

    CORBA::Any_var tmp;
    return tmp;
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::DeviceProxy() - constructor for device proxy object
//
//-----------------------------------------------------------------------------

DeviceProxy::DeviceProxy(string &name, CORBA::ORB *orb)
    : Connection(orb),
      db_dev(NULL),
      is_alias(false),
      adm_device(NULL),
      lock_ctr(0),
      ext_proxy(new DeviceProxyExt())
{
    real_constructor(name, true);
}

DeviceProxy::DeviceProxy(const char *na, CORBA::ORB *orb)
    : Connection(orb),
      db_dev(NULL),
      is_alias(false),
      adm_device(NULL),
      lock_ctr(0),
      ext_proxy(new DeviceProxyExt())
{
    string name(na);
    real_constructor(name, true);
}

DeviceProxy::DeviceProxy(string &name, bool need_check_acc, CORBA::ORB *orb)
    : Connection(orb),
      db_dev(NULL),
      is_alias(false),
      adm_device(NULL),
      lock_ctr(0),
      ext_proxy(new DeviceProxyExt())
{
    real_constructor(name, need_check_acc);
}

DeviceProxy::DeviceProxy(const char *na, bool need_check_acc, CORBA::ORB *orb)
    : Connection(orb),
      db_dev(NULL),
      is_alias(false),
      adm_device(NULL),
      lock_ctr(0),
      ext_proxy(new DeviceProxyExt())
{
    string name(na);
    real_constructor(name, need_check_acc);
}

void DeviceProxy::real_constructor(string &name, bool need_check_acc)
{

//
// Parse device name
//

    parse_name(name);
    string corba_name;
    bool exported = true;

    if (dbase_used == true)
    {
        try
        {
            if (from_env_var == true)
            {
                ApiUtil *ui = ApiUtil::instance();
                db_dev = new DbDevice(device_name);
                int ind = ui->get_db_ind();
                db_host = (ui->get_db_vect())[ind]->get_db_host();
                db_port = (ui->get_db_vect())[ind]->get_db_port();
                db_port_num = (ui->get_db_vect())[ind]->get_db_port_num();
            }
            else
            {
                db_dev = new DbDevice(device_name, db_host, db_port);
                if (ext_proxy->nethost_alias == true)
                {
                    Database *tmp_db = db_dev->get_dbase();
                    const string &orig = tmp_db->get_orig_tango_host();
                    if (orig.empty() == true)
                    {
                        string orig_tg_host = ext_proxy->orig_tango_host;
                        if (orig_tg_host.find('.') == string::npos)
                        {
                            get_fqdn(orig_tg_host);
                        }
                        tmp_db->set_orig_tango_host(ext_proxy->orig_tango_host);
                    }
                }
            }
        }
        catch (Tango::DevFailed &e)
        {
            if (strcmp(e.errors[0].reason.in(), API_TangoHostNotSet) == 0)
            {
                cerr << e.errors[0].desc.in() << endl;
            }
            throw;
        }

        try
        {
            corba_name = get_corba_name(need_check_acc);
        }
        catch (Tango::DevFailed &dfe)
        {
            if (strcmp(dfe.errors[0].reason, "DB_DeviceNotDefined") == 0)
            {
                delete db_dev;
                TangoSys_OMemStream desc;
                desc << "Can't connect to device " << device_name << ends;
                ApiConnExcept::re_throw_exception(dfe,
                                                  (const char *) "API_DeviceNotDefined",
                                                  desc.str(),
                                                  (const char *) "DeviceProxy::DeviceProxy");
            }
            else if (strcmp(dfe.errors[0].reason, API_DeviceNotExported) == 0)
                exported = false;
        }
    }
    else
    {
        corba_name = build_corba_name();

//
// If we are not using the database, give write access
//

        access = ACCESS_WRITE;
    }

//
// Implement stateless new() i.e. even if connect fails continue
// If the DeviceProxy was created using device alias, ask for the real
// device name.
//

    try
    {
        if (exported == true)
        {
            connect(corba_name);

            if (is_alias == true)
            {
                CORBA::String_var real_name = device->name();
                device_name = real_name.in();
                transform(device_name.begin(), device_name.end(), device_name.begin(), ::tolower);
                db_dev->set_name(device_name);
            }
        }
    }
    catch (Tango::ConnectionFailed &dfe)
    {
        set_connection_state(CONNECTION_NOTOK);
        if (dbase_used == false)
        {
            if (strcmp(dfe.errors[1].reason, "API_DeviceNotDefined") == 0)
                throw;
        }
    }
    catch (CORBA::SystemException &)
    {
        set_connection_state(CONNECTION_NOTOK);
        if (dbase_used == false)
            throw;
    }

//
// For non-database device , try to ping them. It's the only way to know that
// the device is not defined
//

    if (dbase_used == false)
    {
        try
        {
            ping();
        }
        catch (Tango::ConnectionFailed &dfe)
        {
            if (strcmp(dfe.errors[1].reason, "API_DeviceNotDefined") == 0)
                throw;
        }
    }

//
// get the name of the asscociated device when connecting
// inside a device server
//

    try
    {
        ApiUtil *ui = ApiUtil::instance();
        if (ui->in_server() == true)
        {
            Tango::Util *tg = Tango::Util::instance(false);
            tg->get_sub_dev_diag().register_sub_device(tg->get_sub_dev_diag().get_associated_device(), name);
        }
    }
    catch (Tango::DevFailed &e)
    {
        if (::strcmp(e.errors[0].reason.in(), "API_UtilSingletonNotCreated") != 0)
            throw;
    }

    return;
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::DeviceProxy() - copy constructor
//
//-----------------------------------------------------------------------------

DeviceProxy::DeviceProxy(const DeviceProxy &sou)
    : Connection(sou), ext_proxy(Tango_nullptr)
{

//
// Copy DeviceProxy members
//

    device_name = sou.device_name;
    alias_name = sou.alias_name;
    is_alias = sou.is_alias;
    adm_dev_name = sou.adm_dev_name;
    lock_ctr = sou.lock_ctr;

    if (dbase_used == true)
    {
        if (from_env_var == true)
        {
            ApiUtil *ui = ApiUtil::instance();
            if (ui->in_server() == true)
                db_dev = new DbDevice(device_name, Tango::Util::instance()->get_database());
            else
                db_dev = new DbDevice(device_name);
        }
        else
        {
            db_dev = new DbDevice(device_name, db_host, db_port);
        }
    }

//
// Copy adm device pointer
//

    if (sou.adm_device == NULL)
        adm_device = NULL;
    else
    {
        adm_device = new DeviceProxy(sou.adm_device->dev_name().c_str());
    }

//
// Copy extension class
//

#ifdef HAS_UNIQUE_PTR
    if (sou.ext_proxy.get() != NULL)
    {
        ext_proxy.reset(new DeviceProxyExt);
        *(ext_proxy.get()) = *(sou.ext_proxy.get());
    }
#else
                                                                                                                            if (sou.ext_proxy == NULL)
		ext_proxy = NULL;
	else
	{
		ext_proxy = new DeviceProxyExt();
		*ext_proxy = *(sou.ext_proxy);
	}
#endif

}

//-----------------------------------------------------------------------------
//
// DeviceProxy::DeviceProxy() - assignement operator
//
//-----------------------------------------------------------------------------

DeviceProxy &DeviceProxy::operator=(const DeviceProxy &rval)
{

    if (this != &rval)
    {
        this->Connection::operator=(rval);

//
// Now DeviceProxy members
//

        device_name = rval.device_name;
        alias_name = rval.alias_name;
        is_alias = rval.is_alias;
        adm_dev_name = rval.adm_dev_name;
        lock_ctr = rval.lock_ctr;
        lock_valid = rval.lock_valid;

        delete db_dev;
        if (dbase_used == true)
        {
            if (from_env_var == true)
            {
                ApiUtil *ui = ApiUtil::instance();
                if (ui->in_server() == true)
                    db_dev = new DbDevice(device_name, Tango::Util::instance()->get_database());
                else
                    db_dev = new DbDevice(device_name);
            }
            else
            {
                db_dev = new DbDevice(device_name, db_host, db_port);
            }
        }

        delete adm_device;

        if (rval.adm_device != NULL)
        {
            adm_device = new DeviceProxy(rval.adm_device->dev_name().c_str());
        }
        else
            adm_device = NULL;

#ifdef HAS_UNIQUE_PTR
        if (rval.ext_proxy.get() != NULL)
        {
            ext_proxy.reset(new DeviceProxyExt);
            *(ext_proxy.get()) = *(rval.ext_proxy.get());
        }
        else
            ext_proxy.reset();
#else
                                                                                                                                delete ext_proxy;
        if (rval.ext_proxy != NULL)
        {
            ext_proxy = new DeviceProxyExt;
            *ext_proxy = *(rval.ext_proxy);
        }
        else
            ext_proxy = NULL;
#endif
    }

    return *this;
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::parse_name() - Parse device name according to Tango device
//			       name syntax
//
// in :	- full_name : The device name
//
//-----------------------------------------------------------------------------

void DeviceProxy::parse_name(string &full_name)
{
    string name_wo_prot;
    string name_wo_db_mod;
    string dev_name, object_name;

//
// Error of the string is empty
//

    if (full_name.empty() == true)
    {
        TangoSys_OMemStream desc;
        desc << "The given name is an empty string!!! " << full_name << endl;
        desc << "Device name syntax is domain/family/member" << ends;

        ApiWrongNameExcept::throw_exception((const char *) API_WrongDeviceNameSyntax,
                                            desc.str(),
                                            (const char *) "DeviceProxy::parse_name()");
    }

//
// Device name in lower case letters
//

    string full_name_low(full_name);
    transform(full_name_low.begin(), full_name_low.end(), full_name_low.begin(), ::tolower);

//
// Try to find protocol specification in device name and analyse it
//

    string::size_type pos = full_name_low.find(PROT_SEP);
    if (pos == string::npos)
    {
        if (full_name_low.size() > 2)
        {
            if ((full_name_low[0] == '/') && (full_name_low[1] == '/'))
                name_wo_prot = full_name_low.substr(2);
            else
                name_wo_prot = full_name_low;
        }
        else
            name_wo_prot = full_name_low;
    }
    else
    {
        string protocol = full_name_low.substr(0, pos);

        if (protocol == TANGO_PROTOCOL)
        {
            name_wo_prot = full_name_low.substr(pos + 3);
        }
        else if (protocol == TACO_PROTOCOL)
        {
            TangoSys_OMemStream desc;
            desc << "Taco protocol is not supported" << ends;
            ApiWrongNameExcept::throw_exception((const char *) "API_UnsupportedProtocol",
                                                desc.str(),
                                                (const char *) "DeviceProxy::parse_name()");
        }
        else
        {
            TangoSys_OMemStream desc;
            desc << protocol;
            desc << " protocol is an unsupported protocol" << ends;
            ApiWrongNameExcept::throw_exception((const char *) "API_UnsupportedProtocol",
                                                desc.str(),
                                                (const char *) "DeviceProxy::parse_name()");
        }
    }

//
// Try to find database database modifier and analyse it
//

    pos = name_wo_prot.find(MODIFIER);
    if (pos != string::npos)
    {
        string mod = name_wo_prot.substr(pos + 1);

        if (mod == DBASE_YES)
        {
            string::size_type len = name_wo_prot.size();
            name_wo_db_mod = name_wo_prot.substr(0, len - (len - pos));
            dbase_used = true;
        }
        else if (mod == DBASE_NO)
        {
            string::size_type len = name_wo_prot.size();
            name_wo_db_mod = name_wo_prot.substr(0, len - (len - pos));
            dbase_used = false;
        }
        else
        {
            //cerr << mod << " is a non supported database modifier!" << endl;

            TangoSys_OMemStream desc;
            desc << mod;
            desc << " modifier is an unsupported db modifier" << ends;
            ApiWrongNameExcept::throw_exception((const char *) "API_UnsupportedDBaseModifier",
                                                desc.str(),
                                                (const char *) "DeviceProxy::parse_name()");
        }
    }
    else
    {
        name_wo_db_mod = name_wo_prot;
        dbase_used = true;
    }

    if (dbase_used == false)
    {

//
// Extract host name and port number
//

        pos = name_wo_db_mod.find(HOST_SEP);
        if (pos == string::npos)
        {
            TangoSys_OMemStream desc;
            desc << "Host and port not correctly defined in device name " << full_name << ends;

            ApiWrongNameExcept::throw_exception((const char *) API_WrongDeviceNameSyntax,
                                                desc.str(),
                                                (const char *) "DeviceProxy::parse_name()");
        }

        host = name_wo_db_mod.substr(0, pos);
        string::size_type tmp = name_wo_db_mod.find(PORT_SEP);
        if (tmp == string::npos)
        {
            TangoSys_OMemStream desc;
            desc << "Host and port not correctly defined in device name " << full_name << ends;

            ApiWrongNameExcept::throw_exception((const char *) API_WrongDeviceNameSyntax,
                                                desc.str(),
                                                (const char *) "DeviceProxy::parse_name()");
        }
        port = name_wo_db_mod.substr(pos + 1, tmp - pos - 1);
        TangoSys_MemStream s;
        s << port << ends;
        s >> port_num;
        device_name = name_wo_db_mod.substr(tmp + 1);

//
// Check device name syntax (domain/family/member). Alias are forbidden without
// using the db
//

        tmp = device_name.find(DEV_NAME_FIELD_SEP);
        if (tmp == string::npos)
        {
            TangoSys_OMemStream desc;
            desc << "Wrong device name syntax (domain/family/member) in " << full_name << endl;
            desc << "Rem: Alias are forbidden when not using a database" << ends;

            ApiWrongNameExcept::throw_exception((const char *) API_WrongDeviceNameSyntax,
                                                desc.str(),
                                                (const char *) "DeviceProxy::parse_name()");
        }
        string::size_type prev_sep = tmp;
        tmp = device_name.find(DEV_NAME_FIELD_SEP, tmp + 1);
        if ((tmp == string::npos) || (tmp == prev_sep + 1))
        {
            TangoSys_OMemStream desc;
            desc << "Wrong device name syntax (domain/family/member) in " << full_name << endl;
            desc << "Rem: Alias are forbidden when not using a database" << ends;

            ApiWrongNameExcept::throw_exception((const char *) API_WrongDeviceNameSyntax,
                                                desc.str(),
                                                (const char *) "DeviceProxy::parse_name()");
        }
        prev_sep = tmp;
        tmp = device_name.find(DEV_NAME_FIELD_SEP, tmp + 1);
        if (tmp != string::npos)
        {
            TangoSys_OMemStream desc;
            desc << "Wrong device name syntax (domain/family/member) in " << full_name << endl;
            desc << "Rem: Alias are forbidden when not using a database" << ends;

            ApiWrongNameExcept::throw_exception((const char *) API_WrongDeviceNameSyntax,
                                                desc.str(),
                                                (const char *) "DeviceProxy::parse_name()");
        }

        db_host = db_port = NOT_USED;
        db_port_num = 0;
        from_env_var = false;
    }
    else
    {

//
// Search if host and port are specified
//

        pos = name_wo_db_mod.find(PORT_SEP);
        if (pos == string::npos)
        {

//
// It could be an alias name, check its syntax
//

            pos = name_wo_db_mod.find(HOST_SEP);
            if (pos != string::npos)
            {
                TangoSys_OMemStream desc;
                desc << "Wrong alias name syntax in " << full_name << " (: is not allowed in alias name)" << ends;

                ApiWrongNameExcept::throw_exception((const char *) API_WrongDeviceNameSyntax,
                                                    desc.str(),
                                                    (const char *) "DeviceProxy::parse_name()");
            }

            pos = name_wo_db_mod.find(RES_SEP);
            if (pos != string::npos)
            {
                TangoSys_OMemStream desc;
                desc << "Wrong alias name syntax in " << full_name << " (-> is not allowed in alias name)" << ends;

                ApiWrongNameExcept::throw_exception((const char *) API_WrongDeviceNameSyntax,
                                                    desc.str(),
                                                    (const char *) "DeviceProxy::parse_name()");
            }

//
// Alias name syntax OK
//

            alias_name = device_name = name_wo_db_mod;
            is_alias = true;
            from_env_var = true;
            port_num = 0;
            host = FROM_IOR;
            port = FROM_IOR;
        }
        else
        {
            string bef_sep = name_wo_db_mod.substr(0, pos);
            string::size_type tmp = bef_sep.find(HOST_SEP);
            if (tmp == string::npos)
            {

//
// There is at least one / in dev name but it is not a TANGO_HOST definition.
// A correct dev name must have 2 /. Check this. An alias cannot have any /
//

                if (pos == 0)
                {
                    TangoSys_OMemStream desc;
                    desc << "Wrong device name syntax (domain/family/member) in " << full_name << ends;

                    ApiWrongNameExcept::throw_exception((const char *) API_WrongDeviceNameSyntax,
                                                        desc.str(),
                                                        (const char *) "DeviceProxy::parse_name()");
                }

                string::size_type prev_sep = pos;
                pos = name_wo_db_mod.find(DEV_NAME_FIELD_SEP, pos + 1);
                if ((pos == string::npos) || (pos == prev_sep + 1))
                {

                    TangoSys_OMemStream desc;
                    desc << "Wrong device name syntax (domain/family/member) in " << full_name << ends;

                    ApiWrongNameExcept::throw_exception((const char *) API_WrongDeviceNameSyntax,
                                                        desc.str(),
                                                        (const char *) "DeviceProxy::parse_name()");
                }

                prev_sep = pos;
                pos = name_wo_db_mod.find(DEV_NAME_FIELD_SEP, prev_sep + 1);
                if (pos != string::npos)
                {
                    TangoSys_OMemStream desc;
                    desc << "Wrong device name syntax (domain/family/member) in " << full_name << ends;

                    ApiWrongNameExcept::throw_exception((const char *) API_WrongDeviceNameSyntax,
                                                        desc.str(),
                                                        (const char *) "DeviceProxy::parse_name()");
                }

                device_name = name_wo_db_mod;
                from_env_var = true;
                port_num = 0;
                port = FROM_IOR;
                host = FROM_IOR;
            }
            else
            {
                string tmp_host(bef_sep.substr(0, tmp));
                string safe_tmp_host(tmp_host);

                if (tmp_host.find('.') == string::npos)
                    get_fqdn(tmp_host);

                string::size_type pos2 = tmp_host.find('.');
                bool alias_used = false;
                string fq;
                if (pos2 != string::npos)
                {
                    string h_name = tmp_host.substr(0, pos2);
                    fq = tmp_host.substr(pos2);
                    if (h_name != tmp_host)
                        alias_used = true;
                }

                if (alias_used == true)
                {
                    ext_proxy->nethost_alias = true;
                    ext_proxy->orig_tango_host = safe_tmp_host;
                    if (safe_tmp_host.find('.') == string::npos)
                        ext_proxy->orig_tango_host = ext_proxy->orig_tango_host + fq;
                }
                else
                    ext_proxy->nethost_alias = false;

                db_host = tmp_host;
                db_port = bef_sep.substr(tmp + 1);
                TangoSys_MemStream s;
                s << db_port << ends;
                s >> db_port_num;
                object_name = name_wo_db_mod.substr(pos + 1);

//
// We should now check if the object name is a device name or an alias
//

                pos = object_name.find(DEV_NAME_FIELD_SEP);
                if (pos == string::npos)
                {

//
// It is an alias. Check its syntax
//

                    pos = object_name.find(HOST_SEP);
                    if (pos != string::npos)
                    {
                        TangoSys_OMemStream desc;
                        desc << "Wrong alias name syntax in " << full_name << " (: is not allowed in alias name)"
                             << ends;

                        ApiWrongNameExcept::throw_exception((const char *) API_WrongDeviceNameSyntax,
                                                            desc.str(),
                                                            (const char *) "DeviceProxy::parse_name()");
                    }

                    pos = object_name.find(RES_SEP);
                    if (pos != string::npos)
                    {
                        TangoSys_OMemStream desc;
                        desc << "Wrong alias name syntax in " << full_name << " (-> is not allowed in alias name)"
                             << ends;

                        ApiWrongNameExcept::throw_exception((const char *) API_WrongDeviceNameSyntax,
                                                            desc.str(),
                                                            (const char *) "DeviceProxy::parse_name()");
                    }
                    alias_name = device_name = object_name;
                    is_alias = true;

//
// Alias name syntax OK, but is it really an alias defined in db ?
//
                }
                else
                {

//
// It's a device name. Check its syntax.
// There is at least one / in dev name but it is not a TANGO_HOST definition.
// A correct dev name must have 2 /. Check this. An alias cannot have any /
//

                    string::size_type prev_sep = pos;
                    pos = object_name.find(DEV_NAME_FIELD_SEP, pos + 1);
                    if ((pos == string::npos) || (pos == prev_sep + 1))
                    {
                        TangoSys_OMemStream desc;
                        desc << "Wrong device name syntax (domain/family/member) in " << full_name << ends;

                        ApiWrongNameExcept::throw_exception((const char *) API_WrongDeviceNameSyntax,
                                                            desc.str(),
                                                            (const char *) "DeviceProxy::parse_name()");
                    }

                    prev_sep = pos;
                    pos = object_name.find(DEV_NAME_FIELD_SEP, prev_sep + 1);
                    if (pos != string::npos)
                    {
                        TangoSys_OMemStream desc;
                        desc << "Wrong device name syntax (domain/family/member) in " << full_name << ends;

                        ApiWrongNameExcept::throw_exception((const char *) API_WrongDeviceNameSyntax,
                                                            desc.str(),
                                                            (const char *) "DeviceProxy::parse_name()");
                    }

                    device_name = object_name;
                }
                from_env_var = false;
                port_num = 0;
                port = FROM_IOR;
                host = FROM_IOR;
            }
        }

    }

}

//-----------------------------------------------------------------------------
//
// DeviceProxy::get_corba_name() - return IOR for device from database
//
//-----------------------------------------------------------------------------

string DeviceProxy::get_corba_name(bool need_check_acc)
{
//
// If we are in a server, try a local import
// (in case the device is embedded in the same process)
//

    string local_ior;
    if (ApiUtil::instance()->in_server() == true)
        local_import(local_ior);

//
// If we are not in a server or if the device is not in the same process,
// ask the database
//

    DbDevImportInfo import_info;

    if (local_ior.size() == 0)
    {
        import_info = db_dev->import_device();

        if (import_info.exported != 1)
        {
            connection_state = CONNECTION_NOTOK;

            TangoSys_OMemStream desc;
            desc << "Device " << device_name << " is not exported (hint: try starting the device server)" << ends;
            ApiConnExcept::throw_exception(API_DeviceNotExported, desc.str(),
                                           "DeviceProxy::get_corba_name()");
        }
    }

//
// Get device access right
//

    if (need_check_acc == true)
        access = db_dev->check_access_control();
    else
        check_acc = false;

    if (local_ior.size() != 0)
        return local_ior;
    else
        return import_info.ior;
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::build_corba_name() - build corba name for non database device
//				     server. In this case, corba name uses
//				     the "corbaloc" naming schema
//
//-----------------------------------------------------------------------------

string DeviceProxy::build_corba_name()
{

    string db_corbaloc = "corbaloc:iiop:";
    db_corbaloc = db_corbaloc + host + ":" + port;
    db_corbaloc = db_corbaloc + "/" + device_name;

    return db_corbaloc;
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::reconnect() - Call the reconnection method and in case
//			      the device has been created from its alias,
//		              get its real name.
//
//-----------------------------------------------------------------------------

void DeviceProxy::reconnect(bool db_used)
{
    Connection::reconnect(db_used);

    if (connection_state == CONNECTION_OK)
    {
        if (is_alias == true)
        {
            CORBA::String_var real_name = device->name();
            device_name = real_name.in();
            transform(device_name.begin(), device_name.end(), device_name.begin(), ::tolower);
            db_dev->set_name(device_name);
        }
    }
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::import_info() - return import info for device from database
//
//-----------------------------------------------------------------------------

DbDevImportInfo DeviceProxy::import_info()
{
    DbDevImportInfo import_info;

    if (dbase_used == false)
    {
        TangoSys_OMemStream desc;
        desc << "Method not available for device ";
        desc << device_name;
        desc << " which is a non database device";

        ApiNonDbExcept::throw_exception((const char *) API_NonDatabaseDevice,
                                        desc.str(),
                                        (const char *) "DeviceProxy::import_info");
    }
    else
    {
        import_info = db_dev->import_device();
    }

    return (import_info);
}


//-----------------------------------------------------------------------------
//
// DeviceProxy::~DeviceProxy() - destructor to destroy proxy to TANGO device
//
//-----------------------------------------------------------------------------

DeviceProxy::~DeviceProxy()
{
    if (dbase_used == true)
        delete db_dev;

//
// If the device has some subscribed event, unsubscribe them
//

    if (ApiUtil::_is_instance_null() == false)
    {
        ApiUtil *api_ptr = ApiUtil::instance();
        ZmqEventConsumer *zmq = api_ptr->get_zmq_event_consumer();
        if (zmq != Tango_nullptr)
        {
            vector<int> ids;
            zmq->get_subscribed_event_ids(this, ids);
            if (ids.empty() == false)
            {
                vector<int>::iterator ite;
                for (ite = ids.begin(); ite != ids.end(); ++ite)
                {
                    unsubscribe_event(*ite);
                }

            }
        }
    }

//
// If the device is locked, unlock it whatever the lock counter is
//

    if (ApiUtil::_is_instance_null() == false)
    {
        if (lock_ctr > 0)
        {
            try
            {
                unlock(true);
            }
            catch (...)
            {}
        }
    }

//
// Delete memory
//

    delete adm_device;

#ifndef HAS_UNIQUE_PTR
    delete ext_proxy;
#endif
}


//-----------------------------------------------------------------------------
//
// DeviceProxy::s) - ping TANGO device and return time elapsed in microseconds
//
//-----------------------------------------------------------------------------

int DeviceProxy::ping()
{
    int elapsed;

#ifndef _TG_WINDOWS_
    struct timeval before, after;

    gettimeofday(&before, NULL);
#else
                                                                                                                            struct _timeb before, after;

	_ftime(&before);
#endif /* _TG_WINDOWS_ */

    int ctr = 0;

    while (ctr < 2)
    {
        try
        {
            check_and_reconnect();

            Device_var dev = Device::_duplicate(device);
            dev->ping();
            ctr = 2;
        }
        catch (CORBA::TRANSIENT &trans)
        {
            TRANSIENT_NOT_EXIST_EXCEPT(trans, "DeviceProxy", "ping", this);
        }
        catch (CORBA::OBJECT_NOT_EXIST &one)
        {
            if (one.minor() == omni::OBJECT_NOT_EXIST_NoMatch || one.minor() == 0)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(one, "DeviceProxy", "ping", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute ping on device " << device_name << ends;
                ApiCommExcept::re_throw_exception(one,
                                                  (const char *) "API_CommunicationFailed",
                                                  desc.str(),
                                                  (const char *) "DeviceProxy::ping()");
            }
        }
        catch (CORBA::COMM_FAILURE &comm)
        {
            if (comm.minor() == omni::COMM_FAILURE_WaitingForReply)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(comm, "DeviceProxy", "ping", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute ping on device " << device_name << ends;
                ApiCommExcept::re_throw_exception(comm,
                                                  (const char *) "API_CommunicationFailed",
                                                  desc.str(),
                                                  (const char *) "DeviceProxy::ping()");
            }
        }
        catch (CORBA::SystemException &ce)
        {
            set_connection_state(CONNECTION_NOTOK);

            TangoSys_OMemStream desc;
            desc << "Failed to execute ping on device " << device_name << ends;
            ApiCommExcept::re_throw_exception(ce,
                                              (const char *) "API_CommunicationFailed",
                                              desc.str(),
                                              (const char *) "DeviceProxy::ping()");
        }
    }
#ifndef _TG_WINDOWS_
    gettimeofday(&after, NULL);
    elapsed = (after.tv_sec - before.tv_sec) * 1000000;
    elapsed = (after.tv_usec - before.tv_usec) + elapsed;
#else
                                                                                                                            _ftime(&after);
	elapsed = (after.time-before.time)*1000000;
	elapsed = (after.millitm-before.millitm)*1000 + elapsed;
#endif /* _TG_WINDOWS_ */

    return (elapsed);
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::name() - return TANGO device name as string
//
//-----------------------------------------------------------------------------

string DeviceProxy::name()
{
    string na;
    int ctr = 0;

    while (ctr < 2)
    {
        try
        {
            check_and_reconnect();

            Device_var dev = Device::_duplicate(device);
            CORBA::String_var n = dev->name();
            ctr = 2;
            na = n;
        }
        catch (CORBA::TRANSIENT &trans)
        {
            TRANSIENT_NOT_EXIST_EXCEPT(trans, "DeviceProxy", "name", this);
        }
        catch (CORBA::OBJECT_NOT_EXIST &one)
        {
            if (one.minor() == omni::OBJECT_NOT_EXIST_NoMatch || one.minor() == 0)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(one, "DeviceProxy", "name", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute name() on device " << device_name << ends;
                ApiCommExcept::re_throw_exception(one,
                                                  (const char *) "API_CommunicationFailed",
                                                  desc.str(),
                                                  (const char *) "DeviceProxy::name()");
            }
        }
        catch (CORBA::COMM_FAILURE &comm)
        {
            if (comm.minor() == omni::COMM_FAILURE_WaitingForReply)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(comm, "DeviceProxy", "name", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute name() on device " << device_name << ends;
                ApiCommExcept::re_throw_exception(comm,
                                                  (const char *) "API_CommunicationFailed",
                                                  desc.str(),
                                                  (const char *) "DeviceProxy::name()");
            }
        }
        catch (CORBA::SystemException &ce)
        {
            set_connection_state(CONNECTION_NOTOK);

            TangoSys_OMemStream desc;
            desc << "Failed to execute name() on device " << device_name << ends;
            ApiCommExcept::re_throw_exception(ce,
                                              (const char *) "API_CommunicationFailed",
                                              desc.str(),
                                              (const char *) "DeviceProxy::name()");
        }
    }

    return (na);
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::alias() - return TANGO device alias (if any)
//
//-----------------------------------------------------------------------------

string DeviceProxy::alias()
{
    if (alias_name.size() == 0)
    {
        Database *db = this->get_device_db();
        if (db != NULL)
            db->get_alias(device_name, alias_name);
        else
        {
            Tango::Except::throw_exception((const char *) "DB_AliasNotDefined",
                                           (const char *) "No alias found for your device",
                                           (const char *) "DeviceProxy::alias()");
        }
    }

    return alias_name;
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::state() - return TANGO state of device
//
//-----------------------------------------------------------------------------

DevState DeviceProxy::state()
{
    DevState sta = Tango::UNKNOWN;
    int ctr = 0;

    while (ctr < 2)
    {
        try
        {
            check_and_reconnect();

            Device_var dev = Device::_duplicate(device);
            sta = dev->state();
            ctr = 2;
        }
        catch (CORBA::TRANSIENT &transp)
        {
            TRANSIENT_NOT_EXIST_EXCEPT(transp, "DeviceProxy", "state", this);
        }
        catch (CORBA::OBJECT_NOT_EXIST &one)
        {
            if (one.minor() == omni::OBJECT_NOT_EXIST_NoMatch || one.minor() == 0)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(one, "DeviceProxy", "state", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute state() on device " << device_name << ends;
                ApiCommExcept::re_throw_exception(one,
                                                  (const char *) "API_CommunicationFailed",
                                                  desc.str(),
                                                  (const char *) "DeviceProxy::state()");
            }
        }
        catch (CORBA::COMM_FAILURE &comm)
        {
            if (comm.minor() == omni::COMM_FAILURE_WaitingForReply)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(comm, "DeviceProxy", "state", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute state() on device " << device_name << ends;
                ApiCommExcept::re_throw_exception(comm,
                                                  (const char *) "API_CommunicationFailed",
                                                  desc.str(),
                                                  (const char *) "DeviceProxy::state()");
            }
        }
        catch (CORBA::SystemException &ce)
        {
            set_connection_state(CONNECTION_NOTOK);

            TangoSys_OMemStream desc;
            desc << "Failed to execute state() on device " << device_name << ends;
            ApiCommExcept::re_throw_exception(ce,
                                              (const char *) "API_CommunicationFailed",
                                              desc.str(),
                                              (const char *) "DeviceProxy::state()");
        }
    }

    return sta;
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::status() - return TANGO status of device
//
//-----------------------------------------------------------------------------

string DeviceProxy::status()
{
    string status_str;
    int ctr = 0;

    while (ctr < 2)
    {
        try
        {
            check_and_reconnect();

            Device_var dev = Device::_duplicate(device);
            CORBA::String_var st = dev->status();
            ctr = 2;
            status_str = st;
        }
        catch (CORBA::TRANSIENT &transp)
        {
            TRANSIENT_NOT_EXIST_EXCEPT(transp, "DeviceProxy", "status", this);
        }
        catch (CORBA::OBJECT_NOT_EXIST &one)
        {
            if (one.minor() == omni::OBJECT_NOT_EXIST_NoMatch || one.minor() == 0)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(one, "DeviceProxy", "status", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute status() on device " << device_name << ends;
                ApiCommExcept::re_throw_exception(one,
                                                  (const char *) "API_CommunicationFailed",
                                                  desc.str(),
                                                  (const char *) "DeviceProxy::status()");
            }
        }
        catch (CORBA::COMM_FAILURE &comm)
        {
            if (comm.minor() == omni::COMM_FAILURE_WaitingForReply)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(comm, "DeviceProxy", "status", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute status() on device " << device_name << ends;
                ApiCommExcept::re_throw_exception(comm,
                                                  (const char *) "API_CommunicationFailed",
                                                  desc.str(),
                                                  (const char *) "DeviceProxy::status()");
            }
        }
        catch (CORBA::SystemException &ce)
        {
            set_connection_state(CONNECTION_NOTOK);

            TangoSys_OMemStream desc;
            desc << "Failed to execute status() on device (CORBA exception)" << ends;
            ApiCommExcept::re_throw_exception(ce,
                                              (const char *) "API_CommunicationFailed",
                                              desc.str(),
                                              (const char *) "DeviceProxy::status()");
        }
    }

    return (status_str);
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::adm_name() - return TANGO admin name of device
//
//-----------------------------------------------------------------------------

string DeviceProxy::adm_name()
{
    string adm_name_str;
    int ctr = 0;

    while (ctr < 2)
    {
        try
        {
            check_and_reconnect();

            Device_var dev = Device::_duplicate(device);
            CORBA::String_var st = dev->adm_name();
            ctr = 2;
            adm_name_str = st;

            if (dbase_used == false)
            {
                string prot("tango://");
                if (host.find('.') == string::npos)
                    Connection::get_fqdn(host);
                prot = prot + host + ':' + port + '/';
                adm_name_str.insert(0, prot);
                adm_name_str.append(MODIFIER_DBASE_NO);
            }
            else if (from_env_var == false)
            {
                string prot("tango://");
                prot = prot + db_host + ':' + db_port + '/';
                adm_name_str.insert(0, prot);
            }
        }
        catch (CORBA::TRANSIENT &transp)
        {
            TRANSIENT_NOT_EXIST_EXCEPT(transp, "DeviceProxy", "adm_name", this);
        }
        catch (CORBA::OBJECT_NOT_EXIST &one)
        {
            if (one.minor() == omni::OBJECT_NOT_EXIST_NoMatch || one.minor() == 0)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(one, "DeviceProxy", "adm_name", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute adm_name() on device " << device_name << ends;
                ApiCommExcept::re_throw_exception(one,
                                                  (const char *) "API_CommunicationFailed",
                                                  desc.str(),
                                                  (const char *) "DeviceProxy::adm_name()");
            }
        }
        catch (CORBA::COMM_FAILURE &comm)
        {
            if (comm.minor() == omni::COMM_FAILURE_WaitingForReply)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(comm, "DeviceProxy", "adm_name", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute adm_name() on device " << device_name << ends;
                ApiCommExcept::re_throw_exception(comm,
                                                  (const char *) "API_CommunicationFailed",
                                                  desc.str(),
                                                  (const char *) "DeviceProxy::adm_name()");
            }
        }
        catch (CORBA::SystemException &ce)
        {
            set_connection_state(CONNECTION_NOTOK);

            TangoSys_OMemStream desc;
            desc << "Failed to execute adm_name() on device " << device_name << ends;
            ApiCommExcept::re_throw_exception(ce,
                                              (const char *) "API_CommunicationFailed",
                                              desc.str(),
                                              (const char *) "DeviceProxy::adm_name()");
        }
    }

    return (adm_name_str);
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::description() - return TANGO device description as string
//
//-----------------------------------------------------------------------------

string DeviceProxy::description()
{
    string description_str;
    int ctr = 0;

    while (ctr < 2)
    {
        try
        {
            check_and_reconnect();

            Device_var dev = Device::_duplicate(device);
            CORBA::String_var st = dev->description();
            ctr = 2;
            description_str = st;
        }
        catch (CORBA::TRANSIENT &trans)
        {
            TRANSIENT_NOT_EXIST_EXCEPT(trans, "DeviceProxy", "description", this);
        }
        catch (CORBA::OBJECT_NOT_EXIST &one)
        {
            if (one.minor() == omni::OBJECT_NOT_EXIST_NoMatch || one.minor() == 0)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(one, "DeviceProxy", "description", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute description() on device " << device_name << ends;
                ApiCommExcept::re_throw_exception(one,
                                                  (const char *) "API_CommunicationFailed",
                                                  desc.str(),
                                                  (const char *) "DeviceProxy::description()");
            }
        }
        catch (CORBA::COMM_FAILURE &comm)
        {
            if (comm.minor() == omni::COMM_FAILURE_WaitingForReply)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(comm, "DeviceProxy", "description", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute description() on device " << device_name << ends;
                ApiCommExcept::re_throw_exception(comm,
                                                  (const char *) "API_CommunicationFailed",
                                                  desc.str(),
                                                  (const char *) "DeviceProxy::description()");
            }
        }
        catch (CORBA::SystemException &ce)
        {
            set_connection_state(CONNECTION_NOTOK);

            TangoSys_OMemStream desc;
            desc << "Failed to execute description() on device " << device_name << ends;
            ApiCommExcept::re_throw_exception(ce,
                                              (const char *) "API_CommunicationFailed",
                                              desc.str(),
                                              (const char *) "DeviceProxy::description()");
        }
    }

    return (description_str);
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::black_box() - return the list of the last n commands exectued on
//		this TANGO device
//
//-----------------------------------------------------------------------------

vector<string> *DeviceProxy::black_box(int last_n_commands)
{
    DevVarStringArray_var last_commands;
    int ctr = 0;

    while (ctr < 2)
    {
        try
        {
            check_and_reconnect();

            Device_var dev = Device::_duplicate(device);
            last_commands = dev->black_box(last_n_commands);
            ctr = 2;
        }
        catch (CORBA::TRANSIENT &trans)
        {
            TRANSIENT_NOT_EXIST_EXCEPT(trans, "DeviceProxy", "black_box", this);
        }
        catch (CORBA::OBJECT_NOT_EXIST &one)
        {
            if (one.minor() == omni::OBJECT_NOT_EXIST_NoMatch || one.minor() == 0)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(one, "DeviceProxy", "black_box", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute black_box on device " << device_name << ends;
                ApiCommExcept::re_throw_exception(one,
                                                  (const char *) "API_CommunicationFailed",
                                                  desc.str(),
                                                  (const char *) "DeviceProxy::black_box()");
            }
        }
        catch (CORBA::COMM_FAILURE &comm)
        {
            if (comm.minor() == omni::COMM_FAILURE_WaitingForReply)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(comm, "DeviceProxy", "black_box", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute black_box on device " << device_name << ends;
                ApiCommExcept::re_throw_exception(comm,
                                                  (const char *) "API_CommunicationFailed",
                                                  desc.str(),
                                                  (const char *) "DeviceProxy::black_box()");
            }
        }
        catch (CORBA::SystemException &ce)
        {
            set_connection_state(CONNECTION_NOTOK);

            TangoSys_OMemStream desc;
            desc << "Failed to execute black_box on device " << device_name << ends;
            ApiCommExcept::re_throw_exception(ce,
                                              (const char *) "API_CommunicationFailed",
                                              desc.str(),
                                              (const char *) "DeviceProxy::black_box()");
        }
    }

    vector<string> *last_commands_vector = new(vector<string>);
    last_commands_vector->resize(last_commands->length());

    for (unsigned int i = 0; i < last_commands->length(); i++)
    {
        (*last_commands_vector)[i] = last_commands[i];
    }

    return (last_commands_vector);
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::info() - return information about this device
//
//-----------------------------------------------------------------------------

DeviceInfo const &DeviceProxy::info()
{
    DevInfo_var dev_info;
    DevInfo_3_var dev_info_3;
    int ctr = 0;

    while (ctr < 2)
    {
        try
        {
            check_and_reconnect();

            if (version >= 3)
            {
                Device_3_var dev = Device_3::_duplicate(device_3);
                dev_info_3 = dev->info_3();

                _info.dev_class = dev_info_3->dev_class;
                _info.server_id = dev_info_3->server_id;
                _info.server_host = dev_info_3->server_host;
                _info.server_version = dev_info_3->server_version;
                _info.doc_url = dev_info_3->doc_url;
                _info.dev_type = dev_info_3->dev_type;
            }
            else
            {
                Device_var dev = Device::_duplicate(device);
                dev_info = dev->info();

                _info.dev_class = dev_info->dev_class;
                _info.server_id = dev_info->server_id;
                _info.server_host = dev_info->server_host;
                _info.server_version = dev_info->server_version;
                _info.doc_url = dev_info->doc_url;
                _info.dev_type = NotSet;
            }
            ctr = 2;
        }
        catch (CORBA::TRANSIENT &trans)
        {
            TRANSIENT_NOT_EXIST_EXCEPT(trans, "DeviceProxy", "info", this);
        }
        catch (CORBA::OBJECT_NOT_EXIST &one)
        {
            if (one.minor() == omni::OBJECT_NOT_EXIST_NoMatch || one.minor() == 0)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(one, "DeviceProxy", "info", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute info() on device " << device_name << ends;
                ApiCommExcept::re_throw_exception(one,
                                                  (const char *) "API_CommunicationFailed",
                                                  desc.str(),
                                                  (const char *) "DeviceProxy::info()");
            }
        }
        catch (CORBA::COMM_FAILURE &comm)
        {
            if (comm.minor() == omni::COMM_FAILURE_WaitingForReply)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(comm, "DeviceProxy", "info", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute info() on device " << device_name << ends;
                ApiCommExcept::re_throw_exception(comm,
                                                  (const char *) "API_CommunicationFailed",
                                                  desc.str(),
                                                  (const char *) "DeviceProxy::info()");
            }
        }
        catch (CORBA::SystemException &ce)
        {
            set_connection_state(CONNECTION_NOTOK);

            TangoSys_OMemStream desc;
            desc << "Failed to execute info() on device " << device_name << ends;
            ApiCommExcept::re_throw_exception(ce,
                                              (const char *) "API_CommunicationFailed",
                                              desc.str(),
                                              (const char *) "DeviceProxy::info()");
        }
    }

    return (_info);
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::command_query() - return the description for the specified
//		command implemented for this TANGO device
//
//-----------------------------------------------------------------------------



CommandInfo DeviceProxy::command_query(string cmd)
{
    DevCmdInfo_var cmd_info;
    DevCmdInfo_2_var cmd_info_2;
    DevCmdInfo_6_var cmd_info_6;
    int ctr = 0;

    while (ctr < 2)
    {
        try
        {
            check_and_reconnect();


            Device_var dev;
            Device_2_var dev_2;
            Device_6_var dev_6;
            switch (version)
            {
                case 1:
                    dev = Device::_duplicate(device);
                    cmd_info = dev->command_query(cmd.c_str());

                    return create_CommandInfo(cmd_info.in());
                case 2:
                case 3:
                case 4:
                case 5:
                    dev_2 = Device_2::_duplicate(device_2);
                    cmd_info_2 = dev_2->command_query_2(cmd.c_str());

                    return create_CommandInfo(cmd_info_2.in());
                case 6:
                default:
                    dev_6 = Device_6::_duplicate(device_6);
                    cmd_info_6 = dev_6->command_query_6(cmd.c_str());

                    return create_CommandInfo(cmd_info_6.in());
            }
        }
        catch (CORBA::TRANSIENT &trans)
        {
            TRANSIENT_NOT_EXIST_EXCEPT(trans, "DeviceProxy", "command_query", this);
        }
        catch (CORBA::OBJECT_NOT_EXIST &one)
        {
            if (one.minor() == omni::OBJECT_NOT_EXIST_NoMatch || one.minor() == 0)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(one, "DeviceProxy", "command_query", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute command_query on device " << device_name << ends;
                ApiCommExcept::re_throw_exception(one,
                                                  (const char *) "API_CommunicationFailed",
                                                  desc.str(),
                                                  (const char *) "DeviceProxy::command_query()");
            }
        }
        catch (CORBA::COMM_FAILURE &comm)
        {
            if (comm.minor() == omni::COMM_FAILURE_WaitingForReply)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(comm, "DeviceProxy", "command_query", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute command_query on device " << device_name << ends;
                ApiCommExcept::re_throw_exception(comm,
                                                  (const char *) "API_CommunicationFailed",
                                                  desc.str(),
                                                  (const char *) "DeviceProxy::command_query()");
            }
        }
        catch (CORBA::SystemException &ce)
        {
            set_connection_state(CONNECTION_NOTOK);

            TangoSys_OMemStream desc;
            desc << "Failed to execute command_query on device " << device_name << ends;
            ApiCommExcept::re_throw_exception(ce,
                                              (const char *) "API_CommunicationFailed",
                                              desc.str(),
                                              (const char *) "DeviceProxy::command_query()");
        }
    }
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::get_command_config() - return the command info for a set of commands
//
//-----------------------------------------------------------------------------

CommandInfoList *DeviceProxy::get_command_config(vector<string> &cmd_names)
{
    CommandInfoList *all_cmds = command_list_query();

//
// Leave method if the user requires config for all commands
//

    if (cmd_names.size() == 1 && cmd_names[0] == AllCmd)
        return all_cmds;

//
// Return only the required commands config
//

    CommandInfoList *ret_cmds = new CommandInfoList;
    vector<string>::iterator ite;
    for (ite = cmd_names.begin(); ite != cmd_names.end(); ++ite)
    {
        string w_str(*ite);
        transform(w_str.begin(), w_str.end(), w_str.begin(), ::tolower);

        vector<CommandInfo>::iterator pos;
        for (pos = all_cmds->begin(); pos != all_cmds->end(); ++pos)
        {
            string lower_cmd(pos->cmd_name);
            transform(lower_cmd.begin(), lower_cmd.end(), lower_cmd.begin(), ::tolower);
            if (w_str == lower_cmd)
            {
                ret_cmds->push_back(*pos);
                break;
            }
        }
    }

    delete all_cmds;

    return ret_cmds;
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::command_list_query() - return the list of commands implemented for this TANGO device
//
//-----------------------------------------------------------------------------

CommandInfoList *DeviceProxy::command_list_query()
{
    DevCmdInfoList_var cmd_info_list;
    DevCmdInfoList_2_var cmd_info_list_2;
    DevCmdInfoList_6_var cmd_info_list_6;
    int ctr = 0;

    //TODO extract common code as template function that takes other function to execute inside this loop, see also command_query
    while (ctr < 2)
    {
        try
        {
            check_and_reconnect();

            Device_var dev;
            Device_2_var dev_2;
            Device_6_var dev_6;
            switch (version)
            {
                case 1:
                    dev = Device::_duplicate(device);
                    cmd_info_list = dev->command_list_query();

                    return newCommandInfoList(cmd_info_list);
                case 2:
                case 3:
                case 4:
                case 5:
                    dev_2 = Device_2::_duplicate(device_2);
                    cmd_info_list_2 = dev_2->command_list_query_2();

                    return newCommandInfoList(cmd_info_list_2);
                case 6:
                default:
                    dev_6 = Device_6::_duplicate(device_6);
                    cmd_info_list_6 = dev_6->command_list_query_6();

                    return newCommandInfoList(cmd_info_list_6);
            }
        }
        catch (CORBA::TRANSIENT &trans)
        {
            TRANSIENT_NOT_EXIST_EXCEPT(trans, "DeviceProxy", "command_list_query", this);
        }
        catch (CORBA::OBJECT_NOT_EXIST &one)
        {
            if (one.minor() == omni::OBJECT_NOT_EXIST_NoMatch || one.minor() == 0)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(one, "DeviceProxy", "command_list_query", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute command_list_query on device " << device_name << ends;
                ApiCommExcept::re_throw_exception(one,
                                                  (const char *) "API_CommunicationFailed",
                                                  desc.str(),
                                                  (const char *) "DeviceProxy::command_list_query()");
            }
        }
        catch (CORBA::COMM_FAILURE &comm)
        {
            if (comm.minor() == omni::COMM_FAILURE_WaitingForReply)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(comm, "DeviceProxy", "command_list_query", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute command_list_query on device " << device_name << ends;
                ApiCommExcept::re_throw_exception(comm,
                                                  (const char *) "API_CommunicationFailed",
                                                  desc.str(),
                                                  (const char *) "DeviceProxy::command_list_query()");
            }
        }
        catch (CORBA::SystemException &ce)
        {
            set_connection_state(CONNECTION_NOTOK);

            TangoSys_OMemStream desc;
            desc << "Failed to execute command_list_query on device " << device_name << ends;
            ApiCommExcept::re_throw_exception(ce,
                                              (const char *) "API_CommunicationFailed",
                                              desc.str(),
                                              (const char *) "DeviceProxy::command_list_query()");
        }
    }
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::get_command_list() - return the list of commands implemented for this TANGO device (only names)
//
//-----------------------------------------------------------------------------

vector<string> *DeviceProxy::get_command_list()
{
    CommandInfoList *all_cmd_config;

    all_cmd_config = command_list_query();

    vector<string> *cmd_list = new vector<string>;
    cmd_list->resize(all_cmd_config->size());
    for (unsigned int i = 0; i < all_cmd_config->size(); i++)
    {
        (*cmd_list)[i] = (*all_cmd_config)[i].cmd_name;
    }
    delete all_cmd_config;

    return cmd_list;
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::get_property() - get a property from the database
//
//-----------------------------------------------------------------------------

void DeviceProxy::get_property(string &property_name, DbData &db_data)
{
    if (dbase_used == false)
    {
        TangoSys_OMemStream desc;
        desc << "Method not available for device ";
        desc << device_name;
        desc << " which is a non database device";

        ApiNonDbExcept::throw_exception((const char *) API_NonDatabaseDevice,
                                        desc.str(),
                                        (const char *) "DeviceProxy::get_property");
    }
    else
    {
        db_data.resize(1);
        db_data[0] = DbDatum(property_name);

        db_dev->get_property(db_data);
    }

    return;
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::get_property() - get a property from the database
//
//-----------------------------------------------------------------------------

void DeviceProxy::get_property(vector<string> &property_names, DbData &db_data)
{
    if (dbase_used == false)
    {
        TangoSys_OMemStream desc;
        desc << "Method not available for device ";
        desc << device_name;
        desc << " which is a non database device";

        ApiNonDbExcept::throw_exception((const char *) API_NonDatabaseDevice,
                                        desc.str(),
                                        (const char *) "DeviceProxy::get_property");
    }
    else
    {
        db_data.resize(property_names.size());
        for (unsigned int i = 0; i < property_names.size(); i++)
        {
            db_data[i] = DbDatum(property_names[i]);
        }

        db_dev->get_property(db_data);
    }

    return;
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::get_property() - get a property from the database
//
//-----------------------------------------------------------------------------

void DeviceProxy::get_property(DbData &db_data)
{
    if (dbase_used == false)
    {
        TangoSys_OMemStream desc;
        desc << "Method not available for device ";
        desc << device_name;
        desc << " which is a non database device";

        ApiNonDbExcept::throw_exception((const char *) API_NonDatabaseDevice,
                                        desc.str(),
                                        (const char *) "DeviceProxy::get_property");
    }
    else
    {
        db_dev->get_property(db_data);
    }

    return;
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::put_property() - put a property from the database
//
//-----------------------------------------------------------------------------

void DeviceProxy::put_property(DbData &db_data)
{
    if (dbase_used == false)
    {
        TangoSys_OMemStream desc;
        desc << "Method not available for device ";
        desc << device_name;
        desc << " which is a non database device";

        ApiNonDbExcept::throw_exception((const char *) API_NonDatabaseDevice,
                                        desc.str(),
                                        (const char *) "DeviceProxy::put_property");
    }
    else
    {
        db_dev->put_property(db_data);
    }

    return;
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::delete_property() - delete a property from the database
//
//-----------------------------------------------------------------------------

void DeviceProxy::delete_property(string &property_name)
{
    if (dbase_used == false)
    {
        TangoSys_OMemStream desc;
        desc << "Method not available for device ";
        desc << device_name;
        desc << " which is a non database device";

        ApiNonDbExcept::throw_exception((const char *) API_NonDatabaseDevice,
                                        desc.str(),
                                        (const char *) "DeviceProxy::delete_property");
    }
    else
    {
        DbData db_data;

        db_data.push_back(DbDatum(property_name));

        db_dev->delete_property(db_data);
    }

    return;
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::delete_property() - delete a property from the database
//
//-----------------------------------------------------------------------------

void DeviceProxy::delete_property(vector<string> &property_names)
{
    if (dbase_used == false)
    {
        TangoSys_OMemStream desc;
        desc << "Method not available for device ";
        desc << device_name;
        desc << " which is a non database device";

        ApiNonDbExcept::throw_exception((const char *) API_NonDatabaseDevice,
                                        desc.str(),
                                        (const char *) "DeviceProxy::delete_property");
    }
    else
    {
        DbData db_data;

        for (unsigned int i = 0; i < property_names.size(); i++)
        {
            db_data.push_back(DbDatum(property_names[i]));
        }

        db_dev->delete_property(db_data);
    }

    return;
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::delete_property() - delete a property from the database
//
//-----------------------------------------------------------------------------

void DeviceProxy::delete_property(DbData &db_data)
{
    if (dbase_used == false)
    {
        TangoSys_OMemStream desc;
        desc << "Method not available for device ";
        desc << device_name;
        desc << " which is a non database device";

        ApiNonDbExcept::throw_exception((const char *) API_NonDatabaseDevice,
                                        desc.str(),
                                        (const char *) "DeviceProxy::delete_property");
    }
    else
    {
        db_dev->delete_property(db_data);
    }

    return;
}


//-----------------------------------------------------------------------------
//
// DeviceProxy::get_property_list() - get a list of property names from the database
//
//-----------------------------------------------------------------------------

void DeviceProxy::get_property_list(const string &wildcard, vector<string> &prop_list)
{
    if (dbase_used == false)
    {
        TangoSys_OMemStream desc;
        desc << "Method not available for device ";
        desc << device_name;
        desc << " which is a non database device";

        ApiNonDbExcept::throw_exception((const char *) API_NonDatabaseDevice,
                                        desc.str(),
                                        (const char *) "DeviceProxy::get_property_list");
    }
    else
    {
        int num = 0;
        num = count(wildcard.begin(), wildcard.end(), '*');

        if (num > 1)
        {
            ApiWrongNameExcept::throw_exception((const char *) "API_WrongWildcardUsage",
                                                (const char *) "Only one wildcard character (*) allowed!",
                                                (const char *) "DeviceProxy::get_property_list");
        }
        db_dev->get_property_list(wildcard, prop_list);
    }

    return;
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::get_attribute_config() - return a list of attributes
//
//-----------------------------------------------------------------------------

AttributeInfoList *DeviceProxy::get_attribute_config(vector<string> &attr_string_list)
{
    AttributeConfigList_var attr_config_list;
    AttributeConfigList_2_var attr_config_list_2;
    AttributeInfoList *dev_attr_config = new AttributeInfoList();
    DevVarStringArray attr_list;
    int ctr = 0;

    attr_list.length(attr_string_list.size());
    for (unsigned int i = 0; i < attr_string_list.size(); i++)
    {
        if (attr_string_list[i] == AllAttr)
        {
            if (version >= 3)
                attr_list[i] = string_dup(AllAttr_3);
            else
                attr_list[i] = string_dup(AllAttr);
        }
        else if (attr_string_list[i] == AllAttr_3)
        {
            if (version < 3)
                attr_list[i] = string_dup(AllAttr);
            else
                attr_list[i] = string_dup(AllAttr_3);
        }
        else
            attr_list[i] = string_dup(attr_string_list[i].c_str());
    }

    while (ctr < 2)
    {
        try
        {
            check_and_reconnect();

            if (version == 1)
            {
                Device_var dev = Device::_duplicate(device);
                attr_config_list = dev->get_attribute_config(attr_list);

                dev_attr_config->resize(attr_config_list->length());

                for (unsigned int i = 0; i < attr_config_list->length(); i++)
                {
                    (*dev_attr_config)[i].name = attr_config_list[i].name;
                    (*dev_attr_config)[i].writable = attr_config_list[i].writable;
                    (*dev_attr_config)[i].data_format = attr_config_list[i].data_format;
                    (*dev_attr_config)[i].data_type = attr_config_list[i].data_type;
                    (*dev_attr_config)[i].max_dim_x = attr_config_list[i].max_dim_x;
                    (*dev_attr_config)[i].max_dim_y = attr_config_list[i].max_dim_y;
                    (*dev_attr_config)[i].description = attr_config_list[i].description;
                    (*dev_attr_config)[i].label = attr_config_list[i].label;
                    (*dev_attr_config)[i].unit = attr_config_list[i].unit;
                    (*dev_attr_config)[i].standard_unit = attr_config_list[i].standard_unit;
                    (*dev_attr_config)[i].display_unit = attr_config_list[i].display_unit;
                    (*dev_attr_config)[i].format = attr_config_list[i].format;
                    (*dev_attr_config)[i].min_value = attr_config_list[i].min_value;
                    (*dev_attr_config)[i].max_value = attr_config_list[i].max_value;
                    (*dev_attr_config)[i].min_alarm = attr_config_list[i].min_alarm;
                    (*dev_attr_config)[i].max_alarm = attr_config_list[i].max_alarm;
                    (*dev_attr_config)[i].writable_attr_name = attr_config_list[i].writable_attr_name;
                    (*dev_attr_config)[i].extensions.resize(attr_config_list[i].extensions.length());
                    for (unsigned int j = 0; j < attr_config_list[i].extensions.length(); j++)
                    {
                        (*dev_attr_config)[i].extensions[j] = attr_config_list[i].extensions[j];
                    }
                    (*dev_attr_config)[i].disp_level = Tango::OPERATOR;
                }
            }
            else
            {
                Device_2_var dev = Device_2::_duplicate(device_2);
                attr_config_list_2 = dev->get_attribute_config_2(attr_list);

                dev_attr_config->resize(attr_config_list_2->length());

                for (unsigned int i = 0; i < attr_config_list_2->length(); i++)
                {
                    (*dev_attr_config)[i].name = attr_config_list_2[i].name;
                    (*dev_attr_config)[i].writable = attr_config_list_2[i].writable;
                    (*dev_attr_config)[i].data_format = attr_config_list_2[i].data_format;
                    (*dev_attr_config)[i].data_type = attr_config_list_2[i].data_type;
                    (*dev_attr_config)[i].max_dim_x = attr_config_list_2[i].max_dim_x;
                    (*dev_attr_config)[i].max_dim_y = attr_config_list_2[i].max_dim_y;
                    (*dev_attr_config)[i].description = attr_config_list_2[i].description;
                    (*dev_attr_config)[i].label = attr_config_list_2[i].label;
                    (*dev_attr_config)[i].unit = attr_config_list_2[i].unit;
                    (*dev_attr_config)[i].standard_unit = attr_config_list_2[i].standard_unit;
                    (*dev_attr_config)[i].display_unit = attr_config_list_2[i].display_unit;
                    (*dev_attr_config)[i].format = attr_config_list_2[i].format;
                    (*dev_attr_config)[i].min_value = attr_config_list_2[i].min_value;
                    (*dev_attr_config)[i].max_value = attr_config_list_2[i].max_value;
                    (*dev_attr_config)[i].min_alarm = attr_config_list_2[i].min_alarm;
                    (*dev_attr_config)[i].max_alarm = attr_config_list_2[i].max_alarm;
                    (*dev_attr_config)[i].writable_attr_name = attr_config_list_2[i].writable_attr_name;
                    (*dev_attr_config)[i].extensions.resize(attr_config_list_2[i].extensions.length());
                    for (unsigned int j = 0; j < attr_config_list_2[i].extensions.length(); j++)
                    {
                        (*dev_attr_config)[i].extensions[j] = attr_config_list_2[i].extensions[j];
                    }
                    (*dev_attr_config)[i].disp_level = attr_config_list_2[i].level;
                }
            }
            ctr = 2;
        }
        catch (CORBA::TRANSIENT &trans)
        {
            TRANSIENT_NOT_EXIST_EXCEPT(trans, "DeviceProxy", "get_attribute_config", this);
        }
        catch (CORBA::OBJECT_NOT_EXIST &one)
        {
            if (one.minor() == omni::OBJECT_NOT_EXIST_NoMatch || one.minor() == 0)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(one, "DeviceProxy", "get_attribute_config", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute get_attribute_config on device " << device_name << ends;
                ApiCommExcept::re_throw_exception(one,
                                                  (const char *) "API_CommunicationFailed",
                                                  desc.str(),
                                                  (const char *) "DeviceProxy::get_attribute_config()");
            }
        }
        catch (CORBA::COMM_FAILURE &comm)
        {
            if (comm.minor() == omni::COMM_FAILURE_WaitingForReply)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(comm, "DeviceProxy", "get_attribute_config", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute get_attribute_config on device " << device_name << ends;
                ApiCommExcept::re_throw_exception(comm,
                                                  (const char *) "API_CommunicationFailed",
                                                  desc.str(),
                                                  (const char *) "DeviceProxy::get_attribute_config()");
            }
        }
        catch (CORBA::SystemException &ce)
        {
            set_connection_state(CONNECTION_NOTOK);

            TangoSys_OMemStream desc;
            desc << "Failed to execute get_attribute_config on device " << device_name << ends;
            ApiCommExcept::re_throw_exception(ce,
                                              (const char *) "API_CommunicationFailed",
                                              desc.str(),
                                              (const char *) "DeviceProxy::get_attribute_config()");
        }
        catch (Tango::DevFailed)
        {
            delete dev_attr_config;
            throw;
        }
    }

    return (dev_attr_config);
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::get_attribute_config_ex() - return a list of attributes
//
//-----------------------------------------------------------------------------

AttributeInfoListEx *DeviceProxy::get_attribute_config_ex(vector<string> &attr_string_list)
{
    AttributeConfigList_var attr_config_list;
    AttributeConfigList_2_var attr_config_list_2;
    AttributeConfigList_3_var attr_config_list_3;
    AttributeConfigList_5_var attr_config_list_5;
    AttributeInfoListEx *dev_attr_config = new AttributeInfoListEx();
    DevVarStringArray attr_list;
    int ctr = 0;

    attr_list.length(attr_string_list.size());
    for (unsigned int i = 0; i < attr_string_list.size(); i++)
    {
        if (attr_string_list[i] == AllAttr)
        {
            if (version >= 3)
                attr_list[i] = string_dup(AllAttr_3);
            else
                attr_list[i] = string_dup(AllAttr);
        }
        else if (attr_string_list[i] == AllAttr_3)
        {
            if (version < 3)
                attr_list[i] = string_dup(AllAttr);
            else
                attr_list[i] = string_dup(AllAttr_3);
        }
        else
            attr_list[i] = string_dup(attr_string_list[i].c_str());
    }

    while (ctr < 2)
    {
        try
        {
            check_and_reconnect();

            switch (version)
            {
                case 1:
                {
                    Device_var dev = Device::_duplicate(device);
                    attr_config_list = dev->get_attribute_config(attr_list);
                    dev_attr_config->resize(attr_config_list->length());

                    for (size_t i = 0; i < attr_config_list->length(); i++)
                    {
                        COPY_BASE_CONFIG((*dev_attr_config), attr_config_list)
                        (*dev_attr_config)[i].min_alarm = attr_config_list[i].min_alarm;
                        (*dev_attr_config)[i].max_alarm = attr_config_list[i].max_alarm;
                        (*dev_attr_config)[i].disp_level = Tango::OPERATOR;
                    }
                }
                    break;


                case 2:
                {
                    Device_2_var dev = Device_2::_duplicate(device_2);
                    attr_config_list_2 = dev->get_attribute_config_2(attr_list);
                    dev_attr_config->resize(attr_config_list_2->length());

                    for (size_t i = 0; i < attr_config_list_2->length(); i++)
                    {
                        COPY_BASE_CONFIG((*dev_attr_config), attr_config_list_2)
                        (*dev_attr_config)[i].min_alarm = attr_config_list_2[i].min_alarm;
                        (*dev_attr_config)[i].max_alarm = attr_config_list_2[i].max_alarm;
                        (*dev_attr_config)[i].disp_level = attr_config_list_2[i].level;
                    }

                    get_remaining_param(dev_attr_config);
                }
                    break;

                case 3:
                case 4:
                {
                    Device_3_var dev = Device_3::_duplicate(device_3);
                    attr_config_list_3 = dev->get_attribute_config_3(attr_list);
                    dev_attr_config->resize(attr_config_list_3->length());

                    for (size_t i = 0; i < attr_config_list_3->length(); i++)
                    {
                        COPY_BASE_CONFIG((*dev_attr_config), attr_config_list_3)

                        for (size_t j = 0; j < attr_config_list_3[i].sys_extensions.length(); j++)
                        {
                            (*dev_attr_config)[i].sys_extensions[j] = attr_config_list_3[i].sys_extensions[j];
                        }
                        (*dev_attr_config)[i].min_alarm = attr_config_list_3[i].att_alarm.min_alarm;
                        (*dev_attr_config)[i].max_alarm = attr_config_list_3[i].att_alarm.max_alarm;
                        (*dev_attr_config)[i].disp_level = attr_config_list_3[i].level;
                        (*dev_attr_config)[i].memorized = NOT_KNOWN;

                        COPY_ALARM_CONFIG((*dev_attr_config), attr_config_list_3)

                        COPY_EVENT_CONFIG((*dev_attr_config), attr_config_list_3)
                    }
                }
                    break;

                case 5:
                case 6:
                {
                    Device_5_var dev = Device_5::_duplicate(device_5);
                    attr_config_list_5 = dev->get_attribute_config_5(attr_list);
                    dev_attr_config->resize(attr_config_list_5->length());

                    for (size_t i = 0; i < attr_config_list_5->length(); i++)
                    {
                        COPY_BASE_CONFIG((*dev_attr_config), attr_config_list_5)

                        for (size_t j = 0; j < attr_config_list_5[i].sys_extensions.length(); j++)
                        {
                            (*dev_attr_config)[i].sys_extensions[j] = attr_config_list_5[i].sys_extensions[j];
                        }
                        (*dev_attr_config)[i].disp_level = attr_config_list_5[i].level;
                        (*dev_attr_config)[i].min_alarm = attr_config_list_5[i].att_alarm.min_alarm;
                        (*dev_attr_config)[i].max_alarm = attr_config_list_5[i].att_alarm.max_alarm;
                        (*dev_attr_config)[i].root_attr_name = attr_config_list_5[i].root_attr_name;
                        if (attr_config_list_5[i].memorized == false)
                            (*dev_attr_config)[i].memorized = NONE;
                        else
                        {
                            if (attr_config_list_5[i].mem_init == false)
                                (*dev_attr_config)[i].memorized = MEMORIZED;
                            else
                                (*dev_attr_config)[i].memorized = MEMORIZED_WRITE_INIT;
                        }
                        if (attr_config_list_5[i].data_type == DEV_ENUM)
                        {
                            for (size_t loop = 0; loop < attr_config_list_5[i].enum_labels.length(); loop++)
                                (*dev_attr_config)[i].enum_labels.push_back(
                                    attr_config_list_5[i].enum_labels[loop].in());
                        }
                        COPY_ALARM_CONFIG((*dev_attr_config), attr_config_list_5)

                        COPY_EVENT_CONFIG((*dev_attr_config), attr_config_list_5)
                    }
                }
                    break;

                default:
                    //TODO an exception must be thrown here, but it produces segfault in ~AttributeProxy
                    //Tango::Except::throw_exception("Unsupported TANGO protocol version=" + version,"Unsupported TANGO protocol version=" + version,"DeviceProxy::get_attribute_config_ex");
                    break;
            }

            ctr = 2;
        }
        catch (CORBA::TRANSIENT &trans)
        {
            TRANSIENT_NOT_EXIST_EXCEPT(trans, "DeviceProxy", "get_attribute_config", this);
        }
        catch (CORBA::OBJECT_NOT_EXIST &one)
        {
            if (one.minor() == omni::OBJECT_NOT_EXIST_NoMatch || one.minor() == 0)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(one, "DeviceProxy", "get_attribute_config", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute get_attribute_config on device " << device_name << ends;
                ApiCommExcept::re_throw_exception(one,
                                                  (const char *) "API_CommunicationFailed",
                                                  desc.str(),
                                                  (const char *) "DeviceProxy::get_attribute_config()");
            }
        }
        catch (CORBA::COMM_FAILURE &comm)
        {
            if (comm.minor() == omni::COMM_FAILURE_WaitingForReply)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(comm, "DeviceProxy", "get_attribute_config", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute get_attribute_config on device " << device_name << ends;
                ApiCommExcept::re_throw_exception(comm,
                                                  (const char *) "API_CommunicationFailed",
                                                  desc.str(),
                                                  (const char *) "DeviceProxy::get_attribute_config()");
            }
        }
        catch (CORBA::SystemException &ce)
        {
            set_connection_state(CONNECTION_NOTOK);

            TangoSys_OMemStream desc;
            desc << "Failed to execute get_attribute_config on device " << device_name << ends;
            ApiCommExcept::re_throw_exception(ce,
                                              (const char *) "API_CommunicationFailed",
                                              desc.str(),
                                              (const char *) "DeviceProxy::get_attribute_config()");
        }
        catch (Tango::DevFailed)
        {
            delete dev_attr_config;
            throw;
        }
    }

    return (dev_attr_config);
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::get_remaining_param()
//
// For device implementing device_2, get attribute config param from db
// instead of getting them from device. The wanted parameters are the
// warning alarm parameters, the RDS parameters and the event param.
// This method is called only for device_2 device
//
// In : dev_attr_config : ptr to attribute config vector returned
//			  to caller
//
//-----------------------------------------------------------------------------

void DeviceProxy::get_remaining_param(AttributeInfoListEx *dev_attr_config)
{

//
// Give a default value to all param.
//

    for (unsigned int loop = 0; loop < dev_attr_config->size(); loop++)
    {
        (*dev_attr_config)[loop].alarms.min_alarm = (*dev_attr_config)[loop].min_alarm;
        (*dev_attr_config)[loop].alarms.max_alarm = (*dev_attr_config)[loop].max_alarm;
        (*dev_attr_config)[loop].alarms.min_warning = AlrmValueNotSpec;
        (*dev_attr_config)[loop].alarms.max_warning = AlrmValueNotSpec;
        (*dev_attr_config)[loop].alarms.delta_t = AlrmValueNotSpec;
        (*dev_attr_config)[loop].alarms.delta_val = AlrmValueNotSpec;
        (*dev_attr_config)[loop].events.ch_event.abs_change = AlrmValueNotSpec;
        (*dev_attr_config)[loop].events.ch_event.rel_change = AlrmValueNotSpec;
        (*dev_attr_config)[loop].events.per_event.period = AlrmValueNotSpec;
        (*dev_attr_config)[loop].events.arch_event.archive_abs_change = AlrmValueNotSpec;
        (*dev_attr_config)[loop].events.arch_event.archive_rel_change = AlrmValueNotSpec;
        (*dev_attr_config)[loop].events.arch_event.archive_period = AlrmValueNotSpec;
    }

//
// If device does not use db, simply retruns
//

    if (dbase_used == false)
    {
        return;
    }
    else
    {

//
// First get device class (if not already done)
//

        if (_info.dev_class.empty() == true)
        {
            this->info();
        }

//
// Get class attribute properties
//

        DbData db_data_class, db_data_device;
        unsigned int i, k;
        int j;
        for (i = 0; i < dev_attr_config->size(); i++)
        {
            db_data_class.push_back(DbDatum((*dev_attr_config)[i].name));
            db_data_device.push_back(DbDatum((*dev_attr_config)[i].name));
        }
        db_dev->get_dbase()->get_class_attribute_property(_info.dev_class, db_data_class);

//
// Now get device attribute properties
//

        db_dev->get_attribute_property(db_data_device);

//
// Init remaining parameters from them retrieve at class level
//

        for (i = 0; i < db_data_class.size(); i++)
        {
            long nb_prop;

            string &att_name = db_data_class[i].name;
            db_data_class[i] >> nb_prop;
            i++;

            for (j = 0; j < nb_prop; j++)
            {

//
// Extract prop value
//

                string prop_value;
                string &prop_name = db_data_class[i].name;
                if (db_data_class[i].size() != 1)
                {
                    vector<string> tmp;
                    db_data_class[i] >> tmp;
                    prop_value = tmp[0] + ", " + tmp[1];
                }
                else
                    db_data_class[i] >> prop_value;
                i++;

//
// Store prop value in attribute config vector
//

                for (k = 0; k < dev_attr_config->size(); k++)
                {
                    if ((*dev_attr_config)[k].name == att_name)
                    {
                        if (prop_name == "min_warning")
                            (*dev_attr_config)[k].alarms.min_warning = prop_value;
                        else if (prop_name == "max_warning")
                            (*dev_attr_config)[k].alarms.max_warning = prop_value;
                        else if (prop_name == "delta_t")
                            (*dev_attr_config)[k].alarms.delta_t = prop_value;
                        else if (prop_name == "delta_val")
                            (*dev_attr_config)[k].alarms.delta_val = prop_value;
                        else if (prop_name == "abs_change")
                            (*dev_attr_config)[k].events.ch_event.abs_change = prop_value;
                        else if (prop_name == "rel_change")
                            (*dev_attr_config)[k].events.ch_event.rel_change = prop_value;
                        else if (prop_name == "period")
                            (*dev_attr_config)[k].events.per_event.period = prop_value;
                        else if (prop_name == "archive_abs_change")
                            (*dev_attr_config)[k].events.arch_event.archive_abs_change = prop_value;
                        else if (prop_name == "archive_rel_change")
                            (*dev_attr_config)[k].events.arch_event.archive_rel_change = prop_value;
                        else if (prop_name == "archive_period")
                            (*dev_attr_config)[k].events.arch_event.archive_period = prop_value;
                    }
                }
            }
        }

//
// Init remaining parameters from them retrieve at device level
//

        for (i = 0; i < db_data_device.size(); i++)
        {
            long nb_prop;

            string &att_name = db_data_device[i].name;
            db_data_device[i] >> nb_prop;
            i++;

            for (j = 0; j < nb_prop; j++)
            {

//
// Extract prop value
//

                string prop_value;
                string &prop_name = db_data_device[i].name;
                if (db_data_device[i].size() != 1)
                {
                    vector<string> tmp;
                    db_data_device[i] >> tmp;
                    prop_value = tmp[0] + ", " + tmp[1];
                }
                else
                    db_data_device[i] >> prop_value;
                i++;

//
// Store prop value in attribute config vector
//

                for (k = 0; k < dev_attr_config->size(); k++)
                {
                    if ((*dev_attr_config)[k].name == att_name)
                    {
                        if (prop_name == "min_warning")
                            (*dev_attr_config)[k].alarms.min_warning = prop_value;
                        else if (prop_name == "max_warning")
                            (*dev_attr_config)[k].alarms.max_warning = prop_value;
                        else if (prop_name == "delta_t")
                            (*dev_attr_config)[k].alarms.delta_t = prop_value;
                        else if (prop_name == "delta_val")
                            (*dev_attr_config)[k].alarms.delta_val = prop_value;
                        else if (prop_name == "abs_change")
                            (*dev_attr_config)[k].events.ch_event.abs_change = prop_value;
                        else if (prop_name == "rel_change")
                            (*dev_attr_config)[k].events.ch_event.rel_change = prop_value;
                        else if (prop_name == "period")
                            (*dev_attr_config)[k].events.per_event.period = prop_value;
                        else if (prop_name == "archive_abs_change")
                            (*dev_attr_config)[k].events.arch_event.archive_abs_change = prop_value;
                        else if (prop_name == "archive_rel_change")
                            (*dev_attr_config)[k].events.arch_event.archive_rel_change = prop_value;
                        else if (prop_name == "archive_period")
                            (*dev_attr_config)[k].events.arch_event.archive_period = prop_value;
                    }
                }
            }
        }

    }

}


//-----------------------------------------------------------------------------
//
// DeviceProxy::get_attribute_config() - return a single attribute config
//
//-----------------------------------------------------------------------------

AttributeInfoEx DeviceProxy::get_attribute_config(const string &attr_string)
{
    vector<string> attr_string_list;
    AttributeInfoListEx *dev_attr_config_list;
    AttributeInfoEx dev_attr_config;

    attr_string_list.push_back(attr_string);
    dev_attr_config_list = get_attribute_config_ex(attr_string_list);

    dev_attr_config = (*dev_attr_config_list)[0];
    delete (dev_attr_config_list);

    return (dev_attr_config);
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::set_attribute_config() - set config for a list of attributes
//
//-----------------------------------------------------------------------------

void DeviceProxy::set_attribute_config(AttributeInfoList &dev_attr_list)
{
    AttributeConfigList attr_config_list;
    DevVarStringArray attr_list;
    int ctr = 0;

    attr_config_list.length(dev_attr_list.size());

    for (unsigned int i = 0; i < attr_config_list.length(); i++)
    {
        attr_config_list[i].name = dev_attr_list[i].name.c_str();
        attr_config_list[i].writable = dev_attr_list[i].writable;
        attr_config_list[i].data_format = dev_attr_list[i].data_format;
        attr_config_list[i].data_type = dev_attr_list[i].data_type;
        attr_config_list[i].max_dim_x = dev_attr_list[i].max_dim_x;
        attr_config_list[i].max_dim_y = dev_attr_list[i].max_dim_y;
        attr_config_list[i].description = dev_attr_list[i].description.c_str();
        attr_config_list[i].label = dev_attr_list[i].label.c_str();
        attr_config_list[i].unit = dev_attr_list[i].unit.c_str();
        attr_config_list[i].standard_unit = dev_attr_list[i].standard_unit.c_str();
        attr_config_list[i].display_unit = dev_attr_list[i].display_unit.c_str();
        attr_config_list[i].format = dev_attr_list[i].format.c_str();
        attr_config_list[i].min_value = dev_attr_list[i].min_value.c_str();
        attr_config_list[i].max_value = dev_attr_list[i].max_value.c_str();
        attr_config_list[i].min_alarm = dev_attr_list[i].min_alarm.c_str();
        attr_config_list[i].max_alarm = dev_attr_list[i].max_alarm.c_str();
        attr_config_list[i].writable_attr_name = dev_attr_list[i].writable_attr_name.c_str();
        attr_config_list[i].extensions.length(dev_attr_list[i].extensions.size());
        for (unsigned int j = 0; j < dev_attr_list[i].extensions.size(); j++)
        {
            attr_config_list[i].extensions[j] = string_dup(dev_attr_list[i].extensions[j].c_str());
        }
    }

    while (ctr < 2)
    {
        try
        {
            check_and_reconnect();

            Device_var dev = Device::_duplicate(device);
            dev->set_attribute_config(attr_config_list);
            ctr = 2;

        }
        catch (Tango::DevFailed &e)
        {
            if (::strcmp(e.errors[0].reason, DEVICE_UNLOCKED_REASON) == 0)
            {
                TangoSys_OMemStream desc;
                desc << "Failed to execute set_attribute_config on device " << device_name << ends;

                DeviceUnlockedExcept::re_throw_exception(e, (const char *) DEVICE_UNLOCKED_REASON,
                                                         desc.str(),
                                                         (const char *) "DeviceProxy::set_attribute_config()");
            }
            else
                throw;
        }
        catch (CORBA::TRANSIENT &trans)
        {
            TRANSIENT_NOT_EXIST_EXCEPT(trans, "DeviceProxy", "set_attribute_config", this);
        }
        catch (CORBA::OBJECT_NOT_EXIST &one)
        {
            if (one.minor() == omni::OBJECT_NOT_EXIST_NoMatch || one.minor() == 0)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(one, "DeviceProxy", "set_attribute_config", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute set_attribute_config on device " << device_name << ends;
                ApiCommExcept::re_throw_exception(one,
                                                  (const char *) "API_CommunicationFailed",
                                                  desc.str(),
                                                  (const char *) "DeviceProxy::set_attribute_config()");
            }
        }
        catch (CORBA::COMM_FAILURE &comm)
        {
            if (comm.minor() == omni::COMM_FAILURE_WaitingForReply)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(comm, "DeviceProxy", "set_attribute_config", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute set_attribute_config on device " << device_name << ends;
                ApiCommExcept::re_throw_exception(comm,
                                                  (const char *) "API_CommunicationFailed",
                                                  desc.str(),
                                                  (const char *) "DeviceProxy::set_attribute_config()");
            }
        }
        catch (CORBA::SystemException &ce)
        {
            set_connection_state(CONNECTION_NOTOK);

            TangoSys_OMemStream desc;
            desc << "Failed to execute set_attribute_config on device " << device_name << ends;
            ApiCommExcept::re_throw_exception(ce,
                                              (const char *) "API_CommunicationFailed",
                                              desc.str(),
                                              (const char *) "DeviceProxy::set_attribute_config()");
        }
    }


    return;
}

void DeviceProxy::set_attribute_config(AttributeInfoListEx &dev_attr_list)
{
    AttributeConfigList attr_config_list;
    AttributeConfigList_3 attr_config_list_3;
    AttributeConfigList_5 attr_config_list_5;
    DevVarStringArray attr_list;
    int ctr = 0;
    unsigned int i, j;


    if (version >= 5)
    {
        attr_config_list_5.length(dev_attr_list.size());

        for (i = 0; i < attr_config_list_5.length(); i++)
        {
            ApiUtil::AttributeInfoEx_to_AttributeConfig(&dev_attr_list[i], &attr_config_list_5[i]);
        }
    }
    else if (version >= 3)
    {
        attr_config_list_3.length(dev_attr_list.size());

        for (i = 0; i < attr_config_list_3.length(); i++)
        {
            attr_config_list_3[i].name = dev_attr_list[i].name.c_str();
            attr_config_list_3[i].writable = dev_attr_list[i].writable;
            attr_config_list_3[i].data_format = dev_attr_list[i].data_format;
            attr_config_list_3[i].data_type = dev_attr_list[i].data_type;
            attr_config_list_3[i].max_dim_x = dev_attr_list[i].max_dim_x;
            attr_config_list_3[i].max_dim_y = dev_attr_list[i].max_dim_y;
            attr_config_list_3[i].description = dev_attr_list[i].description.c_str();
            attr_config_list_3[i].label = dev_attr_list[i].label.c_str();
            attr_config_list_3[i].unit = dev_attr_list[i].unit.c_str();
            attr_config_list_3[i].standard_unit = dev_attr_list[i].standard_unit.c_str();
            attr_config_list_3[i].display_unit = dev_attr_list[i].display_unit.c_str();
            attr_config_list_3[i].format = dev_attr_list[i].format.c_str();
            attr_config_list_3[i].min_value = dev_attr_list[i].min_value.c_str();
            attr_config_list_3[i].max_value = dev_attr_list[i].max_value.c_str();
            attr_config_list_3[i].writable_attr_name = dev_attr_list[i].writable_attr_name.c_str();
            attr_config_list_3[i].level = dev_attr_list[i].disp_level;
            attr_config_list_3[i].extensions.length(dev_attr_list[i].extensions.size());
            for (j = 0; j < dev_attr_list[i].extensions.size(); j++)
            {
                attr_config_list_3[i].extensions[j] = string_dup(dev_attr_list[i].extensions[j].c_str());
            }
            for (j = 0; j < dev_attr_list[i].sys_extensions.size(); j++)
            {
                attr_config_list_3[i].sys_extensions[j] = string_dup(dev_attr_list[i].sys_extensions[j].c_str());
            }

            attr_config_list_3[i].att_alarm.min_alarm = dev_attr_list[i].alarms.min_alarm.c_str();
            attr_config_list_3[i].att_alarm.max_alarm = dev_attr_list[i].alarms.max_alarm.c_str();
            attr_config_list_3[i].att_alarm.min_warning = dev_attr_list[i].alarms.min_warning.c_str();
            attr_config_list_3[i].att_alarm.max_warning = dev_attr_list[i].alarms.max_warning.c_str();
            attr_config_list_3[i].att_alarm.delta_t = dev_attr_list[i].alarms.delta_t.c_str();
            attr_config_list_3[i].att_alarm.delta_val = dev_attr_list[i].alarms.delta_val.c_str();
            for (j = 0; j < dev_attr_list[i].alarms.extensions.size(); j++)
            {
                attr_config_list_3[i].att_alarm.extensions[j] = string_dup(
                    dev_attr_list[i].alarms.extensions[j].c_str());
            }

            attr_config_list_3[i].event_prop.ch_event.rel_change = dev_attr_list[i].events.ch_event.rel_change.c_str();
            attr_config_list_3[i].event_prop.ch_event.abs_change = dev_attr_list[i].events.ch_event.abs_change.c_str();
            for (j = 0; j < dev_attr_list[i].events.ch_event.extensions.size(); j++)
            {
                attr_config_list_3[i].event_prop.ch_event.extensions[j] = string_dup(
                    dev_attr_list[i].events.ch_event.extensions[j].c_str());
            }

            attr_config_list_3[i].event_prop.per_event.period = dev_attr_list[i].events.per_event.period.c_str();
            for (j = 0; j < dev_attr_list[i].events.per_event.extensions.size(); j++)
            {
                attr_config_list_3[i].event_prop.per_event.extensions[j] = string_dup(
                    dev_attr_list[i].events.per_event.extensions[j].c_str());
            }

            attr_config_list_3[i].event_prop.arch_event.rel_change =
                dev_attr_list[i].events.arch_event.archive_rel_change.c_str();
            attr_config_list_3[i].event_prop.arch_event.abs_change =
                dev_attr_list[i].events.arch_event.archive_abs_change.c_str();
            attr_config_list_3[i].event_prop.arch_event.period =
                dev_attr_list[i].events.arch_event.archive_period.c_str();
            for (j = 0; j < dev_attr_list[i].events.ch_event.extensions.size(); j++)
            {
                attr_config_list_3[i].event_prop.arch_event.extensions[j] = string_dup(
                    dev_attr_list[i].events.arch_event.extensions[j].c_str());
            }
        }
    }
    else
    {
        attr_config_list.length(dev_attr_list.size());

        for (i = 0; i < attr_config_list.length(); i++)
        {
            attr_config_list[i].name = dev_attr_list[i].name.c_str();
            attr_config_list[i].writable = dev_attr_list[i].writable;
            attr_config_list[i].data_format = dev_attr_list[i].data_format;
            attr_config_list[i].data_type = dev_attr_list[i].data_type;
            attr_config_list[i].max_dim_x = dev_attr_list[i].max_dim_x;
            attr_config_list[i].max_dim_y = dev_attr_list[i].max_dim_y;
            attr_config_list[i].description = dev_attr_list[i].description.c_str();
            attr_config_list[i].label = dev_attr_list[i].label.c_str();
            attr_config_list[i].unit = dev_attr_list[i].unit.c_str();
            attr_config_list[i].standard_unit = dev_attr_list[i].standard_unit.c_str();
            attr_config_list[i].display_unit = dev_attr_list[i].display_unit.c_str();
            attr_config_list[i].format = dev_attr_list[i].format.c_str();
            attr_config_list[i].min_value = dev_attr_list[i].min_value.c_str();
            attr_config_list[i].max_value = dev_attr_list[i].max_value.c_str();
            attr_config_list[i].min_alarm = dev_attr_list[i].min_alarm.c_str();
            attr_config_list[i].max_alarm = dev_attr_list[i].max_alarm.c_str();
            attr_config_list[i].writable_attr_name = dev_attr_list[i].writable_attr_name.c_str();
            attr_config_list[i].extensions.length(dev_attr_list[i].extensions.size());
            for (j = 0; j < dev_attr_list[i].extensions.size(); j++)
            {
                attr_config_list_3[i].extensions[j] = string_dup(dev_attr_list[i].extensions[j].c_str());
            }
        }
    }

    while (ctr < 2)
    {
        try
        {
            check_and_reconnect();

            if (version >= 4)
            {
                ClntIdent ci;
                ApiUtil *au = ApiUtil::instance();
                ci.cpp_clnt(au->get_client_pid());

                if (version >= 5)
                {
                    Device_5_var dev = Device_5::_duplicate(device_5);
                    dev->set_attribute_config_5(attr_config_list_5, ci);
                }
                else
                {
                    Device_4_var dev = Device_4::_duplicate(device_4);
                    dev->set_attribute_config_4(attr_config_list_3, ci);
                }
            }
            else if (version == 3)
            {
                Device_3_var dev = Device_3::_duplicate(device_3);
                dev->set_attribute_config_3(attr_config_list_3);
            }
            else
            {
                Device_var dev = Device::_duplicate(device);
                device->set_attribute_config(attr_config_list);
            }
            ctr = 2;

        }
        catch (Tango::DevFailed &e)
        {
            if (::strcmp(e.errors[0].reason, DEVICE_UNLOCKED_REASON) == 0)
            {
                TangoSys_OMemStream desc;
                desc << "Failed to execute set_attribute_config on device " << device_name << ends;

                DeviceUnlockedExcept::re_throw_exception(e, (const char *) DEVICE_UNLOCKED_REASON,
                                                         desc.str(),
                                                         (const char *) "DeviceProxy::set_attribute_config()");
            }
            else
                throw;
        }
        catch (CORBA::TRANSIENT &trans)
        {
            TRANSIENT_NOT_EXIST_EXCEPT(trans, "DeviceProxy", "set_attribute_config", this);
        }
        catch (CORBA::OBJECT_NOT_EXIST &one)
        {
            if (one.minor() == omni::OBJECT_NOT_EXIST_NoMatch || one.minor() == 0)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(one, "DeviceProxy", "set_attribute_config", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute set_attribute_config on device " << device_name << ends;
                ApiCommExcept::re_throw_exception(one,
                                                  (const char *) "API_CommunicationFailed",
                                                  desc.str(),
                                                  (const char *) "DeviceProxy::set_attribute_config()");
            }
        }
        catch (CORBA::COMM_FAILURE &comm)
        {
            if (comm.minor() == omni::COMM_FAILURE_WaitingForReply)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(comm, "DeviceProxy", "set_attribute_config", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute set_attribute_config on device " << device_name << ends;
                ApiCommExcept::re_throw_exception(comm,
                                                  (const char *) "API_CommunicationFailed",
                                                  desc.str(),
                                                  (const char *) "DeviceProxy::set_attribute_config()");
            }
        }
        catch (CORBA::SystemException &ce)
        {
            set_connection_state(CONNECTION_NOTOK);

            TangoSys_OMemStream desc;
            desc << "Failed to execute set_attribute_config on device " << device_name << ends;
            ApiCommExcept::re_throw_exception(ce,
                                              (const char *) "API_CommunicationFailed",
                                              desc.str(),
                                              (const char *) "DeviceProxy::set_attribute_config()");
        }
    }

    return;
}


//-----------------------------------------------------------------------------
//
// DeviceProxy::get_pipe_config() - return a list of pipe config
//
//-----------------------------------------------------------------------------

PipeInfoList *DeviceProxy::get_pipe_config(vector<string> &pipe_string_list)
{
    PipeConfigList_var pipe_config_list_5;
    PipeInfoList *dev_pipe_config = new PipeInfoList();
    DevVarStringArray pipe_list;
    int ctr = 0;

//
// Error if device does not support IDL 5
//

    if (version > 0 && version < 5)
    {
        stringstream ss;
        ss << "Device " << device_name << " too old to use get_pipe_config() call. Please upgrade to Tango 9/IDL5";

        ApiNonSuppExcept::throw_exception(API_UnsupportedFeature,
                                          ss.str(), "DeviceProxy::get_pipe_config()");
    }

//
// Prepare sent parameters
//

    pipe_list.length(pipe_string_list.size());
    for (unsigned int i = 0; i < pipe_string_list.size(); i++)
    {
        if (pipe_string_list[i] == AllPipe)
            pipe_list[i] = string_dup(AllPipe);
        else
            pipe_list[i] = string_dup(pipe_string_list[i].c_str());
    }

//
// Call device
//

    while (ctr < 2)
    {
        try
        {
            check_and_reconnect();

            Device_5_var dev = Device_5::_duplicate(device_5);
            pipe_config_list_5 = dev->get_pipe_config_5(pipe_list);
            dev_pipe_config->resize(pipe_config_list_5->length());

            for (size_t i = 0; i < pipe_config_list_5->length(); i++)
            {
                (*dev_pipe_config)[i].disp_level = pipe_config_list_5[i].level;
                (*dev_pipe_config)[i].name = pipe_config_list_5[i].name;
                (*dev_pipe_config)[i].description = pipe_config_list_5[i].description;
                (*dev_pipe_config)[i].label = pipe_config_list_5[i].label;
                (*dev_pipe_config)[i].writable = pipe_config_list_5[i].writable;
                for (size_t j = 0; j < pipe_config_list_5[i].extensions.length(); j++)
                {
                    (*dev_pipe_config)[i].extensions[j] = pipe_config_list_5[i].extensions[j];
                }
            }

            ctr = 2;
        }
        catch (CORBA::TRANSIENT &trans)
        {
            TRANSIENT_NOT_EXIST_EXCEPT(trans, "DeviceProxy", "get_pipe_config", this);
        }
        catch (CORBA::OBJECT_NOT_EXIST &one)
        {
            if (one.minor() == omni::OBJECT_NOT_EXIST_NoMatch || one.minor() == 0)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(one, "DeviceProxy", "get_pipe_config", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute get_pipe_config on device " << device_name << ends;
                ApiCommExcept::re_throw_exception(one, "API_CommunicationFailed",
                                                  desc.str(), "DeviceProxy::get_pipe_config()");
            }
        }
        catch (CORBA::COMM_FAILURE &comm)
        {
            if (comm.minor() == omni::COMM_FAILURE_WaitingForReply)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(comm, "DeviceProxy", "get_pipe_config", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute get_pipe_config on device " << device_name << ends;
                ApiCommExcept::re_throw_exception(comm, "API_CommunicationFailed",
                                                  desc.str(), "DeviceProxy::get_pipe_config()");
            }
        }
        catch (CORBA::SystemException &ce)
        {
            set_connection_state(CONNECTION_NOTOK);

            TangoSys_OMemStream desc;
            desc << "Failed to execute get_pipe_config on device " << device_name << ends;
            ApiCommExcept::re_throw_exception(ce, "API_CommunicationFailed",
                                              desc.str(), "DeviceProxy::get_pipe_config()");
        }
        catch (Tango::DevFailed)
        {
            delete dev_pipe_config;
            throw;
        }
    }

    return (dev_pipe_config);
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::get_pipe_config() - return a pipe config
//
//-----------------------------------------------------------------------------

PipeInfo DeviceProxy::get_pipe_config(const string &pipe_name)
{
    vector<string> pipe_string_list;
    PipeInfoList *dev_pipe_config_list;
    PipeInfo dev_pipe_config;

    pipe_string_list.push_back(pipe_name);
    dev_pipe_config_list = get_pipe_config(pipe_string_list);

    dev_pipe_config = (*dev_pipe_config_list)[0];
    delete (dev_pipe_config_list);

    return (dev_pipe_config);
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::set_pipe_config() - set config for a list of pipes
//
//-----------------------------------------------------------------------------

void DeviceProxy::set_pipe_config(PipeInfoList &dev_pipe_list)
{
//
// Error if device does not support IDL 5
//

    if (version > 0 && version < 5)
    {
        stringstream ss;
        ss << "Device " << device_name << " too old to use set_pipe_config() call. Please upgrade to Tango 9/IDL5";

        ApiNonSuppExcept::throw_exception(API_UnsupportedFeature, ss.str(), "DeviceProxy::set_pipe_config()");
    }

    PipeConfigList pipe_config_list;
    int ctr = 0;

    pipe_config_list.length(dev_pipe_list.size());

    for (unsigned int i = 0; i < pipe_config_list.length(); i++)
    {
        pipe_config_list[i].name = dev_pipe_list[i].name.c_str();
        pipe_config_list[i].writable = dev_pipe_list[i].writable;
        pipe_config_list[i].description = dev_pipe_list[i].description.c_str();
        pipe_config_list[i].label = dev_pipe_list[i].label.c_str();
        pipe_config_list[i].level = dev_pipe_list[i].disp_level;
        pipe_config_list[i].extensions.length(dev_pipe_list[i].extensions.size());
        for (unsigned int j = 0; j < dev_pipe_list[i].extensions.size(); j++)
        {
            pipe_config_list[i].extensions[j] = string_dup(dev_pipe_list[i].extensions[j].c_str());
        }
    }

    while (ctr < 2)
    {
        try
        {
            check_and_reconnect();

            ClntIdent ci;
            ApiUtil *au = ApiUtil::instance();
            ci.cpp_clnt(au->get_client_pid());
            Device_5_var dev = Device_5::_duplicate(device_5);
            dev->set_pipe_config_5(pipe_config_list, ci);

            ctr = 2;

        }
        catch (Tango::DevFailed &e)
        {
            if (::strcmp(e.errors[0].reason, DEVICE_UNLOCKED_REASON) == 0)
            {
                TangoSys_OMemStream desc;
                desc << "Failed to execute set_pipe_config on device " << device_name << ends;

                DeviceUnlockedExcept::re_throw_exception(e, DEVICE_UNLOCKED_REASON,
                                                         desc.str(), "DeviceProxy::set_pipe_config()");
            }
            else
                throw;
        }
        catch (CORBA::TRANSIENT &trans)
        {
            TRANSIENT_NOT_EXIST_EXCEPT(trans, "DeviceProxy", "set_pipe_config", this);
        }
        catch (CORBA::OBJECT_NOT_EXIST &one)
        {
            if (one.minor() == omni::OBJECT_NOT_EXIST_NoMatch || one.minor() == 0)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(one, "DeviceProxy", "set_pipe_config", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute set_pipe_config on device " << device_name << ends;
                ApiCommExcept::re_throw_exception(one,
                                                  (const char *) "API_CommunicationFailed",
                                                  desc.str(),
                                                  (const char *) "DeviceProxy::set_pipe_config()");
            }
        }
        catch (CORBA::COMM_FAILURE &comm)
        {
            if (comm.minor() == omni::COMM_FAILURE_WaitingForReply)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(comm, "DeviceProxy", "set_pipe_config", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute set_pipe_config on device " << device_name << ends;
                ApiCommExcept::re_throw_exception(comm,
                                                  (const char *) "API_CommunicationFailed",
                                                  desc.str(),
                                                  (const char *) "DeviceProxy::set_pipe_config()");
            }
        }
        catch (CORBA::SystemException &ce)
        {
            set_connection_state(CONNECTION_NOTOK);

            TangoSys_OMemStream desc;
            desc << "Failed to execute set_pipe_config on device " << device_name << ends;
            ApiCommExcept::re_throw_exception(ce,
                                              (const char *) "API_CommunicationFailed",
                                              desc.str(),
                                              (const char *) "DeviceProxy::set_pipe_config()");
        }
    }


    return;
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::get_pipe_list() - get list of pipes
//
//-----------------------------------------------------------------------------

vector<string> *DeviceProxy::get_pipe_list()
{
    vector<string> all_pipe;
    PipeInfoList *all_pipe_config;

    all_pipe.push_back(AllPipe);
    all_pipe_config = get_pipe_config(all_pipe);

    vector<string> *pipe_list = new vector<string>;
    pipe_list->resize(all_pipe_config->size());
    for (unsigned int i = 0; i < all_pipe_config->size(); i++)
    {
        (*pipe_list)[i] = (*all_pipe_config)[i].name;
    }
    delete all_pipe_config;

    return pipe_list;
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::read_pipe() - read a single pipe
//
//-----------------------------------------------------------------------------


DevicePipe DeviceProxy::read_pipe(const string &pipe_name)
{
    DevPipeData_var pipe_value_5;
    DevicePipe dev_pipe;
    int ctr = 0;

//
// Error if device does not support IDL 5
//

    if (version > 0 && version < 5)
    {
        stringstream ss;
        ss << "Device " << device_name << " too old to use read_pipe() call. Please upgrade to Tango 9/IDL5";

        ApiNonSuppExcept::throw_exception(API_UnsupportedFeature, ss.str(), "DeviceProxy::read_pipe()");
    }

    while (ctr < 2)
    {
        try
        {
            check_and_reconnect();

            ClntIdent ci;
            ApiUtil *au = ApiUtil::instance();
            ci.cpp_clnt(au->get_client_pid());
            Device_5_var dev = Device_5::_duplicate(device_5);
            pipe_value_5 = dev->read_pipe_5(pipe_name.c_str(), ci);

            ctr = 2;
        }
        catch (Tango::ConnectionFailed &e)
        {
            stringstream desc;
            desc << "Failed to read_pipe on device " << device_name << ", pipe " << pipe_name;
            ApiConnExcept::re_throw_exception(e, API_PipeFailed, desc.str(), "DeviceProxy::read_pipe()");
        }
        catch (Tango::DevFailed &e)
        {
            stringstream desc;
            desc << "Failed to read_pipe on device " << device_name << ", pipe " << pipe_name;
            Except::re_throw_exception(e, API_PipeFailed, desc.str(), "DeviceProxy::read_pipe()");
        }
        catch (CORBA::TRANSIENT &trans)
        {
            TRANSIENT_NOT_EXIST_EXCEPT(trans, "DeviceProxy", "read_pipe", this);
        }
        catch (CORBA::OBJECT_NOT_EXIST &one)
        {
            if (one.minor() == omni::OBJECT_NOT_EXIST_NoMatch || one.minor() == 0)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(one, "DeviceProxy", "read_pipe", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                stringstream desc;
                desc << "Failed to read_pipe on device " << device_name;
                ApiCommExcept::re_throw_exception(one, "API_CommunicationFailed", desc.str(),
                                                  "DeviceProxy::read_pipe()");
            }
        }
        catch (CORBA::COMM_FAILURE &comm)
        {
            if (comm.minor() == omni::COMM_FAILURE_WaitingForReply)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(comm, "DeviceProxy", "read_pipe", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                stringstream desc;
                desc << "Failed to read_pipe on device " << device_name;
                ApiCommExcept::re_throw_exception(comm, "API_CommunicationFailed", desc.str(),
                                                  "DeviceProxy::read_pipe()");
            }
        }
        catch (CORBA::SystemException &ce)
        {
            set_connection_state(CONNECTION_NOTOK);
            stringstream desc;
            desc << "Failed to read_pipe on device " << device_name;
            ApiCommExcept::re_throw_exception(ce, "API_CommunicationFailed", desc.str(),
                                              "DeviceProxy::read_pipe()");
        }
    }

//
// Pass received data to the caller.
// For thw data elt sequence, we create a new one with size and buffer from the original one.
// This is required because the whole object received by the call will be deleted at the end of this method
//

    dev_pipe.set_name(pipe_value_5->name.in());
    dev_pipe.set_time(pipe_value_5->time);

    DevULong max, len;
    max = pipe_value_5->data_blob.blob_data.maximum();
    len = pipe_value_5->data_blob.blob_data.length();
    DevPipeDataElt *buf = pipe_value_5->data_blob.blob_data.get_buffer((DevBoolean) true);
    DevVarPipeDataEltArray *dvpdea = new DevVarPipeDataEltArray(max, len, buf, true);

    dev_pipe.get_root_blob().reset_extract_ctr();
    dev_pipe.get_root_blob().reset_insert_ctr();
    dev_pipe.get_root_blob().set_name(pipe_value_5->data_blob.name.in());
    delete dev_pipe.get_root_blob().get_extract_data();
    dev_pipe.get_root_blob().set_extract_data(dvpdea);
    dev_pipe.get_root_blob().set_extract_delete(true);

    return dev_pipe;
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::write_pipe() - write a single pipe
//
//-----------------------------------------------------------------------------


void DeviceProxy::write_pipe(DevicePipe &dev_pipe)
{
    DevPipeData pipe_value_5;
    int ctr = 0;

//
// Error if device does not support IDL 5
//

    if (version > 0 && version < 5)
    {
        stringstream ss;
        ss << "Device " << device_name << " too old to use write_pipe() call. Please upgrade to Tango 9/IDL5";

        ApiNonSuppExcept::throw_exception(API_UnsupportedFeature, ss.str(), "DeviceProxy::write_pipe()");
    }

//
// Prepare data sent to device
//

    pipe_value_5.name = dev_pipe.get_name().c_str();
    const string &bl_name = dev_pipe.get_root_blob().get_name();
    if (bl_name.size() != 0)
        pipe_value_5.data_blob.name = bl_name.c_str();

    DevVarPipeDataEltArray *tmp_ptr = dev_pipe.get_root_blob().get_insert_data();
    if (tmp_ptr == Tango_nullptr)
    {
        Except::throw_exception(API_PipeNoDataElement, "No data in pipe!", "DeviceProxy::write_pipe()");
    }

    DevULong max, len;
    max = tmp_ptr->maximum();
    len = tmp_ptr->length();
    pipe_value_5.data_blob.blob_data.replace(max, len, tmp_ptr->get_buffer((DevBoolean) true), true);

    while (ctr < 2)
    {
        try
        {
            check_and_reconnect();

            ClntIdent ci;
            ApiUtil *au = ApiUtil::instance();
            ci.cpp_clnt(au->get_client_pid());
            Device_5_var dev = Device_5::_duplicate(device_5);
            dev->write_pipe_5(pipe_value_5, ci);

            ctr = 2;
        }
        catch (Tango::ConnectionFailed &e)
        {
            dev_pipe.get_root_blob().reset_insert_ctr();
            delete tmp_ptr;

            stringstream desc;
            desc << "Failed to write_pipe on device " << device_name << ", pipe " << dev_pipe.get_name();
            ApiConnExcept::re_throw_exception(e, API_PipeFailed, desc.str(), "DeviceProxy::write_pipe()");
        }
        catch (Tango::DevFailed &e)
        {
            dev_pipe.get_root_blob().reset_insert_ctr();
            delete tmp_ptr;

            stringstream desc;
            desc << "Failed to write_pipe on device " << device_name << ", pipe " << dev_pipe.get_name();
            Except::re_throw_exception(e, API_PipeFailed, desc.str(), "DeviceProxy::write_pipe()");
        }
        catch (CORBA::TRANSIENT &trans)
        {
            TRANSIENT_NOT_EXIST_EXCEPT(trans, "DeviceProxy", "write_pipe", this);
        }
        catch (CORBA::OBJECT_NOT_EXIST &one)
        {
            if (one.minor() == omni::OBJECT_NOT_EXIST_NoMatch || one.minor() == 0)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(one, "DeviceProxy", "write_pipe", this);
            }
            else
            {
                dev_pipe.get_root_blob().reset_insert_ctr();
                delete tmp_ptr;

                set_connection_state(CONNECTION_NOTOK);
                stringstream desc;
                desc << "Failed to write_pipe on device " << device_name;
                ApiCommExcept::re_throw_exception(one, "API_CommunicationFailed", desc.str(),
                                                  "DeviceProxy::write_pipe()");
            }
        }
        catch (CORBA::COMM_FAILURE &comm)
        {
            if (comm.minor() == omni::COMM_FAILURE_WaitingForReply)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(comm, "DeviceProxy", "write_pipe", this);
            }
            else
            {
                dev_pipe.get_root_blob().reset_insert_ctr();
                delete tmp_ptr;

                set_connection_state(CONNECTION_NOTOK);
                stringstream desc;
                desc << "Failed to write_pipe on device " << device_name;
                ApiCommExcept::re_throw_exception(comm, "API_CommunicationFailed", desc.str(),
                                                  "DeviceProxy::write_pipe()");
            }
        }
        catch (CORBA::SystemException &ce)
        {
            dev_pipe.get_root_blob().reset_insert_ctr();
            delete tmp_ptr;

            set_connection_state(CONNECTION_NOTOK);
            stringstream desc;
            desc << "Failed to write_pipe on device " << device_name;
            ApiCommExcept::re_throw_exception(ce, "API_CommunicationFailed", desc.str(),
                                              "DeviceProxy::write_pipe()");
        }
    }

    dev_pipe.get_root_blob().reset_insert_ctr();
    delete tmp_ptr;
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::write_read_pipe() - write then read a single pipe
//
//-----------------------------------------------------------------------------

DevicePipe DeviceProxy::write_read_pipe(DevicePipe &pipe_data)
{
    DevPipeData pipe_value_5;
    DevPipeData_var r_pipe_value_5;
    DevicePipe r_dev_pipe;
    int ctr = 0;

//
// Error if device does not support IDL 5
//

    if (version > 0 && version < 5)
    {
        stringstream ss;
        ss << "Device " << device_name << " too old to use write_read_pipe() call. Please upgrade to Tango 9/IDL5";

        ApiNonSuppExcept::throw_exception(API_UnsupportedFeature, ss.str(), "DeviceProxy::write_read_pipe()");
    }

//
// Prepare data sent to device
//

    pipe_value_5.name = pipe_data.get_name().c_str();
    const string &bl_name = pipe_data.get_root_blob().get_name();
    if (bl_name.size() != 0)
        pipe_value_5.data_blob.name = bl_name.c_str();

    DevVarPipeDataEltArray *tmp_ptr = pipe_data.get_root_blob().get_insert_data();
    DevULong max, len;
    max = tmp_ptr->maximum();
    len = tmp_ptr->length();
    pipe_value_5.data_blob.blob_data.replace(max, len, tmp_ptr->get_buffer((DevBoolean) true), true);

    delete tmp_ptr;

    while (ctr < 2)
    {
        try
        {
            check_and_reconnect();

            ClntIdent ci;
            ApiUtil *au = ApiUtil::instance();
            ci.cpp_clnt(au->get_client_pid());
            Device_5_var dev = Device_5::_duplicate(device_5);
            r_pipe_value_5 = dev->write_read_pipe_5(pipe_value_5, ci);

            ctr = 2;
        }
        catch (Tango::ConnectionFailed &e)
        {
            stringstream desc;
            desc << "Failed to write_read_pipe on device " << device_name << ", pipe " << pipe_data.get_name();
            ApiConnExcept::re_throw_exception(e, API_PipeFailed, desc.str(), "DeviceProxy::write_read_pipe()");
        }
        catch (Tango::DevFailed &e)
        {
            stringstream desc;
            desc << "Failed to write_pipe on device " << device_name << ", pipe " << pipe_data.get_name();
            Except::re_throw_exception(e, API_PipeFailed, desc.str(), "DeviceProxy::write_read_pipe()");
        }
        catch (CORBA::TRANSIENT &trans)
        {
            TRANSIENT_NOT_EXIST_EXCEPT(trans, "DeviceProxy", "write_read_pipe", this);
        }
        catch (CORBA::OBJECT_NOT_EXIST &one)
        {
            if (one.minor() == omni::OBJECT_NOT_EXIST_NoMatch || one.minor() == 0)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(one, "DeviceProxy", "write_read_pipe", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                stringstream desc;
                desc << "Failed to write_read_pipe on device " << device_name;
                ApiCommExcept::re_throw_exception(one, "API_CommunicationFailed", desc.str(),
                                                  "DeviceProxy::write_read_pipe()");
            }
        }
        catch (CORBA::COMM_FAILURE &comm)
        {
            if (comm.minor() == omni::COMM_FAILURE_WaitingForReply)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(comm, "DeviceProxy", "write_read_pipe", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                stringstream desc;
                desc << "Failed to write_read_pipe on device " << device_name;
                ApiCommExcept::re_throw_exception(comm, "API_CommunicationFailed", desc.str(),
                                                  "DeviceProxy::write_read_pipe()");
            }
        }
        catch (CORBA::SystemException &ce)
        {
            set_connection_state(CONNECTION_NOTOK);
            stringstream desc;
            desc << "Failed to write_read_pipe on device " << device_name;
            ApiCommExcept::re_throw_exception(ce, "API_CommunicationFailed", desc.str(),
                                              "DeviceProxy::write_read_pipe()");
        }
    }

//
// Pass received data to the caller.
// For thw data elt sequence, we create a new one with size and buffer from the original one.
// This is required because the whole object received by the call will be deleted at the end of this method
//

    r_dev_pipe.set_name(r_pipe_value_5->name.in());
    r_dev_pipe.set_time(r_pipe_value_5->time);

    max = r_pipe_value_5->data_blob.blob_data.maximum();
    len = r_pipe_value_5->data_blob.blob_data.length();
    DevPipeDataElt *buf = r_pipe_value_5->data_blob.blob_data.get_buffer((DevBoolean) true);
    DevVarPipeDataEltArray *dvpdea = new DevVarPipeDataEltArray(max, len, buf, true);

    r_dev_pipe.get_root_blob().reset_extract_ctr();
    r_dev_pipe.get_root_blob().reset_insert_ctr();
    r_dev_pipe.get_root_blob().set_name(r_pipe_value_5->data_blob.name.in());
    r_dev_pipe.get_root_blob().set_extract_data(dvpdea);
    r_dev_pipe.get_root_blob().set_extract_delete(true);

    return r_dev_pipe;

}


//-----------------------------------------------------------------------------
//
// DeviceProxy::read_attributes() - Read attributes
//
//-----------------------------------------------------------------------------

vector<DeviceAttribute> *DeviceProxy::read_attributes(vector<string> &attr_string_list)
{
    AttributeValueList_var attr_value_list;
    AttributeValueList_3_var attr_value_list_3;
    AttributeValueList_4_var attr_value_list_4;
    AttributeValueList_5_var attr_value_list_5;
    DevVarStringArray attr_list;

//
// Check that the caller did not give two times the same attribute
//

    same_att_name(attr_string_list, "Deviceproxy::read_attributes()");
    unsigned long i;

    attr_list.length(attr_string_list.size());
    for (i = 0; i < attr_string_list.size(); i++)
    {
        attr_list[i] = string_dup(attr_string_list[i].c_str());
    }

    int ctr = 0;
    Tango::DevSource local_source;

    while (ctr < 2)
    {
        try
        {
            check_and_reconnect(local_source);

            ClntIdent ci;
            ApiUtil *au = ApiUtil::instance();
            ci.cpp_clnt(au->get_client_pid());

            if (version >= 5)
            {
                Device_5_var dev = Device_5::_duplicate(device_5);
                attr_value_list_5 = dev->read_attributes_5(attr_list, local_source, ci);
            }
            else if (version == 4)
            {
                Device_4_var dev = Device_4::_duplicate(device_4);
                attr_value_list_4 = dev->read_attributes_4(attr_list, local_source, ci);
            }
            else if (version == 3)
            {
                Device_3_var dev = Device_3::_duplicate(device_3);
                attr_value_list_3 = dev->read_attributes_3(attr_list, local_source);
            }
            else if (version == 2)
            {
                Device_2_var dev = Device_2::_duplicate(device_2);
                attr_value_list = dev->read_attributes_2(attr_list, local_source);
            }
            else
            {
                Device_var dev = Device::_duplicate(device);
                attr_value_list = dev->read_attributes(attr_list);
            }

            ctr = 2;
        }
        catch (Tango::ConnectionFailed &e)
        {
            TangoSys_OMemStream desc;
            desc << "Failed to read_attributes on device " << device_name;
            desc << ", attributes ";
            int nb_attr = attr_string_list.size();
            for (int i = 0; i < nb_attr; i++)
            {
                desc << attr_string_list[i];
                if (i != nb_attr - 1)
                    desc << ", ";
            }
            desc << ends;
            ApiConnExcept::re_throw_exception(e, (const char *) API_AttributeFailed,
                                              desc.str(), (const char *) "DeviceProxy::read_attributes()");
        }
        catch (Tango::DevFailed &e)
        {
            TangoSys_OMemStream desc;
            desc << "Failed to read_attributes on device " << device_name;
            desc << ", attributes ";
            int nb_attr = attr_string_list.size();
            for (int i = 0; i < nb_attr; i++)
            {
                desc << attr_string_list[i];
                if (i != nb_attr - 1)
                    desc << ", ";
            }
            desc << ends;
            Except::re_throw_exception(e, (const char *) API_AttributeFailed,
                                       desc.str(), (const char *) "DeviceProxy::read_attributes()");
        }
        catch (CORBA::TRANSIENT &trans)
        {
            TRANSIENT_NOT_EXIST_EXCEPT(trans, "DeviceProxy", "read_attributes", this);
        }
        catch (CORBA::OBJECT_NOT_EXIST &one)
        {
            if (one.minor() == omni::OBJECT_NOT_EXIST_NoMatch || one.minor() == 0)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(one, "DeviceProxy", "read_attributes", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute read_attributes on device " << device_name << ends;
                ApiCommExcept::re_throw_exception(one,
                                                  (const char *) "API_CommunicationFailed",
                                                  desc.str(),
                                                  (const char *) "DeviceProxy::read_attributes()");
            }
        }
        catch (CORBA::COMM_FAILURE &comm)
        {
            if (comm.minor() == omni::COMM_FAILURE_WaitingForReply)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(comm, "DeviceProxy", "read_attributes", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute read_attributes on device " << device_name << ends;
                ApiCommExcept::re_throw_exception(comm,
                                                  (const char *) "API_CommunicationFailed",
                                                  desc.str(),
                                                  (const char *) "DeviceProxy::read_attributes()");
            }
        }
        catch (CORBA::SystemException &ce)
        {
            set_connection_state(CONNECTION_NOTOK);
            TangoSys_OMemStream desc;
            desc << "Failed to execute read_attributes on device " << device_name << ends;
            ApiCommExcept::re_throw_exception(ce,
                                              (const char *) "API_CommunicationFailed",
                                              desc.str(),
                                              (const char *) "DeviceProxy::read_attributes()");
        }
    }

    unsigned long nb_received;
    if (version >= 5)
        nb_received = attr_value_list_5->length();
    else if (version == 4)
        nb_received = attr_value_list_4->length();
    else if (version == 3)
        nb_received = attr_value_list_3->length();
    else
        nb_received = attr_value_list->length();

    vector<DeviceAttribute> *dev_attr = new(vector<DeviceAttribute>);
    dev_attr->resize(nb_received);

    for (i = 0; i < nb_received; i++)
    {
        if (version >= 3)
        {
            if (version >= 5)
                ApiUtil::attr_to_device(&(attr_value_list_5[i]), version, &(*dev_attr)[i]);
            else if (version == 4)
                ApiUtil::attr_to_device(&(attr_value_list_4[i]), version, &(*dev_attr)[i]);
            else
                ApiUtil::attr_to_device(NULL, &(attr_value_list_3[i]), version, &(*dev_attr)[i]);

//
// Add an error in the error stack in case there is one
//

            DevErrorList_var &err_list = (*dev_attr)[i].get_error_list();
            long nb_except = err_list.in().length();
            if (nb_except != 0)
            {
                TangoSys_OMemStream desc;
                desc << "Failed to read_attributes on device " << device_name;
                desc << ", attribute " << (*dev_attr)[i].name << ends;

                err_list.inout().length(nb_except + 1);
                err_list[nb_except].reason = Tango::string_dup(API_AttributeFailed);
                err_list[nb_except].origin = Tango::string_dup("DeviceProxy::read_attributes()");

                string st = desc.str();
                err_list[nb_except].desc = Tango::string_dup(st.c_str());
                err_list[nb_except].severity = Tango::ERR;
            }
        }
        else
        {
            ApiUtil::attr_to_device(&(attr_value_list[i]), NULL, version, &(*dev_attr)[i]);
        }
    }

    return (dev_attr);
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::read_attribute() - return a single attribute
//
//-----------------------------------------------------------------------------


DeviceAttribute DeviceProxy::read_attribute(string &attr_string)
{
    AttributeValueList_var attr_value_list;
    AttributeValueList_3_var attr_value_list_3;
    AttributeValueList_4_var attr_value_list_4;
    AttributeValueList_5_var attr_value_list_5;
    DeviceAttribute dev_attr;
    DevVarStringArray attr_list;
    int ctr = 0;
    Tango::DevSource local_source;

    attr_list.length(1);
    attr_list[0] = Tango::string_dup(attr_string.c_str());

    while (ctr < 2)
    {
        try
        {
            check_and_reconnect(local_source);

            if (version >= 5)
            {
                ClntIdent ci;
                ApiUtil *au = ApiUtil::instance();
                ci.cpp_clnt(au->get_client_pid());
                Device_5_var dev = Device_5::_duplicate(device_5);
                attr_value_list_5 = dev->read_attributes_5(attr_list, local_source, ci);
            }
            else if (version == 4)
            {
                ClntIdent ci;
                ApiUtil *au = ApiUtil::instance();
                ci.cpp_clnt(au->get_client_pid());
                Device_4_var dev = Device_4::_duplicate(device_4);
                attr_value_list_4 = dev->read_attributes_4(attr_list, local_source, ci);
            }
            else if (version == 3)
            {
                Device_3_var dev = Device_3::_duplicate(device_3);
                attr_value_list_3 = dev->read_attributes_3(attr_list, local_source);
            }
            else if (version == 2)
            {
                Device_2_var dev = Device_2::_duplicate(device_2);
                attr_value_list = dev->read_attributes_2(attr_list, local_source);
            }
            else
            {
                Device_var dev = Device::_duplicate(device);
                attr_value_list = dev->read_attributes(attr_list);
            }
            ctr = 2;
        }
        READ_ATT_EXCEPT(attr_string, this)
    }

    if (version >= 3)
    {
        if (version >= 5)
            ApiUtil::attr_to_device(&(attr_value_list_5[0]), version, &dev_attr);
        else if (version == 4)
            ApiUtil::attr_to_device(&(attr_value_list_4[0]), version, &dev_attr);
        else
            ApiUtil::attr_to_device(NULL, &(attr_value_list_3[0]), version, &dev_attr);

//
// Add an error in the error stack in case there is one
//

        DevErrorList_var &err_list = dev_attr.get_error_list();
        long nb_except = err_list.in().length();
        if (nb_except != 0)
        {
            TangoSys_OMemStream desc;
            desc << "Failed to read_attribute on device " << device_name;
            desc << ", attribute " << dev_attr.name << ends;

            err_list.inout().length(nb_except + 1);
            err_list[nb_except].reason = Tango::string_dup(API_AttributeFailed);
            err_list[nb_except].origin = Tango::string_dup("DeviceProxy::read_attribute()");

            string st = desc.str();
            err_list[nb_except].desc = Tango::string_dup(st.c_str());
            err_list[nb_except].severity = Tango::ERR;
        }
    }
    else
    {
        ApiUtil::attr_to_device(&(attr_value_list[0]), NULL, version, &dev_attr);
    }

    return (dev_attr);
}

void DeviceProxy::read_attribute(const char *attr_str, DeviceAttribute &dev_attr)
{
    AttributeValueList *attr_value_list = NULL;
    AttributeValueList_3 *attr_value_list_3 = NULL;
    AttributeValueList_4 *attr_value_list_4 = NULL;
    AttributeValueList_5 *attr_value_list_5 = NULL;
    DevVarStringArray attr_list;
    int ctr = 0;
    Tango::DevSource local_source;

    attr_list.length(1);
    attr_list[0] = Tango::string_dup(attr_str);

    while (ctr < 2)
    {
        try
        {
            check_and_reconnect(local_source);

            ClntIdent ci;
            ApiUtil *au = ApiUtil::instance();
            ci.cpp_clnt(au->get_client_pid());

            if (version >= 5)
            {
                Device_5_var dev = Device_5::_duplicate(device_5);
                attr_value_list_5 = dev->read_attributes_5(attr_list, local_source, ci);
            }
            else if (version == 4)
            {
                Device_4_var dev = Device_4::_duplicate(device_4);
                attr_value_list_4 = dev->read_attributes_4(attr_list, local_source, ci);
            }
            else if (version == 3)
            {
                Device_3_var dev = Device_3::_duplicate(device_3);
                attr_value_list_3 = dev->read_attributes_3(attr_list, local_source);
            }
            else if (version == 2)
            {
                Device_2_var dev = Device_2::_duplicate(device_2);
                attr_value_list = dev->read_attributes_2(attr_list, local_source);
            }
            else
            {
                Device_var dev = Device::_duplicate(device);
                attr_value_list = dev->read_attributes(attr_list);
            }
            ctr = 2;
        }
        READ_ATT_EXCEPT(attr_str, this)
    }

    if (version >= 3)
    {

        if (version >= 5)
        {
            ApiUtil::attr_to_device(&((*attr_value_list_5)[0]), version, &dev_attr);
            delete attr_value_list_5;
        }
        else if (version == 4)
        {
            ApiUtil::attr_to_device(&((*attr_value_list_4)[0]), version, &dev_attr);
            delete attr_value_list_4;
        }
        else
        {
            ApiUtil::attr_to_device(NULL, &((*attr_value_list_3)[0]), version, &dev_attr);
            delete attr_value_list_3;
        }

//
// Add an error in the error stack in case there is one
//

        DevErrorList_var &err_list = dev_attr.get_error_list();
        long nb_except = err_list.in().length();
        if (nb_except != 0)
        {
            TangoSys_OMemStream desc;
            desc << "Failed to read_attribute on device " << device_name;
            desc << ", attribute " << dev_attr.name << ends;

            err_list.inout().length(nb_except + 1);
            err_list[nb_except].reason = Tango::string_dup(API_AttributeFailed);
            err_list[nb_except].origin = Tango::string_dup("DeviceProxy::read_attribute()");

            string st = desc.str();
            err_list[nb_except].desc = Tango::string_dup(st.c_str());
            err_list[nb_except].severity = Tango::ERR;
        }
    }
    else
    {
        ApiUtil::attr_to_device(&((*attr_value_list)[0]), NULL, version, &dev_attr);
        delete attr_value_list;
    }

}

void DeviceProxy::read_attribute(const string &attr_str, AttributeValue_4 *&av_4)
{
    DevVarStringArray attr_list;
    int ctr = 0;
    Tango::DevSource local_source;

    if (version < 4)
    {
        stringstream ss;
        ss << "Device " << dev_name()
           << " is too old to support this call. Please, update to IDL 4 (Tango 7.x or more)";
        Except::throw_exception(API_NotSupported, ss.str(), "DeviceProxy::read_attribute");
    }

    attr_list.length(1);
    attr_list[0] = Tango::string_dup(attr_str.c_str());

    while (ctr < 2)
    {
        try
        {
            check_and_reconnect(local_source);

            ClntIdent ci;
            ApiUtil *au = ApiUtil::instance();
            ci.cpp_clnt(au->get_client_pid());

            Device_4_var dev = Device_4::_duplicate(device_4);
            AttributeValueList_4 *attr_value_list_4 = dev->read_attributes_4(attr_list, local_source, ci);
            av_4 = attr_value_list_4->get_buffer(true);
            delete attr_value_list_4;

            ctr = 2;
        }
        READ_ATT_EXCEPT(attr_str, this)
    }

//
// Add an error in the error stack in case there is one
//

    long nb_except = av_4->err_list.length();
    if (nb_except != 0)
    {
        TangoSys_OMemStream desc;
        desc << "Failed to read_attribute on device " << device_name;
        desc << ", attribute " << attr_str << ends;

        av_4->err_list.length(nb_except + 1);
        av_4->err_list[nb_except].reason = Tango::string_dup(API_AttributeFailed);
        av_4->err_list[nb_except].origin = Tango::string_dup("DeviceProxy::read_attribute()");

        string st = desc.str();
        av_4->err_list[nb_except].desc = Tango::string_dup(st.c_str());
        av_4->err_list[nb_except].severity = Tango::ERR;
    }
}

void DeviceProxy::read_attribute(const string &attr_str, AttributeValue_5 *&av_5)
{
    DevVarStringArray attr_list;
    int ctr = 0;
    Tango::DevSource local_source;

    if (version < 5)
    {
        stringstream ss;
        ss << "Device " << dev_name()
           << " is too old to support this call. Please, update to IDL 5 (Tango 9.x or more)";
        Except::throw_exception(API_NotSupported, ss.str(), "DeviceProxy::read_attribute");
    }

    attr_list.length(1);
    attr_list[0] = Tango::string_dup(attr_str.c_str());

    while (ctr < 2)
    {
        try
        {
            check_and_reconnect(local_source);

            ClntIdent ci;
            ApiUtil *au = ApiUtil::instance();
            ci.cpp_clnt(au->get_client_pid());

            Device_5_var dev = Device_5::_duplicate(device_5);
            AttributeValueList_5 *attr_value_list_5 = dev->read_attributes_5(attr_list, local_source, ci);
            av_5 = attr_value_list_5->get_buffer(true);
            delete attr_value_list_5;

            ctr = 2;
        }
        READ_ATT_EXCEPT(attr_str, this)
    }

//
// Add an error in the error stack in case there is one
//

    long nb_except = av_5->err_list.length();
    if (nb_except != 0)
    {
        TangoSys_OMemStream desc;
        desc << "Failed to read_attribute on device " << device_name;
        desc << ", attribute " << attr_str << ends;

        av_5->err_list.length(nb_except + 1);
        av_5->err_list[nb_except].reason = Tango::string_dup(API_AttributeFailed);
        av_5->err_list[nb_except].origin = Tango::string_dup("DeviceProxy::read_attribute()");

        string st = desc.str();
        av_5->err_list[nb_except].desc = Tango::string_dup(st.c_str());
        av_5->err_list[nb_except].severity = Tango::ERR;
    }
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::write_attributes() - write a list of attributes
//
//-----------------------------------------------------------------------------

void DeviceProxy::write_attributes(vector<DeviceAttribute> &attr_list)
{
    AttributeValueList attr_value_list;
    AttributeValueList_4 attr_value_list_4;

    Tango::AccessControlType local_act;

    if (version == 0)
        check_and_reconnect(local_act);

    if (version >= 4)
        attr_value_list_4.length(attr_list.size());
    else
        attr_value_list.length(attr_list.size());

    for (unsigned int i = 0; i < attr_list.size(); i++)
    {
        if (version >= 4)
        {
            attr_value_list_4[i].name = attr_list[i].name.c_str();
            attr_value_list_4[i].quality = attr_list[i].quality;
            attr_value_list_4[i].data_format = attr_list[i].data_format;
            attr_value_list_4[i].time = attr_list[i].time;
            attr_value_list_4[i].w_dim.dim_x = attr_list[i].dim_x;
            attr_value_list_4[i].w_dim.dim_y = attr_list[i].dim_y;
        }
        else
        {
            attr_value_list[i].name = attr_list[i].name.c_str();
            attr_value_list[i].quality = attr_list[i].quality;
            attr_value_list[i].time = attr_list[i].time;
            attr_value_list[i].dim_x = attr_list[i].dim_x;
            attr_value_list[i].dim_y = attr_list[i].dim_y;
        }

        if (attr_list[i].LongSeq.operator->() != NULL)
        {
            if (version >= 4)
                attr_value_list_4[i].value.long_att_value(attr_list[i].LongSeq.in());
            else
                attr_value_list[i].value <<= attr_list[i].LongSeq.in();
            continue;
        }
        if (attr_list[i].Long64Seq.operator->() != NULL)
        {
            if (version >= 4)
                attr_value_list_4[i].value.long64_att_value(attr_list[i].Long64Seq.in());
            else
                attr_value_list[i].value <<= attr_list[i].Long64Seq.in();
            continue;
        }
        if (attr_list[i].ShortSeq.operator->() != NULL)
        {
            if (version >= 4)
                attr_value_list_4[i].value.short_att_value(attr_list[i].ShortSeq.in());
            else
                attr_value_list[i].value <<= attr_list[i].ShortSeq.in();
            continue;
        }
        if (attr_list[i].DoubleSeq.operator->() != NULL)
        {
            if (version >= 4)
                attr_value_list_4[i].value.double_att_value(attr_list[i].DoubleSeq.in());
            else
                attr_value_list[i].value <<= attr_list[i].DoubleSeq.in();
            continue;
        }
        if (attr_list[i].StringSeq.operator->() != NULL)
        {
            if (version >= 4)
                attr_value_list_4[i].value.string_att_value(attr_list[i].StringSeq.in());
            else
                attr_value_list[i].value <<= attr_list[i].StringSeq.in();
            continue;
        }
        if (attr_list[i].FloatSeq.operator->() != NULL)
        {
            if (version >= 4)
                attr_value_list_4[i].value.float_att_value(attr_list[i].FloatSeq.in());
            else
                attr_value_list[i].value <<= attr_list[i].FloatSeq.in();
            continue;
        }
        if (attr_list[i].BooleanSeq.operator->() != NULL)
        {
            if (version >= 4)
                attr_value_list_4[i].value.bool_att_value(attr_list[i].BooleanSeq.in());
            else
                attr_value_list[i].value <<= attr_list[i].BooleanSeq.in();
            continue;
        }
        if (attr_list[i].UShortSeq.operator->() != NULL)
        {
            if (version >= 4)
                attr_value_list_4[i].value.ushort_att_value(attr_list[i].UShortSeq.in());
            else
                attr_value_list[i].value <<= attr_list[i].UShortSeq.in();
            continue;
        }
        if (attr_list[i].UCharSeq.operator->() != NULL)
        {
            if (version >= 4)
                attr_value_list_4[i].value.uchar_att_value(attr_list[i].UCharSeq.in());
            else
                attr_value_list[i].value <<= attr_list[i].UCharSeq.in();
            continue;
        }
        if (attr_list[i].ULongSeq.operator->() != NULL)
        {
            if (version >= 4)
                attr_value_list_4[i].value.ulong_att_value(attr_list[i].ULongSeq.in());
            else
                attr_value_list[i].value <<= attr_list[i].ULongSeq.in();
            continue;
        }
        if (attr_list[i].ULong64Seq.operator->() != NULL)
        {
            if (version >= 4)
                attr_value_list_4[i].value.ulong64_att_value(attr_list[i].ULong64Seq.in());
            else
                attr_value_list[i].value <<= attr_list[i].ULong64Seq.in();
            continue;
        }
        if (attr_list[i].StateSeq.operator->() != NULL)
        {
            if (version >= 4)
                attr_value_list_4[i].value.state_att_value(attr_list[i].StateSeq.in());
            else
                attr_value_list[i].value <<= attr_list[i].StateSeq.in();
            continue;
        }
    }

    int ctr = 0;

    while (ctr < 2)
    {
        try
        {
            check_and_reconnect(local_act);

//
// Throw exception if caller not allowed to write_attribute
//

            if (local_act == ACCESS_READ)
            {
                try
                {
                    Device_var dev = Device::_duplicate(device);
                    dev->ping();
                }
                catch (...)
                {
                    set_connection_state(CONNECTION_NOTOK);
                    throw;
                }

                TangoSys_OMemStream desc;
                desc << "Writing attribute(s) on device " << dev_name() << " is not authorized" << ends;

                NotAllowedExcept::throw_exception((const char *) API_ReadOnlyMode, desc.str(),
                                                  (const char *) "DeviceProxy::write_attributes()");
            }

//
// Now, write the attribute(s)
//

            if (version >= 4)
            {
                ClntIdent ci;
                ApiUtil *au = ApiUtil::instance();
                ci.cpp_clnt(au->get_client_pid());

                Device_4_var dev = Device_4::_duplicate(device_4);
                dev->write_attributes_4(attr_value_list_4, ci);
            }
            else if (version == 3)
            {
                Device_3_var dev = Device_3::_duplicate(device_3);
                dev->write_attributes_3(attr_value_list);
            }
            else
            {
                Device_var dev = Device::_duplicate(device);
                dev->write_attributes(attr_value_list);
            }
            ctr = 2;
        }
        catch (Tango::MultiDevFailed &e)
        {
            throw Tango::NamedDevFailedList(e,
                                            device_name,
                                            (const char *) "DeviceProxy::write_attributes",
                                            (const char *) API_AttributeFailed);
        }
        catch (Tango::DevFailed &e)
        {
            TangoSys_OMemStream desc;
            desc << "Failed to write_attributes on device " << device_name;
            desc << ", attributes ";
            int nb_attr = attr_value_list.length();
            for (int i = 0; i < nb_attr; i++)
            {
                desc << attr_value_list[i].name.in();
                if (i != nb_attr - 1)
                    desc << ", ";
            }
            desc << ends;

            if (::strcmp(e.errors[0].reason, DEVICE_UNLOCKED_REASON) == 0)
                DeviceUnlockedExcept::re_throw_exception(e, (const char *) DEVICE_UNLOCKED_REASON,
                                                         desc.str(),
                                                         (const char *) "DeviceProxy::write_attribute()");
            else
                Except::re_throw_exception(e, (const char *) API_AttributeFailed,
                                           desc.str(), (const char *) "DeviceProxy::write_attribute()");
        }
        catch (CORBA::TRANSIENT &trans)
        {
            TRANSIENT_NOT_EXIST_EXCEPT(trans, "DeviceProxy", "write_attributes", this);
        }
        catch (CORBA::OBJECT_NOT_EXIST &one)
        {
            if (one.minor() == omni::OBJECT_NOT_EXIST_NoMatch || one.minor() == 0)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(one, "DeviceProxy", "write_attributes", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute write_attribute on device " << device_name << ends;
                ApiCommExcept::re_throw_exception(one,
                                                  (const char *) "API_CommunicationFailed",
                                                  desc.str(),
                                                  (const char *) "DeviceProxy::write_attributes()");
            }
        }
        catch (CORBA::COMM_FAILURE &comm)
        {
            if (comm.minor() == omni::COMM_FAILURE_WaitingForReply)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(comm, "DeviceProxy", "write_attributes", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute write_attribute on device " << device_name << ends;
                ApiCommExcept::re_throw_exception(comm,
                                                  (const char *) "API_CommunicationFailed",
                                                  desc.str(),
                                                  (const char *) "DeviceProxy::write_attributes()");
            }
        }
        catch (CORBA::SystemException &ce)
        {
            set_connection_state(CONNECTION_NOTOK);

            TangoSys_OMemStream desc;
            desc << "Failed to execute write_attributes on device " << device_name << ends;
            ApiCommExcept::re_throw_exception(ce,
                                              (const char *) "API_CommunicationFailed",
                                              desc.str(),
                                              (const char *) "DeviceProxy::write_attributes()");
        }
    }

    return;
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::write_attribute() - write a single attribute
//
//-----------------------------------------------------------------------------

void DeviceProxy::write_attribute(DeviceAttribute &dev_attr)
{
    AttributeValueList attr_value_list;
    AttributeValueList_4 attr_value_list_4;
    Tango::AccessControlType local_act;

    if (version == 0)
        check_and_reconnect(local_act);

    if (version >= 4)
    {
        attr_value_list_4.length(1);

        attr_value_list_4[0].name = dev_attr.name.c_str();
        attr_value_list_4[0].quality = dev_attr.quality;
        attr_value_list_4[0].data_format = dev_attr.data_format;
        attr_value_list_4[0].time = dev_attr.time;
        attr_value_list_4[0].w_dim.dim_x = dev_attr.dim_x;
        attr_value_list_4[0].w_dim.dim_y = dev_attr.dim_y;

        if (dev_attr.LongSeq.operator->() != NULL)
            attr_value_list_4[0].value.long_att_value(dev_attr.LongSeq.in());
        else if (dev_attr.Long64Seq.operator->() != NULL)
            attr_value_list_4[0].value.long64_att_value(dev_attr.Long64Seq.in());
        else if (dev_attr.ShortSeq.operator->() != NULL)
            attr_value_list_4[0].value.short_att_value(dev_attr.ShortSeq.in());
        else if (dev_attr.DoubleSeq.operator->() != NULL)
            attr_value_list_4[0].value.double_att_value(dev_attr.DoubleSeq.in());
        else if (dev_attr.StringSeq.operator->() != NULL)
            attr_value_list_4[0].value.string_att_value(dev_attr.StringSeq.in());
        else if (dev_attr.FloatSeq.operator->() != NULL)
            attr_value_list_4[0].value.float_att_value(dev_attr.FloatSeq.in());
        else if (dev_attr.BooleanSeq.operator->() != NULL)
            attr_value_list_4[0].value.bool_att_value(dev_attr.BooleanSeq.in());
        else if (dev_attr.UShortSeq.operator->() != NULL)
            attr_value_list_4[0].value.ushort_att_value(dev_attr.UShortSeq.in());
        else if (dev_attr.UCharSeq.operator->() != NULL)
            attr_value_list_4[0].value.uchar_att_value(dev_attr.UCharSeq.in());
        else if (dev_attr.ULongSeq.operator->() != NULL)
            attr_value_list_4[0].value.ulong_att_value(dev_attr.ULongSeq.in());
        else if (dev_attr.ULong64Seq.operator->() != NULL)
            attr_value_list_4[0].value.ulong64_att_value(dev_attr.ULong64Seq.in());
        else if (dev_attr.StateSeq.operator->() != NULL)
            attr_value_list_4[0].value.state_att_value(dev_attr.StateSeq.in());
        else if (dev_attr.EncodedSeq.operator->() != NULL)
            attr_value_list_4[0].value.encoded_att_value(dev_attr.EncodedSeq.in());
    }
    else
    {
        attr_value_list.length(1);

        attr_value_list[0].name = dev_attr.name.c_str();
        attr_value_list[0].quality = dev_attr.quality;
        attr_value_list[0].time = dev_attr.time;
        attr_value_list[0].dim_x = dev_attr.dim_x;
        attr_value_list[0].dim_y = dev_attr.dim_y;

        if (dev_attr.LongSeq.operator->() != NULL)
            attr_value_list[0].value <<= dev_attr.LongSeq.in();
        else if (dev_attr.Long64Seq.operator->() != NULL)
            attr_value_list[0].value <<= dev_attr.Long64Seq.in();
        else if (dev_attr.ShortSeq.operator->() != NULL)
            attr_value_list[0].value <<= dev_attr.ShortSeq.in();
        else if (dev_attr.DoubleSeq.operator->() != NULL)
            attr_value_list[0].value <<= dev_attr.DoubleSeq.in();
        else if (dev_attr.StringSeq.operator->() != NULL)
            attr_value_list[0].value <<= dev_attr.StringSeq.in();
        else if (dev_attr.FloatSeq.operator->() != NULL)
            attr_value_list[0].value <<= dev_attr.FloatSeq.in();
        else if (dev_attr.BooleanSeq.operator->() != NULL)
            attr_value_list[0].value <<= dev_attr.BooleanSeq.in();
        else if (dev_attr.UShortSeq.operator->() != NULL)
            attr_value_list[0].value <<= dev_attr.UShortSeq.in();
        else if (dev_attr.UCharSeq.operator->() != NULL)
            attr_value_list[0].value <<= dev_attr.UCharSeq.in();
        else if (dev_attr.ULongSeq.operator->() != NULL)
            attr_value_list[0].value <<= dev_attr.ULongSeq.in();
        else if (dev_attr.ULong64Seq.operator->() != NULL)
            attr_value_list[0].value <<= dev_attr.ULong64Seq.in();
        else if (dev_attr.StateSeq.operator->() != NULL)
            attr_value_list[0].value <<= dev_attr.StateSeq.in();
    }

    int ctr = 0;

    while (ctr < 2)
    {
        try
        {
            check_and_reconnect(local_act);

//
// Throw exception if caller not allowed to write_attribute
//

            if (local_act == ACCESS_READ)
            {
                try
                {
                    Device_var dev = Device::_duplicate(device);
                    dev->ping();
                }
                catch (...)
                {
                    set_connection_state(CONNECTION_NOTOK);
                    throw;
                }

                TangoSys_OMemStream desc;
                desc << "Writing attribute(s) on device " << dev_name() << " is not authorized" << ends;

                NotAllowedExcept::throw_exception((const char *) API_ReadOnlyMode, desc.str(),
                                                  (const char *) "DeviceProxy::write_attribute()");
            }

//
// Now, write the attribute(s)
//

            if (version >= 4)
            {
                ClntIdent ci;
                ApiUtil *au = ApiUtil::instance();
                ci.cpp_clnt(au->get_client_pid());

                Device_4_var dev = Device_4::_duplicate(device_4);
                dev->write_attributes_4(attr_value_list_4, ci);
            }
            else if (version == 3)
            {
                Device_3_var dev = Device_3::_duplicate(device_3);
                dev->write_attributes_3(attr_value_list);
            }
            else
            {
                Device_var dev = Device::_duplicate(device);
                dev->write_attributes(attr_value_list);
            }
            ctr = 2;

        }
        catch (Tango::MultiDevFailed &e)
        {

//
// Transfer this exception into a DevFailed exception
//

            Tango::DevFailed ex(e.errors[0].err_list);
            TangoSys_OMemStream desc;
            desc << "Failed to write_attribute on device " << device_name;
            desc << ", attribute ";
            desc << dev_attr.name;
            desc << ends;
            Except::re_throw_exception(ex, (const char *) API_AttributeFailed,
                                       desc.str(), (const char *) "DeviceProxy::write_attribute()");

        }
        catch (Tango::DevFailed &e)
        {
            TangoSys_OMemStream desc;
            desc << "Failed to write_attribute on device " << device_name;
            desc << ", attribute ";
            desc << dev_attr.name;
            desc << ends;

            if (::strcmp(e.errors[0].reason, DEVICE_UNLOCKED_REASON) == 0)
                DeviceUnlockedExcept::re_throw_exception(e, (const char *) DEVICE_UNLOCKED_REASON,
                                                         desc.str(),
                                                         (const char *) "DeviceProxy::write_attribute()");
            else
                Except::re_throw_exception(e, (const char *) API_AttributeFailed,
                                           desc.str(), (const char *) "DeviceProxy::write_attribute()");
        }
        catch (CORBA::TRANSIENT &trans)
        {
            TRANSIENT_NOT_EXIST_EXCEPT(trans, "DeviceProxy", "write_attribute()", this);
        }
        catch (CORBA::OBJECT_NOT_EXIST &one)
        {
            if (one.minor() == omni::OBJECT_NOT_EXIST_NoMatch || one.minor() == 0)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(one, "DeviceProxy", "write_attribute", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute write_attribute on device " << device_name << ends;
                ApiCommExcept::re_throw_exception(one,
                                                  (const char *) "API_CommunicationFailed",
                                                  desc.str(),
                                                  (const char *) "DeviceProxy::write_attribute()");
            }
        }
        catch (CORBA::COMM_FAILURE &comm)
        {
            if (comm.minor() == omni::COMM_FAILURE_WaitingForReply)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(comm, "DeviceProxy", "write_attribute", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute write_attribute on device " << device_name << ends;
                ApiCommExcept::re_throw_exception(comm,
                                                  (const char *) "API_CommunicationFailed",
                                                  desc.str(),
                                                  (const char *) "DeviceProxy::write_attribute()");
            }
        }
        catch (CORBA::SystemException &ce)
        {
            set_connection_state(CONNECTION_NOTOK);

            TangoSys_OMemStream desc;
            desc << "Failed to execute write_attributes on device " << device_name << ends;
            ApiCommExcept::re_throw_exception(ce,
                                              (const char *) "API_CommunicationFailed",
                                              desc.str(),
                                              (const char *) "DeviceProxy::write_attribute()");
        }
    }

    return;
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::write_attribute() - write attribute(s) using the CORBA data type
//
//-----------------------------------------------------------------------------

void DeviceProxy::write_attribute(const AttributeValueList &attr_val)
{

    int ctr = 0;
    Tango::AccessControlType local_act;

    while (ctr < 2)
    {
        try
        {
            check_and_reconnect(local_act);

//
// Throw exception if caller not allowed to write_attribute
//

            if (local_act == ACCESS_READ)
            {
                try
                {
                    Device_var dev = Device::_duplicate(device);
                    dev->ping();
                }
                catch (...)
                {
                    set_connection_state(CONNECTION_NOTOK);
                    throw;
                }

                TangoSys_OMemStream desc;
                desc << "Writing attribute(s) on device " << dev_name() << " is not authorized" << ends;

                NotAllowedExcept::throw_exception((const char *) API_ReadOnlyMode, desc.str(),
                                                  (const char *) "DeviceProxy::write_attribute()");
            }

//
// Now, write the attribute(s)
//


            if (version >= 3)
            {
                Device_3_var dev = Device_3::_duplicate(device_3);
                dev->write_attributes_3(attr_val);
            }
            else
            {
                Device_var dev = Device::_duplicate(device);
                dev->write_attributes(attr_val);
            }
            ctr = 2;

        }
        catch (Tango::MultiDevFailed &e)
        {

//
// Transfer this exception into a DevFailed exception
//

            Tango::DevFailed ex(e.errors[0].err_list);
            TangoSys_OMemStream desc;
            desc << "Failed to write_attribute on device " << device_name;
            desc << ", attribute ";
            desc << attr_val[0].name.in();
            desc << ends;
            Except::re_throw_exception(ex, (const char *) API_AttributeFailed,
                                       desc.str(), (const char *) "DeviceProxy::write_attribute()");

        }
        catch (Tango::DevFailed &e)
        {
            TangoSys_OMemStream desc;
            desc << "Failed to write_attribute on device " << device_name;
            desc << ", attribute ";
            desc << attr_val[0].name.in();
            desc << ends;

            if (::strcmp(e.errors[0].reason, DEVICE_UNLOCKED_REASON) == 0)
                DeviceUnlockedExcept::re_throw_exception(e, (const char *) DEVICE_UNLOCKED_REASON,
                                                         desc.str(),
                                                         (const char *) "DeviceProxy::write_attribute()");
            else
                Except::re_throw_exception(e, (const char *) API_AttributeFailed,
                                           desc.str(), (const char *) "DeviceProxy::write_attribute()");
        }
        catch (CORBA::TRANSIENT &trans)
        {
            TRANSIENT_NOT_EXIST_EXCEPT(trans, "DeviceProxy", "write_attribute()", this);
        }
        catch (CORBA::OBJECT_NOT_EXIST &one)
        {
            if (one.minor() == omni::OBJECT_NOT_EXIST_NoMatch || one.minor() == 0)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(one, "DeviceProxy", "write_attribute", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute write_attribute on device " << device_name << ends;
                ApiCommExcept::re_throw_exception(one,
                                                  (const char *) "API_CommunicationFailed",
                                                  desc.str(),
                                                  (const char *) "DeviceProxy::write_attribute()");
            }
        }
        catch (CORBA::COMM_FAILURE &comm)
        {
            if (comm.minor() == omni::COMM_FAILURE_WaitingForReply)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(comm, "DeviceProxy", "write_attribute", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute write_attribute on device " << device_name << ends;
                ApiCommExcept::re_throw_exception(comm,
                                                  (const char *) "API_CommunicationFailed",
                                                  desc.str(),
                                                  (const char *) "DeviceProxy::write_attribute()");
            }
        }
        catch (CORBA::SystemException &ce)
        {
            set_connection_state(CONNECTION_NOTOK);

            TangoSys_OMemStream desc;
            desc << "Failed to execute write_attributes on device " << device_name << ends;
            ApiCommExcept::re_throw_exception(ce,
                                              (const char *) "API_CommunicationFailed",
                                              desc.str(),
                                              (const char *) "DeviceProxy::write_attribute()");
        }
    }

    return;
}

void DeviceProxy::write_attribute(const AttributeValueList_4 &attr_val)
{

    Tango::AccessControlType local_act;

    if (version == 0)
        check_and_reconnect(local_act);

//
// Check that the device supports IDL V4
//

    if (version < 4)
    {
        TangoSys_OMemStream desc;
        desc << "Failed to write_attribute on device " << device_name;
        desc << ", attribute ";
        desc << attr_val[0].name.in();
        desc << ". The device does not support thi stype of data (Bad IDL release)";
        desc << ends;
        Tango::Except::throw_exception((const char *) API_NotSupportedFeature,
                                       desc.str(), (const char *) "DeviceProxy::write_attribute()");
    }

    int ctr = 0;

    while (ctr < 2)
    {
        try
        {
            check_and_reconnect(local_act);

//
// Throw exception if caller not allowed to write_attribute
//

            if (local_act == ACCESS_READ)
            {
                try
                {
                    Device_var dev = Device::_duplicate(device);
                    dev->ping();
                }
                catch (...)
                {
                    set_connection_state(CONNECTION_NOTOK);
                    throw;
                }

                TangoSys_OMemStream desc;
                desc << "Writing attribute(s) on device " << dev_name() << " is not authorized" << ends;

                NotAllowedExcept::throw_exception((const char *) API_ReadOnlyMode, desc.str(),
                                                  (const char *) "DeviceProxy::write_attribute()");
            }

//
// Now, write the attribute(s)
//

            ClntIdent ci;
            ApiUtil *au = ApiUtil::instance();
            ci.cpp_clnt(au->get_client_pid());

            Device_4_var dev = Device_4::_duplicate(device_4);
            dev->write_attributes_4(attr_val, ci);
            ctr = 2;

        }
        catch (Tango::MultiDevFailed &e)
        {

//
// Transfer this exception into a DevFailed exception
//

            Tango::DevFailed ex(e.errors[0].err_list);
            TangoSys_OMemStream desc;
            desc << "Failed to write_attribute on device " << device_name;
            desc << ", attribute ";
            desc << attr_val[0].name.in();
            desc << ends;
            Except::re_throw_exception(ex, (const char *) API_AttributeFailed,
                                       desc.str(), (const char *) "DeviceProxy::write_attribute()");

        }
        catch (Tango::DevFailed &e)
        {
            TangoSys_OMemStream desc;
            desc << "Failed to write_attribute on device " << device_name;
            desc << ", attribute ";
            desc << attr_val[0].name.in();
            desc << ends;

            if (::strcmp(e.errors[0].reason, DEVICE_UNLOCKED_REASON) == 0)
                DeviceUnlockedExcept::re_throw_exception(e, (const char *) DEVICE_UNLOCKED_REASON,
                                                         desc.str(),
                                                         (const char *) "DeviceProxy::write_attribute()");
            else
                Except::re_throw_exception(e, (const char *) API_AttributeFailed,
                                           desc.str(), (const char *) "DeviceProxy::write_attribute()");
        }
        catch (CORBA::TRANSIENT &trans)
        {
            TRANSIENT_NOT_EXIST_EXCEPT(trans, "DeviceProxy", "write_attribute()", this);
        }
        catch (CORBA::OBJECT_NOT_EXIST &one)
        {
            if (one.minor() == omni::OBJECT_NOT_EXIST_NoMatch || one.minor() == 0)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(one, "DeviceProxy", "write_attribute", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute write_attribute on device " << device_name << ends;
                ApiCommExcept::re_throw_exception(one,
                                                  (const char *) "API_CommunicationFailed",
                                                  desc.str(),
                                                  (const char *) "DeviceProxy::write_attribute()");
            }
        }
        catch (CORBA::COMM_FAILURE &comm)
        {
            if (comm.minor() == omni::COMM_FAILURE_WaitingForReply)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(comm, "DeviceProxy", "write_attribute", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute write_attribute on device " << device_name << ends;
                ApiCommExcept::re_throw_exception(comm,
                                                  (const char *) "API_CommunicationFailed",
                                                  desc.str(),
                                                  (const char *) "DeviceProxy::write_attribute()");
            }
        }
        catch (CORBA::SystemException &ce)
        {
            set_connection_state(CONNECTION_NOTOK);

            TangoSys_OMemStream desc;
            desc << "Failed to execute write_attributes on device " << device_name << ends;
            ApiCommExcept::re_throw_exception(ce,
                                              (const char *) "API_CommunicationFailed",
                                              desc.str(),
                                              (const char *) "DeviceProxy::write_attribute()");
        }
    }

    return;
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::get_attribute_list() - get list of attributes
//
//-----------------------------------------------------------------------------

vector<string> *DeviceProxy::get_attribute_list()
{
    vector<string> all_attr;
    AttributeInfoListEx *all_attr_config;

    all_attr.push_back(AllAttr_3);
    all_attr_config = get_attribute_config_ex(all_attr);

    vector<string> *attr_list = new vector<string>;
    attr_list->resize(all_attr_config->size());
    for (unsigned int i = 0; i < all_attr_config->size(); i++)
    {
        (*attr_list)[i] = (*all_attr_config)[i].name;
    }
    delete all_attr_config;

    return attr_list;
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::attribute_list_query() - get list of attributes
//
//-----------------------------------------------------------------------------

AttributeInfoList *DeviceProxy::attribute_list_query()
{
    vector<string> all_attr;
    AttributeInfoList *all_attr_config;

    all_attr.push_back(AllAttr_3);
    all_attr_config = get_attribute_config(all_attr);

    return all_attr_config;
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::attribute_list_query_ex() - get list of attributes
//
//-----------------------------------------------------------------------------

AttributeInfoListEx *DeviceProxy::attribute_list_query_ex()
{
    vector<string> all_attr;
    AttributeInfoListEx *all_attr_config;

    all_attr.push_back(AllAttr_3);
    all_attr_config = get_attribute_config_ex(all_attr);

    return all_attr_config;
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::command_history() - get command history (only for polled command)
//
//-----------------------------------------------------------------------------

vector<DeviceDataHistory> *DeviceProxy::command_history(string &cmd_name, int depth)
{
    if (version == 1)
    {
        TangoSys_OMemStream desc;
        desc << "Device " << device_name;
        desc << " does not support command_history feature" << ends;
        ApiNonSuppExcept::throw_exception((const char *) API_UnsupportedFeature,
                                          desc.str(),
                                          (const char *) "DeviceProxy::command_history");
    }

    DevCmdHistoryList *hist = NULL;
    DevCmdHistory_4_var hist_4 = NULL;

    int ctr = 0;

    while (ctr < 2)
    {
        try
        {
            check_and_reconnect();

            if (version <= 3)
            {
                Device_2_var dev = Device_2::_duplicate(device_2);
                hist = dev->command_inout_history_2(cmd_name.c_str(), depth);
            }
            else
            {
                Device_4_var dev = Device_4::_duplicate(device_4);
                hist_4 = dev->command_inout_history_4(cmd_name.c_str(), depth);
            }
            ctr = 2;
        }
        catch (CORBA::TRANSIENT &trans)
        {
            TRANSIENT_NOT_EXIST_EXCEPT(trans, "DeviceProxy", "command_history", this);
        }
        catch (CORBA::OBJECT_NOT_EXIST &one)
        {
            if (one.minor() == omni::OBJECT_NOT_EXIST_NoMatch || one.minor() == 0)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(one, "DeviceProxy", "command_history", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Command_history failed on device " << device_name << ends;
                ApiCommExcept::re_throw_exception(one,
                                                  (const char *) "API_CommunicationFailed",
                                                  desc.str(),
                                                  (const char *) "DeviceProxy::command_history()");
            }
        }
        catch (CORBA::COMM_FAILURE &comm)
        {
            if (comm.minor() == omni::COMM_FAILURE_WaitingForReply)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(comm, "DeviceProxy", "command_history", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Command_history failed on device " << device_name << ends;
                ApiCommExcept::re_throw_exception(comm,
                                                  (const char *) "API_CommunicationFailed",
                                                  desc.str(),
                                                  (const char *) "DeviceProxy::command_history()");
            }
        }
        catch (CORBA::SystemException &ce)
        {
            set_connection_state(CONNECTION_NOTOK);

            TangoSys_OMemStream desc;
            desc << "Command_history failed on device " << device_name << ends;
            ApiCommExcept::re_throw_exception(ce,
                                              (const char *) "API_CommunicationFailed",
                                              desc.str(),
                                              (const char *) "DeviceProxy::command_history()");
        }
    }


    vector<DeviceDataHistory> *ddh = new vector<DeviceDataHistory>;

    if (version <= 3)
    {
        int *ctr_ptr = new int;
        *ctr_ptr = 0;

        ddh->reserve(hist->length());

        for (unsigned int i = 0; i < hist->length(); i++)
        {
            ddh->push_back(DeviceDataHistory(i, ctr_ptr, hist));
        }
    }
    else
    {
        ddh->reserve(hist_4->dates.length());
        for (unsigned int i = 0; i < hist_4->dates.length(); i++)
            ddh->push_back(DeviceDataHistory());
        from_hist4_2_DataHistory(hist_4, ddh);
    }

    return ddh;

}

//-----------------------------------------------------------------------------
//
// DeviceProxy::attribute_history() - get attribute history
//				      (only for polled attribute)
//
//-----------------------------------------------------------------------------

vector<DeviceAttributeHistory> *DeviceProxy::attribute_history(string &cmd_name, int depth)
{
    if (version == 1)
    {
        TangoSys_OMemStream desc;
        desc << "Device " << device_name;
        desc << " does not support attribute_history feature" << ends;
        ApiNonSuppExcept::throw_exception((const char *) API_UnsupportedFeature,
                                          desc.str(),
                                          (const char *) "DeviceProxy::attribute_history");
    }

    DevAttrHistoryList_var hist;
    DevAttrHistoryList_3_var hist_3;
    DevAttrHistory_4_var hist_4;
    DevAttrHistory_5_var hist_5;

    int ctr = 0;

    while (ctr < 2)
    {
        try
        {
            check_and_reconnect();

            if (version == 2)
            {
                Device_2_var dev = Device_2::_duplicate(device_2);
                hist = device_2->read_attribute_history_2(cmd_name.c_str(), depth);
            }
            else
            {
                if (version == 3)
                {
                    Device_3_var dev = Device_3::_duplicate(device_3);
                    hist_3 = dev->read_attribute_history_3(cmd_name.c_str(), depth);
                }
                else if (version == 4)
                {
                    Device_4_var dev = Device_4::_duplicate(device_4);
                    hist_4 = dev->read_attribute_history_4(cmd_name.c_str(), depth);
                }
                else
                {
                    Device_5_var dev = Device_5::_duplicate(device_5);
                    hist_5 = dev->read_attribute_history_5(cmd_name.c_str(), depth);
                }
            }
            ctr = 2;
        }
        catch (CORBA::TRANSIENT &trans)
        {
            TRANSIENT_NOT_EXIST_EXCEPT(trans, "DeviceProxy", "attribute_history", this);
        }
        catch (CORBA::OBJECT_NOT_EXIST &one)
        {
            if (one.minor() == omni::OBJECT_NOT_EXIST_NoMatch || one.minor() == 0)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(one, "DeviceProxy", "attribute_history", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Attribute_history failed on device " << device_name << ends;
                ApiCommExcept::re_throw_exception(one,
                                                  (const char *) "API_CommunicationFailed",
                                                  desc.str(),
                                                  (const char *) "DeviceProxy::attribute_history()");
            }
        }
        catch (CORBA::COMM_FAILURE &comm)
        {
            if (comm.minor() == omni::COMM_FAILURE_WaitingForReply)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(comm, "DeviceProxy", "attribute_history", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Attribute_history failed on device " << device_name << ends;
                ApiCommExcept::re_throw_exception(comm,
                                                  (const char *) "API_CommunicationFailed",
                                                  desc.str(),
                                                  (const char *) "DeviceProxy::attribute_history()");
            }
        }
        catch (CORBA::SystemException &ce)
        {
            set_connection_state(CONNECTION_NOTOK);
            TangoSys_OMemStream desc;
            desc << "Attribute_history failed on device " << device_name << ends;
            ApiCommExcept::re_throw_exception(ce,
                                              (const char *) "API_CommunicationFailed",
                                              desc.str(),
                                              (const char *) "DeviceProxy::attribute_history()");
        }
    }

    vector<DeviceAttributeHistory> *ddh = new vector<DeviceAttributeHistory>;

    if (version > 4)
    {
        ddh->reserve(hist_5->dates.length());
        for (unsigned int i = 0; i < hist_5->dates.length(); i++)
            ddh->push_back(DeviceAttributeHistory());
        from_hist_2_AttHistory(hist_5, ddh);
        for (unsigned int i = 0; i < hist_5->dates.length(); i++)
            (*ddh)[i].data_type = hist_5->data_type;
    }
    else if (version == 4)
    {
        ddh->reserve(hist_4->dates.length());
        for (unsigned int i = 0; i < hist_4->dates.length(); i++)
            ddh->push_back(DeviceAttributeHistory());
        from_hist_2_AttHistory(hist_4, ddh);
    }
    else if (version == 3)
    {
        ddh->reserve(hist_3->length());
        for (unsigned int i = 0; i < hist_3->length(); i++)
        {
            ddh->push_back(DeviceAttributeHistory(i, hist_3));
        }
    }
    else
    {
        ddh->reserve(hist->length());
        for (unsigned int i = 0; i < hist->length(); i++)
        {
            ddh->push_back(DeviceAttributeHistory(i, hist));
        }
    }

    return ddh;
}

//---------------------------------------------------------------------------------------------------------------------
//
// method:
//		DeviceProxy::connect_to_adm_device()
//
// description:
//		Create a connection to the admin device of the Tango device server process where the device is running.
//
//--------------------------------------------------------------------------------------------------------------------

void DeviceProxy::connect_to_adm_device()
{
    adm_dev_name = adm_name();

    adm_device = new DeviceProxy(adm_dev_name);
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::polling_status() - get device polling status
//
//-----------------------------------------------------------------------------

vector<string> *DeviceProxy::polling_status()
{
    check_connect_adm_device();

    DeviceData dout, din;
    string cmd("DevPollStatus");
    din.any <<= device_name.c_str();

//
// In case of connection failed error, do a re-try
//

    try
    {
        dout = adm_device->command_inout(cmd, din);
    }
    catch (Tango::CommunicationFailed &)
    {
        dout = adm_device->command_inout(cmd, din);
    }

    const DevVarStringArray *out_str;
    dout >> out_str;

    vector<string> *poll_stat = new vector<string>;
    poll_stat->reserve(out_str->length());

    for (unsigned int i = 0; i < out_str->length(); i++)
    {
        string str = (*out_str)[i].in();
        poll_stat->push_back(str);
    }
    return poll_stat;
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::is_polled() - return true if the object "obj_name" is polled.
//			      In this case, the upd string is initialised with
//			      the polling period.
//
//-----------------------------------------------------------------------------

bool DeviceProxy::is_polled(polled_object obj, string &obj_name, string &upd)
{
    bool ret = false;
    vector<string> *poll_str;

    poll_str = polling_status();
    if (poll_str->empty() == true)
    {
        delete poll_str;
        return ret;
    }

    string loc_obj_name(obj_name);
    transform(loc_obj_name.begin(), loc_obj_name.end(), loc_obj_name.begin(), ::tolower);

    for (unsigned int i = 0; i < poll_str->size(); i++)
    {
        string &tmp_str = (*poll_str)[i];
        string::size_type pos, end;
        pos = tmp_str.find(' ');
        pos++;
        end = tmp_str.find(' ', pos + 1);
        string obj_type = tmp_str.substr(pos, end - pos);
        if (obj_type == "command")
        {
            if (obj == Attr)
                continue;
        }
        else if (obj_type == "attribute")
        {
            if (obj == Cmd)
            {
                if ((loc_obj_name != "state") && (loc_obj_name != "status"))
                    continue;
            }
        }

        pos = tmp_str.find('=');
        pos = pos + 2;
        end = tmp_str.find(". S", pos + 1);
        if (end == string::npos)
            end = tmp_str.find('\n', pos + 1);
        string name = tmp_str.substr(pos, end - pos);
        transform(name.begin(), name.end(), name.begin(), ::tolower);

        if (name == loc_obj_name)
        {

//
// Now that it's found, search for its polling period
//

            pos = tmp_str.find("triggered", end);
            if (pos != string::npos)
            {
                ret = true;
                upd = "0";
                break;
            }
            else
            {
                pos = tmp_str.find('=', end);
                pos = pos + 2;
                end = tmp_str.find('\n', pos + 1);
                string per = tmp_str.substr(pos, end - pos);
                upd = per;
                ret = true;
                break;
            }
        }
    }

    delete poll_str;

    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::get_command_poll_period() - Return command polling period
//					    (in mS)
//
//-----------------------------------------------------------------------------

int DeviceProxy::get_command_poll_period(string &cmd_name)
{
    string poll_per;
    bool poll = is_polled(Cmd, cmd_name, poll_per);

    int ret;
    if (poll == true)
    {
        TangoSys_MemStream stream;

        stream << poll_per << ends;
        stream >> ret;
    }
    else
        ret = 0;

    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::get_attribute_poll_period() - Return attribute polling period
//					    (in mS)
//
//-----------------------------------------------------------------------------

int DeviceProxy::get_attribute_poll_period(string &attr_name)
{
    string poll_per;
    bool poll = is_polled(Attr, attr_name, poll_per);

    int ret;
    if (poll == true)
    {
        TangoSys_MemStream stream;

        stream << poll_per << ends;
        stream >> ret;
    }
    else
        ret = 0;

    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::poll_command() - If object is already polled, just update its
//				 polling period. If object is not polled, add
//				 it to the list of polled objects
//
//-----------------------------------------------------------------------------

void DeviceProxy::poll_command(string &cmd_name, int period)
{
    string poll_per;
    bool poll = is_polled(Cmd, cmd_name, poll_per);

    DevVarLongStringArray in;
    in.lvalue.length(1);
    in.svalue.length(3);

    in.svalue[0] = Tango::string_dup(device_name.c_str());
    in.svalue[1] = Tango::string_dup("command");
    in.svalue[2] = Tango::string_dup(cmd_name.c_str());
    in.lvalue[0] = period;

    if (poll == true)
    {

//
// If object is polled and the polling period is the same, simply retruns
//
        TangoSys_MemStream stream;
        int per;

        stream << poll_per << ends;
        stream >> per;

        if ((per == period) || (per == 0))
            return;
        else
        {

//
// If object is polled, this is an update of the polling period
//

            DeviceData din;
            string cmd("UpdObjPollingPeriod");
            din.any <<= in;

            try
            {
                adm_device->command_inout(cmd, din);
            }
            catch (Tango::CommunicationFailed &)
            {
                adm_device->command_inout(cmd, din);
            }

        }
    }
    else
    {

//
// This a AddObjPolling command
//

        DeviceData din;
        string cmd("AddObjPolling");
        din.any <<= in;

        try
        {
            adm_device->command_inout(cmd, din);
        }
        catch (Tango::CommunicationFailed &)
        {
            adm_device->command_inout(cmd, din);
        }

    }

}

//-----------------------------------------------------------------------------
//
// DeviceProxy::poll_attribute() - If object is already polled, just update its
//				 polling period. If object is not polled, add
//				 it to the list of polled objects
//
//-----------------------------------------------------------------------------

void DeviceProxy::poll_attribute(string &attr_name, int period)
{
    string poll_per;
    bool poll = is_polled(Attr, attr_name, poll_per);

    DevVarLongStringArray in;
    in.lvalue.length(1);
    in.svalue.length(3);

    in.svalue[0] = Tango::string_dup(device_name.c_str());
    in.svalue[1] = Tango::string_dup("attribute");
    in.svalue[2] = Tango::string_dup(attr_name.c_str());
    in.lvalue[0] = period;

    if (poll == true)
    {

//
// If object is polled and the polling period is the same, simply returns
//

        TangoSys_MemStream stream;
        int per;

        stream << poll_per << ends;
        stream >> per;

        if ((per == period) || (per == 0))
            return;
        else
        {

//
// If object is polled, this is an update of the polling period
//

            DeviceData din;
            string cmd("UpdObjPollingPeriod");
            din.any <<= in;

            try
            {
                adm_device->command_inout(cmd, din);
            }
            catch (Tango::CommunicationFailed &)
            {
                adm_device->command_inout(cmd, din);
            }

        }
    }
    else
    {

//
// This a AddObjPolling command
//

        DeviceData din;
        string cmd("AddObjPolling");
        din.any <<= in;

        try
        {
            adm_device->command_inout(cmd, din);
        }
        catch (Tango::CommunicationFailed &)
        {
            adm_device->command_inout(cmd, din);
        }

    }

}

//-----------------------------------------------------------------------------
//
// DeviceProxy::is_cmd_polled() - return true if the command is polled
//
//-----------------------------------------------------------------------------

bool DeviceProxy::is_command_polled(string &cmd_name)
{
    string upd;
    return is_polled(Cmd, cmd_name, upd);
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::is_attribute_polled() - return true if the attribute is polled
//
//-----------------------------------------------------------------------------

bool DeviceProxy::is_attribute_polled(string &attr_name)
{
    string upd;
    return is_polled(Attr, attr_name, upd);
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::stop_poll_command() - Stop polling a command
//
//-----------------------------------------------------------------------------

void DeviceProxy::stop_poll_command(string &cmd_name)
{
    check_connect_adm_device();

    DevVarStringArray in;
    in.length(3);

    in[0] = Tango::string_dup(device_name.c_str());
    in[1] = Tango::string_dup("command");
    in[2] = Tango::string_dup(cmd_name.c_str());

    DeviceData din;
    string cmd("RemObjPolling");
    din.any <<= in;

    try
    {
        adm_device->command_inout(cmd, din);
    }
    catch (Tango::CommunicationFailed &)
    {
        adm_device->command_inout(cmd, din);
    }

}

//-----------------------------------------------------------------------------
//
// DeviceProxy::stop_poll_attribute() - Stop polling attribute
//
//-----------------------------------------------------------------------------

void DeviceProxy::stop_poll_attribute(string &attr_name)
{
    check_connect_adm_device();

    DevVarStringArray in;
    in.length(3);

    in[0] = Tango::string_dup(device_name.c_str());
    in[1] = Tango::string_dup("attribute");
    in[2] = Tango::string_dup(attr_name.c_str());

    DeviceData din;
    string cmd("RemObjPolling");
    din.any <<= in;

    try
    {
        adm_device->command_inout(cmd, din);
    }
    catch (Tango::CommunicationFailed &)
    {
        adm_device->command_inout(cmd, din);
    }

}

#ifdef TANGO_HAS_LOG4TANGO

//-----------------------------------------------------------------------------
//
// DeviceProxy::add_logging_target - Add a logging target
//
//-----------------------------------------------------------------------------
void DeviceProxy::add_logging_target(const string &target_type_name)
{
    check_connect_adm_device();

    DevVarStringArray in(2);
    in.length(2);

    in[0] = Tango::string_dup(device_name.c_str());
    in[1] = Tango::string_dup(target_type_name.c_str());

    DeviceData din;
    string cmd("AddLoggingTarget");
    din.any <<= in;

    try
    {
        adm_device->command_inout(cmd, din);
    }
    catch (Tango::CommunicationFailed &)
    {
        adm_device->command_inout(cmd, din);
    }
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::remove_logging_target - Remove a logging target
//
//-----------------------------------------------------------------------------
void DeviceProxy::remove_logging_target(const string &target_type_name)
{
    check_connect_adm_device();

    DevVarStringArray in(2);
    in.length(2);

    in[0] = Tango::string_dup(device_name.c_str());
    in[1] = Tango::string_dup(target_type_name.c_str());

    DeviceData din;
    string cmd("RemoveLoggingTarget");
    din.any <<= in;

    try
    {
        adm_device->command_inout(cmd, din);
    }
    catch (Tango::CommunicationFailed &)
    {
        adm_device->command_inout(cmd, din);
    }
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::get_logging_target - Returns the logging target list
//
//-----------------------------------------------------------------------------
vector<string> DeviceProxy::get_logging_target(void)
{
    check_connect_adm_device();

    DeviceData din;
    din << device_name;

    string cmd("GetLoggingTarget");

    DeviceData dout;
    DevVarStringArray_var logging_targets;
    try
    {
        dout = adm_device->command_inout(cmd, din);
    }
    catch (Tango::CommunicationFailed &)
    {
        dout = adm_device->command_inout(cmd, din);
    }

    vector<string> logging_targets_vec;

    dout >> logging_targets_vec;

    return logging_targets_vec;
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::get_logging_level - Returns the current logging level
//
//-----------------------------------------------------------------------------

int DeviceProxy::get_logging_level(void)
{
    check_connect_adm_device();

    string cmd("GetLoggingLevel");

    DevVarStringArray in;
    in.length(1);
    in[0] = Tango::string_dup(device_name.c_str());

    DeviceData din;
    din.any <<= in;

    DeviceData dout;
    try
    {
        dout = adm_device->command_inout(cmd, din);
    }
    catch (Tango::CommunicationFailed &)
    {
        dout = adm_device->command_inout(cmd, din);
    }

    long level;
    if ((dout >> level) == false)
    {
        const Tango::DevVarLongStringArray *lsarr;
        dout >> lsarr;

        string devnm = dev_name();
        std::transform(devnm.begin(), devnm.end(), devnm.begin(), ::tolower);

        for (unsigned int i = 0; i < lsarr->svalue.length(); i++)
        {
            string nm(lsarr->svalue[i]);
            std::transform(nm.begin(), nm.end(), nm.begin(), ::tolower);

            if (devnm == nm)
            {
                level = lsarr->lvalue[i];
                break;
            }
        }
    }

    return (int) level;
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::set_logging_level - Set the logging level
//
//-----------------------------------------------------------------------------

void DeviceProxy::set_logging_level(int level)
{
    check_connect_adm_device();

    string cmd("SetLoggingLevel");

    DevVarLongStringArray in;
    in.lvalue.length(1);
    in.lvalue[0] = level;
    in.svalue.length(1);
    in.svalue[0] = Tango::string_dup(device_name.c_str());

    DeviceData din;
    din.any <<= in;

    try
    {
        adm_device->command_inout(cmd, din);
    }
    catch (Tango::CommunicationFailed &)
    {
        adm_device->command_inout(cmd, din);
    }
}

#endif // TANGO_HAS_LOG4TANGO



//--------------------------------------------------------------------------------------------------------------------
//
// method :
// 		DeviceProxy::subscribe_event
//
// description :
//		Subscribe to an event - Old interface for compatibility
//
//-------------------------------------------------------------------------------------------------------------------

int DeviceProxy::subscribe_event(const string &attr_name, EventType event,
                                 CallBack *callback, const vector<string> &filters)
{
    return subscribe_event(attr_name, event, callback, filters, false);
}

//-------------------------------------------------------------------------------------------------------------------
//
// method :
// 		DeviceProxy::subscribe_event
//
// description :
//		Subscribe to an event- Adds the statless flag for stateless event subscription.
//
//-------------------------------------------------------------------------------------------------------------------

int DeviceProxy::subscribe_event(const string &attr_name, EventType event,
                                 CallBack *callback, const vector<string> &filters,
                                 bool stateless)
{
    //TODO inject
    ApiUtil *api_ptr = ApiUtil::instance();
    if (api_ptr->get_zmq_event_consumer() == NULL)
    {
        api_ptr->create_zmq_event_consumer();
    }

    return api_ptr->get_zmq_event_consumer()->subscribe_event(this, attr_name, event, callback, filters,
                                                              stateless);
}

//------------------------------------------------------------------------------------------------------------------
//
// method :
// 		DeviceProxy::subscribe_event
//
// description :
//		Subscribe to an event with the usage of the event queue for data reception. Adds the statless flag for
//		stateless event subscription.
//
//-----------------------------------------------------------------------------------------------------------------

int DeviceProxy::subscribe_event(const string &attr_name, EventType event,
                                 int event_queue_size, const vector<string> &filters,
                                 bool stateless)
{
    //TODO inject
    ApiUtil *api_ptr = ApiUtil::instance();
    if (api_ptr->get_zmq_event_consumer() == NULL)
    {
        api_ptr->create_zmq_event_consumer();
    }

    return api_ptr->get_zmq_event_consumer()->subscribe_event(this, attr_name, event, event_queue_size, filters,
                                                              stateless);
}

//-------------------------------------------------------------------------------------------------------------------
//
// method :
// 		DeviceProxy::subscribe_event
//
// description :
//		Subscribe to a device event- Add the statless flag for stateless event subscription.
//
//-------------------------------------------------------------------------------------------------------------------

int DeviceProxy::subscribe_event(EventType event, CallBack *callback, bool stateless)
{
    if (version < MIN_IDL_DEV_INTR)
    {
        stringstream ss;
        ss << "Device " << dev_name() << " does not support device interface change event\n";
        ss << "Available since Tango release 9 AND for device inheriting from IDL release 5 (Device_5Impl)";

        Tango::Except::throw_exception(API_NotSupportedFeature, ss.str(), "DeviceProxy::subscribe_event()");
    }

    ApiUtil *api_ptr = ApiUtil::instance();
    if (api_ptr->get_zmq_event_consumer() == NULL)
    {
        api_ptr->create_zmq_event_consumer();
    }

    int ret;
    ret = api_ptr->get_zmq_event_consumer()->subscribe_event(this, event, callback, stateless);

    return ret;
}

//------------------------------------------------------------------------------------------------------------------
//
// method :
// 		DeviceProxy::subscribe_event
//
// description :
//		Subscribe to an event with the usage of the event queue for data reception. Adds the statless flag for
//		stateless event subscription.
//
//-----------------------------------------------------------------------------------------------------------------

int DeviceProxy::subscribe_event(EventType event, int event_queue_size, bool stateless)
{
    if (version < MIN_IDL_DEV_INTR)
    {
        stringstream ss;
        ss << "Device " << dev_name() << " does not support device interface change event\n";
        ss << "Available since Tango release 9 AND for device inheriting from IDL release 5 (Device_5Impl)";

        Tango::Except::throw_exception(API_NotSupportedFeature, ss.str(), "DeviceProxy::subscribe_event()");
    }

    ApiUtil *api_ptr = ApiUtil::instance();
    if (api_ptr->get_zmq_event_consumer() == NULL)
    {
        api_ptr->create_zmq_event_consumer();
    }

    int ret;
    ret = api_ptr->get_zmq_event_consumer()->subscribe_event(this, event, event_queue_size, stateless);

    return ret;
}

//--------------------------------------------------------------------------------------------------------------------
//
// method :
// 		DeviceProxy::unsubscribe_event
//
// description :
//		Unsubscribe to an event
//
//-------------------------------------------------------------------------------------------------------------------

void DeviceProxy::unsubscribe_event(int event_id)
{
    ApiUtil *api_ptr = ApiUtil::instance();
    if (api_ptr->get_zmq_event_consumer() == NULL)
    {
        TangoSys_OMemStream desc;
        desc << "Could not find event consumer object, \n";
        desc << "probably no event subscription was done before!";
        desc << ends;
        Tango::Except::throw_exception(
            (const char *) "API_EventConsumer",
            desc.str(),
            (const char *) "DeviceProxy::unsubscribe_event()");
    }

    if (api_ptr->get_zmq_event_consumer()->get_event_system_for_event_id(event_id) == ZMQ)
    {
        api_ptr->get_zmq_event_consumer()->unsubscribe_event(event_id);
    }
    else
    {
        TangoSys_OMemStream desc;
        desc << "Could not find event consumer object, \n";
        desc << "probably no event subscription was done before!";
        desc << ends;
        Tango::Except::throw_exception(
            (const char *) "API_EventConsumer",
            desc.str(),
            (const char *) "DeviceProxy::unsubscribe_event()");
    }
}

//-----------------------------------------------------------------------------
//
// method :       DeviceProxy::get_events()
//
// description :  Return a vector with all events stored in the event queue.
//                Events are kept in the buffer since the last extraction
//                with get_events().
//                After returning the event data, the event queue gets
//                emptied!
//
// argument : in  : event_id   : The event identifier
// argument : out : event_list : A reference to an event data list to be filled
//-----------------------------------------------------------------------------
void DeviceProxy::get_events(int event_id, EventDataList &event_list)
{
    ApiUtil *api_ptr = ApiUtil::instance();
    if (api_ptr->get_zmq_event_consumer() == NULL)
    {
        TangoSys_OMemStream desc;
        desc << "Could not find event consumer object, \n";
        desc << "probably no event subscription was done before!";
        desc << ends;
        Tango::Except::throw_exception(
            (const char *) "API_EventConsumer",
            desc.str(),
            (const char *) "DeviceProxy::get_events()");
    }

    if (api_ptr->get_zmq_event_consumer()->get_event_system_for_event_id(event_id) == ZMQ)
    {
        api_ptr->get_zmq_event_consumer()->get_events(event_id, event_list);
    }
    else
    {
        TangoSys_OMemStream desc;
        desc << "Could not find event consumer object, \n";
        desc << "probably no event subscription was done before!";
        desc << ends;
        Tango::Except::throw_exception(
            (const char *) "API_EventConsumer",
            desc.str(),
            (const char *) "DeviceProxy::get_events()");
    }
}

//-----------------------------------------------------------------------------
//
// method :       DeviceProxy::get_events()
//
// description :  Return a vector with all attribute configuration events
//                stored in the event queue.
//                Events are kept in the buffer since the last extraction
//                with get_events().
//                After returning the event data, the event queue gets
//                emptied!
//
// argument : in  : event_id   : The event identifier
// argument : out : event_list : A reference to an event data list to be filled
//-----------------------------------------------------------------------------
void DeviceProxy::get_events(int event_id, AttrConfEventDataList &event_list)
{
    ApiUtil *api_ptr = ApiUtil::instance();
    if (api_ptr->get_zmq_event_consumer() == NULL)
    {
        TangoSys_OMemStream desc;
        desc << "Could not find event consumer object, \n";
        desc << "probably no event subscription was done before!";
        desc << ends;
        Tango::Except::throw_exception(API_EventConsumer, desc.str(), "DeviceProxy::get_events()");
    }

    if (api_ptr->get_zmq_event_consumer()->get_event_system_for_event_id(event_id) == ZMQ)
    {
        api_ptr->get_zmq_event_consumer()->get_events(event_id, event_list);
    }
    else
    {
        TangoSys_OMemStream desc;
        desc << "Could not find event consumer object, \n";
        desc << "probably no event subscription was done before!";
        desc << ends;
        Tango::Except::throw_exception(API_EventConsumer, desc.str(), "DeviceProxy::get_events()");
    }
}

void DeviceProxy::get_events(int event_id, DataReadyEventDataList &event_list)
{
    ApiUtil *api_ptr = ApiUtil::instance();
    if (api_ptr->get_zmq_event_consumer() == NULL)
    {
        TangoSys_OMemStream desc;
        desc << "Could not find event consumer object, \n";
        desc << "probably no event subscription was done before!";
        desc << ends;
        Tango::Except::throw_exception(API_EventConsumer, desc.str(), "DeviceProxy::get_events()");
    }

    if (api_ptr->get_zmq_event_consumer()->get_event_system_for_event_id(event_id) == ZMQ)
    {
        api_ptr->get_zmq_event_consumer()->get_events(event_id, event_list);
    }
    else
    {
        TangoSys_OMemStream desc;
        desc << "Could not find event consumer object, \n";
        desc << "probably no event subscription was done before!";
        desc << ends;
        Tango::Except::throw_exception(API_EventConsumer, desc.str(), "DeviceProxy::get_events()");
    }
}

void DeviceProxy::get_events(int event_id, DevIntrChangeEventDataList &event_list)
{
    ApiUtil *api_ptr = ApiUtil::instance();
    if (api_ptr->get_zmq_event_consumer() == NULL)
    {
        stringstream desc;
        desc << "Could not find event consumer object, \n";
        desc << "probably no event subscription was done before!";
        Tango::Except::throw_exception(API_EventConsumer, desc.str(), "DeviceProxy::get_events()");
    }

    if (api_ptr->get_zmq_event_consumer()->get_event_system_for_event_id(event_id) == ZMQ)
    {
        api_ptr->get_zmq_event_consumer()->get_events(event_id, event_list);
    }
    else
    {
        stringstream desc;
        desc << "Event Device Interface Change not implemented in old Tango event system (notifd)";
        Tango::Except::throw_exception(API_UnsupportedFeature, desc.str(), "DeviceProxy::get_events()");
    }
}

void DeviceProxy::get_events(int event_id, PipeEventDataList &event_list)
{
    ApiUtil *api_ptr = ApiUtil::instance();
    if (api_ptr->get_zmq_event_consumer() == NULL)
    {
        stringstream desc;
        desc << "Could not find event consumer object, \n";
        desc << "probably no event subscription was done before!";
        Tango::Except::throw_exception(API_EventConsumer, desc.str(), "DeviceProxy::get_events()");
    }

    if (api_ptr->get_zmq_event_consumer()->get_event_system_for_event_id(event_id) == ZMQ)
    {
        api_ptr->get_zmq_event_consumer()->get_events(event_id, event_list);
    }
    else
    {
        stringstream desc;
        desc << "Pipe event not implemented in old Tango event system (notifd)";
        Tango::Except::throw_exception(API_UnsupportedFeature, desc.str(), "DeviceProxy::get_events()");
    }
}

//-----------------------------------------------------------------------------
//
// method :       DeviceProxy::get_events()
//
// description :  Call the callback method for all events stored
//                in the event queue.
//                Events are kept in the buffer since the last extraction
//                with get_events().
//                After returning the event data, the event queue gets
//                emptied!
//
// argument : in  : event_id   : The event identifier
// argument : out : cb : The callback object pointer
//-----------------------------------------------------------------------------
void DeviceProxy::get_events(int event_id, CallBack *cb)
{
    ApiUtil *api_ptr = ApiUtil::instance();
    if (api_ptr->get_zmq_event_consumer() == NULL)
    {
        TangoSys_OMemStream desc;
        desc << "Could not find event consumer object, \n";
        desc << "probably no event subscription was done before!";
        desc << ends;
        Tango::Except::throw_exception(
            (const char *) "API_EventConsumer",
            desc.str(),
            (const char *) "DeviceProxy::get_events()");
    }

    if (api_ptr->get_zmq_event_consumer()->get_event_system_for_event_id(event_id) == ZMQ)
    {
        api_ptr->get_zmq_event_consumer()->get_events(event_id, cb);
    }
    else
    {
        TangoSys_OMemStream desc;
        desc << "Could not find event consumer object, \n";
        desc << "probably no event subscription was done before!";
        desc << ends;
        Tango::Except::throw_exception(
            (const char *) "API_EventConsumer",
            desc.str(),
            (const char *) "DeviceProxy::get_events()");
    }
}

//+----------------------------------------------------------------------------
//
// method :       DeviceProxy::event_queue_size()
//
// description :  Returns the number of events stored in the event queue
//
// argument : in : event_id   : The event identifier
//
//-----------------------------------------------------------------------------
int DeviceProxy::event_queue_size(int event_id)
{
    ApiUtil *api_ptr = ApiUtil::instance();
    if (api_ptr->get_zmq_event_consumer() == NULL)
    {
        TangoSys_OMemStream desc;
        desc << "Could not find event consumer object, \n";
        desc << "probably no event subscription was done before!";
        desc << ends;
        Tango::Except::throw_exception(
            (const char *) "API_EventConsumer",
            desc.str(),
            (const char *) "DeviceProxy::event_queue_size()");
    }

    EventConsumer *ev = NULL;
    if (api_ptr->get_zmq_event_consumer()->get_event_system_for_event_id(event_id) == ZMQ)
    {
        ev = api_ptr->get_zmq_event_consumer();
    }
    else
    {
        TangoSys_OMemStream desc;
        desc << "Could not find event consumer object, \n";
        desc << "probably no event subscription was done before!";
        desc << ends;
        Tango::Except::throw_exception(
            (const char *) "API_EventConsumer",
            desc.str(),
            (const char *) "DeviceProxy::event_queue_size()");
    }

    return ev->event_queue_size(event_id);
}

//+----------------------------------------------------------------------------
//
// method :       DeviceProxy::is_event_queue_empty()
//
// description :  Returns true when the event queue is empty
//
// argument : in : event_id   : The event identifier
//
//-----------------------------------------------------------------------------
bool DeviceProxy::is_event_queue_empty(int event_id)
{
    ApiUtil *api_ptr = ApiUtil::instance();
    if (api_ptr->get_zmq_event_consumer() == NULL)
    {
        TangoSys_OMemStream desc;
        desc << "Could not find event consumer object, \n";
        desc << "probably no event subscription was done before!";
        desc << ends;
        Tango::Except::throw_exception(
            (const char *) "API_EventConsumer",
            desc.str(),
            (const char *) "DeviceProxy::is_event_queue_empty()");
    }

    EventConsumer *ev = NULL;
    if (api_ptr->get_zmq_event_consumer()->get_event_system_for_event_id(event_id) == ZMQ)
    {
        ev = api_ptr->get_zmq_event_consumer();
    }
    else
    {
        TangoSys_OMemStream desc;
        desc << "Could not find event consumer object, \n";
        desc << "probably no event subscription was done before!";
        desc << ends;
        Tango::Except::throw_exception(
            (const char *) "API_EventConsumer",
            desc.str(),
            (const char *) "DeviceProxy::is_event_queue_empty()");
    }

    return (ev->is_event_queue_empty(event_id));
}

//+----------------------------------------------------------------------------
//
// method :       DeviceProxy::get_last_event_date()
//
// description :  Get the time stamp of the last inserted event
//
// argument : in : event_id   : The event identifier
//
//-----------------------------------------------------------------------------
TimeVal DeviceProxy::get_last_event_date(int event_id)
{
    ApiUtil *api_ptr = ApiUtil::instance();
    if (api_ptr->get_zmq_event_consumer() == NULL)
    {
        TangoSys_OMemStream desc;
        desc << "Could not find event consumer object, \n";
        desc << "probably no event subscription was done before!";
        desc << ends;
        Tango::Except::throw_exception(
            (const char *) "API_EventConsumer",
            desc.str(),
            (const char *) "DeviceProxy::get_last_event_date()");
    }

    EventConsumer *ev = NULL;
    if (api_ptr->get_zmq_event_consumer()->get_event_system_for_event_id(event_id) == ZMQ)
    {
        ev = api_ptr->get_zmq_event_consumer();
    }
    else
    {
        TangoSys_OMemStream desc;
        desc << "Could not find event consumer object, \n";
        desc << "probably no event subscription was done before!";
        desc << ends;
        Tango::Except::throw_exception(
            (const char *) "API_EventConsumer",
            desc.str(),
            (const char *) "DeviceProxy::get_last_event_date()");
    }

    return (ev->get_last_event_date(event_id));
}


//-----------------------------------------------------------------------------
//
// DeviceProxy::get_device_db - get database
//
//-----------------------------------------------------------------------------

Database *DeviceProxy::get_device_db()
{
    if ((db_port_num != 0) && (db_dev != NULL))
        return db_dev->get_dbase();
    else
        return (Database *) NULL;
}

//-----------------------------------------------------------------------------
//
// clean_lock - Litle function installed in the list of function(s) to be called
// at exit time. It will clean all locking thread(s) and unlock locked device(s)
//
//-----------------------------------------------------------------------------

void clean_lock()
{
    if (ApiUtil::_is_instance_null() == false)
    {
        ApiUtil *au = ApiUtil::instance();
        au->clean_locking_threads();
    }
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::lock - Lock the device
//
//-----------------------------------------------------------------------------

void DeviceProxy::lock(int lock_validity)
{

//
// Feature unavailable for device without database
//

    if (dbase_used == false)
    {
        TangoSys_OMemStream desc;
        desc << "Feature not available for device ";
        desc << device_name;
        desc << " which is a non database device";

        ApiNonDbExcept::throw_exception((const char *) API_NonDatabaseDevice,
                                        desc.str(),
                                        (const char *) "DeviceProxy::lock");
    }

//
// Some checks on lock validity
//

    if (lock_validity < MIN_LOCK_VALIDITY)
    {
        TangoSys_OMemStream desc;
        desc << "Lock validity can not be lower than " << MIN_LOCK_VALIDITY << " seconds" << ends;

        Except::throw_exception((const char *) API_MethodArgument, desc.str(),
                                (const char *) "DeviceProxy::lock");
    }

    {
        omni_mutex_lock guard(lock_mutex);
        if (lock_ctr != 0)
        {
            if (lock_validity != lock_valid)
            {
                TangoSys_OMemStream desc;

                desc << "Device " << device_name << " is already locked with another lock validity (";
                desc << lock_valid << " sec)" << ends;

                Except::throw_exception((const char *) API_MethodArgument, desc.str(),
                                        (const char *) "DeviceProxy::lock");
            }
        }
    }

//
// Check if the function to be executed atexit is already installed
//

    Tango::ApiUtil *au = ApiUtil::instance();
    if (au->is_lock_exit_installed() == false)
    {
        atexit(clean_lock);
        au->set_sig_handler();
        au->set_lock_exit_installed(true);
    }

//
// Connect to the admin device if not already done
//

    check_connect_adm_device();

//
// Send command to admin device
//

    string cmd("LockDevice");
    DeviceData din;
    DevVarLongStringArray sent_data;
    sent_data.svalue.length(1);
    sent_data.svalue[0] = Tango::string_dup(device_name.c_str());
    sent_data.lvalue.length(1);
    sent_data.lvalue[0] = lock_validity;
    din << sent_data;

    adm_device->command_inout(cmd, din);

//
// Increment locking counter
//

    {
        omni_mutex_lock guard(lock_mutex);

        lock_ctr++;
        lock_valid = lock_validity;
    }

//
// Try to find the device's server admin device locking thread
// in the ApiUtil map.
// If the thread is not there, start one.
// If the thread is there, ask it to add the device to its list
// of locked devices
//

    {
        omni_mutex_lock oml(au->lock_th_map);

        map<string, LockingThread>::iterator pos = au->lock_threads.find(adm_dev_name);
        if (pos == au->lock_threads.end())
        {
            create_locking_thread(au, lock_validity);
        }
        else
        {
            bool local_suicide;
            {
                omni_mutex_lock sync(*(pos->second.mon));
                local_suicide = pos->second.shared->suicide;
            }

            if (local_suicide == true)
            {
                delete pos->second.shared;
                delete pos->second.mon;
                au->lock_threads.erase(pos);

                create_locking_thread(au, lock_validity);
            }
            else
            {
                int interupted;

                omni_mutex_lock sync(*(pos->second.mon));
                if (pos->second.shared->cmd_pending == true)
                {
                    interupted = pos->second.mon->wait(DEFAULT_TIMEOUT);

                    if ((pos->second.shared->cmd_pending == true) && (interupted == 0))
                    {
                        cout4 << "TIME OUT" << endl;
                        Except::throw_exception((const char *) API_CommandTimedOut,
                                                (const char *) "Locking thread blocked !!!",
                                                (const char *) "DeviceProxy::lock");
                    }
                }
                pos->second.shared->cmd_pending = true;
                pos->second.shared->cmd_code = LOCK_ADD_DEV;
                pos->second.shared->dev_name = device_name;
                {
                    omni_mutex_lock guard(lock_mutex);
                    pos->second.shared->lock_validity = lock_valid;
                }

                pos->second.mon->signal();

                cout4 << "Cmd sent to locking thread" << endl;

                while (pos->second.shared->cmd_pending == true)
                {
                    interupted = pos->second.mon->wait(DEFAULT_TIMEOUT);

                    if ((pos->second.shared->cmd_pending == true) && (interupted == 0))
                    {
                        cout4 << "TIME OUT" << endl;
                        Except::throw_exception((const char *) API_CommandTimedOut,
                                                (const char *) "Locking thread blocked !!!",
                                                (const char *) "DeviceProxy::lock");
                    }
                }
            }
        }
    }
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::unlock - Unlock the device
//
//-----------------------------------------------------------------------------

void DeviceProxy::unlock(bool force)
{

//
// Feature unavailable for device without database
//

    if (dbase_used == false)
    {
        TangoSys_OMemStream desc;
        desc << "Feature not available for device ";
        desc << device_name;
        desc << " which is a non database device";

        ApiNonDbExcept::throw_exception((const char *) API_NonDatabaseDevice,
                                        desc.str(),
                                        (const char *) "DeviceProxy::unlock");
    }

    check_connect_adm_device();

//
// Send command to admin device
//

    string cmd("UnLockDevice");
    DeviceData din, dout;
    DevVarLongStringArray sent_data;
    sent_data.svalue.length(1);
    sent_data.svalue[0] = Tango::string_dup(device_name.c_str());
    sent_data.lvalue.length(1);
    if (force == true)
        sent_data.lvalue[0] = 1;
    else
        sent_data.lvalue[0] = 0;
    din << sent_data;

//
// Send request to the DS admin device
//

    dout = adm_device->command_inout(cmd, din);

//
// Decrement locking counter or replace it by the device global counter
// returned by the server
//

    Tango::DevLong glob_ctr;

    dout >> glob_ctr;
    int local_lock_ctr;

    {
        omni_mutex_lock guard(lock_mutex);

        lock_ctr--;
        if (glob_ctr != lock_ctr)
            lock_ctr = glob_ctr;
        local_lock_ctr = lock_ctr;
    }

//
// Try to find the device's server admin device locking thread
// in the ApiUtil map.
// Ask the thread to remove the device to its list of locked devices
//

    if ((local_lock_ctr == 0) || (force == true))
    {
        Tango::ApiUtil *au = Tango::ApiUtil::instance();

        {
            omni_mutex_lock oml(au->lock_th_map);
            map<string, LockingThread>::iterator pos = au->lock_threads.find(adm_dev_name);
            if (pos == au->lock_threads.end())
            {
//				TangoSys_OMemStream o;

//				o << "Can't find the locking thread for device " << device_name << " and admin device " << adm_dev_name << ends;
//				Tango::Except::throw_exception((const char *)API_CantFindLockingThread,o.str(),
//                                           		(const char *)"DeviceProxy::unlock()");
            }
            else
            {
                if (pos->second.shared->suicide == true)
                {
                    delete pos->second.shared;
                    delete pos->second.mon;
                    au->lock_threads.erase(pos);
                }
                else
                {
                    int interupted;

                    omni_mutex_lock sync(*(pos->second.mon));
                    if (pos->second.shared->cmd_pending == true)
                    {
                        interupted = pos->second.mon->wait(DEFAULT_TIMEOUT);

                        if ((pos->second.shared->cmd_pending == true) && (interupted == 0))
                        {
                            cout4 << "TIME OUT" << endl;
                            Except::throw_exception((const char *) API_CommandTimedOut,
                                                    (const char *) "Locking thread blocked !!!",
                                                    (const char *) "DeviceProxy::unlock");
                        }
                    }
                    pos->second.shared->cmd_pending = true;
                    pos->second.shared->cmd_code = LOCK_REM_DEV;
                    pos->second.shared->dev_name = device_name;

                    pos->second.mon->signal();

                    cout4 << "Cmd sent to locking thread" << endl;

                    while (pos->second.shared->cmd_pending == true)
                    {
                        interupted = pos->second.mon->wait(DEFAULT_TIMEOUT);

                        if ((pos->second.shared->cmd_pending == true) && (interupted == 0))
                        {
                            cout4 << "TIME OUT" << endl;
                            Except::throw_exception((const char *) API_CommandTimedOut,
                                                    (const char *) "Locking thread blocked !!!",
                                                    (const char *) "DeviceProxy::unlock");
                        }
                    }
                }
            }
        }
    }
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::create_locking_thread - Create and start a locking thread
//
//-----------------------------------------------------------------------------

void DeviceProxy::create_locking_thread(ApiUtil *au, DevLong dl)
{

    LockingThread lt;
    lt.mon = NULL;
    lt.l_thread = NULL;
    lt.shared = NULL;

    pair<map<string, LockingThread>::iterator, bool> status;
    status = au->lock_threads.insert(make_pair(adm_dev_name, lt));
    if (status.second == false)
    {
        TangoSys_OMemStream o;
        o << "Can't create the locking thread for device " << device_name << " and admin device " << adm_dev_name
          << ends;
        Tango::Except::throw_exception((const char *) API_CantCreateLockingThread, o.str(),
                                       (const char *) "DeviceProxy::create_locking_thread()");
    }
    else
    {
        map<string, LockingThread>::iterator pos;

        pos = status.first;
        pos->second.mon = new TangoMonitor(adm_dev_name.c_str());
        pos->second.shared = new LockThCmd;
        pos->second.shared->cmd_pending = false;
        pos->second.shared->suicide = false;
        pos->second.l_thread = new LockThread(*pos->second.shared, *pos->second.mon, adm_device, device_name, dl);

        pos->second.l_thread->start();
    }
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::locking_status - Return a device locking status as a string
//
//-----------------------------------------------------------------------------

string DeviceProxy::locking_status()
{
    vector<string> v_str;
    vector<DevLong> v_l;

    ask_locking_status(v_str, v_l);

    string str(v_str[0]);
    return str;
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::is_locked - Check if a device is locked
//
// Returns true if locked, false otherwise
//
//-----------------------------------------------------------------------------

bool DeviceProxy::is_locked()
{
    vector<string> v_str;
    vector<DevLong> v_l;

    ask_locking_status(v_str, v_l);

    return (bool) v_l[0];
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::is_locked_by_me - Check if a device is locked by the caller
//
// Returns true if the caller is the locker, false otherwise
//
//-----------------------------------------------------------------------------

bool DeviceProxy::is_locked_by_me()
{
    vector<string> v_str;
    vector<DevLong> v_l;

    ask_locking_status(v_str, v_l);

    bool ret = false;

    if (v_l[0] == 0)
        ret = false;
    else
    {
#ifndef _TG_WINDOWS_
        if (getpid() != v_l[1])
#else
            if (_getpid() != v_l[1])
#endif
            ret = false;
        else if ((v_l[2] != 0) || (v_l[3] != 0) || (v_l[4] != 0) || (v_l[5] != 0))
            ret = false;
        else
        {
            string full_ip_str;
            get_locker_host(v_str[1], full_ip_str);

//
// If the call is local, as the PID is already the good one, the caller is the locker
//

            if (full_ip_str == TG_LOCAL_HOST)
                ret = true;
            else
            {

//
// Get the host address(es) and check if it is the same than the one sent by the server
//

                ApiUtil *au = ApiUtil::instance();
                vector<string> adrs;
                string at_least_one;

                au->get_ip_from_if(adrs);

                for (unsigned int nb_adrs = 0; nb_adrs < adrs.size(); nb_adrs++)
                {
                    if (adrs[nb_adrs] == full_ip_str)
                    {
                        ret = true;
                        break;
                    }
                }
            }
        }
    }
    return ret;
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::get_locker - Get some info on the device locker if the device
// is locked
// This method returns true if the device is effectively locked.
// Otherwise, it returns false
//
//-----------------------------------------------------------------------------

bool DeviceProxy::get_locker(LockerInfo &lock_info)
{
    vector<string> v_str;
    vector<DevLong> v_l;

    ask_locking_status(v_str, v_l);

    if (v_l[0] == 0)
        return false;
    else
    {

//
// If the PID info coming from server is not 0, the locker is CPP
// Otherwise, it is Java
//

        if (v_l[1] != 0)
        {
            lock_info.ll = Tango::CPP;
            lock_info.li.LockerPid = v_l[1];

            lock_info.locker_class = "Not defined";
        }
        else
        {
            lock_info.ll = Tango::JAVA;
            for (int loop = 0; loop < 4; loop++)
                lock_info.li.UUID[loop] = v_l[2 + loop];

            string full_ip;
            get_locker_host(v_str[1], full_ip);

            lock_info.locker_class = v_str[2];
        }

//
// Add locker host name
//

        string full_ip;
        get_locker_host(v_str[1], full_ip);

//
// Convert locker IP address to its name
//

        if (full_ip != TG_LOCAL_HOST)
        {
            struct sockaddr_in si;
            si.sin_family = AF_INET;
            si.sin_port = 0;
#ifdef _TG_WINDOWS_
                                                                                                                                    int slen = sizeof(si);
			WSAStringToAddress((char *)full_ip.c_str(),AF_INET,NULL,(SOCKADDR *)&si,&slen);
#else
            inet_pton(AF_INET, full_ip.c_str(), &si.sin_addr);
#endif

            char host_os[512];

            int res = getnameinfo((const sockaddr *) &si, sizeof(si), host_os, 512, 0, 0, 0);

            if (res == 0)
                lock_info.locker_host = host_os;
            else
                lock_info.locker_host = full_ip;
        }
        else
        {
            char h_name[80];
            gethostname(h_name, 80);

            lock_info.locker_host = h_name;
        }
    }

    return true;
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::ask_locking_status - Get the device locking status
//
//-----------------------------------------------------------------------------

void DeviceProxy::ask_locking_status(vector<string> &v_str, vector<DevLong> &v_l)
{

//
// Feature unavailable for device without database
//

    if (dbase_used == false)
    {
        TangoSys_OMemStream desc;
        desc << "Feature not available for device ";
        desc << device_name;
        desc << " which is a non database device";

        ApiNonDbExcept::throw_exception((const char *) API_NonDatabaseDevice,
                                        desc.str(),
                                        (const char *) "DeviceProxy::locking_status");
    }

    check_connect_adm_device();

//
// Send command to admin device
//

    string cmd("DevLockStatus");
    DeviceData din, dout;
    din.any <<= device_name.c_str();

    dout = adm_device->command_inout(cmd, din);

//
// Extract data and return data to caller
//

    dout.extract(v_l, v_str);
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::get_locker_host - Isolate from the host string as it is returned
// by omniORB, only the host IP address
//
//-----------------------------------------------------------------------------

void DeviceProxy::get_locker_host(string &f_addr, string &ip_addr)
{
//
// The hostname is returned in the following format:
// "giop:tcp:160.103.5.157:32989" or "giop:tcp:[::ffff:160.103.5.157]:32989
// or "giop:unix:/tmp..."
// We need to isolate the IP address
//

    bool ipv6 = false;

    if (f_addr.find('[') != string::npos)
        ipv6 = true;

    if (f_addr.find(":unix:") != string::npos)
    {
        ip_addr = TG_LOCAL_HOST;
    }
    else
    {
        string::size_type pos;
        if ((pos = f_addr.find(':')) == string::npos)
        {
            Tango::Except::throw_exception((const char *) API_WrongLockingStatus,
                                           (const char *) "Locker IP address returned by server is unvalid",
                                           (const char *) "DeviceProxy::get_locker_host()");
        }
        pos++;
        if ((pos = f_addr.find(':', pos)) == string::npos)
        {
            Tango::Except::throw_exception((const char *) API_WrongLockingStatus,
                                           (const char *) "Locker IP address returned by server is unvalid",
                                           (const char *) "DeviceProxy::get_locker_host()");
        }
        pos++;

        if (ipv6 == true)
        {
            pos = pos + 3;
            if ((pos = f_addr.find(':', pos)) == string::npos)
            {
                Tango::Except::throw_exception((const char *) API_WrongLockingStatus,
                                               (const char *) "Locker IP address returned by server is unvalid",
                                               (const char *) "DeviceProxy::get_locker_host()");
            }
            pos++;
            string ip_str = f_addr.substr(pos);
            if ((pos = ip_str.find(']')) == string::npos)
            {
                Tango::Except::throw_exception((const char *) API_WrongLockingStatus,
                                               (const char *) "Locker IP address returned by server is unvalid",
                                               (const char *) "DeviceProxy::get_locker_host()");
            }
            ip_addr = ip_str.substr(0, pos);
        }
        else
        {
            string ip_str = f_addr.substr(pos);
            if ((pos = ip_str.find(':')) == string::npos)
            {
                Tango::Except::throw_exception((const char *) API_WrongLockingStatus,
                                               (const char *) "Locker IP address returned by server is unvalid",
                                               (const char *) "DeviceProxy::get_locker_host()");
            }
            ip_addr = ip_str.substr(0, pos);
        }
    }
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::write_read_attribute() - write then read a single attribute
//
//-----------------------------------------------------------------------------

DeviceAttribute DeviceProxy::write_read_attribute(DeviceAttribute &dev_attr)
{

//
// This call is available only for Devices implemented IDL V4
//

    if (version < 4)
    {
        TangoSys_OMemStream desc;
        desc << "Device " << device_name;
        desc << " does not support write_read_attribute feature" << ends;
        ApiNonSuppExcept::throw_exception((const char *) API_UnsupportedFeature,
                                          desc.str(),
                                          (const char *) "DeviceProxy::write_read_attribute");
    }

//
// Data into the AttributeValue object
//

    AttributeValueList_4 attr_value_list;
    attr_value_list.length(1);

    attr_value_list[0].name = dev_attr.name.c_str();
    attr_value_list[0].quality = dev_attr.quality;
    attr_value_list[0].data_format = dev_attr.data_format;
    attr_value_list[0].time = dev_attr.time;
    attr_value_list[0].w_dim.dim_x = dev_attr.dim_x;
    attr_value_list[0].w_dim.dim_y = dev_attr.dim_y;

    if (dev_attr.LongSeq.operator->() != NULL)
        attr_value_list[0].value.long_att_value(dev_attr.LongSeq.in());
    else if (dev_attr.Long64Seq.operator->() != NULL)
        attr_value_list[0].value.long64_att_value(dev_attr.Long64Seq.in());
    else if (dev_attr.ShortSeq.operator->() != NULL)
        attr_value_list[0].value.short_att_value(dev_attr.ShortSeq.in());
    else if (dev_attr.DoubleSeq.operator->() != NULL)
        attr_value_list[0].value.double_att_value(dev_attr.DoubleSeq.in());
    else if (dev_attr.StringSeq.operator->() != NULL)
        attr_value_list[0].value.string_att_value(dev_attr.StringSeq.in());
    else if (dev_attr.FloatSeq.operator->() != NULL)
        attr_value_list[0].value.float_att_value(dev_attr.FloatSeq.in());
    else if (dev_attr.BooleanSeq.operator->() != NULL)
        attr_value_list[0].value.bool_att_value(dev_attr.BooleanSeq.in());
    else if (dev_attr.UShortSeq.operator->() != NULL)
        attr_value_list[0].value.ushort_att_value(dev_attr.UShortSeq.in());
    else if (dev_attr.UCharSeq.operator->() != NULL)
        attr_value_list[0].value.uchar_att_value(dev_attr.UCharSeq.in());
    else if (dev_attr.ULongSeq.operator->() != NULL)
        attr_value_list[0].value.ulong_att_value(dev_attr.ULongSeq.in());
    else if (dev_attr.ULong64Seq.operator->() != NULL)
        attr_value_list[0].value.ulong64_att_value(dev_attr.ULong64Seq.in());
    else if (dev_attr.StateSeq.operator->() != NULL)
        attr_value_list[0].value.state_att_value(dev_attr.StateSeq.in());
    else if (dev_attr.EncodedSeq.operator->() != NULL)
        attr_value_list[0].value.encoded_att_value(dev_attr.EncodedSeq.in());

    Tango::DevVarStringArray dvsa;
    dvsa.length(1);
    dvsa[0] = Tango::string_dup(dev_attr.name.c_str());

    int ctr = 0;
    AttributeValueList_4_var attr_value_list_4;
    AttributeValueList_5_var attr_value_list_5;
    Tango::AccessControlType local_act;

    while (ctr < 2)
    {
        try
        {
            check_and_reconnect(local_act);

//
// Throw exception if caller not allowed to write_attribute
//

            if (local_act == ACCESS_READ)
            {
                try
                {
                    Device_var dev = Device::_duplicate(device);
                    dev->ping();
                }
                catch (...)
                {
                    set_connection_state(CONNECTION_NOTOK);
                    throw;
                }

                TangoSys_OMemStream desc;
                desc << "Writing attribute(s) on device " << dev_name() << " is not authorized" << ends;

                NotAllowedExcept::throw_exception((const char *) API_ReadOnlyMode, desc.str(),
                                                  (const char *) "DeviceProxy::write_read_attribute()");
            }

//
// Now, call the server
//

            ClntIdent ci;
            ApiUtil *au = ApiUtil::instance();
            ci.cpp_clnt(au->get_client_pid());

            if (version >= 5)
            {
                Device_5_var dev = Device_5::_duplicate(device_5);
                attr_value_list_5 = dev->write_read_attributes_5(attr_value_list, dvsa, ci);
            }
            else
            {
                Device_4_var dev = Device_4::_duplicate(device_4);
                attr_value_list_4 = dev->write_read_attributes_4(attr_value_list, ci);
            }

            ctr = 2;

        }
        catch (Tango::MultiDevFailed &e)
        {

//
// Transfer this exception into a DevFailed exception
//

            Tango::DevFailed ex(e.errors[0].err_list);
            TangoSys_OMemStream desc;
            desc << "Failed to write_read_attribute on device " << device_name;
            desc << ", attribute ";
            desc << attr_value_list[0].name.in();
            desc << ends;
            Except::re_throw_exception(ex, (const char *) API_AttributeFailed,
                                       desc.str(), (const char *) "DeviceProxy::write_read_attribute()");

        }
        catch (Tango::DevFailed &e)
        {
            TangoSys_OMemStream desc;
            desc << "Failed to write_read_attribute on device " << device_name;
            desc << ", attribute ";
            desc << attr_value_list[0].name.in();
            desc << ends;

            if (::strcmp(e.errors[0].reason, DEVICE_UNLOCKED_REASON) == 0)
                DeviceUnlockedExcept::re_throw_exception(e, (const char *) DEVICE_UNLOCKED_REASON,
                                                         desc.str(),
                                                         (const char *) "DeviceProxy::write_read_attribute()");
            else
                Except::re_throw_exception(e, (const char *) API_AttributeFailed,
                                           desc.str(), (const char *) "DeviceProxy::write_read_attribute()");
        }
        catch (CORBA::TRANSIENT &trans)
        {
            TRANSIENT_NOT_EXIST_EXCEPT(trans, "DeviceProxy", "write_read_attribute()", this);
        }
        catch (CORBA::OBJECT_NOT_EXIST &one)
        {
            if (one.minor() == omni::OBJECT_NOT_EXIST_NoMatch || one.minor() == 0)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(one, "DeviceProxy", "write_read_attribute", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute write_read_attribute on device " << device_name << ends;
                ApiCommExcept::re_throw_exception(one,
                                                  (const char *) "API_CommunicationFailed",
                                                  desc.str(),
                                                  (const char *) "DeviceProxy::write_read_attribute()");
            }
        }
        catch (CORBA::COMM_FAILURE &comm)
        {
            if (comm.minor() == omni::COMM_FAILURE_WaitingForReply)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(comm, "DeviceProxy", "write_read_attribute", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute write_attribute on device " << device_name << ends;
                ApiCommExcept::re_throw_exception(comm,
                                                  (const char *) "API_CommunicationFailed",
                                                  desc.str(),
                                                  (const char *) "DeviceProxy::write_read_attribute()");
            }
        }
        catch (CORBA::SystemException &ce)
        {
            set_connection_state(CONNECTION_NOTOK);

            TangoSys_OMemStream desc;
            desc << "Failed to execute write_attributes on device " << device_name << ends;
            ApiCommExcept::re_throw_exception(ce,
                                              (const char *) "API_CommunicationFailed",
                                              desc.str(),
                                              (const char *) "DeviceProxy::write_read_attribute()");
        }
    }

//
// Init the returned DeviceAttribute instance
//

    DeviceAttribute ret_dev_attr;
    if (version >= 5)
        ApiUtil::attr_to_device(&(attr_value_list_5[0]), version, &ret_dev_attr);
    else
        ApiUtil::attr_to_device(&(attr_value_list_4[0]), version, &ret_dev_attr);

//
// Add an error in the error stack in case there is one
//

    DevErrorList_var &err_list = ret_dev_attr.get_error_list();
    long nb_except = err_list.in().length();
    if (nb_except != 0)
    {
        TangoSys_OMemStream desc;
        desc << "Failed to write_read_attribute on device " << device_name;
        desc << ", attribute " << dev_attr.name << ends;

        err_list.inout().length(nb_except + 1);
        err_list[nb_except].reason = Tango::string_dup(API_AttributeFailed);
        err_list[nb_except].origin = Tango::string_dup("DeviceProxy::write_read_attribute()");

        string st = desc.str();
        err_list[nb_except].desc = Tango::string_dup(st.c_str());
        err_list[nb_except].severity = Tango::ERR;
    }

    return (ret_dev_attr);
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::write_read_attributes() - write then read a single attribute
//
//-----------------------------------------------------------------------------

vector<DeviceAttribute> *
DeviceProxy::write_read_attributes(vector<DeviceAttribute> &attr_list, vector<string> &r_names)
{

//
// This call is available only for Devices implemented IDL V5
//

    if (version < 5)
    {
        TangoSys_OMemStream desc;
        desc << "Device " << device_name;
        desc << " does not support write_read_attributes feature" << ends;
        ApiNonSuppExcept::throw_exception((const char *) API_UnsupportedFeature,
                                          desc.str(),
                                          (const char *) "DeviceProxy::write_read_attributes");
    }

//
// Data into the AttributeValue object
//

    AttributeValueList_4 attr_value_list;
    attr_value_list.length(attr_list.size());

    for (unsigned int i = 0; i < attr_list.size(); i++)
    {
        attr_value_list[i].name = attr_list[i].name.c_str();
        attr_value_list[i].quality = attr_list[i].quality;
        attr_value_list[i].data_format = attr_list[i].data_format;
        attr_value_list[i].time = attr_list[i].time;
        attr_value_list[i].w_dim.dim_x = attr_list[i].dim_x;
        attr_value_list[i].w_dim.dim_y = attr_list[i].dim_y;

        if (attr_list[i].LongSeq.operator->() != NULL)
            attr_value_list[i].value.long_att_value(attr_list[i].LongSeq.in());
        else if (attr_list[i].Long64Seq.operator->() != NULL)
            attr_value_list[i].value.long64_att_value(attr_list[i].Long64Seq.in());
        else if (attr_list[i].ShortSeq.operator->() != NULL)
            attr_value_list[i].value.short_att_value(attr_list[i].ShortSeq.in());
        else if (attr_list[i].DoubleSeq.operator->() != NULL)
            attr_value_list[i].value.double_att_value(attr_list[i].DoubleSeq.in());
        else if (attr_list[i].StringSeq.operator->() != NULL)
            attr_value_list[i].value.string_att_value(attr_list[i].StringSeq.in());
        else if (attr_list[i].FloatSeq.operator->() != NULL)
            attr_value_list[i].value.float_att_value(attr_list[i].FloatSeq.in());
        else if (attr_list[i].BooleanSeq.operator->() != NULL)
            attr_value_list[i].value.bool_att_value(attr_list[i].BooleanSeq.in());
        else if (attr_list[i].UShortSeq.operator->() != NULL)
            attr_value_list[i].value.ushort_att_value(attr_list[i].UShortSeq.in());
        else if (attr_list[i].UCharSeq.operator->() != NULL)
            attr_value_list[i].value.uchar_att_value(attr_list[i].UCharSeq.in());
        else if (attr_list[i].ULongSeq.operator->() != NULL)
            attr_value_list[i].value.ulong_att_value(attr_list[i].ULongSeq.in());
        else if (attr_list[i].ULong64Seq.operator->() != NULL)
            attr_value_list[i].value.ulong64_att_value(attr_list[i].ULong64Seq.in());
        else if (attr_list[i].StateSeq.operator->() != NULL)
            attr_value_list[i].value.state_att_value(attr_list[i].StateSeq.in());
        else if (attr_list[i].EncodedSeq.operator->() != NULL)
            attr_value_list[i].value.encoded_att_value(attr_list[i].EncodedSeq.in());
    }

//
// Create remaining parameter
//

    Tango::DevVarStringArray dvsa;
    dvsa << r_names;

//
// Call device
//

    int ctr = 0;
    AttributeValueList_5_var attr_value_list_5;
    Tango::AccessControlType local_act;

    while (ctr < 2)
    {
        try
        {
            check_and_reconnect(local_act);

//
// Throw exception if caller not allowed to write_attribute
//

            if (local_act == ACCESS_READ)
            {
                try
                {
                    Device_var dev = Device::_duplicate(device);
                    dev->ping();
                }
                catch (...)
                {
                    set_connection_state(CONNECTION_NOTOK);
                    throw;
                }

                TangoSys_OMemStream desc;
                desc << "Writing attribute(s) on device " << dev_name() << " is not authorized" << ends;

                NotAllowedExcept::throw_exception((const char *) API_ReadOnlyMode, desc.str(),
                                                  (const char *) "DeviceProxy::write_read_attribute()");
            }

//
// Now, call the server
//

            ClntIdent ci;
            ApiUtil *au = ApiUtil::instance();
            ci.cpp_clnt(au->get_client_pid());

            Device_5_var dev = Device_5::_duplicate(device_5);
            attr_value_list_5 = dev->write_read_attributes_5(attr_value_list, dvsa, ci);

            ctr = 2;
        }
        catch (Tango::MultiDevFailed &e)
        {

//
// Transfer this exception into a DevFailed exception
//

            Tango::DevFailed ex(e.errors[0].err_list);
            TangoSys_OMemStream desc;
            desc << "Failed to write_read_attributes on device " << device_name;
            desc << ", attribute ";
            desc << attr_value_list[0].name.in();
            desc << ends;
            Except::re_throw_exception(ex, (const char *) API_AttributeFailed,
                                       desc.str(), (const char *) "DeviceProxy::write_read_attributes()");

        }
        catch (Tango::DevFailed &e)
        {
            TangoSys_OMemStream desc;
            desc << "Failed to write_read_attributes on device " << device_name;
            desc << ", attribute ";
            desc << attr_value_list[0].name.in();
            desc << ends;

            if (::strcmp(e.errors[0].reason, DEVICE_UNLOCKED_REASON) == 0)
                DeviceUnlockedExcept::re_throw_exception(e, (const char *) DEVICE_UNLOCKED_REASON,
                                                         desc.str(),
                                                         (const char *) "DeviceProxy::write_read_attributes()");
            else
                Except::re_throw_exception(e, (const char *) API_AttributeFailed,
                                           desc.str(), (const char *) "DeviceProxy::write_read_attributes()");
        }
        catch (CORBA::TRANSIENT &trans)
        {
            TRANSIENT_NOT_EXIST_EXCEPT(trans, "DeviceProxy", "write_read_attributes()", this);
        }
        catch (CORBA::OBJECT_NOT_EXIST &one)
        {
            if (one.minor() == omni::OBJECT_NOT_EXIST_NoMatch || one.minor() == 0)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(one, "DeviceProxy", "write_read_attributes", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute write_read_attributes on device " << device_name << ends;
                ApiCommExcept::re_throw_exception(one,
                                                  (const char *) "API_CommunicationFailed",
                                                  desc.str(),
                                                  (const char *) "DeviceProxy::write_read_attributes()");
            }
        }
        catch (CORBA::COMM_FAILURE &comm)
        {
            if (comm.minor() == omni::COMM_FAILURE_WaitingForReply)
            {
                TRANSIENT_NOT_EXIST_EXCEPT(comm, "DeviceProxy", "write_read_attributes", this);
            }
            else
            {
                set_connection_state(CONNECTION_NOTOK);
                TangoSys_OMemStream desc;
                desc << "Failed to execute write_attributes on device " << device_name << ends;
                ApiCommExcept::re_throw_exception(comm,
                                                  (const char *) "API_CommunicationFailed",
                                                  desc.str(),
                                                  (const char *) "DeviceProxy::write_read_attributes()");
            }
        }
        catch (CORBA::SystemException &ce)
        {
            set_connection_state(CONNECTION_NOTOK);

            TangoSys_OMemStream desc;
            desc << "Failed to execute write_read_attributes on device " << device_name << ends;
            ApiCommExcept::re_throw_exception(ce,
                                              (const char *) "API_CommunicationFailed",
                                              desc.str(),
                                              (const char *) "DeviceProxy::write_read_attributes()");
        }
    }

//
// Init the returned DeviceAttribute vector

    unsigned long nb_received;
    nb_received = attr_value_list_5->length();

    vector<DeviceAttribute> *dev_attr = new(vector<DeviceAttribute>);
    dev_attr->resize(nb_received);

    for (unsigned int i = 0; i < nb_received; i++)
    {
        ApiUtil::attr_to_device(&(attr_value_list_5[i]), 5, &(*dev_attr)[i]);

//
// Add an error in the error stack in case there is one
//

        DevErrorList_var &err_list = (*dev_attr)[i].get_error_list();
        long nb_except = err_list.in().length();
        if (nb_except != 0)
        {
            TangoSys_OMemStream desc;
            desc << "Failed to write_read_attribute on device " << device_name;
            desc << ", attribute " << (*dev_attr)[i].name << ends;

            err_list.inout().length(nb_except + 1);
            err_list[nb_except].reason = Tango::string_dup(API_AttributeFailed);
            err_list[nb_except].origin = Tango::string_dup("DeviceProxy::write_read_attributes()");

            string st = desc.str();
            err_list[nb_except].desc = Tango::string_dup(st.c_str());
            err_list[nb_except].severity = Tango::ERR;
        }
    }

    return (dev_attr);
}

//-----------------------------------------------------------------------------
//
// method : 		DeviceProxy::same_att_name()
//
// description : 	Check if in the attribute name list there is not several
//					times the same attribute. Throw exception in case of
//
// argin(s) :		attr_list : The attribute name(s) list
//					met_name : The calling method name (for exception)
//
//-----------------------------------------------------------------------------


void DeviceProxy::same_att_name(vector<string> &attr_list, const char *met_name)
{
    if (attr_list.size() > 1)
    {
        unsigned int i;
        vector<string> same_att = attr_list;

        for (i = 0; i < same_att.size(); ++i)
            transform(same_att[i].begin(), same_att[i].end(), same_att[i].begin(), ::tolower);
        sort(same_att.begin(), same_att.end());
        vector<string> same_att_lower = same_att;

        vector<string>::iterator pos = unique(same_att.begin(), same_att.end());

        int duplicate_att;
        duplicate_att = distance(attr_list.begin(), attr_list.end()) - distance(same_att.begin(), pos);

        if (duplicate_att != 0)
        {
            TangoSys_OMemStream desc;
            desc << "Several times the same attribute in required attributes list: ";
            int ctr = 0;
            for (i = 0; i < same_att_lower.size() - 1; i++)
            {
                if (same_att_lower[i] == same_att_lower[i + 1])
                {
                    ctr++;
                    desc << same_att_lower[i];
                    if (ctr < duplicate_att)
                        desc << ", ";
                }
            }
            desc << ends;
            ApiConnExcept::throw_exception((const char *) API_AttributeFailed, desc.str(), met_name);
        }
    }
}

//-----------------------------------------------------------------------------
//
// DeviceProxy::local_import() - If the device is embedded within the same
// process, re-create its IOR and returns it. This save one DB call.
//
//-----------------------------------------------------------------------------

void DeviceProxy::local_import(string &local_ior)
{
    Tango::Util *tg = NULL;

//
// In case of controlled access used, this method is called while the
// Util object is still in its construction case.
// Catch this exception and return from this method in this case
//

    try
    {
        tg = Tango::Util::instance(false);
    }
    catch (Tango::DevFailed &e)
    {
        string reas(e.errors[0].reason);
        if (reas == "API_UtilSingletonNotCreated")
            return;
    }

    const vector<Tango::DeviceClass *> *cl_list_ptr = tg->get_class_list();
    for (unsigned int loop = 0; loop < cl_list_ptr->size(); loop++)
    {
        Tango::DeviceClass *cl_ptr = (*cl_list_ptr)[loop];
        vector<Tango::DeviceImpl *> dev_list = cl_ptr->get_device_list();
        for (unsigned int lo = 0; lo < dev_list.size(); lo++)
        {
            if (dev_list[lo]->get_name_lower() == device_name)
            {
                if (Tango::Util::_UseDb == true)
                {
                    Database *db = tg->get_database();
                    if (db->get_db_host() != get_db_host())
                        return;
                }

                Tango::Device_var d_var = dev_list[lo]->get_d_var();
                CORBA::ORB_ptr orb_ptr = tg->get_orb();

                char *s = orb_ptr->object_to_string(d_var);
                local_ior = s;

                CORBA::release(orb_ptr);
                CORBA::string_free(s);

                return;
            }
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
//
// method:
//		DeviceProxy::get_tango_lib_version()
//
// description:
//		Returns the Tango lib version number used by the remote device
//
// return:
//		The device Tango lib version as a 3 or 4 digits number
//		Possible return value are: 100,200,500,520,700,800,810,...
//
//---------------------------------------------------------------------------------------------------------------------

int DeviceProxy::get_tango_lib_version()
{
    int ret = 0;

    check_connect_adm_device();

//
// Get admin device IDL release and command list
//

    int admin_idl_vers = adm_device->get_idl_version();
    Tango::CommandInfoList *cmd_list;
    cmd_list = adm_device->command_list_query();

    switch (admin_idl_vers)
    {
        case 1:
            ret = 100;
            break;

        case 2:
            ret = 200;
            break;

        case 3:
        {

//
// IDL 3 is for Tango 5 and 6. Unfortunately, there is no way from the client side to determmine if it is
// Tango 5 or 6. The beast we can do is to get the info that it is Tango 5.2 (or above)
//

#ifdef HAS_LAMBDA_FUNC
            auto pos = find_if((*cmd_list).begin(), (*cmd_list).end(),
                               [](Tango::CommandInfo &cc) -> bool
                               {
                                   return cc.cmd_name == "QueryWizardClassProperty";
                               });
#else
                                                                                                                                    vector<CommandInfo>::iterator pos,end;
		for (pos = (*cmd_list).begin(), end = (*cmd_list).end();pos != end;++pos)
		{
			if (pos->cmd_name == "QueryWizardClassProperty")
				break;
		}
#endif
            if (pos != (*cmd_list).end())
                ret = 520;
            else
                ret = 500;
            break;
        }

        case 4:
        {

//
// IDL 4 is for Tango 7 and 8.
//

            bool ecs = false;
            bool zesc = false;

#ifdef HAS_RANGE_BASE_FOR
            for (const auto &cmd : *cmd_list)
            {
                if (cmd.cmd_name == "EventConfirmSubscription")
                {
                    ecs = true;
                    break;
                }

                if (cmd.cmd_name == "ZmqEventSubscriptionChange")
                    zesc = true;
            }
#else
                                                                                                                                    vector<CommandInfo>::iterator pos,pos_end;
		for (pos = (*cmd_list).begin(), pos_end = (*cmd_list).end();pos != pos_end;++pos)
		{
			if (pos->cmd_name == "EventConfirmSubscription")
			{
				ecs = true;
				break;
			}

			if  (pos->cmd_name == "ZmqEventSubscriptionChange")
				zesc = true;
		}
#endif
            if (ecs == true)
                ret = 810;
            else if (zesc == true)
                ret = 800;
            else
                ret = 700;

            break;
        }

        case 5:
            ret = 902;
            break;

            //TODO extract class hierarchy based on version!!!
        case 6:
            ret = 1001;
            break;

        default:
            break;
    }

    delete cmd_list;

    return ret;
}

} // End of Tango namespace
