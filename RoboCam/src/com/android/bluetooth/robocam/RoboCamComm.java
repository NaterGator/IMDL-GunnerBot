package com.android.bluetooth.robocam;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.UUID;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.content.Context;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;

public class RoboCamComm {
	private static final UUID MY_UUID = UUID.fromString("00001101-0000-1000-8000-00805f9b34fb");
	
	public static final int STATE_DISCONNECTED = 0;
	public static final int STATE_CONNECTING = 1;
	public static final int STATE_CONNECTED = 2;
	
    private final BluetoothAdapter mAdapter;
    private final Handler mHandler;
    private ConnectThread mConnectThread;
    private ConnectedThread mConnectedThread;
    private int mState;
	
    /* Construct the BluetoothComm Object
     * 
     */
	public RoboCamComm(Context context, Handler handler) {
		mAdapter = BluetoothAdapter.getDefaultAdapter();
		mState = STATE_DISCONNECTED;
		mHandler = handler;
	}
	
    public synchronized int getState() {
        return mState;
    }
    
    private synchronized void setState(int state) {
    	mState = state;
    	
    	//return the state to the Testing class so it knows a change happened
    	mHandler.obtainMessage(RoboCam.MESSAGE_STATE_CHANGE, state, -1).sendToTarget();
    }

    
    /* Establish a connection with a paired device
     * 
     * Sup
     */
    public synchronized void connect(BluetoothDevice device) {

        // Cancel any thread attempting to make a connection
        if (mState == STATE_CONNECTING) {
            if (mConnectThread != null) {mConnectThread.cancel(); mConnectThread = null;}
        }

        // Cancel any thread currently running a connection
        if (mConnectedThread != null) {mConnectedThread.cancel(); mConnectedThread = null;}

        // Start the thread to connect with the given device
        mConnectThread = new ConnectThread(device);
        mConnectThread.start();
        setState(STATE_CONNECTING);
    }
	
    /**
     * Start the ConnectedThread to begin managing a Bluetooth connection
     * @param socket  The BluetoothSocket on which the connection was made
     * @param device  The BluetoothDevice that has been connected
     */
    public synchronized void connected(BluetoothSocket socket, BluetoothDevice device) {

        // Cancel the thread that completed the connection
        if (mConnectThread != null) {mConnectThread.cancel(); mConnectThread = null;}

        // Cancel any thread currently running a connection
        if (mConnectedThread != null) {mConnectedThread.cancel(); mConnectedThread = null;}


        // Start the thread to manage the connection and perform transmissions
        mConnectedThread = new ConnectedThread(socket);
        mConnectedThread.start();

        // TODO: Send the name of the connected device back to the UI Activity
        /*Message msg = mHandler.obtainMessage(BluetoothTesting.MESSAGE_DEVICE_NAME);
        Bundle bundle = new Bundle();
        bundle.putString(BluetoothTesting.DEVICE_NAME, device.getName());
        msg.setData(bundle);
        mHandler.sendMessage(msg);*/

        setState(STATE_CONNECTED);
    }
    
    /**
     * Stop all threads
     */
    public synchronized void stop() {
        if (mConnectThread != null) {mConnectThread.cancel(); mConnectThread = null;}
        if (mConnectedThread != null) {mConnectedThread.cancel(); mConnectedThread = null;}
        setState(STATE_DISCONNECTED);
    }
    
    /**
     * Write to the ConnectedThread in an unsynchronized manner
     * @param out The bytes to write
     * @see ConnectedThread#write(byte[])
     */
    public void write(byte[] out) {
        // Create temporary object
        ConnectedThread r;
        // Synchronize a copy of the ConnectedThread
        synchronized (this) {
            if (mState != STATE_CONNECTED) return;
            r = mConnectedThread;
        }
        // Perform the write unsynchronized
        r.write(out);
    }

    

    /**
     * Indicate that the connection attempt failed and notify the UI Activity.
     */
    private void connectionFailed() {
        setState(STATE_DISCONNECTED);

        // Send a failure message back to the Activity
        Message msg = mHandler.obtainMessage(RoboCam.MESSAGE_TOAST);
        Bundle bundle = new Bundle();
        bundle.putString(RoboCam.TOAST, "Unable to connect device");
        msg.setData(bundle);
        mHandler.sendMessage(msg);
    }

    /**
     * Indicate that the connection was lost and notify the UI Activity.
     */
    private void connectionLost() {
        setState(STATE_DISCONNECTED);

        // Send a failure message back to the Activity
        Message msg = mHandler.obtainMessage(RoboCam.MESSAGE_TOAST);
        Bundle bundle = new Bundle();
        bundle.putString(RoboCam.TOAST, "Device connection was lost");
        msg.setData(bundle);
        mHandler.sendMessage(msg);
    }
    
    
    /* Try to make an outgoing connection attempt with a device
     * 
     */
    private class ConnectThread extends Thread {
        private final BluetoothSocket mmSocket;
        private final BluetoothDevice mmDevice;

        public ConnectThread(BluetoothDevice device) {
            mmDevice = device;
            BluetoothSocket tmp = null;

            // Get a BluetoothSocket for a connection with the
            // given BluetoothDevice
            try {
                tmp = device.createRfcommSocketToServiceRecord(MY_UUID);
            } catch (IOException e) {
                
            }
            mmSocket = tmp;
        }

        public void run() {
            setName("ConnectThread");

            // Always cancel discovery because it will slow down a connection
            mAdapter.cancelDiscovery();

            // Make a connection to the BluetoothSocket
            try {
                // This is a blocking call and will only return on a
                // successful connection or an exception
                mmSocket.connect();
            } catch (IOException e) {
                connectionFailed();
                // Close the socket
                try {
                    mmSocket.close();
                } catch (IOException e2) {
                    
                }
                return;
            }

            // Reset the ConnectThread because we're done connecting
            synchronized (RoboCamComm.this) {
                mConnectThread = null;
            }

            // Start the connected thread
            connected(mmSocket, mmDevice);
        }

        public void cancel() {
            try {
                mmSocket.close();
            } catch (IOException e) {

            }
        }
    }

    /**
     * This thread runs during a connection with a remote device.
     * It handles all incoming and outgoing transmissions.
     */
    private class ConnectedThread extends Thread {
        private final BluetoothSocket mmSocket;
        private final InputStream mmInStream;
        private final OutputStream mmOutStream;

        public ConnectedThread(BluetoothSocket socket) {
            mmSocket = socket;
            InputStream tmpIn = null;
            OutputStream tmpOut = null;

            // Get the BluetoothSocket input and output streams
            try {
                tmpIn = socket.getInputStream();
                tmpOut = socket.getOutputStream();
            } catch (IOException e) {
                
            }

            mmInStream = tmpIn;
            mmOutStream = tmpOut;
        }

        public void run() {
            
            byte[] buffer = new byte[5000];
            int bytes;

            // Keep listening to the InputStream for a command page
            while (true) {
                try {
                    // Read from the InputStream
                	StringBuffer dataIn = new StringBuffer();
                	while(true) {
                		int startloc = dataIn.indexOf("$FRAMESTART$");
                		if(startloc!=-1) {
                			int endloc = dataIn.indexOf("$FRAMEEND$", startloc);
                			if(endloc!=-1) {
                				String astring = dataIn.substring(startloc+12, endloc);
                				dataIn = new StringBuffer(astring);
                				break;
                			}
                		}
                		bytes = mmInStream.read(buffer);
                		dataIn.append(new String(buffer, 0, bytes));
                	}
                     //Send the obtained bytes to the UI Activity
                    mHandler.obtainMessage(RoboCam.MESSAGE_DATA_IN, -1, -1, dataIn).sendToTarget();
                } catch (IOException e) {

                    connectionLost();
                    break;
                }
            }
        }

        /**
         * Write to the connected OutStream.
         * @param buffer  The bytes to write
         */
        public void write(byte[] buffer) {
            try {
                mmOutStream.write(buffer);
//do nothing
                // Share the sent message back to the UI Activity
            //    mHandler.obtainMessage(BluetoothTesting.MESSAGE_WRITE, -1, -1, buffer)
              //          .sendToTarget();
            } catch (IOException e) {

            }
        }

        public void cancel() {
            try {
                mmSocket.close();
            } catch (IOException e) {

            }
        }
    }
}
