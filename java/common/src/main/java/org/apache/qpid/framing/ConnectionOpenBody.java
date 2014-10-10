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

public class ConnectionOpenBody extends AMQMethodBodyImpl implements EncodableAMQDataBlock, AMQMethodBody
{

    public static final int CLASS_ID =  10;
    public static final int METHOD_ID = 40;

    // Fields declared in specification
    private final AMQShortString _virtualHost; // [virtualHost]
    private final AMQShortString _capabilities; // [capabilities]
    private final boolean _insist; // [insist]

    // Constructor
    public ConnectionOpenBody(MarkableDataInput buffer) throws AMQFrameDecodingException, IOException
    {
        _virtualHost = buffer.readAMQShortString();
        _capabilities = buffer.readAMQShortString();
        _insist = (buffer.readByte() & 0x01) == 0x01;
    }

    public ConnectionOpenBody(
            AMQShortString virtualHost,
            AMQShortString capabilities,
            boolean insist
                             )
    {
        _virtualHost = virtualHost;
        _capabilities = capabilities;
        _insist = insist;
    }

    public int getClazz()
    {
        return CLASS_ID;
    }

    public int getMethod()
    {
        return METHOD_ID;
    }

    public final AMQShortString getVirtualHost()
    {
        return _virtualHost;
    }
    public final AMQShortString getCapabilities()
    {
        return _capabilities;
    }
    public final boolean getInsist()
    {
        return _insist;
    }

    protected int getBodySize()
    {
        int size = 1;
        size += getSizeOf( _virtualHost );
        size += getSizeOf( _capabilities );
        return size;
    }

    public void writeMethodPayload(DataOutput buffer) throws IOException
    {
        writeAMQShortString( buffer, _virtualHost );
        writeAMQShortString( buffer, _capabilities );
        writeBitfield( buffer, _insist ? (byte)1 : (byte)0);
    }

    public boolean execute(MethodDispatcher dispatcher, int channelId) throws AMQException
	{
        return dispatcher.dispatchConnectionOpen(this, channelId);
	}

    public String toString()
    {
        StringBuilder buf = new StringBuilder("[ConnectionOpenBodyImpl: ");
        buf.append( "virtualHost=" );
        buf.append(  getVirtualHost() );
        buf.append( ", " );
        buf.append( "capabilities=" );
        buf.append(  getCapabilities() );
        buf.append( ", " );
        buf.append( "insist=" );
        buf.append(  getInsist() );
        buf.append("]");
        return buf.toString();
    }

    public static void process(final MarkableDataInput buffer, final MethodProcessor dispatcher) throws IOException
    {

        AMQShortString virtualHost = buffer.readAMQShortString();
        AMQShortString capabilities = buffer.readAMQShortString();
        boolean insist = (buffer.readByte() & 0x01) == 0x01;
        dispatcher.receiveConnectionOpen(virtualHost, capabilities, insist);
    }
}
