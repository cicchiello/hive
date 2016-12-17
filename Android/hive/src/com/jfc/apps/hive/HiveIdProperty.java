package com.jfc.apps.hive;

import java.util.HashMap;
import java.util.Map;
import java.util.Set;

import android.Manifest;
import android.app.Activity;
import android.app.AlertDialog;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.os.Build;
import android.preference.PreferenceManager;
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
	private static final String TAG = MCUTempProperty.class.getName();

	private static final String HIVE_ID_PROPERTY = "HIVE_ID_PROPERTY";
	private static final String DEFAULT_HIVE_ID = "<unknown>";
	
    private static final int PERMISSION_REQUEST_FINE_LOCATION = 1;

    static final int grayColor = HiveEnv.ModifiableBackgroundColor;
	
	private AlertDialog alert;
	private Activity activity;
	private TextView idTv;
	private BluetoothPipeSrvc mPipe;
	
	public void setPipe(BluetoothPipeSrvc pipe) {mPipe = pipe;}
	
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
		this.activity = activity;
		this.idTv = idTv;
		
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

	public AlertDialog getAlertDialog() {return alert;}

	private void setHiveIdUndefined() {
		idTv.setText(DEFAULT_HIVE_ID);
		idTv.setBackgroundColor(0xffff0000); // RED
	}
	
	private void displayHiveId(String id) {
		idTv.setText(id);
		idTv.setBackgroundColor(grayColor);
	}
	
	private void setHiveId(String id) {
		setHiveIdProperty(activity, id);
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
	
	public boolean onActivityResult(Activity activity, int requestCode, int resultCode, Intent intent) {
		if (resultCode == REQUEST_ENABLE_BT) {
    		selectFromBondedDevices();
			return true;
		}
		return false;
	}

	public void onScanActivity() {
        BluetoothAdapter mBluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
        if (mBluetoothAdapter == null) {
            // Device does not support Bluetooth
	    	Runnable cancelAction = new Runnable() {
	    		@Override
	    		public void run() {alert = null;}
	    	};
	    	alert = DialogUtils.createAndShowAlertDialog(activity, 
	    												 R.string.no_bluetooth, 
	    												 0, null, 
	    												 android.R.string.cancel, cancelAction);
        } else {
        	// get the list of bonded bluetooth devices -- first need to see if bluetooth is enabled
    		if (!BluetoothAdapter.getDefaultAdapter().isEnabled()) {
    		    Intent enableBtIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
    		    activity.startActivityForResult(enableBtIntent, REQUEST_ENABLE_BT);
    		}
        	
    		selectFromBondedDevices();
        }
	}
	
	private void selectFromBondedDevices() {
        final ArrayAdapter<String> arrayAdapter = new ArrayAdapter<String>(activity, R.layout.list_target);
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

        AlertDialog.Builder builder = new AlertDialog.Builder(activity);
        builder.setIcon(R.drawable.ic_hive);
        builder.setTitle(R.string.choose_hive);
        builder.setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
        	@Override
        	public void onClick(DialogInterface dialog, int which) {alert.dismiss(); alert = null;}
        });
        builder.setAdapter(arrayAdapter, new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
            	String selection = arrayAdapter.getItem(which);
        		alert.dismiss();
        		alert = null;
        		setHiveId(m.get(selection).getAddress());
        		SplashyText.highlightModifiedField(activity, idTv);
        		
        		if (mPipe != null)
        			mPipe.addDevice(m.get(selection).getAddress());
            }
        });
        alert = builder.show();
	}

	private void requestLocationPermissionIfNeeded() {
	    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
	        // Android M Permission check
	        if (activity.checkSelfPermission(Manifest.permission.ACCESS_FINE_LOCATION) != PackageManager.PERMISSION_GRANTED) {
	            final AlertDialog.Builder builder = new AlertDialog.Builder(activity);
	            builder.setTitle("This app needs location access");
	            builder.setMessage("Please grant location access so this app can scan for Bluetooth peripherals");
	            builder.setPositiveButton(android.R.string.ok, null);
	            builder.setOnDismissListener(new DialogInterface.OnDismissListener() {
	                public void onDismiss(DialogInterface dialog) {
	                    activity.requestPermissions(new String[]{Manifest.permission.ACCESS_FINE_LOCATION}, PERMISSION_REQUEST_FINE_LOCATION);
	                }
	            });
	            builder.show();
	        }
	    }
	}

}
