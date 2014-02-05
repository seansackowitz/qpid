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
package org.apache.qpid.server.queue;

import org.apache.qpid.server.subscription.Subscription;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.Iterator;
import java.util.concurrent.ConcurrentHashMap;


public class AssignedSubscriptionMessageGroupManager implements MessageGroupManager
{
    private static final Logger _logger = LoggerFactory.getLogger(AssignedSubscriptionMessageGroupManager.class);


    private final String _groupId;
    private final ConcurrentHashMap<Integer, QueueSubscription> _groupMap = new ConcurrentHashMap<Integer, QueueSubscription>();
    private final int _groupMask;

    public AssignedSubscriptionMessageGroupManager(final String groupId, final int maxGroups)
    {
        _groupId = groupId;
        _groupMask = pow2(maxGroups)-1;
    }

    private static int pow2(final int i)
    {
        int val = 1;
        while(val < i)
        {
            val<<=1;
        }
        return val;
    }

    public QueueSubscription getAssignedSubscription(final QueueEntry entry)
    {
        Object groupVal = entry.getMessage().getMessageHeader().getHeader(_groupId);
        return groupVal == null ? null : _groupMap.get(groupVal.hashCode() & _groupMask);
    }

    public boolean acceptMessage(QueueSubscription sub, QueueEntry entry)
    {
        if(assignMessage(sub, entry))
        {
            return entry.acquire(sub);
        }
        else
        {
            return false;
        }
    }

    private boolean assignMessage(QueueSubscription sub, QueueEntry entry)
    {
        Object groupVal = entry.getMessage().getMessageHeader().getHeader(_groupId);
        if(groupVal == null)
        {
            return true;
        }
        else
        {
            Integer group = groupVal.hashCode() & _groupMask;
            QueueSubscription assignedSub = _groupMap.get(group);
            if(assignedSub == sub)
            {
                return true;
            }
            else
            {
                if(assignedSub == null)
                {
                    if(_logger.isDebugEnabled())
                    {
                        _logger.debug("Assigning group " + groupVal + " to sub " + sub);
                    }
                    assignedSub = _groupMap.putIfAbsent(group, sub);
                    return assignedSub == null || assignedSub == sub;
                }
                else
                {
                    return false;
                }
            }
        }
    }
    
    public QueueEntry findEarliestAssignedAvailableEntry(QueueSubscription sub)
    {
        EntryFinder visitor = new EntryFinder(sub);
        sub.getQueue().visit(visitor);
        return visitor.getEntry();
    }

    private class EntryFinder implements QueueEntryVisitor
    {
        private QueueEntry _entry;
        private QueueSubscription _sub;

        public EntryFinder(final QueueSubscription sub)
        {
            _sub = sub;
        }

        public boolean visit(final QueueEntry entry)
        {
            if(!entry.isAvailable())
            {
                return false;
            }

            Object groupId = entry.getMessage().getMessageHeader().getHeader(_groupId);
            if(groupId == null)
            {
                return false;
            }

            Integer group = groupId.hashCode() & _groupMask;
            Subscription assignedSub = _groupMap.get(group);
            if(assignedSub == _sub)
            {
                _entry = entry;
                return true;
            }
            else
            {
                return false;
            }
        }

        public QueueEntry getEntry()
        {
            return _entry;
        }
    }

    public void clearAssignments(QueueSubscription sub)
    {
        Iterator<QueueSubscription> subIter = _groupMap.values().iterator();
        while(subIter.hasNext())
        {
            if(subIter.next() == sub)
            {
                subIter.remove();
            }
        }
    }
}