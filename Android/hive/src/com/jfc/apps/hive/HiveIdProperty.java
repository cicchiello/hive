package com.jfc.apps.hive;

import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

import android.Manifest;
import android.app.Activity;
import android.app.AlertDialog;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.preference.PreferenceManager;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.ArrayAdapter;
import android.widget.ImageButton;
import android.widget.TextView;

import com.example.hive.R;
import com.jfc.misc.prop.IPropertyMgr;
import com.jfc.srvc.ble2cld.BluetoothPipeSrvc;
import com.jfc.util.misc.DialogUtils;
import com.jfc.util.misc.SplashyText;


public class HiveIdProperty implements IPropertyMgr {
	private static final String TAG = HiveIdProperty.class.getName();

	private static final String HIVE_ID_PROPERTY = "HIVE_ID_PROPERTY";
	private static final String DEFAULT_HIVE_ID = "<unknown>";
	
    private static final int PERMISSION_REQUEST_COARSE_LOCATION = 1;
	private static final long SCAN_PERIOD = 12000;  // stop scanning after 12 seconds

    static final int grayColor = HiveEnv.ModifiableBackgroundColor;
	
    // created on constructions -- no need to save on pause
	private TextView mIdTv;
	private Activity mActivity;

	// transient variables -- no need to save on pause
	private AlertDialog mAlert;
	private Handler mScanCanceller = new Handler();
	private boolean mScanning;
	private Map<String,String> mReadableToAddresses = null;
    private ArrayAdapter<String> mDiscoveredDevices = null; 
	private Runnable mOnPermissionGranted = null;

	// this may need to save state on pause
	private BluetoothPipeSrvc mPipe;
	

	public void setPipe(BluetoothPipeSrvc pipe) {
		mPipe = pipe;
	}
	
	public static boolean isHiveIdPropertyDefined(Activity activity) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(activity.getBaseContext());
		return SP.contains(HIVE_ID_PROPERTY) && 
			!SP.getString(HIVE_ID_PROPERTY, DEFAULT_HIVE_ID).equals(DEFAULT_HIVE_ID);
	}
	
	public static String getHiveIdProperty(Activity activity) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(activity.getBaseContext());
		String id = SP.getString(HIVE_ID_PROPERTY, DEFAULT_HIVE_ID);
		return id;
	}
	
	public static void resetHiveIdProperty(Activity activity) {
		setHiveIdProperty(activity, DEFAULT_HIVE_ID);
	}
	
	public HiveIdProperty(final Activity activity, final TextView idTv, ImageButton button) {
		this.mActivity = activity;
		this.mIdTv = idTv;
		
		final String id = getHiveIdProperty(activity);
    	if (id == null || id.equals(DEFAULT_HIVE_ID)) 
    		setHiveIdUndefined();
    	else 
    		displayHiveId(id);
    	
    	button.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
				onScanActivity();
			}
		});
	}

	public AlertDialog getAlertDialog() {return mAlert;}

	private void setHiveIdUndefined() {
		mIdTv.setText(DEFAULT_HIVE_ID);
		mIdTv.setBackgroundColor(0xffff0000); // RED
	}
	
	private void displayHiveId(String id) {
		mIdTv.setText(id);
		mIdTv.setBackgroundColor(grayColor);
	}
	
	private void setHiveId(String id) {
		setHiveIdProperty(mActivity, id);
		displayHiveId(id);
	}
	
	public static void setHiveIdProperty(Activity activity, String id) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(activity.getBaseContext());
		if (!SP.getString(HIVE_ID_PROPERTY, DEFAULT_HIVE_ID).equals(id)) {
			SharedPreferences.Editor editor = SP.edit();
			editor.putString(HIVE_ID_PROPERTY, id);
			editor.commit();
		}
	}

	static private final int REQUEST_ENABLE_BT = 3210;
	
	@Override
	public boolean onActivityResult(int requestCode, int resultCode, Intent intent) {
		if (resultCode == REQUEST_ENABLE_BT) {
    		selectFromBondedDevices();
			return true;
		}
		return false;
	}

	// Device scan callback.
	private BluetoothAdapter.LeScanCallback mLeScanCallback = new BluetoothAdapter.LeScanCallback() {
	    @Override
	    public void onLeScan(final BluetoothDevice device, int rssi,
	            byte[] scanRecord) {
	        mActivity.runOnUiThread(new Runnable() {
	           @Override
	           public void run() {
	        	   Log.i(TAG, device.getAddress());
	        	   if (!mReadableToAddresses.containsValue(device.getAddress())) {
	        		   String readable = device.getName() + "      " + device.getAddress();
		               mReadableToAddresses.put(readable, device.getAddress());
	        		   mDiscoveredDevices.add(readable);
		               mDiscoveredDevices.notifyDataSetChanged();
	        	   }
	           }
	       });
	   }
	};
	
    private void scanLeDevice(final boolean enable) {
        if (enable) {
            final Map<String,BluetoothDevice> m = new HashMap<String,BluetoothDevice>();
            mDiscoveredDevices = new ArrayAdapter<String>(mActivity, R.layout.list_target);
            mReadableToAddresses = new HashMap<String,String>();
            
            AlertDialog.Builder builder = new AlertDialog.Builder(mActivity);
            builder.setIcon(R.drawable.ic_hive);
            builder.setTitle(R.string.choose_hive);
            builder.setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
            	@Override
            	public void onClick(DialogInterface dialog, int which) {
            		mAlert.dismiss(); 
            		scanLeDevice(false); // stop scanning
            		mAlert = null;
            	}
            });
            builder.setAdapter(mDiscoveredDevices, new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int which) {
                	String selection = mDiscoveredDevices.getItem(which);
            		mAlert.dismiss();
            		String address = mReadableToAddresses.get(selection);
            		setHiveId(address);
            		SplashyText.highlightModifiedField(mActivity, mIdTv);
            		scanLeDevice(false); // stop scanning
            		mAlert = null;
            		
            		if (mPipe != null)
            			mPipe.addDevice(address);
                }
            });
            mAlert = builder.show();

        	// Stops scanning after a pre-defined scan period.
            mScanCanceller.postDelayed(new Runnable() {
                @Override
                public void run() {
                    mScanning = false;
                    BluetoothAdapter.getDefaultAdapter().stopLeScan(mLeScanCallback);
                }
            }, SCAN_PERIOD);

            mScanning = true;
            BluetoothAdapter.getDefaultAdapter().startLeScan(mLeScanCallback);
        } else {
            mScanning = false;
            BluetoothAdapter.getDefaultAdapter().stopLeScan(mLeScanCallback);
        }
    }
    
	public void onScanActivity() {
		requestLocationPermissionIfNeeded(new Runnable() {public void run() {scanLeDevice(true);}});
	}
	
	private void selectFromBondedDevices() {
        final ArrayAdapter<String> arrayAdapter = new ArrayAdapter<String>(mActivity, R.layout.list_target);
        final Map<String,BluetoothDevice> m = new HashMap<String,BluetoothDevice>();
        
    	// Loop through paired devices
        BluetoothAdapter mBluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
        Set<BluetoothDevice> pairedDevices = mBluetoothAdapter.getBondedDevices();
    	for (BluetoothDevice device : pairedDevices) {
    		// Add the name and address to an array adapter to show in a ListView
    		String label = device.getName()+" : "+device.getAddress();
    		arrayAdapter.add(label);
    		m.put(label, device);
    	}

        AlertDialog.Builder builder = new AlertDialog.Builder(mActivity);
        builder.setIcon(R.drawable.ic_hive);
        builder.setTitle(R.string.choose_hive);
        builder.setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
        	@Override
        	public void onClick(DialogInterface dialog, int which) {mAlert.dismiss(); mAlert = null;}
        });
        builder.setAdapter(arrayAdapter, new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
            	String selection = arrayAdapter.getItem(which);
        		mAlert.dismiss();
        		mAlert = null;
        		setHiveId(m.get(selection).getAddress());
        		SplashyText.highlightModifiedField(mActivity, mIdTv);
        		
        		if (mPipe != null)
        			mPipe.addDevice(m.get(selection).getAddress());
            }
        });
        mAlert = builder.show();
	}

	@Override
	public void onPermissionResult(int requestCode, String[] permissions, int[] grantResults) {
        switch (requestCode) {
        case PERMISSION_REQUEST_COARSE_LOCATION: {
            // If request is cancelled, the result arrays are empty.
            if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
            	Log.i(TAG, "Permissions granted");
            	mOnPermissionGranted.run();
            } else{
            	Log.e(TAG, "Permissions NOT granted");
            }
            return;
        }
        }
	}

	private void requestLocationPermissionIfNeeded(final Runnable onDismiss) {
	    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
	        // Android M Permission check
	        if (mActivity.checkSelfPermission(Manifest.permission.ACCESS_COARSE_LOCATION) != PackageManager.PERMISSION_GRANTED) {
	            final AlertDialog.Builder builder = new AlertDialog.Builder(mActivity);
	            builder.setTitle("This app needs location access");
	            builder.setMessage("Please grant location access so this app can scan for Bluetooth peripherals");
	            builder.setPositiveButton(android.R.string.ok, null);
                mOnPermissionGranted = onDismiss;
	            builder.setOnDismissListener(new DialogInterface.OnDismissListener() {
	                public void onDismiss(DialogInterface dialog) {
	                    mActivity.requestPermissions(new String[]{Manifest.permission.ACCESS_COARSE_LOCATION}, PERMISSION_REQUEST_COARSE_LOCATION);
	                }
	            });
	            mAlert = builder.show();
	        } else {
	        	onDismiss.run();
	        }
	    } else {
	    	onDismiss.run();
	    }
	}

	@Override
    public void onRestoreInstanceState(Bundle savedInstanceState) {
		mPipe.onRestoreInstanceState(savedInstanceState);
	}
	
	@Override
    public void onSaveInstanceState(Bundle savedInstanceState) {
		mPipe.onSaveInstanceState(savedInstanceState);
	}

}
