package com.jfc.srvc.ble2cld;

import java.nio.charset.Charset;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;

import com.example.hive.R;
import com.jfc.apps.hive.EnableBridgeProperty;
import com.jfc.apps.hive.MainActivity;
import com.jfc.apps.hive.NumHivesProperty;
import com.jfc.apps.hive.PairedHiveProperty;
import com.jfc.srvc.ble2cld.BleGattExecutor.BleExecutorListener;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothGattService;
import android.content.Context;
import android.content.Intent;
import android.graphics.drawable.Icon;
import android.os.Handler;
import android.os.IBinder;
import android.util.Log;

public class BluetoothPipeSrvc extends Service {
	private static final String TAG = BluetoothPipeSrvc.class.getName();

    public static final String UUID_TX = "6e400002-b5a3-f393-e0a9-e50e24dcca9e";
    public static final String UUID_RX = "6e400003-b5a3-f393-e0a9-e50e24dcca9e";
    public static final String UUID_UART_SERVICE = "6e400001-b5a3-f393-e0a9-e50e24dcca9e";
    public static final int kTxMaxCharacters = 20;

    private boolean mEnabled = false, mInitialized = false;
    private Notification mNotification;
    private List<String> mAddresses = new ArrayList<String>();
    private List<ConnectionMgr> mMgrs = new ArrayList<ConnectionMgr>();

    public class ConnectionMgr implements BleExecutorListener {
    	private String mAddress;
    	private BluetoothAdapter mAdapter = null;	
    	private BluetoothDevice mDevice = null;
    	private AtomicBoolean mShutdown = new AtomicBoolean(false), mIsConnected = new AtomicBoolean(false);
    	private AtomicBoolean mStopped = new AtomicBoolean(false);
        private StringBuffer rxLine = new StringBuffer();
    	private BleGattExecutor mExecutor = null;
    	private BluetoothGattService mUartService = null;
    	private BluetoothGatt mGatt = null;
        private boolean queueConsumerStarted = false;
    	
    	public ConnectionMgr(String address) {
    		mAddress = address;
    		connect(mAddress);
    	}
    	
    	public String getAddress() {return mAddress;}
    	public boolean isConnected() {
    		return mIsConnected.get();
    	}
    	
    	public void setShutdown(boolean v) {
    		mShutdown.set(v);
    		if (v && (mGatt != null)) {
    			mGatt.disconnect();
    			mGatt.close();
    		}
    	}
    	
    	public boolean isStopped() {
    		return mStopped.get();
    	}
    	
		@Override
		public void onConnectionStateChange(BluetoothGatt gatt, int status, int newState) {
			if (newState == BluetoothGatt.STATE_CONNECTED) {
	    		Log.d(TAG, "onConnectionStateChange; CONNECTED");
	            // Attempts to discover services after successful connection.
	    		boolean wasConnected = mIsConnected.getAndSet(true);
	            gatt.discoverServices();
	    		if (!wasConnected) 
	    			updateNotification();
			} else if (newState == BluetoothGatt.STATE_DISCONNECTED) {
	    		Log.d(TAG, "onConnectionStateChange; DISCONNECTED");
	    		mExecutor.clear();

	    		boolean wasConnected = mIsConnected.getAndSet(false);
	    		if (wasConnected)
	    			updateNotification();
			} else 
	    		Log.e(TAG, "onConnectionStateChange; status="+status+" new="+newState);
		}

		@Override
		public void onServicesDiscovered(BluetoothGatt gatt, int status) {
			if (mExecutor != null) {
	    		Log.d(TAG, "onServicesDiscovered; status="+status);
	            final UUID serviceUuid = UUID.fromString(UUID_UART_SERVICE);
	            mUartService = gatt.getService(serviceUuid);
	            if (mUartService != null) {
		            mExecutor.enableNotification(mUartService, UUID_RX, true);
		            mExecutor.execute(gatt);
	            }
			}
		}

		@Override
		public void onCharacteristicRead(BluetoothGatt gatt,
				BluetoothGattCharacteristic characteristic, int status) {
			Log.e(TAG, "onCharacteristicRead");
		}

		@Override
		public void onCharacteristicChanged(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic) {
	        if (characteristic.getService().getUuid().toString().equalsIgnoreCase(UUID_UART_SERVICE)) {
	            if (characteristic.getUuid().toString().equalsIgnoreCase(UUID_RX)) {
	            	final byte[] bytes = characteristic.getValue();
	            	rxLine.append(new String(bytes));
	            	int loc = rxLine.indexOf("\r");
	            	if (loc == -1) loc = rxLine.indexOf("\n");
	            	if (loc != -1) {
	            		// then I have a valid line
	            		String cmd = rxLine.substring(0, loc);
	            		Log.i(TAG, "Received: "+cmd);
	            		rxLine.delete(0,loc+1);
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

		@Override
		public void onDescriptorRead(BluetoothGatt gatt,
				BluetoothGattDescriptor descriptor, int status) {
			Log.e(TAG, "onDescriptorRead");
		}

		@Override
		public void onReadRemoteRssi(BluetoothGatt gatt, int rssi, int status) {
			Log.e(TAG, "onReadRemoteRssi");
		}
    	
	    private Runnable queueRunner = new Runnable() {
	        @Override
	        public void run() {
	            while (!mShutdown.get()) {
	                try {
	                	String s = queue.poll(100, TimeUnit.MILLISECONDS);
	                	if (s != null) {
		                    Log.i(TAG, "Sending: "+s);
		                    s = s + "\\n";
	
		                    final byte[] value = s.getBytes(Charset.forName("UTF-8"));
		                    
		                    // Split the value into chunks (UART service has a maximum number of characters that can be written )
		                    for (int i = 0; i < value.length; i += kTxMaxCharacters) {
		                        final byte[] chunk = Arrays.copyOfRange(value, i, Math.min(i + kTxMaxCharacters, value.length));
		                        
		                        if (mUartService != null) {
		                            if (BluetoothAdapter.getDefaultAdapter() == null || mGatt == null) {
		                                Log.w(TAG, "writeService: BluetoothAdapter not initialized");
		                                return;
		                            }
	
		                            mExecutor.write(mUartService, UUID_TX, chunk);
		                            mExecutor.execute(mGatt);
		                        }
		                    }
	                	}
	                	
	                    try {
	                        Thread.sleep(5);
	                    } catch (InterruptedException e) {
	                        e.printStackTrace();
	                    }
	                    
	                } catch (InterruptedException e) {
	                    e.printStackTrace();
	                }
	            }
	    		if (mShutdown.get() && (mGatt != null)) {
	    			mGatt.disconnect();
	    			mGatt.close();
	    		}
	    		Log.i(TAG, "queueRunner shutdown");
	        }
	        
	    };
	    
	    private int queueCnt = 0;
	    private Thread queueConsumer = new Thread(queueRunner, "BLEQueue"+(queueCnt++));

	    private void connect(final String address) {
	        Runnable runnable = new Runnable() {
				@Override
				public void run() {
			        mAdapter = BluetoothAdapter.getDefaultAdapter();
			        mDevice = mAdapter.getRemoteDevice(address);
			        if (mDevice == null) {
			        	Log.e(TAG, "BLE device unknown: "+address);
			        	return;
			        }
			        
			        mIsConnected.set(false);
			        mExecutor = BleGattExecutor.createExecutor(ConnectionMgr.this);
			        
			        while (!mShutdown.get()) {
			        	if (!mIsConnected.get()) {
			        		mGatt = mDevice.connectGatt(BluetoothPipeSrvc.this, false, mExecutor);
			        	}
			        	for (int i = 0; i < 50 && !mShutdown.get(); i++) {
				        	try {
								Thread.sleep(100);
							} catch (InterruptedException e) {
								// TODO Auto-generated catch block
								e.printStackTrace();
							}
			        	}
			        }
			        Log.i(TAG, "connection thread shutdown");

			        if (mGatt != null) {
		    			mGatt.disconnect();
		    			mGatt.close();
		    		}

			        mStopped.set(true);
				}
			};
	        Thread t = new Thread(runnable, "BLE");
	        t.start();
		}
		
    };
    
    // this is the state that needs to be maintained onPause
    private BlockingQueue<String> queue = new LinkedBlockingQueue<String>();
	
	public BluetoothPipeSrvc() {
		Log.i(TAG, "Service zero-argumnet constructor called");
	}
	
	public void onDestroy() {
		Log.e(TAG, "Service being destroyed");
		super.onDestroy();
	}
	
	@Override
	public void onCreate() {
		super.onCreate();
	}
	  
	// Device scan callback.
	private BluetoothAdapter.LeScanCallback mLeScanCallback = new BluetoothAdapter.LeScanCallback() {
	    @Override
	    public void onLeScan(final BluetoothDevice device, int rssi, byte[] scanRecord) {
	    	// I think this is completely wrong -- there should be some way to initialize everything 
	    	// sufficiently to just attempt connections to the known devices, but I haven't found it.  The scan 
	    	// appears to be the only thing that is doing all the initializations...  and I don't need to do anything
	    	// special as a result of this callback, either -- it's enough to let the scan run for a few seconds,
	    	// then do the connections
	   }
	};
	
	private static final long SCAN_PERIOD = 2000;  // stop scanning after 12 seconds
	private Handler mScanCanceller = new Handler();
    private void scanLeDevice() {
    	// Stops scanning after a pre-defined scan period.
        mScanCanceller.postDelayed(new Runnable() {
            @Override
            public void run() {
        		int num = NumHivesProperty.getNumHivesProperty(BluetoothPipeSrvc.this);
        		for (int i = 0; i < num; i++) {
        			String hiveId = PairedHiveProperty.getPairedHiveId(BluetoothPipeSrvc.this, i);

        			boolean found = false;
        			for (ConnectionMgr mgr : mMgrs) 
        				found |= mgr.getAddress().equals(hiveId);
        			if (!found) {
            			mAddresses.add(hiveId);
            			mMgrs.add(new ConnectionMgr(hiveId));
            			Log.i(TAG, "Address added: "+hiveId);
        			}
        		}
                BluetoothAdapter.getDefaultAdapter().stopLeScan(mLeScanCallback);
            }
        }, SCAN_PERIOD);

        BluetoothAdapter.getDefaultAdapter().startLeScan(mLeScanCallback);
    }
    


	@Override
	public int onStartCommand(Intent intent, int flags, int startId) {
    	mEnabled = EnableBridgeProperty.getEnableBridgeProperty(this);
    	if (mEnabled) {
			for (ConnectionMgr mgr : mMgrs) 
				mgr.setShutdown(true);
			mMgrs = new ArrayList<ConnectionMgr>();
			mAddresses = new ArrayList<String>();
			
    		scanLeDevice();
    	} else {
			for (ConnectionMgr mgr : mMgrs) 
				mgr.setShutdown(true);
			
			boolean allDone = false;
			while (!allDone) {
				allDone = true;
				for (ConnectionMgr mgr : mMgrs) {
					if (!mgr.isStopped())
						allDone = false;
				}
			}
			mMgrs = new ArrayList<ConnectionMgr>();
			mAddresses = new ArrayList<String>();
			Log.i(TAG, "Disabled");
    	}
    	
		updateNotification();
		
		// If we get killed, after returning from here, restart
		return START_STICKY;
	}

	@Override
	public IBinder onBind(Intent intent) {
		// We don't provide binding, so return null
		return null;
	}

	private void updateNotification() {
		NotificationManager notificationManager = (NotificationManager) getSystemService(NOTIFICATION_SERVICE); 

		if (mNotification != null) {
			notificationManager.cancel(0);
			mNotification = null;
		}
		
		Intent intent = new Intent(this, MainActivity.class);
		intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
		PendingIntent pIntent = PendingIntent.getActivity(this, (int) System.currentTimeMillis(), intent, 0);

		// build notification
		Notification.Builder builder = new Notification.Builder(this)
	          .setSmallIcon(R.drawable.ic_hive_status)
	          .setLargeIcon(Icon.createWithResource(this, R.drawable.ic_hive))
	          .setContentIntent(pIntent)
	          .setStyle(new Notification.BigTextStyle().bigText("Hi there"));
		
		if (mEnabled) {
			// build notification
			builder.setContentTitle("Ble2cld Pipe: enabled");
	
			boolean foundConnections = false;
			String hiveNames = "no connections";
			if (mMgrs.size() > 0) {
				for (int i = 0; i < mMgrs.size(); i++) {
					if (mMgrs.get(i).isConnected()) {
						boolean foundName = false;
						for (int j = 0; !foundName && (j < NumHivesProperty.getNumHivesProperty(this)); j++) {
							if (PairedHiveProperty.getPairedHiveId(this, j).equals(mMgrs.get(i).getAddress())) {
								foundName = true;
								String name = PairedHiveProperty.getPairedHiveName(this, j);
								if (!foundConnections) {
									hiveNames = "Connected: "+name;
									foundConnections = true;
								} else {
									hiveNames = hiveNames + ", " + name;
								}
							}
						}
					}
				}
			}
			builder.setContentText(hiveNames);
			
			mNotification = builder.build();
			mNotification.flags |= Notification.FLAG_NO_CLEAR;
			mNotification.flags |= Notification.FLAG_ONGOING_EVENT;
			mNotification.flags |= Notification.FLAG_LOCAL_ONLY;
		} else {
			builder.setContentTitle("Ble2cld Pipe: disabled");
			mNotification = builder.build();
			mNotification.flags |= Notification.FLAG_AUTO_CANCEL;
			mNotification.flags |= Notification.FLAG_LOCAL_ONLY;
		}
		
		notificationManager.notify(0, mNotification);
	}
	

	public static void startBlePipes(Context ctxt) {
		Intent mBle2cldIntent= new Intent(ctxt, BluetoothPipeSrvc.class);
		ctxt.startService(mBle2cldIntent);
	}
	
}
