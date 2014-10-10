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

/*
 * This file is auto-generated by Qpid Gentools v.0.1 - do not modify.
 * Supported AMQP version:
 *   8-0
 */

package org.apache.qpid.framing;

import java.io.DataOutput;
import java.io.IOException;

import org.apache.qpid.AMQException;
import org.apache.qpid.codec.MarkableDataInput;

public class ChannelOpenBody extends AMQMethodBodyImpl implements EncodableAMQDataBlock, AMQMethodBody
{

    public static final int CLASS_ID =  20;
    public static final int METHOD_ID = 10;


    // Constructor
    public ChannelOpenBody(MarkableDataInput buffer) throws AMQFrameDecodingException, IOException
    {
        // ignore unused OOB string
        buffer.readAMQShortString();
    }

    public ChannelOpenBody()
    {

    }

    public int getClazz()
    {
        return CLASS_ID;
    }

    public int getMethod()
    {
        return METHOD_ID;
    }

    protected int getBodySize()
    {
        return 1;
    }

    public void writeMethodPayload(DataOutput buffer) throws IOException
    {
        writeAMQShortString( buffer, null );
    }

    public boolean execute(MethodDispatcher dispatcher, int channelId) throws AMQException
	{
        return dispatcher.dispatchChannelOpen(this, channelId);
	}

    public String toString()
    {
        return "[ChannelOpenBody] ";
    }

    public static void process(final int channelId,
                                final MarkableDataInput buffer,
                                final MethodProcessor dispatcher) throws IOException
    {
        buffer.readAMQShortString();
        dispatcher.receiveChannelOpen(channelId);
    }
}
