#ifndef _broker_LinkRegistry_h
#define _broker_LinkRegistry_h

/*
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 *
 */

#include <map>
#include "qpid/broker/BrokerImportExport.h"
#include "qpid/broker/Bridge.h"
#include "qpid/broker/MessageStore.h"
#include "qpid/Address.h"
#include "qpid/sys/Mutex.h"
#include "qpid/management/Manageable.h"
#include <boost/shared_ptr.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/function.hpp>

namespace qpid {
namespace broker {

    class Link;
    class Broker;
    class Connection;
    class LinkRegistry {
        typedef std::map<std::string, boost::shared_ptr<Link> > LinkMap;
        typedef std::map<std::string, Bridge::shared_ptr> BridgeMap;
        typedef std::map<std::string, std::string> ConnectionMap;

        LinkMap   links;    /** indexed by name of Link */
        BridgeMap bridges;  /** indexed by name of Bridge */
        ConnectionMap   connections;  /** indexed by connection identifier, gives link name */

        qpid::sys::Mutex lock;
        Broker* broker;
        management::Manageable* parent;
        MessageStore* store;
        bool passive;
        std::string realm;

        boost::shared_ptr<Link> findLink(const std::string& key);

        // Methods called by the connection observer, key is connection identifier
        void notifyConnection (const std::string& key, Connection* c);
        void notifyOpened     (const std::string& key);
        void notifyClosed     (const std::string& key);
        void notifyConnectionForced    (const std::string& key, const std::string& text);
        friend class LinkRegistryConnectionObserver;

        /** Notify the registry that a Link has been destroyed */
        void linkDestroyed(Link*);
        /** Request to destroy a Bridge */
        void destroyBridge(Bridge*);

    public:
        QPID_BROKER_EXTERN LinkRegistry (); // Only used in store tests
        QPID_BROKER_EXTERN LinkRegistry (Broker* _broker);
        QPID_BROKER_EXTERN ~LinkRegistry();

        QPID_BROKER_EXTERN std::pair<boost::shared_ptr<Link>, bool>
        declare(const std::string& name,
                const std::string& host,
                uint16_t     port,
                const std::string& transport,
                bool         durable,
                const std::string& authMechanism,
                const std::string& username,
                const std::string& password);
        /** determine if Link exists */
        QPID_BROKER_EXTERN boost::shared_ptr<Link>
          getLink(const std::string& name);
        QPID_BROKER_EXTERN boost::shared_ptr<Link>
          getLink(const std::string& host,
                  uint16_t           port,
                  const std::string& transport = std::string());

        QPID_BROKER_EXTERN std::pair<Bridge::shared_ptr, bool>
        declare(const std::string& name,
                Link& link,
                bool         durable,
                const std::string& src,
                const std::string& dest,
                const std::string& key,
                bool         isQueue,
                bool         isLocal,
                const std::string& id,
                const std::string& excludes,
                bool         dynamic,
                uint16_t     sync,
                Bridge::InitializeCallback=0
        );
        /** determine if Bridge exists */
        QPID_BROKER_EXTERN Bridge::shared_ptr
          getBridge(const std::string&  name);
        QPID_BROKER_EXTERN Bridge::shared_ptr
          getBridge(const Link&  link,
                    const std::string& src,
                    const std::string& dest,
                    const std::string& key);

        /**
         * Register the manageable parent for declared queues
         */
        void setParent (management::Manageable* _parent) { parent = _parent; }

        /**
         * Set the store to use.  May only be called once.
         */
        QPID_BROKER_EXTERN void setStore (MessageStore*);

        /**
         * Return the message store used.
         */
        QPID_BROKER_EXTERN MessageStore* getStore() const;

        QPID_BROKER_EXTERN std::string getAuthMechanism   (const std::string& key);
        QPID_BROKER_EXTERN std::string getAuthCredentials (const std::string& key);
        QPID_BROKER_EXTERN std::string getAuthIdentity    (const std::string& key);
        QPID_BROKER_EXTERN std::string getUsername        (const std::string& key);
        QPID_BROKER_EXTERN std::string getPassword        (const std::string& key);
        QPID_BROKER_EXTERN std::string getHost            (const std::string& key);
        QPID_BROKER_EXTERN uint16_t    getPort            (const std::string& key);

        /**
         * Called to alter passive state. In passive state the links
         * and bridges managed by a link registry will be recorded and
         * updated but links won't actually establish connections and
         * bridges won't therefore pull or push any messages.
         */
        QPID_BROKER_EXTERN void setPassive(bool);
        QPID_BROKER_EXTERN bool isPassive() { return passive; }

        /** Iterate over each link in the registry. Used for cluster updates. */
        QPID_BROKER_EXTERN void eachLink(boost::function<void(boost::shared_ptr<Link>)> f);
        /** Iterate over each bridge in the registry. Used for cluster updates. */
        QPID_BROKER_EXTERN void eachBridge(boost::function<void(boost::shared_ptr< Bridge>)> f);
    };
}
}


#endif  /*!_broker_LinkRegistry_h*/
