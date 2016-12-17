package com.jfc.srvc.ble2cld;

import java.nio.charset.Charset;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.atomic.AtomicBoolean;

import com.jfc.misc.prop.IPropertyMgr;
import com.jfc.srvc.ble2cld.BleGattExecutor.BleExecutorListener;

import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothGattService;
import android.util.Log;

public class BluetoothPipeSrvcBackup {
	private static final String TAG = BluetoothPipeSrvc.class.getName();

    public static final int kTxMaxCharacters = 20;
    public static final String UUID_TX = "6e400002-b5a3-f393-e0a9-e50e24dcca9e";
    public static final String UUID_RX = "6e400003-b5a3-f393-e0a9-e50e24dcca9e";
    public static final String UUID_UART_SERVICE = "6e400001-b5a3-f393-e0a9-e50e24dcca9e";


	private Activity mActivity = null;
	private Map<String,BluetoothDevice> mDevices = new HashMap<String,BluetoothDevice>();
	private AtomicBoolean mShutdown = new AtomicBoolean(false);
	
	public BluetoothPipeSrvcBackup(Activity activity) {
		mActivity = activity;
	}
	
	public boolean addDevice(BluetoothDevice device) {
		if (!mDevices.containsKey(device.getAddress())) {
			mDevices.put(device.getAddress(), device);
//			connect(device);
			connect("DD:D1:09:0A:1A:0B");
		}
		return true;
	}
	
    public class MyBleExecutorListener implements BleExecutorListener {
    	
        private final BleGattExecutor mExecutor = BleGattExecutor.createExecutor(this);
        private StringBuffer rxLine = new StringBuffer();
        
        public BleGattExecutor getExecutor() {return mExecutor;}
        
		@Override
		public void onServicesDiscovered(BluetoothGatt gatt, int status) {
    		Log.d(TAG, "onServicesDiscovered; status="+status);
            final UUID serviceUuid = UUID.fromString(UUID_UART_SERVICE);
            BluetoothGattService uartService = gatt.getService(serviceUuid);
            mExecutor.enableNotification(uartService, UUID_RX, true);
            mExecutor.execute(gatt);
		}
		@Override
		public void onReadRemoteRssi(BluetoothGatt gatt, int rssi, int status) {
    		Log.e(TAG, "onReadRemoteRssi");
		}
		@Override
		public void onDescriptorRead(BluetoothGatt gatt, BluetoothGattDescriptor descriptor, int status) {
    		Log.e(TAG, "onDescriptorRead");
		}
		@Override
		public void onConnectionStateChange(BluetoothGatt gatt, int status, int newState) {
    		if (newState == BluetoothGatt.STATE_CONNECTED) {
        		Log.d(TAG, "onConnectionStateChange; CONNECTED");
                // Attempts to discover services after successful connection.
                gatt.discoverServices();
    		} else if (newState == BluetoothGatt.STATE_DISCONNECTED) {
        		Log.d(TAG, "onConnectionStateChange; DISCONNECTED");
    		} else 
        		Log.e(TAG, "onConnectionStateChange; status="+status+" new="+newState);
		}
		@Override
		public void onCharacteristicRead(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, int status) {
    		Log.e(TAG, "onCharacteristicRead");
		}
		@Override
		public void onCharacteristicChanged(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic) {
	        if (characteristic.getService().getUuid().toString().equalsIgnoreCase(UUID_UART_SERVICE)) {
	            if (characteristic.getUuid().toString().equalsIgnoreCase(UUID_RX)) {
	            	final byte[] bytes = characteristic.getValue();
	            	Log.i(TAG, "Have data: "+new String(bytes));
	            	rxLine.append(new String(bytes));
	            	int loc = rxLine.indexOf("\r");
	            	if (loc == -1) loc = rxLine.indexOf("\n");
	            	if (loc != -1) {
	            		// then I have a valid line
	            		String cmd = rxLine.substring(0, loc);
	            		rxLine.delete(0,loc+1);
	            		Log.i(TAG, "sending line off for processing: '"+cmd+"'");
	            		if (rxLine.length() == 0) 
	            			Log.i(TAG, "buffer is now empty");
	            		else 
	            			Log.i(TAG, "buffer now contains: '"+rxLine.toString()+"'");
	            		
	                    CmdOnCompletion onCompletion = new CmdOnCompletion() {
	                        @Override
	                        public void complete(String reply) {
	                            if (!queueConsumerStarted) {
	                                queueConsumer.start();
	                                queueConsumerStarted = true;
	                            }
	                            Log.i(TAG, "queuing: "+reply);
	                            queue.offer(reply);
	                        }
	                    };
	            		String results[] = new String[2];
	            		CmdProcess.process(cmd, results, onCompletion);
	            	}
	            } else Log.e(TAG, "onCharacteristicChangedCalled (1)");
	        } else Log.e(TAG, "onCharacteristicChangedCalled (2)");
		}
		

	    private BlockingQueue<String> queue = new LinkedBlockingQueue<String>();
	    private boolean queueConsumerStarted = false;
	    private Thread queueConsumer = new Thread(new Runnable() {
	        @Override
	        public void run() {
	            while (true) {
	                try {
	                    String s = queue.take();

	                    final byte[] value = s.getBytes(Charset.forName("UTF-8"));
	                    
	                    // Split the value into chunks (UART service has a maximum number of characters that can be written )
	                    for (int i = 0; i < value.length; i += kTxMaxCharacters) {
	                        final byte[] chunk = Arrays.copyOfRange(value, i, Math.min(i + kTxMaxCharacters, value.length));
//	                        mBleManager.writeService(mUartService, UUID_TX, chunk);
	                    }
	                    
//	                    sendData(value);
	                    try {
	                        Thread.sleep(5);
	                    } catch (InterruptedException e) {
	                        e.printStackTrace();
	                    }
	                } catch (InterruptedException e) {
	                    e.printStackTrace();
	                }
	            }
	        }
	    });
	};

    
    private void connect(final String address) {
        Runnable runnable = new Runnable() {
			@Override
			public void run() {
				final MyBleExecutorListener listener = new MyBleExecutorListener();
		        BluetoothDevice device = BluetoothAdapter.getDefaultAdapter().getRemoteDevice(address);
		        final BluetoothGatt gatt = device.connectGatt(mActivity, false, listener.getExecutor());
		        
		        while (!mShutdown.get()) {
		        	try {
						Thread.sleep(100);
					} catch (InterruptedException e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}
		        }
			}
		};
        Thread t = new Thread(runnable);
        t.start();
	}
	
    public void onPause() {
    	Log.i(TAG, "onPause");
    	
    	mActivity = null;
    }
    
}
