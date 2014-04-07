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
package org.apache.qpid.server.security;

import org.apache.qpid.server.model.AbstractConfiguredObjectTypeFactory;
import org.apache.qpid.server.model.Broker;
import org.apache.qpid.server.model.ConfiguredObject;

import java.util.HashMap;
import java.util.Map;
import java.util.UUID;

public class FileTrustStoreFactory extends AbstractConfiguredObjectTypeFactory<FileTrustStore>
{
    public FileTrustStoreFactory()
    {
        super(FileTrustStore.class);
    }

    protected final Broker getBroker(ConfiguredObject<?>... parents)
    {
        if(parents.length != 1 && !(parents[0] instanceof Broker))
        {
            throw new IllegalArgumentException("Should have exactly one parent of type broker");
        }
        return (Broker) parents[0];
    }

    @Override
    public FileTrustStore createInstance(final Map<String, Object> attributes, final ConfiguredObject<?>... parents)
    {
        HashMap<String, Object> attributesWithoutId = new HashMap<String, Object>(attributes);
        Object idObj = attributesWithoutId.remove(ConfiguredObject.ID);
        UUID id = idObj == null ? UUID.randomUUID() : idObj instanceof UUID ? (UUID) idObj : UUID.fromString(idObj.toString());
        return new FileTrustStore(id, getParent(Broker.class, parents), attributesWithoutId);
    }

}